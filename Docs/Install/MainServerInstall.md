# 메인(운용) 서버 설치 매뉴얼 (Ubuntu 24.04)

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 메인 서버 PC 설치 매뉴얼 |
| **대상 OS** | Ubuntu 24.04 LTS |
| **호스트** | `10.10.10.97` ([시스템_연결구조 v2.2 §1.2](../시스템_연결구조_ver2.md)) |
| **역할** | Drogon C++ HTTP, MariaDB(사용자/약 풀/이력/식약처 데이터/photo_storage), DUR 안내, 인젝션 방어 |
| **버전** | v0.3 (사진 흐름 ⑤+⑥ + Storage 시크릿 + 청소 잡) |
| **개정일** | 2026-05-13 |
| **이전 버전** | v0.2 (2026-05-07) |

> ⭐ **v0.3 변경 핵심**: ① `MEDIBRIDGE_STORAGE_SECRET` 신규 환경변수 (보관 PC 토큰 — JWT 시크릿과 분리, 32+ 바이트) ② `MEDIBRIDGE_STORAGE_BASE_URL` (기본 10.10.10.122:8004) ③ 청소 잡 인터벌 환경변수 ④ 마이그레이션 002 (`photo_storage` status/expires_at/committed_at) ⑤ `config.json` 파싱 (jsoncpp) 지원 ⑥ wkhtmltopdf 의존성 추가 (Report PDF) ⑦ `import_pdma.py` (식약처 CSV 적재) — `python3-pymysql` 의존성
>
> ⭐ **v0.2 변경 핵심**: ① Python/FastAPI → **Drogon C++** 으로 갱신 ② **TestMode** 도입 (`MEDIBRIDGE_TEST_MODE`) — 추론·파일저장 우회 + DB seed 사용 ③ 스키마 마이그레이션·시드 적용 절차

---

## 1. 설치 항목 체크리스트

| # | 도구 / 컴포넌트 | 용도 | 상태 |
| --- | --- | --- | --- |
| 1 | build-essential, cmake (≥3.16), git | C++ 빌드 | ⏳ |
| 2 | **Drogon 1.9+** (HTTP 프레임워크 + ORM) | REST API 백엔드 | ⏳ |
| 3 | **MariaDB 11.x** | 메인 DB | ⏳ |
| 4 | MariaDB Connector/C (`libmariadb-dev`) | Drogon ORM ↔ MariaDB | ⏳ |
| 5 | OpenSSL (`libssl-dev`) | TestMode mock JWT SHA256 + Drogon 의존 | ⏳ |
| 6 | (운영용) libbcrypt 또는 OpenSSL EVP_PBKDF2 | 비밀번호 해시 | ⏳ |
| 7 | (운영용) jwt-cpp (header-only) | JWT 발급·검증 | ⏳ |
| 8 | `nginx` (선택) | 리버스 프록시 | ⏳ |
| 9 | `ufw` 방화벽 | 외부 노출 포트 통제 | ⏳ |
| 10 | systemd 서비스 등록 | 자동 시작 | ⏳ |

---

## 2. 사전 준비

- Ubuntu 24.04 LTS 설치
- LAN `10.10.10.0/24` 대역에 메인 서버 PC 가 `10.10.10.97` 로 고정
- sudo 권한 사용자

---

## 3. 설치 절차

### 3.1 시스템 패키지

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y \
    build-essential cmake git pkg-config \
    libssl-dev libmariadb-dev libjsoncpp-dev \
    uuid-dev zlib1g-dev libbrotli-dev \
    libdrogon-dev \
    wkhtmltopdf \
    python3-pymysql
```

- `wkhtmltopdf` — Report PDF 변환 (`format=pdf`)
- `python3-pymysql` — 식약처 CSV 적재 도구 (`Scripts/import_pdma.py`)
- `libdrogon-dev` — Ubuntu 24.04 에서 apt 로 가능 (3.2 의 소스 빌드 대신 빠른 옵션)

### 3.2 Drogon 설치 (소스 빌드 권장)

```bash
git clone https://github.com/drogonframework/drogon.git
cd drogon
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../..
```

### 3.3 MariaDB 설치 + 사용자 생성

```bash
sudo apt install -y mariadb-server mariadb-client
sudo systemctl enable --now mariadb
sudo mysql_secure_installation   # root 비밀번호·익명 사용자·원격 root 등 정리

sudo mysql -u root -p <<'SQL'
CREATE DATABASE medibridge CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'medibridge_app'@'localhost' IDENTIFIED BY '<강력한 비밀번호>';
GRANT SELECT, INSERT, UPDATE, DELETE ON medibridge.* TO 'medibridge_app'@'localhost';
FLUSH PRIVILEGES;
SQL
```

### 3.4 스키마 + 시드 적용

```bash
# 1) 스키마 (한 번만)
mysql -u medibridge_app -p medibridge < MainServer/Database/Migrations/001_init_schema.sql

