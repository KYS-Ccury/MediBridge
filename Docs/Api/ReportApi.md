# Report API — 통합 보고서 (모듈 5)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.2 |
| **개정일** | 2026-05-13 |
| **이전 버전** | v0.1 (2026-05-06) |
| **모듈** | 모듈 5 (통합 보고서) |
| **관련 문서** | [프로토콜 v2.2 §2.2](../프로토콜_ver2.md), [요구사항 분석서 v2 §5.11 FR-B9 / §6.10 FR-C8](../요구사항_분석서_ver2.md), [DB ERD v4.1 — user_reports](../DB_ERD_ver4.md) |
| **공통 규칙** | [ApiOverview v0.4](ApiOverview.md) |

> 지정 기간의 복약 이력을 표 형태로 출력하고, 약별 부작용 주요 증상은 식약처 e약은요 데이터에서 인용한 형태로 표시. **LLM 자연어 생성 미사용**. 인쇄·PDF 출력으로 의사·약사 제출.
>
> ⭐ **v0.2 변경 핵심** (구현 완료):
> - `format=pdf` 실제 동작 — `wkhtmltopdf` subprocess (A4, 한글 폰트 자동, fork+execvp 보안)
> - `format=html` 인쇄 친화 CSS (@page A4, @media print, 컬러 팔레트, XSS escape)
> - WorkerPool 위임 (Drogon 핸들러 스레드 차단 방지)

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
- 헤더: `Content-Type: application/pdf`, `Content-Disposition: inline; filename="medibridge_<report_id>.pdf"`
- 본문: PDF 바이너리 (A4, 한국어 폰트 자동, Qt 5.15 producer)
- 구현: `wkhtmltopdf` subprocess (fork+execvp), WorkerPool 위임. 평균 0.5~1.5초.
- 의존성: `apt install wkhtmltopdf` (Ubuntu 24.04)

### 응답 — `format=html`
- **200 OK**
- 헤더: `Content-Type: text/html; charset=utf-8`
- 본문: 인쇄 친화적 HTML (단일 파일, 외부 자원 의존 0, `@page A4` + `@media print` 인쇄 CSS)
- XSS escape 적용 (drug_name, memo, 식약처 본문 모두)
- 약 6KB 내외 (인용 4건 기준)

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_DATE_RANGE` | `from_date > to_date` 또는 형식 오류 |
| 400 | `INVALID_FORMAT` | `format` 값이 `pdf`/`html`/`json` 외 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 |
| 422 | `EMPTY_PERIOD` | 해당 기간 복약 이력 없음 |
| 500 | `PDF_RENDER_FAILED` | wkhtmltopdf 미설치 또는 변환 실패 (format=pdf 시) |

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
| **v0.2** | **2026-05-13** | **PDF/HTML 실제 구현** — `format=pdf` 는 wkhtmltopdf subprocess (fork+execvp, mkstemps race-free, RAII 임시파일 정리, WorkerPool 위임). HTML 은 단일 파일 (외부 자원 0, @page A4 + @media print). XSS escape 적용. 500 `PDF_RENDER_FAILED` 에러 코드 추가. 서비스 분리: `ReportHtmlRenderer` + `ReportPdfRenderer`. |
