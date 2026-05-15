"""
Config — 추론 서버 전역 설정

환경변수 또는 config.json 으로 로드. 시크릿(LLM API 키 등)은 환경변수 강제.

기본 LLM = OpenAI gpt-4.1-nano (2026-05-15 결정). Ollama 는 fallback.
"""
import os
from dataclasses import dataclass, field
from functools import lru_cache


@dataclass
class Config:
    # ----- HTTP 서버 -----
    server_port: int = 8002
    worker_pool_size: int = 4

    # ----- Vision 모델 (인효 담당, 본 PC 아님) -----
    yolo_weights_path: str = "Models/yolo26_pills.pt"
    ocr_weights_path: str = "Models/paddleocr_pills"
    use_gpu: bool = True

    # ----- LLM Provider (LlmProvider 추상화) -----
    # backend = "openai" | "ollama"
    llm_backend: str = "openai"
    # OpenAI
    openai_model: str = "gpt-4.1-nano"
    openai_api_key: str = field(default="")
    openai_base_url: str = "https://api.openai.com/v1"
    # Ollama
    ollama_host: str = "http://localhost:11434"
    ollama_model: str = "gemma4:e4b"
    # 공통
    llm_temperature: float = 0.0
    llm_timeout_ms: int = 8000

    # ----- 임베딩 / RAG -----
    embedding_model: str = "nlpai-lab/KURE-v1"
    embedding_device: str = "cuda"           # "cuda" / "cpu"
    chroma_persist_dir: str = "./chroma_db"

    # ----- 추론 옵션 (Vision) -----
    yolo_conf_threshold: float = 0.5
    yolo_iou_threshold: float = 0.45
    ocr_max_candidates: int = 5

    # ----- 헬스체크 -----
    health_check_timeout_ms: int = 3000


@lru_cache
def get_config() -> Config:
    """싱글톤 Config 반환 (lru_cache 로 한 번만 로드)"""
    config = Config()

    # ----- LLM Provider -----
    config.llm_backend = os.getenv("MEDIBRIDGE_LLM_BACKEND", config.llm_backend)
    config.openai_model = os.getenv("OPENAI_MODEL", config.openai_model)
    config.openai_api_key = os.getenv("OPENAI_API_KEY", config.openai_api_key)
    config.openai_base_url = os.getenv("OPENAI_BASE_URL", config.openai_base_url)
    config.ollama_host = os.getenv("OLLAMA_HOST", config.ollama_host)
    config.ollama_model = os.getenv("OLLAMA_MODEL", config.ollama_model)
    if temp := os.getenv("LLM_TEMPERATURE"):
        try: config.llm_temperature = float(temp)
        except ValueError: pass
    if t := os.getenv("LLM_TIMEOUT_MS"):
        try: config.llm_timeout_ms = int(t)
        except ValueError: pass

    # ----- 임베딩 -----
    config.embedding_model = os.getenv("EMBEDDING_MODEL", config.embedding_model)
    config.embedding_device = os.getenv("EMBEDDING_DEVICE", config.embedding_device)
    config.chroma_persist_dir = os.getenv("CHROMA_PERSIST_DIR", config.chroma_persist_dir)

    # ----- Vision -----
    if path := os.getenv("MEDIBRIDGE_YOLO_WEIGHTS"):
        config.yolo_weights_path = path

    # ----- 포트 -----
    if port := os.getenv("MEDIBRIDGE_INFERENCE_PORT"):
        try: config.server_port = int(port)
        except ValueError: pass

    return config
