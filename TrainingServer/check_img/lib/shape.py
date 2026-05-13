"""모양 분석 — 윤곽선 기반 → 식약처 모양 카테고리 매핑.

식약처 낱알식별 shape 값:
  원형 / 타원형 / 장방형 / 캡슐형 / 삼각형 / 사각형 / 마름모형 /
  오각형 / 육각형 / 팔각형 / 반원형 / 기타
"""

from __future__ import annotations
from dataclasses import dataclass

import cv2
import numpy as np


@dataclass
class ShapeResult:
    label:        str
    circularity:  float    # 1.0 = 완벽한 원
    aspect_ratio: float    # 외접 사각형 가로/세로
    elongation:   float    # 회전 사각형 긴 변/짧은 변


def analyze_shape(contour: np.ndarray) -> ShapeResult:
    """윤곽선 → 모양 카테고리.

    Args:
        contour: detection.PillBox.contour 그대로 입력
    """
    if contour is None or len(contour) < 5:
        return ShapeResult("기타", 0.0, 1.0, 1.0)

    area = cv2.contourArea(contour)
    perimeter = cv2.arcLength(contour, True)
    if perimeter == 0 or area == 0:
        return ShapeResult("기타", 0.0, 1.0, 1.0)

    # 원형도 — 4πA / P²  (원이면 1.0, 멀어질수록 작아짐)
    circularity = 4.0 * np.pi * area / (perimeter * perimeter)

    # 외접 사각형
    x, y, w, h = cv2.boundingRect(contour)
    aspect = w / max(h, 1)

    # 회전 최소 외접 사각형
    rect = cv2.minAreaRect(contour)
    (rw, rh) = rect[1]
    short_side = min(rw, rh) if min(rw, rh) > 0 else 1
    long_side  = max(rw, rh)
    elongation = long_side / short_side

    # 분류 휴리스틱
    if circularity >= 0.85 and elongation < 1.2:
        label = "원형"
    elif circularity >= 0.65 and 1.2 <= elongation < 2.0:   # 타원형 상한 1.7 → 2.0
        label = "타원형"
    elif elongation >= 2.5:
        label = "캡슐형"
    elif 2.0 <= elongation < 2.5 and circularity >= 0.55:   # 장방형 하한 1.7 → 2.0
        label = "장방형"
    else:
        # 다각형 추정 — approxPolyDP 로 꼭짓점 수 추정
        epsilon = 0.04 * perimeter
        approx = cv2.approxPolyDP(contour, epsilon, True)
        v = len(approx)
        polygon_map = {3: "삼각형", 4: "사각형", 5: "오각형",
                       6: "육각형", 8: "팔각형"}
        label = polygon_map.get(v, "기타")

    return ShapeResult(
        label=label,
        circularity=float(circularity),
        aspect_ratio=float(aspect),
        elongation=float(elongation),
    )
