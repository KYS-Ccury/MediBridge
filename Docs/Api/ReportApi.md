# Report API — 통합 보고서 (모듈 5)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.1 |
| **작성일** | 2026-05-06 |
| **모듈** | 모듈 5 (통합 보고서) |
| **관련 문서** | [프로토콜 v2 §2.2](../프로토콜_ver2.md), [요구사항 분석서 v2 §5.11 FR-B9 / §6.10 FR-C8](../요구사항_분석서_ver2.md), [DB ERD v2 — user_reports](../DB_ERD_ver2.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> 지정 기간의 복약 이력을 표 형태로 출력하고, 약별 부작용 주요 증상은 식약처 e약은요 데이터에서 인용한 형태로 표시. **LLM 자연어 생성 미사용**. 인쇄·PDF 출력으로 의사·약사 제출.

---

## 엔드포인트 목록

| HTTP | URL | 설명 | 인증 |
| --- | --- | --- | --- |
| GET | `/v1/report/generate` | 보고서 생성 (HTML/PDF) | ✓ |

---

## 1. GET /v1/report/generate — 보고서 생성

### 요청
- 헤더:
  - `Authorization: Bearer <JWT>` ✓
  - `Accept: application/pdf` (PDF 다운로드) 또는 `application/json` (JSON 응답)
- 쿼리 파라미터:

| 파라미터 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| `from_date` | string (YYYY-MM-DD) | ✓ | 보고서 시작일 (포함) |
| `to_date` | string (YYYY-MM-DD) | ✓ | 보고서 종료일 (포함) |
| `format` | string | – | `pdf` (기본) / `html` / `json` |

예: `GET /v1/report/generate?from_date=2026-04-01&to_date=2026-05-06&format=pdf`

### 응답 — `format=json`
- **200 OK**
```json
{
  "report_id": "r_def456",
  "user": {
    "user_id": "u_abc123",
    "user_name": "홍길동"
  },
  "period": {
    "from_date": "2026-04-01",
    "to_date": "2026-05-06"
  },
  "consumed_summary": {
    "total_intakes": 145,
    "by_drug": [
      {
        "item_code": "201801234",
        "drug_name": "타이레놀정500mg",
        "total_quantity": 30,
        "first_intake": "2026-04-01T08:00:00+09:00",
        "last_intake": "2026-05-06T08:30:00+09:00"
      }
    ]
  },
  "side_effect_quotes": [
    {
      "item_code": "201801234",
      "drug_name": "타이레놀정500mg",
      "caution_text": "(식약처 e약은요 주의사항 본문 그대로 인용)",
      "side_effect_text": "(식약처 e약은요 부작용 본문 그대로 인용)",
      "source_note": "본 정보는 식약처 e약은요 데이터에서 인용한 것이며, 단정적 안내가 아닙니다. 약사·의사 상담을 권유드립니다."
    }
  ],
  "intake_logs": [
    {
      "intake_datetime": "2026-05-06T08:30:00+09:00",
      "drug_name": "타이레놀정500mg",
      "quantity": 1,
      "memo": "아침 식후 30분"
    }
  ],
  "generated_at": "2026-05-06T15:00:00+09:00"
}
```

| 필드 | 설명 |
| --- | --- |
| `consumed_summary` | 약별 통계 (개수·기간) |
| `side_effect_quotes` | 약별 부작용 주의사항 — **식약처 데이터 그대로 인용**, LLM 변환 없음 |
| `source_note` | 모든 인용에 첨부되는 비단정·상담 권유 안내 |
| `intake_logs` | 기간 내 복용 이력 표 |

### 응답 — `format=pdf` 또는 `Accept: application/pdf`
- **200 OK**
- 헤더: `Content-Type: application/pdf`, `Content-Disposition: attachment; filename="medibridge_report_<from>_<to>.pdf"`
- 본문: PDF 바이너리

### 응답 — `format=html`
- **200 OK**
- 헤더: `Content-Type: text/html; charset=utf-8`
- 본문: 인쇄 친화적 HTML (브라우저에서 인쇄·PDF 저장 가능)

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_DATE_RANGE` | `from_date > to_date` 또는 형식 오류 |
| 400 | `INVALID_FORMAT` | `format` 값이 `pdf`/`html`/`json` 외 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |
| 422 | `EMPTY_PERIOD` | 해당 기간 복약 이력 없음 |

### Schemas/ 매핑
- `MainServer/Schemas/ReportSchema.h::ReportRequest`
- `MainServer/Schemas/ReportSchema.h::ReportResponse`
- `MainServer/Schemas/ReportSchema.h::ConsumedSummary`
- `MainServer/Schemas/ReportSchema.h::SideEffectQuote`
- `MainServer/Schemas/ReportSchema.h::IntakeLog`

---

## 표현 톤 정책 적용

| 위반 사례 | 권장 |
| --- | --- |
| ❌ "이 약은 부작용이 거의 없습니다" | ✅ "식약처 e약은요에 따르면: <원문 인용> — 약사·의사 상담을 권유드립니다" |
| ❌ "안전하게 복용 가능합니다" | ✅ "식약처 DUR에 등록 확인되지 않았습니다. 안심을 위해 약사·의사 상담을 권유드립니다." |

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — JSON/PDF/HTML 3가지 응답 형식, 식약처 데이터 인용 정책, 표현 톤 적용 |
