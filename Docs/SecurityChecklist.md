# 메디브릿지 보안 체크리스트

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 보안 점검·실 구현 가이드 |
| **버전** | v1.1 |
| **개정일** | 2026-05-13 |
| **이전 버전** | v1.0 (2026-05-07) |

> 본 문서는 영역 A/B/C 의 TODO 본 구현 시 **반드시 준수해야 할 보안 규칙**을 정리한다.
> v0.1 골격 단계의 보안 점검(2026-05-07) 결과 발견된 이슈들의 해결책 + 향후 작업 시 참고용 가이드.
>
> ⭐ **v1.1 추가 항목** (사진 흐름 ⑤+⑥ 구현 후):
> - HMAC 시크릿 분리 (`MEDIBRIDGE_JWT_SECRET` ≠ `MEDIBRIDGE_STORAGE_SECRET`)
> - 보관 PC 11가지 검증 (`iss`/`aud`/`exp`/`op`/`sub`/`jti`/`mime`/`max`/CT/경로/ID 화이트리스트)
> - 가명화 — 토큰 `sub` 에 `anonymous_id` 사용 (user_id 노출 X)
> - 파일 시스템 — atomic write (.tmp → rename), 경로 traversal 방어
> - PDF 변환 — `system()` 금지, `fork+execvp` 사용

---

## 1. SQL Injection 방어 (필수)

**원칙**: **모든 SQL은 prepared statement (파라미터 바인딩) 사용. 문자열 결합 절대 금지.**

### ✅ Drogon ORM (권장)
```cpp
auto db = drogon::app().getDbClient();
auto future = db->execSqlAsyncFuture(
    "SELECT * FROM users WHERE email = $1 AND status = $2",
    email, status);     // 바인딩 (자동 escape)
```

### ✅ LIKE 와일드카드는 값에 포함
```cpp
db->execSqlAsyncFuture(
    "SELECT * FROM pill_identification WHERE engraving_front LIKE $1",
    "%" + engraving + "%");   // ← 와일드카드를 값에 (SQL에 X)
```

### ❌ 금지 — 문자열 결합
```cpp
std::string sql = "SELECT * FROM users WHERE email = '" + email + "'";  // SQL Injection!
db->execSqlSync(sql);
```

### ❌ 금지 — Drogon 특수 케이스
- `db->execSqlSync()` 의 첫 인자에 사용자 입력 결합 X
- 동적 IN 쿼리 시 placeholder 갯수만큼 동적 생성 (값은 바인딩)

---

## 2. JWT 시크릿 관리 (필수)

### 환경변수 강제
- `MEDIBRIDGE_JWT_SECRET` 환경변수로 시크릿 주입
- production 모드(`MEDIBRIDGE_ENV=production`)에서는 미설정 시 서버 시작 거부 (`std::abort`)
- HS256 권장 길이: **32바이트 이상** (Config 자동 검증)

### 시크릿 생성 예시 (Linux)
```bash
openssl rand -base64 48      # → 64자 랜덤 문자열
export MEDIBRIDGE_JWT_SECRET="..."
```

### 토큰 검증 정책
- 만료 시간(`exp`) 검증 필수
- 서명 알고리즘 화이트리스트 (`alg: none` 거부)
- 블랙리스트 (logout 시 등록)
- 시계 오차 허용 (`clock_skew` 30초)

---

## 3. 비밀번호 해시 (필수)

### 알고리즘
- **bcrypt (cost=12 권장)** 또는 Argon2id
- 평문 저장 절대 금지 (DB·로그 모두)

### 검증 시 타이밍 공격 방어
- bcrypt 자체 상수 시간 비교 (별도 처리 불필요)
- 직접 비교 시 `CRYPTO_memcmp` 또는 `crypto_verify_*` 사용

### 강도 정책 (FR-B8 권장 확장)
- 최소 8자
- 영문·숫자·특수문자 혼합 권장
- 사전 단어 차단 (선택)

---

## 4. 프롬프트 인젝션 방어 (필수)

### 다층 방어 (요구사항 분석서 §5.6 / FR-B6)
1. **InjectionFilter** (정규식 1차 필터) — `InferenceServer/Llm/InjectionFilter.py`
2. **시스템 프롬프트 락** — JSON 스키마 강제 출력 (FR-A5-02)
3. **영역 분리** — 의료 안내 영역 응답 생성 코드 경로에 LLM 호출 부재 (FR-B6-03)
4. **로그 감시** — 의심 패턴 로깅 (FR-B6-05)

