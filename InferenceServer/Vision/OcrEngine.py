"""
OcrEngine — PaddleOCR 각인 인식 (싱글톤)

⭐ 2026-05-15 — Vision PC 담당자 (인효) 의 `engines/ocr_engine.py` + `lib/ocr.py` 코드를 이식.
   원본 핵심:
     - PaddleOCR(use_angle_cls=True, lang='korean')
     - 원본 + sharpening 2-variant 시도 후 가장 긴 결과 채택
     - join_lines: 신뢰도 순 결합
"""
import io
from dataclasses import dataclass
from typing import List, Optional, Tuple

import cv2
import numpy as np
from loguru import logger


@dataclass
class OcrLine:
    text: str
    confidence: float


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
        """모델 로딩 — PaddleOCR 한국어. 첫 호출 시 가중치 자동 다운로드 (~수십 MB)."""
        try:
            from paddleocr import PaddleOCR
            self.engine = PaddleOCR(use_angle_cls=True, lang="korean", show_log=False)
            self.is_loaded = True
            logger.info("[OcrEngine] PaddleOCR(korean) 로드 완료")
        except Exception as e:
            logger.warning(f"[OcrEngine] 로드 실패 (graceful): {e}")
            self.is_loaded = False

    def recognize(self, crop_image_bytes: bytes) -> Tuple[str, float]:
        """
        crop된 알약 이미지 (bytes) 에서 각인 텍스트 인식.

        Returns:
            (각인 텍스트, 평균 신뢰도) — 인식 실패 시 ("각인없음", 0.0)
        """
        img = self._decode(crop_image_bytes)
        if img is None:
            return ("각인없음", 0.0)
        return self.recognize_array(img)

    def recognize_array(self, crop_bgr: np.ndarray) -> Tuple[str, float]:
        """numpy 배열 직접 입력 — YoloDetector 의 _crop_cache 와 직결."""
        if not self.is_loaded or self.engine is None:
            return ("각인없음", 0.0)
        if crop_bgr is None or crop_bgr.size == 0:
            return ("각인없음", 0.0)

        best_lines: List[OcrLine] = []
        best_avg_conf = 0.0
        for variant in self._variants(crop_bgr):
            lines = self._ocr_one(variant)
            if not lines:
                continue
            # 가장 긴 결과 우선 (담당자 휴리스틱 — 짧은 false positive 회피)
            joined = " ".join(l.text for l in lines)
            best_joined = " ".join(l.text for l in best_lines) if best_lines else ""
            if len(joined) > len(best_joined):
                best_lines = lines
                avg = sum(l.confidence for l in lines) / max(len(lines), 1)
                best_avg_conf = avg

        if not best_lines:
            return ("각인없음", 0.0)

        # 신뢰도 순 결합 (담당자 join_lines 규칙)
        sorted_lines = sorted(best_lines, key=lambda l: l.confidence, reverse=True)
        text = " ".join(l.text for l in sorted_lines).strip()
        return (text or "각인없음", round(best_avg_conf, 3))

    # ---------- 내부 ----------

    def _decode(self, image_bytes: bytes) -> Optional[np.ndarray]:
        try:
            arr = np.frombuffer(image_bytes, dtype=np.uint8)
            return cv2.imdecode(arr, cv2.IMREAD_COLOR)
        except Exception:
            return None

    def _variants(self, img: np.ndarray):
        """원본 + sharpening + CLAHE 대비강화.

        ⭐ 2026-05-15 — 음각/양각 각인 가시화용 CLAHE variant 추가.
           흰 알약에 같은 색으로 음각된 글자(KG 등)는 대비가 거의
           없어 일반 OCR 이 실패 → CLAHE 로 국소 대비를 끌어올려
           각인 윤곽을 드러낸다. (모델 재학습 아님, 전처리 보강)
        """
        yield img
        try:
            kernel = np.array([[0, -0.5, 0], [-0.5, 3, -0.5], [0, -0.5, 0]])
            yield cv2.filter2D(img, -1, kernel)
        except Exception:
            pass
        try:
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8))
            enhanced = clahe.apply(gray)
            yield cv2.cvtColor(enhanced, cv2.COLOR_GRAY2BGR)
        except Exception:
            pass
        try:
            # CLAHE + sharpen 조합 (음각 윤곽 더 강조)
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            clahe = cv2.createCLAHE(clipLimit=4.0, tileGridSize=(8, 8))
            enh = clahe.apply(gray)
            k = np.array([[0, -1, 0], [-1, 5, -1], [0, -1, 0]])
            sharp = cv2.filter2D(enh, -1, k)
            yield cv2.cvtColor(sharp, cv2.COLOR_GRAY2BGR)
        except Exception:
            pass

    def _ocr_one(self, img: np.ndarray, min_confidence: float = 0.3) -> List[OcrLine]:
        try:
            result = self.engine.ocr(img, cls=True)
        except Exception as e:
            logger.warning(f"[OcrEngine] PaddleOCR 호출 예외: {e}")
            return []
        if not result or result[0] is None:
            return []
        out: List[OcrLine] = []
        for entry in result[0]:
            if entry is None or len(entry) < 2:
                continue
            tp = entry[1]
            if not isinstance(tp, (list, tuple)) or len(tp) < 2:
                continue
            try:
                text = str(tp[0]).strip()
                conf = float(tp[1])
            except Exception:
                continue
            if conf >= min_confidence and text:
                out.append(OcrLine(text=text, confidence=conf))
        return out
