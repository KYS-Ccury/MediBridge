"""
LlmProvider — LLM 백엔드 추상화

목적:
    - OpenAI (gpt-4.1-nano, 기본) ↔ Ollama (gemma4:e4b, fallback) 전환을
      환경변수 한 줄로 가능하게 함. 라우터 코드는 본 인터페이스에만 의존.

사용:
    from Llm.LlmProvider import make_provider
    provider = make_provider()
    resp = provider.chat(system="...", user="...", format="json")
    print(resp.json_obj)
"""
from __future__ import annotations

import time
import json as _json
from dataclasses import dataclass
from typing import Optional, Protocol

from loguru import logger

from Config import get_config


@dataclass
class ChatResponse:
    text: str                       # raw 응답 텍스트
    json_obj: Optional[dict]        # format="json" 이면 파싱된 dict, 아니면 None
    latency_ms: int
    provider: str                   # "openai" / "ollama"
    model: str
    error: Optional[str] = None     # 에러 시 코드 (TIMEOUT / API_ERROR / PARSE_ERROR)


class LlmProvider(Protocol):
    """LLM Provider 인터페이스"""

    def chat(
        self,
        *,
        system: str,
        user: str,
        format: str | None = None,         # "json" 이면 JSON 강제
        temperature: float | None = None,
        timeout_ms: int | None = None,
    ) -> ChatResponse: ...

    def health(self) -> bool:
        """Provider 도달 가능 여부"""
        ...

    @property
    def name(self) -> str:
        """provider 식별자 ("openai" / "ollama")"""
        ...

    @property
    def model(self) -> str:
        """현재 사용 중인 모델 이름"""
        ...


_provider: Optional[LlmProvider] = None


def make_provider() -> LlmProvider:
    """싱글톤 Provider — 시작 시 1회 호출. 환경변수로 결정."""
    global _provider
    if _provider is not None:
        return _provider

    cfg = get_config()
    backend = cfg.llm_backend.lower()

    if backend == "openai":
        from Llm.OpenAIProvider import OpenAIProvider
        _provider = OpenAIProvider()
        logger.info(f"[LlmProvider] backend=OpenAI model={cfg.openai_model}")
    elif backend == "ollama":
        from Llm.OllamaProvider import OllamaProvider
        _provider = OllamaProvider()
        logger.info(f"[LlmProvider] backend=Ollama model={cfg.ollama_model} host={cfg.ollama_host}")
    else:
        raise ValueError(f"Unknown LLM backend: {backend} (expected: openai|ollama)")

    return _provider


def parse_json_safe(text: str) -> Optional[dict]:
    """LLM 응답에서 JSON 추출. 실패 시 None."""
    if not text:
        return None
    # 1) 그대로 파싱
    try:
        return _json.loads(text)
    except _json.JSONDecodeError:
        pass
    # 2) ```json ... ``` 블록 추출
    import re
    m = re.search(r"```(?:json)?\s*(\{.*?\}|\[.*?\])\s*```", text, re.DOTALL)
    if m:
        try:
            return _json.loads(m.group(1))
        except _json.JSONDecodeError:
            pass
    # 3) 첫 { ... } 추출
    m = re.search(r"\{.*\}", text, re.DOTALL)
    if m:
        try:
            return _json.loads(m.group(0))
        except _json.JSONDecodeError:
            pass
    return None


def measure_ms(start: float) -> int:
    return int((time.perf_counter() - start) * 1000)