### 절대 규칙
- 외부 인터넷 출처 사용 금지 (식약처 데이터만)
- LLM 응답이 JSON 스키마 위반 시 reject → OTHER 강제

---

## 5. URL 인코딩 (Client API 클라이언트)

### ✅ QUrlQuery 사용
```cpp
QUrlQuery query;
query.addQueryItem("from_date", from_date);   // 자동 percent-encoding
query.addQueryItem("memo", memo);             // '&', '=', 한글 모두 안전
const QString path = "/v1/history/list?" + query.toString(QUrl::FullyEncoded);
```

### ❌ 금지 — 문자열 결합
```cpp
QString path = "/v1/history/list?from_date=" + from_date;  // 특수문자 깨짐 + 인젝션 표면
```

---

## 6. 로깅·PII 마스킹 (필수)

### 로그에 절대 안 찍을 것
- 비밀번호 평문
- JWT 토큰 본체 (길이만 OK)
- 사용자 발화 본문 (`request.body()` 통째로)
- 이미지 바이너리
- API 키·시크릿

### 마스킹 패턴
```cpp
// ❌ 위험
qInfo() << "STT 텍스트:" << request.body();

// ✅ 안전
qInfo() << "STT 텍스트 size:" << request.body().size() << "bytes (본문 마스킹)";
```

### 로그 레벨 분리
- `qDebug` — 개발용 (production에서 비활성)
- `qInfo` — 운영 정보 (PII X)
- `qWarning` — 잠재적 문제
- `qCritical` — 즉시 대응 필요

---

## 7. DoS 방어 (Drogon)

### Main.cpp 설정 (이미 적용)
```cpp
drogon::app()
    .setClientMaxBodySize(20 * 1024 * 1024)              // 본문 최대 20MB
    .setClientMaxMemoryBodySize(1 * 1024 * 1024)         // 메모리 1MB 초과 시 디스크
    .setClientMaxWebSocketMessageSize(1 * 1024 * 1024)
    .setMaxConnectionNumPerIP(50)                         // IP별 50 연결
    .setIdleConnectionTimeout(60);
```

### 추가 권장 (확장 단계)
- Rate limiting (사용자별·IP별 분당 N회)
- 캐시 키 일관 (인증된 사용자만 응답 캐시)
- WAF (사내 환경이라 우선순위 ↓)

---

## 8. Thread Safety (멀티스레드 환경)

### 사용 금지 함수
- ❌ `std::gmtime()` — 정적 버퍼, race condition. **`utils::current_iso8601_utc()` 헬퍼 사용**
- ❌ `std::localtime()` — 동일 위험
- ❌ `std::asctime()` / `std::ctime()` — 동일

### 권장
- ✅ `gmtime_r` (POSIX) / `gmtime_s` (Windows)
- ✅ C++20 `std::format("{:%FT%TZ}", ...)`
- ✅ thread_local 버퍼

본 프로젝트는 `MainServer/Utils/TimeUtil.h` 헬퍼를 통해 이미 적용.

---

## 9. 외부 프로세스 실행 (PhoneLink)

### `QProcess::start("adb", ...)` 안전 패턴
- ✅ 인자는 **고정값** 만 (`{"devices"}`, `{"reverse", "tcp:8000", "tcp:8000"}`)
- ❌ 사용자 입력으로 인자 만들기 금지 (커맨드 인젝션)
- ❌ shell 호출 금지 (`QProcess::startCommand` 가급적 X)

본 프로젝트의 `AdbDeviceMonitor`, `AdbReverseManager` 는 모두 고정 인자 → 안전.

---

## 10. 본 골격 단계 적용 완료된 이슈

| # | 이슈 | 해결 |
| --- | --- | --- |
| 1 | `std::gmtime` 3곳 사용 | `Utils/TimeUtil.h::current_iso8601_utc()` 헬퍼로 교체 |
| 2 | JWT 시크릿 기본값 | `Config::load_from_file` 에서 production 모드 강제 검증 |
| 3 | URL 쿼리 미인코딩 | HistoryApiClient·ReportApiClient 가 `QUrlQuery` 사용 |
| 4 | PhoneServer 로그 PII | request.body() 통째 출력 → 크기만 |
| 5 | Drogon body 크기 미설정 | `setClientMaxBodySize(20MB)` + DoS 방어 옵션 |
| 6 | request_id 예측 가능 | `QUuid::createUuid()` 로 변경 |
| 7 | TODO 주석 약함 | UserManager·PillIdentificationCache·DurQueryEngine 에 prepare 예시 |

