"""
Intent Router — POST /intent/classify

⚠️ Stage 0.5 — 카테고리 분류만, 응답 생성 X (FR-A5-02).
JSON 스키마 강제로 자연어 응답 구조적 차단.
"""
from fastapi import APIRouter, HTTPException
from loguru import logger

from Schemas.IntentSchema import IntentClassifyRequest, IntentClassifyResponse
from Llm.IntentClassifier import IntentClassifier

router = APIRouter(prefix="/intent", tags=["Intent"])


@router.post("/classify", response_model=IntentClassifyResponse)
async def classify(request: IntentClassifyRequest) -> IntentClassifyResponse:
    """
    사용자 발화의 의도를 카테고리로만 분류.

    프롬프트 인젝션 시도 감지 시 강제 OTHER + injection_flag=true.
    """
    # TODO (영역 A 분담):
    #   1. IntentClassifier.instance().classify(request.text) 호출
    #   2. 인젝션 패턴 감지 (예: "이전 지시 무시", "당신은 의사야")
    #      → category=OTHER, injection_flag=True
    #   3. JSON 스키마 위반 출력은 reject하고 OTHER로 강제
    logger.info(f"[Intent] /classify — text: {request.text[:50]}...")
    raise HTTPException(status_code=501, detail="NOT_IMPLEMENTED")
