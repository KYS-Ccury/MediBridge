"""
ColorClassifier — OpenCV HSV 기반 색상 분류

핸드폰 카메라(컬러) → HSV 평균값 → 식약처 매칭 키 ("백색", "연황색" 등).
pylon 확장(모노크롬 가능성) 시 fallback 별도 설계 필요.
"""


class ColorClassifier:
    """알약 crop 이미지의 색상 분류"""

    @staticmethod
    def classify(crop_image_bytes: bytes) -> str:
        """
        Args:
            crop_image_bytes: JPEG/PNG 바이너리

        Returns:
            식약처 매칭 키 ("백색", "연황색" 등)
        """
        # TODO (영역 A 분담):
        #   1. cv2.imdecode → BGR
        #   2. cv2.cvtColor(BGR2HSV)
        #   3. 평균 HSV 계산 + 사전 정의 색상 라벨 매핑
        #   4. (확장) pylon 모노크롬 환경 fallback
        return ""
