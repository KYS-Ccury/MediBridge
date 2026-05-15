# LLM 모델 후보 비교표 — MediBridge LLM PC

| 항목 | 값 |
|---|---|
| 작성일 | 2026-05-15 |
| 개정일 | 2026-05-15 (v2 — 16GB VRAM 한도 + 한국어 임베딩 + 외부 API 옵션 반영) |
| 작성자 | (자료 정리) Claude — 팀 검토용 |
| 대상 PC | LLM 학습+추론 PC (`10.10.10.128:8002`) |
| GPU 총 VRAM | 24 GB |
| **MediBridge 할당 한도** | **16 GB (임베딩 + LLM 합산)** — 공용 PC, 다른 팀 작업과 공존 |
| 라이선스 | 상업적 사용 가능 (포트폴리오 → 사업화 대비) |
| 런타임 후보 | Ollama (로컬) · OpenAI API (외부) — `LlmProvider` 어댑터로 즉시 전환 |

---

## 0. TL;DR — 빠른 결론

### 임베딩 모델 (1개만 고정 상주)
| 추천 | 모델 | 파라미터 | VRAM (FP16) | 한국어 | 비고 |
|---|---|---|---|---|---|
| 🥇 | **nlpai-lab/KURE-v1** | 568 M | ~2.5 GB | ★★★★★ | 고려대 NLP&AI Lab, **MTEB-ko-retrieval 1위**, bge-m3 한국어 fine-tune |
| 🥈 | dragonkue/BGE-m3-ko | 568 M | ~2.5 GB | ★★★★★ | bge-m3 한국어 fine-tune, KURE-v1 다음 |
| 🥉 | BAAI/bge-m3 | 568 M | ~2.5 GB | ★★★★☆ | 다국어, 한국어도 강함, MIT |
| 가벼움 | jhgan/ko-sroberta-multitask | 110 M | ~0.5 GB | ★★★☆☆ | KURE 보다 ~20% 낮음, VRAM 5배 절약 |

### LLM 모델 (Provider 어댑터로 교체 가능)
| Stage | Provider | 모델 | VRAM | 응답 (예상) | 비용 |
|---|---|---|---|---|---|
| **1순위 시작** | Ollama (Local) | **Gemma 4 E4B Q4** | ~3 GB | ~300-500 ms | 0 |
| 2순위 — 한국어 ↑ | Ollama (Local) | Qwen2.5 7B Q4 | ~5 GB | ~700-1200 ms | 0 |
| 3순위 — 더 강력 | Ollama (Local) | Gemma 4 26B A4B Q4 (MoE) | ~13 GB | ~1500-2500 ms | 0 |
| **🌐 외부 fallback** | OpenAI API | **gpt-4o-mini** | 0 (인터넷) | ~400-800 ms | ~$0.0001/요청 |
| 🌐 외부 빠른 모델 | OpenAI API | **gpt-4.1-nano** | 0 (인터넷) | ~300-600 ms | ~$0.00006/요청 |

> 사용자 결정 사항: **외부 API 옵션은 "방법 하나" 로 상시 가용** — `LlmProvider` 인터페이스 + OpenAIProvider 구현체를 처음부터 만들어 둬서 코드 1줄 변경으로 전환 가능하게.

---

## 1. VRAM 16 GB 예산 — 가능 조합 (1순위~7순위)

| # | 임베딩 | LLM | 합계 VRAM | 한국어 점수 | 추천도 |
|---|---|---|---|---|---|
| **1** ⭐ | KURE-v1 (2.5) | **Gemma 4 E4B Q4 (3)** | **5.5 GB** | ★★★★☆ | 🥇 시작 권장 — 매우 여유, 응답 빠름 |
| **2** ⭐ | KURE-v1 (2.5) | Qwen2.5 7B Q4 (5) | 7.5 GB | ★★★★★ | 🥈 의도 분류 정확도 ↑ 필요 시 업그레이드 |
| 3 | KURE-v1 (2.5) | Gemma 4 26B A4B Q4 (13) | 15.5 GB | ★★★★★ | ⚠ 16 GB 빠듯, OOM 위험 — 추천 X |
| 4 | KURE-v1 (2.5) | EXAONE 3.5 7.8B Q4 (5) | 7.5 GB | ★★★★★ | 한국어 최강 ⚠ 비상업 라이선스 |
| 5 | ko-sroberta (0.5) | Gemma 4 26B A4B Q4 (13) | 13.5 GB | ★★★★☆ | 26B 쓰고 싶으면 임베딩 가벼운 걸로 |
| **6** ⭐ | KURE-v1 (2.5) | **OpenAI gpt-4o-mini (외부)** | **2.5 GB** | ★★★★★ | 🥇 VRAM 최소화 — LLM 부담 0 |
| **7** | KURE-v1 (2.5) | OpenAI gpt-4.1-nano (외부) | 2.5 GB | ★★★★☆ | 외부 — 더 저렴/빠름 |

