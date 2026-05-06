"""
WhisperFallback — 음성 바이너리 → 텍스트 변환 (확장 fallback)

MVP는 폰 온디바이스 STT(Galaxy AI)를 사용하므로 본 모듈은 호출되지 않음.
폰 STT 미사용 환경 또는 정확도 비교 평가 시에만 활성화.
"""
from typing import Optional, Tuple
from loguru import logger

# from faster_whisper import WhisperModel   # TODO 단계에서


class WhisperFallback:
    _instance: Optional["WhisperFallback"] = None

    @classmethod
    def instance(cls) -> "WhisperFallback":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self.model = None
        self.is_loaded: bool = False

    def load_model(self, model_size: str = "medium") -> None:
        """Whisper 모델 로딩 (확장 단계)"""
        # TODO (확장):
        #   self.model = WhisperModel(model_size, device="cuda", compute_type="float16")
        logger.info(f"[WhisperFallback] load_model TODO — size: {model_size}")
        self.is_loaded = False

    def transcribe(self, audio_path: str, language: str = "ko") -> Tuple[str, float]:
        """
        오디오 파일 → 텍스트 변환.

        Returns:
            (텍스트, 평균 신뢰도)
        """
        # TODO (확장):
        #   segments, info = self.model.transcribe(audio_path, language=language)
        #   text = " ".join(seg.text for seg in segments)
        return ("", 0.0)
