# LLM 학습+추론 PC — 설계 문서

| 항목 | 값 |
|---|---|
| 작성일 | 2026-05-15 |
| 대상 PC | `10.10.10.128:8002` (학습 + 추론 동거) |
| GPU 총 VRAM | 24 GB · **MediBridge 할당 16 GB** (공용 PC) |
| 런타임 | **OpenAI gpt-4.1-nano** (기본) + Ollama (로컬 대체) |
| 임베딩 | **nlpai-lab/KURE-v1** (한국어 SOTA, 568M, ~2.5 GB) |
| 벡터 DB | Chroma (local persist) |
| 관련 문서 | [LLM_Model_Candidates.md](LLM_Model_Candidates.md) · [Api/SpeechApi.md](Api/SpeechApi.md) · [Meeting_2026-05-15.md](Meeting_2026-05-15.md) |

---

## 1. 결정 사항 (확정)

| # | 결정 | 비고 |
|---|---|---|
| 1 | **기본 LLM = OpenAI gpt-4.1-nano (외부 API)** | 사용자 결정 — 비용 ~$0.000025/요청, 응답 ~300-600 ms |
| 2 | 로컬 fallback = Gemma 4 E4B Q4 (Ollama, ~3 GB) | 인터넷 단절·OpenAI 장애 시 |
| 3 | 임베딩 = KURE-v1 (~2.5 GB, MTEB-ko 1위) | 한국어 알약·문서 검색 |
| 4 | 벡터 DB = Chroma | local persist, gitignore |
| 5 | **VRAM 합산 ≤ 16 GB** | 공용 PC 다른 팀 작업 공존 |
| 6 | `LlmProvider` 추상화 | 코드 변경 X, 환경변수만으로 OpenAI ↔ Ollama 전환 |
| 7 | 의료 안내 영역 LLM 미사용 | OpenAI 도 DUR 위험 안내에는 호출 X (요구사항 §3.2) |
| 8 | 인젝션 다층 방어 | InjectionFilter + JSON 스키마 + Temperature=0 + 외부 출처 차단 |

---

## 2. 전체 구조

```
┌─────────────────────────────────────────────────────────────────────────┐
│  LLM PC (10.10.10.128 : FastAPI :8002)                                  │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │  Routers (FastAPI)                                              │    │
│  │  /intent · /onboarding · /summary · /speech · /monitoring       │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│         │                                                                │
│         ▼                                                                │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │  Llm 비즈니스 로직                                              │    │
│  │  IntentClassifier · OnboardingNormalizer ·                       │    │
│  │  OnboardingDisambiguator · NonMedicalSummarizer                  │    │
│  │     │              │            │                                │    │
│  │     │              │            └─→ OutputSanitizer (단정 차단) │    │
│  │     │              └─→ RagClient (Chroma)                        │    │
│  │     └─→ InjectionFilter (이미 구현 ✅)                          │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│         │                          │                                     │
│         ▼                          ▼                                     │
│  ┌──────────────────┐      ┌──────────────────┐                         │
│  │  LlmProvider     │      │  RagClient       │                         │
│  │  (추상화)        │      │  (Chroma)        │                         │
│  └──────────────────┘      └──────────────────┘                         │
│      │       │                  │                                        │
│      │       │                  ▼                                        │
│      │       │             [ chroma_db/ ]                                │
│      │       │             pdma_overview · dur_interactions              │
│      │       │                                                            │
│      │       └──→ OllamaProvider ──→ [ Ollama :11434 ]                  │
│      │                                gemma4:e4b (fallback)              │
│      └──→ OpenAIProvider ──→ [ api.openai.com ]                          │
│                                gpt-4.1-nano (기본)                       │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
         ▲                                ▲
         │                                │
[ MainServer 10.10.10.97 ]      [ TrainingServer (같은 PC) ]
  REST 호출                       BuildRagIndex.py 1회 빌드
                                  → chroma_db/ 생성 → 같은 디렉토리 공유
```

---

## 3. 디렉토리 구조 (구현 대상)

