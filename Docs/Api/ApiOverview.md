# 메디브릿지 API 명세서 — 전체 개요

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | API 명세서 인덱스 + 공통 규칙 |
| **버전** | v0.1 |
| **작성일** | 2026-05-06 |
| **작성자** | 팀 (3인) |

> 본 폴더(`Docs/Api/`)는 메디브릿지 클라이언트 ↔ 메인 서버 ↔ 추론 서버 간의 **REST API 변경 불가 계약**을 정의한다. 영역별 분담 작업의 1차 인터페이스 합의 문서.

---

## 1. API 명세서 목록

| 파일 | 모듈 | 엔드포인트 |
| --- | --- | --- |
| [AuthApi.md](AuthApi.md) | 모듈 6 (인증) | `POST /v1/auth/signup`, `/v1/auth/login`, `/v1/auth/logout` |
| [HistoryApi.md](HistoryApi.md) | 모듈 2 (복약 이력) | `POST /v1/history/record`, `GET /v1/history/list` |
| [ReportApi.md](ReportApi.md) | 모듈 5 (통합 보고서) | `GET /v1/report/generate` |
| [MediaApi.md](MediaApi.md) | 미디어 송수신 | `POST /v1/media/image` |
| [SpeechApi.md](SpeechApi.md) | 음성 텍스트 (폰 STT) | `POST /v1/speech/utterance` |
| [PillApi.md](PillApi.md) | 모듈 1 (식별·DUR + 약 풀) | `POST /v1/pill/identify`, `/v1/pill/pool/*` |
| [MonitoringApi.md](MonitoringApi.md) | 자원 모니터링 (전 영역) | `GET /health`, `GET /metrics` |

---

## 2. 호스트·포트·버전

| 서버 | OS | 호스트 | 기본 포트 | 베이스 URL |
| --- | --- | --- | --- | --- |
| Client (PhoneAdapter) | Windows 10/11 | `localhost` (폰 → adb reverse) | 8000 | `http://localhost:8000` |
| MainServer | Ubuntu 24.04 | LAN 고정 IP | 8001 | `http://<MAIN_IP>:8001/v1` |
| InferenceServer | Ubuntu 24.04 (GPU) | LAN 고정 IP | 8002 | `http://<INF_IP>:8002/v1` |

> **버전 prefix `/v1/`**: 향후 호환성 깨지는 변경 시 `/v2/` 로 분기. MVP는 `/v1/` 고정.

---

## 3. 공통 헤더

### 3.1 요청 헤더

| 헤더 | 필수 | 설명 |
| --- | --- | --- |
| `Content-Type` | ✓ | `application/json` (기본) / `multipart/form-data` (파일 업로드) |
| `Authorization` | 인증 필요 시 ✓ | `Bearer <JWT>` — 로그인 후 발급된 토큰 |
| `X-Request-ID` | 권장 | 클라가 발급하는 요청 추적 ID (디버깅용 UUID 등) |

### 3.2 응답 헤더

| 헤더 | 설명 |
| --- | --- |
| `Content-Type` | `application/json` (기본) / `application/pdf` (보고서) |
| `X-Request-ID` | 요청 헤더에 있던 값을 그대로 echo (없으면 서버가 생성) |

---

## 4. 표준 응답 형식

### 4.1 성공 응답

엔드포인트별 데이터를 그대로 JSON으로 반환. 별도 envelope 사용하지 않음.

```json
{
  "user_id": "u_abc123",
  "created_at": "2026-05-06T10:30:00Z"
}
```

### 4.2 에러 응답 (공통 envelope)

모든 4xx·5xx 응답은 다음 형식으로 통일:

```json
{
  "error": {
    "code": "INVALID_EMAIL",
    "message": "이메일 형식이 올바르지 않습니다",
    "details": {
      "field": "email",
      "value": "invalid@@example"
    }
  }
}
```

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| `error.code` | string (UPPER_SNAKE_CASE) | 에러 식별자 (클라가 분기 판단) |
| `error.message` | string | 한국어 사용자 안내 메시지 |
| `error.details` | object (선택) | 디버깅용 추가 정보 |

---

## 5. 표준 상태 코드

