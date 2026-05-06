# Auth API — 인증 (모듈 6)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.1 |
| **작성일** | 2026-05-06 |
| **모듈** | 모듈 6 (사용자 계정 / 인증) |
| **관련 문서** | [프로토콜 v2 §2.1](../프로토콜_ver2.md), [요구사항 분석서 v2 §5.10 FR-B8 / §6.9 FR-C7](../요구사항_분석서_ver2.md), [DB ERD v2 — users 테이블](../DB_ERD_ver2.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> 사용자별 약 풀·복약 이력을 분리 관리하기 위한 인증 API. 비밀번호는 bcrypt 해시 저장, 로그인 후 JWT 발급. 모든 보호 API는 `Authorization: Bearer <JWT>` 헤더 필수.

---

## 엔드포인트 목록

| HTTP | URL | 설명 | 인증 |
| --- | --- | --- | --- |
| POST | `/v1/auth/signup` | 회원가입 | X |
| POST | `/v1/auth/login` | 로그인 + JWT 발급 | X |
| POST | `/v1/auth/logout` | 로그아웃 + 토큰 무효화 | ✓ |

---

## 1. POST /v1/auth/signup — 회원가입

### 요청
- 헤더: `Content-Type: application/json`
- 바디:
```json
{
  "email": "user@example.com",
  "password": "P@ssw0rd!",
  "user_name": "홍길동"
}
```

| 필드 | 타입 | 필수 | 검증 |
| --- | --- | --- | --- |
| `email` | string | ✓ | RFC 5322 이메일 형식, 길이 ≤ 255 |
| `password` | string | ✓ | 8자 이상, 영문·숫자·특수문자 포함 권장 |
| `user_name` | string | ✓ | 길이 1~100 |

### 응답
- **201 Created**
```json
{
  "user_id": "u_abc123",
  "email": "user@example.com",
  "user_name": "홍길동",
  "created_at": "2026-05-06T10:30:00Z"
}
```

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_EMAIL` | 이메일 형식 오류 |
| 400 | `WEAK_PASSWORD` | 비밀번호 정책 미달 |
| 400 | `MISSING_FIELD` | 필수 필드 누락 |
| 409 | `EMAIL_ALREADY_EXISTS` | 이메일 중복 |

### Schemas/ 매핑
- `MainServer/Schemas/AuthSchema.h::SignupRequest`
- `MainServer/Schemas/AuthSchema.h::SignupResponse`

---

## 2. POST /v1/auth/login — 로그인

### 요청
- 헤더: `Content-Type: application/json`
- 바디:
```json
{
  "email": "user@example.com",
  "password": "P@ssw0rd!"
}
```

### 응답
- **200 OK**
```json
{
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6Ikp...",
  "token_type": "Bearer",
  "expires_in": 86400,
  "user": {
    "user_id": "u_abc123",
    "email": "user@example.com",
    "user_name": "홍길동"
  }
}
```

| 필드 | 설명 |
| --- | --- |
| `access_token` | JWT 토큰 (이후 모든 인증 요청에 사용) |
| `token_type` | 항상 `"Bearer"` |
| `expires_in` | 토큰 유효 시간(초). 기본 86400 (24시간) |
| `user` | 로그인 사용자 정보 (UI 표시용) |

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `MISSING_FIELD` | email 또는 password 누락 |
| 401 | `INVALID_CREDENTIALS` | 이메일·비밀번호 불일치 |
| 423 | `ACCOUNT_LOCKED` | (확장) 다회 실패로 잠김 |

### Schemas/ 매핑
- `MainServer/Schemas/AuthSchema.h::LoginRequest`
- `MainServer/Schemas/AuthSchema.h::LoginResponse`
- `MainServer/Schemas/AuthSchema.h::UserSummary`

---

## 3. POST /v1/auth/logout — 로그아웃

### 요청
- 헤더:
  - `Authorization: Bearer <JWT>` ✓
- 바디: 없음

### 응답
- **204 No Content** — 본문 없음

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 401 | `MISSING_TOKEN` | Authorization 헤더 없음 |
| 401 | `INVALID_TOKEN` | 토큰 형식·서명 오류 |
| 401 | `EXPIRED_TOKEN` | 만료된 토큰 |

### 동작
- 서버: 토큰 블랙리스트 등록 또는 만료 처리
- 클라: 로컬 저장된 토큰 삭제 + 로그인 화면으로 이동

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — 회원가입·로그인·로그아웃 정의, JWT 발급 정책, 에러 코드 정리 |
