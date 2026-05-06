"""
SizeMeasurer — 캘리브레이션 기반 알약 크기 측정 (mm)
"""
from typing import Optional


class SizeMeasurer:
    """알약 crop 이미지에서 실제 크기 측정"""

    @staticmethod
    def measure_mm(
        crop_image_bytes: bytes,
        calibration_mm_per_pixel: Optional[float] = None,
    ) -> Optional[float]:
        """
        Args:
            crop_image_bytes: JPEG/PNG 바이너리
            calibration_mm_per_pixel: 1픽셀당 mm (캘리브레이션 결과)

        Returns:
            mm 단위 크기 (캘리브레이션 미입력 시 None)
        """
        if calibration_mm_per_pixel is None:
            return None

        # TODO (영역 A 분담):
        #   1. contour 의 minEnclosingCircle 또는 boundingRect → pixel 길이
        #   2. * calibration_mm_per_pixel
        #   3. 알약은 보통 직경 5~20mm 범위 — 벗어나면 측정 실패로 None 반환
        return None
