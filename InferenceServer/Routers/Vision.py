"""
Vision Router — Vision PC (10.10.10.120:8003)

엔드포인트:
    POST /vision/detect          — 이미지 raw 바이너리 → 검출 (POC)
    POST /vision/analyze?crop_id — crop 한 개 분석 (각인·색·모양·크기)
    POST /vision/detect_remote   — ⭐ 메인서버 호출 — 보관 PC GET → 검출 → 통합 분석

⭐ 2026-05-15 — 인효 담당자 코드 (engines/yolo_engine + lib/* + engines/cv_engine) 이식 후
    네트워크 라우터 작성. AWS S3+RDS 패턴 (사진 본체는 메인서버 통과 X — 보관 PC 직접 GET).
"""
import base64
from typing import Optional

import cv2
import httpx
import numpy as np
from fastapi import APIRouter, UploadFile, File, HTTPException, Query
from loguru import logger

from Schemas.VisionSchema import (
    DetectResponse, AnalyzeResponse, DetectedPill,
    DetectRemoteRequest, DetectRemoteResponse, DetectRemoteCandidate, BoundingBox,
)
from Vision.YoloDetector import YoloDetector
from Vision.OcrEngine import OcrEngine
from Vision.ColorClassifier import ColorClassifier
from Vision.ShapeClassifier import ShapeClassifier
from Vision.SizeMeasurer import SizeMeasurer

router = APIRouter(prefix="/vision", tags=["Vision"])


# =====================================================
# POST /vision/detect — 이미지 raw → 검출 (POC, 단독 테스트용)
# =====================================================
@router.post("/detect", response_model=DetectResponse)
async def detect(image: UploadFile = File(...)) -> DetectResponse:
    """단일 이미지 → 검출만 (analyze 는 별도 호출)."""
    image_bytes = await image.read()
    if not image_bytes:
        raise HTTPException(status_code=400, detail="EMPTY_IMAGE")

    detections = YoloDetector.instance().detect(image_bytes)
    if not detections:
        return DetectResponse(
            detections=[], status="no_pill_detected",
            message="알약이 검출되지 않았습니다.",
        )
    return DetectResponse(detections=detections, status="ok")


# =====================================================
# POST /vision/analyze — crop 한 개 분석 (POC)
# =====================================================
@router.post("/analyze", response_model=AnalyzeResponse)
async def analyze(crop_id: str = Query(..., min_length=1)) -> AnalyzeResponse:
    """이전 detect 단계에서 발급된 crop_id 로 각인·색·모양·크기 분석."""
    crop = YoloDetector.instance().get_crop(crop_id)
    if crop is None:
        raise HTTPException(status_code=404, detail="CROP_NOT_FOUND")

    engraving_text, engraving_conf = OcrEngine.instance().recognize_array(crop)
    color = ColorClassifier.classify_array(crop)
    shape = ShapeClassifier.classify_array(crop)
    size_mm = SizeMeasurer.measure_mm_array(crop)

    # 종합 신뢰도 — 가중 평균 (각인 50% / 색 25% / 모양 25%)
    overall = (
        engraving_conf * 0.5
        + color.confidence * 0.25
        + (1.0 if shape.label != "기타" else 0.3) * 0.25
    )
    return AnalyzeResponse(
        crop_id=crop_id,
        engraving_text=engraving_text,
        engraving_confidence=engraving_conf,
        color_hsv_label=color.label,
        shape_label=shape.label,
        size_mm=size_mm,
        overall_confidence=round(float(overall), 3),
    )