```
InferenceServer/                            ★ = 신규 / ✓ = 채움 / ⏸ = 다른 담당
├─ Main.py                                  ✓ (FastAPI 진입점, 이미 골격)
├─ Config.py                                ✓ (env 로드)
├─ requirements.txt                         ✓ + 신규 의존성 추가
├─ .env.sample                              ★ 신규
├─ chroma_db/                               ★ 신규 (gitignore)
│   ├─ pdma_overview/                       (e약은요 본문 청크)
│   └─ dur_interactions/                    (DUR 페어)
├─ Llm/
│   ├─ LlmProvider.py                       ★ 신규 — 추상화 + 팩토리
│   ├─ OpenAIProvider.py                    ★ 신규 — gpt-4.1-nano (기본)
│   ├─ OllamaProvider.py                    ★ 신규 — gemma4:e4b (fallback)
│   ├─ RagClient.py                         ★ 신규 — Chroma + KURE-v1
│   ├─ PromptTemplates.py                   ★ 신규 — 시스템 프롬프트 정본
│   ├─ OutputSanitizer.py                   ★ 신규 — 단정 표현 차단
│   ├─ InjectionFilter.py                   ✅ 이미 구현
│   ├─ IntentClassifier.py                  ✓ 채움 (501 → 200)
│   ├─ OnboardingNormalizer.py              ✓ 채움
│   ├─ OnboardingDisambiguator.py           ✓ 채움
│   └─ NonMedicalSummarizer.py              ✓ 채움
├─ Routers/
│   ├─ Intent.py                            ✓ 채움
│   ├─ Onboarding.py                        ✓ 채움
│   ├─ Summary.py                           ✓ 채움
│   ├─ Speech.py                            ⏸ Whisper fallback (확장, 골격만)
│   ├─ Vision.py                            ⏸ 인효 담당 — 본 PC 아님 (라우팅만 둠)
│   └─ Monitoring.py                        ✓ 채움 (Ollama/OpenAI/Chroma 상태)
├─ Schemas/                                 ✅ 이미 구현 (Pydantic)
├─ Speech/
│   └─ WhisperFallback.py                   ⏸ (확장)
└─ Monitoring/                              ✅ 이미 구현

TrainingServer/
├─ Scripts/
│   ├─ BuildRagIndex.py                     ★ 신규 — MariaDB → Chroma 인덱스 빌드
│   ├─ EvalPrompts.py                       ★ 신규 (선택) — 응답 시간 측정
│   ├─ TrainYolo.py                         ⏸ 인효 담당
│   └─ FinetuneOcr.py                       ⏸ 인효 담당
└─ requirements.txt                         ✓ 추가
```

---

## 4. LlmProvider 추상화

### 4.1 인터페이스

```python
from typing import Protocol, Optional
from dataclasses import dataclass

@dataclass
class ChatResponse:
    text: str
    json_obj: Optional[dict]
    latency_ms: int
    provider: str       # "openai" / "ollama"
    model: str

class LlmProvider(Protocol):
    def chat(self, *, system: str, user: str,
             format: str | None = None,
             temperature: float = 0.0,
             timeout_ms: int = 8000) -> ChatResponse: ...
```

### 4.2 환경변수 (Config)

```bash
# 기본 = OpenAI gpt-4.1-nano
MEDIBRIDGE_LLM_BACKEND=openai
OPENAI_MODEL=gpt-4.1-nano
OPENAI_API_KEY=sk-...

# Fallback = Ollama
OLLAMA_HOST=http://localhost:11434
OLLAMA_MODEL=gemma4:e4b

# 임베딩 (KURE-v1)
EMBEDDING_MODEL=nlpai-lab/KURE-v1
EMBEDDING_DEVICE=cuda

# Chroma
CHROMA_PERSIST_DIR=./chroma_db
```

### 4.3 팩토리

