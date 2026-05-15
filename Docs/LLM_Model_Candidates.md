# LLM 모델 후보 비교표 — MediBridge LLM PC

| 항목 | 값 |
|---|---|
| 작성일 | 2026-05-15 |
| 작성자 | (자료 정리) Claude — 팀 검토용 초안 |
| 대상 PC | LLM 추론 PC (`10.10.10.120:8002`) |
| GPU/VM 메모리 | **24 GB** |
| 라이선스 | 상업적 사용 가능 (포트폴리오 → 사업화 대비) |
| 런타임 후보 | Ollama (1순위) · vLLM · llama.cpp |

---

## 0. TL;DR — 빠른 결론

| 단계 | 추천 1순위 | 추천 2순위 | 비고 |
|---|---|---|---|
| **Stage A** — 추론서버 키값 4종 → 약품 식별·정규화 | **Gemma 4 E4B** (~3 GB Q4) | Qwen2.5 3B (~2 GB Q4) | 가벼움, 한국어 OK, Ollama 즉시 |
| **Stage B** — 사용자 자연어 질문 응답 + RAG | **Gemma 4 26B A4B** (MoE ~13 GB Q4) | Gemma 4 31B Dense Q4 (~19 GB) | **동주 검증 모델 가능성 ↑** |
| **Plan B** — 시간 부족 단축 | **Gemma 4 (동주 일주일 테스트 모델)** | — | 이미 24GB 검증, 프롬프트만 개조 |

> 두 단계를 **한 모델로 합치면** Gemma 4 26B A4B 가 유력 (MoE → 26B 전체이지만 추론 시 4B 만 활성, 컨슈머 GPU 24GB 에 최적).
> Stage A/B 분리하면 응답 속도 ↑ + 자원 효율 ↑.

### ⭐ 동주 일주일 테스트 모델 — **Gemma 4** 확정 (2026-04-02 출시)
- Gemma 4 는 2026-04-02 출시 — 4종 (E2B / E4B / 26B A4B (MoE) / 31B Dense)
- **라이선스: Apache 2.0** — 상업 사용 완전 자유 (이전 Gemma 2/3 Terms 보다 자유로움)
- "24GB Max 로 겨우 돌아간다" 표현 매칭:

| 후보 | 사이즈 (Q 양자화) | 24GB 매칭 |
|---|---|---|
| **Gemma 4 31B Dense Q4** ⭐ | ~18-20 GB | ★★★ 가장 가능성 높음 (Max 80%) |
| **Gemma 4 26B A4B Q8** | ~16-19 GB | ★★☆ Max 매칭 |
| **Gemma 4 26B A4B Q6** | ~14-16 GB | ★★☆ |
| Gemma 4 26B A4B Q4 | ~13 GB | ★☆☆ (Max 까진 아님) |
| Gemma 4 31B Dense BF16 | ~62 GB | ❌ 24GB 초과 |
| Gemma 4 E4B | ~9 GB (BF16) / ~3 GB (Q4) | ❌ Max 표현 안 맞음 |

→ **동주 님께 정확한 모델/양자화 사이즈 확인 요청** (31B Q4 / 26B A4B Q6~Q8 가능성)

---

## 1. 후보 모델 비교표

> VRAM 추정치는 **Q4_K_M 양자화 기준** (Ollama 기본). bf16 원본은 약 4배.
> 한국어 점수는 KoBest / KMMLU / Korean LLM Leaderboard 등 공개 벤치 종합 인상 평가.

