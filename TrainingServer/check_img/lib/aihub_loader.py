"""AI Hub 경구약제 어노테이션 JSON 로더 — COCO-like 포맷.

가이드라인 §4.3 의 스키마 기준:
  images[]:      id, file_name, drug_N, drug_shape, color_class1/2,
                 print_front/back, ... 등 약 메타 필드
  annotations[]: image_id, bbox=[x,y,w,h], category_id, area, ...
  categories[]:  id, name, supercategory

본 로더는 평가 시 GT (Ground Truth) 추출에 사용.
"""

from __future__ import annotations
import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple


@dataclass
class AiHubBox:
    """단일 알약의 GT 박스 + 카테고리."""
    image_id:    int
    bbox:        Tuple[float, float, float, float]   # (x, y, w, h) 픽셀
    category_id: int
    area:        Optional[float] = None


@dataclass
class AiHubImage:
    """단일 사진의 GT 메타 + 박스 리스트."""
    id:           int
    file_name:    str
    width:        int
    height:       int

    # 약 식별 메타 (단일 약 사진 기준 — 조합 사진은 다중 박스)
    drug_name:    Optional[str] = None      # drug_N
    drug_shape:   Optional[str] = None      # 원형/타원형/캡슐형/...
    color_class1: Optional[str] = None      # 흰색/노란색/...
    color_class2: Optional[str] = None
    print_front:  Optional[str] = None      # 앞면 각인 텍스트
    print_back:   Optional[str] = None
    item_seq:     Optional[str] = None
    di_class_no:  Optional[str] = None      # 식약처 약효분류번호

    # 어노테이션 박스 리스트 (조합 사진은 여러 개)
    boxes:        List[AiHubBox] = field(default_factory=list)


@dataclass
class AiHubCategory:
    id:            int
    name:          str
    supercategory: Optional[str] = None


def _safe_get(d: dict, *keys, default=None):
    for k in keys:
        if k in d and d[k] is not None:
            return d[k]
    return default


def load_aihub_json(json_path: Path) -> Tuple[Dict[int, AiHubImage], Dict[int, AiHubCategory]]:
    """JSON 1개 파일 → (image_id → AiHubImage, category_id → AiHubCategory).

    AI Hub JSON 은 1 파일에 여러 이미지·박스를 포함. 본 함수는 그 전체를 로드.
    """
    with open(json_path, "r", encoding="utf-8") as f:
        root = json.load(f)

    raw_images   = root.get("images", [])
    raw_anns     = root.get("annotations", [])
    raw_cats     = root.get("categories", [])

    # 이미지 dict 구성
    images: Dict[int, AiHubImage] = {}
    for img in raw_images:
        img_id = int(img.get("id"))
        images[img_id] = AiHubImage(
            id           = img_id,
            file_name    = str(_safe_get(img, "file_name", default="")),
            width        = int(_safe_get(img, "width",  default=0) or 0),
            height       = int(_safe_get(img, "height", default=0) or 0),

            drug_name    = _safe_get(img, "drug_N", "drug_name"),
            drug_shape   = _safe_get(img, "drug_shape", "form_code_name"),
            color_class1 = _safe_get(img, "color_class1"),
            color_class2 = _safe_get(img, "color_class2"),
            print_front  = _safe_get(img, "print_front"),
            print_back   = _safe_get(img, "print_back"),
            item_seq     = (str(img["item_seq"]) if img.get("item_seq") is not None else None),
            di_class_no  = _safe_get(img, "di_class_no"),
        )

    # 박스 매핑
    for ann in raw_anns:
        img_id = int(ann.get("image_id"))
        if img_id not in images:
            continue
        bbox = ann.get("bbox") or [0, 0, 0, 0]
        if len(bbox) != 4:
            continue
        images[img_id].boxes.append(AiHubBox(
            image_id    = img_id,
            bbox        = (float(bbox[0]), float(bbox[1]),
                           float(bbox[2]), float(bbox[3])),
            category_id = int(ann.get("category_id", -1)),
            area        = float(ann["area"]) if ann.get("area") is not None else None,
        ))

    # 카테고리
    categories: Dict[int, AiHubCategory] = {}
    for c in raw_cats:
        cid = int(c.get("id", -1))
        categories[cid] = AiHubCategory(
            id            = cid,
            name          = str(c.get("name", "")),
            supercategory = c.get("supercategory"),
        )

    return images, categories