# 2) 마이그레이션 002 — photo_storage 에 status/expires_at/committed_at 추가 (v0.3)
mysql -u medibridge_app -p medibridge < MainServer/Database/Migrations/002_photo_storage_intent.sql

# 3) 더미 데이터 (TestMode 사용 시에만)
mysql -u medibridge_app -p medibridge < MainServer/Database/Seeds/dev_seed.sql
```

또는 한 번에:
```bash
sudo bash MainServer/Scripts/smoke_setup.sh
```

> 더미 식별 prefix: `999800xxx` (식약처 실 코드와 충돌 X), `test_user_xxx`, `anon_test_xxx`. 상세는 [Docs/TestMode.md](../TestMode.md) §2.

### 3.5 메인 서버 빌드

```bash
cd MainServer
mkdir build && cd build
cmake ..
make -j$(nproc)
```

빌드 결과물: `build/MediBridgeMainServer`

### 3.6 환경 변수 설정 (v0.3)

```bash
# /etc/environment 또는 systemd Environment= 설정
export MEDIBRIDGE_DB_PASSWORD='<3.3 에서 정한 비밀번호>'
export MEDIBRIDGE_JWT_SECRET='<32자 이상 랜덤 문자열>'       # production 필수
export MEDIBRIDGE_STORAGE_SECRET='<32자 이상, JWT 와 다른 시크릿>'    # ⭐ v0.3 신규 — 보관 PC 토큰
export MEDIBRIDGE_STORAGE_BASE_URL='http://10.10.10.122:8004'        # ⭐ v0.3
export MEDIBRIDGE_STORAGE_TOKEN_TTL='300'                            # PUT/GET 토큰 만료(초)
export MEDIBRIDGE_STORAGE_MAX_BYTES='10485760'                       # 10MB
export MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL='300'                     # PENDING 청소 잡 인터벌(초). 0=비활성
export MEDIBRIDGE_PDMA_KEY='<식약처 OpenAPI 서비스 키>'              # 운영 시 lazy 호출용
export MEDIBRIDGE_ENV='development'                                   # 또는 production

# TestMode 활성 (개발 단계 권장)
export MEDIBRIDGE_TEST_MODE='true'
```

> ⚠ **production 환경(`MEDIBRIDGE_ENV=production`)에서 `MEDIBRIDGE_TEST_MODE` 는 강제 비활성**됩니다 (Config.cpp 검증). 보안상 의도된 동작.
> ⚠ **`MEDIBRIDGE_STORAGE_SECRET` 은 JWT 시크릿과 절대 동일 X.** 보관 PC 와 같은 값을 공유해야 PUT/GET 토큰 검증 가능.

### 3.6-B `config.json` 사용 (선택 — v0.3 신규)

비밀 정보(시크릿·DB 비밀번호)는 환경변수에 두되, 그 외 설정은 `config.json` 으로 관리 가능. 샘플: `MainServer/config.sample.json`.

```json
{
  "server":    { "port": 8001, "thread_num": 0, "worker_pool_size": 4 },
  "db":        { "host": "127.0.0.1", "port": 3306, "user": "medibridge_app", "name": "medibridge", "pool_size": 10 },
  "inference": { "llm_base": "http://10.10.10.128:8002", "vision_base": "http://10.10.10.120:8003", "request_timeout_ms": 5000 },
  "storage":   { "base_url": "http://10.10.10.122:8004", "token_ttl_seconds": 300, "max_bytes": 10485760, "cleanup_interval_seconds": 300 },
  "jwt":       { "expire_seconds": 86400 },
  "pdma":      { "api_base_url": "https://apis.data.go.kr" },
  "test_mode": false
}
```

우선순위: **기본값 < `config.json` < 환경변수** (환경변수가 항상 최우선).

### 3.7 실행

직접 실행:
```bash
cd MainServer
./build/MediBridgeMainServer

# 시작 로그에 다음이 보여야 정상:
# [Config] MEDIBRIDGE_TEST_MODE = TRUE  → 추론·파일저장 우회, DB seed 사용
# [DB] connected — 127.0.0.1:3306/medibridge (pool=10)
# [Main]   - 포트: 8001
# [Main]   - 청소 잡 등록 — 300초 간격 (PENDING expires_at 경과 → EXPIRED)
```

또는 부팅 스크립트 (시연용):
```bash
bash MainServer/Scripts/medibridge-up.sh
```

→ MariaDB 자동 시작 + 환경변수 + 백그라운드 실행 + `/health` ping 부팅 확인.

### 3.8 동작 확인

```bash
# Health
curl http://10.10.10.97:8001/health

# 시드 사용자 로그인 (test1234)
curl -X POST http://10.10.10.97:8001/v1/auth/login \
     -H "Content-Type: application/json" \
     -d '{"email":"test@medibridge.local","password":"test1234"}'

