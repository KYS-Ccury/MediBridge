"""
OnboardingNormalizer — STT 약명 정규화 (싱글톤)

✅ Onboarding RAG 허용 영역 (FR-A6).
음성 인식 결과 텍스트에서 약품명만 추출·정규화.
RAG 로 추가 검증 가능 (Chroma pdma_overview).
"""
from typing import Optional

from loguru import logger

from Schemas.OnboardingSchema import NormalizeResponse, DrugCandidate
from Llm.InjectionFilter import InjectionFilter
from Llm.LlmProvider import make_provider
from Llm.PromptTemplates import NORMALIZE_SYSTEM
from Llm.RagClient import RagClient


class OnboardingNormalizer:
    _instance: Optional["OnboardingNormalizer"] = None

    @classmethod
    def instance(cls) -> "OnboardingNormalizer":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self._provider = None
        self.is_loaded: bool = False

    def load_model(self) -> None:
        try:
            self._provider = make_provider()
            # RAG 는 첫 검색 시 lazy load — 여기서는 시도만
            RagClient.instance().load()
            self.is_loaded = True
            logger.info(f"[OnboardingNormalizer] ready — provider={self._provider.name}")
        except Exception as e:
            logger.exception(f"[OnboardingNormalizer] load_model 실패: {e}")
            self.is_loaded = False

    def normalize(self, raw_text: str) -> NormalizeResponse:
        # 1) 인젝션 검사 — 약명 발화에 인젝션 시도면 빈 후보로 거부
        if InjectionFilter.detect(raw_text):
            logger.warning(f"[OnboardingNormalizer] 인젝션 의심 — 빈 응답: {raw_text[:60]}")
            return NormalizeResponse(candidates=[])

        if not self.is_loaded or self._provider is None:
            return NormalizeResponse(candidates=[])

        # 2) LLM 추출
        resp = self._provider.chat(
            system=NORMALIZE_SYSTEM,
            user=raw_text,
            format="json",
        )
        if resp.error or resp.json_obj is None:
            logger.warning(f"[OnboardingNormalizer] LLM 오류 err={resp.error}")
            return NormalizeResponse(candidates=[])

        # 3) 스키마 변환
        candidates: list[DrugCandidate] = []
        for c in resp.json_obj.get("candidates", [])[:5]:
            try:
                normalized = str(c.get("normalized", "")).strip()
                if not normalized:
                    continue
                confidence = float(c.get("confidence", 0.5))
                confidence = max(0.0, min(1.0, confidence))
                raw_phrase = str(c.get("raw_phrase", "")).strip() or None
                candidates.append(DrugCandidate(
                    drug_name=normalized,
                    confidence=confidence,
                    note=raw_phrase,
                ))
            except Exception:
                continue

        # 4) (선택) RAG 로 식약처 등재 약품인지 검증 — 일치도 낮은 후보는 confidence 감소
        rag = RagClient.instance()
        if rag.is_loaded() and candidates:
            for cand in candidates:
                hits = rag.search_overview(cand.drug_name, n_results=1)
                if hits:
                    meta = hits[0].get("metadata") or {}
                    if "item_code" in meta:
                        cand.item_code = meta["item_code"]
                else:
                    # 검색 0건이면 신뢰도 깎음 (검증 실패)
                    cand.confidence = min(cand.confidence, 0.4)

        logger.info(f"[OnboardingNormalizer] candidates={len(candidates)} latency={resp.latency_ms}ms")
        return NormalizeResponse(candidates=candidates)
