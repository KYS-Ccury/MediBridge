# Client — 영역 C (Qt6 + QML + C++, MVVM 패턴, Windows 10/11)

> 메디브릿지 GUI 클라이언트. **프론트엔드(QML) ↔ 백엔드(C++) 명확 분리**.
> 폰(안드로이드 S24)을 **USB 케이블로 직결**하여 카메라·마이크·온디바이스 STT 자원만 사용.

---

## 빠른 시작

1. [Docs/Install/ClientPcInstall.md](../Docs/Install/ClientPcInstall.md) 의 Qt SDK·ADB·scrcpy 셋업
2. Qt Creator에서 본 폴더 `CMakeLists.txt` 열기
3. Kit: **Desktop Qt 6.11.0 MinGW 64-bit**
4. 빌드 → 실행 → 콘솔 + Material 스타일 GUI 윈도우 표시
5. 폰 USB 연결 → 자동으로 `adb reverse tcp:8000 tcp:8000` 실행됨
6. 폰 브라우저에서 `http://localhost:8000` 접속 → PWA 페이지

---

## 폴더 구조

```
Client/
├─ Main.cpp                      ← QGuiApplication + QQmlApplicationEngine
├─ CMakeLists.txt
│
├─ Frontend/                     ← QML (UI만 — 비즈니스 로직 ❌)
│  ├─ Resources.qrc
│  ├─ Main.qml                   ← Window + StackView (라우터)
│  ├─ Pages/                     ← 화면별 (7종)
│  └─ Components/                ← 재사용 (4종)
│
├─ Backend/                      ← C++ (비즈니스 로직)
│  ├─ Controllers/               ← QML 노출용 ViewModel
│  └─ Models/                    ← QAbstractListModel
│
├─ PhoneLink/                    ← USB 유선 연동 자동화
│  ├─ AdbDeviceMonitor           ← `adb devices` 폴링
│  └─ AdbReverseManager          ← 자동 `adb reverse`
│
├─ PhoneAdapter/                 ← 폰 PWA 수신 HTTP 서버
├─ MainServerClient/             ← 메인서버 REST 호출 (조립자 + 7 카테고리)
├─ Services/                     ← ImageForwarder, UtteranceForwarder
├─ Threading/                    ← QtConcurrent 워커 풀
└─ Monitoring/                   ← 자체 자원 측정 (콘솔 로그)
```

---

## MVVM 패턴 — 절대 규칙

### Frontend (QML)
- ❌ `import network::ApiClient` 등 직접 import 금지
- ❌ JSON 파싱·검증·포맷팅 등 비즈니스 로직 금지
- ✅ `*_controller.method(...)` 호출만 (Q_INVOKABLE)
- ✅ `*_controller.property` 바인딩 (Q_PROPERTY)
- ✅ `Connections { target: ... ; function on*_succeeded() }` 시그널 수신

### Backend (C++ Controllers)
- 본인 자신은 비즈니스 로직 작성 X (Service 위임)
- QML 노출 인터페이스만 (`Q_PROPERTY` + `Q_INVOKABLE` + `signals`)
- `is_loading_changed`, `login_succeeded` 같은 시그널로 결과 전파

---

## QML ↔ Controller 매핑

| QML에서 사용 | Controller (C++) | 책임 |
| --- | --- | --- |
| `auth_controller` | `AuthController` | 로그인·회원가입·로그아웃, JWT 자동 관리 |
| `pill_controller` | `PillController` | 식별·DUR + 약 풀 CRUD |
| `history_controller` | `HistoryController` | 복약 이력 기록·조회 |
| `report_controller` | `ReportController` | 보고서 PDF/HTML 생성 |
| `phone_link_controller` | `PhoneLinkController` | 폰 USB 연결 상태 + 수동 재연결 |
| `app_controller` | `AppController` | 전역 상태 (페이지·로딩·토스트) |

---

## 폰 USB 유선 연동

본 클라이언트는 폰을 **USB 케이블로만** 연결한다. WiFi 페어링 사용 X.