```python
def make_provider() -> LlmProvider:
    backend = os.getenv("MEDIBRIDGE_LLM_BACKEND", "openai")
    if backend == "openai":
        return OpenAIProvider(model=os.getenv("OPENAI_MODEL", "gpt-4.1-nano"),
                              api_key=os.environ["OPENAI_API_KEY"])
    return OllamaProvider(model=os.getenv("OLLAMA_MODEL", "gemma4:e4b"),
                          host=os.getenv("OLLAMA_HOST", "http://localhost:11434"))
```

라우터는 추상화만 의존:
```python
provider = make_provider()              # 시작 시 1회
resp = provider.chat(system=SYS, user=text, format="json")
```

---

## 5. 라우터 상세

### 5.1 `POST /intent/classify` — Stage 0.5 의도 분류

**요청**:
```json
{ "text": "이 약 뭐예요?", "image_request_id": "req_abc" }
```

**처리**:
1. `InjectionFilter.detect(text)` → 매칭 시 즉시 `OTHER + injection_flag=true`
2. `LlmProvider.chat(system=INTENT_SYSTEM, user=text, format="json", temperature=0)`
3. Pydantic `IntentClassifyResponse` 검증 → 스키마 위반 시 `OTHER` 강제
4. 응답

**시스템 프롬프트** (`PromptTemplates.INTENT_SYSTEM`):
```
당신은 약품 챗봇의 의도 분류기입니다.
사용자 발화를 다음 7개 중 정확히 하나로 분류:
PILL_IDENTIFY / RISK_CHECK / INFO_LOOKUP /
REGISTER_REQUEST / HISTORY_QUERY / REPORT_REQUEST / OTHER

응답 형식 (반드시 JSON):
{"category": "<one-of-7>", "confidence": <0.0-1.0>}

규칙:
- 위 7개 외 값 출력 금지
- 자연어 설명 금지 — JSON 만 출력
- 의료 조언·진단·복약 지시 금지 (OTHER 로 분류)
- 한국어 외 응답 금지
```

**응답** (요구사항 호환):
```json
{
  "intent": {"category": "PILL_IDENTIFY", "confidence": 0.93},
  "injection_flag": false
}
```

### 5.2 `POST /onboarding/normalize` — 약명 정규화

**요청**: `{"text": "타이레놀 오백 짜리"}`

**처리**:
1. InjectionFilter
2. LLM 호출 → 후보 약명 list 추출
3. (선택) Chroma 로 검증 — 식약처 등재 약품만 유지
4. 응답

**프롬프트**:
```
한국어 STT 결과에서 약품명만 추출·정규화하여 JSON 으로 출력.

규칙:
- 1~5개 후보 (확신 없으면 빈 배열)
- 약품 외 발화는 빈 배열
- 한국어 외 응답 금지
- 자연어 설명·의료 조언 금지

응답:
{"candidates": [{"normalized": "타이레놀500mg", "raw_phrase": "..."}, ...]}
```

### 5.3 `POST /onboarding/disambiguate` — 동명 후보 분기 질문

**요청**:
```json
{
  "candidates": [
    {"item_code": "X1", "drug_name": "OO정 (혈압)"},
    {"item_code": "X2", "drug_name": "OO정 (진통)"}
  ]
}
```

**처리**: LLM 으로 1회 분기 질문 생성. 사용자 선택은 별도 라우터 X (다음 API 호출에 item_code 포함).

**응답**:
```json
{
  "question": "예전에 드신 OO정(혈압)인가요, 최근 드신 OO정(진통)인가요?",
  "options": [{"item_code": "X1", "label": "혈압약"}, ...]
}
```

### 5.4 `POST /summary/non-medical` — e약은요 비위험 정보 요약

**요청**: `{"item_code": "999800001", "section": "usage"}`

**처리**:
1. 메인서버 DB `drug_overview` 에서 원문 SELECT (또는 메인서버 API 호출)
2. RagClient 로 유사 청크 추가 검색 (Chroma `pdma_overview`)
3. LLM 으로 자연스러운 한국어 요약 1-2 문단
4. **OutputSanitizer** 로 단정 표현 거부 ("복용 가능합니다 / 안전합니다 / 위험합니다")

