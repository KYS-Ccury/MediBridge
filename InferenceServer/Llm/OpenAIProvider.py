"""
OpenAIProvider — gpt-4.1-nano (기본) 호출

장점:
    - VRAM 0 (인터넷 의존)
    - 한국어 우수 + 응답 빠름 (~300-600 ms)
    - 비용 ~$0.000025/요청 (시연 단계 사실상 무시 가능)

⚠ 정책:
    - 의료 안내 영역(DUR 위험 안내) 에는 호출 X — 라우터 단에서 보장
    - 사용자 발화 텍스트가 외부 송신됨 — PII 정책 검토 필요
    - API Key 노출 금지 (환경변수 OPENAI_API_KEY)
"""
from __future__ import annotations

import time
from typing import Optional

import httpx
from loguru import logger

from Config import get_config
from Llm.LlmProvider import ChatResponse, parse_json_safe, measure_ms


class OpenAIProvider:
    """OpenAI Chat Completions API 호출"""

    def __init__(self) -> None:
        cfg = get_config()
        self._model = cfg.openai_model
        self._api_key = cfg.openai_api_key
        self._base_url = cfg.openai_base_url
        self._default_temp = cfg.llm_temperature
        self._default_timeout_ms = cfg.llm_timeout_ms

        if not self._api_key:
            logger.warning("[OpenAIProvider] OPENAI_API_KEY 미설정 — chat() 호출 시 API_KEY_MISSING 에러 반환")

    @property
    def name(self) -> str:
        return "openai"

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
        if not self._api_key:
            return ChatResponse(
                text="", json_obj=None, latency_ms=0,
                provider=self.name, model=self._model,
                error="API_KEY_MISSING",
            )

        start = time.perf_counter()
        payload = {
            "model": self._model,
            "messages": [
                {"role": "system", "content": system},
                {"role": "user", "content": user},
            ],
            "temperature": temperature if temperature is not None else self._default_temp,
        }
        if format == "json":
            payload["response_format"] = {"type": "json_object"}

        headers = {
            "Authorization": f"Bearer {self._api_key}",
            "Content-Type": "application/json",
        }
        timeout_s = (timeout_ms or self._default_timeout_ms) / 1000.0

        try:
            with httpx.Client(timeout=timeout_s) as client:
                r = client.post(
                    f"{self._base_url}/chat/completions",
                    json=payload,
                    headers=headers,
                )
            if r.status_code != 200:
                logger.warning(f"[OpenAIProvider] status={r.status_code} body={r.text[:200]}")
                return ChatResponse(
                    text="", json_obj=None, latency_ms=measure_ms(start),
                    provider=self.name, model=self._model,
                    error=f"API_ERROR_{r.status_code}",
                )

            data = r.json()
            text = data.get("choices", [{}])[0].get("message", {}).get("content", "")
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
        except Exception as e:
            logger.exception(f"[OpenAIProvider] 예외: {e}")
            return ChatResponse(
                text="", json_obj=None, latency_ms=measure_ms(start),
                provider=self.name, model=self._model, error="EXCEPTION",
            )

    def health(self) -> bool:
        """API 도달성 — Authorization 검증으로 짧은 GET (List Models)"""
        if not self._api_key:
            return False
        try:
            with httpx.Client(timeout=3.0) as client:
                r = client.get(
                    f"{self._base_url}/models",
                    headers={"Authorization": f"Bearer {self._api_key}"},
                )
            return r.status_code == 200
        except Exception:
            return False
