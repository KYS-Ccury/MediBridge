# 메디브릿지 변경 이력 (CHANGELOG)

본 문서는 메디브릿지 프로젝트의 **코드·문서 변경 이력**을 통합 관리한다.
- 형식: [Keep a Changelog](https://keepachangelog.com/ko/1.1.0/) 1.1.0
- 버저닝: 문서는 ver1·ver2·ver3 단위 + API 는 v0.x semantic-like
- 최신 항목이 위로

> **Docs 변경 사유는 본 파일에 기록**, 코드 변경 사유는 git commit 메시지를 정본으로 한다(본 파일은 인덱스).

---

## [Unreleased]

### Changed (2026-05-13 — 클라이언트 사진 흐름 ⑤+⑥ 통합)

#### 클라이언트 정상 흐름 적용
- `Client/MainServerClient/MediaApiClient.{cpp,h}` — 3개 메소드 신규:
  - `request_intent(mime, size, purpose, cb)` → `POST /v1/media/intent`
  - `put_to_storage(storage_url, put_token, image, mime, cb)` → PUT 보관 PC (메인 우회, JWT 대신 put_token 헤더, `QNetworkAccessManager` 직접 호출)
  - `commit_upload(photo_id, cb)` → `POST /v1/media/commit`
  - 레거시 `upload_image` 는 유지 (TestMode/보관 PC 미가동 fallback)
- `Client/Backend/Controllers/PillController.cpp::on_capture_succeeded` — 4단계 콜백 체인으로 교체:
  1. `/v1/media/intent` → photo_id + storage_url + put_token + request_id 수신
  2. `PUT <storage_url>` (보관 PC `10.10.10.122:8004`) — Authorization: Bearer put_token
  3. `/v1/media/commit` → status PENDING → READY 전이
  4. `/v1/pill/identify` (request_id) — 기존 흐름
- 단계별 에러 코드: `INTENT_FAILED_<status>`, `INTENT_INVALID_RESPONSE`, `STORAGE_PUT_FAILED_<status>`, `COMMIT_FAILED_<status>`
- 사진 본체가 메인서버를 통과하지 않음 (컨트롤 평면 ↔ 데이터 평면 분리 완성)

#### Docs
- `Docs/Api/MediaApi.md v0.2` — 클라 통합 완료 표기
- `Docs/시스템 흐름 정리본_ver3.md §17.2` — 클라 통합 완료 (실제 흐름 동작)
- `Docs/CHANGELOG.md` — 본 항목

---

### Added (2026-05-13 — 사진 흐름 ⑤+⑥ + 보관 PC + Report PDF + 청소 잡)

#### 신규 모듈 — DataStorageServer (보관 PC Drogon 미니 서버, port 8004)
- `DataStorageServer/Main.cpp` + `Config.{cpp,h}` + `CMakeLists.txt` — 진입점·설정·빌드
- `DataStorageServer/Routers/Photo.{cpp,h}` — `PUT/GET /storage/photos/{anon}/{photo_id}.{ext}`
  - 11가지 보안 검증 (서명 / iss / aud / exp / op / sub / jti / mime / max / Content-Type / 경로 화이트리스트)
- `DataStorageServer/Routers/Monitoring.{cpp,h}` — `/health` + 디스크 여유
- `DataStorageServer/Services/TokenVerifier.{cpp,h}` — HMAC-SHA256 (JWT 호환) 검증
- `DataStorageServer/Services/StorageManager.{cpp,h}` — atomic write (.tmp → rename), 경로 traversal 방어
- `DataStorageServer/Scripts/datastorage-up.sh` + `smoke_test.sh` — 부팅·풀 시나리오 검증
- `DataStorageServer/README.md` — 환경변수·보안표·디스크 구조

#### 메인서버 — 사진 흐름 ⑤+⑥ 통합
- `MainServer/Services/Media/StorageTokenIssuer.{cpp,h}` — HMAC 토큰 발급 (`op`=put|get, `aud`=datastorage, `jti`=photo_id)
- `MainServer/Routers/Media.{cpp,h}` 확장 — 신규 엔드포인트 3개:
  - `POST /v1/media/intent` — 의향 신호 → photo_id + storage_url + put_token + expires_at
  - `POST /v1/media/commit` — PUT 완료 통지 → PENDING → READY (idempotent)
  - `POST /v1/media/get_token` — Vision PC 용 GET 토큰 (READY 상태만)
- `MainServer/Schemas/MediaSchema.{cpp,h}` — Intent/Commit/GetToken Request/Response 6 struct
- `MainServer/Routers/Pill.cpp` `handle_identify` 운영 모드 — photo_storage 조회 → GET 토큰 발급 → Vision PC `detect_pills_remote` 호출 → 응답 → DUR 페어 매칭
- `MainServer/Services/Inference/VisionInferenceClient` — `detect_pills_remote(RemoteDetectParams)` 추가 (페이로드: photo_id + storage_url + get_token + mime)
- `MainServer/Database/Migrations/002_photo_storage_intent.sql` — `status`(ENUM PENDING/READY/EXPIRED/FAILED) + `expires_at` + `committed_at` 컬럼 + 인덱스

#### 메인서버 — 청소 잡 (PENDING expires_at → EXPIRED)
- `MainServer/Main.cpp` — Drogon `app().getLoop()->runEvery(interval, ...)` 등록
- `MainServer/Config` — `storage_cleanup_interval_seconds` 추가 (env `MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL`, 기본 300s, 0=비활성)

#### 메인서버 — 식약처·Report·DUR·Config
- `MainServer/Scripts/import_pdma.py` — 식약처 CSV/Excel 일괄 적재 (UPSERT, UTF-8/CP949 자동, .xlsx 지원)
  - 샘플: `sample_pdma_pill.csv`, `sample_pdma_overview.csv`, `sample_pdma_dur.csv`
- `MainServer/Services/Report/ReportHtmlRenderer.cpp` — A4 인쇄용 HTML (`@page` + `@media print`, XSS escape, 인쇄 색감)
- `MainServer/Services/Report/ReportPdfRenderer.cpp` — wkhtmltopdf subprocess (fork+execvp, mkstemps race-free, RAII 정리)
- `MainServer/Services/Dur/DurQueryEngine.cpp` — 양방향 페어 SQL + dedup, `prohibit_reason` 그대로 인용
- `MainServer/Services/Pdma/PdmaApiClient.cpp` — e약은요 HTTPS 호출 (`getDrbEasyDrugList`), 비동기 콜백
- `MainServer/Services/Pdma/DrugOverviewCache.cpp` — 캐시 hit/miss + UPSERT
- `MainServer/Config.cpp` — `apply_json_file` (jsoncpp), `apply_env_vars`, `validate` 3단계 분리. 환경변수 항상 최우선.
- `MainServer/config.sample.json` — 시크릿 제외 모든 설정 예시
- `MainServer/Routers/Report.cpp` `format=pdf` 분기 — WorkerPool 위임, `Content-Disposition` 헤더

#### 신규 환경변수
- `MEDIBRIDGE_STORAGE_BASE_URL` — 보관 PC URL (기본 `http://10.10.10.122:8004`)
- `MEDIBRIDGE_STORAGE_SECRET` — 보관 PC 토큰 시크릿 (JWT 와 분리, 32+ 바이트)
- `MEDIBRIDGE_STORAGE_TOKEN_TTL` — 토큰 만료 (기본 300s)
- `MEDIBRIDGE_STORAGE_MAX_BYTES` — 업로드 최대 (기본 10MB)
- `MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL` — 청소 잡 인터벌 (기본 300s, 0=비활성)

#### Verified — end-to-end 풀 시나리오
- 메인서버 + 보관 PC 동시 가동 → 로그인 → intent → PUT → commit → 디스크 파일 존재 + MD5 무결성 일치
- 보안: 만료 토큰 401 / PUT 토큰으로 GET 401 / jti mismatch 403
- Report PDF: A4 1페이지, 64KB, Qt 5.15 producer, 한국어 폰트 자동
- 청소 잡: PENDING 만료 row → EXPIRED 마킹 + 로그 출력
- 식약처 CSV: pill 4건 / overview 4건 / dur 2건 UPSERT 성공

#### Docs
- `Docs/Architecture/MediBridge_Architecture_MVP.pptx` — 10 슬라이드 아키텍처 도해 (격자 + 엘보 화살표 + ⑥ Client/Vision↔Storage 데이터 평면)
- `Docs/system_prompt.md` — 시스템 운영 명령어 정본 (시작/종료/로그/portproxy/트러블슈팅)
- `MainServer/Scripts/medibridge-portproxy.ps1` — 8001 + 8004 일괄 포워딩
- `MainServer/Scripts/medibridge-up.sh` — `MEDIBRIDGE_STORAGE_*` 환경변수 추가

---

### Added (2026-05-07 추가)
- `MainServer/Services/Auth/PasswordHasher.cpp` 실 구현 (PBKDF2-SHA256 / OpenSSL only)
- `MainServer/Services/Auth/JwtIssuer.cpp` 실 구현 (HS256 표준 JWT / OpenSSL HMAC)
- `MainServer/Services/Auth/Authenticator.cpp` + `UserManager.cpp` 실 DB CRUD
- `MainServer/Services/Inference/InferenceClientCommon.cpp` Drogon HttpClient 실 호출 (post_json/post_file)
- `MainServer/Services/Inference/InferenceClient.cpp` LLM/Vision 카테고리별 Common 분리 (10.10.10.120 / 10.10.10.128)
- `MainServer/Config` 에 `MEDIBRIDGE_INFERENCE_LLM_BASE` / `..._VISION_BASE` 환경변수 + 접근자
- `Docs/시스템 흐름 정리본_ver3.md` — 단일 정본화 통합 (시스템_연결구조 v2.2 + DB ERD v4 + Pill v0.3 + TestMode + Auth + 데이터보관 PC + ChromaDB)
- `Docs/Old/시스템 흐름 정리본_ver2_2026-05-07.md` (이동)

### Changed (2026-05-07)
- 모든 라우터(Pill/History/Media/Report/Speech) 의 인증 → `JwtIssuer::extract_user_id_from_header` 로 일괄 교체 (MockAuth 제거, CMake 빌드에서 제외)
- `Routers/Auth.cpp` — 실 Authenticator/UserManager/JwtIssuer 사용 (signup/login/logout)

### Verified
- **WSL Ubuntu 24.04 + Drogon 1.8.7 빌드 성공** (`apt install libdrogon-dev libjsoncpp-dev libpq-dev libsqlite3-dev libhiredis-dev libc-ares-dev libyaml-cpp-dev`).
- 스모크 테스트 통과 — `/health` 200 (`status:ok, test_mode_active`), `/v1/auth/login` 시드 사용자 로그인 성공, `/v1/pill/pool` 시드 풀 4건 반환, `/v1/pill/onboarding/normalize "타이레놀 등록할게"` → 3 후보 NEED_DISAMBIGUATION + tts_text + choice_token 정상.
- 새 스크립트: `MainServer/Scripts/smoke_setup.sh` (DB·시드 일괄), `smoke_run.sh` (서버 부팅 + 4개 엔드포인트 호출).



### Docs
- (예정) 시스템 흐름 정리본 v3 — `시스템_연결구조_ver2.md` v2.2 통합
- (예정) DB ERD v5 — 가명화 마이그레이션 SQL 보강
- (예정) Install/InferenceServerInstall.md 분리 — LLM/Vision 별 설치 매뉴얼

### Code
- (예정) Production 모드 Auth — bcrypt/PBKDF2 PasswordHasher + jwt-cpp JwtIssuer 실구현
- (예정) MainServer Config — `INFERENCE_LLM_BASE` / `INFERENCE_VISION_BASE` 환경변수 분리
- (예정) Pseudonymization 익명화 systemd timer 적용

---

## [2026-05-07] — MainServer TestMode 도입 (Tier 1 동작 코드)

### Docs
- **Added** — `Docs/TestMode.md` v0.1 — 설계서 (DB-backed 더미 정책, seed prefix 규칙, 서비스 분기 패턴)
- **Changed** — `Docs/Install/MainServerInstall.md` v0.1 → **v0.2** — Python/FastAPI → Drogon C++, 스키마/시드 적용, `MEDIBRIDGE_TEST_MODE` 환경변수, 시드 로그인 검증 절차

### Code (MainServer)
- **Added** — `Database/Migrations/001_init_schema.sql` — DB ERD v4 전체 스키마 (`users`, `pseudonym_map`, `pill_identification`, `dur_interaction_cache`, `user_medication_pool`, `medication_intake_logs`, `user_reports`, `photo_storage` 등)
- **Added** — `Database/Seeds/dev_seed.sql` — 테스트 사용자 2 + 가명 매핑 + 식약처 더미 약 10 + DUR 페어 2 + 풀 4 + 이력 6
- **Added** — `Services/TestMode/MockAuth.h/.cpp` — TestMode mock JWT (`MEDIBRIDGE_TEST.<base64-payload>.<sig>`) 발급/검증
- **Changed** — `Config.h/.cpp` — `MEDIBRIDGE_TEST_MODE` env 읽기 + production 환경 강제 비활성
- **Changed** — `Database/Connection.cpp` — Drogon `DbClient::newMysqlClient` 실 초기화 + `ping()`
- **Changed** — `Schemas/PillSchema.h` v0.2 → **v0.3** — `OnboardingNormalize*` / `OnboardingState` / `OnboardingCandidate/Question/Confirmation/Resolved/PrevSelection` 추가
- **Added** — `Schemas/PillSchema.cpp` (신규) + `AuthSchema.cpp` + `SpeechSchema.cpp` + `MediaSchema.cpp` + `HistorySchema.cpp` + `ReportSchema.cpp` + `HealthSchema.cpp` — JSON 직렬화 일괄 구현
- **Renamed** — `SpeechSchema::GuidanceMessage` → `SpeechGuidance` (PillSchema 의 `GuidanceMessage` 와 ODR 충돌 방지)
- **Changed** — `Routers/Pill.cpp` — `/v1/pill/identify`, `/v1/pill/identify/narrow`, `/v1/pill/onboarding/normalize`, `/v1/pill/pool` (GET/POST), `/v1/pill/pool/{id}` (DELETE), `/v1/pill/pool/all` (DELETE) 모두 동작 구현 (TestMode = DB seed 응답 / 풀 CRUD = 실 동작)
- **Changed** — `Routers/Pill.h` — `/v1/pill/identify/narrow`, `/v1/pill/onboarding/normalize` 라우트 추가
- **Changed** — `Routers/Auth.cpp` — signup/login/logout 동작 구현 (TestMode 평문 비교 fallback + mock JWT 발급)
- **Changed** — `Routers/Speech.cpp` — TestMode 의도 분류 더미 (키워드 룰) + 인젝션 1차 필터
- **Changed** — `Routers/Media.cpp` — multipart 수신 + `photo_storage` 메타 INSERT (실 파일 discard, 가짜 storage_path)
- **Changed** — `Routers/History.cpp` — 실 DB CRUD (record / list + 페이지네이션 + time_slot 자동 분류)
- **Changed** — `Routers/Report.cpp` — 실 DB 집계 + 식약처 e약은요 인용 (LLM 변환 X) + JSON/HTML 출력
- **Changed** — `Routers/Monitoring.cpp` — `/health` (DB ping) + `/metrics` 기본 응답
- **Changed** — `Main.cpp` — TestMode 플래그 시작 로그 라인 추가
- **Changed** — `CMakeLists.txt` — Schemas .cpp 7개 + MockAuth.cpp 등록, OpenSSL::Crypto 링크

### 사유 (Why)
- 사용자 요청: 추론 서버(LLM/Vision PC)·데이터 보관 PC 가 아직 가동되지 않은 시점에 메인 서버만으로 클라이언트가 진짜처럼 동작하는 서비스를 체험할 수 있어야 함.
- 사용자 결정: **DB-backed 더미** — 코드에 박힌 const 가 아니라 MariaDB seed 행. 운영 전환 시 row 만 갈아끼우면 끝. SQL 인젝션 방어·prepared statement·가명화 FK 체인이 실전 검증됨.
- 운영 모드 강제 비활성: `MEDIBRIDGE_ENV=production` 환경에서 `MEDIBRIDGE_TEST_MODE` 가 켜져 있어도 강제로 false 처리 (Config.cpp). 시연 / 발표 시 더미가 노출되는 것 방지.

---

## [2026-05-07] — Onboarding RAG round-trip + TTS 어댑터 + ChromaDB 채택 (A+B+C)

### 기술 선정
- **벡터 DB: ChromaDB** (사용자 확정) — Onboarding RAG 약명 정규화 + 일반 안내 보조에 사용. LLM PC `10.10.10.120` 에 동거(`pip install chromadb`). 운영 단순성 + Python 친화 + 단일 PC 적합 (FAISS·Qdrant·Pinecone 대비 경량).
- 영향 문서: `PillApi.md` §3 백엔드 처리 흐름, `시스템_연결구조 §2.8`, `Install/InferenceServerInstall.md` 항목 11, `아이템_ver3.md §6.3`.

### Docs (사용자 확정 사항 반영)
- **Added** — `Docs/Api/PillApi.md` v0.2 → **v0.3** — 신규 `POST /v1/pill/onboarding/normalize` 엔드포인트
    - 음성 등록 시 의료용어를 모르는 사용자를 위한 약명 정규화 RAG round-trip
    - stateless 다회 round-trip 허용 (`max_rounds=5` 기본, `max_input_tokens=200`, rate_limit 명시)
    - `state` 3분기: `NEED_DISAMBIGUATION` / `RESOLVED` / `NOT_FOUND`
    - 응답에 `tts_text` 필드 (TTS 합성용, 숫자·약어 한글 풀어쓰기) — 클라 자체 TTS 우선
    - 표현 톤 정책 준수 (e약은요 본문 인용, 단정 표현 X)
    - Schemas/ 매핑: `OnboardingNormalizeRequest/Response`, `OnboardingCandidate/Question/Confirmation`
- **Changed** — `Docs/Api/ApiOverview.md` v0.3 — PillApi 행 v0.3 으로 갱신, Schemas/ 매핑 표 추가
- **Changed** — `Docs/시스템_연결구조_ver2.md` v2.1 → **v2.2**
    - **§2.8 Onboarding RAG round-trip 흐름** 신규 (9단계 sequence + 시각화 도식)
    - **§2.9 TTS 어댑터 정책** 신규 (3방식 데이터량 비교 + `ITtsProvider` 어댑터 인터페이스 + fallback 발동 조건)
    - §2.1 음성 안내 행 명확화 (클라 우선 + 메인 서버 fallback)
- **Changed** — `Docs/프로토콜_ver2.md` v2.0 → **v2.1**
    - **§3.3 TTS 정책 신설** — 클라 자체 TTS 우선 + 메인 서버 fallback 어댑터
    - 두 onboarding 엔드포인트 통합 안내 (단일 `/v1/pill/onboarding/normalize`)
- **Changed** — `Docs/아이템_ver3.md` v3.0 → **v3.1**
    - §6.1 음성 인식 등록 — Onboarding RAG round-trip 명세 + API 참조
    - §6.3 다층 안전 메커니즘 — TTS 어댑터 정책 추가
- **Changed** — `Docs/기획서_ver3.md` v3.1 → **v3.2**
    - §8 어필 포인트에 TTS 어댑터 + Onboarding RAG round-trip 추가

### 사유 (Why)
- 사용자 발견: 기존 §2.7 음성 텍스트 추론 흐름은 일반 라우팅만 다루고 **약명 정규화 + 분기 질문 + TTS 안내 round-trip 의 세부 단계가 누락**. PillApi 의 `POST /v1/pill/pool` 은 이미 정규화된 `item_code` 를 받는 구조라 그 앞 단계의 RAG 검색·분기 질문 엔드포인트 부재.
- 사용자 결정: **다회 round-trip 허용** (의료용어 모르는 사용자가 단일 발화로 약 특정 어려움). 단, 무한 반복 방어를 위해 `max_rounds`·`max_input_tokens`·rate_limit 한계 명시.
- 사용자 결정: **TTS 클라 자체 TTS 우선**. 데이터량 우려에 답해 비교 표 추가 — 메인 서버 TTS 는 사진 1장(1~3MB)보다 작지만 **클라 TTS 대비 100~300배 증가** + 클라우드 비용·지연. 어댑터 인터페이스(`ITtsProvider`) 로 PoC 시점 평가 후 결정 가능 구조.

---

## [2026-05-07] — 시스템_연결구조 v2.1 + ApiOverview v0.3 + 기획서 v3.1 (PC 구성 확정)

### Docs (사용자 확정 사항 반영)
- **Changed** — `Docs/시스템_연결구조_ver2.md` v2.0 → **v2.1** (in-place 갱신)
    - **§1.2 IP·호스트 매트릭스** 신규 — 메인 10.10.10.97 / 데이터보관 10.10.10.122 / LLM 10.10.10.120 / Vision 10.10.10.128 / 클라 로컬
    - **§1.1 디바이스 표 재분배** — ⑤ 추론 PC + ⑥ 학습 서버 → ⑤ LLM PC (학습+추론 동거) + ⑥ Vision PC (학습+추론 동거). 카테고리별 분리, 네트워크 분리 가능 설계
    - **데이터 보관 PC = 사진 전용** 명시 (음성 원본 폰 외부 비송신 → DB·DataStorage 모두 보관 X. 추출 구조화 데이터만 메인 MariaDB 보관 OK)
    - **§2.6 Vision 학습 일괄 다운로드 흐름** 신규
    - **§2.7 음성 텍스트 추론 흐름** 신규
    - **§5 v2.x vs 기존** 표 갱신, **§8.3 PII 분류** 음성 항목 3행 분리
- **Changed** — `Docs/Api/ApiOverview.md` v0.2 → **v0.3**
    - §2 호스트 표 IP 확정 + DataStoragePC 행 추가 + InferenceServer 행 LLM/Vision 분리
    - 추론 라우팅 책임 = 메인 서버 (텍스트/이미지 분기) 명시
    - 엔드포인트 자체는 변경 없음 (호환 변경)
- **Changed** — `Docs/기획서_ver3.md` v3.0 → **v3.1**
    - §2.1 통합 시스템 구성: 5대 + 폰 + IP 확정
    - §3 사전 자원 요청: PC 5대 + 폰 분리 환경 확정, LAN 대역 명시
    - §6 시스템 아키텍처 핵심 요지: LLM·Vision 분리, 데이터 보관 PC 컨트롤/데이터 평면, 학습 일괄 PULL, 음성 원본 비송신
    - §7 To-Do: "5대 + 폰 + 가명화 구조 결정" → 완료(v2.1) 표기
    - §8 어필 포인트: "MLOps 4대 분리(검토)" → "5대 + 폰 분리 + 카테고리별 추론 + 학습+추론 동거"
    - §9 MVP PC 구성 표 갱신

### 사유 (Why)
- 사용자 확정: 학습+추론 통합 운용은 운영상 비표준이지만 **여유 PC 부족**으로 동거 채택. 단, 포트·네트워크 정책 분리해 향후 PC 증설 시 즉시 분리 가능 (네트워크 분리 가능 설계).
- 사용자 확정: **추론을 LLM/Vision 카테고리별로 분리** — 워크로드 특성(GPU 사용 패턴·메모리 프로파일)이 달라 합치면 상호 간섭. 분리해 두면 PC 증설·교체 가능성 모두 열림.
- 사용자 확정: 데이터 보관 PC 는 **사진 전용**. 음성은 폰 STT → 텍스트만 흐르고 원본 비송신/비저장. 텍스트에서 추출한 구조화 데이터(약명·개수)는 메인 MariaDB 정상 보관.
- 사용자 확정: 학습 데이터셋은 **학습 시점 일괄 다운로드** (Vision PC ← Data Storage PC). 학습+추론 동거이므로 모델 가중치 SCP 단계 생략.

---

## [2026-05-07] — Docs 정리 + 가명화 정책 도입 + API v0.2

### Docs (정리·일원화)
- **Added**
    - `Docs/CHANGELOG.md` (본 파일) — 코드·문서 통합 변경 이력
    - `Docs/README.md` — Docs 인덱스
    - `Docs/기획서_ver3.md` — Docs 정리 반영 (§5 시장성·시나리오 절 제거 → 아이템 v3 일원화, §6 시스템 아키텍처 시스템 흐름 정리본 참조로 단순화, §9 MVP 범위 표 요지만 유지). 약 360행 → 약 230행
    - `Docs/아이템_ver3.md` — Docs 정리 반영 (모듈 구조 표 / MVP vs 확장 매트릭스 / 기술 스택 상세 제거). 시장성·시나리오·핵심 설계 원칙에 집중. 약 300행 → 약 200행
    - `Docs/시스템_연결구조_ver2.md` — 5대 PC + 폰 + Data Storage PC 구조 제안 + §8 가명화 정책
    - `Docs/DB_ERD_ver4.md` — `pseudonym_map` 테이블 도입(단일-컷 매핑), `user_id` FK 를 `anonymous_id` FK 로 전환, `photo_storage` 테이블 추가
    - `Docs/Api/PillApi.md` v0.2 — 목업 분석 반영 (`efficacy_text`/`usage_text`/`user_category` 추가, 신규 `POST /v1/pill/identify/narrow` 단계별 좁히기)
    - `Docs/Api/ApiOverview.md` v0.2 — PillApi v0.2 변경 반영
- **Moved (Old/)**
    - `Docs/Old/기획서_ver2_2026-05-07.md`
    - `Docs/Old/아이템_ver2_2026-05-07.md`
    - `Docs/Old/시스템_연결구조_ver1_2026-05-07.md`
    - `Docs/Old/DB_ERD_ver3_2026-05-07.md`
    - `Docs/Old/Api/...` (v0.1 PillApi, ApiOverview)
- **Reason**
    - Docs 폴더 내 **중복 기록 통합** + **단일 책임 원칙(SRP)** 적용 — 모듈 구조표는 기획서, MVP/확장 매트릭스는 시스템 흐름 정리본, 시장성·시나리오는 아이템으로 분리.
    - 목업 「메디브릿지 목업.pptx」 분석 결과를 API/DB 에 반영.
    - PIPA·GDPR 대응 가명화 패턴(단일-컷 매핑) 도입.

### Code (commit `50293cb`, `62feb1d`)
- **Added** — 가명화 정책 (DB ERD v4) — `pseudonym_map` 테이블, `anonymous_id` FK 전환, 익명화 SQL 절차
- **Added** — Pill API 단계별 좁히기 (`POST /v1/pill/identify/narrow`) 상태없음(stateless) 다단계 식별 흐름
- **Changed** — `MainServer/Schemas/PillSchema.h` v0.2 — `classification_name`/`efficacy_text`/`usage_text`/`user_category` 추가, `FallbackAction` enum 신설, `NarrowAttributes`/`NarrowDownRequest`/`NarrowDownResponse`/`NarrowQuestion` 구조체 추가

### Build / Infra (commit `beb8439`, `1dbb219`)
- **Fixed** — `.gitignore` 인라인 주석으로 인한 패턴 매칭 실패 → 인라인 주석 제거 + `**/build/`, `**/build-*/`, `**/.qtcreator/` 패턴 강화

### Client (commit `d57afba`)
- **Added** — PC 트리거 폰 화면 캡쳐 — '📸 촬영 + 식별' 버튼 (`adb exec-out screencap -p` 활용, 폰 갤러리 비저장)

### Client — Refactor·Fix (commit `a620371`, `0db73e8`)
- **Fixed** — `Monitoring/HealthChecker.cpp` Pattern B 분리 누락 보정 (`api_client_->monitoring().check_health(...)`)
- **Fixed** — `AdbDeviceMonitor.cpp` `QRegularExpression` 불완전 타입 → `#include <QRegularExpression>` 추가
- **Docs (Install)** — `Install/ClientPcInstall.md` 트러블슈팅 + Qt Creator 빌드·실행 절차

### Security (commit `8bf2cac`)
- **Fixed** — `std::gmtime()` thread-safety 이슈 3 곳 → `Utils/TimeUtil.h` 헬퍼(`gmtime_r`/`gmtime_s`) 적용
- **Added** — JWT 시크릿 길이·엔트로피 검증
- **Added** — URL 인코딩, body 크기 제한, Prepared Statement 강제 등 OWASP 대응

### Client — MVVM (commit `235e861`)
- **Changed** — Frontend(QML) ↔ Backend(C++) 명확 분리 (Q_PROPERTY/Q_INVOKABLE 기반)

---

## [2026-05-06] — 영역별 골격 + 패턴 B/A2 리팩터링

### Code (commit `4cb89de`, `6c7630e`, `8748026`)
- **Refactored** — InferenceServer P1 분리 (Llm + Vision 카테고리별 클래스)
- **Refactored** — MainServer Pattern B + A2 적용 (Services/ 5클래스 → 16클래스, 카테고리별 폴더 분리)
- **Refactored** — Client Pattern B 적용 (카테고리별 클래스 분리)

### Code (commit `47a3545`)
- **Added** — 영역별 골격: Client + MainServer + InferenceServer + TrainingServer

### Docs (commit `f9c8257`)
- **Added** — REST API 명세서 8종 v0.1 (AuthApi, HistoryApi, ReportApi, MediaApi, SpeechApi, PillApi, MonitoringApi + ApiOverview)

### Infra (commit `97f5c8b`)
- **Added** — 프로젝트 초기 구조: Git 저장소 · `.gitignore` · README · 영역 폴더 골격

### Docs (사전 작성)
- **Added** — `기획서_ver2.md`, `아이템_ver2.md`, `시스템 흐름 정리본_ver2.md`, `요구사항_분석서_ver2.md`, `프로토콜_ver2.md`, `개발계획서_ver2.md`, `DB_ERD_ver2.md`
- **Reason** — 통신 표기 통일, 안드로이드 S24 + 온디바이스 STT 명시, 6장 그림 갱신, 8장 어필 포인트 보강

---

## [2026-04-30] — 초안

### Docs
- **Added** — `기획서_ver1.md`, `아이템_ver1.md`, `시스템 흐름 정리본_ver1.md`, `요구사항_분석서_ver1.md`, `프로토콜_ver1.md`, `개발계획서_ver1.md`, `DB_ERD_ver1.md`
- **Reason** — 프로젝트 초안 (3인 팀, 16일 일정 2026-04-27 ~ 2026-05-12)

---

## 변경 사유 기록 정책

| 분류 | 정본 | 본 파일 기록 |
| --- | --- | --- |
| 코드 변경 | git commit 메시지 | 요약 한 줄 (커밋 해시 명시) |
| 문서 신규 | 본 파일 + 해당 문서 「변경 이력」 절 | 사유·이전 버전 위치 |
| 문서 갱신 | 해당 문서 「변경 이력」 절 | 갱신 사유 한 줄 |
| Old/ 이동 | 본 파일 | 이동 사유 + 후속 문서 위치 |
| API 버전 | 해당 API 명세서 「변경 이력」 + ApiOverview | 호환·비호환 여부 |

> **호환성 표기**: API 는 `/v1/` prefix 내에서 v0.x 단위 호환 변경(필드 추가만 허용)·비호환 시 `/v2/` 분기.
