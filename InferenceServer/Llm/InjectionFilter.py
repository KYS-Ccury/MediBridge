"""
InjectionFilter — 프롬프트 인젝션 시도 패턴 1차 필터 (FR-A5-03 / FR-B6-01)

본 필터는 LLM 호출 전 정규식 기반으로 명백한 인젝션 시도를 즉시 차단.
LLM 호출 비용·환각 노출을 줄임. 우회된 시도는 LLM의 JSON 스키마 강제 + 영역 분리로 2차 차단.
"""
import re
from typing import List


# 프롬프트 인젝션 시도 패턴 (한국어·영어)
# 본 목록은 보수적으로 시작하고, 운용 중 의심 로그 분석으로 확장.
INJECTION_PATTERNS: List[str] = [
    # 한국어
    r"이전\s*지시.*무시",
    r"이전\s*명령.*무시",
    r"당신은\s*(?:이제\s*)?의사",
    r"당신은\s*(?:이제\s*)?약사",
    r"진단\s*(?:해줘|해주세요)",

    # 영어
    r"forget\s+(?:all\s+)?previous",
    r"ignore\s+(?:all\s+)?previous",
    r"system\s*prompt",
    r"jailbreak",
    r"you\s+are\s+(?:now\s+)?(?:a\s+)?doctor",
    r"act\s+as\s+(?:a\s+)?doctor",
]


class InjectionFilter:
    """프롬프트 인젝션 패턴 감지기"""

    @classmethod
    def detect(cls, text: str) -> bool:
        """
        명백한 인젝션 시도 패턴이 감지되면 True 반환.

        Args:
            text: 사용자 발화 텍스트

        Returns:
            True 면 IntentClassifier 가 OTHER + injection_flag=True 강제
        """
        if not text:
            return False
        lowered = text.lower()
        for pattern in INJECTION_PATTERNS:
            if re.search(pattern, lowered, re.IGNORECASE):
                return True
        return False

    @classmethod
    def matched_patterns(cls, text: str) -> List[str]:
        """디버깅·로깅용 — 감지된 패턴 목록 반환"""
        if not text:
            return []
        lowered = text.lower()
        return [
            pattern for pattern in INJECTION_PATTERNS
            if re.search(pattern, lowered, re.IGNORECASE)
        ]
