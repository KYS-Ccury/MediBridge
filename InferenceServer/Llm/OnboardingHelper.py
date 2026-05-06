"""
OnboardingHelper — 등록·비의료 일반 안내 보조 (RAG 허용 영역)

✅ 약명 정규화·동명 분기 질문·e약은요 자연어 요약
❌ DUR 위험 안내·진단성 응답 — 절대 호출 X
"""
from typing import List, Optional
from loguru import logger

from Schemas.OnboardingSchema import DrugCandidate


class OnboardingHelper:
    _instance: Optional["OnboardingHelper"] = None

    @classmethod
    def instance(cls) -> "OnboardingHelper":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.model = None
        self.rag_index = None    # 벡터 DB 인덱스 (RAG)

    def load_model(self) -> None:
        """LLM + RAG 인덱스 로딩"""
        # TODO (영역 A 분담):
        #   - LLM 클라이언트 초기화 (의도 분류기와 별도 인스턴스 가능)
        #   - 식약처 데이터 임베딩 인덱스 로드 (FAISS, ChromaDB 등)
        logger.info("[OnboardingHelper] load_model TODO")

    def normalize_drug_name(self, raw_text: str) -> List[DrugCandidate]:
        """
        폰 STT 결과 약명 → 정규화 + 식약처 캐시 후보 검색.
        """
        # TODO (영역 A 분담):
        #   1. RAG 검색 — 식약처 낱알식별 캐시에서 유사 약명 후보 추출
        #   2. LLM 정규화 — 발음 오류·띄어쓰기 보정
        #   3. DrugCandidate 리스트 반환
        return []

    def generate_disambiguation_question(
        self, candidates: List[DrugCandidate]
    ) -> tuple[str, List[str]]:
        """
        동명·동성분 약 후보가 여럿일 때 분기 질문 생성.
        예: "예전에 드셨던 OO약(혈압약)인가요? 최근 드시는 OO약(진통제)인가요?"

        Returns:
            (질문 텍스트, 선택지 리스트)
        """
        # TODO (영역 A 분담):
        #   - LLM 프롬프트: 후보 리스트 → 한국어 친화적 질문 + 선택지
        #   - 의료 안내 영역 진입 차단 (단정 표현 출력 reject)
        return ("", [])

    def summarize_non_medical(self, source: dict, style: str = "concise") -> str:
        """
        e약은요 비위험 정보 → TTS 출력 친화적 자연어 요약.

        ⚠ source 에 의료 안내 키워드("부작용", "금기") 포함 시 거부.
        """
        # TODO (영역 A 분담):
        #   - 입력 검증: 의료 안내 키워드 차단
        #   - LLM 호출 (RAG 허용)
        #   - 출력 검증: 단정 표현 reject ("복용 가능합니다" 등)
        return ""
