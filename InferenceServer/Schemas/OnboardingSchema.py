"""
OnboardingSchema — Onboarding API (POST /onboarding/normalize, /disambiguate)

✅ RAG 허용 영역 (등록 단계). 의료 안내 영역으로 진입 X.
"""
from typing import List, Optional
from pydantic import BaseModel, Field


class NormalizeRequest(BaseModel):
    """POST /onboarding/normalize — 폰 STT 결과 약명 정규화"""
    raw_text: str = Field(min_length=1, max_length=200)


class DrugCandidate(BaseModel):
    """정규화된 약명 후보 1건"""
    item_code: Optional[str] = None
    drug_name: str
    confidence: float = Field(ge=0.0, le=1.0)
    note: Optional[str] = None


class NormalizeResponse(BaseModel):
    """POST /onboarding/normalize 응답"""
    candidates: List[DrugCandidate]


class DisambiguateRequest(BaseModel):
    """POST /onboarding/disambiguate — 동명·동성분 분기 질문 생성"""
    candidates: List[DrugCandidate]


class DisambiguateResponse(BaseModel):
    """POST /onboarding/disambiguate 응답"""
    question: str = Field(description="사용자에게 물어볼 한국어 질문")
    options: List[str] = Field(description="선택지 (TTS 출력용)")
