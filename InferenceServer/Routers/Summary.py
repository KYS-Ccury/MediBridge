"""
Summary Router — POST /summary/non-medical

⚠️ 비의료 영역 한정 (e약은요 효능·복용법 자연어 요약). DUR 위험 안내 영역 사용 금지.
"""
from fastapi import APIRouter, HTTPException
from loguru import logger

from Schemas.SummarySchema import NonMedicalSummaryRequest, NonMedicalSummaryResponse
from Llm.NonMedicalSummarizer import NonMedicalSummarizer

router = APIRouter(prefix="/summary", tags=["Summary"])


@router.post("/non-medical", response_model=NonMedicalSummaryResponse)
async def non_medical(request: NonMedicalSummaryRequest) -> NonMedicalSummaryResponse:
    """
    e약은요 비위험 정보를 자연어로 요약 (TTS 출력용).

    ⚠️ 의료 안내(DUR 위험 안내) 데이터는 본 엔드포인트에 절대 입력 금지.
    """
    logger.info(f"[Summary] /non-medical style={request.style} source_keys={list(request.source.keys())}")
    try:
        summary = NonMedicalSummarizer.instance().summarize(
            source=request.source,
            style=request.style,
        )
    except ValueError as e:
        # 의료 안내 키워드 포함 입력 — 정책 위반
        logger.warning(f"[Summary] 입력 거부 — {e}")
        raise HTTPException(status_code=400, detail=str(e))

    return NonMedicalSummaryResponse(summary_text=summary)
