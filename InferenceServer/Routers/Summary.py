"""
Summary Router — POST /summary/non-medical

⚠️ 비의료 영역 한정 (e약은요 효능·복용법 자연어 요약). DUR 위험 안내 영역 사용 금지.
"""
from fastapi import APIRouter, HTTPException

from Schemas.SummarySchema import NonMedicalSummaryRequest, NonMedicalSummaryResponse
from Llm.NonMedicalSummarizer import NonMedicalSummarizer

router = APIRouter(prefix="/summary", tags=["Summary"])


@router.post("/non-medical", response_model=NonMedicalSummaryResponse)
async def non_medical(request: NonMedicalSummaryRequest) -> NonMedicalSummaryResponse:
    """
    e약은요 비위험 정보를 자연어로 요약 (TTS 출력용).

    ⚠️ 의료 안내(DUR 위험 안내) 데이터는 본 엔드포인트에 절대 입력 금지.
    """
    # TODO (영역 A 분담):
    #   1. request.source 검증 — 의료 안내 키워드 차단 ("부작용", "금기" 등 입력 거부)
    #   2. LLM 호출 (RAG 허용)
    #   3. 단정 표현 출력 검증 ("복용 가능합니다" 등 reject)
    #   4. source_note (비단정·상담 권유) 자동 첨부
    raise HTTPException(status_code=501, detail="NOT_IMPLEMENTED")
