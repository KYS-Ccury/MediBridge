"""
OnboardingNormalizer — 폰 STT 약명 정규화 + 식약처 캐시 후보 검색

✅ RAG 허용 영역 (등록 단계, 비의료).
"""
from typing import List, Optional
from loguru import logger

from Schemas.OnboardingSchema import DrugCandidate


class OnboardingNormalizer:
    _instance: Optional["OnboardingNormalizer"] = None

    @classmethod
    def instance(cls) -> "OnboardingNormalizer":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.model = None
        self.rag_index = None    # 식약처 낱알식별 임베딩 인덱스
        self.is_loaded: bool = False

    def load_model(self) -> None:
        """LLM + 식약처 데이터 RAG 인덱스 로딩"""
        # TODO (영역 A 분담):
        #   - LLM 클라이언트 초기화
        #   - 식약처 낱알식별 임베딩 인덱스 로드 (FAISS, ChromaDB 등)
        logger.info("[OnboardingNormalizer] load_model TODO")
        self.is_loaded = False

    def normalize(self, raw_text: str) -> List[DrugCandidate]:
        """
        STT 결과 약명 → 정규화 + 식약처 캐시 검색 후보.

        Args:
            raw_text: 폰 STT 결과 (발음 오류·띄어쓰기 가능)

        Returns:
            식약처 매칭 약명 후보 리스트
        """
        # TODO (영역 A 분담):
        #   1. RAG 검색 — 식약처 낱알식별 캐시에서 유사 약명 후보 추출
        #   2. LLM 정규화 — 발음·띄어쓰기 보정
        #   3. DrugCandidate 리스트 반환
        return []
