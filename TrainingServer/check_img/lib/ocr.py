"""각인 OCR — PaddleOCR + 전처리 강화 (업스케일·CLAHE·샤프닝).

알약 각인은 일반 텍스트와 달리 작고·음각이라 그림자 대비가 약하고·곡면 위에
있어 PaddleOCR 사전학습 모델이 곧바로 잘 잡지 못함. 본 모듈은 다음 3가지
전처리로 가독성을 보강:

  A) 업스케일 — 짧은 변이 240px 미만이면 INTER_CUBIC 으로 확대
  B) CLAHE   — LAB L 채널 로컬 콘트라스트 향상 (음각 그림자 강조)
  C) Unsharp — 가우시안 블러 빼기 방식 샤프닝 (글자 윤곽 선명화)

또한 언어 모델을 'korean' → **'en'** 으로 변경. 의약품 각인은 거의 영문·숫자.
첫 호출 시 신 모델(~수십 MB) 자동 다운로드.

POC 가정: 각인이 어느 정도 보이는 사진. 흐릿한 미세 각인은 fine-tuning 필요.
"""

from __future__ import annotations
from dataclasses import dataclass
from typing import List, Optional

import cv2
import numpy as np


@dataclass
class OcrLine:
    text:       str
    confidence: float


_paddle_ocr = None              # lazy 초기화
TARGET_MIN_SIDE = 240           # 업스케일 기준 (짧은 변 px)


def _get_ocr():
    """PaddleOCR — 영문/숫자 모델 lazy 생성. 첫 호출 시 모델 다운로드 발생."""
    global _paddle_ocr
    if _paddle_ocr is None:
        from paddleocr import PaddleOCR
        # use_angle_cls=True : 90/180도 회전된 텍스트도 인식
        # lang='en'          : 알약 각인은 거의 영문·숫자 — 한글 모델보다 정확·경량
        try:
            _paddle_ocr = PaddleOCR(use_angle_cls=True, lang='en', show_log=False)
        except TypeError:
            # 신 버전 paddleocr 는 show_log 인자 미지원
            _paddle_ocr = PaddleOCR(use_angle_cls=True, lang='en')
    return _paddle_ocr


def _preprocess_for_ocr(crop_bgr: np.ndarray) -> np.ndarray:
    """A) 업스케일 + B) CLAHE + C) Unsharp — 각인 가독성 향상."""
    if crop_bgr is None or crop_bgr.size == 0:
        return crop_bgr

    out = crop_bgr.copy()

    # A) 업스케일 — 짧은 변이 240px 미만이면 INTER_CUBIC 확대
    h, w = out.shape[:2]
    short_side = min(h, w)
    if 0 < short_side < TARGET_MIN_SIDE:
        scale = TARGET_MIN_SIDE / float(short_side)
        new_w = max(1, int(round(w * scale)))
        new_h = max(1, int(round(h * scale)))
        out = cv2.resize(out, (new_w, new_h), interpolation=cv2.INTER_CUBIC)

    # B) CLAHE — LAB L 채널 로컬 콘트라스트 향상
    lab = cv2.cvtColor(out, cv2.COLOR_BGR2LAB)
    l_chan, a_chan, b_chan = cv2.split(lab)
    clahe = cv2.createCLAHE(clipLimit=2.5, tileGridSize=(8, 8))
    l_eq = clahe.apply(l_chan)
    enhanced = cv2.cvtColor(cv2.merge([l_eq, a_chan, b_chan]), cv2.COLOR_LAB2BGR)

    # C) Unsharp mask — 가우시안 블러 빼기 방식
    gaussian = cv2.GaussianBlur(enhanced, (0, 0), sigmaX=1.5)
    sharpened = cv2.addWeighted(enhanced, 1.5, gaussian, -0.5, 0)

    return sharpened


def recognize_engraving(crop_bgr: np.ndarray,
                        min_confidence: float = 0.25) -> List[OcrLine]:
    """알약 crop 이미지 → 전처리 → PaddleOCR 각인 텍스트 추출.

    Args:
        crop_bgr:        OpenCV BGR crop
        min_confidence:  이 값 이하의 인식 결과는 버림 (0.25 권장 — 각인은 confidence 가
                         일반 텍스트보다 낮은 경향)

    Returns:
        OcrLine 리스트. 인식 실패 시 빈 리스트.
    """
    if crop_bgr is None or crop_bgr.size == 0:
        return []

    preprocessed = _preprocess_for_ocr(crop_bgr)

    ocr = _get_ocr()
    try:
        result = ocr.ocr(preprocessed, cls=True)
    except TypeError:
        # 신 버전 paddleocr 는 cls 인자 제거됨
        result = ocr.ocr(preprocessed)
    except Exception:
        return []

    lines: List[OcrLine] = []
    if not result or result[0] is None:
        return lines

    for entry in result[0]:
        if entry is None or len(entry) < 2:
            continue
        text_part = entry[1]
        if not isinstance(text_part, (list, tuple)) or len(text_part) < 2:
            continue
        text, conf = text_part[0], float(text_part[1])
        if conf < min_confidence:
            continue
        lines.append(OcrLine(text=str(text).strip(), confidence=conf))

    return lines


def join_lines(lines: List[OcrLine]) -> str:
    """OCR 라인들을 신뢰도 순으로 결합한 단일 문자열 (사람 가독용)."""
    if not lines:
        return ""
    sorted_lines = sorted(lines, key=lambda l: l.confidence, reverse=True)
    return " | ".join(f"{l.text}({l.confidence:.2f})" for l in sorted_lines)
