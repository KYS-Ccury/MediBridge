"""
IntentClassifier — Stage 0.5 의도 분류기 (싱글톤)

⚠️ 분류만 수행. 응답 생성·의료 안내 영역 진입 X (FR-A5).
JSON 스키마 강제로 자연어 응답을 구조적으로 차단.

인젝션 패턴 1차 필터는 InjectionFilter 모듈에 분리됨.
"""
from typing import Optional
from loguru import logger

from Schemas.IntentSchema import IntentCategory, IntentClassifyResponse
from Llm.InjectionFilter import InjectionFilter


class IntentClassifier:
    _instance: Optional["IntentClassifier"] = None

    @classmethod
    def instance(cls) -> "IntentClassifier":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.model = None
        self.is_loaded: bool = False

    def load_model(self) -> None:
        """LLM 모델 로딩 (모델 추후 선정)"""
        # TODO (영역 A 분담):
        #   - Ollama 클라이언트, vLLM, 또는 외부 API 호출 클라이언트 초기화
        #   - 시스템 프롬프트 락: JSON 스키마 강제 출력 지시
        logger.info("[IntentClassifier] load_model TODO")
        self.is_loaded = False

    def classify(
        self, text: str, image_request_id: Optional[str] = None
    ) -> IntentClassifyResponse:
        """
        사용자 발화의 의도를 카테고리로만 분류.

        흐름:
            1. InjectionFilter.detect() — 1차 정규식 필터
            2. 감지 시 즉시 OTHER + injection_flag=True 반환
            3. 미감지 시 LLM 호출 (JSON 스키마 강제)
            4. LLM 응답 검증 — 스키마 위반 시 OTHER 강제
        """
        # 1. 인젝션 1차 필터 (LLM 호출 전)
        if InjectionFilter.detect(text):
            matched = InjectionFilter.matched_patterns(text)
            logger.warning(
                f"[IntentClassifier] 인젝션 패턴 감지 — text: {text[:80]}, "
                f"matched: {matched}"
            )
            return IntentClassifyResponse(
                category=IntentCategory.OTHER,
                confidence=1.0,
                injection_flag=True,
            )

        # 2. LLM 호출 (TODO)
        # TODO (영역 A 분담):
        #   - 시스템 프롬프트: "사용자 발화를 다음 카테고리 중 하나로만 분류:
        #     PILL_IDENTIFY/RISK_CHECK/INFO_LOOKUP/REGISTER_REQUEST/
        #     HISTORY_QUERY/REPORT_REQUEST/OTHER. JSON {category, confidence} 만 출력."
        #   - 응답을 JSON 파싱. 스키마 위반 시 OTHER 강제
        return IntentClassifyResponse(
            category=IntentCategory.OTHER,
            confidence=0.0,
            injection_flag=False,
        )
