# MainServer — 영역 B (Drogon C++ 백엔드, Ubuntu 24.04)

> 메디브릿지 메인 서버. FastAPI 대신 **Drogon (C++)** 채택 — Client와 동일 언어로 통일.

---

## 빠른 시작 (TestMode)

### 최초 1회만 — 셋업

상세 절차는 [Docs/Install/MainServerInstall.md](../Docs/Install/MainServerInstall.md). 요약:

```bash
# 1) 의존성 (Ubuntu 24.04)
sudo apt install -y \
    build-essential cmake git pkg-config libssl-dev libmariadb-dev \
    libjsoncpp-dev libpq-dev libsqlite3-dev libhiredis-dev \
    libc-ares-dev libyaml-cpp-dev uuid-dev zlib1g-dev libbrotli-dev \
    libdrogon-dev mariadb-server

# 2) 빌드
cd MainServer && mkdir -p build-wsl && cd build-wsl
cmake .. && make -j$(nproc) && cd ..

# 3) DB·시드 (root 권한)
sudo bash Scripts/smoke_setup.sh
```

### 매번 PC 부팅 후 — 두 명령으로 시연 환경 띄우기

**(1) WSL 또는 Ubuntu 터미널**:

```bash
bash MainServer/Scripts/medibridge-up.sh
```

→ MariaDB 시작 + 환경변수 + 백그라운드 실행 + `/health` 부팅 확인까지 자동.

**(2) Windows 관리자 PowerShell** *(WSL 환경에서만 — LAN 노출용. 실 Ubuntu PC 면 생략)*:

```powershell
cd C:\Users\LMS\Desktop\Project\MediBridge\MainServer\Scripts
.\medibridge-portproxy.ps1
```

→ WSL 의 8001 포트를 Windows LAN 으로 노출 + 방화벽 허용.

> 처음 한 번만 실행 정책 풀기:
> ```powershell
> Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
> ```

### 동작 확인

| 위치 | URL | 기대 |
| --- | --- | --- |
| 같은 PC 브라우저 | `http://127.0.0.1:8001/health` | `{"status":"ok",...}` JSON |
| LAN 다른 PC 브라우저 | `http://10.10.10.97:8001/health` | 같은 JSON (portproxy 등록 후) |

### 자주 쓰는 보조 명령

| 상황 | 명령 |
| --- | --- |
| 서버 로그 실시간 보기 | `tail -f /tmp/medibridge.log` |
| 서버 종료 | `pkill -f MediBridgeMainServer` |
| 재시작 | `bash MainServer/Scripts/medibridge-up.sh` (이전 인스턴스 자동 종료 후 재실행) |
| 코드 수정 후 재빌드+재실행 | `cd build-wsl && make -j4 && cd .. && bash Scripts/medibridge-up.sh` |
| portproxy 등록 확인 | (관리자 PS) `netsh interface portproxy show v4tov4` |

### 시드 사용자 (TestMode)

| email | password | user_id |
| --- | --- | --- |
| `test@medibridge.local` | `test1234` | `test_user_001` |
| `demo@medibridge.local` | `demo1234` | `test_user_002` |

→ 시드 약 풀 4건이 `test_user_001` 에 이미 등록되어 있음 (타이레놀500, 이부프로펜200, 베아제, 아스피린).

### 설정 우선순위

```
Config.h 멤버 기본값  <  config.json  <  환경변수
                                       (항상 최우선)
```

- `config.json` 은 선택적. 없으면 환경변수+기본값만 사용 (자동 폴백).
- 깨진 JSON 도 안전하게 폴백 (파싱 실패 경고만 출력).
- 시크릿 (`db.password` / `jwt.secret` / `storage.secret` / `pdma.service_key`) 은 **config.json 에 절대 넣지 말 것** — 환경변수 전용 강제 정책. 샘플은 `config.sample.json` 참조.

### 환경변수

| 이름 | 기본값 | 비고 |
| --- | --- | --- |
| `MEDIBRIDGE_TEST_MODE` | `false` | `true` 시 추론·파일저장 우회, DB seed 응답. production 환경에서는 강제 OFF |
| `MEDIBRIDGE_DB_PASSWORD` | (없음) | medibridge_app 계정 비밀번호 |
| `MEDIBRIDGE_JWT_SECRET` | (없음) | 32 바이트 이상 권장 |
| `MEDIBRIDGE_PDMA_KEY` | (없음) | 식약처 OpenAPI 키 (운영 모드) |
| `MEDIBRIDGE_STORAGE_BASE_URL` | `http://10.10.10.122:8004` | 데이터 보관 PC URL (⑤+⑥) |
| `MEDIBRIDGE_STORAGE_SECRET` | (없음) | 보관 PC PUT/GET 토큰 HMAC 시크릿. **JWT 와 다른 32+ 바이트 시크릿** |
| `MEDIBRIDGE_STORAGE_TOKEN_TTL` | `300` | PUT/GET 토큰 만료 (초) |
| `MEDIBRIDGE_STORAGE_MAX_BYTES` | `10485760` | 업로드 허용 최대 바이트 (10MB) |
| `MEDIBRIDGE_ENV` | `development` | `production` 시 보안 검증 강화 |

### 포트·베이스 URL

기본 포트 **8001**, 베이스 URL `http://<host>:8001/v1`. 자세한 IP·포트 매트릭스는 [시스템_연결구조 v2.2 §1.2](../Docs/시스템_연결구조_ver2.md).

---

## TestMode 전체 설계

