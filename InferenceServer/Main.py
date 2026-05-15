"""
MediBridge InferenceServer — 진입점

본 코드는 LLM PC (10.10.10.128:8002) 와 Vision PC (10.10.10.120:8003) 양쪽에서 공유.
환경변수로 활성/비활성 분기:

    MEDIBRIDGE_VISION_ENABLED=true|false   # 기본 true (Vision PC 에선 ON, LLM PC 에선 OFF)
    MEDIBRIDGE_LLM_ENABLED=true|false      # 기본 true (LLM PC 에선 ON, Vision PC 에선 OFF)

LLM PC 셋업 예 (.env):
    MEDIBRIDGE_VISION_ENABLED=false
    MEDIBRIDGE_LLM_ENABLED=true
    MEDIBRIDGE_INFERENCE_PORT=8002

Vision PC 셋업 예 (.env):
    MEDIBRIDGE_VISION_ENABLED=true
    MEDIBRIDGE_LLM_ENABLED=false
    MEDIBRIDGE_INFERENCE_PORT=8003
"""
import os
from contextlib import asynccontextmanager

from fastapi import FastAPI
from loguru import logger

from Config import get_config
from Routers import Vision, Intent, Onboarding, Summary, Speech, Monitoring
from Threading.WorkerPool import WorkerPool


def _flag(name: str, default: bool) -> bool:
    val = os.getenv(name, "true" if default else "false").strip().lower()
    return val in ("1", "true", "yes", "on")


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    FastAPI 앱 수명 주기 — 시작 시 활성 모델만 로딩, 종료 시 정리.
    """
    config = get_config()
    vision_enabled = _flag("MEDIBRIDGE_VISION_ENABLED", default=True)
    llm_enabled = _flag("MEDIBRIDGE_LLM_ENABLED", default=True)

    logger.info(
        f"[Main] 시작 — port={config.server_port} "
        f"vision_enabled={vision_enabled} llm_enabled={llm_enabled}"
    )

    # ----- Vision (인효 담당, Vision PC 에서만 활성) -----
    if vision_enabled:
        try:
            from Vision.YoloDetector import YoloDetector
            from Vision.OcrEngine import OcrEngine
            YoloDetector.instance().load_model(config.yolo_weights_path)
            OcrEngine.instance().load_model()
            logger.info("[Main] Vision 모델 로딩 완료")
        except Exception as e:
            logger.warning(f"[Main] Vision 로딩 실패 (skip): {e}")
    else:
        logger.info("[Main] Vision 비활성 — LLM PC 모드")

    # ----- LLM (윤식·동주 담당, LLM PC 에서 활성) -----
    if llm_enabled:
        try:
            from Llm.IntentClassifier import IntentClassifier
            from Llm.OnboardingNormalizer import OnboardingNormalizer
            from Llm.OnboardingDisambiguator import OnboardingDisambiguator
            from Llm.NonMedicalSummarizer import NonMedicalSummarizer
            from Llm.RagClient import RagClient

            IntentClassifier.instance().load_model()
            OnboardingNormalizer.instance().load_model()
            OnboardingDisambiguator.instance().load_model()
            NonMedicalSummarizer.instance().load_model()
            # RAG 는 첫 검색 시 lazy load 가 기본 — 시작 시 명시 로드는 선택
            if _flag("MEDIBRIDGE_RAG_PRELOAD", default=False):
                RagClient.instance().load()
            logger.info("[Main] LLM 모델 로딩 완료")
        except Exception as e:
            logger.warning(f"[Main] LLM 로딩 실패 (graceful): {e}")
    else:
        logger.info("[Main] LLM 비활성 — Vision PC 모드")

    # ----- WorkerPool -----
    WorkerPool.instance().init(config.worker_pool_size)

    yield

    logger.info("[Main] 종료")
    WorkerPool.instance().shutdown()


# FastAPI 앱 생성
app = FastAPI(
    title="MediBridge InferenceServer",
    version="0.2.0",
    description="알약 식별·OCR·LLM 의도 분류 추론 서버 (LLM PC + Vision PC 공유 코드)",
    lifespan=lifespan,
)

# 라우터 등록
app.include_router(Vision.router)
app.include_router(Intent.router)
app.include_router(Onboarding.router)
app.include_router(Summary.router)
app.include_router(Speech.router)
app.include_router(Monitoring.router)


if __name__ == "__main__":
    import uvicorn

    config = get_config()
    uvicorn.run(
        "Main:app",
        host="0.0.0.0",
        port=config.server_port,
        log_level="info",
        reload=False,
    )