### 자동화 흐름 (Q4=C: 자동 + 수동)
```
[ AdbDeviceMonitor ] 5초마다 `adb devices` 폴링
       ↓ 디바이스 감지
[ device_changed("R3CXXXXXXX", "device") 시그널 ]
       ↓
[ PhoneLinkController.on_device_changed ]
       ↓
[ AdbReverseManager.setup_reverse() ] — 자동
       ↓ 성공
[ phone_ready 시그널 ] → QML 토스트 + LED 초록
```

수동 재연결 버튼은 `PhoneStatusIndicator` 컴포넌트에 포함.

---

## 명명 규칙

| 항목 | 규칙 | 예 |
| --- | --- | --- |
| 폴더·파일 | PascalCase | `AuthController.h` |
| 클래스·네임스페이스 | PascalCase | `medibridge::controllers::AuthController` |
| 메소드·변수 | snake_case | `login()`, `api_client_` |
| 상수 | UPPER_SNAKE_CASE | `PHONE_ADAPTER_PORT` |
| Q_PROPERTY | snake_case | `is_authenticated` |
| NOTIFY 시그널 | `<name>_changed` | `is_authenticated_changed` |
| QML 파일 | PascalCase | `LoginPage.qml` |
| QML id | snake_case | `id: email_field` |
| QML role 이름 | snake_case | `model.drug_name` |
| 외부 표준 (Qt Material, METHOD_LIST_BEGIN 등) | 그대로 | — |

---

## TODO 분담 가능 작업

| 영역 | 파일 | 작업 |
| --- | --- | --- |
| Backend | `Controllers/AuthController.cpp` | 로그인 응답 JSON 파싱 → 토큰·user 정보 채우기 |
| Backend | `Controllers/PillController.cpp` | 식별 응답 JSON → confidence_tier·tts_text·dur_check 채우기 + ListModel 갱신 |
| Backend | `Controllers/HistoryController.cpp` | 응답 JSON → HistoryListModel 채우기 |
| Backend | `Controllers/ReportController.cpp` | PDF 바이너리 저장, HTML 시스템 브라우저 열기 |
| Backend | `MainServerClient/ApiClientCommon.cpp` | `send_request` QNetworkAccessManager 통합 + 401 처리 |
| Backend | `MainServerClient/MediaApiClient.cpp` | QHttpMultiPart 이미지 업로드 |
| PhoneLink | `AdbDeviceMonitor.cpp` | (이미 골격 동작) — 안드로이드 환경 테스트 |
| Frontend | `Pages/IdentifyResultPage.qml` | `pill_candidate_list_model`, `dur_detail_list_model` 컨텍스트 등록 후 ListView/Repeater 바인딩 |
| Frontend | `Pages/PillPoolPage.qml` | `pool_item_list_model` 등록 후 ListView |
| Frontend | `Pages/HistoryPage.qml` | `history_list_model` 등록 후 ListView |
| Frontend | (신규) DatePicker 컴포넌트 | 기간 선택 UX 개선 |

> ListModel 컨텍스트 등록은 Main.cpp 에서 `setContextProperty("pill_candidate_list_model", &model)` 추가 필요. 현재는 Controller만 노출됨.

---

## 외부 의존성

| 라이브러리 | 용도 |
| --- | --- |
| Qt6::Core, Gui, Qml, Quick, QuickControls2 | GUI + QML |
| Qt6::Network | QNetworkAccessManager |
| Qt6::HttpServer | QHttpServer (PhoneAdapter) |
| Qt6::Concurrent | QtConcurrent::run |
| (외부 도구) ADB Platform-Tools | `adb devices`, `adb reverse` |

---

## 관련 문서

- [Docs/시스템 흐름 정리본_ver2.md](../Docs/시스템%20흐름%20정리본_ver2.md) §2.1
- [Docs/요구사항_분석서_ver2.md](../Docs/요구사항_분석서_ver2.md) §6 영역 C
- [Docs/Api/](../Docs/Api/) 모든 API 명세서
- [Docs/Install/ClientPcInstall.md](../Docs/Install/ClientPcInstall.md)
- [Docs/Install/AndroidPhoneSetup.md](../Docs/Install/AndroidPhoneSetup.md) — USB 연결 셋업
