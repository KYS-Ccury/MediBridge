"""
SizeMeasurer — 알약 크기 측정 (mm)

⚠ 담당자 원본 코드에는 size 모듈이 없음 — 본 모듈은 본 프로젝트에서 추가 구현.

캘리브레이션 두 가지 모드:
    A) calibration_mm_per_pixel 명시  — pylon·캘리브레이션 보드 사용 시
    B) 추정 모드                       — bbox 픽셀 + 알려진 카메라 거리로 추정
                                         (POC 단계 — 정확도 ±20% 가정)

알약은 보통 직경 5~20mm. 측정 결과가 이 범위 벗어나면 None 반환.
"""
from typing import Optional, Tuple

import cv2
import numpy as np


# 알약 직경 합리적 범위 (mm)
PILL_DIAMETER_MIN_MM = 3.0
PILL_DIAMETER_MAX_MM = 30.0

# POC 추정 모드 — 폰 카메라 평균 거리에서의 mm/px 휴리스틱
# 실 사용 시 캘리브레이션 보드 권장. 본 값은 30cm 거리 가정 (조정 가능).
DEFAULT_MM_PER_PIXEL_AT_PHONE: float = 0.06


def _diameter_pixels(crop_bgr: np.ndarray) -> Optional[float]:
    """알약 contour 의 직경(픽셀)을 minEnclosingCircle 로 측정."""
    if crop_bgr is None or crop_bgr.size == 0:
        return None
    gray = cv2.cvtColor(crop_bgr, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:
        return None
    cnt = max(contours, key=cv2.contourArea)
    if len(cnt) < 5:
        return None
    (_, _), radius = cv2.minEnclosingCircle(cnt)
    return float(radius * 2)


class SizeMeasurer:
    """알약 크기 측정 (정적 유틸)."""

    @staticmethod
    def measure_mm_array(
        crop_bgr: np.ndarray,
        calibration_mm_per_pixel: Optional[float] = None,
    ) -> Optional[float]:
        """
        numpy 배열 입력 → 직경(mm).

        Args:
            crop_bgr: BGR crop
            calibration_mm_per_pixel: 1픽셀당 mm. None 이면 POC 추정값 사용.

        Returns:
            5~30mm 범위 직경 또는 None.
        """
        diameter_px = _diameter_pixels(crop_bgr)
        if diameter_px is None:
            return None

        mm_per_px = calibration_mm_per_pixel
        if mm_per_px is None:
            mm_per_px = DEFAULT_MM_PER_PIXEL_AT_PHONE

        diameter_mm = diameter_px * mm_per_px
        if diameter_mm < PILL_DIAMETER_MIN_MM or diameter_mm > PILL_DIAMETER_MAX_MM:
            return None
        return round(diameter_mm, 2)

    @staticmethod
    def measure_mm(
        crop_image_bytes: bytes,
        calibration_mm_per_pixel: Optional[float] = None,
    ) -> Optional[float]:
        try:
            arr = np.frombuffer(crop_image_bytes, dtype=np.uint8)
            img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        except Exception:
            return None
        if img is None:
            return None
        return SizeMeasurer.measure_mm_array(img, calibration_mm_per_pixel)
