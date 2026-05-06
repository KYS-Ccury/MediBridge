# Pill API — 알약 식별·DUR + 약 풀 관리 (모듈 1)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.1 |
| **작성일** | 2026-05-06 |
| **모듈** | 모듈 1 (알약 식별 + 안전 점검) |
| **관련 문서** | [프로토콜 v2 §3.1](../프로토콜_ver2.md), [요구사항 분석서 v2 §5.2 FR-B2 / §5.4 FR-B4 / §6.5 FR-C5](../요구사항_분석서_ver2.md), [DB ERD v2 — user_medication_pool, dur_interaction_cache](../DB_ERD_ver2.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> 식별·DUR 위험 검출 결과 반환 + 사용자별 약 풀(추가/삭제/전체 리셋) 관리. **DUR 위험 안내는 LLM·RAG 미사용**, 식약처 데이터 정해진 템플릿으로만 출력. **단정 문구 금지**.

---

## 엔드포인트 목록

| HTTP | URL | 설명 | 인증 |
| --- | --- | --- | --- |
| POST | `/v1/pill/identify` | 이미지 + 약 풀로 식별 + DUR 위험 검출 | ✓ |
| GET | `/v1/pill/pool` | 사용자 약 풀 조회 | ✓ |
| POST | `/v1/pill/pool` | 약 풀에 약 추가 (등록) | ✓ |
| DELETE | `/v1/pill/pool/{pool_id}` | 약 1건 개별 삭제 (soft delete) | ✓ |
| DELETE | `/v1/pill/pool/all` | 전체 리셋 (사용자 명시 트리거만) | ✓ |

---

## 1. POST /v1/pill/identify — 식별 + DUR 위험 검출

### 요청
- 헤더:
  - `Content-Type: application/json`
  - `Authorization: Bearer <JWT>` ✓
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
| `utterance_request_id` | – | 결합할 발화 request_id (Stage 0.5 의도 컨텍스트) |
| `include_dur_check` | – | 기본 `true`. `false` 시 식별 결과만 반환 |

### 응답
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
      "in_user_pool": true
    }
  ],
  "confidence_tier": "HIGH",
  "guidance": {
    "tts_text": "타이레놀정으로 추정됩니다. 화면을 확인해주세요.",
    "active_guide": null,
    "interactive_guide": null
  },
  "dur_check": {
    "result": "no_risk_found",
    "checked_at": "2026-05-06T10:30:05+09:00",
    "details": []
  }
}
```

### `dur_check.result` 값

| 값 | 의미 | 응답 톤 |
| --- | --- | --- |
| `no_risk_found` | DUR에 등록 확인되지 않음 | "DUR에 등록 확인되지 않았습니다. 안심을 위해 약사·의사 상담을 권유드립니다." |
| `risk_found` | 병용금기 등 위험 검출 | `details` 배열에 식약처 원문 인용 |

위험 케이스 응답 예:
```json
"dur_check": {
  "result": "risk_found",
  "checked_at": "2026-05-06T10:30:05+09:00",
  "details": [
    {
      "dur_type": "병용금기",
      "drug_a": { "item_code": "201801234", "drug_name": "약A" },
      "drug_b": { "item_code": "201805678", "drug_name": "약B" },
      "prohibit_reason": "(식약처 DUR 데이터 본문 그대로 인용)",
      "action_message": "즉시 약사·의사 상담이 필요합니다."
    }
  ]
}
```

> ⚠️ **모든 메시지는 정해진 템플릿** ([요구사항 분석서 v2 §3.3 / §9.3](../요구사항_분석서_ver2.md)). LLM 자연어 변환·단정 표현 금지.

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_REQUEST_ID` | image_request_id 형식 오류 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |
| 404 | `IMAGE_NOT_FOUND` | request_id에 해당하는 이미지 없음 (만료 등) |
| 422 | `NO_PILL_DETECTED` | YOLO26 검출 실패. guidance에 능동 가이드 텍스트 |
| 422 | `NO_PILL_IN_POOL` | 사용자 약 풀이 비어있음. 사전 등록 안내 |
| 503 | `INFERENCE_SERVER_UNAVAILABLE` | 추론 서버 연결 끊김 |

