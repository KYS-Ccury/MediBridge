"""
OnboardingDisambiguator — 동명·동성분 약 분기 질문 생성

✅ RAG 허용 영역. 의료 안내 영역(단정 표현) 진입 X.
"""
from typing import List, Optional, Tuple
from loguru import logger

from Schemas.OnboardingSchema import DrugCandidate


class OnboardingDisambiguator:
    _instance: Optional["OnboardingDisambiguator"] = None

    @classmethod
    def instance(cls) -> "OnboardingDisambiguator":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.model = None
        self.is_loaded: bool = False

    def load_model(self) -> None:
        """LLM 클라이언트 로딩"""
        # TODO (영역 A 분담): LLM 클라이언트 초기화 (Normalizer와 인스턴스 공유 가능)
        logger.info("[OnboardingDisambiguator] load_model TODO")
        self.is_loaded = False

    def generate_question(
        self, candidates: List[DrugCandidate]
    ) -> Tuple[str, List[str]]:
        """
        동명·동성분 약 후보 → 사용자 친화적 분기 질문 + 선택지.

        예: "예전에 드셨던 OO약(혈압약)인가요? 최근 드시는 OO약(진통제)인가요?"

        Returns:
            (한국어 질문, 선택지 리스트)
        """
        # TODO (영역 A 분담):
        #   1. LLM 프롬프트 — 후보 리스트 + 친화적 질문 생성 지시
        #   2. 단정 표현 출력 검증 ("복용 가능합니다" 등 reject)
        #   3. (text, options) 반환
        return ("", [])
