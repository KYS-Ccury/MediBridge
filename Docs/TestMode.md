# 메디브릿지 — TestMode 설계서

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | TestMode (개발 중 더미 응답) 설계서 |
| **버전** | v0.3 |
| **개정일** | 2026-05-13 |
| **이전 버전** | v0.1 (2026-05-07) |

> 추론 서버(LLM/Vision PC) 와 데이터 보관 PC 가 아직 가동되지 않은 시점에 **메인 서버만으로 클라이언트가 진짜처럼 동작하는 서비스를 체험**할 수 있게 하는 개발용 동작 모드.
>
> ⭐ **핵심 원칙**: **DB·Auth 는 항상 실(real)**, **추론·파일저장만 우회**. 더미 데이터는 코드에 박지 않고 **MariaDB seed 데이터** 로 둬서 운영 전환 시 row 만 갈아끼우면 끝나도록 한다.

---

## 1. 모드 분기

### 1.1 환경 변수
```
MEDIBRIDGE_TEST_MODE = "true" | "false"  (기본 false)
```

### 1.2 모드별 동작

| 영역 | TestMode 동작 | Production 동작 |
| --- | --- | --- |
| MariaDB 연결 | **실** (seed 적용 DB) | **실** (운영 DB) |
| Auth (signup/login/logout) | **실** (DB 사용자 검증, 단 세션 mock JWT 발급 — bcrypt/jwt-cpp 미설치 환경 허용) | **실** (bcrypt + jwt-cpp) |
| `POST /v1/pill/identify` | Vision PC 호출 X. **seed `pill_identification` 테이블에서 후보 N개 랜덤 추출** | Vision PC 호출 |
| `POST /v1/pill/identify/narrow` | Vision PC 호출 X. seed 기반 다음 질문 결정 (color/shape/engraving 순) | Vision PC 호출 |
| `POST /v1/pill/onboarding/normalize` | LLM PC 호출 X. **seed `pill_identification` 의 `drug_name` LIKE 매칭** + 가짜 분기 질문 | LLM PC 호출 (ChromaDB 벡터 검색) |
| `POST /v1/speech/utterance` | LLM PC 호출 X. 발화 텍스트 키워드 매칭으로 의도 분류 더미 | LLM PC 호출 |
| `POST /v1/media/intent` ⭐ v0.2 | **실 DB** — `photo_storage` INSERT (status=PENDING, expires_at=NOW()+TTL) + 실 HMAC put_token 발급. 보관 PC URL 은 환경변수대로 응답 | 동일 — 실 동작 |
| `POST /v1/media/commit` ⭐ v0.2 | **실 DB** — status PENDING → READY. idempotent | 동일 |
| `POST /v1/media/get_token` ⭐ v0.2 | **실 DB + 실 HMAC** — 본인 소유 + READY 상태만 발급 | 동일 |
| `POST /v1/media/image` (레거시) | Data Storage PC 호출 X. **`photo_storage` 메타만 INSERT**, 가짜 `storage_path` (실 파일 X), status=READY 즉시 마킹 | 레거시 — production 미사용 |
| `POST /v1/pill/pool` | **실 DB CRUD** (item_code 검증 포함) | 동일 |
| `POST /v1/history/record` | **실 DB CRUD** | 동일 |
| `GET /v1/report/generate` | **실 DB 조회** + 기본 HTML/PDF | 동일 |
| `GET /health` | DB ping + WorkerPool 상태 | 동일 |
| 식약처 OpenAPI (`PdmaApiClient`) | **호출 X**. seed `pill_identification` 활용 | lazy 캐싱 호출 |

### 1.3 모드 검증 시그널

서버 시작 시 stdout 에 다음 라인 출력:
```
[Config] MEDIBRIDGE_TEST_MODE = TRUE  → 추론·파일저장 우회, DB seed 사용
```
또는
```
[Config] MEDIBRIDGE_TEST_MODE = FALSE → 운영 모드 (모든 외부 의존성 활성)
```

---

## 2. seed 데이터 정책

### 2.1 식별 prefix
- 모든 가짜 `item_code` 는 `999800xxx` 으로 시작 (식약처 실 코드와 충돌 X)
- 가짜 `user_id` 는 `test_user_xxx`
- 가짜 `anonymous_id` 는 `anon_test_xxx`
- 가짜 storage_path 는 `/test/dev_seed/...` 로 시작

### 2.2 seed 적용 절차
```bash
# 1. 스키마 생성 (한 번만)
mysql -u medibridge_app -p medibridge < MainServer/Database/Migrations/001_init_schema.sql

# 2. 더미 데이터 적용 (TestMode 사용 시)
mysql -u medibridge_app -p medibridge < MainServer/Database/Seeds/dev_seed.sql
```

### 2.3 seed 내용 (요약)

