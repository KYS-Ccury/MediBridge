# 메디브릿지 API 명세서 — 전체 개요

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | API 명세서 인덱스 + 공통 규칙 |
| **버전** | v0.4 |
| **개정일** | 2026-05-13 |
| **이전 버전** | v0.3 (2026-05-07) / v0.2 (2026-05-07) / v0.1 (2026-05-06) → `Docs/Old/Api/ApiOverview_v0.1_2026-05-07.md` |

> 본 폴더(`Docs/Api/`)는 메디브릿지 클라이언트 ↔ 메인 서버 ↔ 추론 서버 간의 **REST API 변경 불가 계약**을 정의한다.
>
> ⭐ **v0.4 변경 핵심** (사진 흐름 ⑤+⑥ 구현 완료 — 호환 변경):
> - **MediaApi v0.2** — `POST /v1/media/intent`, `POST /v1/media/commit`, `POST /v1/media/get_token` **신규 3개**. 기존 `POST /v1/media/image` 는 레거시/TestMode fallback 으로 유지
> - **사진 본체는 메인서버 통과 X** — 메인은 HMAC put_token 만 발급, 클라가 보관 PC (port 8004) 에 직접 PUT
> - **ReportApi v0.2** — `format=pdf` 실제 동작 (wkhtmltopdf, A4)
> - **DataStoragePC 포트 확정 8004** (Drogon C++ 미니 서버)
> - HMAC 토큰 시크릿 분리 — `MEDIBRIDGE_JWT_SECRET` ≠ `MEDIBRIDGE_STORAGE_SECRET`
>
> ⭐ **v0.3 변경 핵심** (시스템_연결구조 v2.1 반영):
> - **§2 호스트 표 IP 확정** — 메인 10.10.10.97 / 데이터 보관 10.10.10.122 / **LLM 추론 10.10.10.120 / Vision 추론 10.10.10.128** (추론 서버 카테고리별 2대 분리)
> - **§2 베이스 URL 분기** — `INFERENCE_LLM_BASE` / `INFERENCE_VISION_BASE` 명시 (학습+추론 동거, 네트워크 분리 가능 설계)
> - 음성 텍스트 추론은 LLM 서버, 이미지 식별 추론은 Vision 서버 — 라우팅 책임은 메인 서버
>
> ⭐ **v0.2 변경 핵심** (목업 「메디브릿지 목업.pptx」 반영):
> - **PillApi 확장** — `IdentifyResponse` 에 `efficacy_text`/`usage_text` 추가, `PoolItem` 에 `user_category` 추가
> - **PillApi 신규 엔드포인트** — `POST /v1/pill/identify/narrow` (단계별 좁히기, 음성·이미지로 식별 어려울 때 fallback)
> - DB ERD v3 의 `pill_identification.classification_no/name` + `user_medication_pool.user_category` 컬럼과 매핑

---

## 1. API 명세서 목록

| 파일 | 모듈 | 엔드포인트 | 본 버전 |
| --- | --- | --- | --- |
| [AuthApi.md](AuthApi.md) | 모듈 6 (인증) | `POST /v1/auth/signup`, `/v1/auth/login`, `/v1/auth/logout` | v0.1 |
| [HistoryApi.md](HistoryApi.md) | 모듈 2 (복약 이력) | `POST /v1/history/record`, `GET /v1/history/list` | v0.1 |
| [ReportApi.md](ReportApi.md) | 모듈 5 (통합 보고서) | `GET /v1/report/generate` (json/html/pdf) | **v0.2** |
| [MediaApi.md](MediaApi.md) | 미디어 송수신 | `POST /v1/media/{intent, commit, get_token, image(레거시)}` ⭐ | **v0.2** |
| [SpeechApi.md](SpeechApi.md) | 음성 텍스트 (폰 STT) | `POST /v1/speech/utterance` | v0.1 |
| [PillApi.md](PillApi.md) | 모듈 1 (식별·DUR + 약 풀 + 단계별 좁히기 + Onboarding 정규화) | `POST /v1/pill/identify`, `/v1/pill/identify/narrow`, `/v1/pill/onboarding/normalize` ⭐, `/v1/pill/pool/*` | **v0.3** |
| [MonitoringApi.md](MonitoringApi.md) | 자원 모니터링 (전 영역) | `GET /health`, `GET /metrics` | v0.1 |

---

## 2. 호스트·포트·버전

