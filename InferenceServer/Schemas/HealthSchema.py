"""
HealthSchema — Monitoring API (GET /health, /metrics)
"""
from typing import List, Optional
from pydantic import BaseModel, Field


class HealthResponse(BaseModel):
    status: str = Field(description="ok / degraded / down")
    service: str = "inference_server"
    version: str = "0.1.0"
    uptime_seconds: int
    checked_at: str
    reasons: List[str] = []


class GpuMetrics(BaseModel):
    index: int
    name: str
    utilization_percent: float
    memory_used_mb: float
    memory_total_mb: float
    memory_percent: float
    temperature_c: Optional[int] = None


class ModelMetrics(BaseModel):
    name: str
    loaded: bool
    infer_count_total: int = 0
    avg_latency_ms: float = 0.0
    last_infer_at: Optional[str] = None


class SystemMetrics(BaseModel):
    cpu_percent: float
    memory_used_mb: float
    memory_total_mb: float
    memory_percent: float
    disk_used_gb: float
    disk_total_gb: float
    disk_percent: float
    uptime_seconds: int


class ProcessMetrics(BaseModel):
    pid: int
    cpu_percent: float
    memory_mb: float
    thread_count: int


class MetricsResponse(BaseModel):
    service: str = "inference_server"
    collected_at: str
    system: SystemMetrics
    process: ProcessMetrics
    gpu: List[GpuMetrics] = []
    models: List[ModelMetrics] = []