**프롬프트**:
```
식약처 e약은요 본문을 자연스러운 한국어 1-2문장으로 요약.

규칙:
- 출력은 식약처 본문 사실만 — 의료 조언·진단·복약 지시 금지
- "복용 가능 / 불가 / 안전 / 위험 / 진단" 단정 어구 금지
- "약사·의사 상담을 권유드립니다" 톤 유지
- 외부 출처 인용 금지 (식약처 본문 외)
```

### 5.5 `POST /speech/stt` — Whisper fallback (확장)

MVP 미구현. 인터페이스만 정의. (FR-A4-05)

### 5.6 `GET /monitoring/*` — 자체 상태

- `/health` — OpenAI 도달성 + Ollama 도달성 + Chroma 컬렉션 존재 여부
- `/metrics` — 라우터별 호출 카운트·평균 응답 시간

---

## 6. RAG 인덱싱 (학습 서버 = 같은 PC)

### 6.1 데이터 소스 (메인서버 DB)
| 컬렉션 | 출처 테이블 | 청크 단위 | 메타데이터 |
|---|---|---|---|
| `pdma_overview` | `drug_overview` | 섹션 (efficacy/usage/warning/caution/interaction/side_effect/storage) | item_code · drug_name · section |
| `dur_interactions` | `dur_interaction_cache` | 페어 1개 | dur_id · base_item_code · target_item_code · dur_type · prohibit_reason |

### 6.2 빌드 흐름

```bash
# 1. 메인서버 DB 에 접근 가능한지 확인 (TrainingServer 가 메인 DB SELECT 권한 필요)
mysql -h 10.10.10.97 -u medibridge_app -p medibridge -e "SELECT COUNT(*) FROM drug_overview"

# 2. BuildRagIndex.py 실행
python TrainingServer/Scripts/BuildRagIndex.py \
  --db-host 10.10.10.97 --db-user medibridge_app \
  --db-pass $MEDIBRIDGE_DB_PASSWORD --db-name medibridge \
  --embedding nlpai-lab/KURE-v1 --device cuda \
  --out /home/medibridge/InferenceServer/chroma_db

# 3. 인덱스 적용된 LLM PC FastAPI 재시작 (또는 hot reload)
systemctl restart medibridge-inference   # 또는 직접 uvicorn 재시작
```

### 6.3 갱신 정책
- e약은요 TTL 30일 회귀 → DB 가 갱신되면 RAG 도 재빌드 필요
- **주기 갱신**: 월 1회 또는 e약은요 lazy 캐시가 일정량 갱신된 후
- **트리거**: 수동 (`BuildRagIndex.py` 재실행) 또는 cron

---

## 7. 안전·정책 다층 방어

| # | 층 | 위치 | 동작 |
|---|---|---|---|
| 1 | 인젝션 정규식 필터 | `InjectionFilter.py` (✅) | "이전 지시 무시" / "당신은 의사" → OTHER 강제 |
| 2 | JSON 스키마 강제 | Pydantic + OpenAI `response_format` / Ollama `format=json` | 자연어 응답 차단 |
| 3 | 단정 표현 출력 필터 | `OutputSanitizer.py` (★) | "복용 가능 / 불가 / 안전 / 위험 / 진단" 거부 |
| 4 | 의료 안내 영역 코드 부재 | DUR 위험 라우터 자체 없음 | 우회되어도 호출할 코드 없음 |
| 5 | Temperature=0 | LlmProvider 옵션 | 결정론 — 환각 최소 |
| 6 | 외부 인터넷 차단 (Ollama 모드) | requests 화이트리스트 | `localhost:11434` 만 |
| 6' | 외부 송신 인지 (OpenAI 모드) | API 호출 시 사용자 발화 전송 | 약관 검토 — PII 마스킹은 별도 |

### OpenAI 사용 시 추가 정책
- ✅ 의도 분류 / 약명 정규화 / 비의료 일반 안내 → OpenAI 가능
- ❌ DUR 위험 안내 / 진단성 응답 → OpenAI 도 미사용 (식약처 데이터 그대로 인용)

