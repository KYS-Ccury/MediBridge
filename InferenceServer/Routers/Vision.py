"""
Vision Router — POST /vision/detect, /vision/analyze

알약 검출(YOLO26) + 각인·색·모양 분석(PaddleOCR + OpenCV).
"""
from fastapi import APIRouter, UploadFile, File, HTTPException
from loguru import logger

from Schemas.VisionSchema import DetectResponse, AnalyzeResponse
from Vision.YoloDetector import YoloDetector
from Vision.OcrEngine import OcrEngine
from Vision.ColorShape import ColorShape

router = APIRouter(prefix="/vision", tags=["Vision"])


@router.post("/detect", response_model=DetectResponse)
async def detect(image: UploadFile = File(...)) -> DetectResponse:
    """
    알약 검출 (YOLO26).

    요청: 이미지 바이너리 (multipart/form-data)
    응답: 검출된 알약별 bounding box + crop_id + confidence
    """
    # TODO (영역 A 분담):
    #   1. UploadFile 을 임시 파일로 저장 또는 메모리 버퍼로 변환
    #   2. YoloDetector.instance().detect(image_bytes) 호출
    #   3. crop_id 발급 + 각 crop 이미지를 캐시 (analyze에서 재사용)
    #   4. DetectResponse 반환
    logger.info(f"[Vision] /detect — file: {image.filename}, size: {image.size}")
    raise HTTPException(status_code=501, detail="NOT_IMPLEMENTED")


@router.post("/analyze", response_model=AnalyzeResponse)
async def analyze(crop_id: str) -> AnalyzeResponse:
    """
    각인 인식 + 색·모양·크기 분석.

    요청: 이전 detect 단계에서 발급된 crop_id
    응답: 각인 텍스트, HSV 색상 라벨, 모양, 크기, 신뢰도
    """
    # TODO (영역 A 분담):
    #   1. crop_id 로 caching된 crop 이미지 조회
    #   2. OcrEngine.instance().recognize(crop) → 각인 텍스트
    #   3. ColorShape.classify_color(crop) + classify_shape(crop) + measure_size(crop)
    #   4. AnalyzeResponse 반환
    raise HTTPException(status_code=501, detail="NOT_IMPLEMENTED")
