"""
OcrEngine — PaddleOCR 각인 인식 (싱글톤)

알약 데이터 fine-tuning된 가중치 사용.
"""
from typing import Optional, Tuple
from loguru import logger

# from paddleocr import PaddleOCR   # TODO 단계에서


class OcrEngine:
    _instance: Optional["OcrEngine"] = None

    @classmethod
    def instance(cls) -> "OcrEngine":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.engine = None
        self.is_loaded: bool = False

    def load_model(self) -> None:
        """모델 로딩"""
        # TODO (영역 A 분담):
        #   self.engine = PaddleOCR(
        #       use_angle_cls=True,
        #       lang='korean',
        #       rec_model_dir=Config.ocr_weights_path,
        #       use_gpu=True,
        #   )
        logger.info("[OcrEngine] load_model TODO")
        self.is_loaded = False

    def recognize(self, crop_image_bytes: bytes) -> Tuple[str, float]:
        """
        crop된 알약 이미지에서 각인 텍스트 인식.

        Returns:
            (각인 텍스트, 신뢰도)
        """
        # TODO (영역 A 분담):
        #   1. cv2.imdecode(crop_image_bytes) → numpy array
        #   2. result = self.engine.ocr(img, cls=True)
        #   3. 텍스트 결합 + 평균 신뢰도 계산
        #   4. 인식 실패 시 ResNet fallback (FR-A2-04, 확장 단계)
        return ("", 0.0)
