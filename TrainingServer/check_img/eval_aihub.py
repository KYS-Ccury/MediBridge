#!/usr/bin/env python3
"""MediBridge POC — AI Hub 경구약제 데이터셋으로 정량 평가.

본 스크립트는 AI Hub COCO-like JSON 어노테이션을 정답(GT)으로,
detect_pills.py 가 사용하는 lib/* 의 결과와 비교하여 다음 메트릭을 산출:

  검출 (Detection)
    • IoU ≥ 0.5 / 0.7 / 0.8 / 0.9 매칭 비율
    • 검출 알약 수 일치 비율 (count match)

  각인 OCR (PaddleOCR)
    • exact match (정규화 후 완전 일치)
    • substring match (한쪽이 다른 쪽 포함)

  색 (HSV → 식약처 카테고리)
    • color_class1 일치 비율

  모양 (윤곽선 → 식약처 카테고리)
    • drug_shape 일치 비율

사용 예:
    # 기본 — JSON + 이미지 폴더 지정
    python eval_aihub.py --json /path/to/aihub.json --images /path/to/images/

    # 빠른 검증 — 처음 50장만
    python eval_aihub.py --json ... --images ... --limit 50

    # 결과 저장 폴더 변경
    python eval_aihub.py --json ... --images ... --out my_eval/
"""

from __future__ import annotations
import argparse
import json
import sys
import time
from collections import Counter
from pathlib import Path
from typing import Dict, List, Tuple

import cv2

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from lib.detection    import detect_pills, crop_pill, PillBox
from lib.ocr          import recognize_engraving, join_lines
from lib.color        import analyze_color
from lib.shape        import analyze_shape
from lib.aihub_loader import (
    load_aihub_json, AiHubImage,
    best_iou_match, engraving_matches, label_match, normalize_engraving,
)


# 합격선 임계값 (가이드라인 §4.1·§4.2 IoU 0.8 기준 + 본 POC 보조 임계)
IOU_BUCKETS = [0.5, 0.7, 0.8, 0.9]


def find_image_path(images_dir: Path, file_name: str) -> Path | None:
    """AI Hub 이미지 폴더에서 file_name 으로 실제 파일 위치 탐색.

    AI Hub 데이터셋은 약별 하위 폴더로 분리되어 있을 수 있어 재귀 탐색.
    file_name 이 절대/상대 경로를 포함하면 그대로 시도.
    """
    candidate = images_dir / file_name
    if candidate.exists():
        return candidate
    # 파일명만으로 재귀 탐색 — 캐시 만들면 더 빠르나 본 단계 단순화
    for p in images_dir.rglob(Path(file_name).name):
        if p.is_file():
            return p
    return None


