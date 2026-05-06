"""
IntentSchema — Intent API 요청/응답 (POST /intent/classify)

⚠️ Stage 0.5 분류기 — 응답 생성 절대 X. 카테고리 라벨만 반환 (FR-A5-02).
"""
from enum import Enum
from typing import Optional
from pydantic import BaseModel, Field


class IntentCategory(str, Enum):
    """사용자 발화 의도 카테고리 (FR-A5-01)"""
    PILL_IDENTIFY = "PILL_IDENTIFY"
    RISK_CHECK = "RISK_CHECK"
    INFO_LOOKUP = "INFO_LOOKUP"
    REGISTER_REQUEST = "REGISTER_REQUEST"
    HISTORY_QUERY = "HISTORY_QUERY"
    REPORT_REQUEST = "REPORT_REQUEST"
    OTHER = "OTHER"


class IntentClassifyRequest(BaseModel):
    """POST /intent/classify 요청"""
    text: str = Field(min_length=1, max_length=500, description="폰 STT 결과 텍스트")
    image_request_id: Optional[str] = None
    language: str = "ko-KR"


class IntentClassifyResponse(BaseModel):
    """POST /intent/classify 응답 (JSON 스키마 강제 — 자연어 X)"""
    category: IntentCategory
    confidence: float = Field(ge=0.0, le=1.0)
    injection_flag: bool = Field(
        default=False,
        description="프롬프트 인젝션 시도 감지 시 true → 강제 OTHER (FR-A5-03)"
    )
