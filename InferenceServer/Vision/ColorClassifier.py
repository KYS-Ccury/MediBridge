"""
ColorClassifier — HSV 분포 → 식약처 색상 카테고리 매핑

⭐ 2026-05-15 v2 — TrainingServer/check_img/lib/color.py (사용자 개선본) 이식.
   기존 Vision PC 이식본의 문제(흰 알약 → '검정' 오분류) 를 근본 해결:
     1) HSV 카테고리에 S 상한(S_hi) 추가 → 컬러 픽셀이 무채색으로 새는
        것 차단
     2) 카테고리 순서: 컬러(고채도) 먼저, 무채색(흰/회/검) 마지막 fallback
     3) contour 기반 정확 마스크 + 가장자리 erode (반사광·AA 픽셀 제외)
        — Otsu 만 쓰던 기존 경로가 밝은 배경 흰 알약에서 배경을 잡던
          문제를 회피
     4) _vibrant_center 폐기 — 정확 마스크 + S_hi 로 회색 오판이 이미
        해결되며, 저채도 픽셀 검정처리가 흰/회색 알약을 파괴했음
"""
from dataclasses import dataclass
from typing import Dict, Optional, Tuple

import cv2
import numpy as np


@dataclass
class ColorResult:
    label: str
    hsv_mean: Tuple[float, float, float]
    confidence: float


# 튜플: (label, H_lo, H_hi, S_lo, S_hi, V_lo, V_hi)
#   컬러 카테고리(고채도) 먼저 → 무채색은 마지막 fallback.
#   S_hi 로 컬러 픽셀이 흰/회/검 으로 새는 것을 차단.
HSV_CATEGORIES = [
    # ----- 컬러 카테고리 (먼저 매칭) -----
    ("빨간색",    0,    7,   80,  255,   60,  255),
    ("빨간색",  165,  179,   80,  255,   60,  255),
    ("주황",      8,   25,   60,  255,   80,  255),
    ("노란색",   26,   35,   60,  255,   80,  255),
    ("연두",     36,   55,   40,  255,   60,  255),
    ("초록",     56,   85,   60,  255,   40,  255),
    ("파란색",   86,  130,   40,  255,   40,  255),
    ("보라",    131,  159,   40,  255,   40,  255),
    ("분홍",      0,   10,   30,   80,  150,  255),
    ("분홍",    160,  179,   30,   80,  150,  255),
    ("갈색",      0,   25,   60,  255,   30,  130),
    # ----- 무채색 (낮은 S, fallback) -----
    ("흰색",      0,  179,    0,   50,  170,  255),
    ("회색",      0,  179,    0,   60,   60,  170),
    ("검정",      0,  179,    0,  255,    0,   60),
]


def _largest_contour(crop_bgr: np.ndarray) -> Optional[np.ndarray]:
    """알약 본체 contour — 중앙(=알약)이 속한 Otsu 쪽을 전경으로 선택.

    YOLO 가 알약을 타이트 크롭하므로 중앙은 곧 알약. 밝은 배경 위
    흰 알약/어두운 배경 위 검은 알약 모두 대응.
    """
    gray = cv2.cvtColor(crop_bgr, cv2.COLOR_BGR2GRAY)
    _, th = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    h, w = gray.shape[:2]
    cy0, cy1 = int(h * 0.35), int(h * 0.65)
    cx0, cx1 = int(w * 0.35), int(w * 0.65)
    center = th[cy0:cy1, cx0:cx1]
    if center.size > 0 and float((center > 0).mean()) < 0.5:
        th = cv2.bitwise_not(th)
    cnts, _ = cv2.findContours(th, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not cnts:
        return None
    return max(cnts, key=cv2.contourArea)


def _mask_from_contour(shape: tuple, contour: np.ndarray) -> np.ndarray:
    """contour 를 채운 마스크 + 가장자리 erode (반사광·AA 픽셀 제외)."""
    h, w = shape[:2]
    mask = np.zeros((h, w), dtype=np.uint8)
    cv2.drawContours(mask, [contour.astype(np.int32)], -1, 255, thickness=-1)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    mask = cv2.erode(mask, kernel, iterations=2)
    # erode 로 다 사라지면(작은 알약) erode 약화
    if int(np.count_nonzero(mask)) < 30:
        mask = np.zeros((h, w), dtype=np.uint8)
        cv2.drawContours(mask, [contour.astype(np.int32)], -1, 255, thickness=-1)
        mask = cv2.erode(mask, kernel, iterations=1)
    return mask


def _classify_pixel(h: int, s: int, v: int) -> Optional[str]:
    for label, h_lo, h_hi, s_lo, s_hi, v_lo, v_hi in HSV_CATEGORIES:
        if h_lo <= h <= h_hi and s_lo <= s <= s_hi and v_lo <= v <= v_hi:
            return label
    return None


class ColorClassifier:
    """알약 crop 의 식약처 색상 카테고리 분류 (정적 유틸)."""

    @staticmethod
    def classify_array(crop_bgr: np.ndarray,
                       *, use_vibrant_center: bool = False) -> ColorResult:
        # use_vibrant_center 인자는 호환용으로만 유지(무시) — v2 미사용
        if crop_bgr is None or crop_bgr.size == 0:
            return ColorResult("기타", (0.0, 0.0, 0.0), 0.0)

        hsv = cv2.cvtColor(crop_bgr, cv2.COLOR_BGR2HSV)

        cnt = _largest_contour(crop_bgr)
        if cnt is not None and len(cnt) >= 3:
            mask = _mask_from_contour(crop_bgr.shape, cnt)
        else:
            # contour 실패 시 중앙 영역 fallback
            h, w = crop_bgr.shape[:2]
            mask = np.zeros((h, w), dtype=np.uint8)
            mask[int(h * 0.3):int(h * 0.7), int(w * 0.3):int(w * 0.7)] = 255

        pixels = hsv[mask > 0]
        if pixels.size == 0:
            return ColorResult("기타", (0.0, 0.0, 0.0), 0.0)

        counts: Dict[str, int] = {}
        for h_, s_, v_ in pixels:
            label = _classify_pixel(int(h_), int(s_), int(v_)) or "기타"
            counts[label] = counts.get(label, 0) + 1
        total = sum(counts.values())
        label = max(counts, key=counts.get)
        confidence = counts[label] / total
        return ColorResult(
            label=label,
            hsv_mean=(
                float(np.mean(pixels[:, 0])),
                float(np.mean(pixels[:, 1])),
                float(np.mean(pixels[:, 2])),
            ),
            confidence=float(confidence),
        )

    @staticmethod
    def classify(crop_image_bytes: bytes) -> str:
        """bytes 입력 → 식약처 색상 라벨 문자열만."""
        try:
            arr = np.frombuffer(crop_image_bytes, dtype=np.uint8)
            img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        except Exception:
            return "기타"
        if img is None:
            return "기타"
        return ColorClassifier.classify_array(img).label
