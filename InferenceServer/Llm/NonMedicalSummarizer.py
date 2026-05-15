"""
NonMedicalSummarizer — e약은요 비위험 정보 자연어 요약 (싱글톤)

⚠️ 비의료 영역 한정 (효능·복용법·보관법 등). 위험 정보(부작용·금기) 입력 금지.
✅ RAG 허용. OutputSanitizer 로 단정 표현 차단.
"""
import json as _json
from typing import Optional, Dict, Any

from loguru import logger

from Llm.LlmProvider import make_provider
from Llm.PromptTemplates import SUMMARY_NON_MEDICAL_SYSTEM
from Llm.OutputSanitizer import force_safe
from Llm.RagClient import RagClient


# 의료 안내 영역 키워드 — 입력 source 에 포함되면 거부
MEDICAL_GUIDANCE_KEYWORDS = [
    "부작용", "금기", "병용금기", "임부금기",
    "노인주의", "용량주의", "투여기간주의",
]

# source 에서 LLM 입력으로 허용할 비의료 필드만
ALLOWED_SECTIONS = ("efficacy_text", "usage_text", "storage_text")


class NonMedicalSummarizer:
    _instance: Optional["NonMedicalSummarizer"] = None

    @classmethod
    def instance(cls) -> "NonMedicalSummarizer":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self._provider = None
        self.is_loaded: bool = False

    def load_model(self) -> None:
        try:
            self._provider = make_provider()
            self.is_loaded = True
            logger.info(f"[NonMedicalSummarizer] ready — provider={self._provider.name}")
        except Exception as e:
            logger.exception(f"[NonMedicalSummarizer] load_model 실패: {e}")
            self.is_loaded = False

    def summarize(self, source: Dict[str, Any], style: str = "concise") -> str:
        """
        e약은요 비위험 정보를 TTS 출력 친화적 자연어로 요약.

        ⚠ source 에 의료 안내 키워드 포함 시 ValueError.
        ⚠ LLM 출력에 단정 표현 포함 시 안전 fallback 반환.
        """
        # 1. 입력 검증
        self._validate_input(source)

        if not self.is_loaded or self._provider is None:
            return "자세한 사항은 약사·의사 상담을 권유드립니다."

        # 2. 비의료 섹션만 추출
        non_medical = {
            k: source[k] for k in ALLOWED_SECTIONS
            if k in source and isinstance(source[k], str) and source[k].strip()
        }
        if not non_medical:
            return "해당 약품의 비의료 정보가 없습니다. 자세한 사항은 약사·의사 상담을 권유드립니다."

        # 3. (선택) RAG 로 유사 약품 정보 보강 — drug_name 있을 때만
        rag = RagClient.instance()
        rag_context = ""
        if rag.is_loaded() and source.get("drug_name"):
            hits = rag.search_overview(str(source["drug_name"]), n_results=2)
            if hits:
                rag_context = "\n\n[참고 본문]\n" + "\n".join(
                    str(h.get("document", ""))[:300] for h in hits if h.get("document")
                )

        user_payload = _json.dumps(non_medical, ensure_ascii=False) + rag_context

        # 4. LLM 호출 (자연어 출력 — JSON 강제 X)
        resp = self._provider.chat(
            system=SUMMARY_NON_MEDICAL_SYSTEM,
            user=user_payload,
        )
        if resp.error:
            logger.warning(f"[NonMedicalSummarizer] LLM 오류 err={resp.error}")
            return "자세한 사항은 약사·의사 상담을 권유드립니다."

        # 5. 단정 표현 차단 (OutputSanitizer)
        safe_text = force_safe(resp.text)
        logger.info(
            f"[NonMedicalSummarizer] summary len={len(safe_text)} latency={resp.latency_ms}ms"
        )
        return safe_text

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