### 💡 권장 시작 순서
```
Stage 1 (이번 주):   #1 — KURE-v1 + Gemma 4 E4B Q4  →  Ollama 셋업 PoC
Stage 2 (성능 부족): #2 — KURE-v1 + Qwen2.5 7B Q4   →  의도 분류 정확도 측정
Stage 3 (전환 검토): #6 — KURE-v1 + gpt-4o-mini     →  외부 API 어댑터 가동
```

→ **시작은 #1**, 응답 시간·정확도 측정 결과 따라 **#2 또는 #6 으로 전환**.

---

## 2. 임베딩 모델 상세 비교

> 한국어 알약 데이터·문서 (DUR·식약처 e약은요) 검색용. **MTEB-ko-retrieval 벤치 기준 정렬**.

| # | 모델 | 파라미터 | 차원 | Max Token | VRAM | MTEB-ko Recall | 라이선스 | 비고 |
|---|---|---|---|---|---|---|---|---|
| 1 | **nlpai-lab/KURE-v1** | 568 M | 1024 | 8192 | ~2.5 GB | **0.7385** | MIT | 🥇 고려대 NLP&AI Lab, 2024-12-21 릴리스 |
| 2 | dragonkue/BGE-m3-ko | 568 M | 1024 | 8192 | ~2.5 GB | ~0.73 | MIT | bge-m3 한국어 fine-tune |
| 3 | upskyy/bge-m3-Korean | 568 M | 1024 | 8192 | ~2.5 GB | ~0.72 | MIT | bge-m3 한국어 fine-tune |
| 4 | BAAI/bge-m3 | 568 M | 1024 | 8192 | ~2.5 GB | 0.7295 | MIT | 다국어 — 한국어도 강함 |
| 5 | intfloat/multilingual-e5-large | 560 M | 1024 | 512 | ~2.0 GB | ~0.65 | MIT | 다국어 |
| 6 | jhgan/ko-sroberta-multitask | 110 M | 768 | 512 | ~0.5 GB | 0.4693 | Apache 2.0 | 한국어 SBERT, 가벼움 |

### 권장 임베딩 — **KURE-v1**
- 한국어 검색 SOTA (MTEB-ko-retrieval 1위)
- bge-m3 기반 fine-tune 이라 안정적
- 1024차원 + 8192 토큰 long context — DUR 페어 + e약은요 섹션 단위 청크 모두 커버
- VRAM 2.5 GB — 16 GB 한도에서 LLM 에 13.5 GB 여유

---

## 3. LLM Local 후보 — VRAM Q4 양자화 기준

> "16 GB - 임베딩 2.5 GB = LLM 13.5 GB 한도" 기준.

| # | 모델 | 파라미터 | VRAM (Q4) | 한국어 | 라이선스 | Ollama 태그 | 16GB 한도 |
|---|---|---|---|---|---|---|---|
| 1 | **Gemma 4 E4B** ⭐ | 4.5 B (effective) | ~3 GB | ★★★★☆ | Apache 2.0 | `gemma4:e4b` | ✅ 매우 여유 |
| 2 | Gemma 4 E2B | 2.3 B | ~1.5 GB | ★★★★☆ | Apache 2.0 | `gemma4:e2b` | ✅ 가장 가벼움 |
| 3 | **Qwen2.5 7B** ⭐ | 7 B | ~5 GB | ★★★★☆ | Apache 2.0 | `qwen2.5:7b` | ✅ 여유 |
| 4 | Qwen3 8B | 8 B | ~5 GB | ★★★★☆ | Apache 2.0 | `qwen3:8b` | ✅ 더 최신 |
| 5 | Llama 3.1 8B | 8 B | ~5 GB | ★★★☆☆ | Llama 3.x | `llama3.1:8b` | ✅ 한국어 fine-tune 필요 |
| 6 | Bllossom-Llama-3-8B | 8 B | ~5 GB | ★★★★☆ | Llama 3 | `MLP-KTLim/llama-3-Korean-Bllossom-8B` | ✅ 한국어 fine-tune 완료 |
| 7 | EXAONE 3.5 7.8B | 7.8 B | ~5 GB | ★★★★★ | ⚠ 비상업 | `exaone3.5:7.8b` | ✅ 단 사업화 X |
| 8 | Qwen2.5 14B | 14 B | ~9 GB | ★★★★★ | Apache 2.0 | `qwen2.5:14b` | ✅ 한국어 ↑ |
| 9 | Gemma 4 26B A4B | 26 B / 4 active | ~13 GB | ★★★★★ | Apache 2.0 | `gemma4:26b-a4b` | ⚠ 임베딩 합쳐서 15.5 GB |
| 10 | Gemma 4 31B Dense | 31 B | ~19 GB | ★★★★★ | Apache 2.0 | `gemma4:31b` | ❌ 단독으로도 한도 초과 |
| 11 | Qwen2.5 3B | 3 B | ~2 GB | ★★★★☆ | ⚠ Research only | `qwen2.5:3b` | ✅ 라이선스 주의 |