| # | 모델 | 파라미터 | VRAM (Q4) | 한국어 | 라이선스 | Ollama | 본 프로젝트 적합도 |
|---|---|---|---|---|---|---|---|
| 1 | **Qwen2.5 1.5B Instruct** | 1.5 B | ~1.0 GB | ★★★☆☆ | Apache 2.0 | ✅ `qwen2.5:1.5b` | Stage A 후보. 매우 가벼움 |
| 2 | **Qwen2.5 3B Instruct** | 3 B | ~2.0 GB | ★★★★☆ | Qwen RESEARCH (상업 제한) | ✅ `qwen2.5:3b` | ⚠ 3B 만 상업 제한 — Stage A 1순위지만 라이선스 주의 |
| 3 | **Qwen2.5 7B Instruct** | 7 B | ~4.5 GB | ★★★★☆ | Apache 2.0 | ✅ `qwen2.5:7b` | Stage A+B 통합 — 상업 OK |
| 4 | **Qwen2.5 14B Instruct** | 14 B | ~9 GB | ★★★★★ | Apache 2.0 | ✅ `qwen2.5:14b` | 충분한 여유 + 강력 |
| 5 | **Gemma 2 2B Instruct** | 2 B | ~1.5 GB | ★★★☆☆ | Gemma (상업 OK) | ✅ `gemma2:2b` | 레거시 — Gemma 4 출시 후 권장도 ↓ |
| 6 | **Gemma 2 9B Instruct** | 9 B | ~5.5 GB | ★★★★☆ | Gemma (상업 OK) | ✅ `gemma2:9b` | 레거시 |
| 7 | **Gemma 2 27B Instruct** | 27 B | ~16 GB | ★★★★☆ | Gemma (상업 OK) | ✅ `gemma2:27b` | 레거시 |
| 7-A | **Gemma 4 E2B** | ~2.3 B 효과 | ~1.5 GB Q4 | ★★★★☆ | **Apache 2.0** ✅ | ✅ `gemma4:e2b` | Stage A 후보, 128K 컨텍스트 |
| 7-B | **Gemma 4 E4B** | ~4.5 B 효과 | ~3 GB Q4 | ★★★★☆ | **Apache 2.0** ✅ | ✅ `gemma4:e4b` | Stage A 1순위, 128K |
| 7-C | **Gemma 4 26B A4B** (MoE) ⭐ | 26B / 4B active | ~13 GB Q4 · ~19 GB Q8 | ★★★★★ | **Apache 2.0** ✅ | ✅ `gemma4:26b-a4b` | **MoE — 24GB 최적**, 256K |
| 7-D | **Gemma 4 31B Dense** ⭐ | 31 B | ~19 GB Q4 · BF16 불가 | ★★★★★ | **Apache 2.0** ✅ | ✅ `gemma4:31b` | **동주 24GB Max 검증 추정**, 256K |
| 8 | **Llama 3.2 1B Instruct** | 1 B | ~0.8 GB | ★★☆☆☆ | Llama 3.2 (상업 OK, MAU 7억 미만) | ✅ `llama3.2:1b` | 한국어 약함. fine-tune 필요 |
| 9 | **Llama 3.2 3B Instruct** | 3 B | ~2.2 GB | ★★★☆☆ | Llama 3.2 (상업 OK) | ✅ `llama3.2:3b` | 영문 강세. 한국어 fine-tune 권장 |
| 10 | **Llama 3.1 8B Instruct** | 8 B | ~5 GB | ★★★☆☆ | Llama 3.1 (상업 OK) | ✅ `llama3.1:8b` | Bllossom 등 한국어 fine-tune 풍부 |
| 11 | **Bllossom-Llama-3-8B** | 8 B | ~5 GB | ★★★★☆ | Llama 3 (상업 OK) | ✅ `MLP-KTLim/llama-3-Korean-Bllossom-8B` | **한국어 풀 파인튜닝** — 서울대 MLP 연구실 |
| 12 | **EXAONE 3.5 2.4B Instruct** | 2.4 B | ~1.8 GB | ★★★★☆ | EXAONE AI Model License (연구·비상업) | ✅ `exaone3.5:2.4b` | ⚠ **비상업 라이선스** — 포트폴리오 OK, 사업화 X |
| 13 | **EXAONE 3.5 7.8B Instruct** | 7.8 B | ~5 GB | ★★★★★ | EXAONE AI Model License (연구·비상업) | ✅ `exaone3.5:7.8b` | 한국어 최강급 ⚠ 비상업 |
| 14 | **EXAONE 3.5 32B Instruct** | 32 B | ~19 GB | ★★★★★ | EXAONE AI Model License (연구·비상업) | ✅ `exaone3.5:32b` | 24GB 가능 ⚠ 비상업 |
| 15 | **Solar 10.7B Instruct** | 10.7 B | ~7 GB | ★★★★★ | CC-BY-NC-4.0 (Solar Mini) | ✅ `solar:10.7b` | ⚠ 비상업 — Upstage 한국어 강점 |
| 16 | **Solar Pro Preview 22B** | 22 B | ~14 GB | ★★★★★ | Solar AI License (상업 가능, 조건부) | ⚠ 부분 지원 | 한국어 최상위 — 라이선스 조건 확인 필요 |
| 17 | **HyperCLOVA X SEED 1.5B** | 1.5 B | ~1 GB | ★★★★☆ | HyperCLOVA X SEED License (상업 OK, 별도 약관) | ⚠ HF 직접 로드 | 네이버 한국어 네이티브 작은 모델 |

---

## 2. 평가 기준별 단계 추천

### Stage A — 추론서버 키값 4종 → 약품명 정규화 / 자연어 요약

**입력 (추정)**:
```json
{
  "marking_text": "GS-7",
  "color": "WHITE",
  "shape": "ROUND",
  "size_mm": 9.2
}
```