### Schemas/ 매핑
- `MainServer/Schemas/PillSchema.h::IdentifyRequest`
- `MainServer/Schemas/PillSchema.h::IdentifyResponse`
- `MainServer/Schemas/PillSchema.h::PillCandidate`
- `MainServer/Schemas/PillSchema.h::DurCheckResult`
- `MainServer/Schemas/PillSchema.h::DurDetail`

---

## 2. GET /v1/pill/pool — 사용자 약 풀 조회

### 요청
- 헤더: `Authorization: Bearer <JWT>` ✓
- 쿼리: `include_inactive` (bool, 기본 `false` — soft delete된 항목 포함 여부)

### 응답
- **200 OK**
```json
{
  "items": [
    {
      "pool_id": 12,
      "item_code": "201801234",
      "drug_name": "타이레놀정500mg",
      "reg_method": "VOICE",
      "is_active": true,
      "created_at": "2026-04-15T09:00:00+09:00"
    }
  ],
  "total_count": 1
}
```

### Schemas/ 매핑
- `MainServer/Schemas/PillSchema.h::PoolListResponse`
- `MainServer/Schemas/PillSchema.h::PoolItem`

---

## 3. POST /v1/pill/pool — 약 풀에 약 추가

### 요청
- 헤더: `Content-Type: application/json`, `Authorization: Bearer <JWT>` ✓
- 바디:
```json
{
  "item_code": "201801234",
  "reg_method": "VOICE"
}
```

| 필드 | 필수 | 설명 |
| --- | --- | --- |
| `item_code` | ✓ | 식약처 품목기준코드 |
| `reg_method` | ✓ | `VOICE` / `MANUAL` / `IMAGE` (확장) |

### 응답
- **201 Created** — 추가된 PoolItem 반환

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 404 | `ITEM_CODE_NOT_FOUND` | 식약처 데이터에 없는 품목코드 |
| 409 | `ALREADY_IN_POOL` | 이미 등록된 약 (활성 상태) |

---

## 4. DELETE /v1/pill/pool/{pool_id} — 개별 삭제 (soft delete)

### 요청
- 경로: `/v1/pill/pool/12`
- 헤더: `Authorization: Bearer <JWT>` ✓

### 응답
- **204 No Content**

### 동작
- DB: `is_active = FALSE`, `deactivated_at = NOW()` (등록 이력 보존)

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 404 | `POOL_ITEM_NOT_FOUND` | pool_id 없음 또는 본인 데이터 아님 |
| 403 | `FORBIDDEN` | 다른 사용자의 pool_id |

---

## 5. DELETE /v1/pill/pool/all — 전체 리셋

### 요청
- 헤더:
  - `Authorization: Bearer <JWT>` ✓
  - `X-Confirm-Reset: true` ⭐ (안전 가드 — 명시적 확인 헤더)
- 바디: 없음

### 응답
- **204 No Content**

### 동작
- DB: 모든 활성 항목 `is_active = FALSE` 일괄 처리

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `MISSING_CONFIRM_HEADER` | `X-Confirm-Reset` 헤더 누락 |
| 401 | `INVALID_TOKEN` | 인증 실패 |

> ⚠️ **시스템이 임의로 호출 금지** ([요구사항 분석서 v2 FR-B2-03](../요구사항_분석서_ver2.md)). 사용자가 UI에서 명시적 "전체 리셋" 버튼 + 확인 다이얼로그 후에만.

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — 식별·DUR 검출, 약 풀 CRUD, soft delete, 전체 리셋 안전 가드, 표현 톤 정책 적용 |
