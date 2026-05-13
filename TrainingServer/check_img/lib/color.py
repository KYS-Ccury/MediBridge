"""색상 분석 — HSV 분포 → 식약처 색상 카테고리 매핑.

식약처 낱알식별 데이터의 color_front 값:
  흰색 / 노란색 / 주황 / 빨간색 / 분홍 / 갈색 / 연두 / 파란색 / 초록 / 보라 / 검정 / 회색 / 기타

알약 본체만 분석하기 위해 윤곽선 마스크를 만들어 그 영역만 평균.
"""

from __future__ import annotations
from dataclasses import dataclass
from typing import Dict, List, Optional

import cv2
import numpy as np


@dataclass
class ColorResult:
    label:       str           # 식약처 색상 카테고리
    hsv_mean:    tuple         # (H, S, V) — 디버그용
    confidence:  float         # 분포 집중도 (0~1)


# HSV 색상 카테고리 정의 — H 0~179 (OpenCV), S 0~255, V 0~255
#
# ⭐ v2 — S 상한(S_hi) 추가 + 카테고리 순서 재배치:
#   - 컬러 카테고리(높은 채도)가 먼저 매칭
#   - 무채색(흰/회/검)은 마지막 fallback (S_hi 로 컬러 픽셀 침범 차단)
#
# 튜플: (label, H_lo, H_hi, S_lo, S_hi, V_lo, V_hi)
# 분류 함수는 H_lo<=h<=H_hi AND S_lo<=s<=S_hi AND V_lo<=v<=V_hi 모두 만족 시 매칭.
HSV_CATEGORIES = [
    # ----- 컬러 카테고리 (먼저 매칭) -----
    ("빨간색",          0,    7,   80,  255,   60,  255),  # 순수 빨강 (H 상한 10→7)
    ("빨간색",        165,  179,   80,  255,   60,  255),  # 빨강 wrap-around
    ("주황",            8,   25,   60,  255,   80,  255),  # H 11→8 (LC500 H=9 잡히도록)
    ("노란색",         26,   35,   60,  255,   80,  255),
    ("연두",           36,   55,   40,  255,   60,  255),
    ("초록",           56,   85,   60,  255,   40,  255),
    ("파란색",         86,  130,   40,  255,   40,  255),
    ("보라",          131,  159,   40,  255,   40,  255),
    ("분홍",            0,   10,   30,   80,  150,  255),  # 채도 낮은 밝은 빨강
    ("분홍",          160,  179,   30,   80,  150,  255),
    ("갈색",            0,   25,   60,  255,   30,  130),  # 어두운 빨강~노랑

    # ----- 무채색 (낮은 S, fallback) -----
    ("흰색",            0,  179,    0,   50,  170,  255),  # 매우 낮은 S + 밝음
    ("회색",            0,  179,    0,   60,   60,  170),  # 낮은 S + 중간 밝기
    ("검정",            0,  179,    0,  255,    0,   60),  # 매우 어두움 (S 무관)
]


def _make_mask(crop_bgr: np.ndarray) -> np.ndarray:
    """배경 제외 알약 본체 마스크 — Otsu 이진화 반전 (fallback).

    [⚠ deprecated path] 정확한 마스크는 detect_pills 가 만든 contour 를
    `analyze_color(..., contour=...)` 인자로 받는 경로 사용 권장. 본 함수는
    contour 가 없을 때만 fallback. 어두운 배경/투톤 사진에서 부정확.
    """
    gray = cv2.cvtColor(crop_bgr, cv2.COLOR_BGR2GRAY)
    _, mask = cv2.threshold(gray, 0, 255,
                            cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=1)
    return mask


def _make_mask_from_contour(crop_shape: tuple,
                            contour: np.ndarray,
                            crop_offset: tuple = (0, 0)) -> np.ndarray:
    """detect_pills 가 만든 contour 로 정확한 알약 본체 마스크 생성.

    contour 는 원본 이미지 좌표계 → crop_offset 만큼 빼서 crop 좌표로 이동.
    가장자리 안티앨리어싱·반사광 영향 제거를 위해 살짝 erode.
    """
    h, w = crop_shape[:2]
    mask = np.zeros((h, w), dtype=np.uint8)
    ox, oy = crop_offset
    shifted = (contour - np.array([[[ox, oy]]], dtype=contour.dtype)).astype(np.int32)
    cv2.drawContours(mask, [shifted], -1, 255, thickness=-1)
    # 가장자리 1~2px erode — 알약 외곽 안티앨리어싱·하이라이트 픽셀 제외
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    mask = cv2.erode(mask, kernel, iterations=1)
    return mask


def _classify_pixel(h: int, s: int, v: int) -> Optional[str]:
    """카테고리 순서대로 매칭 — 첫 매칭을 반환.

    컬러 카테고리가 먼저 정의되어 있으므로 채도(S) 가 충분한 컬러 픽셀은
    무채색 fallback 까지 가지 않는다. 무채색 카테고리는 S_hi 상한으로
    컬러 픽셀 침범을 차단.
    """
    for label, h_lo, h_hi, s_lo, s_hi, v_lo, v_hi in HSV_CATEGORIES:
        if (h_lo <= h <= h_hi
                and s_lo <= s <= s_hi
                and v_lo <= v <= v_hi):
            return label
    return None


def analyze_color(crop_bgr: np.ndarray,
                  contour: Optional[np.ndarray] = None,
                  crop_offset: tuple = (0, 0)) -> ColorResult:
    """알약 crop 의 대표 색상을 반환.

    Args:
        crop_bgr:    OpenCV BGR crop
        contour:     detect_pills 가 만든 윤곽선 (원본 이미지 좌표)
        crop_offset: crop 의 원본 이미지 내 (x, y) 시작 좌표

    contour 가 주어지면 그것으로 정확한 알약 본체 마스크 생성 (배경 픽셀
    완전 제외). 없으면 _make_mask 의 Otsu fallback (덜 정확).
    """
    if crop_bgr is None or crop_bgr.size == 0:
        return ColorResult("기타", (0, 0, 0), 0.0)

    hsv = cv2.cvtColor(crop_bgr, cv2.COLOR_BGR2HSV)

    if contour is not None and len(contour) >= 3:
        mask = _make_mask_from_contour(crop_bgr.shape, contour, crop_offset)
    else:
        mask = _make_mask(crop_bgr)

    pixels = hsv[mask > 0]
    if pixels.size == 0:
        return ColorResult("기타", (0, 0, 0), 0.0)

    counts: Dict[str, int] = {}
    for h, s, v in pixels:
        label = _classify_pixel(int(h), int(s), int(v)) or "기타"
        counts[label] = counts.get(label, 0) + 1

    total = sum(counts.values())
    label = max(counts, key=counts.get)
    confidence = counts[label] / total

    h_mean = float(np.mean(pixels[:, 0]))
    s_mean = float(np.mean(pixels[:, 1]))
    v_mean = float(np.mean(pixels[:, 2]))

    return ColorResult(label=label,
                       hsv_mean=(h_mean, s_mean, v_mean),
                       confidence=confidence)
