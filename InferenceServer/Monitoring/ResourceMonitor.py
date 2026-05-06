"""
ResourceMonitor — psutil + pynvml 기반 자원 측정
"""
from typing import List, Optional
from datetime import datetime, timezone

import psutil
# import pynvml   # TODO 단계 활성화

from Schemas.HealthSchema import SystemMetrics, ProcessMetrics, GpuMetrics


class ResourceMonitor:
    _instance: Optional["ResourceMonitor"] = None

    @classmethod
    def instance(cls) -> "ResourceMonitor":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self._boot_time = psutil.boot_time()
        # TODO: pynvml.nvmlInit() — GPU 사용 시

    def measure_system(self) -> SystemMetrics:
        """CPU·메모리·디스크 측정 (psutil)"""
        mem = psutil.virtual_memory()
        disk = psutil.disk_usage("/")
        uptime = int(datetime.now().timestamp() - self._boot_time)

        return SystemMetrics(
            cpu_percent=psutil.cpu_percent(interval=0.1),
            memory_used_mb=mem.used / (1024 * 1024),
            memory_total_mb=mem.total / (1024 * 1024),
            memory_percent=mem.percent,
            disk_used_gb=disk.used / (1024 ** 3),
            disk_total_gb=disk.total / (1024 ** 3),
            disk_percent=disk.percent,
            uptime_seconds=uptime,
        )

    def measure_process(self) -> ProcessMetrics:
        """현재 프로세스 자원 측정"""
        proc = psutil.Process()
        return ProcessMetrics(
            pid=proc.pid,
            cpu_percent=proc.cpu_percent(interval=0.1),
            memory_mb=proc.memory_info().rss / (1024 * 1024),
            thread_count=proc.num_threads(),
        )

    def measure_gpu(self) -> List[GpuMetrics]:
        """NVIDIA GPU 측정 (pynvml)"""
        # TODO (영역 A 분담):
        #   import pynvml
        #   pynvml.nvmlInit()
        #   count = pynvml.nvmlDeviceGetCount()
        #   for i in range(count):
        #       handle = pynvml.nvmlDeviceGetHandleByIndex(i)
        #       util = pynvml.nvmlDeviceGetUtilizationRates(handle)
        #       mem = pynvml.nvmlDeviceGetMemoryInfo(handle)
        #       temp = pynvml.nvmlDeviceGetTemperature(handle, 0)
        #       ...
        return []