# =====================================================
# 평가 헬퍼 — IoU, OCR/색/모양 매칭
# =====================================================

def iou(box_a: Tuple[float, float, float, float],
        box_b: Tuple[float, float, float, float]) -> float:
    """COCO 형식 (x, y, w, h) 두 박스의 IoU."""
    ax1, ay1, aw, ah = box_a
    bx1, by1, bw, bh = box_b
    ax2, ay2 = ax1 + aw, ay1 + ah
    bx2, by2 = bx1 + bw, by1 + bh

    inter_x1 = max(ax1, bx1); inter_y1 = max(ay1, by1)
    inter_x2 = min(ax2, bx2); inter_y2 = min(ay2, by2)
    iw = max(0.0, inter_x2 - inter_x1)
    ih = max(0.0, inter_y2 - inter_y1)
    inter = iw * ih
    union = aw * ah + bw * bh - inter
    return inter / union if union > 0 else 0.0


def normalize_engraving(text: str) -> str:
    """각인 비교용 정규화 — 공백·특수문자 제거, 대문자."""
    if not text:
        return ""
    keep = []
    for ch in text:
        if ch.isalnum() or ('가' <= ch <= '힣'):  # 한글 음절
            keep.append(ch.upper())
    return "".join(keep)


def engraving_matches(predicted: str, gt: Optional[str]) -> Tuple[bool, bool]:
    """(exact_match, substring_match) 반환.

    AI Hub 의 print_front 는 보통 "TYL 500" 같은 짧은 문자열.
    PaddleOCR 결과는 노이즈를 포함할 수 있어 substring 매칭도 함께 평가.
    """
    if not gt:
        # GT 없는 약 (각인 없음) — 예측도 비어야 정답
        return (predicted == "", predicted == "")

    p = normalize_engraving(predicted)
    g = normalize_engraving(gt)
    if not g:
        return (p == "", p == "")
    if not p:
        return (False, False)

    return (p == g, (g in p) or (p in g))


def label_match(predicted: Optional[str], gt: Optional[str]) -> bool:
    """색·모양 라벨 비교 — 단순 문자열 일치.

    AI Hub 색상 카테고리: 흰색/노란색/주황/빨간색/분홍/갈색/연두/파란색/초록/보라/검정/회색/기타
    AI Hub 모양:        원형/타원형/장방형/캡슐형/삼각형/사각형/마름모형/오각형/육각형/팔각형/반원형/기타
    """
    if not predicted and not gt:
        return True
    if not predicted or not gt:
        return False
    return predicted.strip() == gt.strip()


def best_iou_match(pred_boxes: List[Tuple[float, float, float, float]],
                   gt_boxes:   List[Tuple[float, float, float, float]],
                   iou_threshold: float = 0.5) -> List[Tuple[int, int, float]]:
    """탐욕적 매칭 — 각 GT 에 대해 가장 IoU 높은 pred 선택 (한 번씩만 사용).

    Returns:
        [(gt_index, pred_index, iou_value)] — pred 가 매칭 안 되면 pred_index = -1
    """
    matches: List[Tuple[int, int, float]] = []
    used_pred = set()
    for gi, gb in enumerate(gt_boxes):
        best = (-1, 0.0)
        for pi, pb in enumerate(pred_boxes):
            if pi in used_pred:
                continue
            v = iou(pb, gb)
            if v > best[1]:
                best = (pi, v)
        if best[0] >= 0 and best[1] >= iou_threshold:
            matches.append((gi, best[0], best[1]))
            used_pred.add(best[0])
        else:
            matches.append((gi, -1, best[1]))
    return matches
