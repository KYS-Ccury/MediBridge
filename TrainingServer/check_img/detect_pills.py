#!/usr/bin/env python3
"""MediBridge POC — 다중 알약 사진 1장 → 검출 + 각인 OCR + 색·모양.

사용 예:
    python detect_pills.py samples/two_pills.jpg
    python detect_pills.py samples/  --out results/

본 스크립트는 "알약 식별 파이프라인이 가능한가" 만 검증.
실 운영 단계엔 YOLO fine-tuning + 식약처 DB 매칭이 더해진다.
"""

from __future__ import annotations
import argparse
import json
import os
import sys
from pathlib import Path
from typing import List

import cv2

# 본 패키지 import
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from lib.detection import detect_pills, annotate, crop_pill, set_debug_output, PillBox
from lib.ocr       import recognize_engraving, join_lines
from lib.color     import analyze_color
from lib.shape     import analyze_shape


# 지원 이미지 확장자
IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".tif", ".tiff"}


def process_one(image_path: Path, out_dir: Path,
                verbose: bool = True, debug: bool = False) -> dict:
    """이미지 1장 처리 — 검출 + 각 알약 OCR/색/모양.

    Returns:
        결과 dict (JSON 직렬화 가능)
    """
    image = cv2.imread(str(image_path))
    if image is None:
        return {
            "image": str(image_path),
            "error": "imread_failed",
        }

    if debug:
        set_debug_output(str(out_dir), image_path.stem)
    pills = detect_pills(image, debug=debug)
    if verbose:
        print(f"[{image_path.name}] 검출 알약 수: {len(pills)}")

    pill_results = []
    for p in pills:
        crop = crop_pill(image, p, pad=8)
        # crop 시작 좌표 (color 분석용 contour offset)
        crop_x = max(0, p.bbox[0] - 8)
        crop_y = max(0, p.bbox[1] - 8)

        # 1) 각인 OCR
        ocr_lines = recognize_engraving(crop)
        engraving_text = join_lines(ocr_lines)

        # 2) 색 — contour 로 알약 본체만 마스킹 (배경 제외)
        color_res = analyze_color(crop, contour=p.contour,
                                  crop_offset=(crop_x, crop_y))

        # 3) 모양
        shape_res = analyze_shape(p.contour)

        if verbose:
            print(f"  #{p.index:>2}  bbox={p.bbox}"
                  f"  색={color_res.label}({color_res.confidence:.2f})"
                  f"  모양={shape_res.label}(circ {shape_res.circularity:.2f},"
                  f" elong {shape_res.elongation:.2f})"
                  f"  각인=[{engraving_text or '—'}]")

        pill_results.append({
            "index":   p.index,
            "bbox":    list(p.bbox),
            "center":  list(p.center),
            "area":    p.area,
            "color":   {
                "label":      color_res.label,
                "confidence": color_res.confidence,
                "hsv_mean":   list(color_res.hsv_mean),
            },
            "shape":   {
                "label":        shape_res.label,
                "circularity":  shape_res.circularity,
                "aspect_ratio": shape_res.aspect_ratio,
                "elongation":   shape_res.elongation,
            },
            "ocr":     [
                {"text": l.text, "confidence": l.confidence} for l in ocr_lines
            ],
        })

        # crop 도 결과 폴더에 저장 (디버깅용)
        crop_path = out_dir / f"{image_path.stem}_pill{p.index:02d}.png"
        cv2.imwrite(str(crop_path), crop)

    # annotated 이미지 저장
    annotated = annotate(image, pills)
    out_img_path = out_dir / f"{image_path.stem}_annotated.jpg"
    cv2.imwrite(str(out_img_path), annotated)

    result = {
        "image":            str(image_path),
        "annotated_image":  str(out_img_path),
        "detected_count":   len(pills),
        "pills":            pill_results,
    }

    # JSON 저장
    json_path = out_dir / f"{image_path.stem}_result.json"
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=2)

    if verbose:
        print(f"  → 저장: {out_img_path.name}, {json_path.name}")
    return result


def collect_images(target: Path) -> List[Path]:
    if target.is_file():
        return [target] if target.suffix.lower() in IMAGE_EXTS else []
    if target.is_dir():
        return sorted([p for p in target.iterdir()
                       if p.is_file() and p.suffix.lower() in IMAGE_EXTS])
    return []


def main():
    parser = argparse.ArgumentParser(description="MediBridge POC 알약 식별")
    parser.add_argument("input", help="이미지 파일 또는 디렉토리")
    parser.add_argument("--out", default=str(HERE / "results"),
                        help="결과 저장 디렉토리 (기본: results/)")
    parser.add_argument("-q", "--quiet", action="store_true",
                        help="상세 출력 끄기")
    parser.add_argument("--debug", action="store_true",
                        help="검출 디버그 마스크 저장 (*_dbg_thresh_*.png, *_dbg_contours.png)")
    args = parser.parse_args()

    in_path  = Path(args.input).resolve()
    out_path = Path(args.out).resolve()
    out_path.mkdir(parents=True, exist_ok=True)

    images = collect_images(in_path)
    if not images:
        print(f"이미지 없음: {in_path}", file=sys.stderr)
        sys.exit(1)

    summary = []
    for img in images:
        try:
            r = process_one(img, out_path,
                            verbose=not args.quiet,
                            debug=args.debug)
            summary.append(r)
        except Exception as e:
            print(f"[ERROR] {img.name}: {e}", file=sys.stderr)
            summary.append({"image": str(img), "error": str(e)})

    # 일괄 요약
    summary_path = out_path / "_summary.json"
    with open(summary_path, "w", encoding="utf-8") as f:
        json.dump(summary, f, ensure_ascii=False, indent=2)

    total_pills = sum(s.get("detected_count", 0) for s in summary)
    print(f"\n=== 전체 처리 ===")
    print(f"이미지 수: {len(summary)}")
    print(f"총 검출 알약 수: {total_pills}")
    print(f"요약: {summary_path}")


if __name__ == "__main__":
    main()