# 응답에서 access_token 받아 약 풀 조회
TOKEN="<위 응답의 access_token>"
curl -H "Authorization: Bearer $TOKEN" http://10.10.10.97:8001/v1/pill/pool
```

### 3.9 방화벽

```bash
sudo ufw allow 8001/tcp comment 'MediBridge MainServer'
sudo ufw enable
```

### 3.10 systemd 서비스 등록 (선택)

```bash
sudo tee /etc/systemd/system/medibridge-main.service <<'UNIT'
[Unit]
Description=MediBridge Main Server (Drogon)
After=mariadb.service
Wants=mariadb.service

[Service]
Type=simple
User=medibridge
WorkingDirectory=/opt/medibridge/MainServer
EnvironmentFile=/etc/medibridge.env
ExecStart=/opt/medibridge/MainServer/build/MediBridgeMainServer
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
UNIT

sudo systemctl daemon-reload
sudo systemctl enable --now medibridge-main
```

---

## 4. 설치 완료 체크리스트 (v0.3)

- [ ] Ubuntu 24.04 + sudo 사용자 준비
- [ ] LAN 고정 IP `10.10.10.97`
- [ ] Drogon 1.8+ 설치 (apt 또는 소스 빌드)
- [ ] `wkhtmltopdf` 설치 (Report PDF 용)
- [ ] `python3-pymysql` 설치 (식약처 CSV 적재)
- [ ] MariaDB 11 + `medibridge` DB + `medibridge_app` 사용자
- [ ] 스키마 적용 (`001_init_schema.sql`)
- [ ] **마이그레이션 002 적용** (`002_photo_storage_intent.sql`) ⭐ v0.3
- [ ] 시드 적용 (`dev_seed.sql`) — TestMode 사용 시
- [ ] 환경변수 설정 — `MEDIBRIDGE_DB_PASSWORD`, `MEDIBRIDGE_JWT_SECRET`, **`MEDIBRIDGE_STORAGE_SECRET`** ⭐ v0.3, `MEDIBRIDGE_TEST_MODE`
- [ ] **데이터 보관 PC 와 `MEDIBRIDGE_STORAGE_SECRET` 동일성 확인** ⭐ v0.3
- [ ] `MainServer` CMake 빌드
- [ ] 실행 → Health 200 응답 + `[Main] - 청소 잡 등록 — N초 간격` 로그 보임 / 시드 로그인 성공
- [ ] PDF 보고서 — `GET /v1/report/generate?format=pdf` 200 + `application/pdf` 응답
- [ ] 사진 흐름 — `/v1/media/intent` → 보관 PC PUT → `/v1/media/commit` 풀 시나리오 통과
- [ ] ufw 8001/tcp 허용

---

## 5. 트러블슈팅

| 증상 | 원인 / 해결 |
| --- | --- |
| `[DB] FATAL — DbClient 생성 실패` | DB 비밀번호 / 호스트 / 사용자 권한 확인. `mysql -u medibridge_app -p medibridge` 로 직접 접속 검증. |
| `Drogon::Drogon` 못 찾음 | `sudo ldconfig` 후 `find_package(Drogon)` 가능. 또는 `cmake -DDrogon_DIR=...` |
| `INVALID_TOKEN` 만 반환 | `MEDIBRIDGE_JWT_SECRET` 가 서버 재시작과 다르게 설정됨. 토큰 재발급 필요. |
| TestMode 인데 Vision/LLM 호출 시도 | `MEDIBRIDGE_TEST_MODE=true` 환경변수 확인 + 시작 로그의 `[Config] MEDIBRIDGE_TEST_MODE = TRUE` 라인 확인. |
| seed 적용 안 됨 (FK 위반) | `001_init_schema.sql` 먼저 적용 후 `dev_seed.sql` 적용. 순서 주의. |
| production 에서 시드 사용자로 로그인 가능 | seed 미제거. production 적용 전 `DELETE FROM users WHERE email LIKE '%@medibridge.local'`. |
| 사진 PUT 시 보관 PC 가 401 INVALID_TOKEN | 메인 `MEDIBRIDGE_STORAGE_SECRET` ≠ 보관 PC `MEDIBRIDGE_STORAGE_SECRET`. 동일하게 맞추기. |
| `format=pdf` 500 `PDF_RENDER_FAILED` | `wkhtmltopdf` 미설치 → `sudo apt install wkhtmltopdf`. 또는 PATH 확인 (`which wkhtmltopdf`). |
| `/v1/pill/identify` 운영 모드 502 VISION_UPSTREAM_ERROR | Vision PC (10.10.10.120:8003) 미가동. TestMode 활성 또는 Vision PC 띄우기. |
| 청소 잡이 동작 안 함 | 부팅 로그에 `[Main] - 청소 잡 등록` 라인 확인. `MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL=0` 이면 비활성. |

---

## 6. 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 스켈레톤 (Python/FastAPI 가정) |
| v0.2 | 2026-05-07 | Drogon C++ 로 전환 / 스키마 마이그레이션·시드 절차 / TestMode 환경변수 / 시드 사용자 로그인 검증 / systemd 유닛 예시 |