# =====================================================
# POST /vision/detect_remote — ⭐ 메인서버 호출용 통합 라우터
# =====================================================
@router.post("/detect_remote", response_model=DetectRemoteResponse)
async def detect_remote(req: DetectRemoteRequest) -> DetectRemoteResponse:
    """
    메인서버가 photo_id + storage_url + get_token 을 전달.
    Vision PC 가 보관 PC 에서 이미지 GET → YOLO → OCR + 색·모양·크기 → 통합 응답.
    """
    logger.info(
        f"[Vision] /detect_remote photo_id={req.photo_id} purpose={req.purpose}"
    )

    # 1) 보관 PC 에서 이미지 GET
    try:
        with httpx.Client(timeout=8.0) as client:
            r = client.get(
                req.storage_url,
                headers={"Authorization": f"Bearer {req.get_token}"},
            )
        if r.status_code != 200:
            logger.warning(f"[Vision] storage GET 실패 status={r.status_code}")
            return DetectRemoteResponse(
                photo_id=req.photo_id, candidates=[], confidence_tier="LOW",
                status="storage_get_failed", message=f"storage status={r.status_code}",
            )
        image_bytes = r.content
    except Exception as e:
        logger.exception(f"[Vision] storage GET 예외: {e}")
        return DetectRemoteResponse(
            photo_id=req.photo_id, candidates=[], confidence_tier="LOW",
            status="storage_get_failed", message=str(e),
        )

    # 2) YOLO 검출
    detections = YoloDetector.instance().detect(image_bytes)
    if not detections:
        return DetectRemoteResponse(
            photo_id=req.photo_id, candidates=[], confidence_tier="LOW",
            status="no_pill_detected",
        )

    # 3) 각 crop 분석 + 매칭 키 종합
    candidates = []
    confidences = []
    for det in detections:
        crop = YoloDetector.instance().get_crop(det.crop_id)
        if crop is None:
            continue

        engraving_text, engraving_conf = OcrEngine.instance().recognize_array(crop)
        color = ColorClassifier.classify_array(crop)
        shape = ShapeClassifier.classify_array(crop)
        size_mm = SizeMeasurer.measure_mm_array(crop)

        # 매칭 키 — ⚠ 크기(mm)는 제외.
        #   기준물체(동전 등) 없는 단일 2D 사진에선 카메라 거리에 따라
        #   값이 제멋대로(같은 알약이 10.7~12.8mm)라 매칭/표시에 부적합.
        #   size_mm 은 응답 필드로는 유지(참고용)하되 match_keys 엔 불포함.
        match_keys = []
        if engraving_text and engraving_text != "각인없음":
            match_keys.append(f"각인:{engraving_text}")
        match_keys.append(f"색:{color.label}")
        match_keys.append(f"모양:{shape.label}")

        # crop 썸네일 인코딩 (최대 변 240px 리사이즈 → JPEG q70 → base64).
        #   다중 알약 시 클라가 "어느 카드 = 어느 실물" 판단하도록 전달.
        crop_b64 = None
        try:
            ch, cw = crop.shape[:2]
            if max(ch, cw) > 240:
                scale = 240.0 / float(max(ch, cw))
                thumb = cv2.resize(
                    crop, (max(1, int(cw * scale)), max(1, int(ch * scale))),
                    interpolation=cv2.INTER_AREA,
                )
            else:
                thumb = crop
            ok, buf = cv2.imencode(
                ".jpg", thumb, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
            if ok:
                crop_b64 = base64.b64encode(buf.tobytes()).decode("ascii")
        except Exception as e:
            logger.warning(f"[Vision] crop 썸네일 인코딩 실패: {e}")

        candidates.append(DetectRemoteCandidate(
            crop_id=det.crop_id,
            bbox=det.bbox,
            detection_confidence=det.confidence,
            engraving_text=engraving_text,
            engraving_confidence=engraving_conf,
            color_label=color.label,
            shape_label=shape.label,
            size_mm=size_mm,
            match_keys=match_keys,
            crop_jpeg_b64=crop_b64,
        ))
        # 종합 신뢰도 가중치
        overall = det.confidence * 0.4 + engraving_conf * 0.4 + color.confidence * 0.2
        confidences.append(overall)

    # 4) 신뢰도 tier 산정 (요구사항 FR-C4-01)
    if not confidences:
        tier = "LOW"
    else:
        avg = sum(confidences) / len(confidences)
        if avg >= 0.95:
            tier = "HIGH"
        elif avg >= 0.70:
            tier = "MEDIUM"
        else:
            tier = "LOW"

    logger.info(
        f"[Vision] /detect_remote 완료 candidates={len(candidates)} tier={tier}"
    )
    return DetectRemoteResponse(
        photo_id=req.photo_id,
        candidates=candidates,
        confidence_tier=tier,
        status="ok",
    )
