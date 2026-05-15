"""
ColorClassifier — HSV 분포 → 식약처 색상 카테고리 매핑

⭐ 2026-05-15 — Vision PC 담당자 (인효) `lib/color.py` + `engines/cv_engine.py` 이식.
   원본 카테고리: 흰색·노란색·주황·빨간색·분홍·갈색·연두·파란색·초록·보라·검정·회색·기타
   원본 휴리스틱: 중앙 30~70% + 채도 상위 50% (회색 오판정 방지)
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


# HSV 카테고리 — 담당자 lib/color.py 그대로
HSV_CATEGORIES = [
    # (label,         H_lo, H_hi, S_lo, V_lo, V_hi)
    ("흰색",            0,  179,    0,  170,  255),
    ("검정",            0,  179,    0,    0,   60),
    ("회색",            0,  179,    0,   60,  170),
    ("빨간색",          0,   10,   60,   60,  255),
    ("빨간색",        160,  179,   60,   60,  255),
    ("주황",           11,   25,   60,   80,  255),
    ("노란색",         26,   35,   60,   80,  255),
    ("연두",           36,   55,   40,   60,  255),
    ("초록",           56,   85,   40,   40,  255),
    ("파란색",         86,  130,   40,   40,  255),
    ("보라",          131,  159,   40,   40,  255),
    ("분홍",            0,   10,   30,  150,  255),
    ("갈색",            0,   25,   60,   30,  130),
]


def _make_mask(crop_bgr: np.ndarray) -> np.ndarray:
    gray = cv2.cvtColor(crop_bgr, cv2.COLOR_BGR2GRAY)
    _, mask = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    return cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=1)


def _classify_pixel(h: int, s: int, v: int) -> Optional[str]:
    for label, h_lo, h_hi, s_lo, v_lo, v_hi in HSV_CATEGORIES:
        if h_lo <= h <= h_hi and s >= s_lo and v_lo <= v <= v_hi:
            return label
    return None


def _vibrant_center(img: np.ndarray) -> np.ndarray:
    """담당자 engines/cv_engine._get_vibrant_center — 회색 오판정 회피."""
    h, w = img.shape[:2]
    center = img[int(h * 0.3):int(h * 0.7), int(w * 0.3):int(w * 0.7)].copy()
    if center.size == 0:
        return img
    try:
        hsv = cv2.cvtColor(center, cv2.COLOR_BGR2HSV)
        s_channel = hsv[:, :, 1]
        mask = s_channel > np.median(s_channel)
        if np.any(mask):
            center[~mask] = [0, 0, 0]
    except Exception:
        pass
    return center


class ColorClassifier:
    """알약 crop 의 식약처 색상 카테고리 분류 (정적 유틸)."""

    @staticmethod
    def classify_array(crop_bgr: np.ndarray, *, use_vibrant_center: bool = True) -> ColorResult:
        if crop_bgr is None or crop_bgr.size == 0:
            return ColorResult("기타", (0.0, 0.0, 0.0), 0.0)

        src = _vibrant_center(crop_bgr) if use_vibrant_center else crop_bgr
        hsv = cv2.cvtColor(src, cv2.COLOR_BGR2HSV)
        mask = _make_mask(src)
        pixels = hsv[mask > 0]
        if pixels.size == 0:
            return ColorResult("기타", (0.0, 0.0, 0.0), 0.0)

        counts: Dict[str, int] = {}
        for h, s, v in pixels:
            label = _classify_pixel(int(h), int(s), int(v)) or "기타"
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