def evaluate_one(image_path: Path, gt: AiHubImage, out_dir: Path,
                 verbose: bool = True) -> dict:
    """이미지 1장 평가 → 메트릭 dict."""
    image = cv2.imread(str(image_path))
    if image is None:
        return {"file_name": gt.file_name, "error": "imread_failed"}

    pills = detect_pills(image)
    pred_boxes = [(float(p.bbox[0]), float(p.bbox[1]),
                   float(p.bbox[2]), float(p.bbox[3])) for p in pills]
    gt_boxes = [b.bbox for b in gt.boxes]

    # 검출 매칭 — 가장 낮은 임계 IoU 0.5 로 매칭한 다음 모든 임계에서 평가
    matches = best_iou_match(pred_boxes, gt_boxes, iou_threshold=0.5)

    iou_counts = {f"iou_{int(t*100):d}": 0 for t in IOU_BUCKETS}
    matched_pairs: List[Tuple[int, int, float]] = []
    for gi, pi, v in matches:
        if pi < 0:
            continue
        matched_pairs.append((gi, pi, v))
        for t in IOU_BUCKETS:
            if v >= t:
                iou_counts[f"iou_{int(t*100):d}"] += 1

    # OCR / 색 / 모양 — GT 가 단일 약 메타만 가짐 (단일 사진 가정).
    # 조합 사진(여러 박스)이라도 색·모양은 보통 박스별로 다른 약이라
    # GT 메타와 직접 매칭은 단일 사진에서만 정확. 본 평가는 단일 사진 가정 후
    # 다중 사진은 OCR/색/모양 매칭을 첫 박스에 한해서만 수행.
    ocr_exact = ocr_substr = ocr_total = 0
    color_match = color_total = 0
    shape_match = shape_total = 0
    pill_records: List[dict] = []

    for gi, pi, v in matched_pairs:
        # 단일 약 사진은 GT 메타 1개를 모든 박스에 적용 가능.
        # 조합 사진은 GT 메타가 어느 박스에 해당하는지 모호 — 첫 매칭에만 측정.
        eligible_for_meta = (len(gt_boxes) == 1) or (gi == 0)
        pred_pill = pills[pi]
        crop = crop_pill(image, pred_pill, pad=8)
        crop_x = max(0, pred_pill.bbox[0] - 8)
        crop_y = max(0, pred_pill.bbox[1] - 8)

        # OCR
        ocr_lines = recognize_engraving(crop)
        ocr_text = " ".join(l.text for l in ocr_lines)

        ocr_e = ocr_s = False
        color_ok = shape_ok = False
        color_label = analyze_color(crop, contour=pred_pill.contour,
                                    crop_offset=(crop_x, crop_y))
        shape_label = analyze_shape(pred_pill.contour)

        if eligible_for_meta:
            # OCR
            ocr_e, ocr_s = engraving_matches(ocr_text, gt.print_front)
            if gt.print_front:
                ocr_total += 1
                if ocr_e:    ocr_exact  += 1
                if ocr_s:    ocr_substr += 1
            # 색
            if gt.color_class1:
                color_total += 1
                color_ok = label_match(color_label.label, gt.color_class1)
                if color_ok: color_match += 1
            # 모양
            if gt.drug_shape:
                shape_total += 1
                shape_ok = label_match(shape_label.label, gt.drug_shape)
                if shape_ok: shape_match += 1

        pill_records.append({
            "gt_index":      gi,
            "pred_index":    pi,
            "iou":           v,
            "pred_bbox":     [int(x) for x in pred_pill.bbox],
            "gt_bbox":       [int(x) for x in gt_boxes[gi]],
            "ocr":           ocr_text,
            "ocr_gt":        gt.print_front if eligible_for_meta else None,
            "ocr_exact":     ocr_e,
            "ocr_substring": ocr_s,
            "color_pred":    color_label.label,
            "color_gt":      gt.color_class1 if eligible_for_meta else None,
            "color_match":   color_ok,
            "shape_pred":    shape_label.label,
            "shape_gt":      gt.drug_shape if eligible_for_meta else None,
            "shape_match":   shape_ok,
        })

    record = {
        "file_name":        gt.file_name,
        "drug_name":        gt.drug_name,
        "gt_box_count":     len(gt_boxes),
        "pred_box_count":   len(pred_boxes),
        "matched":          len(matched_pairs),
        **iou_counts,
        "ocr_total":        ocr_total,
        "ocr_exact":        ocr_exact,
        "ocr_substring":    ocr_substr,
        "color_total":      color_total,
        "color_match":      color_match,
        "shape_total":      shape_total,
        "shape_match":      shape_match,
        "pills":            pill_records,
    }

    if verbose:
        print(f"  {gt.file_name}: GT={len(gt_boxes)} pred={len(pred_boxes)} "
              f"matched={len(matched_pairs)} "
              f"iou80={iou_counts['iou_80']} "
              f"ocr={ocr_substr}/{ocr_total} "
              f"color={color_match}/{color_total} "
              f"shape={shape_match}/{shape_total}")
    return record


def aggregate(records: List[dict]) -> dict:
    """전체 이미지 결과 합산 → 비율."""
    n = len(records)
    if n == 0:
        return {"image_count": 0}

    sum_field = lambda key: sum(r.get(key, 0) for r in records)

    gt_total   = sum_field("gt_box_count")
    pred_total = sum_field("pred_box_count")
    matched    = sum_field("matched")

    iou_buckets = {}
    for t in IOU_BUCKETS:
        key = f"iou_{int(t*100):d}"
        iou_buckets[key] = sum_field(key)

    ocr_total       = sum_field("ocr_total")
    ocr_exact       = sum_field("ocr_exact")
    ocr_substring   = sum_field("ocr_substring")
    color_total     = sum_field("color_total")
    color_match     = sum_field("color_match")
    shape_total     = sum_field("shape_total")
    shape_match     = sum_field("shape_match")

    def safe_ratio(num, den):
        return (num / den) if den > 0 else None

    # count match — GT 박스 수와 pred 박스 수가 같은 이미지 비율
    count_matches = sum(1 for r in records
                        if r.get("gt_box_count") == r.get("pred_box_count"))

    return {
        "image_count":               n,
        "gt_box_total":              gt_total,
        "pred_box_total":            pred_total,
        "count_match_image_ratio":   safe_ratio(count_matches, n),
        "matched_box_total":         matched,
        # 검출 — GT 박스 대비 매칭 비율
        "iou_50_ratio":              safe_ratio(iou_buckets["iou_50"], gt_total),
        "iou_70_ratio":              safe_ratio(iou_buckets["iou_70"], gt_total),
        "iou_80_ratio":              safe_ratio(iou_buckets["iou_80"], gt_total),
        "iou_90_ratio":              safe_ratio(iou_buckets["iou_90"], gt_total),
        # OCR
        "ocr_evaluated":             ocr_total,
        "ocr_exact_ratio":           safe_ratio(ocr_exact,     ocr_total),
        "ocr_substring_ratio":       safe_ratio(ocr_substring, ocr_total),
        # 색·모양
        "color_evaluated":           color_total,
        "color_match_ratio":         safe_ratio(color_match,   color_total),
        "shape_evaluated":           shape_total,
        "shape_match_ratio":         safe_ratio(shape_match,   shape_total),
    }


