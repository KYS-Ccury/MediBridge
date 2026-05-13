"""MediBridge POC — 알약 식별 라이브러리 모듈.

목적: 사진 한 장에서 여러 알약을 검출하고, 각 알약의
  - 각인 텍스트 (PaddleOCR)
  - 색 (HSV 분포)
  - 모양 (윤곽선 기반)
을 추출 가능한지 가능성 검증.

운영 단계로 진입하면 detection 부분은 YOLO fine-tuning 으로 교체.
"""

from .detection import detect_pills
from .ocr        import recognize_engraving
from .color      import analyze_color
from .shape      import analyze_shape
from .aihub_loader import (
    load_aihub_json, iou, best_iou_match,
    normalize_engraving, engraving_matches, label_match,
)

__all__ = [
    "detect_pills",
    "recognize_engraving",
    "analyze_color",
    "analyze_shape",
    "load_aihub_json",
    "iou", "best_iou_match",
    "normalize_engraving", "engraving_matches", "label_match",
]