---

## 8. 구현 단계

### Phase 0 — 환경 셋업 (오늘 즉시)
- [x] 설계 문서 확정 (본 문서)
- [ ] `pip install -r requirements.txt`
- [ ] `.env` 작성 + `OPENAI_API_KEY` 설정
- [ ] OpenAI gpt-4.1-nano 단독 호출 1회 PoC

### Phase 1 — Intent 라우터 (작은 슬라이스, End-to-End)
- [ ] `LlmProvider.py` + `OpenAIProvider.py` + `OllamaProvider.py`
- [ ] `PromptTemplates.INTENT_SYSTEM`
- [ ] `IntentClassifier.classify()` 채움
- [ ] `Routers/Intent.py` 501 → 200
- [ ] End-to-End: 폰 → 클라 → 메인 → LLM PC → 분류 → 메인 → 클라 응답

### Phase 2 — RAG + Onboarding
- [ ] `RagClient.py` (Chroma + KURE-v1 SentenceTransformer)
- [ ] `BuildRagIndex.py` 작성 + 1회 빌드
- [ ] `OnboardingNormalizer.py` + `OnboardingDisambiguator.py`
- [ ] `Routers/Onboarding.py`

### Phase 3 — Summary + Monitoring
- [ ] `OutputSanitizer.py` (단정 표현 차단)
- [ ] `NonMedicalSummarizer.py`
- [ ] `Routers/Summary.py`
- [ ] `Routers/Monitoring.py` (OpenAI/Ollama/Chroma 헬스)

### Phase 확장
- Whisper STT fallback / 응답 캐싱 / 회귀 평가 / 멀티 모델

---

## 9. 응답 시간 / 비용 예측

| 라우터 | 모델 | 응답 시간 (예상) | 비용 |
|---|---|---|---|
| `/intent/classify` | gpt-4.1-nano | ~400 ms | ~$0.000025 |
| `/onboarding/normalize` | gpt-4.1-nano | ~500 ms | ~$0.00003 |
| `/onboarding/disambiguate` | gpt-4.1-nano | ~500 ms | ~$0.00003 |
| `/summary/non-medical` | gpt-4.1-nano | ~800 ms | ~$0.00005 |

10,000 회/월 시연 가정 → 약 **$1 미만** ≈ 1,300원 미만. 사실상 무시 가능.

---

## 10. 위험 / 대응

| # | 위험 | 영향 | 대응 |
|---|---|---|---|
| 1 | OpenAI API 장애 | 의도 분류 불가 | LlmProvider 가 자동으로 Ollama 로 fallback |
| 2 | 인터넷 단절 | OpenAI 사용 불가 | 환경변수 `MEDIBRIDGE_LLM_BACKEND=ollama` 즉시 전환 |
| 3 | OpenAI 응답 가변 | 모델 업데이트로 출력 변경 | 모델 버전 고정 권장 (`gpt-4.1-nano-2025-XX-XX`) |
| 4 | KURE-v1 다운로드 실패 | 임베딩 사용 불가 | 사전 다운로드 (HuggingFace `snapshot_download`) |
| 5 | Chroma 손상 | RAG 검색 실패 | `BuildRagIndex.py` 재실행 (~10분) |
| 6 | API Key 노출 | 비용 폭주 | `.env` gitignore + 키 회전 |
| 7 | 한국어 정확도 부족 | 약명 정규화 오류 | Stage 2 — Qwen2.5 7B 또는 EXAONE 비교 |

---

## 11. 참고 자료
- [LLM_Model_Candidates.md](LLM_Model_Candidates.md) — 모델 비교·VRAM 예산
- [Api/SpeechApi.md](Api/SpeechApi.md) — 인터페이스 계약
- [KURE GitHub](https://github.com/nlpai-lab/KURE)
- [Chroma docs](https://docs.trychroma.com/)
- [OpenAI API Reference](https://platform.openai.com/docs/api-reference)
- [Ollama Library](https://ollama.com/library/gemma4)
