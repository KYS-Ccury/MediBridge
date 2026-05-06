"""
Config — 추론 서버 전역 설정

환경변수 또는 config.json 으로 로드. 시크릿(LLM API 키 등)은 환경변수 강제.
"""
import os
from dataclasses import dataclass, field
from functools import lru_cache


@dataclass
class Config:
    # ----- HTTP 서버 -----
    server_port: int = 8002
    worker_pool_size: int = 4   # CPU 무거운 작업용 (Python GIL 우회 — 보조)

    # ----- Vision 모델 -----
    yolo_weights_path: str = "Models/yolo26_pills.pt"
    ocr_weights_path: str = "Models/paddleocr_pills"
    use_gpu: bool = True

    # ----- LLM (의도 분류기·등록 보조) -----
    llm_model_name: str = "TODO_llm_model"
    llm_api_url: str = ""
    llm_api_key: str = field(default="")     # 환경변수에서 로드

    # ----- 추론 옵션 -----
    yolo_conf_threshold: float = 0.5
    yolo_iou_threshold: float = 0.45
    ocr_max_candidates: int = 5

    # ----- 헬스체크 -----
    health_check_timeout_ms: int = 3000


@lru_cache
def get_config() -> Config:
    """싱글톤 Config 반환 (lru_cache 로 한 번만 로드)"""
    config = Config()

    # 환경변수 우선 적용
    if pw := os.getenv("MEDIBRIDGE_LLM_API_KEY"):
        config.llm_api_key = pw

    if path := os.getenv("MEDIBRIDGE_YOLO_WEIGHTS"):
        config.yolo_weights_path = path

    return config
