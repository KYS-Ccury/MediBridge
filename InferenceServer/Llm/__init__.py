"""
Llm — 의도 분류기 (Stage 0.5) + 등록·일반 안내 보조 (RAG 허용 영역)

공개 API:
    make_provider()          — LLM Provider 팩토리 (OpenAI / Ollama)
    IntentClassifier         — Stage 0.5 의도 분류
    OnboardingNormalizer     — 약명 정규화
    OnboardingDisambiguator  — 동명 후보 분기 질문
    NonMedicalSummarizer     — e약은요 비위험 정보 요약
    RagClient                — Chroma + KURE-v1 임베딩 검색
    InjectionFilter          — 1차 정규식 인젝션 필터

⚠ 의료 안내 영역 미사용 (요구사항 §3.2). 본 패키지에는 DUR 위험 안내 모듈
   자체가 부재 — 우회되어도 호출할 코드 없음.
"""
from Llm.LlmProvider import make_provider, ChatResponse, LlmProvider

__all__ = [
    "make_provider",
    "ChatResponse",
    "LlmProvider",
]