[Docs/TestMode.md](../Docs/TestMode.md) — DB-backed 더미 응답 정책, seed prefix(`999800xxx`/`test_user_*`/`anon_test_*`), 어떤 엔드포인트가 우회되는지 등.

---

## 폴더 구조

| 폴더 | 책임 |
| --- | --- |
| `Routers/` | HTTP 엔드포인트 (얇음 — 검증·JSON 변환만) |
| `Schemas/` | 요청/응답 JSON 형식 정의 (.h만, 헤더 온리) |
| `Services/` | 비즈니스 로직 (인증·DUR·추론 호출·캐시·보고서) |
| `Database/` | DB 모델 + 연결 풀 |
| `Threading/` | 무거운 CPU 작업용 워커 풀 (싱글톤) |
| `Monitoring/` | /health, /metrics 핸들러 + 자체 자원 측정 |
| `Scripts/` | 식약처 CSV 일괄 적재 등 CLI 도구 |

---

## 명명 규칙

- **폴더·파일·클래스·네임스페이스**: PascalCase (예: `medibridge::services::AuthService`)
- **메소드·변수**: snake_case (예: `create_user()`, `db_client_`)
- **상수**: UPPER_SNAKE_CASE
- 외부 라이브러리(Drogon `METHOD_LIST_BEGIN`, `HttpRequestPtr` 등)는 그대로

---

## TODO 마커별 분담 가능 작업

| 영역 | 파일 | 작업 |
| --- | --- | --- |
| 인증 | `Services/AuthService.cpp` | bcrypt 해시·JWT 발급 (jwt-cpp 사용), 사용자 CRUD |
| 라우터 | `Routers/Auth.cpp`, `History.cpp`, `Pill.cpp`, ... | 8개 라우터 모두 본체 구현 (Schema 파싱 → Service 호출 → 응답) |
| DUR | `Services/DurChecker.cpp` | dur_interaction_cache 페어 SQL 조회 |
| 추론 호출 | `Services/InferenceClient.cpp` | Drogon HttpClient 비동기 호출 6종 (Vision·Intent·Onboarding·Summary) |
| 식약처 캐시 | `Services/PdmaCacheManager.cpp` | e약은요 lazy 캐싱 + 외부 API HTTPS 호출 |
| 보고서 | `Services/ReportGenerator.cpp` | HTML 템플릿·PDF 렌더 (WorkerPool 위임) |
| DB | `Database/Connection.cpp` | Drogon DbClient 풀 초기화·핑 |
| 모니터링 | `Monitoring/ResourceMonitor.cpp`, `HealthChecker.cpp`, `MetricsExporter.cpp` | /proc 파싱·sysinfo·statvfs |
| 일괄 적재 | `Scripts/ImportPdmaData.cpp` (신규) | 식약처 CSV/EXCEL → MariaDB 일괄 INSERT |

---

## API 엔드포인트 (전체)

| 라우터 | 엔드포인트 | 명세 |
| --- | --- | --- |
| Auth | POST /v1/auth/{signup,login,logout} | [AuthApi.md](../Docs/Api/AuthApi.md) |
| History | POST /v1/history/record, GET /v1/history/list | [HistoryApi.md](../Docs/Api/HistoryApi.md) |
| Report | GET /v1/report/generate | [ReportApi.md](../Docs/Api/ReportApi.md) |
| Media | POST /v1/media/image | [MediaApi.md](../Docs/Api/MediaApi.md) |
| Speech | POST /v1/speech/utterance | [SpeechApi.md](../Docs/Api/SpeechApi.md) |
| Pill | POST /v1/pill/identify, /v1/pill/pool/* | [PillApi.md](../Docs/Api/PillApi.md) |
| Monitoring | GET /health, /metrics | [MonitoringApi.md](../Docs/Api/MonitoringApi.md) |

---

## 외부 의존성

| 라이브러리 | 용도 | 설치 |
| --- | --- | --- |
| Drogon (1.9+) | HTTP 프레임워크 + ORM | `apt install drogon-dev` 또는 소스 빌드 |
| MariaDB Connector/C | DB 클라이언트 | `apt install libmariadb-dev` |
| jwt-cpp | JWT 서명·검증 | 헤더 온리 (vcpkg 또는 소스) |
| OpenSSL / libbcrypt | 비밀번호 해시 | `apt install libssl-dev` |
| nlohmann/json (선택) | JSON (Drogon 내장 Json::Value 도 가능) | `apt install nlohmann-json3-dev` |

---

## 핵심 정책 (절대 준수)

1. **DUR 위험 안내에 LLM·RAG 절대 미사용** — 식약처 본문 그대로 인용
2. **단정 표현 금지** — "복용 가능합니다" / "복용 불가합니다" 출력 금지
3. **모든 SQL은 파라미터 바인딩** — SQL 인젝션 방어
4. **비밀번호는 평문 저장 금지** — bcrypt 해시 필수
5. **외부 인터넷 출처 사용 금지** — 식약처 데이터만 사용

---

## 관련 문서

- [Docs/시스템 흐름 정리본_ver2.md](../Docs/시스템%20흐름%20정리본_ver2.md) §2.1 PC 구성
- [Docs/요구사항_분석서_ver2.md](../Docs/요구사항_분석서_ver2.md) §5 영역 B
- [Docs/DB_ERD_ver2.md](../Docs/DB_ERD_ver2.md) MariaDB 스키마
- [Docs/Api/](../Docs/Api/) 모든 API 명세서
