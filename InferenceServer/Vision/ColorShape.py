"""
ColorShape — OpenCV 기반 색상·모양·크기 분석

핸드폰 카메라(컬러) → HSV 색상 분류 + 윤곽선 모양 분류 + 캘리브레이션 크기.
pylon 확장(모노크롬 가능성) 시 색상 매칭 fallback 별도 설계.
"""
from typing import Optional, Tuple


class ColorShape:
    @staticmethod
    def classify_color(crop_image_bytes: bytes) -> str:
        """
        HSV 색상 분류 → 식약처 매칭 키 ("백색", "연황색" 등).
        """
        # TODO (영역 A 분담):
        #   1. cv2.imdecode → BGR
        #   2. cv2.cvtColor(BGR2HSV)
        #   3. 평균 HSV 계산 + 사전 정의 색상 라벨 매핑
        return ""

    @staticmethod
    def classify_shape(crop_image_bytes: bytes) -> str:
        """
        윤곽선 분석 → 모양 분류 (원/타원/캡슐형 등).
        """
        # TODO (영역 A 분담):
        #   1. 그레이 → threshold → findContours
        #   2. 가장 큰 contour 의 종횡비·convexity로 분류
        return ""

    @staticmethod
    def measure_size_mm(
        crop_image_bytes: bytes,
        calibration_mm_per_pixel: Optional[float] = None,
    ) -> Optional[float]:
        """
        캘리브레이션 기반 크기 측정 (mm).

        calibration_mm_per_pixel 미입력 시 None 반환.
        """
        # TODO (영역 A 분담):
        #   contour 의 minEnclosingCircle 또는 boundingRect → pixel 길이
        #   * calibration_mm_per_pixel
        return None
