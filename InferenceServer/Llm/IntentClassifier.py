"""
IntentClassifier — Stage 0.5 의도 분류기 (싱글톤)

⚠️ 분류만 수행. 응답 생성·의료 안내 영역 진입 X (FR-A5).
JSON 스키마 강제로 자연어 응답을 구조적으로 차단.
"""
import re
from typing import Optional
from loguru import logger

from Schemas.IntentSchema import IntentCategory, IntentClassifyResponse


# 프롬프트 인젝션 시도 패턴 (FR-A5-03)
INJECTION_PATTERNS = [
    r"이전\s*지시.*무시",
    r"당신은\s*(?:이제\s*)?의사",
    r"system\s*prompt",
    r"jailbreak",
    r"forget\s+previous",
    r"ignore\s+(?:all\s+)?previous",
]


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

    def classify(self, text: str, image_request_id: Optional[str] = None) -> IntentClassifyResponse:
        """
        사용자 발화의 의도를 카테고리로만 분류.

        Returns:
            IntentClassifyResponse — 카테고리 + 신뢰도 + injection_flag
        """
        # 1. 인젝션 패턴 1차 검사 (LLM 호출 전)
        if self._detect_injection(text):
            logger.warning(f"[IntentClassifier] 인젝션 시도 감지: {text[:80]}")
            return IntentClassifyResponse(
                category=IntentCategory.OTHER,
                confidence=1.0,
                injection_flag=True,
            )

        # 2. LLM 호출
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

    @staticmethod
    def _detect_injection(text: str) -> bool:
        """간단한 정규식 기반 인젝션 패턴 감지 (1차 필터)"""
        lowered = text.lower()
        for pattern in INJECTION_PATTERNS:
            if re.search(pattern, lowered, re.IGNORECASE):
                return True
        return False
