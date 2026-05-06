"""
HealthChecker — 서비스 헬스 점검 (GPU·모델 로딩·디스크)
"""
from typing import Optional
from datetime import datetime, timezone

from Schemas.HealthSchema import HealthResponse
from Vision.YoloDetector import YoloDetector
from Vision.OcrEngine import OcrEngine
from Llm.IntentClassifier import IntentClassifier


class HealthChecker:
    _instance: Optional["HealthChecker"] = None

    @classmethod
    def instance(cls) -> "HealthChecker":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self._start_time = datetime.now(timezone.utc)

    def check(self) -> HealthResponse:
        """현재 상태 점검"""
        reasons: list[str] = []

        # 모델 로딩 상태
        if not YoloDetector.instance().is_loaded:
            reasons.append("yolo_not_loaded")
        if not OcrEngine.instance().is_loaded:
            reasons.append("ocr_not_loaded")
        if not IntentClassifier.instance().is_loaded:
            reasons.append("intent_classifier_not_loaded")

        # TODO (영역 A 분담):
        #   - GPU 가용성 (pynvml)
        #   - 디스크 여유 공간
        #   - 외부 API 연결 (LLM 외부 사용 시)

        # 상태 판정
        if not reasons:
            status = "ok"
        elif "yolo_not_loaded" in reasons or "ocr_not_loaded" in reasons:
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
