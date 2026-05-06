# Client — 영역 C (Qt6 + C++ + QML, Windows 10/11)

> 메디브릿지의 GUI 클라이언트. 안드로이드 S24 폰의 입력(이미지·STT 텍스트)을 수신해 메인서버로 전달하고, 결과를 표시한다.

---

## 빠른 시작

1. [Docs/Install/ClientPcInstall.md](../Docs/Install/ClientPcInstall.md) 의 Qt SDK·ADB·scrcpy 셋업 완료
2. Qt Creator 열기 → "Open Project" → 본 폴더의 `CMakeLists.txt` 선택
3. Kit: **Desktop Qt 6.11.0 MinGW 64-bit** 선택
4. 빌드 → 실행 → 콘솔에 다음 표시되면 OK:
   ```
   [Main] WorkerPool 초기화 완료
   [Main] ApiClient 생성 — base_url: http://localhost:8001
   [PhoneServer] 시작됨 — 포트: 8000
   [ResourceMonitor] 시작됨 — 주기: 10000 ms
   [HealthChecker] 시작됨 — 주기: 30000 ms
   ```
5. 폰 ↔ PC USB 연결 + `adb reverse tcp:8000 tcp:8000`
6. 폰 브라우저에서 `http://localhost:8000` → PWA 페이지 표시

---

## 폴더 구조

| 폴더 | 책임 | 주요 파일 |
| --- | --- | --- |
| `PhoneAdapter/` | 폰 PWA 수신 HTTP 서버 | `PhoneServer.h/.cpp`, `Static/Index.html` |
| `MainServerClient/` | 메인서버 REST 호출 (JWT 자동 첨부) | `ApiClient.h/.cpp` |
| `Threading/` | 무거운 작업 분리용 워커 풀 (싱글톤) | `WorkerPool.h/.cpp` |
| `Services/` | 폰 → 메인서버 중계 비즈니스 로직 | `MediaForwarder.h/.cpp` |
| `Monitoring/` | 자체 자원 측정 + MainServer /health 폴링 (콘솔 로그만) | `ResourceMonitor.h/.cpp`, `HealthChecker.h/.cpp` |
| `Ui/` | QML UI (확장 단계) | (비어있음) |

---

## 명명 규칙

- **폴더·파일·클래스**: PascalCase (`PhoneServer`, `ApiClient`)
- **메소드·변수**: snake_case (`start_server()`, `api_client_`)
- **상수**: UPPER_SNAKE_CASE (`PHONE_ADAPTER_PORT`)
- **네임스페이스**: `medibridge::xxx` (`medibridge::phone`, `medibridge::network` 등)
- 외부 표준(Qt 시그널 오버라이드 등)은 그대로

---

## 분담 가능한 작업 단위 (TODO 마커)

각 `.cpp` 파일의 `TODO` 주석이 채워야 할 작업.

| 영역 | 파일 | 작업 |
| --- | --- | --- |
| HTTP 서버 | `PhoneAdapter/PhoneServer.cpp` | multipart 파싱, JSON 파싱, MediaForwarder 호출 |
| REST 클라 | `MainServerClient/ApiClient.cpp` | `send_request` 본체 구현 (QNetworkAccessManager 통합), 401 감지·재로그인 시그널 |
| 비즈니스 | `Services/MediaForwarder.cpp` | 이미지 검증·리사이즈 (WorkerPool 위임), 인젝션 패턴 1차 필터 |
| 모니터링 | `Monitoring/ResourceMonitor.cpp` | Windows API (PDH·GlobalMemoryStatusEx·GetDiskFreeSpaceEx) 통합 |
| 모니터링 | `Monitoring/HealthChecker.cpp` | QElapsedTimer 기반 latency 측정, 상태 전이 자동 재연결 로직 |
| UI | `Ui/` | QML 화면 (확장 단계: 로그인·식별·복약 이력·보고서) |

---

## 핵심 흐름

```
[ 폰 PWA ]
   └─ adb reverse tcp:8000 → localhost:8000
   ↓
[ PhoneAdapter (PhoneServer) ]
   ├─ GET /  → Index.html
   ├─ POST /v1/media/image    ┐
   └─ POST /v1/speech/utterance┘ → MediaForwarder
   ↓
[ MediaForwarder ]
   ├─ (선택) WorkerPool — 이미지 검증·리사이즈
   └─ ApiClient.upload_image / send_utterance
   ↓
[ ApiClient ]
   └─ QNetworkAccessManager + JWT 헤더
   ↓
[ MainServer (8001) ]
```

---

## 외부 의존성

| 라이브러리 | 용도 | 모듈 |
| --- | --- | --- |
| `Qt6::Core` | 기본 객체·이벤트 루프 | 전체 |
| `Qt6::Network` | QNetworkAccessManager | MainServerClient |
| `Qt6::HttpServer` | QHttpServer | PhoneAdapter |
| `Qt6::Concurrent` | QtConcurrent::run | Threading |
| (Windows API) Pdh.lib | CPU·메모리 측정 | Monitoring (TODO 단계 추가) |

---

## 관련 문서

- [Docs/시스템 흐름 정리본_ver2.md](../Docs/시스템%20흐름%20정리본_ver2.md) §2.1 PC 구성
- [Docs/요구사항_분석서_ver2.md](../Docs/요구사항_분석서_ver2.md) §6 영역 C
- [Docs/Api/](../Docs/Api/) 모든 API 명세서 (PhoneServer·ApiClient는 여기 정의된 형식 준수)
- [Docs/Install/ClientPcInstall.md](../Docs/Install/ClientPcInstall.md) 환경 셋업
