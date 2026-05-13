# Media API — 이미지 송수신 (사진 흐름 ⑤+⑥)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.2 |
| **개정일** | 2026-05-13 |
| **이전 버전** | v0.1 (2026-05-06) → `Docs/Old/Api/MediaApi_v0.1_*.md` |
| **모듈** | 미디어 송수신 (모듈 1 식별의 입력) |
| **관련 문서** | [프로토콜 v2.2 §2.3](../프로토콜_ver2.md), [시스템 흐름 v3.1 §17](../시스템%20흐름%20정리본_ver3.md), [DB ERD v4.1 photo_storage](../DB_ERD_ver4.md) |
| **공통 규칙** | [ApiOverview v0.4](ApiOverview.md) |

> ⭐ **v0.2 변경 핵심** (사진 흐름 ⑤+⑥ 통합):
> - **사진 본체가 메인서버 통과 X.** 메인은 HMAC 토큰만 발급, 클라가 보관 PC(`10.10.10.122:8004`) 에 직접 PUT.
> - **신규 엔드포인트 3개**: `POST /v1/media/intent`, `POST /v1/media/commit`, `POST /v1/media/get_token`
> - **레거시 `POST /v1/media/image`** — TestMode/보관 PC 미가동 fallback 으로만 유지
> - HMAC 시크릿 분리 — JWT 시크릿과 별개의 `MEDIBRIDGE_STORAGE_SECRET`

---

## 통신 흐름

```
[클라PC]                    [메인 :8001]                    [보관 PC :8004]
   │                            │                               │
   │ ① POST /v1/media/intent    │                               │
   │ ──────────────────────────▶│ photo_storage INSERT          │
   │                            │   (status=PENDING, exp=now+TTL)│
   │ ◀────────────────────────  │ {photo_id, storage_url,        │
   │                            │  put_token, expires_at}        │
   │                            │                               │
   │ ② PUT storage_url          │                               │
   │   Authorization: put_token │                               │
   │ ────────────────────────────────────────────────────────▶ │ 11가지 검증
   │                            │                               │ atomic write
   │ ◀──── 201 Created ─────────────────────────────────────── │
   │                            │                               │
   │ ③ POST /v1/media/commit    │                               │
   │ ──────────────────────────▶│ status=PENDING → READY        │
   │ ◀──── 200 ────────────────│                               │
   │                            │                               │
   │ ④ POST /v1/pill/identify   │                               │
   │ ──────────────────────────▶│ get_token 발급 → Vision PC    │
                                                                  ↓ GET storage_url
                                                                  ↓ Authorization: get_token
                                                              [Vision PC]
```

---

## 엔드포인트 목록

| HTTP | URL | 설명 | 인증 | 위치 |
| --- | --- | --- | --- | --- |
| POST | `/v1/media/intent` | 사진 업로드 의향 + 토큰 발급 ⭐ | ✓ JWT | MainServer |
| PUT | `<storage_url>` (보관 PC 8004) | 사진 본체 전송 ⭐ | ✓ put_token | DataStoragePC |
| POST | `/v1/media/commit` | PUT 완료 통지 (PENDING → READY) ⭐ | ✓ JWT | MainServer |
| POST | `/v1/media/get_token` | Vision PC 용 GET 토큰 (READY만) ⭐ | ✓ JWT | MainServer |
| GET | `<storage_url>` (보관 PC 8004) | 사진 본체 다운로드 (Vision PC) | ✓ get_token | DataStoragePC |
| POST | `/v1/media/image` | **레거시** — 메인서버 로컬 `uploads/` 저장 | ✓ JWT | MainServer |

---

## 1. POST /v1/media/intent — 사진 업로드 의향 신호

클라가 사진 본체를 보내기 전 메인에 "올려도 됨?" 묻고 보관 PC URL + 단기 토큰을 받음.

