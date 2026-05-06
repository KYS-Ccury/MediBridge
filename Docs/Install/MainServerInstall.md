# 메인(운용) 서버 설치 매뉴얼 (Ubuntu 24.04)

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 메인 서버 PC 설치 매뉴얼 |
| **대상 OS** | Ubuntu 24.04 LTS |
| **호스트** | `10.10.10.97` ([시스템_연결구조 v2.2 §1.2](../시스템_연결구조_ver2.md)) |
| **역할** | Drogon C++ HTTP, MariaDB(사용자/약 풀/이력/식약처 데이터/photo_storage), DUR 안내, 인젝션 방어 |
| **버전** | v0.2 (TestMode 도입 반영) |
| **개정일** | 2026-05-07 |

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
    uuid-dev zlib1g-dev libbrotli-dev
```

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

# 2) 더미 데이터 (TestMode 사용 시에만)
mysql -u medibridge_app -p medibridge < MainServer/Database/Seeds/dev_seed.sql
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

### 3.6 환경 변수 설정

```bash
# /etc/environment 또는 systemd Environment= 설정
export MEDIBRIDGE_DB_PASSWORD='<3.3 에서 정한 비밀번호>'
export MEDIBRIDGE_JWT_SECRET='<32자 이상 랜덤 문자열>'    # production 필수
export MEDIBRIDGE_PDMA_KEY='<식약처 OpenAPI 서비스 키>'  # 운영 시 필요
export MEDIBRIDGE_ENV='development'                       # 또는 production

# TestMode 활성 (개발 단계 권장)
export MEDIBRIDGE_TEST_MODE='true'
```

> ⚠ **production 환경(`MEDIBRIDGE_ENV=production`)에서 `MEDIBRIDGE_TEST_MODE` 는 강제 비활성**됩니다 (Config.cpp 검증). 보안상 의도된 동작.

### 3.7 실행

```bash
cd MainServer
./build/MediBridgeMainServer

# 시작 로그에 다음이 보여야 정상:
# [Config] MEDIBRIDGE_TEST_MODE = TRUE  → 추론·파일저장 우회, DB seed 사용
# [DB] connected — 127.0.0.1:3306/medibridge (pool=10)
# [Main]   - 포트: 8001
```

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

## 4. 설치 완료 체크리스트

- [ ] Ubuntu 24.04 + sudo 사용자 준비
- [ ] LAN 고정 IP `10.10.10.97`
- [ ] Drogon 1.9+ 빌드/설치
- [ ] MariaDB 11 + `medibridge` DB + `medibridge_app` 사용자
- [ ] 스키마 적용 (`001_init_schema.sql`)
- [ ] 시드 적용 (`dev_seed.sql`) — TestMode 사용 시
- [ ] 환경변수 설정 (`MEDIBRIDGE_DB_PASSWORD`, `MEDIBRIDGE_JWT_SECRET`, `MEDIBRIDGE_TEST_MODE`)
- [ ] `MainServer` CMake 빌드
- [ ] 실행 → Health 200 응답 / 시드 로그인 성공
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

---

## 6. 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 스켈레톤 (Python/FastAPI 가정) |
| v0.2 | 2026-05-07 | Drogon C++ 로 전환 / 스키마 마이그레이션·시드 절차 / TestMode 환경변수 / 시드 사용자 로그인 검증 / systemd 유닛 예시 |
