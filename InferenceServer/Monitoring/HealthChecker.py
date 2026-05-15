"""
HealthChecker — 서비스 헬스 점검 (GPU·모델 로딩·디스크·LLM·RAG)

LLM PC ↔ Vision PC 코드 공유 — 환경변수로 분기:
    MEDIBRIDGE_VISION_ENABLED=true|false   (기본 true)
    MEDIBRIDGE_LLM_ENABLED=true|false      (기본 true)

각 모드에서 활성 모듈만 점검. 비활성 모듈의 미로드는 reason 에 포함시키지 않음.
"""
import os
from datetime import datetime, timezone
from typing import Optional

from Schemas.HealthSchema import HealthResponse


def _flag(name: str, default: bool) -> bool:
    val = os.getenv(name, "true" if default else "false").strip().lower()
    return val in ("1", "true", "yes", "on")


class HealthChecker:
    _instance: Optional["HealthChecker"] = None

    @classmethod
    def instance(cls) -> "HealthChecker":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self._start_time = datetime.now(timezone.utc)
        self._vision_enabled = _flag("MEDIBRIDGE_VISION_ENABLED", default=True)
        self._llm_enabled = _flag("MEDIBRIDGE_LLM_ENABLED", default=True)

    def check(self) -> HealthResponse:
        """현재 상태 점검 — 활성 모듈만"""
        reasons: list[str] = []
        critical_missing = False  # status=down 트리거

        # ----- Vision (Vision PC 에서만 점검) -----
        if self._vision_enabled:
            try:
                from Vision.YoloDetector import YoloDetector
                from Vision.OcrEngine import OcrEngine
                if not YoloDetector.instance().is_loaded:
                    reasons.append("yolo_not_loaded")
                    critical_missing = True
                if not OcrEngine.instance().is_loaded:
                    reasons.append("ocr_not_loaded")
                    critical_missing = True
            except Exception as e:
                reasons.append(f"vision_import_failed:{type(e).__name__}")
                critical_missing = True

        # ----- LLM (LLM PC 에서만 점검) -----
        if self._llm_enabled:
            try:
                from Llm.IntentClassifier import IntentClassifier
                from Llm.OnboardingNormalizer import OnboardingNormalizer
                from Llm.NonMedicalSummarizer import NonMedicalSummarizer
                from Llm.LlmProvider import make_provider

                if not IntentClassifier.instance().is_loaded:
                    reasons.append("intent_classifier_not_loaded")
                if not OnboardingNormalizer.instance().is_loaded:
                    reasons.append("onboarding_normalizer_not_loaded")
                if not NonMedicalSummarizer.instance().is_loaded:
                    reasons.append("non_medical_summarizer_not_loaded")

                # LlmProvider 도달성 (network)
                try:
                    provider = make_provider()
                    # health() 는 짧은 타임아웃의 GET 호출 — 본 함수가 너무 느려지지 않게
                    if not provider.health():
                        reasons.append(f"llm_provider_unreachable:{provider.name}")
                        # OpenAI 미도달은 critical 처리 — 의도 분류 불가
                        critical_missing = True
                except Exception as e:
                    reasons.append(f"llm_provider_init_failed:{type(e).__name__}")
                    critical_missing = True

                # RAG 도달성 (선택적 — RAG 없어도 LLM 본 기능은 동작)
                try:
                    from Llm.RagClient import RagClient
                    if not RagClient.instance().is_loaded():
                        reasons.append("rag_not_loaded")
                except Exception:
                    reasons.append("rag_import_failed")
            except Exception as e:
                reasons.append(f"llm_import_failed:{type(e).__name__}")
                critical_missing = True

        # ----- 상태 판정 -----
        if not reasons:
            status = "ok"
        elif critical_missing:
            status = "down"
        else:
            status = "degraded"

        uptime = int((datetime.now(timezone.utc) - self._start_time).total_seconds())
        return HealthResponse(
            status=status,
            uptime_seconds=uptime,
            checked_at=datetime.now(timezone.utc).isoformat(),
            reasons=reasons,
        )
