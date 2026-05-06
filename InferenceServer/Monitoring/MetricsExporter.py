"""
MetricsExporter — /metrics 응답 빌더
"""
from typing import Optional
from datetime import datetime, timezone

from Schemas.HealthSchema import MetricsResponse, ModelMetrics
from Monitoring.ResourceMonitor import ResourceMonitor


class MetricsExporter:
    _instance: Optional["MetricsExporter"] = None

    @classmethod
    def instance(cls) -> "MetricsExporter":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def collect_json(self) -> MetricsResponse:
        """현재 메트릭 수집 → JSON 응답"""
        rm = ResourceMonitor.instance()

        return MetricsResponse(
            collected_at=datetime.now(timezone.utc).isoformat(),
            system=rm.measure_system(),
            process=rm.measure_process(),
            gpu=rm.measure_gpu(),
            models=self._collect_model_metrics(),
        )

    def _collect_model_metrics(self) -> list[ModelMetrics]:
        """모델별 통계 (추론 횟수·평균 지연 등)"""
        # TODO (영역 A 분담):
        #   - 각 모듈(YoloDetector, OcrEngine, IntentClassifier 등)에 통계 수집 인터페이스 추가
        #   - infer_count_total, avg_latency_ms 트래킹
        return []