### 요청
- 헤더: `Authorization: Bearer <JWT>`, `Content-Type: application/json`
- 본문:
```json
{
  "mime_type": "image/jpeg",
  "size_bytes": 2048000,
  "purpose":   "IDENTIFY",
  "request_id": "req_abc123"
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| `mime_type` | string | ✓ | `image/jpeg` 또는 `image/png` |
| `size_bytes` | int | ✓ | 본문 예상 크기. 서버 `max_bytes` 와 비교 |
| `purpose` | string | – | `IDENTIFY`(기본) / `TRAIN` / `OTHER` |
| `request_id` | string | – | 클라 부여 추적 ID. 미입력 시 서버 생성 |

### 응답 — **201 Created**

```json
{
  "photo_id":    "ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d",
  "request_id":  "req_abc123",
  "storage_url": "http://10.10.10.122:8004/storage/photos/anon_test_001/ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d.jpg",
  "put_token":   "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJkYXRhc3RvcmFnZSIsInN1YiI6ImFub25fdGVzdF8wMDEiLCJqdGkiOiJwaF8zZjBhLi4uIiwib3AiOiJwdXQiLCJtaW1lIjoiaW1hZ2UvanBlZyIsIm1heCI6MTA0ODU3NjAsImlhdCI6MTcxNDAwMCwiZXhwIjoxNzE0MzAwfQ.signature",
  "expires_at":  "2026-05-13T15:40:00Z",
  "max_bytes":   10485760
}
```

### put_token payload (HS256, `MEDIBRIDGE_STORAGE_SECRET` 서명)
```json
{
  "iss":  "medibridge-main",
  "aud":  "datastorage",
  "sub":  "<anonymous_id>",     // 가명화 — user_id 노출 X
  "jti":  "<photo_id>",          // 1회 사용
  "op":   "put",                 // PUT 전용 (GET 불가)
  "mime": "image/jpeg",
  "max":  10485760,
  "iat":  <unix>,
  "exp":  <unix + 300>           // 300s TTL
}
```

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_MIME_TYPE` | jpeg/png 아님 |
| 400 | `INVALID_SIZE` | size_bytes ≤ 0 |
| 400 | `SIZE_EXCEEDED` | size_bytes > storage_max_bytes |
| 400 | `INVALID_PURPOSE` | IDENTIFY/TRAIN/OTHER 외 |
| 401 | `INVALID_TOKEN` | JWT 검증 실패 |
| 503 | `DB_UNAVAILABLE` | DB 미준비 |

---

## 2. PUT `<storage_url>` (보관 PC 8004) — 사진 본체 직접 전송

보관 PC `10.10.10.122:8004` 의 `PUT /storage/photos/{anon}/{photo_id}.{ext}` 호출. 메인서버 통과 X.

### 요청
- 헤더: `Authorization: Bearer <put_token>`, `Content-Type: image/jpeg|png`
- 본문: 바이너리 이미지 (≤ max_bytes)

### 응답 — **201 Created**
```json
{ "photo_id": "ph_…", "bytes": 2048000, "status": "stored" }
```

### 보관 PC 측 11가지 검증
1. HMAC 서명 일치
2. `iss="medibridge-main"`
3. `aud="datastorage"`
4. `exp > now` (만료 X)
5. `op="put"` 일치 (GET 토큰 불가)
6. URL `{anon}` ↔ 토큰 `sub` 일치
7. URL `{photo_id}` ↔ 토큰 `jti` 일치
8. URL 확장자 ↔ 토큰 `mime` 일치
9. Content-Type ↔ 토큰 `mime` 일치
10. Content-Length ≤ `min(토큰 max, 서버 max)`
11. ID 화이트리스트 `[A-Za-z0-9_-]{1,64}` (경로 traversal 방어)

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 401 | `INVALID_TOKEN` | 토큰 만료/위조/op 불일치 |
| 403 | `ANON_MISMATCH` | URL anon ≠ 토큰 sub |
| 403 | `PHOTO_ID_MISMATCH` | URL photo_id ≠ 토큰 jti |
| 400 | `MIME_EXT_MISMATCH` | 확장자 ≠ 토큰 mime |
| 400 | `CONTENT_TYPE_MISMATCH` | CT 헤더 ≠ 토큰 mime |
| 400 | `EMPTY_BODY` | 본문 0 바이트 |
| 413 | `SIZE_EXCEEDED` | 본문 > 한도 |
| 500 | `STORAGE_FAILED` | 디스크 I/O 실패 |

