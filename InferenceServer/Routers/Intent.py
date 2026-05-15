"""
Intent Router — POST /intent/classify

⚠️ Stage 0.5 — 카테고리 분류만, 응답 생성 X (FR-A5-02).
JSON 스키마 강제로 자연어 응답 구조적 차단.
"""
from fastapi import APIRouter
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
    logger.info(f"[Intent] /classify text='{request.text[:50]}...'")
    return IntentClassifier.instance().classify(
        text=request.text,
        image_request_id=request.image_request_id,
    )