| 코드 | 의미 | 사용 |
| --- | --- | --- |
| **200 OK** | 성공 (조회·삭제) | GET, DELETE 성공 |
| **201 Created** | 생성 성공 | POST 회원가입·기록 생성 |
| **204 No Content** | 성공 + 본문 없음 | 로그아웃 등 |
| **400 Bad Request** | 요청 형식·필드 오류 | 필수 필드 누락, 타입 오류 |
| **401 Unauthorized** | 인증 안 됨 | JWT 누락·만료·서명 실패 |
| **403 Forbidden** | 권한 없음 | 본인 데이터 아님 |
| **404 Not Found** | 리소스 없음 | 존재하지 않는 ID 조회 |
| **409 Conflict** | 충돌 | 이메일 중복 등 |
| **422 Unprocessable Entity** | 비즈니스 규칙 위반 | 미등록 약 식별 시도 등 |
| **500 Internal Server Error** | 서버 내부 오류 | 예상 못한 예외 |
| **503 Service Unavailable** | 서비스 일시 불가 | 추론 서버 연결 끊김 등 |

---

## 6. 인증 정책 (JWT)

### 6.1 토큰 발급
- `POST /v1/auth/login` 성공 시 응답에 `access_token` 포함
- 토큰 형식: JWT (HS256 또는 RS256, MainServer Config로 결정)
- 만료: 기본 24시간 (Config로 조정 가능)

### 6.2 토큰 사용
모든 보호 엔드포인트(인증 필요 ✓)는 다음 헤더 필수:
```
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6Ikp...
```

### 6.3 토큰 무효화
- `POST /v1/auth/logout` 호출 시 서버가 토큰을 블랙리스트에 등록 (또는 짧은 만료)
- 만료 시 401 반환 → 클라는 재로그인 유도

---

## 7. 페이지네이션 (목록 조회)

`GET /v1/history/list` 같은 목록 API는 다음 쿼리 파라미터 지원:

| 파라미터 | 타입 | 기본값 | 설명 |
| --- | --- | --- | --- |
| `page` | int | 1 | 페이지 번호 (1부터) |
| `page_size` | int | 20 | 페이지당 항목 수 (최대 100) |
| `sort_by` | string | `intake_datetime` | 정렬 기준 필드 |
| `sort_order` | string | `desc` | `asc` / `desc` |

응답 형식:
```json
{
  "items": [ /* 항목 배열 */ ],
  "page": 1,
  "page_size": 20,
  "total_count": 145,
  "total_pages": 8
}
```

---

## 8. 표현 톤 정책 (필수 준수)

[요구사항 분석서 v2 §3.3](../요구사항_분석서_ver2.md) 의 표현 톤 정책을 모든 API 응답 메시지(`error.message`, 식별 결과 메시지 등)에 적용:

- ❌ "복용 가능합니다" / "복용 불가합니다" 등 단정 표현 금지
- ✅ "추정" + "약사·의사 상담 권유" 톤
- ✅ DUR 위험 안내는 식약처 데이터 그대로 인용 + LLM 자연어 변환 금지

---

## 9. Schemas/ 매핑 규칙

각 API 명세는 **MainServer/Schemas/ 의 C++ struct와 1:1 대응**:

| API URL | Schemas/ 파일 | 주요 struct |
| --- | --- | --- |
| `/v1/auth/*` | `AuthSchema.h/.cpp` | `SignupRequest/Response`, `LoginRequest/Response` 등 |
| `/v1/history/*` | `HistorySchema.h/.cpp` | `HistoryRecordRequest/Response`, `HistoryListResponse` |
| `/v1/report/*` | `ReportSchema.h/.cpp` | `ReportRequest/Response` |
| `/v1/media/*` | `MediaSchema.h/.cpp` | `ImageUploadResponse` |
| `/v1/speech/*` | `SpeechSchema.h/.cpp` | `UtteranceRequest/Response` |
| `/v1/pill/*` | `PillSchema.h/.cpp` | `IdentifyRequest/Response`, `PillPoolItem` 등 |
| `/health`, `/metrics` | `HealthSchema.h/.cpp` | `HealthResponse`, `MetricsResponse` |

→ 명세 변경 시 Schemas/ 도 같이 갱신. PR 시 함께 리뷰.

---

## 10. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v0.1 | 2026-05-06 | 팀 (3인) | 초안 작성 — 호스트·포트, 공통 헤더, 표준 응답·에러, 상태 코드, JWT 인증, 페이지네이션, 표현 톤, Schemas 매핑 규칙 정리 |
