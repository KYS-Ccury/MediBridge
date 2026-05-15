"""
YoloDetector — YOLO 알약 검출 (싱글톤)

학습 서버에서 fine-tuning된 가중치를 Models/yolo26_pills.pt 로 배포하거나
환경변수 MEDIBRIDGE_YOLO_WEIGHTS 로 지정. 기본은 ultralytics 가 자동 다운.

⭐ 2026-05-15 — Vision PC 담당자 (인효) 의 `engines/yolo_engine.py` 코드를 이식.
    원본: check_img_ih/engines/yolo_engine.py (`YoloDetector.detect_and_crop`)
    원본 학습: data_602020.yaml + train_single_602020 best.pt
"""
import io
import os
import uuid
from typing import Dict, List, Optional, Tuple

import cv2
import numpy as np
from loguru import logger

from Schemas.VisionSchema import DetectedPill, BoundingBox


def _env_float(name: str, default: float) -> float:
    try:
        return float(os.environ.get(name, "").strip() or default)
    except (TypeError, ValueError):
        return default


def _env_int(name: str, default: int) -> int:
    try:
        return int(os.environ.get(name, "").strip() or default)
    except (TypeError, ValueError):
        return default


def _iou(a: Tuple[int, int, int, int], b: Tuple[int, int, int, int]) -> float:
    """두 박스(x1,y1,x2,y2) 의 IoU."""
    ax1, ay1, ax2, ay2 = a
    bx1, by1, bx2, by2 = b
    ix1, iy1 = max(ax1, bx1), max(ay1, by1)
    ix2, iy2 = min(ax2, bx2), min(ay2, by2)
    iw, ih = max(0, ix2 - ix1), max(0, iy2 - iy1)
    inter = iw * ih
    if inter == 0:
        return 0.0
    area_a = max(0, ax2 - ax1) * max(0, ay2 - ay1)
    area_b = max(0, bx2 - bx1) * max(0, by2 - by1)
    union = area_a + area_b - inter
    return inter / union if union > 0 else 0.0


def _containment(inner: Tuple[int, int, int, int],
                  outer: Tuple[int, int, int, int]) -> float:
    """inner 박스가 outer 박스에 얼마나 포함되는지 (inner 면적 대비 교집합 비율)."""
    ix1, iy1 = max(inner[0], outer[0]), max(inner[1], outer[1])
    ix2, iy2 = min(inner[2], outer[2]), min(inner[3], outer[3])
    iw, ih = max(0, ix2 - ix1), max(0, iy2 - iy1)
    inter = iw * ih
    area_inner = max(1, (inner[2] - inner[0]) * (inner[3] - inner[1]))
    return inter / area_inner


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
        conf_threshold: Optional[float] = None,
    ) -> List[DetectedPill]:
        """
        이미지에서 다중 알약 검출.

        과검출(알약 1개가 여러 박스로 쪼개짐) 방지:
          1) conf / iou / max_det / agnostic_nms 를 추론 단계에서 강화
          2) 추론 후 IoU·포함관계 기반 dedup 으로 같은 알약 중복 박스 병합

        모든 임계값은 환경변수로 튜닝 가능 (모델 재학습 불필요):
          MEDIBRIDGE_YOLO_CONF       (기본 0.60)  검출 신뢰도 임계
          MEDIBRIDGE_YOLO_IOU        (기본 0.45)  NMS IoU (낮을수록 중복 억제 ↑)
          MEDIBRIDGE_YOLO_MAX_DET    (기본 10)    최대 검출 개수
          MEDIBRIDGE_YOLO_DEDUP_IOU  (기본 0.55)  후처리 dedup IoU 임계
          MEDIBRIDGE_YOLO_CONTAIN    (기본 0.70)  후처리 포함율 임계

        Args:
            image_bytes: JPEG/PNG 바이너리
            conf_threshold: 명시 시 env 보다 우선

        Returns:
            검출된 알약 리스트 (각 항목에 crop_id 발급, crop 이미지는 메모리 캐시).
        """
        if not self.is_loaded or self.model is None:
            logger.warning("[YoloDetector] 미로드 — 빈 결과 반환")
            return []

        img = self._decode_image(image_bytes)
        if img is None:
            return []

        conf = conf_threshold if conf_threshold is not None \
            else _env_float("MEDIBRIDGE_YOLO_CONF", 0.60)
        iou = _env_float("MEDIBRIDGE_YOLO_IOU", 0.45)
        max_det = _env_int("MEDIBRIDGE_YOLO_MAX_DET", 10)
        dedup_iou = _env_float("MEDIBRIDGE_YOLO_DEDUP_IOU", 0.55)
        contain_thr = _env_float("MEDIBRIDGE_YOLO_CONTAIN", 0.70)

        infer_kwargs = dict(
            conf=conf, iou=iou, max_det=max_det,
            agnostic_nms=True, verbose=False,
        )
        try:
            results = self.model(img, device=0, **infer_kwargs)
        except TypeError:
            results = self.model(img, **infer_kwargs)
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

        # 1) 원시 박스 수집 (좌표 클리핑 + 미세 박스 제거)
        raw: List[Tuple[Tuple[int, int, int, int], float]] = []
        for box in boxes:
            try:
                x1, y1, x2, y2 = map(int, box.xyxy[0].tolist())
                bconf = float(box.conf[0].item()) if hasattr(box, "conf") else 0.0
            except Exception:
                continue
            x1 = max(0, min(x1, w - 1))
            y1 = max(0, min(y1, h - 1))
            x2 = max(0, min(x2, w))
            y2 = max(0, min(y2, h))
            if x2 - x1 < 4 or y2 - y1 < 4:
                continue
            raw.append(((x1, y1, x2, y2), bconf))

        # 2) 후처리 dedup — 신뢰도 내림차순 greedy.
        #    이미 채택한 박스와 IoU 가 높거나, 그 안에 대부분 포함되면
        #    같은 알약의 중복 검출로 보고 버린다.
        raw.sort(key=lambda t: t[1], reverse=True)
        kept: List[Tuple[Tuple[int, int, int, int], float]] = []
        for bbox, bconf in raw:
            dup = False
            for kbox, _ in kept:
                if _iou(bbox, kbox) >= dedup_iou \
                        or _containment(bbox, kbox) >= contain_thr:
                    dup = True
                    break
            if not dup:
                kept.append((bbox, bconf))

        detections: List[DetectedPill] = []
        for (x1, y1, x2, y2), bconf in kept:
            crop = img[y1:y2, x1:x2]
            crop_id = self._cache_crop(crop)
            detections.append(DetectedPill(
                bbox=BoundingBox(
                    x=float(x1), y=float(y1),
                    width=float(x2 - x1), height=float(y2 - y1),
                ),
                crop_id=crop_id,
                confidence=bconf,
            ))

        logger.info(
            f"[YoloDetector] detect bytes={len(image_bytes)} conf>={conf} "
            f"iou={iou} raw={len(raw)} -> dedup={len(detections)}"
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