---

## 11. 본 구현 시점에 추가 적용해야 할 항목

| # | 항목 | 책임 영역 |
| --- | --- | --- |
| 1 | bcrypt 또는 Argon2id 라이브러리 통합 | 영역 B (`Auth/PasswordHasher.cpp`) |
| 2 | jwt-cpp 통합 + alg:none 거부 | 영역 B (`Auth/JwtIssuer.cpp`) |
| 3 | 모든 DB 쿼리 prepared statement 검증 | 영역 B 전체 |
| 4 | InjectionFilter 패턴 운용 중 확장 | 영역 A |
| 5 | (확장) HTTPS/TLS 적용 (운영 단계) | 인프라 |
| 6 | (확장) Rate limiting | 영역 B |
| 7 | (확장) 시크릿 관리 도구 (Vault·SOPS) | 인프라 |

---

## 12. 사진 흐름 ⑤+⑥ 보안 (v1.1 신규)

### 12.1 HMAC 토큰 시크릿 분리
- `MEDIBRIDGE_JWT_SECRET` (사용자 JWT) ≠ `MEDIBRIDGE_STORAGE_SECRET` (보관 PC 토큰)
- 32+ 바이트 무작위. 별도 시크릿이라 한쪽 유출돼도 다른 쪽 토큰 위조 불가.
- 메인서버와 보관 PC 가 같은 `MEDIBRIDGE_STORAGE_SECRET` 공유 — 토큰 발급(메인) ↔ 검증(보관) 짝.

### 12.2 토큰 payload 검증 (보관 PC 11가지)
1. HMAC 서명 일치 (`hmac_sha256(secret, header.payload)`)
2. `iss="medibridge-main"` 강제
3. `aud="datastorage"` 강제 (JWT 와 혼용 방어)
4. `exp > now` (TTL 300s, 짧게)
5. `op` ∈ {`put`, `get`} 일치 (PUT 토큰으로 GET 불가)
6. URL `{anon}` ↔ token `sub` 일치
7. URL `{photo_id}` ↔ token `jti` 일치
8. URL 확장자 ↔ token `mime` 일치
9. Content-Type ↔ token `mime` 일치
10. Content-Length ≤ `min(token max, server max)`
11. `anon`/`photo_id` 정규식 화이트리스트 `[A-Za-z0-9_-]{1,64}` — 경로 traversal 방어

### 12.3 가명화 (PII 보호)
- 토큰 `sub` 에 **`anonymous_id`** 사용 — `user_id` 토큰 외부 노출 X
- 파일 시스템 경로: `<storage_root>/<anonymous_id>/<photo_id>.<ext>` (`user_id` X)
- DB `photo_storage.anonymous_id` FK 만 보유

### 12.4 파일 시스템 보안
- **atomic write** — `.tmp` 에 쓴 뒤 `rename()` (반쪽 파일 방지)
- 경로 정규식 화이트리스트 통과 후만 디스크 접근
- 확장자 화이트리스트 (`jpg`, `png`) — 임의 파일 업로드 방지

### 12.5 외부 프로세스 호출 (Report PDF)
- `system()` / `popen()` **사용 금지** — 셸 escape 위험
- **`fork+execvp`** + argv 배열 직접 구성 (인자 escape 위험 0)
- 임시 파일 `mkstemps` race-free
- RAII Cleanup struct — 함수 종료 시 항상 `unlink`

### 12.6 청소 잡
- PENDING + `expires_at < NOW()` → EXPIRED 마킹 (DB 누적 방지)
- 토큰은 어차피 `exp` 검증으로 거부되니 보안에 직접 영향 X, 운영 가시성 차원
- 인터벌 `MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL` (기본 300s, 0=비활성)

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v1.0 | 2026-05-07 | 초안 작성 — v0.1 골격 단계 보안 점검 결과 + 본 구현 가이드 |
| **v1.1** | **2026-05-13** | §12 신규 — 사진 흐름 ⑤+⑥ 보안 (HMAC 시크릿 분리, 11가지 토큰 검증, 가명화, atomic write, fork+execvp, 청소 잡). 적용 코드: `Services/Media/StorageTokenIssuer`, `DataStorageServer/{TokenVerifier, StorageManager, Photo}`, `Services/Report/ReportPdfRenderer`, `Main.cpp` (청소 잡 `runEvery`). |
