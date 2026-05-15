"""
Monitoring Router — GET /health, /metrics, /llm/health

인증 불필요. /v1/ prefix 없음 (운영 표준).
"""
import os

from fastapi import APIRouter, Response, status

from Schemas.HealthSchema import HealthResponse, MetricsResponse
from Monitoring.HealthChecker import HealthChecker
from Monitoring.MetricsExporter import MetricsExporter

router = APIRouter(tags=["Monitoring"])


@router.get("/health", response_model=HealthResponse)
async def health(response: Response) -> HealthResponse:
    """헬스체크 — GPU·모델 로딩·디스크·LLM Provider·RAG 상태"""
    result = HealthChecker.instance().check()
    if result.status != "ok":
        response.status_code = status.HTTP_503_SERVICE_UNAVAILABLE
    return result


@router.get("/metrics", response_model=MetricsResponse)
async def metrics() -> MetricsResponse:
    """자원 사용률 + GPU + 모델별 통계"""
    return MetricsExporter.instance().collect_json()


@router.get("/llm/health")
async def llm_health() -> dict:
    """LLM Provider + RAG 단독 상태 (디버깅·튜닝용 verbose 응답)"""
    out: dict = {
        "vision_enabled": os.getenv("MEDIBRIDGE_VISION_ENABLED", "true").lower() in ("1", "true", "yes", "on"),
        "llm_enabled":    os.getenv("MEDIBRIDGE_LLM_ENABLED", "true").lower() in ("1", "true", "yes", "on"),
    }

    if out["llm_enabled"]:
        try:
            from Llm.LlmProvider import make_provider
            p = make_provider()
            out["provider"] = {
                "name": p.name,
                "model": p.model,
                "reachable": p.health(),
            }
        except Exception as e:
            out["provider"] = {"error": f"{type(e).__name__}: {e}"}

        try:
            from Llm.RagClient import RagClient
            rag = RagClient.instance()
            # 첫 호출 시 lazy load 시도
            if not rag.is_loaded():
                rag.load()
            out["rag"] = rag.health()
        except Exception as e:
            out["rag"] = {"error": f"{type(e).__name__}: {e}"}

    return out