### 권장 시작 — **Gemma 4 E4B Q4**
- VRAM 3 GB (임베딩 2.5 + LLM 3 = **5.5 GB**) — 16 GB 한도에서 **10 GB 여유**, 다른 팀 작업 충돌 위험 ↓
- 응답 ~300-500 ms (의도 분류·정규화에는 충분)
- Apache 2.0 — 사업화 OK
- 128 K 컨텍스트 — 약품 정보 긴 텍스트 처리 OK

---

## 4. 외부 API 옵션 — OpenAI 어댑터

회의에서 결정한 **"추론 시간 보고 외부 API 사용 옵션"** 을 처음부터 지원하도록 설계.

### 4.1 추천 모델

| 모델 | 입력 가격 | 출력 가격 | 한국어 | 응답 속도 | 추천 |
|---|---|---|---|---|---|
| **gpt-4o-mini** | $0.15 / 1M tok | $0.60 / 1M tok | ★★★★★ | ~400-800 ms | 🥇 안정적 균형 |
| **gpt-4.1-nano** | $0.10 / 1M tok | $0.40 / 1M tok | ★★★★☆ | ~300-600 ms | 🥈 가장 저렴·빠름 (2025-04 출시) |
| gpt-4o | $2.50 / 1M tok | $10.00 / 1M tok | ★★★★★ | ~600-1500 ms | 비용 큼, 의도 분류엔 과함 |

### 4.2 비용 시뮬레이션 — 의도 분류 1회 (시스템 프롬프트 100 + 입력 50 + 출력 30 토큰 = 180 토큰)

| 모델 | 1회 비용 | 1,000회 비용 | 10,000회 비용 (월 시연 가정) |
|---|---|---|---|
| gpt-4o-mini | $0.000040 | $0.04 ≈ 53원 | $0.40 ≈ 530원 |
| gpt-4.1-nano | $0.000025 | $0.025 ≈ 33원 | $0.25 ≈ 330원 |

→ **포트폴리오 시연·개발 단계 비용 사실상 무시 가능** ($1 미만/월).

### 4.3 외부 API 사용 시 고려사항

| 항목 | 영향 | 대응 |
|---|---|---|
| 인터넷 의존 | 오프라인 시 동작 X | 로컬 Provider 로 자동 fallback |
| 데이터 외부 송신 | OpenAI 서버 도착 — 개인정보 정책 검토 필요 | 발화 텍스트 자체만 송신 (PII 마스킹 X 도 가능) |
| API Key 보안 | 노출 시 비용 폭주 | `OPENAI_API_KEY` 환경변수, .env 파일 git 제외 |
| Rate Limit | RPM 제한 (계정·티어별) | 큐잉·exponential backoff |
| 응답 가변성 | 모델 업데이트 시 출력 변경 | `model="gpt-4o-mini-2025-XX-XX"` 고정 권장 |
| 한국어 정확도 | OpenAI 한국어 매우 우수 | 별도 fine-tune 불필요 |

### 4.4 의료 안내 영역 정책 (필수 준수)

⚠ **OpenAI 사용 시에도 의료 안내 영역에는 호출 X** — 요구사항 §3.2.
- ✅ 의도 분류 / 약명 정규화 / 비의료 일반 안내 → OpenAI 가능
- ❌ DUR 위험 안내 / 진단성 응답 → OpenAI 도 미사용 (식약처 데이터 그대로 인용)

