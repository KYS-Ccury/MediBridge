"""
VisionSchema — Vision API 요청/응답 (POST /vision/detect, /vision/analyze)
"""
from typing import List, Optional
from pydantic import BaseModel, Field


class BoundingBox(BaseModel):
    """알약 바운딩 박스 — YOLO26 출력 형식"""
    x: float
    y: float
    width: float
    height: float


class DetectedPill(BaseModel):
    """검출된 알약 1건"""
    bbox: BoundingBox
    crop_id: str = Field(description="후속 OCR 호출 시 참조할 ID")
    confidence: float = Field(ge=0.0, le=1.0)


class DetectResponse(BaseModel):
    """POST /vision/detect 응답"""
    detections: List[DetectedPill]
    status: str = Field(description="'ok' | 'no_pill_detected'")
    message: Optional[str] = None


class AnalyzeResponse(BaseModel):
    """POST /vision/analyze 응답 — 각인·색·모양 분석"""
    crop_id: str
    engraving_text: str
    engraving_confidence: float
    color_hsv_label: str = Field(description="식약처 매칭 키 (예: '백색', '연황색')")
    shape_label: str = Field(description="원/타원/캡슐형 등")
    size_mm: Optional[float] = None
    overall_confidence: float
