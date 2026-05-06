"""
SpeechSchema — (확장 fallback) Speech API (POST /speech/stt)

MVP는 폰 온디바이스 STT 사용 → 본 엔드포인트 호출 안 됨.
폰 STT 정확도 부족·미사용 환경에서만 사용.
"""
from pydantic import BaseModel, Field


class SttResponse(BaseModel):
    """POST /speech/stt 응답"""
    text: str
    confidence: float = Field(ge=0.0, le=1.0)
    language: str = "ko-KR"
    duration_seconds: float