**원하는 출력**:
- 식약처 낱알식별 캐시 매칭에 사용할 정규화된 키
- 또는 "원형 흰색 정제, 각인 GS-7, 직경 약 9mm" 같은 자연어 요약 1-2문장

**판단**: 매우 단순한 입출력. **1.5~3B 모델로 충분**.

| 순위 | 모델 | 사유 |
|---|---|---|
| 🥇 | Qwen2.5 7B (Apache 2.0) | Stage B 와 통합 가능. 안전한 라이선스. |
| 🥈 | Gemma 2 2B | 더 가벼움. 동일 패밀리(Gemma 2 9B) 와 일관성. |
| 🥉 | Llama 3.2 3B | 영문 강세지만 키 매칭에는 충분. |

### Stage B — 사용자 자연어 질문 응답 (의도 분류 / 약명 정규화 / 비의료 일반 안내 / RAG 요약)

**입력**: 폰 STT 텍스트 (예: "타이레놀이 위장에 안 좋아?")
**원하는 출력**:
- 의도 카테고리 (PILL_IDENTIFY / RISK_CHECK / HISTORY_QUERY / REPORT_REQUEST / OTHER) — JSON 스키마 강제
- 또는 비의료 정보 RAG 요약 (식약처 e약은요 그대로 인용)

**판단**: 한국어 이해 정확도 중요. **7-14B 모델 권장**.

| 순위 | 모델 | 사유 |
|---|---|---|
| 🥇 | **Gemma 2 9B** | 회의 中 동주 일주일 테스트로 24GB 동작 검증됨 ✅ 상업 OK |
| 🥈 | Qwen2.5 14B | 한국어 정확도 ↑ + 24GB 여유 (Q4 ~9GB) |
| 🥉 | Bllossom-Llama-3-8B | 서울대 MLP 한국어 풀 파인튜닝 |

---

## 3. 라이선스 정리 (상업화 대비)

| 라이선스 | 상업 사용 | 비고 |
|---|---|---|
| **Apache 2.0** (Qwen2.5 1.5B/7B/14B) | ✅ 자유 | 가장 안전 |
| **Gemma Terms of Use** (Gemma 2 전 모델) | ✅ 조건부 | "harmful use" 금지. 일반 상업 OK |
| **Llama 3.x** | ✅ 조건부 | MAU 7억 미만이면 자유. 포트폴리오는 무관 |
| **Qwen RESEARCH** (Qwen2.5 3B/72B) | ❌ 제한 | 연구·학술만 |
| **EXAONE AI Model License** | ❌ 비상업 | 연구·교육·평가만. 포트폴리오 데모는 가능, 사업화 X |
| **CC-BY-NC-4.0** (Solar Mini 10.7B) | ❌ 비상업 | 상업화 시 변경 필요 |
| **HyperCLOVA X SEED License** | ✅ 조건부 | 별도 약관 확인 필요 |

> **포트폴리오 단계**: EXAONE / Solar Mini 도 무방 (비영리 데모).
> **사업화 단계 진입 시**: Qwen2.5 7B/14B (Apache) 또는 Gemma 2 9B (Gemma Terms) 로 교체 권장.

---

## 4. 빠른 셋업 명령 (Ollama 기준)

```bash
# 1. Ollama 설치 (Linux)
curl -fsSL https://ollama.com/install.sh | sh

# 2. 모델 받기 (택일)
ollama pull gemma4:26b-a4b         # ⭐ MoE 26B/4B active — 24GB 최적 (Apache 2.0)
ollama pull gemma4:31b             # 31B Dense Q4 — 동주 검증 추정 (Apache 2.0)
ollama pull gemma4:e4b             # 가벼운 Stage A 후보 (Apache 2.0)
ollama pull qwen2.5:14b            # 대안 — Apache 2.0
ollama pull exaone3.5:7.8b         # 한국어 최강 (비상업)
ollama pull MLP-KTLim/llama-3-Korean-Bllossom-8B  # 한국어 fine-tune

# 3. 빠른 한국어 테스트
ollama run gemma4:26b-a4b "타이레놀 500mg 복용법 알려줘"

# 4. API 서버로 띄우기 (기본 11434 포트)
ollama serve
# → POST http://localhost:11434/api/chat
```

`InferenceServer/Llm/IntentClassifier.py` 등에서 `requests.post('http://localhost:11434/api/chat', ...)` 로 호출하면 됨.

---

## 5. 권장 의사결정 트리

