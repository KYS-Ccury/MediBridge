"""
OutputSanitizer — LLM 출력의 단정 표현 차단

정책 (요구사항 §9.3 절대 규정):
    ❌ "복용 가능합니다 / 복용 불가합니다"
    ❌ "안전합니다 / 위험합니다"
    ❌ "진단합니다 / 진단됩니다"
    ❌ "치료됩니다 / 치유됩니다"
    ✅ "추정" + "약사·의사 상담 권유" 톤

본 모듈은 LLM 응답 텍스트에 대해 위 패턴을 검출/거부/대체 한다.
의료 안내 영역 출력의 마지막 방어선 (Defense in Depth).
"""
import re
from typing import Optional

# 단정 표현 패턴 (모두 부분 매칭)
BANNED_PATTERNS: list[re.Pattern] = [
    re.compile(r"복용\s*가능합니다"),
    re.compile(r"복용\s*불가합니다"),
    re.compile(r"복용해도\s*됩니다"),
    re.compile(r"복용하시면\s*안\s*됩니다"),
    re.compile(r"안전합니다"),
    re.compile(r"위험합니다"),
    re.compile(r"진단합니다"),
    re.compile(r"진단됩니다"),
    re.compile(r"치료됩니다"),
    re.compile(r"치유됩니다"),
    re.compile(r"확실히\s*효과"),
    re.compile(r"분명히\s*효과"),
]

SAFE_SUFFIX = " 자세한 사항은 약사·의사 상담을 권유드립니다."


def contains_banned(text: str) -> Optional[str]:
    """첫 번째로 매칭된 단정 표현 반환. 없으면 None."""
    if not text:
        return None
    for pat in BANNED_PATTERNS:
        m = pat.search(text)
        if m:
            return m.group(0)
    return None


def sanitize(text: str) -> tuple[str, bool]:
    """
    단정 표현이 있으면 빈 문자열 + True 반환 (거부).
    없으면 원문 + 안전 suffix 추가 + False 반환.

    반환:
        (sanitized_text, was_rejected)
    """
    if not text:
        return "", False

    banned = contains_banned(text)
    if banned is not None:
        # 거부 — 호출자가 재생성 또는 대체 응답 선택
        return "", True

    # safe — suffix 가 이미 있으면 그대로, 없으면 추가
    if "약사" in text or "의사" in text:
        return text.strip(), False
    return text.rstrip() + SAFE_SUFFIX, False


def force_safe(text: str, *, fallback: str = "자세한 사항은 약사·의사 상담을 권유드립니다.") -> str:
    """
    안전한 출력 강제 — 단정 표현 있으면 fallback 반환.
    """
    sanitized, rejected = sanitize(text)
    if rejected:
        return fallback
    return sanitized
