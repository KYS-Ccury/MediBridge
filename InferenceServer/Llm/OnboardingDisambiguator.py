"""
OnboardingDisambiguator — 동명·동성분 약 분기 질문 생성 (싱글톤)

✅ RAG 허용 영역. 의료 안내 영역(단정 표현) 진입 X.
"""
import json as _json
from typing import List, Optional, Tuple

from loguru import logger

from Schemas.OnboardingSchema import DrugCandidate
from Llm.LlmProvider import make_provider
from Llm.PromptTemplates import DISAMBIGUATE_SYSTEM
from Llm.OutputSanitizer import contains_banned


class OnboardingDisambiguator:
    _instance: Optional["OnboardingDisambiguator"] = None

    @classmethod
    def instance(cls) -> "OnboardingDisambiguator":
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
            logger.info(f"[OnboardingDisambiguator] ready — provider={self._provider.name}")
        except Exception as e:
            logger.exception(f"[OnboardingDisambiguator] load_model 실패: {e}")
            self.is_loaded = False

    def generate_question(
        self, candidates: List[DrugCandidate]
    ) -> Tuple[str, List[str]]:
        """
        동명·동성분 약 후보 → 사용자 친화적 분기 질문 + 선택지.

        Returns:
            (한국어 질문, 선택지 리스트)
        """
        if not candidates or len(candidates) < 2:
            return ("", [])
        if not self.is_loaded or self._provider is None:
            return ("", [])

        # 후보 list 직렬화 — LLM 입력
        cand_payload = _json.dumps(
            [{
                "item_code": c.item_code or "",
                "drug_name": c.drug_name,
                "note": c.note or "",
            } for c in candidates],
            ensure_ascii=False,
        )

        resp = self._provider.chat(
            system=DISAMBIGUATE_SYSTEM,
            user=cand_payload,
            format="json",
        )
        if resp.error or resp.json_obj is None:
            logger.warning(f"[OnboardingDisambiguator] LLM 오류 err={resp.error}")
            return ("", [])

        try:
            question = str(resp.json_obj.get("question", "")).strip()
            options_raw = resp.json_obj.get("options", [])
            options = [str(o).strip() for o in options_raw if str(o).strip()]

            # 단정 표현 검사
            banned = contains_banned(question)
            if banned:
                logger.warning(f"[OnboardingDisambiguator] 단정 표현 거부: '{banned}'")
                return ("", [])

            logger.info(
                f"[OnboardingDisambiguator] q='{question[:40]}...' opts={len(options)}"
                f" latency={resp.latency_ms}ms"
            )
            return (question, options)
        except Exception as e:
            logger.exception(f"[OnboardingDisambiguator] 파싱 실패: {e}")
            return ("", [])