---

## 5. LlmProvider 추상화 패턴

코드 1줄로 Ollama ↔ OpenAI 전환 가능하게 설계.

```python
# InferenceServer/Llm/LlmProvider.py
from typing import Protocol, Optional
from dataclasses import dataclass

@dataclass
class ChatResponse:
    text: str
    json_obj: Optional[dict]
    latency_ms: int
    provider: str           # "ollama" / "openai"
    model: str

class LlmProvider(Protocol):
    def chat(self, *, system: str, user: str,
             format: str | None = None,
             temperature: float = 0.0,
             timeout_ms: int = 5000) -> ChatResponse: ...

# 구현체 1
class OllamaProvider:
    def __init__(self, model="gemma4:e4b", host="http://localhost:11434"):
        ...

# 구현체 2
class OpenAIProvider:
    def __init__(self, model="gpt-4o-mini", api_key=os.getenv("OPENAI_API_KEY")):
        ...

# 팩토리 — 환경변수로 결정
def make_provider() -> LlmProvider:
    backend = os.getenv("MEDIBRIDGE_LLM_BACKEND", "ollama")
    if backend == "openai":
        return OpenAIProvider(model=os.getenv("OPENAI_MODEL", "gpt-4o-mini"))
    return OllamaProvider(model=os.getenv("OLLAMA_MODEL", "gemma4:e4b"))
```

### 환경변수
```bash
# 로컬 (기본)
MEDIBRIDGE_LLM_BACKEND=ollama
OLLAMA_MODEL=gemma4:e4b

# 외부 API (전환 시)
MEDIBRIDGE_LLM_BACKEND=openai
OPENAI_MODEL=gpt-4o-mini
OPENAI_API_KEY=sk-...
```

라우터 코드는 추상화에 의존:
```python
from Llm.LlmProvider import make_provider
provider = make_provider()                          # 시작 시 1회
resp = provider.chat(system=SYS, user=text, format="json")
```

→ 모델 교체 시 코드 0줄 변경, 환경변수만 수정.

---

## 6. 응답 시간 측정 → 전환 의사결정 트리

```
Stage 1 #1 (KURE-v1 + Gemma 4 E4B) PoC
   ↓
   응답 시간 측정 (의도 분류 100회 평균)
   ├─ < 500 ms          → 그대로 유지 ✅
   ├─ 500-1500 ms       → Stage 2 #2 (Qwen2.5 7B) 검토
   │                      또는 외부 API #6 (gpt-4o-mini)
   ├─ > 1500 ms         → 외부 API #6 즉시 전환
   └─ 정확도 < 80%      → Stage 2 #2 또는 외부 API #6
```

측정 스크립트는 `TrainingServer/Scripts/EvalPrompts.py` (신규 예정) 에 포함.

---

## 7. 권장 셋업 명령

### Ollama 로컬 (Stage 1 시작)
```bash
# 1. Ollama 설치 (Ubuntu)
curl -fsSL https://ollama.com/install.sh | sh

# 2. 모델 받기 (Q4 자동)
ollama pull gemma4:e4b              # 권장 시작 ~3 GB
ollama pull qwen2.5:7b              # 업그레이드 후보 ~5 GB
ollama pull gemma4:26b-a4b          # 최강 후보 (임베딩과 합치면 빠듯) ~13 GB

# 3. API 서버 (기본 :11434)
ollama serve

# 4. 빠른 한국어 테스트
ollama run gemma4:e4b "타이레놀 500mg 복용법을 한 문장으로"
```

### 임베딩 (Python)
```bash
pip install sentence-transformers chromadb
```

```python
from sentence_transformers import SentenceTransformer
embedder = SentenceTransformer("nlpai-lab/KURE-v1", device="cuda")
vectors = embedder.encode(["타이레놀500mg", "이부프로펜200mg"])
# (2, 1024)
```

### 외부 API (전환 시)
```bash
pip install openai
export OPENAI_API_KEY="sk-..."
export MEDIBRIDGE_LLM_BACKEND=openai
export OPENAI_MODEL=gpt-4o-mini
```

---

## 8. 권장 의사결정 트리 (최종)

