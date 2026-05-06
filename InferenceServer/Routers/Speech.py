"""
Speech Router — (확장 fallback) POST /speech/stt

MVP는 폰 온디바이스 STT 사용. 본 라우터는 fallback 자리만.
"""
from fastapi import APIRouter, UploadFile, File, HTTPException

from Schemas.SpeechSchema import SttResponse

router = APIRouter(prefix="/speech", tags=["Speech (Fallback)"])


@router.post("/stt", response_model=SttResponse)
async def stt(audio: UploadFile = File(...)) -> SttResponse:
    """
    음성 바이너리 → Whisper STT 텍스트 변환 (확장 fallback).

    MVP에선 호출되지 않음 (폰 온디바이스 STT 사용).
    """
    # TODO (확장 단계):
    #   1. faster-whisper 모델 로딩
    #   2. audio 파일을 임시 저장 → whisper.transcribe(audio_path, language="ko")
    #   3. 결과 텍스트 + confidence 반환
    raise HTTPException(status_code=501, detail="NOT_IMPLEMENTED (확장 fallback)")