---

## 3. POST /v1/media/commit — PUT 완료 통지

PUT 성공 후 클라가 호출 → `photo_storage` status PENDING → READY 전이. **idempotent** (재호출 OK).

### 요청
```json
{ "photo_id": "ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d" }
```

### 응답 — **200 OK**
```json
{ "photo_id": "ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d", "status": "READY" }
```

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `MISSING_PHOTO_ID` | 본문 누락 |
| 401 | `INVALID_TOKEN` | JWT 검증 실패 |
| 404 | `NOT_FOUND` | photo_id 없음 또는 권한 없음 |
| 409 | `INVALID_STATE` | EXPIRED/FAILED 상태에서 commit 시도 |

> 미호출 시: 청소 잡(기본 5분 인터벌)이 `expires_at` 경과 row 를 EXPIRED 로 마킹.

---

## 4. POST /v1/media/get_token — Vision PC 용 GET 토큰 발급

`/v1/pill/identify` 처리 중 메인이 자동 발급하는 게 정상 경로지만, Vision PC 단독 호출/디버그/리커버리용으로 별도 노출. **본인 소유 + status=READY 만**.

### 요청
```json
{ "photo_id": "ph_…" }
```

### 응답 — **200 OK**
```json
{
  "photo_id":    "ph_…",
  "storage_url": "http://10.10.10.122:8004/storage/photos/anon_…/ph_….jpg",
  "get_token":   "eyJ...",     // op=get
  "mime_type":   "image/jpeg",
  "expires_at":  "2026-05-13T15:45:00Z"
}
```

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 404 | `NOT_FOUND` | photo_id 없음 또는 권한 없음 |
| 409 | `INVALID_STATE` | status ≠ READY (PENDING/EXPIRED/FAILED) |

---

## 5. POST /v1/media/image — **레거시 / TestMode fallback**

보관 PC 미가동 또는 시드 시나리오용. 본문은 메인서버 메모리에서 discard, `photo_storage` 메타만 INSERT (status=READY 즉시).

운영 모드에서는 사용하지 않는 것을 권장. 신규 코드는 `/v1/media/intent` → PUT → `/v1/media/commit` 흐름 사용.

요청·응답 schema 는 v0.1 과 동일. 자세한 내용은 코드(`MainServer/Routers/Media.cpp::handle_image_upload`) 참조.

---

## Schemas/ 매핑

| 엔드포인트 | C++ Schema |
| --- | --- |
| `/v1/media/intent` | `MediaIntentRequest`, `MediaIntentResponse` |
| `/v1/media/commit` | `MediaCommitRequest`, `MediaCommitResponse` |
| `/v1/media/get_token` | `MediaGetTokenRequest`, `MediaGetTokenResponse` |
| `/v1/media/image` | `ImageUploadRequest`, `ImageUploadResponse` |

위치: `MainServer/Schemas/MediaSchema.h`

서비스 계층:
- `MainServer/Services/Media/StorageTokenIssuer` — HMAC 토큰 발급 (op=put|get)
- `DataStorageServer/Services/TokenVerifier` — 같은 알고리즘으로 검증 (시크릿 공유)
- `DataStorageServer/Services/StorageManager` — atomic write (.tmp → rename)

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — `POST /v1/media/image` multipart/base64 |
| **v0.2** | **2026-05-13** | **사진 흐름 ⑤+⑥ 통합** — intent / commit / get_token 신규 3개. 사진 본체 메인서버 통과 X (보관 PC 8004 직접 PUT). HMAC 토큰 (op=put\|get, aud=datastorage). 11가지 검증. 청소 잡 자동 동작. 레거시 `/v1/media/image` 는 TestMode fallback. |
