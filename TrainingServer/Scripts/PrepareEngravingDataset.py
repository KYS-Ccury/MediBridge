"""
PrepareEngravingDataset — AIHub 경구약제 → PaddleOCR rec 학습 데이터셋

AIHub 라벨 JSON(images[].print_front + annotations[].bbox)을 읽어
알약 본체를 crop, 각인 문자열을 라벨로 PaddleOCR 인식(rec) 학습
포맷(`상대경로\t라벨`)으로 변환한다.

⭐ 핵심: 도메인 갭 보정.
AIHub 는 스튜디오 고해상(알약 ~200~340px). 실제 추론 입력은 폰
화면 스크린샷에서 잘린 ~80px 저해상. 그대로 학습하면 현장 전이가
안 됨 → 각 crop 에 '현장열화' augmentation(다운스케일→블러→JPEG
열화→재확대) 변형을 함께 생성해 저해상 각인도 읽도록 학습.

사용 (Vision PC):
  python PrepareEngravingDataset.py \
     --src  /.../DATA/01.데이터/2.Validation \
     --out  /.../DATA/_ocr_rec_dataset \
     --aug  3            # crop 당 현장열화 변형 수
"""
from __future__ import annotations
import argparse
import json
import os
import random
import sys
from pathlib import Path

import cv2
import numpy as np


# AIHub print_front 에 섞이는 '주석자 설명어'(실제 각인 아님).
#  - '분할선' : 글자 사이 분할선 존재 표시  → 공백으로 치환
#  - 단독 설명어(마크/선/할선/없음 등) → 학습 제외
_SEP_WORDS = ("분할선", "분할", "마크")
_DESC_ONLY = {"마크", "선", "할선", "없음", "분할선", "분할", "기타"}


def normalize_label(text: str) -> str:
    """각인 라벨 정규화.

    1) 주석 설명어('분할선' 등) 공백 치환
    2) 영숫자/한글/공백만 남기고 대문자화, 다중공백 축약
    3) 결과가 비었거나 순수 설명어면 '' 반환(학습 제외)
    """
    if not text:
        return ""
    t = text
    for w in _SEP_WORDS:
        t = t.replace(w, " ")
    out = []
    for ch in t:
        if ch.isalnum() or ('가' <= ch <= '힣') or ch == ' ':
            out.append(ch.upper())
    norm = " ".join("".join(out).split())
    if not norm:
        return ""
    if norm.replace(" ", "") in {d.upper() for d in _DESC_ONLY}:
        return ""
    return norm


