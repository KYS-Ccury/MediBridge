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


# =====================================================
# /vision/detect_remote — 메인서버 호출용 (사진 본체 보관 PC GET)
# =====================================================

class DetectRemoteRequest(BaseModel):
    """
    메인서버가 호출. Vision PC 가 보관 PC 에서 직접 이미지 GET 후 처리.
    AWS S3+RDS 패턴 — 사진 본체는 메인서버 통과 X.
    """
    photo_id: str
    storage_url: str = Field(description="보관 PC GET URL (10.10.10.122:8004/...)")
    get_token: str = Field(description="HMAC GET 토큰 (Authorization Bearer)")
    mime: str = Field(default="image/jpeg")
    purpose: str = Field(default="IDENTIFY", description="IDENTIFY / REGISTER / EVIDENCE")
    request_id: Optional[str] = Field(default=None, description="추적 ID")


class DetectRemoteCandidate(BaseModel):
    """검출된 알약 1개의 통합 분석 결과 (한 사진 안의 N 후보 중 하나)."""
    crop_id: str
    bbox: BoundingBox
    detection_confidence: float
    engraving_text: str
    engraving_confidence: float
    color_label: str
    shape_label: str
    size_mm: Optional[float] = None
    # 식약처 매칭 키 종합 — 메인서버 DurChecker 등에 전달
    match_keys: list[str] = Field(default_factory=list)
    # 검출 알약 crop 썸네일 (base64 JPEG, prefix 없음) — 다중 알약 카드 구분용
    crop_jpeg_b64: Optional[str] = None


class DetectRemoteResponse(BaseModel):
    """
    Vision PC → 메인서버. 후보 list + 신뢰도 종합.

    메인서버는 본 응답의 candidates 를 식약처 낱알식별 캐시로 매칭해
    PillCandidate (item_code, drug_name) 으로 변환.
    """
    photo_id: str
    candidates: List[DetectRemoteCandidate]
    confidence_tier: str = Field(description="HIGH(95+) / MEDIUM(70-95) / LOW(70-)")
    status: str = Field(default="ok", description="ok | no_pill_detected | storage_get_failed")
    message: Optional[str] = None
