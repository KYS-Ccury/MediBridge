"""
Onboarding Router — POST /onboarding/normalize, /disambiguate

✅ RAG 허용 영역 (등록 단계). 의료 안내 영역 진입 X.
"""
from fastapi import APIRouter
from loguru import logger

from Schemas.OnboardingSchema import (
    NormalizeRequest, NormalizeResponse,
    DisambiguateRequest, DisambiguateResponse,
)
from Llm.OnboardingNormalizer import OnboardingNormalizer
from Llm.OnboardingDisambiguator import OnboardingDisambiguator

router = APIRouter(prefix="/onboarding", tags=["Onboarding"])


@router.post("/normalize", response_model=NormalizeResponse)
async def normalize(request: NormalizeRequest) -> NormalizeResponse:
    """
    폰 STT 결과 약명을 정규화 + 식약처 캐시 검색 후보 반환.
    """
    logger.info(f"[Onboarding] /normalize raw='{request.raw_text[:60]}...'")
    return OnboardingNormalizer.instance().normalize(request.raw_text)


@router.post("/disambiguate", response_model=DisambiguateResponse)
async def disambiguate(request: DisambiguateRequest) -> DisambiguateResponse:
    """
    동명·동성분 약 후보가 여럿일 때 LLM이 분기 질문 생성.
    예: "예전에 드셨던 OO약(혈압약)인가요? 최근 드시는 OO약(진통제)인가요?"
    """
    logger.info(f"[Onboarding] /disambiguate candidates={len(request.candidates)}")
    question, options = OnboardingDisambiguator.instance().generate_question(
        request.candidates
    )
    # 비어있으면 기본 fallback
    if not question:
        question = "후보가 여러 개입니다. 정확한 약을 선택해주세요."
        options = [c.drug_name for c in request.candidates]
    return DisambiguateResponse(question=question, options=options)