```
사업화 의도 + 데이터 외부 송신 가능?
├─ 외부 송신 OK
│   ├─ 비용 ↓ 우선        → 🥇 #6 KURE-v1 + gpt-4o-mini (VRAM 2.5GB)
│   └─ 더 저렴            → #7 KURE-v1 + gpt-4.1-nano
│
└─ 외부 송신 X (로컬만)
    ├─ 시작/MVP            → 🥇 #1 KURE-v1 + Gemma 4 E4B Q4 (VRAM 5.5GB)
    ├─ 한국어 정확도 ↑    → #2 KURE-v1 + Qwen2.5 7B Q4 (VRAM 7.5GB)
    └─ 한국어 최강 (비상업) → #4 KURE-v1 + EXAONE 3.5 7.8B (VRAM 7.5GB)
```

### 시간 부족 시 (Plan B 단축 경로)
- **동주 이전 프로젝트 = Gemma 4 (사이즈 미확정)** — variant 가 26B A4B 이상이면 임베딩 합쳐 16 GB 빠듯 → **Gemma 4 E4B 로 다운사이즈 권장**
- 프롬프트는 그대로 개조해서 재사용

---

## 9. 다음 액션

- [ ] **동주** — 일주일 테스트한 Gemma 4 variant 확인 (E4B / 26B A4B / 31B?). 16 GB 한도면 E4B 로 다운사이즈 필요할 수 있음
- [ ] **윤식** — `LlmProvider` 추상화 골격 작성 (`InferenceServer/Llm/LlmProvider.py`)
- [ ] **공통** — Stage 1 #1 Ollama + Gemma 4 E4B + KURE-v1 PoC (1시간)
- [ ] **공통** — `EvalPrompts.py` 응답 시간 측정 스크립트 작성
- [ ] **공통** — 측정 결과 따라 Stage 2 (#2) 또는 외부 API (#6) 전환 결정

---

## 10. 라이선스 정리 (간단)

| 모델 | 라이선스 | 상업 사용 | 비고 |
|---|---|---|---|
| nlpai-lab/KURE-v1 | MIT | ✅ 자유 | 고려대 NLP&AI Lab |
| BAAI/bge-m3 | MIT | ✅ 자유 | 다국어 |
| Gemma 4 전 모델 | Apache 2.0 | ✅ 자유 | 2026-04-02 출시 |
| Qwen2.5 7B / 14B | Apache 2.0 | ✅ 자유 | Qwen2.5 3B 만 비상업 |
| Llama 3.x | Llama Community | ✅ 조건부 | MAU 7억 미만 |
| EXAONE 3.5 | EXAONE AI Model License | ❌ 비상업 | 연구·평가만 |
| OpenAI gpt-4o-mini / nano | OpenAI 약관 | ✅ 자유 | 외부 호출, 데이터 처리 약관 검토 |

---

## 11. 참고 자료
- **KURE 공식 (한국어 임베딩 SOTA)**: https://github.com/nlpai-lab/KURE
- **BAAI bge-m3**: https://huggingface.co/BAAI/bge-m3
- **dragonkue/BGE-m3-ko**: https://huggingface.co/dragonkue/BGE-m3-ko
- **Gemma 4 공식**: https://ai.google.dev/gemma/docs/core
- **Gemma 4 Google DeepMind**: https://deepmind.google/models/gemma/gemma-4/
- **Gemma 4 Ollama**: https://ollama.com/library/gemma4
- **Ollama Library**: https://ollama.com/library
- **OpenAI API Pricing**: https://openai.com/api/pricing/
- **Korean LLM Leaderboard**: https://huggingface.co/spaces/upstage/open-ko-llm-leaderboard
- **MTEB Leaderboard (다국어)**: https://huggingface.co/spaces/mteb/leaderboard

---

## 12. 변경 이력

| 버전 | 일자 | 변경 사항 |
|---|---|---|
| v1 | 2026-05-15 | 초안 — 17개 후보 비교 + Gemma 4 라인업 + 사업화 의사결정 트리 |
| **v2** | **2026-05-15** | **VRAM 16 GB 한도 + 한국어 특화 임베딩 + 외부 API 옵션 추가** — KURE-v1 (한국어 임베딩 SOTA) 1순위 / Gemma 4 E4B Q4 시작 권장 / OpenAI gpt-4o-mini 외부 fallback / `LlmProvider` 추상화 패턴 / 응답 시간 기반 전환 의사결정 트리 |

---

> ⚠ **주의**: 본 비교표의 한국어 성능 점수는 **공개 벤치 + 일반 인상**이며 본 프로젝트 도메인(약품)에 정확히 매칭되지 않습니다. **반드시 1시간 분량 PoC 로 직접 검증** 후 선정하세요.
