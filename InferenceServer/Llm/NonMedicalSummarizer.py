"""
NonMedicalSummarizer — e약은요 비위험 정보 자연어 요약

⚠️ 비의료 영역 한정 (효능·복용법 등). DUR 위험 안내·부작용·금기는 절대 입력 금지.
✅ RAG 허용. 출력 검증으로 단정 표현 차단.
"""
from typing import Optional, Dict, Any
from loguru import logger


# 의료 안내 영역 키워드 — 입력 source 에 포함되면 거부
MEDICAL_GUIDANCE_KEYWORDS = [
    "부작용", "금기", "병용금기", "임부금기",
    "노인주의", "용량주의", "투여기간주의",
]

# 단정 표현 패턴 — LLM 출력에 포함되면 reject
ASSERTIVE_OUTPUT_PATTERNS = [
    "복용 가능합니다", "복용 불가합니다",
    "복용해도 됩니다", "복용하지 마세요",
    "안전합니다", "위험하지 않습니다",
]


class NonMedicalSummarizer:
    _instance: Optional["NonMedicalSummarizer"] = None

    @classmethod
    def instance(cls) -> "NonMedicalSummarizer":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.model = None
        self.is_loaded: bool = False

    def load_model(self) -> None:
        """LLM 클라이언트 로딩"""
        # TODO (영역 A 분담): LLM 클라이언트 초기화
        logger.info("[NonMedicalSummarizer] load_model TODO")
        self.is_loaded = False

    def summarize(self, source: Dict[str, Any], style: str = "concise") -> str:
        """
        e약은요 비위험 정보를 TTS 출력 친화적 자연어로 요약.

        ⚠ source 에 의료 안내 키워드 포함 시 ValueError.
        ⚠ LLM 출력에 단정 표현 포함 시 reject.
        """
        # 1. 입력 검증 — 의료 안내 키워드 차단
        self._validate_input(source)

        # 2. LLM 호출 (TODO)
        # TODO (영역 A 분담):
        #   - 시스템 프롬프트: 비단정 톤 + 약사·의사 상담 권유 강제
        #   - source 의 efficacy_text, usage_text 만 사용
        summary_text = ""

        # 3. 출력 검증 — 단정 표현 reject
        if self._contains_assertive(summary_text):
            logger.warning(f"[NonMedicalSummarizer] 단정 표현 감지 → reject: {summary_text[:80]}")
            return ""

        return summary_text

    @staticmethod
    def _validate_input(source: Dict[str, Any]) -> None:
        """source 에 의료 안내 키워드 포함 여부 검증"""
        flat_text = " ".join(
            str(v) for v in source.values() if isinstance(v, (str, int, float))
        )
        for keyword in MEDICAL_GUIDANCE_KEYWORDS:
            if keyword in flat_text:
                raise ValueError(
                    f"NonMedicalSummarizer: 의료 안내 키워드 '{keyword}' 포함된 source 입력 금지"
                )

    @staticmethod
    def _contains_assertive(text: str) -> bool:
        """단정 표현 패턴 검사"""
        return any(pattern in text for pattern in ASSERTIVE_OUTPUT_PATTERNS)
