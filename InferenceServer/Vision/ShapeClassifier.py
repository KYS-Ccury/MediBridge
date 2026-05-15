"""
ShapeClassifier — 윤곽선 기반 모양 분류

⭐ 2026-05-15 — Vision PC 담당자 (인효) `lib/shape.py` + `engines/cv_engine.py` 이식.
   원본 카테고리: 원형·타원형·장방형·캡슐형·삼각형·사각형·마름모형·오각형·육각형·팔각형·반원형·기타
   원본 휴리스틱: circularity + elongation + approxPolyDP 꼭짓점 수
"""
from dataclasses import dataclass
from typing import Optional

import cv2
import numpy as np


@dataclass
class ShapeResult:
    label: str
    circularity: float       # 1.0 = 완벽한 원
    aspect_ratio: float
    elongation: float        # 회전 사각형 long/short


def _classify_from_contour(contour: np.ndarray) -> ShapeResult:
    """담당자 lib/shape.py 의 analyze_shape 그대로."""
    if contour is None or len(contour) < 5:
        return ShapeResult("기타", 0.0, 1.0, 1.0)

    area = cv2.contourArea(contour)
    perimeter = cv2.arcLength(contour, True)
    if perimeter == 0 or area == 0:
        return ShapeResult("기타", 0.0, 1.0, 1.0)

    circularity = 4.0 * np.pi * area / (perimeter * perimeter)

    x, y, w, h = cv2.boundingRect(contour)
    aspect = w / max(h, 1)

    rect = cv2.minAreaRect(contour)
    rw, rh = rect[1]
    short_side = min(rw, rh) if min(rw, rh) > 0 else 1
    long_side = max(rw, rh)
    elongation = long_side / short_side

    if circularity >= 0.85 and elongation < 1.2:
        label = "원형"
    elif circularity >= 0.65 and 1.2 <= elongation < 1.7:
        label = "타원형"
    elif elongation >= 2.5:
        label = "캡슐형"
    elif 1.7 <= elongation < 2.5 and circularity >= 0.55:
        label = "장방형"
    else:
        epsilon = 0.04 * perimeter
        approx = cv2.approxPolyDP(contour, epsilon, True)
        v = len(approx)
        polygon_map = {3: "삼각형", 4: "사각형", 5: "오각형", 6: "육각형", 8: "팔각형"}
        label = polygon_map.get(v, "기타")

    return ShapeResult(
        label=label,
        circularity=float(circularity),
        aspect_ratio=float(aspect),
        elongation=float(elongation),
    )


def _largest_contour(crop_bgr: np.ndarray) -> Optional[np.ndarray]:
    """Otsu 후 가장 큰 contour.

    ⚠ 2026-05-15 버그 수정: THRESH_BINARY_INV 고정은 흰 알약(밝은 배경)
       에서 배경(직사각 크롭 프레임)을 전경으로 잡아, 원형 알약을
       4꼭짓점 → '사각형' 으로 오분류했다.
       → 중앙(=알약)이 속한 쪽을 전경으로 선택.
    """
    gray = cv2.cvtColor(crop_bgr, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, 0, 255,
                              cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    h, w = gray.shape[:2]
    cy0, cy1 = int(h * 0.35), int(h * 0.65)
    cx0, cx1 = int(w * 0.35), int(w * 0.65)
    center = thresh[cy0:cy1, cx0:cx1]
    if center.size > 0 and float((center > 0).mean()) < 0.5:
        thresh = cv2.bitwise_not(thresh)
    contours, _ = cv2.findContours(
        thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:
        return None
    return max(contours, key=cv2.contourArea)


class ShapeClassifier:
    """알약 crop 모양 분류 (정적 유틸)."""

    @staticmethod
    def classify_array(crop_bgr: np.ndarray) -> ShapeResult:
        if crop_bgr is None or crop_bgr.size == 0:
            return ShapeResult("기타", 0.0, 1.0, 1.0)
        cnt = _largest_contour(crop_bgr)
        if cnt is None:
            return ShapeResult("기타", 0.0, 1.0, 1.0)
        return _classify_from_contour(cnt)

    @staticmethod
    def classify(crop_image_bytes: bytes) -> str:
        try:
            arr = np.frombuffer(crop_image_bytes, dtype=np.uint8)
            img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        except Exception:
            return "기타"
        if img is None:
            return "기타"
        return ShapeClassifier.classify_array(img).label
