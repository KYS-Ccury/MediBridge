# Pill API — 알약 식별·DUR + 약 풀 관리 + 단계별 좁히기 (모듈 1)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.2 |
| **개정일** | 2026-05-07 |
| **이전 버전** | v0.1 (2026-05-06) → `Docs/Old/Api/PillApi_v0.1_2026-05-07.md` |
| **모듈** | 모듈 1 (알약 식별 + 안전 점검) |
| **관련 문서** | [프로토콜 v2 §3.1](../프로토콜_ver2.md), [요구사항 분석서 v2 §5.2 / §5.4 / §6.5](../요구사항_분석서_ver2.md), [DB ERD v3](../DB_ERD_ver3.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> 식별·DUR 위험 검출 결과 반환 + 사용자별 약 풀(추가/삭제/전체 리셋) 관리.
> **DUR 위험 안내는 LLM·RAG 미사용**, 식약처 데이터 정해진 템플릿. **단정 문구 금지**.
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
| POST | `/v1/pill/identify/narrow` ⭐ | **단계별 속성 좁히기** (음성·이미지로 식별 어려울 때 fallback) | ✓ | v0.2 (신규) |
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
| `image_request_id` | ✓ | 직전 `POST /v1/media/image` 의 request_id |
| `utterance_request_id` | – | 결합할 발화 request_id |
| `include_dur_check` | – | 기본 `true`. `false` 시 식별 결과만 |

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
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |
| 404 | `IMAGE_NOT_FOUND` | request_id 에 해당하는 이미지 없음 |
| 422 | `NO_PILL_DETECTED` | YOLO26 검출 실패 → `guidance.fallback_action: "RECAPTURE"` |
| 422 | `NO_PILL_IN_POOL` | 사용자 약 풀이 비어있음 |
| 503 | `INFERENCE_SERVER_UNAVAILABLE` | 추론 서버 연결 끊김 |

### Schemas/ 매핑
- `MainServer/Schemas/PillSchema.h::IdentifyRequest/Response`
- `MainServer/Schemas/PillSchema.h::PillCandidate` (v0.2: efficacy_text·usage_text·classification_name 필드 추가)
- `MainServer/Schemas/PillSchema.h::DurCheckResult/DurDetail`

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

## 3. GET /v1/pill/pool — 사용자 약 풀 조회 (v0.2 확장)

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

## 4. POST /v1/pill/pool — 약 풀에 약 추가 (v0.2 확장)

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

## 5. DELETE /v1/pill/pool/{pool_id} — 개별 삭제 (v0.1 그대로)

soft delete: `is_active = FALSE`, `deactivated_at = NOW()`. (v0.1 명세 그대로)

---

## 6. DELETE /v1/pill/pool/all — 전체 리셋 (v0.1 그대로)

`X-Confirm-Reset: true` 헤더 필수. 시스템이 임의로 호출 금지. (v0.1 명세 그대로)

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — 식별·DUR + 약 풀 CRUD, soft delete, 전체 리셋 안전 가드 |
| v0.2 | 2026-05-07 | 목업 분석 반영 — ① `PillCandidate` 에 `classification_name`·`efficacy_text`·`usage_text` 추가 (Slide 7) ② `PoolItem` 에 `user_category`·`classification_name` 추가 (Slide 6) ③ `guidance.fallback_action` enum 추가 (RECAPTURE/NARROW_DOWN/CHECK_ENGRAVING) ④ **신규 `POST /v1/pill/identify/narrow`** — 단계별 속성 좁히기 흐름 (Slide 3·9·12, stateless·LLM 미사용·Rule-based DB 조회) ⑤ DB ERD v3 매핑 |