| 서버 | OS | 호스트 (LAN `10.10.10.0/24`) | 기본 포트 | 베이스 URL |
| --- | --- | --- | --- | --- |
| Client (PhoneAdapter) | Windows 10/11 | `localhost` (폰 → adb reverse) | 8000 | `http://localhost:8000` |
| **MainServer** | Ubuntu 24.04 | **10.10.10.97** | 8001 | `http://10.10.10.97:8001/v1` |
| **DataStoragePC** ⭐ | Ubuntu 24.04 | **10.10.10.122** | **8004** | Drogon C++ 미니 서버. `PUT/GET /storage/photos/{anon}/{photo_id}.{ext}` + HMAC 토큰 검증 |
| **InferenceServer (LLM)** ⭐ | Ubuntu 24.04 (GPU) | **10.10.10.120** | 8002 | `http://10.10.10.120:8002/v1` (Stage 0.5 의도 분류 / Onboarding RAG / 일반 안내) |
| **InferenceServer (Vision)** ⭐ | Ubuntu 24.04 (GPU) | **10.10.10.128** | 8003 | `http://10.10.10.128:8003/v1` (YOLO·PaddleOCR·OpenCV) |

> 📌 **추론 서버 카테고리별 2대 분리** — 학습+추론 동거, 네트워크 분리 가능 설계. 향후 PC 증설 시 학습/추론 PC 분리해도 포트·베이스 URL 그대로 유지.
> 📌 **추론 라우팅 책임은 메인 서버** — 클라는 메인 서버 단일 엔드포인트만 호출. 메인 서버가 텍스트 입력은 `INFERENCE_LLM_BASE`, 이미지 입력은 `INFERENCE_VISION_BASE` 로 분기 호출.

> **버전 prefix `/v1/`**: 향후 호환성 깨지는 변경 시 `/v2/` 로 분기.
> `/v1/` 안에서의 v0.x 변경은 **호환 변경**(필드 추가만, 기존 필드 삭제·타입 변경 X) 으로 제한.

---

## 3. 공통 헤더

### 3.1 요청 헤더

| 헤더 | 필수 | 설명 |
| --- | --- | --- |
| `Content-Type` | ✓ | `application/json` (기본) / `multipart/form-data` (파일 업로드) |
| `Authorization` | 인증 필요 시 ✓ | `Bearer <JWT>` |
| `X-Request-ID` | 권장 | 클라가 발급하는 요청 추적 ID (UUID 등) |

### 3.2 응답 헤더

| 헤더 | 설명 |
| --- | --- |
| `Content-Type` | `application/json` (기본) / `application/pdf` (보고서) |
| `X-Request-ID` | 요청 헤더 값 그대로 echo (없으면 서버 생성) |

---

## 4. 표준 응답 형식

### 4.1 성공 응답
엔드포인트별 데이터를 그대로 JSON 으로 반환. envelope X.

### 4.2 에러 응답 (공통 envelope)
```json
{
  "error": {
    "code": "INVALID_EMAIL",
    "message": "이메일 형식이 올바르지 않습니다",
    "details": { "field": "email", "value": "invalid@@example" }
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
- 토큰 형식: JWT (HS256, MainServer Config 시크릿)
- 만료: 기본 24시간

### 6.2 토큰 사용
모든 보호 엔드포인트는 다음 헤더 필수:
```
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6Ikp...
```

### 6.3 토큰 무효화
- `POST /v1/auth/logout` 호출 시 서버가 토큰을 블랙리스트 등록 (또는 짧은 만료)
- 만료 시 401 반환 → 클라는 재로그인 유도

---

## 7. 페이지네이션 (목록 조회)

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

[요구사항 분석서 v2 §3.3](../요구사항_분석서_ver2.md) 의 표현 톤 정책을 모든 API 응답 메시지에 적용:

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
| `/v1/pill/*` | `PillSchema.h/.cpp` | `IdentifyRequest/Response`, `PillCandidate`, `PoolItem`, `NarrowDownRequest/Response`, **`OnboardingNormalizeRequest/Response`·`OnboardingCandidate/Question/Confirmation`** ⭐ |
| `/health`, `/metrics` | `HealthSchema.h/.cpp` | `HealthResponse`, `MetricsResponse` |

---

## 10. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v0.1 | 2026-05-06 | 팀 (3인) | 초안 — 호스트·포트, 공통 헤더, 표준 응답·에러, 상태 코드, JWT, 페이지네이션, 표현 톤, Schemas 매핑 |
| v0.2 | 2026-05-07 | 팀 (3인) | 목업 분석 반영 — PillApi 확장(`efficacy_text`/`usage_text`/`user_category` 추가), 신규 `POST /v1/pill/identify/narrow` (단계별 좁히기), DB ERD v3 매핑. 다른 명세는 변경 없음 (호환 변경) |
| v0.3 | 2026-05-07 | 팀 (3인) | 시스템_연결구조 v2.1 반영 — §2 호스트 표 IP 확정(메인 10.10.10.97 / 데이터보관 10.10.10.122 / **LLM 10.10.10.120 / Vision 10.10.10.128**), 추론 서버 카테고리별 2대 분리, 학습+추론 동거(네트워크 분리 가능 설계). + **PillApi v0.3** 인덱스 갱신 (신규 `POST /v1/pill/onboarding/normalize` Onboarding RAG round-trip). 엔드포인트는 호환 변경 (추가만, 기존 X) |