def fmt_pct(v) -> str:
    if v is None: return "  n/a"
    return f"{v*100:5.1f}%"


def print_summary(agg: dict):
    print("\n" + "=" * 60)
    print(f"AI Hub 평가 결과 — 이미지 {agg['image_count']}장 / GT 박스 {agg['gt_box_total']}개")
    print("=" * 60)
    print(f"  검출 IoU≥0.5  : {fmt_pct(agg['iou_50_ratio'])}  ({agg['iou_50_ratio'] and round(agg['iou_50_ratio']*agg['gt_box_total'])} / {agg['gt_box_total']})")
    print(f"  검출 IoU≥0.7  : {fmt_pct(agg['iou_70_ratio'])}")
    print(f"  검출 IoU≥0.8  : {fmt_pct(agg['iou_80_ratio'])}    ← 가이드라인 합격선")
    print(f"  검출 IoU≥0.9  : {fmt_pct(agg['iou_90_ratio'])}")
    print(f"  박스 수 일치  : {fmt_pct(agg['count_match_image_ratio'])} (이미지 단위)")
    print()
    print(f"  OCR exact     : {fmt_pct(agg['ocr_exact_ratio'])}     (대상 {agg['ocr_evaluated']})")
    print(f"  OCR substring : {fmt_pct(agg['ocr_substring_ratio'])}")
    print(f"  색 일치       : {fmt_pct(agg['color_match_ratio'])}   (대상 {agg['color_evaluated']})")
    print(f"  모양 일치     : {fmt_pct(agg['shape_match_ratio'])}   (대상 {agg['shape_evaluated']})")
    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(description="MediBridge POC — AI Hub 정량 평가")
    parser.add_argument("--json",   required=True, help="AI Hub COCO-like JSON 파일 경로")
    parser.add_argument("--images", required=True, help="이미지 폴더 (재귀 탐색)")
    parser.add_argument("--out",    default=str(HERE / "eval_results"),
                        help="결과 저장 폴더 (기본: eval_results/)")
    parser.add_argument("--limit",  type=int, default=0,
                        help="처음 N장만 평가 (빠른 검증). 0=전체")
    parser.add_argument("-q", "--quiet", action="store_true",
                        help="이미지별 상세 출력 끄기")
    args = parser.parse_args()

    json_path   = Path(args.json).resolve()
    images_dir  = Path(args.images).resolve()
    out_dir     = Path(args.out).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if not json_path.exists():
        print(f"ERROR: JSON 파일 없음 — {json_path}", file=sys.stderr); sys.exit(1)
    if not images_dir.exists():
        print(f"ERROR: 이미지 폴더 없음 — {images_dir}", file=sys.stderr); sys.exit(1)

    print(f"[eval] JSON  : {json_path}")
    print(f"[eval] 이미지: {images_dir}")
    print(f"[eval] JSON 로딩...")
    images, categories = load_aihub_json(json_path)
    print(f"[eval] 이미지 메타 {len(images)}개, 카테고리 {len(categories)}개")

    items = list(images.values())
    if args.limit > 0:
        items = items[: args.limit]
    print(f"[eval] 평가 대상 {len(items)}장 시작...")

    t0 = time.time()
    records: List[dict] = []
    skipped = 0
    for i, gt in enumerate(items):
        path = find_image_path(images_dir, gt.file_name)
        if not path:
            skipped += 1
            if not args.quiet:
                print(f"  [skip] 파일 없음: {gt.file_name}")
            continue
        try:
            r = evaluate_one(path, gt, out_dir, verbose=not args.quiet)
            records.append(r)
        except Exception as e:
            print(f"  [err] {gt.file_name}: {e}", file=sys.stderr)

        if (i + 1) % 50 == 0:
            elapsed = time.time() - t0
            print(f"  ... {i + 1}/{len(items)} 처리됨 (경과 {elapsed:.1f}s)")

    elapsed = time.time() - t0

    agg = aggregate(records)
    agg["elapsed_seconds"]    = elapsed
    agg["skipped_no_file"]    = skipped
    agg["json_path"]          = str(json_path)
    agg["images_dir"]         = str(images_dir)

    # 저장
    detail_path = out_dir / "_detail.json"
    summary_path = out_dir / "_summary.json"
    with open(detail_path, "w", encoding="utf-8") as f:
        json.dump(records, f, ensure_ascii=False, indent=2)
    with open(summary_path, "w", encoding="utf-8") as f:
        json.dump(agg, f, ensure_ascii=False, indent=2)

    print_summary(agg)
    print(f"\n파일 누락 (skip): {skipped}")
    print(f"경과: {elapsed:.1f}s")
    print(f"세부 결과: {detail_path}")
    print(f"요약:      {summary_path}")


if __name__ == "__main__":
    main()
