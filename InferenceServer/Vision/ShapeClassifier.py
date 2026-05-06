"""
ShapeClassifier — OpenCV 윤곽선 기반 모양 분류

원·타원·캡슐형·기타 등의 사전 정의 라벨로 분류.
"""


class ShapeClassifier:
    """알약 crop 이미지의 모양 분류"""

    @staticmethod
    def classify(crop_image_bytes: bytes) -> str:
        """
        Args:
            crop_image_bytes: JPEG/PNG 바이너리

        Returns:
            식약처 매칭 키 ("원형", "타원형", "장방형", "캡슐형" 등)
        """
        # TODO (영역 A 분담):
        #   1. 그레이스케일 → threshold (Otsu 권장)
        #   2. cv2.findContours
        #   3. 가장 큰 contour 의:
        #      - aspect ratio (boundingRect)
        #      - circularity (4*pi*area / perimeter^2)
        #      - convexity defect 수
        #   4. 사전 정의 임계값 기반 라벨 매핑
        return ""
