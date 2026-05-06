# History API — 복약 이력 (모듈 2)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.1 |
| **작성일** | 2026-05-06 |
| **모듈** | 모듈 2 (복약 이력 관리) |
| **관련 문서** | [프로토콜 v2 §2.2](../프로토콜_ver2.md), [요구사항 분석서 v2 §5.11 FR-B9 / §6.10 FR-C8](../요구사항_분석서_ver2.md), [DB ERD v2 — medication_intake_logs](../DB_ERD_ver2.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> 사용자별 복용 시각·약·개수·메모를 영속 저장하고, 일자·약·기간별로 조회. 시각은 분 단위까지. 시각 미입력 시 시스템 시각 자동.

---

## 엔드포인트 목록

| HTTP | URL | 설명 | 인증 |
| --- | --- | --- | --- |
| POST | `/v1/history/record` | 복약 이력 1건 기록 | ✓ |
| GET  | `/v1/history/list` | 복약 이력 조회 (필터·페이지네이션) | ✓ |

---

## 1. POST /v1/history/record — 복약 이력 기록

### 요청
- 헤더:
  - `Content-Type: application/json`
  - `Authorization: Bearer <JWT>` ✓
- 바디:
```json
{
  "item_code": "201801234",
  "intake_datetime": "2026-05-06T08:30:00+09:00",
  "quantity": 1,
  "memo": "아침 식후 30분",
  "confidence_score": 0.94,
  "dur_snapshot": {
    "checked_at": "2026-05-06T08:29:50+09:00",
    "result": "no_risk_found",
    "details": []
  }
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| `item_code` | string | ✓ | 식약처 품목기준코드 |
| `intake_datetime` | string (ISO 8601) | – | 복용 시각. 미입력 시 서버가 현재 시각으로 자동 |
| `quantity` | int | ✓ | 복용 개수 (1 이상) |
| `memo` | string | – | 사용자 메모 (모듈 3 「증상」과 무관, 단순 메모) |
| `confidence_score` | float | – | 식별 신뢰도 스냅샷 (FR-B9-02) |
| `dur_snapshot` | object | – | DUR 위험 검출 결과 스냅샷 (FR-B9-02) |

### 응답
- **201 Created**
```json
{
  "intake_id": "ih_xyz789",
  "user_id": "u_abc123",
  "item_code": "201801234",
  "intake_datetime": "2026-05-06T08:30:00+09:00",
  "quantity": 1,
  "memo": "아침 식후 30분",
  "created_at": "2026-05-06T08:30:05+09:00"
}
```

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_QUANTITY` | quantity가 1 미만 |
| 400 | `INVALID_DATETIME` | ISO 8601 형식 오류 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |
| 404 | `ITEM_CODE_NOT_FOUND` | 등록되지 않은 품목코드 |

### Schemas/ 매핑
- `MainServer/Schemas/HistorySchema.h::HistoryRecordRequest`
- `MainServer/Schemas/HistorySchema.h::HistoryRecordResponse`
- `MainServer/Schemas/HistorySchema.h::DurSnapshot`

---

## 2. GET /v1/history/list — 복약 이력 조회

### 요청
- 헤더:
  - `Authorization: Bearer <JWT>` ✓
- 쿼리 파라미터:

| 파라미터 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| `from_date` | string (YYYY-MM-DD) | – | 조회 시작일 (포함) |
| `to_date` | string (YYYY-MM-DD) | – | 조회 종료일 (포함) |
| `item_code` | string | – | 특정 약만 필터 |
| `page` | int | – | 기본 1 |
| `page_size` | int | – | 기본 20, 최대 100 |
| `sort_order` | string | – | `asc` / `desc` (기본 `desc`) |

예: `GET /v1/history/list?from_date=2026-05-01&to_date=2026-05-06&page=1&page_size=20`

### 응답
- **200 OK**
```json
{
  "items": [
    {
      "intake_id": "ih_xyz789",
      "item_code": "201801234",
      "drug_name": "타이레놀정500mg",
      "intake_datetime": "2026-05-06T08:30:00+09:00",
      "quantity": 1,
      "memo": "아침 식후 30분",
      "time_slot": "아침"
    }
  ],
  "page": 1,
  "page_size": 20,
  "total_count": 145,
  "total_pages": 8
}
```

| 필드 | 설명 |
| --- | --- |
| `time_slot` | 서버가 분 단위 시각 → 아침/점심/저녁/취침으로 자동 분류 |

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_DATE_RANGE` | `from_date > to_date` 또는 형식 오류 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |

### Schemas/ 매핑
- `MainServer/Schemas/HistorySchema.h::HistoryListQuery`
- `MainServer/Schemas/HistorySchema.h::HistoryItem`
- `MainServer/Schemas/HistorySchema.h::HistoryListResponse`

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — 기록·조회 정의, time_slot 자동 분류, dur_snapshot 스냅샷 컬럼 매핑 |
