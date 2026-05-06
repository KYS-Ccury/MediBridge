"""
Monitoring Router — GET /health, /metrics

인증 불필요. /v1/ prefix 없음 (운영 표준).
"""
from datetime import datetime, timezone

from fastapi import APIRouter, Response, status

from Schemas.HealthSchema import HealthResponse, MetricsResponse
from Monitoring.HealthChecker import HealthChecker
from Monitoring.MetricsExporter import MetricsExporter

router = APIRouter(tags=["Monitoring"])


@router.get("/health", response_model=HealthResponse)
async def health(response: Response) -> HealthResponse:
    """헬스체크 — GPU·모델 로딩·디스크 상태"""
    result = HealthChecker.instance().check()
    if result.status != "ok":
        response.status_code = status.HTTP_503_SERVICE_UNAVAILABLE
    return result


@router.get("/metrics", response_model=MetricsResponse)
async def metrics() -> MetricsResponse:
    """자원 사용률 + GPU + 모델별 통계"""
    return MetricsExporter.instance().collect_json()
