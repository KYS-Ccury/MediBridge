"""
WorkerPool — concurrent.futures.ThreadPoolExecutor 래퍼 (싱글톤)

Python GIL 한계가 있지만, 모델 추론은 PyTorch C++ 영역이라 GIL 해제됨.
실질적인 병렬 추론 + 비동기 wait 가능.

⚠️ 모델 추론 자체는 PyTorch가 GPU에서 처리 → 본 풀은 보조용.
주 사용처: 이미지 디코딩·전처리·후처리 등 CPU 작업.
"""
from concurrent.futures import ThreadPoolExecutor, Future
from typing import Callable, Optional, Any
from loguru import logger


class WorkerPool:
    _instance: Optional["WorkerPool"] = None

    @classmethod
    def instance(cls) -> "WorkerPool":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.executor: Optional[ThreadPoolExecutor] = None
        self.worker_count: int = 0

    def init(self, worker_count: int) -> None:
        """풀 초기화 (Main lifespan에서 1회)"""
        if self.executor is not None:
            return
        self.executor = ThreadPoolExecutor(max_workers=worker_count)
        self.worker_count = worker_count
        logger.info(f"[WorkerPool] 초기화 — 워커 수: {worker_count}")

    def submit(self, func: Callable[..., Any], *args, **kwargs) -> Future:
        """
        작업을 워커에 위임 → Future 반환.

        Returns:
            concurrent.futures.Future — await 또는 .result() 로 결과 수신
        """
        if self.executor is None:
            raise RuntimeError("WorkerPool 미초기화 — init() 먼저 호출")
        return self.executor.submit(func, *args, **kwargs)

    def shutdown(self) -> None:
        """풀 종료 (대기 작업 모두 완료 후)"""
        if self.executor is not None:
            self.executor.shutdown(wait=True)
            self.executor = None
            logger.info("[WorkerPool] 종료됨")