| 테이블 | 행 수 | 비고 |
| --- | --- | --- |
| `users` | 2 | `test@medibridge.local` / `pw=test1234` 등 |
| `pseudonym_map` | 2 | user → anonymous 매핑 |
| `pill_identification` | 10 | 타이레놀·게보린·이부프로펜 계열 + 분류 카테고리 다양 |
| `dur_interaction_cache` | 2 | 가짜 위험 페어 (예시용) |
| `user_medication_pool` | 4 | 테스트 사용자 1번이 4개 약 등록 상태 |
| `medication_intake_logs` | 6 | 최근 7일치 복약 이력 샘플 |
| `pill_ingredient_mapping` | 10 | 위 10개 약의 성분 매핑 |

상세 SQL: [`MainServer/Database/Seeds/dev_seed.sql`](../MainServer/Database/Seeds/dev_seed.sql)

---

## 3. 서비스 클래스 분기 패턴

기존 Pattern B 클래스에 **TestMode 분기**를 한 곳에서 추가:

```cpp
// 예: PillService::onboarding_normalize()
if (Config::instance().test_mode()) {
    return TestModeProvider::instance().fake_onboarding_normalize(req);
}
return inference_client_->onboarding().extract_and_search(req);
```

`TestModeProvider` 는 새로 도입하는 싱글톤 — 각 도메인별 더미 응답 생성 책임:

```
MainServer/Services/TestMode/
├── TestModeProvider.h
├── TestModeProvider.cpp
├── DummyPillProvider.cpp        # /pill/identify, /narrow, /onboarding/normalize
├── DummySpeechProvider.cpp      # /speech/utterance
└── DummyMediaProvider.cpp       # /media/image
```

> 현재 본 v0.1 에서는 DummyXxxProvider 분리는 후속 작업으로 미루고, 라우터에서 직접 DB seed 를 조회하는 형태로 단순 구현. 호출자 분리만 명확히 유지.

---

## 4. 클라 측 변경

**없음.** 클라는 본 모드에 대해 알 필요가 없음 (API 계약 동일). 서버가 더미를 돌려줘도 클라는 정상 응답으로 받아 화면에 표시.

---

## 5. 주의 사항

- **TestMode 토큰의 보안 한계**: TestMode 의 mock JWT 는 서명을 단순한 형태로 발급. **production 모드에서 절대 활성화 금지** (`MEDIBRIDGE_ENV=production` 시 TestMode 강제 비활성화).
- **seed `999800xxx` prefix 외부 노출 금지**: 시연·발표 시 더미인 게 보일 수 있으니 노출 시점 인지 필요.
- **DurChecker 테스트**: seed `dur_interaction_cache` 의 가짜 위험 페어로 동작 확인. 실제 복용 위험 정보 X.

---

## 5-A. Dev 로깅 (TestMode 한정)

통합 검증 중 폰 STT 텍스트가 메인서버까지 도달했는지 본문 확인이 필요해, TestMode 일 때만 다음 로그가 출력됩니다.

### MainServer — `Routers/Speech.cpp`
```
[Speech][DEV] len=10 cat=PILL_IDENTIFY inj=N text="이 약 뭐예요?"
```
- 활성 조건: `Config::instance().test_mode() == true`
- 길이 + Stage 0.5 의도 분류 결과 + 인젝션 플래그 + 본문 **앞 80자**
- production 에서는 출력 안 됨 (PII 보호)

### Client — `PhoneAdapter/PhoneServer.cpp`
```
[PhoneServer][DEV] text="이 약 뭐예요?"
```
- 모든 빌드에서 출력 (빌드 플래그로 비활성 가능)
- 앞 80자 미리보기

### 사용 예
```bash
# WSL 메인서버
tail -f /tmp/medibridge.log | grep -iE "Speech|DEV"
```

→ 폰 → 클라 → 메인서버 전 구간에서 텍스트가 정상 전달됐는지 1초 확인 가능.

---

## 6. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v0.1 | 2026-05-07 | 팀 (3인) | 초안 — DB-backed dummy 데이터 정책, seed prefix 규칙, 서비스 분기 패턴 |
| **v0.2** | **2026-05-13** | 팀 (3인) | **사진 흐름 ⑤+⑥ TestMode 분기 명확화** — `/v1/media/{intent, commit, get_token}` 은 TestMode 에서도 **실 DB + 실 HMAC** 으로 동작 (보관 PC 가 같은 시크릿이면 실제 PUT 가능). 레거시 `/v1/media/image` 는 TestMode 한정 fallback. Pill identify 의 운영 모드 분기에서 Vision PC 미가동 시 502 반환 (TestMode 활성화 권장). |
| **v0.3** | **2026-05-15** | 팀 (3인) | §5-A Dev 로깅 신규 — TestMode 한정 utterance 본문 미리보기 (앞 80자) + 분류 결과 + 인젝션 플래그. PII 보호 정책상 production 에서는 출력 안 됨. 커밋 `0593288`. |
