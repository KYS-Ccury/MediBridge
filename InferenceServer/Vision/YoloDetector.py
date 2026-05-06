"""
YoloDetector — YOLO26 알약 검출 (싱글톤)

학습 서버에서 fine-tuning된 가중치를 SCP로 받아 Models/yolo26_pills.pt 로 배포.
"""
from typing import List, Optional
from loguru import logger

# from ultralytics import YOLO   # 실제 import는 TODO 단계에서

from Schemas.VisionSchema import DetectedPill, BoundingBox


class YoloDetector:
    _instance: Optional["YoloDetector"] = None

    @classmethod
    def instance(cls) -> "YoloDetector":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.model = None
        self.weights_path: Optional[str] = None
        self.is_loaded: bool = False

    def load_model(self, weights_path: str) -> None:
        """모델 로딩 (서버 시작 시 1회)"""
        # TODO (영역 A 분담):
        #   from ultralytics import YOLO
        #   self.model = YOLO(weights_path)
        #   self.model.to("cuda")
        self.weights_path = weights_path
        self.is_loaded = False     # 실제 로딩 후 True
        logger.info(f"[YoloDetector] load_model TODO — {weights_path}")

    def detect(
        self,
        image_bytes: bytes,
        conf_threshold: float = 0.5,
    ) -> List[DetectedPill]:
        """
        이미지에서 다중 알약 검출.

        Args:
            image_bytes: JPEG/PNG 바이너리
            conf_threshold: 검출 신뢰도 임계

        Returns:
            검출된 알약 리스트
        """
        # TODO (영역 A 분담):
        #   1. cv2.imdecode(image_bytes) → numpy array
        #   2. results = self.model(img, conf=conf_threshold)
        #   3. results 의 boxes·xyxy·conf 추출 → BoundingBox + crop_id 발급
        #   4. crop 이미지를 메모리 캐시에 저장 (analyze 단계에서 재사용)
        return []

    def shutdown(self) -> None:
        """GPU 메모리 해제"""
        # TODO: del self.model + torch.cuda.empty_cache()
        self.model = None
        self.is_loaded = False
