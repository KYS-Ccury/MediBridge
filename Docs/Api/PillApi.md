# Pill API — 알약 식별·DUR + 약 풀 관리 + 단계별 좁히기 (모듈 1)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.3 |
| **개정일** | 2026-05-07 |
| **이전 버전** | v0.2 (2026-05-07, in-place 갱신) / v0.1 (2026-05-06) → `Docs/Old/Api/PillApi_v0.1_2026-05-07.md` |
| **모듈** | 모듈 1 (알약 식별 + 안전 점검) |
| **관련 문서** | [프로토콜 v2 §3.1](../프로토콜_ver2.md), [요구사항 분석서 v2 §5.2 / §5.4 / §6.5](../요구사항_분석서_ver2.md), [DB ERD v3](../DB_ERD_ver3.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> 식별·DUR 위험 검출 결과 반환 + 사용자별 약 풀(추가/삭제/전체 리셋) 관리.
> **DUR 위험 안내는 LLM·RAG 미사용**, 식약처 데이터 정해진 템플릿. **단정 문구 금지**.
>
> ⭐ **v0.3 변경 핵심** (Onboarding RAG round-trip 흐름 정의):
> 1. **신규 — `POST /v1/pill/onboarding/normalize`** (음성 등록 시 약명 정규화 + 동명·동성분 분기 질문 + 다회 round-trip 허용)
> 2. **TTS 안내 정책** — 응답에 `tts_text` 필드 추가, 클라 자체 TTS 우선 + 메인 서버 TTS fallback 어댑터 구조
> 3. **횟수·토큰 제한** — round-trip 무한 반복 방지 (`max_rounds`, `max_input_tokens` 명시)
>
> ⭐ **v0.2 변경 핵심** (목업 「메디브릿지 목업.pptx」 분석 반영):
> 1. `IdentifyResponse.candidates[]` 에 **`efficacy_text`·`usage_text`** 추가 (Slide 7 식별 결과 효능·복용법 표시)
> 2. `IdentifyResponse.candidates[]` 에 **`classification_name`** 추가 (Slide 7 약효분류)
> 3. `PoolItem` 에 **`user_category`·`classification_name`** 추가 (Slide 6 "종류: 해열진통제" 표시)
> 4. **신규 — `POST /v1/pill/identify/narrow`** (Slide 3·12 단계별 좁히기 흐름)

---

## 엔드포인트 목록

| HTTP | URL | 설명 | 인증 | 본 버전 |
| --- | --- | --- | --- | --- |
| POST | `/v1/pill/identify` | 이미지 + 약 풀로 식별 + DUR 위험 검출 | ✓ | v0.2 (확장) |
| POST | `/v1/pill/identify/narrow` | 단계별 속성 좁히기 (음성·이미지로 식별 어려울 때 fallback) | ✓ | v0.2 (신규) |
| POST | `/v1/pill/onboarding/normalize` ⭐ | **음성 등록 약명 정규화** (RAG → 식약처 매칭 → 동명·동성분 분기 질문) | ✓ | **v0.3 (신규)** |
| GET | `/v1/pill/pool` | 사용자 약 풀 조회 | ✓ | v0.2 (확장) |
| POST | `/v1/pill/pool` | 약 풀에 약 추가 (등록) | ✓ | v0.2 (확장) |
| DELETE | `/v1/pill/pool/{pool_id}` | 약 1건 개별 삭제 (soft delete) | ✓ | v0.1 |
| DELETE | `/v1/pill/pool/all` | 전체 리셋 (사용자 명시 트리거만) | ✓ | v0.1 |

---

## 1. POST /v1/pill/identify — 식별 + DUR 위험 검출

### 요청
- 헤더: `Content-Type: application/json`, `Authorization: Bearer <JWT>` ✓
- 바디:
```json
{
  "image_request_id": "req_abc123",
  "utterance_request_id": null,
  "include_dur_check": true
}
```

| 필드 | 필수 | 설명 |
| --- | --- | --- |
| `image_request_id` | ✓ | 직전 `POST /v1/media/intent` 의 request_id (v0.2 권장) 또는 `POST /v1/media/image` 의 request_id (레거시) |
| `utterance_request_id` | – | 결합할 발화 request_id |
| `include_dur_check` | – | 기본 `true`. `false` 시 식별 결과만 |

> ⭐ **사진 흐름 (MediaApi v0.2)**: `request_id` 로 `photo_storage` SELECT → status=READY 필요. PENDING/EXPIRED 면 `409 PHOTO_NOT_READY`.

### 응답 (v0.2 확장)
- **200 OK**
```json
{
  "request_id": "req_abc123",
  "candidates": [
    {
      "item_code": "201801234",
      "drug_name": "타이레놀정500mg",
      "confidence": 0.94,
      "match_keys": ["engraving", "shape", "color"],
      "in_user_pool": true,
      "classification_name": "해열, 진통, 소염제",
      "efficacy_text": "(식약처 e약은요 efficacy_text 본문 그대로 인용 — 일부 발췌)",
      "usage_text": "(식약처 e약은요 usage_text 본문 그대로 인용 — 일부 발췌)"
    }
  ],
  "confidence_tier": "HIGH",
  "guidance": {
    "tts_text": "타이레놀정으로 추정됩니다. 화면을 확인해주세요.",
    "active_guide": null,
    "interactive_guide": null,
    "fallback_action": null
  },
  "dur_check": {
    "result": "no_risk_found",
    "checked_at": "2026-05-07T10:30:05+09:00",
    "details": []
  }
}
```

### v0.2 신규 필드 (PillCandidate)

| 필드 | 타입 | 출처 | 설명 |
| --- | --- | --- | --- |
| `classification_name` | string | `pill_identification.classification_name` | 식약처 약효분류명 (Slide 7) |
| `efficacy_text` | string (식약처 본문 그대로) | `drug_overview.efficacy_text` | e약은요 효능 텍스트 (Slide 7) |
| `usage_text` | string (식약처 본문 그대로) | `drug_overview.usage_text` | e약은요 사용법 텍스트 (Slide 7) |

> ⚠️ `efficacy_text`/`usage_text` 는 **LLM 자연어 변환 X**. 식약처 그대로 인용. 길이가 길면 클라가 화면에서 줄바꿈/펼치기 처리.

### v0.2 신규 필드 (guidance)

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| `fallback_action` | enum / null | 식별 실패·LOW 신뢰도 시 클라 라우팅 힌트 — `"NARROW_DOWN"`(단계별 질문) / `"RECAPTURE"`(다시 촬영) / `"CHECK_ENGRAVING"`(각인 확인) / `null` |

### `dur_check.result` 값 (변경 없음)

| 값 | 의미 | 응답 톤 |
| --- | --- | --- |
| `no_risk_found` | DUR 미등록 | "DUR에 등록 확인되지 않았습니다. 안심을 위해 약사·의사 상담을 권유드립니다." |
| `risk_found` | 병용금기 등 위험 | `details` 배열에 식약처 원문 인용 |

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_REQUEST_ID` | image_request_id 형식 오류 |
| 404 | `PHOTO_NOT_FOUND` | image_request_id 에 대응하는 photo_storage row 없음 (운영 모드) |
| 409 | `PHOTO_NOT_READY` | photo_storage status 가 READY 아님 (PENDING/EXPIRED) — `/v1/media/commit` 먼저 |
| 502 | `VISION_UPSTREAM_ERROR` | Vision PC 추론 호출 실패 (운영 모드) |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |
| 404 | `IMAGE_NOT_FOUND` | request_id 에 해당하는 이미지 없음 |
| 422 | `NO_PILL_DETECTED` | YOLO26 검출 실패 → `guidance.fallback_action: "RECAPTURE"` |
| 422 | `NO_PILL_IN_POOL` | 사용자 약 풀이 비어있음 |
| 503 | `INFERENCE_SERVER_UNAVAILABLE` | 추론 서버 연결 끊김 |

### Schemas/ 매핑
- `MainServer/Schemas/PillSchema.h::IdentifyRequest/Response`
- `MainServer/Schemas/PillSchema.h::PillCandidate` (v0.2: efficacy_text·usage_text·classification_name 필드 추가)
- `MainServer/Schemas/PillSchema.h::DurCheckResult/DurDetail`
- `MainServer/Services/Inference/VisionInferenceClient::RemoteDetectParams` (운영 모드 내부 호출)

### ⭐ 운영 모드 내부 흐름 (v0.3)

```
클라 → 메인 /v1/pill/identify
  ↓ photo_storage SELECT (request_id, status=READY 확인)
  ↓ StorageTokenIssuer::issue (op=get, TTL 300s)
  ↓ Vision PC POST /vision/detect_remote (아래 schema)
       {photo_id, storage_url, get_token, mime, purpose}
  ← Vision 응답
       {candidates:[{item_code,drug_name,confidence,match_keys}], confidence_tier}
  ↓ DurQueryEngine::check_combination (사용자 풀과 페어 매칭, 양방향, dedup)
  → 클라 응답 (IdentifyResponse)
```

**메인 → Vision PC 호출 schema** (다른 팀원 측 FastAPI 작성 기준):

```http
POST http://10.10.10.120:8003/vision/detect_remote
Content-Type: application/json

{
  "photo_id":    "ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d",
  "storage_url": "http://10.10.10.122:8004/storage/photos/anon_test_001/ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d.jpg",
  "get_token":   "<HS256 JWT, op=get, exp=now+300>",
  "mime":        "image/jpeg",
  "purpose":     "IDENTIFY"
}
```

→ Vision PC 는 `get_token` 으로 보관 PC GET → YOLO·PaddleOCR·OpenCV 추론 후:

```json
{
  "candidates": [
    { "item_code": "...", "drug_name": "...", "confidence": 0.85,
      "match_keys": ["engraving","shape","color"] }
  ],
  "confidence_tier": "HIGH"
}
```

TestMode (`MEDIBRIDGE_TEST_MODE=true`) 에서는 Vision 호출 우회 — `pill_identification` 시드에서 3건 직접 SELECT.

---

## 2. POST /v1/pill/identify/narrow ⭐ 신규 — 단계별 속성 좁히기

목업 Slide 3·9·12 의 "음성만으로 특정하기 어렵다" → 단계별 질문 흐름. 음성·이미지 식별 신뢰도가 낮을 때 사용자에게 색상·모양·각인 등을 차례로 물어 후보를 좁힘.

### 설계 원칙

- **stateless** — 세션 유지 X. 클라가 누적 attributes 매번 송신.
- **idempotent** — 같은 attributes 면 같은 응답.
- 사용자 약 풀과 결합해 후보 수를 우선 좁힘 (식별 정확도 본질 향상).

### 요청
- 헤더: `Content-Type: application/json`, `Authorization: Bearer <JWT>` ✓
- 바디:
```json
{
  "attributes": {
    "color": "흰색",
    "shape": "캡슐형",
    "has_engraving": "yes",
    "engraving_text": "GS-7"
  },
  "use_pool": true,
  "image_request_id": null,
  "utterance_request_id": null
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| `attributes` | object | ✓ (빈 객체 가능 — 첫 호출) | 누적 속성. 알려진 필드만 채움 |
| `attributes.color` | string | – | "흰색" / "노란색" / "빨간색" / "파란색" / "기타" |
| `attributes.shape` | string | – | "원형" / "타원형" / "장방형" / "캡슐형" / "기타" |
| `attributes.has_engraving` | enum | – | `"yes"` / `"no"` / `"unclear"` |
| `attributes.engraving_text` | string | – | `has_engraving="yes"` 일 때 OCR 또는 사용자 입력 텍스트 |
| `use_pool` | bool | – | 기본 `true`. 사용자 약 풀로 1차 좁힘 |
| `image_request_id` | string | – | 결합할 이미지 request_id |
| `utterance_request_id` | string | – | 결합할 발화 request_id |

### 응답
- **200 OK** — 다음 질문이 있는 경우
```json
{
  "step": 2,
  "total_steps_estimate": 4,
  "is_final": false,
  "candidates_count": 12,
  "candidates_preview": [
    { "item_code": "201801234", "drug_name": "타이레놀정500mg", "confidence_estimate": 0.4 },
    { "item_code": "201805678", "drug_name": "다른약A", "confidence_estimate": 0.3 }
  ],
  "next_question": {
    "field": "shape",
    "text_to_speak": "알약의 모양은 어떤가요?",
    "options": [
      { "value": "원형", "label": "원형" },
      { "value": "타원형", "label": "타원형" },
      { "value": "캡슐형", "label": "캡슐형" },
      { "value": "기타", "label": "기타" }
    ]
  },
  "summary_so_far": [
    { "field": "color", "value": "흰색", "label_kr": "알약의 색상은? → 흰색" }
  ]
}
```

- **200 OK** — 후보가 충분히 좁혀진 경우 (`is_final: true`)
```json
{
  "step": 4,
  "total_steps_estimate": 4,
  "is_final": true,
  "candidates_count": 1,
  "final_candidates": [
    {
      "item_code": "201801234",
      "drug_name": "타이레놀정500mg",
      "confidence": 0.85,
      "match_keys": ["color", "shape", "has_engraving", "engraving_text"],
      "in_user_pool": true,
      "classification_name": "해열, 진통, 소염제",
      "efficacy_text": "(식약처 e약은요 본문 그대로 인용)",
      "usage_text": "(식약처 e약은요 본문 그대로 인용)"
    }
  ],
  "next_question": null,
  "summary_so_far": [
    { "field": "color", "value": "흰색", "label_kr": "알약의 색상은? → 흰색" },
    { "field": "shape", "value": "캡슐형", "label_kr": "알약의 모양은? → 캡슐형" },
    { "field": "has_engraving", "value": "yes", "label_kr": "각인이 있나요? → 있음" },
    { "field": "engraving_text", "value": "GS-7", "label_kr": "각인 텍스트 → GS-7" }
  ]
}
```

### 종료 조건
| 조건 | `is_final` | 동작 |
| --- | --- | --- |
| `candidates_count == 1` | true | `final_candidates` 단일 결과 반환 |
| `candidates_count <= 3` | true | Top-3 후보 반환 (사용자 선택 필요) |
| `attributes` 모두 채움 | true | 더 이상 물을 필드 없음, 현재 후보 반환 |
| 그 외 | false | `next_question` 으로 다음 질문 |

### 다음 질문 우선순위 (서버 결정)
1. `engraving_text` 가 비고 `has_engraving == "yes"` → 각인 텍스트 묻기 (가장 식별력 높음)
2. `has_engraving` 미정 → 각인 유무 묻기
3. `shape` 미정 → 모양 묻기
4. `color` 미정 → 색상 묻기

> 식별력 높은 순: 각인 > 모양 > 색상.

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_ATTRIBUTE_VALUE` | enum 값 외 입력 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |
| 422 | `NO_CANDIDATES_FOUND` | 모든 속성 채워도 매칭 0건 |

### Schemas/ 매핑
- `MainServer/Schemas/PillSchema.h::NarrowDownRequest`
- `MainServer/Schemas/PillSchema.h::NarrowDownResponse`
- `MainServer/Schemas/PillSchema.h::NarrowAttributes`
- `MainServer/Schemas/PillSchema.h::NarrowQuestion`

### 백엔드 처리 흐름
```
[클라 attributes 송신]
   ↓
[MainServer Pill::handle_identify_narrow]
   ↓
[Services/Pdma/PillIdentificationCache::narrow_search(attributes, use_pool, user_id)]
   ↓ SQL: SELECT * FROM pill_identification
       WHERE shape = ? AND color_front = ? AND ...
       AND (use_pool 시) item_code IN (사용자 약 풀)
   ↓
[candidates_count 판정 → 종료 또는 다음 질문 결정]
   ↓
[응답 — efficacy_text·usage_text 는 종료 시에만 e약은요 lazy 조회]
```

> ⚠ 본 흐름은 **LLM 미사용** (Rule-based DB 조회). 의료 안내 영역 미진입.

---

## 3. POST /v1/pill/onboarding/normalize ⭐ 신규 (v0.3) — 음성 등록 약명 정규화

사용자가 음성으로 약을 등록할 때, **의료용어를 모르는 상태의 발화**(예: "빨간 알약 진통제 등록해줘")에서 **식약처 의약품 정보 RAG** 를 활용해 약명·성분 후보를 추출·정규화한다. 동명·동성분 약이 여러 개일 때 **분기 질문 round-trip** 을 다회 반복해 사용자가 정확히 어떤 약인지 확정한 뒤, 그 결과(`item_code`)를 [§5 `POST /v1/pill/pool`](#4-post-v1pillpool--약-풀에-약-추가-v02-확장) 으로 풀에 등록.

### 설계 원칙

- **stateless** — 세션 유지 X. 클라가 누적 컨텍스트(이전 후보·선택 토큰)를 매번 송신.
- **다회 round-trip 허용** — 분기 질문이 여러 단계 필요할 수 있음 (예: 약명 → 용량 → 제형).
- **RAG 영역 제한 준수** — Onboarding RAG ✅, 의료 안내(DUR/진단) X ([아이템 v3 §6.3](../아이템_ver3.md), [요구사항 분석서 v2 §3.3](../요구사항_분석서_ver2.md)).
- **TTS 안내 책임은 클라** — 응답의 `question.tts_text` 를 받아 클라 자체 TTS (Qt6 `QTextToSpeech` + Windows SAPI 한국어) 로 합성. 환경 미지원·품질 부족 시 메인 서버 TTS 어댑터로 fallback.
- **남용 방어 (TODO 구현)** — 다회 round-trip 무한 반복 방지: `max_rounds` (기본 5회), `max_input_tokens` (기본 200), 1분당 호출 횟수 제한. 본 v0.3 명세에 한계만 선언, 구현 시 운영 가능 임계값 측정 후 확정.

### 요청

- 헤더: `Content-Type: application/json`, `Authorization: Bearer <JWT>` ✓
- 바디:
```json
{
  "utterance_request_id": "utt_123",
  "utterance_text": "타이레놀 등록할게",
  "round": 1,
  "prev_choice_token": null,
  "prev_selection": null
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| `utterance_request_id` | string | – | `POST /v1/speech/utterance` 의 request_id (가능한 경우 매핑) |
| `utterance_text` | string | ✓ (round=1 시) | 폰 STT 결과 또는 사용자 직접 입력 텍스트. 길이 제한 200자 |
| `round` | int | ✓ | 라운드 번호 (1부터). `max_rounds` 초과 시 422 반환 |
| `prev_choice_token` | string | – (round≥2 필수) | 직전 응답의 `question.choice_token`. 서버가 컨텍스트 복원에 사용 |
| `prev_selection` | object | – (round≥2 필수) | 사용자 응답: `{ "field": "dosage", "value": "500mg" }` 또는 `{ "field": "candidate_pick", "item_code": "201801234" }` |

### 응답

#### Case A: `state == "NEED_DISAMBIGUATION"` — 분기 질문 필요

- **200 OK**
```json
{
  "state": "NEED_DISAMBIGUATION",
  "round": 1,
  "max_rounds": 5,
  "candidates_count": 3,
  "candidates": [
    {
      "item_code": "201801234",
      "item_name": "타이레놀정500mg",
      "ingredient_name": "아세트아미노펜 500mg",
      "classification_name": "해열, 진통, 소염제",
      "manufacturer": "한국얀센",
      "hint": "가장 일반적인 형태"
    },
    {
      "item_code": "201805678",
      "item_name": "타이레놀이알서방정",
      "ingredient_name": "아세트아미노펜 650mg",
      "classification_name": "해열, 진통, 소염제",
      "manufacturer": "한국얀센",
      "hint": "8시간 지속형 (서방정)"
    },
    {
      "item_code": "201809999",
      "item_name": "어린이타이레놀정",
      "ingredient_name": "아세트아미노펜 80mg",
      "classification_name": "해열, 진통, 소염제",
      "manufacturer": "한국얀센",
      "hint": "어린이용 저용량"
    }
  ],
  "question": {
    "field": "dosage",
    "text": "타이레놀이 세 종류가 있어요. 일반 500mg 일까요, 8시간 지속형 650mg 일까요, 아니면 어린이용 80mg 일까요?",
    "tts_text": "타이레놀이 세 종류 있는데요. 일반 오백 밀리그램, 팔시간 지속형 육백오십 밀리그램, 어린이용 팔십 밀리그램, 어떤 거 드셨어요?",
    "options": [
      { "value": "201801234", "label": "일반 500mg" },
      { "value": "201805678", "label": "8시간 지속 650mg" },
      { "value": "201809999", "label": "어린이용 80mg" }
    ],
    "choice_token": "ck_e8f4a..."
  }
}
```

| 필드 | 설명 |
| --- | --- |
| `state` | `"NEED_DISAMBIGUATION"` — 분기 질문 필요. 클라는 `tts_text` 를 TTS 안내 후 사용자 응답 수신, 다음 라운드 호출 |
| `candidates[]` | RAG 검색으로 추출된 후보 (식약처 의약품 정보 기준). 단정 표현 X — 객관 정보만 |
| `candidates[].hint` | 사용자 식별 보조 한 줄 힌트 (LLM 생성, 식약처 정보 인용 형태) |
| `question.text` | 화면 표시용 |
| `question.tts_text` | TTS 합성용 (숫자·약어를 한글로 풀어쓴 형태) |
| `question.choice_token` | 다음 라운드 호출 시 `prev_choice_token` 으로 송신. 서버 컨텍스트 복원 |
| `max_rounds` | 본 흐름의 라운드 상한 (기본 5) |

#### Case B: `state == "RESOLVED"` — 단일 후보 확정

- **200 OK**
```json
{
  "state": "RESOLVED",
  "round": 2,
  "resolved": {
    "item_code": "201801234",
    "item_name": "타이레놀정500mg",
    "ingredient_name": "아세트아미노펜 500mg",
    "classification_name": "해열, 진통, 소염제",
    "manufacturer": "한국얀센",
    "efficacy_text": "(식약처 e약은요 본문 그대로 인용)",
    "usage_text": "(식약처 e약은요 본문 그대로 인용)"
  },
  "confirmation": {
    "text": "타이레놀정 500mg 으로 등록할게요. 맞으신가요?",
    "tts_text": "타이레놀정 오백 밀리그램으로 등록할게요. 맞으세요?"
  }
}
```

> 클라는 `confirmation.tts_text` 안내 후 사용자 "네" 응답을 받으면 [§5 `POST /v1/pill/pool`](#4-post-v1pillpool--약-풀에-약-추가-v02-확장) 으로 `item_code` 송신.

#### Case C: `state == "NOT_FOUND"` — 매칭 실패

- **200 OK**
```json
{
  "state": "NOT_FOUND",
  "round": 1,
  "reason": "발화에서 약명 후보를 추출하지 못했습니다.",
  "tts_text": "약 이름을 듣지 못했어요. 다시 말씀해 주시거나 직접 입력해 주세요.",
  "fallback_action": "RECAPTURE_OR_MANUAL"
}
```

| `fallback_action` | 의미 |
| --- | --- |
| `RECAPTURE_OR_MANUAL` | 음성 재발화 또는 직접 입력 권유 |
| `NARROW_DOWN` | 단계별 속성 좁히기 (`/v1/pill/identify/narrow`) 권유 |

### 종료 조건

| 조건 | `state` | 동작 |
| --- | --- | --- |
| 후보 1건 + 신뢰도 충분 | `RESOLVED` | `confirmation` 안내 후 사용자 확인 → `/v1/pill/pool` |
| 후보 2~5건 | `NEED_DISAMBIGUATION` | `question` 안내 후 다음 라운드 |
| 후보 0건 | `NOT_FOUND` | `fallback_action` 안내 |
| `round > max_rounds` | (422 에러) | 무한 반복 방어 — 클라가 etablish 한 후 처음부터 재시도 권유 |

### 에러

| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_UTTERANCE_TEXT` | 빈 문자열·길이 초과 (200자 제한) |
| 400 | `MISSING_PREV_CHOICE_TOKEN` | round≥2 인데 `prev_choice_token` 없음 |
| 400 | `INVALID_PREV_CHOICE_TOKEN` | 만료·서명 불일치 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |
| 422 | `MAX_ROUNDS_EXCEEDED` | round_max 초과 (남용 방어) |
| 429 | `RATE_LIMITED` | 1분당 호출 횟수 초과 (남용 방어) |

### Schemas/ 매핑

- `MainServer/Schemas/PillSchema.h::OnboardingNormalizeRequest`
- `MainServer/Schemas/PillSchema.h::OnboardingNormalizeResponse`
- `MainServer/Schemas/PillSchema.h::OnboardingCandidate`
- `MainServer/Schemas/PillSchema.h::OnboardingQuestion` (text + tts_text + options + choice_token)
- `MainServer/Schemas/PillSchema.h::OnboardingConfirmation` (text + tts_text)

### 백엔드 처리 흐름

```
[클라 PC] utterance_text 송신 (round=1)
   ↓
[메인서버] PillService::onboarding_normalize()
   ↓ JWT 검증 + max_rounds·max_input_tokens·rate_limit 체크
   ↓
[메인서버 → LLM PC 10.10.10.128] /v1/llm/onboarding/extract_and_search
   ↓ ① 약명 후보 추출 (NER on utterance_text)
   ↓ ② 식약처 의약품 정보 RAG 검색 (**ChromaDB 벡터 DB** over 낱알식별 + e약은요)
   ↓ ③ 후보 정렬 + hint 생성
   ↓
[LLM PC → 메인서버] candidates[]
   ↓
[메인서버] 후보 수에 따라 분기:
   - 1건 → RESOLVED (confirmation 생성, e약은요 인용)
   - 2~5건 → NEED_DISAMBIGUATION (question 생성, choice_token 발급)
   - 0건 → NOT_FOUND (fallback_action 결정)
   ↓
[메인서버 → 클라 PC] OnboardingNormalizeResponse
   ↓
[클라 PC] tts_text 를 클라 자체 TTS 로 합성·재생
   ↓ (어댑터 — Qt6 QTextToSpeech, fallback 시 메인서버 TTS API)
   ↓
[사용자 응답 수신 (음성 또는 터치)]
   ↓ (NEED_DISAMBIGUATION 인 경우 round=2 재호출)
   ↓ (RESOLVED + 사용자 OK 인 경우 POST /v1/pill/pool 호출로 종료)
```

### 표현 톤 정책 (필수 준수)

- ❌ "이 약을 드시면 됩니다" / "복용 가능합니다"
- ✅ "이 약 맞으세요?" / "확인해 주세요" / "약사·의사 상담을 권유드립니다"
- `efficacy_text` / `usage_text` 는 **식약처 e약은요 본문 그대로 인용** (LLM 자연어 변환 X)
- `hint` 는 객관 정보만 (예: "8시간 지속형", "어린이용") — "이 약이 가장 좋아요" 같은 평가 표현 X

---

## 4. GET /v1/pill/pool — 사용자 약 풀 조회 (v0.2 확장)

### 요청
- 헤더: `Authorization: Bearer <JWT>` ✓
- 쿼리: `include_inactive` (bool, 기본 `false`)

### 응답 (v0.2 확장)
- **200 OK**
```json
{
  "items": [
    {
      "pool_id": 12,
      "item_code": "201801234",
      "drug_name": "타이레놀정500mg",
      "user_category": "해열진통제",
      "classification_name": "해열, 진통, 소염제",
      "reg_method": "VOICE",
      "is_active": true,
      "created_at": "2026-04-15T09:00:00+09:00"
    }
  ],
  "total_count": 1
}
```

### v0.2 신규 필드 (PoolItem)

| 필드 | 출처 | 설명 |
| --- | --- | --- |
| `user_category` | `user_medication_pool.user_category` | 사용자가 등록 시 입력한 카테고리 (예: "해열진통제", "건강기능식품") |
| `classification_name` | `pill_identification.classification_name` | 식약처 약효분류명 (등록 시 자동 채움) |

> 목업 Slide 6 "종류: 해열진통제" 는 `user_category` 우선, 없으면 `classification_name` 표시.

### Schemas/ 매핑
- `MainServer/Schemas/PillSchema.h::PoolListResponse`
- `MainServer/Schemas/PillSchema.h::PoolItem` (v0.2: user_category·classification_name 추가)

---

## 5. POST /v1/pill/pool — 약 풀에 약 추가 (v0.2 확장)

### 요청
- 헤더: `Content-Type: application/json`, `Authorization: Bearer <JWT>` ✓
- 바디:
```json
{
  "item_code": "201801234",
  "reg_method": "VOICE",
  "user_category": "해열진통제"
}
```

| 필드 | 필수 | 설명 |
| --- | --- | --- |
| `item_code` | ✓ | 식약처 품목기준코드 |
| `reg_method` | ✓ | `VOICE` / `MANUAL` / `IMAGE` |
| `user_category` | – (v0.2 신규) | 사용자 카테고리 (미입력 시 서버가 `classification_name` 자동 매핑) |

### 응답
- **201 Created** — 추가된 PoolItem 반환 (v0.2: user_category·classification_name 포함)

### 에러 (변경 없음)
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 404 | `ITEM_CODE_NOT_FOUND` | 식약처 데이터에 없는 품목코드 |
| 409 | `ALREADY_IN_POOL` | 이미 등록된 약 (활성 상태) |

---

## 6. DELETE /v1/pill/pool/{pool_id} — 개별 삭제 (v0.1 그대로)

soft delete: `is_active = FALSE`, `deactivated_at = NOW()`. (v0.1 명세 그대로)

---

## 7. DELETE /v1/pill/pool/all — 전체 리셋 (v0.1 그대로)

`X-Confirm-Reset: true` 헤더 필수. 시스템이 임의로 호출 금지. (v0.1 명세 그대로)

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — 식별·DUR + 약 풀 CRUD, soft delete, 전체 리셋 안전 가드 |
| v0.2 | 2026-05-07 | 목업 분석 반영 — ① `PillCandidate` 에 `classification_name`·`efficacy_text`·`usage_text` 추가 (Slide 7) ② `PoolItem` 에 `user_category`·`classification_name` 추가 (Slide 6) ③ `guidance.fallback_action` enum 추가 (RECAPTURE/NARROW_DOWN/CHECK_ENGRAVING) ④ **신규 `POST /v1/pill/identify/narrow`** — 단계별 속성 좁히기 흐름 (Slide 3·9·12, stateless·LLM 미사용·Rule-based DB 조회) ⑤ DB ERD v3 매핑 |
| v0.3 | 2026-05-07 | **신규 `POST /v1/pill/onboarding/normalize`** — 음성 등록 시 의료용어를 모르는 사용자를 위한 약명 정규화 RAG round-trip. ① stateless 다회 round-trip 허용 (`max_rounds=5` 기본, 토큰·rate_limit 한계 선언) ② 응답에 `tts_text` 필드 (클라 자체 TTS 우선 + 메인서버 TTS fallback 어댑터) ③ `state` 3분기 (NEED_DISAMBIGUATION / RESOLVED / NOT_FOUND) ④ 표현 톤 정책 준수 (e약은요 본문 인용, 단정 표현 X) ⑤ Schemas/ 매핑 추가 (`OnboardingNormalizeRequest/Response`, `OnboardingCandidate/Question/Confirmation`) |
| **(v0.3 보강)** | **2026-05-13** | `/v1/pill/identify` 운영 모드 내부 흐름 명시 — MediaApi v0.2 의 `request_id` → photo_storage SELECT → GET 토큰 발급 → Vision PC `POST /vision/detect_remote` 호출 schema 확정. 신규 에러: `PHOTO_NOT_FOUND`(404) / `PHOTO_NOT_READY`(409) / `VISION_UPSTREAM_ERROR`(502). DUR 페어 매칭은 `DurQueryEngine` 모듈로 분리 (양방향 페어 + dedup, `prohibit_reason` 그대로 인용, action_message 정해진 템플릿). |
