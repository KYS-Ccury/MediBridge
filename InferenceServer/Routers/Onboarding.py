"""
Onboarding Router — POST /onboarding/normalize, /disambiguate

✅ RAG 허용 영역 (등록 단계). 의료 안내 영역 진입 X.
"""
from fastapi import APIRouter, HTTPException

from Schemas.OnboardingSchema import (
    NormalizeRequest, NormalizeResponse,
    DisambiguateRequest, DisambiguateResponse,
)
from Llm.OnboardingHelper import OnboardingHelper

router = APIRouter(prefix="/onboarding", tags=["Onboarding"])


@router.post("/normalize", response_model=NormalizeResponse)
async def normalize(request: NormalizeRequest) -> NormalizeResponse:
    """
    폰 STT 결과 약명을 정규화 + 식약처 캐시 검색 후보 반환.
    """
    # TODO (영역 A 분담):
    #   1. OnboardingHelper.instance().normalize_drug_name(request.raw_text)
    #   2. 식약처 낱알식별 캐시(메인서버 경유) 검색
    #   3. NormalizeResponse 반환
    raise HTTPException(status_code=501, detail="NOT_IMPLEMENTED")


@router.post("/disambiguate", response_model=DisambiguateResponse)
async def disambiguate(request: DisambiguateRequest) -> DisambiguateResponse:
    """
    동명·동성분 약 후보가 여럿일 때 LLM이 분기 질문 생성.
    예: "예전에 드셨던 OO약(혈압약)인가요? 최근 드시는 OO약(진통제)인가요?"
    """
    # TODO (영역 A 분담):
    #   1. OnboardingHelper.instance().generate_disambiguation_question(...)
    #   2. 한국어 친화적 자연어 질문 + 선택지 리스트
    raise HTTPException(status_code=501, detail="NOT_IMPLEMENTED")
