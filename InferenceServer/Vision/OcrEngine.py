"""
OcrEngine — PaddleOCR 각인 인식 (싱글톤)

⭐ 2026-05-15 v2 — TrainingServer/check_img/lib/ocr.py (사용자 개선본) 이식.
   기존 이식본 문제: lang='korean' + 약한 sharpen → 영문 각인을 'PXF'
   처럼 오인식·미인식.

   v2 핵심:
     - 언어 모델 'korean' → **'en'** : 의약품 각인은 거의 영문·숫자.
       한글 모델보다 라틴 문자/숫자에 정확·경량.
     - 전처리 3종:
         A) 업스케일 — 짧은 변 < 240px 이면 INTER_CUBIC 확대
         B) CLAHE   — LAB L 채널 로컬 콘트라스트 (음각 그림자 강조)
         C) Unsharp — 가우시안 블러 빼기 샤프닝 (글자 윤곽 선명)
     - min_confidence 0.25 (각인은 일반 텍스트보다 conf 낮은 경향)
"""
from dataclasses import dataclass
from typing import List, Optional, Tuple

import cv2
import numpy as np
from loguru import logger

TARGET_MIN_SIDE = 384   # 업스케일 기준 (짧은 변 px) — R3: 240→384


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
        """PaddleOCR 영문 모델 로딩. 첫 호출 시 가중치 자동 다운로드."""
        try:
            from paddleocr import PaddleOCR
            # lang='en' — 알약 각인은 거의 영문·숫자
            try:
                self.engine = PaddleOCR(use_angle_cls=True, lang="en",
                                        show_log=False)
            except TypeError:
                self.engine = PaddleOCR(use_angle_cls=True, lang="en")
            self.is_loaded = True
            logger.info("[OcrEngine] PaddleOCR(en) 로드 완료")
        except Exception as e:
            logger.warning(f"[OcrEngine] 로드 실패 (graceful): {e}")
            self.is_loaded = False

    # ---------- 전처리 ----------

    @staticmethod
    def _preprocess(crop_bgr: np.ndarray) -> np.ndarray:
        """A) 업스케일 + B) LAB-L CLAHE + C) Unsharp."""
        if crop_bgr is None or crop_bgr.size == 0:
            return crop_bgr
        out = crop_bgr.copy()

        # A) 업스케일
        h, w = out.shape[:2]
        short = min(h, w)
        if 0 < short < TARGET_MIN_SIDE:
            scale = TARGET_MIN_SIDE / float(short)
            out = cv2.resize(
                out, (max(1, int(round(w * scale))),
                      max(1, int(round(h * scale)))),
                interpolation=cv2.INTER_CUBIC)

        # B) CLAHE on LAB L
        try:
            lab = cv2.cvtColor(out, cv2.COLOR_BGR2LAB)
            l_c, a_c, b_c = cv2.split(lab)
            clahe = cv2.createCLAHE(clipLimit=2.5, tileGridSize=(8, 8))
            l_eq = clahe.apply(l_c)
            out = cv2.cvtColor(cv2.merge([l_eq, a_c, b_c]),
                               cv2.COLOR_LAB2BGR)
        except Exception:
            pass

        # C) Unsharp mask
        try:
            g = cv2.GaussianBlur(out, (0, 0), sigmaX=1.5)
            out = cv2.addWeighted(out, 1.5, g, -0.5, 0)
        except Exception:
            pass
        return out

    # ---------- 인식 ----------

    def recognize(self, crop_image_bytes: bytes) -> Tuple[str, float]:
        img = self._decode(crop_image_bytes)
        if img is None:
            return ("각인없음", 0.0)
        return self.recognize_array(img)

    @staticmethod
    def _variants(crop_bgr: np.ndarray):
        """⭐ R3 — 다변형 전처리. 음각/양각 각인은 한 가지 전처리로
        안정적이지 않아 여러 변형을 시도하고 best 를 채택한다."""
        base = OcrEngine._preprocess(crop_bgr)          # 업스케일+CLAHE+Unsharp
        yield ("pre", base)
        try:
            g = cv2.cvtColor(base, cv2.COLOR_BGR2GRAY)
            # 강한 CLAHE
            ce = cv2.createCLAHE(clipLimit=4.0, tileGridSize=(8, 8)).apply(g)
            yield ("clahe4", cv2.cvtColor(ce, cv2.COLOR_GRAY2BGR))
            # Otsu 이진화 + 반전본 (음각/양각 양방향)
            _, ot = cv2.threshold(ce, 0, 255,
                                  cv2.THRESH_BINARY + cv2.THRESH_OTSU)
            yield ("otsu", cv2.cvtColor(ot, cv2.COLOR_GRAY2BGR))
            yield ("otsu_inv", cv2.cvtColor(255 - ot, cv2.COLOR_GRAY2BGR))
            # adaptive threshold (국소 — 곡면 음각에 강함)
            at = cv2.adaptiveThreshold(
                ce, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
                cv2.THRESH_BINARY, 31, 5)
            yield ("adapt", cv2.cvtColor(at, cv2.COLOR_GRAY2BGR))
            # black-hat (양각/음각 그림자 강조)
            k = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (9, 9))
            bh = cv2.morphologyEx(ce, cv2.MORPH_BLACKHAT, k)
            bh = cv2.normalize(bh, None, 0, 255, cv2.NORM_MINMAX)
            yield ("blackhat", cv2.cvtColor(bh, cv2.COLOR_GRAY2BGR))
        except Exception:
            pass

    def recognize_array(self, crop_bgr: np.ndarray) -> Tuple[str, float]:
        """numpy BGR crop → 각인 텍스트, 평균 신뢰도.

        ⭐ R3 — 다변형 전처리 중 best 채택. 점수 = Σ(conf) (라인이
        많고 신뢰도 높은 결과 선호). 너무 긴 잡음 방지 위해 영숫자
        12자 초과는 감점.
        """
        if not self.is_loaded or self.engine is None:
            return ("각인없음", 0.0)
        if crop_bgr is None or crop_bgr.size == 0:
            return ("각인없음", 0.0)

        best_text, best_avg, best_score = "각인없음", 0.0, -1.0
        for _name, var in self._variants(crop_bgr):
            lines = self._ocr_one(var, min_confidence=0.30)
            if not lines:
                continue
            sl = sorted(lines, key=lambda l: l.confidence, reverse=True)
            text = " ".join(l.text for l in sl).strip()
            alnum = "".join(ch for ch in text if ch.isalnum())
            if not alnum:
                continue
            avg = sum(l.confidence for l in lines) / max(len(lines), 1)
            score = sum(l.confidence for l in lines)
            if len(alnum) > 12:          # 잡음 과다 결과 감점
                score *= 0.3
            if score > best_score:
                best_score, best_text, best_avg = score, text, avg

        if best_score < 0:
            return ("각인없음", 0.0)
        return (best_text or "각인없음", round(best_avg, 3))

    # ---------- 내부 ----------

    def _decode(self, image_bytes: bytes) -> Optional[np.ndarray]:
        try:
            arr = np.frombuffer(image_bytes, dtype=np.uint8)
            return cv2.imdecode(arr, cv2.IMREAD_COLOR)
        except Exception:
            return None

    def _ocr_one(self, img: np.ndarray,
                 min_confidence: float = 0.25) -> List[OcrLine]:
        try:
            try:
                result = self.engine.ocr(img, cls=True)
            except TypeError:
                result = self.engine.ocr(img)
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
