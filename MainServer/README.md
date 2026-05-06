# MainServer — 영역 B (Drogon C++ 백엔드, Ubuntu 24.04)

> 메디브릿지 메인 서버. FastAPI 대신 **Drogon (C++)** 채택 — Client와 동일 언어로 통일.

---

## 빠른 시작

1. [Docs/Install/MainServerInstall.md](../Docs/Install/MainServerInstall.md) 의 절차 (현재 스켈레톤 — 셋업 진입 시 채움)
2. Drogon 설치 후 본 폴더에서:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ./MediBridgeMainServer
   ```
3. config.json 또는 환경변수:
   - `MEDIBRIDGE_DB_PASSWORD`
   - `MEDIBRIDGE_JWT_SECRET`
4. 기본 포트: **8001**, 베이스 URL: `http://<host>:8001/v1`

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
