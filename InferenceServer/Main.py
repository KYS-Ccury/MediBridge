"""
MediBridge InferenceServer — 진입점

역할:
    1. FastAPI 앱 인스턴스 생성
    2. 라우터 등록 (Vision, Intent, Onboarding, Summary, Speech, Monitoring)
    3. 모델 로딩 (YOLO26, PaddleOCR, LLM 등)
    4. uvicorn 실행

명명 규칙: 파일·클래스 PascalCase, 메소드·변수 snake_case (사용자 정의)
"""
from contextlib import asynccontextmanager

from fastapi import FastAPI
from loguru import logger

from Config import get_config
from Routers import Vision, Intent, Onboarding, Summary, Speech, Monitoring
from Vision.YoloDetector import YoloDetector
from Vision.OcrEngine import OcrEngine
from Llm.IntentClassifier import IntentClassifier
from Llm.OnboardingNormalizer import OnboardingNormalizer
from Llm.OnboardingDisambiguator import OnboardingDisambiguator
from Llm.NonMedicalSummarizer import NonMedicalSummarizer
from Threading.WorkerPool import WorkerPool


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    FastAPI 앱 수명 주기 — 시작 시 모델 로딩, 종료 시 정리.
    """
    config = get_config()

    # 1. 모델 로딩 (GPU 메모리 점유)
    logger.info("[Main] 모델 로딩 시작")
    YoloDetector.instance().load_model(config.yolo_weights_path)
    OcrEngine.instance().load_model()
    IntentClassifier.instance().load_model()
    OnboardingNormalizer.instance().load_model()
    OnboardingDisambiguator.instance().load_model()
    NonMedicalSummarizer.instance().load_model()
    logger.info("[Main] 모델 로딩 완료")

    # 2. WorkerPool 초기화
    WorkerPool.instance().init(config.worker_pool_size)

    yield

    # 3. 종료 정리
    logger.info("[Main] 종료 — 모델 언로드")
    WorkerPool.instance().shutdown()


# FastAPI 앱 생성
app = FastAPI(
    title="MediBridge InferenceServer",
    version="0.1.0",
    description="알약 식별·OCR·LLM 의도 분류 추론 서버",
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
