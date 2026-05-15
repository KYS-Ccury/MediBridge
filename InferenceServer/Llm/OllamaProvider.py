"""
OllamaProvider — Local fallback (gemma4:e4b 기본)

목적:
    - 인터넷 단절·OpenAI 장애 시 fallback
    - 외부 송신이 부담스러운 환경(병원·금융 등)에서 로컬 추론

장점:
    - 데이터 외부 송신 X
    - 비용 0
    - VRAM ~3 GB (Q4 양자화)

단점:
    - 응답 ~500-1500 ms (gpt-4.1-nano 보다 느림)
    - 모델 다운로드·Ollama 설치 필요

사전 셋업:
    curl -fsSL https://ollama.com/install.sh | sh
    ollama pull gemma4:e4b
    ollama serve     # :11434
"""
from __future__ import annotations

import time
from typing import Optional

import httpx
from loguru import logger

from Config import get_config
from Llm.LlmProvider import ChatResponse, parse_json_safe, measure_ms


class OllamaProvider:
    """Ollama Chat API 호출 (http://localhost:11434/api/chat)"""

    def __init__(self) -> None:
        cfg = get_config()
        self._model = cfg.ollama_model
        self._host = cfg.ollama_host.rstrip("/")
        self._default_temp = cfg.llm_temperature
        self._default_timeout_ms = cfg.llm_timeout_ms

    @property
    def name(self) -> str:
        return "ollama"

    @property
    def model(self) -> str:
        return self._model

    def chat(
        self,
        *,
        system: str,
        user: str,
        format: str | None = None,
        temperature: float | None = None,
        timeout_ms: int | None = None,
    ) -> ChatResponse:
        start = time.perf_counter()
        payload = {
            "model": self._model,
            "messages": [
                {"role": "system", "content": system},
                {"role": "user", "content": user},
            ],
            "stream": False,
            "options": {
                "temperature": temperature if temperature is not None else self._default_temp,
            },
            # Qwen3.5 등 reasoning 모델의 chain-of-thought 비활성 (응답 시간 ↓).
            # 무관한 모델은 무시 — 호환성 안전.
            "think": False,
            # 모델을 메모리에 1시간 유지 — 콜드 스타트 회피 (기본 5분).
            "keep_alive": "1h",
        }
        if format == "json":
            payload["format"] = "json"

        timeout_s = (timeout_ms or self._default_timeout_ms) / 1000.0

        try:
            with httpx.Client(timeout=timeout_s) as client:
                r = client.post(f"{self._host}/api/chat", json=payload)
            if r.status_code != 200:
                logger.warning(f"[OllamaProvider] status={r.status_code} body={r.text[:200]}")
                return ChatResponse(
                    text="", json_obj=None, latency_ms=measure_ms(start),
                    provider=self.name, model=self._model,
                    error=f"API_ERROR_{r.status_code}",
                )

            data = r.json()
            text = data.get("message", {}).get("content", "")
            json_obj = parse_json_safe(text) if format == "json" else None

            return ChatResponse(
                text=text,
                json_obj=json_obj,
                latency_ms=measure_ms(start),
                provider=self.name,
                model=self._model,
            )
        except httpx.TimeoutException:
            return ChatResponse(
                text="", json_obj=None, latency_ms=measure_ms(start),
                provider=self.name, model=self._model, error="TIMEOUT",
            )
        except httpx.ConnectError:
            return ChatResponse(
                text="", json_obj=None, latency_ms=measure_ms(start),
                provider=self.name, model=self._model, error="OLLAMA_OFFLINE",
            )
        except Exception as e:
            logger.exception(f"[OllamaProvider] 예외: {e}")
            return ChatResponse(
                text="", json_obj=None, latency_ms=measure_ms(start),
                provider=self.name, model=self._model, error="EXCEPTION",
            )

    def health(self) -> bool:
        """Ollama 서버 도달성"""
        try:
            with httpx.Client(timeout=2.0) as client:
                r = client.get(f"{self._host}/api/tags")
            return r.status_code == 200
        except Exception:
            return False
