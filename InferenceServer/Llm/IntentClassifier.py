"""
IntentClassifier — Stage 0.5 의도 분류기 (싱글톤)

⚠️ 분류만 수행. 응답 생성·의료 안내 영역 진입 X (FR-A5).
JSON 스키마 강제로 자연어 응답을 구조적으로 차단.

흐름:
    1. InjectionFilter.detect() — 1차 정규식 필터 → 즉시 OTHER 강제
    2. 미감지 시 LlmProvider.chat(format="json") 호출
    3. JSON 파싱 → IntentCategory enum 검증
    4. 스키마 위반 → OTHER 강제
"""
from typing import Optional

from loguru import logger

from Schemas.IntentSchema import IntentCategory, IntentClassifyResponse
from Llm.InjectionFilter import InjectionFilter
from Llm.LlmProvider import make_provider
from Llm.PromptTemplates import INTENT_SYSTEM


class IntentClassifier:
    _instance: Optional["IntentClassifier"] = None

    @classmethod
    def instance(cls) -> "IntentClassifier":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self._provider = None
        self.is_loaded: bool = False

    def load_model(self) -> None:
        """LlmProvider 초기화 (싱글톤 — 이미 생성됐으면 그대로)."""
        try:
            self._provider = make_provider()
            self.is_loaded = True
            logger.info(
                f"[IntentClassifier] ready — provider={self._provider.name}"
                f" model={self._provider.model}"
            )
        except Exception as e:
            logger.exception(f"[IntentClassifier] load_model 실패: {e}")
            self.is_loaded = False

    def classify(
        self, text: str, image_request_id: Optional[str] = None
    ) -> IntentClassifyResponse:
        """
        사용자 발화의 의도를 카테고리로만 분류.
        """
        # 1) 인젝션 1차 필터
        if InjectionFilter.detect(text):
            matched = InjectionFilter.matched_patterns(text)
            logger.warning(
                f"[IntentClassifier] 인젝션 패턴 감지 — text: {text[:80]} matched: {matched}"
            )
            return IntentClassifyResponse(
                category=IntentCategory.OTHER,
                confidence=1.0,
                injection_flag=True,
            )

        # 2) Provider 미준비 시 fallback (개발 환경 가정)
        if not self.is_loaded or self._provider is None:
            logger.warning("[IntentClassifier] provider 미준비 — OTHER 반환")
            return IntentClassifyResponse(
                category=IntentCategory.OTHER,
                confidence=0.0,
                injection_flag=False,
            )

        # 3) LLM 호출 (JSON 강제)
        resp = self._provider.chat(
            system=INTENT_SYSTEM,
            user=text,
            format="json",
        )
        if resp.error or resp.json_obj is None:
            logger.warning(
                f"[IntentClassifier] LLM 응답 오류 err={resp.error} text={resp.text[:120]}"
            )
            return IntentClassifyResponse(
                category=IntentCategory.OTHER,
                confidence=0.0,
                injection_flag=False,
            )

        # 4) 스키마 검증
        try:
            category_str = str(resp.json_obj.get("category", "OTHER")).upper()
            confidence = float(resp.json_obj.get("confidence", 0.5))
            try:
                category = IntentCategory(category_str)
            except ValueError:
                logger.warning(
                    f"[IntentClassifier] 카테고리 enum 위반: {category_str} → OTHER"
                )
                category = IntentCategory.OTHER
                confidence = 0.0
            confidence = max(0.0, min(1.0, confidence))
            logger.info(
                f"[IntentClassifier] result={category.value} conf={confidence:.2f}"
                f" latency={resp.latency_ms}ms"
            )
            return IntentClassifyResponse(
                category=category,
                confidence=confidence,
                injection_flag=False,
            )
        except Exception as e:
            logger.exception(f"[IntentClassifier] 응답 파싱 실패: {e}")
            return IntentClassifyResponse(
                category=IntentCategory.OTHER,
                confidence=0.0,
                injection_flag=False,
            )
