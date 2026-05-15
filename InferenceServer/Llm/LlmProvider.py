"""
LlmProvider — LLM 백엔드 추상화 + 3-모드 선택

3가지 모드 (환경변수 MEDIBRIDGE_LLM_BACKEND):
    - "ollama" — 로컬 Ollama 만 사용 (gemma4:e4b 등). 외부 송신 X. Fallback 없음.
    - "openai" — OpenAI API 만 사용 (gpt-4.1-nano). 인터넷 의존. Fallback 없음.
    - "auto"   — OpenAI 우선 + 실패 시 자동 Ollama fallback.
                 API_KEY_MISSING / 401 / 429 / 5xx / TIMEOUT / EXCEPTION 발생 시 전환.

사용자가 명시적으로 mode 선택 — 무조건 fallback 아님.

사용:
    from Llm.LlmProvider import make_provider
    provider = make_provider()
    resp = provider.chat(system="...", user="...", format="json")
    print(resp.provider, resp.error, resp.json_obj)
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

# auto 모드에서 fallback 트리거 에러 코드 — 일시적·복구 가능 한 종류만
_FALLBACK_TRIGGER_ERRORS = {
    "API_KEY_MISSING",       # OpenAI Key 미설정
    "API_ERROR_401",         # 인증 실패
    "API_ERROR_429",         # rate limit
    "TIMEOUT",               # 응답 타임아웃
    "EXCEPTION",             # 네트워크·DNS 등 일반 예외
}


def _is_fallback_trigger(err: Optional[str]) -> bool:
    if not err:
        return False
    if err in _FALLBACK_TRIGGER_ERRORS:
        return True
    # 5xx 류 (API_ERROR_500 ~ 599) 도 fallback
    if err.startswith("API_ERROR_5"):
        return True
    return False


class FallbackProvider:
    """
    Primary 호출 → 실패(_FALLBACK_TRIGGER_ERRORS) 시 Secondary 로 자동 전환.

    auto 모드 전용 — 사용자가 backend="auto" 선택했을 때만 사용.
    primary 가 정상이면 secondary 는 lazy init (메모리 절약).
    """

    def __init__(self, primary: "LlmProvider", secondary_factory):
        self._primary = primary
        self._secondary_factory = secondary_factory
        self._secondary = None    # lazy
        # primary 가 살아있는 한 secondary 로 stick 하지 않음 (매번 primary 재시도).
        # → "OpenAI 가 일시 장애였다" 케이스 자동 회복.

    @property
    def name(self) -> str:
        return f"auto({self._primary.name})"

    @property
    def model(self) -> str:
        return self._primary.model

    def _get_secondary(self):
        if self._secondary is None:
            self._secondary = self._secondary_factory()
            logger.info(f"[LlmProvider] fallback secondary 활성화: {self._secondary.name}/{self._secondary.model}")
        return self._secondary

    def chat(self, **kwargs) -> "ChatResponse":
        resp = self._primary.chat(**kwargs)
        if not _is_fallback_trigger(resp.error):
            return resp
        logger.warning(
            f"[LlmProvider] primary 실패 ({resp.error}) → fallback 시도"
        )
        sec = self._get_secondary()
        fb_resp = sec.chat(**kwargs)
        # primary 실패 흔적 보존
        fb_resp.provider = f"{self.name}->{sec.name}"
        return fb_resp

    def health(self) -> bool:
        # primary 가 살아있으면 OK. 죽었어도 secondary 살아있으면 OK.
        if self._primary.health():
            return True
        return self._get_secondary().health()


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
        logger.info(f"[LlmProvider] backend=OpenAI (단독) model={cfg.openai_model}")
    elif backend == "ollama":
        from Llm.OllamaProvider import OllamaProvider
        _provider = OllamaProvider()
        logger.info(f"[LlmProvider] backend=Ollama (단독) model={cfg.ollama_model} host={cfg.ollama_host}")
    elif backend in ("auto", "openai_fallback_ollama"):
        from Llm.OpenAIProvider import OpenAIProvider
        from Llm.OllamaProvider import OllamaProvider
        primary = OpenAIProvider()
        _provider = FallbackProvider(
            primary=primary,
            secondary_factory=lambda: OllamaProvider(),
        )
        logger.info(
            f"[LlmProvider] backend=auto — primary=OpenAI({cfg.openai_model}) "
            f"fallback=Ollama({cfg.ollama_model})"
        )
    else:
        raise ValueError(
            f"Unknown LLM backend: {backend} (expected: ollama|openai|auto)"
        )

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