def field_degrade(crop: np.ndarray, rng: random.Random) -> np.ndarray:
    """스튜디오 crop → 폰 스크린샷 ~80px 수준으로 열화 모사."""
    h, w = crop.shape[:2]
    # 1) 무작위 소형 해상도로 다운스케일 (현장 crop ≈ 60~120px)
    target = rng.randint(60, 120)
    scale = target / max(h, w)
    small = cv2.resize(crop, (max(1, int(w * scale)), max(1, int(h * scale))),
                       interpolation=cv2.INTER_AREA)
    # 2) 가우시안 블러 (초점/움직임 흐림)
    if rng.random() < 0.7:
        k = rng.choice([3, 3, 5])
        small = cv2.GaussianBlur(small, (k, k), 0)
    # 3) JPEG 압축 열화
    q = rng.randint(35, 75)
    ok, enc = cv2.imencode(".jpg", small,
                           [int(cv2.IMWRITE_JPEG_QUALITY), q])
    if ok:
        small = cv2.imdecode(enc, cv2.IMREAD_COLOR)
    # 4) 약한 밝기/색 캐스트 (조명 변동·쿨 캐스트 모사)
    if rng.random() < 0.6:
        b = rng.uniform(0.8, 1.2)
        cast = np.array([rng.uniform(0.92, 1.12),   # B
                         rng.uniform(0.95, 1.05),   # G
                         rng.uniform(0.88, 1.08)],  # R
                        dtype=np.float32)
        small = np.clip(small.astype(np.float32) * b * cast,
                        0, 255).astype(np.uint8)
    # 5) 원 크기로 재확대 (추론 파이프라인이 업스케일하는 것과 동일 조건)
    return cv2.resize(small, (w, h), interpolation=cv2.INTER_CUBIC)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", required=True,
                    help="AIHub split 루트 (라벨링데이터/원천데이터 상위)")
    ap.add_argument("--out", required=True, help="출력 데이터셋 루트")
    ap.add_argument("--aug", type=int, default=3,
                    help="crop 당 현장열화 변형 수 (0=clean만)")
    ap.add_argument("--val-ratio", type=float, default=0.08)
    ap.add_argument("--split-mode", choices=["image", "drug"],
                    default="image",
                    help="image=같은 약의 held-out 사진(각인 학습능력 평가, "
                         "약 종수 적을 때 권장) / drug=미학습 약(일반화 평가)")
    ap.add_argument("--max-drugs", type=int, default=0,
                    help=">0 이면 약 코드 수 제한(스모크 테스트용)")
    ap.add_argument("--seed", type=int, default=42)
    args = ap.parse_args()

    rng = random.Random(args.seed)
    src = Path(args.src)
    out = Path(args.out)
    img_dir = out / "images"
    img_dir.mkdir(parents=True, exist_ok=True)

    # 단일경구약제만 (조합 사진은 단일 bbox↔print 매핑 모호 → 제외)
    label_root = src / "라벨링데이터" / "단일경구약제_5000종"
    src_root = src / "원천데이터" / "단일경구약제_5000종"
    json_files = sorted(label_root.rglob("*.json"))
    print(f"[prep] 단일경구약제 JSON {len(json_files)}개 발견")

    drug_codes = sorted({p.stem.split("_")[0] for p in json_files})
    if args.max_drugs > 0:
        rng.shuffle(drug_codes)
        drug_codes = drug_codes[:args.max_drugs]
    code_set = set(drug_codes)
    # drug 모드: 일부 약 자체를 val(미학습 약 → 일반화 평가)
    val_codes: set[str] = set()
    if args.split_mode == "drug":
        sc = sorted(drug_codes)
        rng.shuffle(sc)
        n_val = max(1, int(len(sc) * args.val_ratio))
        val_codes = set(sc[:n_val])
    print(f"[prep] 약 코드 {len(drug_codes)} / split={args.split_mode}"
          + (f" (val 약 {len(val_codes)})" if val_codes else ""))

    def is_val_record(code: str, stem: str) -> bool:
        if args.split_mode == "drug":
            return code in val_codes
        # image 모드: 같은 약의 사진 일부를 결정적 해시로 val 분배
        import hashlib
        h = int(hashlib.md5(stem.encode()).hexdigest(), 16) % 1000
        return h < int(args.val_ratio * 1000)

    train_lines, val_lines = [], []
    charset: set[str] = set()
    n_ok = n_skip = 0

    for ji, jp in enumerate(json_files):
        code = jp.stem.split("_")[0]
        if code not in code_set:
            continue
        try:
            d = json.load(open(jp, encoding="utf-8"))
        except Exception:
            n_skip += 1
            continue
        imgs = d.get("images", [])
        anns = d.get("annotations", [])
        if not imgs or not anns:
            n_skip += 1
            continue
        meta = imgs[0]
        label = normalize_label(meta.get("print_front") or "")
        if not label:                       # 각인 없는 약 → rec 학습 제외
            n_skip += 1
            continue
        fn = meta.get("file_name") or meta.get("imgfile")
        if not fn:
            n_skip += 1
            continue
        # 이미지 경로: 원천데이터/단일경구약제_5000종/<code>/<fn>
        ip = src_root / code / fn
        if not ip.exists():
            cand = list(src_root.rglob(fn))
            if not cand:
                n_skip += 1
                continue
            ip = cand[0]
        img = cv2.imread(str(ip))
        if img is None:
            n_skip += 1
            continue
        x, y, w, h = [int(v) for v in anns[0]["bbox"]]
        H, W = img.shape[:2]
        # 8% 패딩 + 클리핑
        px, py = int(w * 0.08), int(h * 0.08)
        x0, y0 = max(0, x - px), max(0, y - py)
        x1, y1 = min(W, x + w + px), min(H, y + h + py)
        if x1 - x0 < 12 or y1 - y0 < 12:
            n_skip += 1
            continue
        crop = img[y0:y1, x0:x1]

        is_val = is_val_record(code, jp.stem)
        variants = [("clean", crop)]
        if not is_val:                      # 학습셋만 열화 증강
            for a in range(args.aug):
                variants.append((f"deg{a}", field_degrade(crop, rng)))

        for tag, vimg in variants:
            name = f"{jp.stem}_{tag}.jpg"
            cv2.imwrite(str(img_dir / name), vimg,
                        [int(cv2.IMWRITE_JPEG_QUALITY), 92])
            line = f"images/{name}\t{label}"
            (val_lines if is_val else train_lines).append(line)
            charset.update(label)
        n_ok += 1
        if (ji + 1) % 2000 == 0:
            print(f"[prep] {ji+1}/{len(json_files)} ok={n_ok} skip={n_skip}")

    # 라벨 파일 + 문자 사전
    (out / "train_list.txt").write_text("\n".join(train_lines),
                                        encoding="utf-8")
    (out / "val_list.txt").write_text("\n".join(val_lines),
                                       encoding="utf-8")
    chars = sorted(c for c in charset if c != " ")
    (out / "char_dict.txt").write_text("\n".join(chars), encoding="utf-8")

    print(f"[prep] 완료 — 유효 약 {n_ok}, skip {n_skip}")
    print(f"[prep] train {len(train_lines)} / val {len(val_lines)} "
          f"샘플, 문자 {len(chars)}종")
    print(f"[prep] 출력: {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