```
사업화 의도 있나?
├─ 예 (상업 사용 필수)
│   ├─ 1순위: Gemma 4 26B A4B (MoE, Apache 2.0)
│   │        → 26B 전체이지만 추론 시 4B 만 활성 — 24GB 컨슈머 GPU 최적
│   │        → Stage A+B 통합 가능
│   ├─ 2순위: Gemma 4 31B Dense Q4 (Apache 2.0)
│   │        → 강력하지만 24GB 의 80% 사용 (Max 근접)
│   └─ 3순위: Qwen2.5 14B (Apache 2.0)
│            → MoE 안 쓰는 단순 dense 모델 선호 시
│
└─ 아니오 (포트폴리오만, 비상업 OK)
    ├─ 한국어 최강: EXAONE 3.5 7.8B
    └─ 검증된 안전: Gemma 4 (동주 일주일 테스트 모델 그대로)
```

### 시간 부족 시 (Plan B 단축 경로)
- **동주 이전 프로젝트 = Gemma 4 (사이즈 미확정, 26B A4B Q6~Q8 또는 31B Q4 추정)**
- 프롬프트만 개조 → 추가 학습 비용 0, 모델 다운로드 비용도 이미 처리됨
- 라이선스 = **Apache 2.0** → 사업화 진입 시 라이선스 변경 없이 그대로 사용 가능 ✅
- **Gemma 2 → Gemma 4 라이선스 개선**: Gemma Terms (조건부) → Apache 2.0 (완전 자유)

---

## 6. RAG 데이터 / 벡터 DB 후보 (참고)

LLM 모델 선정과 별개로, **DUR + 식약처 e약은요** RAG 를 어떻게 구성할지:

| 옵션 | 장점 | 단점 | 권장 |
|---|---|---|---|
| **Chroma DB** (local) | 가장 단순, Python 한 줄 설치 | 대용량 시 느림 | 🥇 1순위 (MVP) |
| **FAISS** (Meta) | 빠름, 검증됨 | API 학습 곡선 | 2순위 |
| **SQLite + cosine** | 새 의존성 0 | 직접 임베딩 관리 | 3순위 |
| **RAG 안 씀, SQL 키워드 매칭만** | 가장 빠름, LLM 부담 ↓ | 검색 정확도 ↓ | Plan B |

**임베딩 모델 추천**:
- `BAAI/bge-m3` — 다국어, Apache 2.0
- `intfloat/multilingual-e5-large` — 한국어 강세
- `jhgan/ko-sroberta-multitask` — 한국어 전용

---

## 7. 다음 액션

- [ ] **동주** — 본인이 일주일 테스트한 Gemma 4 모델의 정확한 variant 확인 (E4B / 26B A4B / 31B Dense?) + 양자화 사이즈 (Q4 / Q6 / Q8 / BF16?)
- [ ] **동주** — 프롬프트 개조 가능성 평가 (Plan B 적용 시간 추산)
- [ ] **윤식** — Stage A/B 통합 vs 분리 결정 (Gemma 4 26B A4B 라면 MoE 라 통합 유리)
- [ ] **공통** — 라이선스 결정 — Gemma 4 가 Apache 2.0 이므로 사업화 대비도 그대로 OK
- [ ] **공통** — Ollama 환경 셋업 + Gemma 4 26B A4B PoC (1시간 이내)
- [ ] **공통** — RAG 벡터 DB 선정 (Chroma 권장)
- [ ] **공통** — 프롬프트 v1 작성 (시스템 프롬프트 + JSON 스키마 강제 + 인젝션 방어)

---

## 8. 참고 자료
- **Gemma 4 공식**: https://ai.google.dev/gemma/docs/core
- **Gemma 4 Google DeepMind**: https://deepmind.google/models/gemma/gemma-4/
- **Gemma 4 Ollama**: https://ollama.com/library/gemma4
- **Gemma 4 발표 블로그**: https://blog.google/innovation-and-ai/technology/developers-tools/gemma-4/
- **Ollama Library**: https://ollama.com/library
- **Korean LLM Leaderboard (Upstage Open-Ko-LLM)**: https://huggingface.co/spaces/upstage/open-ko-llm-leaderboard
- **KMMLU 한국어 벤치**: https://huggingface.co/datasets/HAERAE-HUB/KMMLU
- **BAAI bge-m3** 임베딩: https://huggingface.co/BAAI/bge-m3
- **Bllossom 한국어 LLM**: https://huggingface.co/MLP-KTLim/llama-3-Korean-Bllossom-8B

---

> ⚠ **주의**: 본 비교표의 한국어 성능 점수는 **공개 벤치 + 일반 인상**이며 본 프로젝트 도메인(약품)에 정확히 매칭되지 않습니다. **반드시 1시간 분량 PoC 로 직접 검증** 후 선정하세요.
