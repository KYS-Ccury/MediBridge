"""
YoloDetector — YOLO 알약 검출 (싱글톤)

학습 서버에서 fine-tuning된 가중치를 Models/yolo26_pills.pt 로 배포하거나
환경변수 MEDIBRIDGE_YOLO_WEIGHTS 로 지정. 기본은 ultralytics 가 자동 다운.

⭐ 2026-05-15 — Vision PC 담당자 (인효) 의 `engines/yolo_engine.py` 코드를 이식.
    원본: check_img_ih/engines/yolo_engine.py (`YoloDetector.detect_and_crop`)
    원본 학습: data_602020.yaml + train_single_602020 best.pt
"""
import io
import uuid
from typing import Dict, List, Optional, Tuple

import cv2
import numpy as np
from loguru import logger

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
        # crop 이미지 메모리 캐시 — analyze 단계에서 재사용 (TTL 단순화: dict + manual evict)
        self._crop_cache: Dict[str, np.ndarray] = {}
        self._max_cache_entries: int = 256

    def load_model(self, weights_path: str) -> None:
        """모델 로딩 (서버 시작 시 1회). 가중치 누락 시 graceful skip."""
        self.weights_path = weights_path
        try:
            from ultralytics import YOLO

            # 가중치 파일 누락이라도 ultralytics 가 자동 다운로드 시도하기 때문에
            # 명시적으로 None 모델은 안 만듬.
            self.model = YOLO(weights_path)
            # GPU 가능하면 cuda 로
            try:
                self.model.to("cuda")
            except Exception as e:
                logger.warning(f"[YoloDetector] CUDA 실패, CPU 사용: {e}")
            self.is_loaded = True
            logger.info(f"[YoloDetector] 로드 완료 — {weights_path}")
        except Exception as e:
            logger.warning(f"[YoloDetector] 로드 실패 (graceful): {e}")
            self.is_loaded = False

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
            검출된 알약 리스트 (각 항목에 crop_id 발급, crop 이미지는 메모리 캐시).
        """
        if not self.is_loaded or self.model is None:
            logger.warning("[YoloDetector] 미로드 — 빈 결과 반환")
            return []

        img = self._decode_image(image_bytes)
        if img is None:
            return []

        try:
            results = self.model(img, conf=conf_threshold, device=0, verbose=False)
        except TypeError:
            # device=0 미지원 환경 (CPU 모드) fallback
            results = self.model(img, conf=conf_threshold, verbose=False)
        except Exception as e:
            logger.exception(f"[YoloDetector] 추론 실패: {e}")
            return []

        if not results:
            return []
        first = results[0]
        boxes = getattr(first, "boxes", None)
        if boxes is None:
            return []

        h, w = img.shape[:2]
        detections: List[DetectedPill] = []
        for box in boxes:
            try:
                x1, y1, x2, y2 = map(int, box.xyxy[0].tolist())
                conf = float(box.conf[0].item()) if hasattr(box, "conf") else 0.0
            except Exception:
                continue

            # 좌표 클리핑
            x1 = max(0, min(x1, w - 1))
            y1 = max(0, min(y1, h - 1))
            x2 = max(0, min(x2, w))
            y2 = max(0, min(y2, h))
            if x2 - x1 < 4 or y2 - y1 < 4:
                continue

            crop = img[y1:y2, x1:x2]
            crop_id = self._cache_crop(crop)

            detections.append(DetectedPill(
                bbox=BoundingBox(
                    x=float(x1), y=float(y1),
                    width=float(x2 - x1), height=float(y2 - y1),
                ),
                crop_id=crop_id,
                confidence=conf,
            ))

        logger.info(
            f"[YoloDetector] detect bytes={len(image_bytes)} conf>={conf_threshold} "
            f"results={len(detections)}"
        )
        return detections

    def get_crop(self, crop_id: str) -> Optional[np.ndarray]:
        """analyze 단계에서 crop 이미지 회수. 없으면 None."""
        return self._crop_cache.get(crop_id)

    def shutdown(self) -> None:
        """GPU 메모리 해제"""
        try:
            del self.model
        except Exception:
            pass
        self.model = None
        self.is_loaded = False
        self._crop_cache.clear()
        try:
            import torch
            torch.cuda.empty_cache()
        except Exception:
            pass

    # ---------- 내부 헬퍼 ----------

    def _decode_image(self, image_bytes: bytes) -> Optional[np.ndarray]:
        try:
            arr = np.frombuffer(image_bytes, dtype=np.uint8)
            img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
            if img is None:
                logger.warning("[YoloDetector] 이미지 디코드 실패")
            return img
        except Exception as e:
            logger.exception(f"[YoloDetector] decode 예외: {e}")
            return None

    def _cache_crop(self, crop: np.ndarray) -> str:
        crop_id = "crop_" + uuid.uuid4().hex[:16]
        # 캐시 LRU 단순화 — 한도 초과 시 가장 오래된 1개 제거 (insertion order 의존)
        if len(self._crop_cache) >= self._max_cache_entries:
            try:
                oldest = next(iter(self._crop_cache))
                del self._crop_cache[oldest]
            except StopIteration:
                pass
        self._crop_cache[crop_id] = crop
        return crop_id
