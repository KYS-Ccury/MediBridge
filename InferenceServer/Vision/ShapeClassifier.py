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
    """모양 분류.

    ⚠ 2026-05-15 — 인효님 원본 engines/cv_engine.analyze 의 '안정적인
       장방형/원형 로직' 으로 회귀. 이전 이식이 circularity +
       approxPolyDP(다각형 꼭짓점) 경로를 추가했다가, 컨투어가 잘못
       잡히면 4꼭짓점→'사각형' 으로 원형 알약을 오분류했다.
       원본은 minAreaRect 종횡비만 보고 원형/타원형/장방형 3종만
       판정해 그런 오분류가 구조적으로 불가능 — 이 견고함을 복원.
       (매우 길쭉한 경우만 캡슐형 추가 — 식약처 카테고리 보존)
    """
    if contour is None or len(contour) < 5:
        return ShapeResult("기타", 0.0, 1.0, 1.0)

    # ⭐ 2026-05-15 R2 — 알약은 모두 볼록(convex). convex hull 로
    #   글자/반사 노치를 제거해 circularity·종횡비를 안정화.
    hull = cv2.convexHull(contour)
    if hull is not None and len(hull) >= 5:
        contour = hull

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
    ratio = long_side / short_side   # 원본의 핵심 지표

    # ⭐ 2026-05-15 R2 — convex hull 적용 후 원형 알약의 minAreaRect
    #   종횡비가 1.10~1.17 로 상승(원본 1.1 타원형 임계 침범).
    #   원형 알약은 촬영각/크롭으로 종횡비 1.2 까지 흔들리므로
    #   종횡비 단독 1.1 임계는 과민. circularity 와 조합해 보정:
    #     - 매우 길쭉(>=2.3) → 캡슐형
    #     - 장방(>=1.45) → 장방형
    #     - 타원: 종횡비>=1.25 거나 circularity 가 낮음(<0.78)
    #     - 그 외 → 원형 (둥근 알약의 약한 종횡비 변동 허용)
    if ratio >= 2.3:
        label = "캡슐형"
    elif ratio >= 1.45:
        label = "장방형"
    elif ratio >= 1.25 or circularity < 0.78:
        label = "타원형"
    else:
        label = "원형"

    return ShapeResult(
        label=label,
        circularity=float(circularity),
        aspect_ratio=float(aspect),
        elongation=float(ratio),
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
    # ⭐ 2026-05-15 R2 — 글자/반사로 생긴 전경 구멍·노치 메우기.
    #   파란 알약의 흰 각인이 Otsu 에서 분리돼 contour 가 들쭉날쭉
    #   (circularity 비정상 ↓) → CLOSE 로 알약 본체를 매끄럽게.
    k = max(3, (min(h, w) // 12) | 1)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (k, k))
    thresh = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, kernel, iterations=2)
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
