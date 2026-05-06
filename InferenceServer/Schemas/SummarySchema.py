"""
SummarySchema — Summary API (POST /summary/non-medical)

⚠️ 비의료 영역 한정 (e약은요 효능·복용법 자연어 요약).
의료 안내(DUR 위험 안내) 영역에는 절대 사용 금지.
"""
from typing import Dict, Any
from pydantic import BaseModel, Field


class NonMedicalSummaryRequest(BaseModel):
    """POST /summary/non-medical 요청"""
    source: Dict[str, Any] = Field(
        description="식약처 e약은요 비위험 정보 원문 (efficacy_text, usage_text 등)"
    )
    style: str = Field(default="concise", description="concise / detailed")


class NonMedicalSummaryResponse(BaseModel):
    """POST /summary/non-medical 응답 — TTS 출력 친화적 텍스트"""
    summary_text: str = Field(description="자연어 요약문 (의료 안내 톤 X)")
    source_note: str = Field(
        default="본 정보는 식약처 e약은요 데이터에서 인용한 것이며, 단정적 안내가 아닙니다. 약사·의사 상담을 권유드립니다."
    )
