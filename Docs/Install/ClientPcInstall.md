# 클라이언트 PC 설치 매뉴얼 (Windows 10/11)

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 클라이언트 GUI PC 설치 매뉴얼 |
| **대상 OS** | Windows 10 / 11 (64-bit) |
| **역할** | Qt6 + C++ + QML 기반 GUI, 안드로이드 폰 입력 수신, 운용 서버 통신 |
| **버전** | v1.2 |
| **작성일** | 2026-05-06 |
| **작성자** | 팀 (3인) |

> 본 매뉴얼은 메디브릿지 클라이언트 GUI PC(Windows)에서 설치해야 할 모든 도구의 절차를 정리한 문서이다. 환경 재구축·온보딩 시 본 문서 한 권으로 클라 PC 셋업이 완료되도록 한다.

---

## 1. 설치 항목 요약

| # | 도구 | 용도 | 설치 방식 |
| --- | --- | --- | --- |
| 1 | **Qt 6.7+ SDK** (MinGW 13.x 64-bit, Qt HTTP Server, Qt Creator, CMake, Ninja) | GUI 클라이언트 본체 + HTTP 서버 (PoC 단계 채택) | Qt Online Installer |
| 2 | **Android SDK Platform-Tools (ADB)** | 안드로이드 폰 연결·디버깅 | Google 공식 ZIP 다운로드 |
| 3 | **scrcpy** | 폰 화면 PC 미러링 + 마우스/키보드 조작 (개발·시연 보조) | Genymobile GitHub 릴리스 |

---

## 2. 사전 준비

- 관리자 권한이 있는 Windows 사용자 계정
- 인터넷 연결 (유선 LAN 가능, PC WiFi 어댑터 미사용 환경에서도 OK)
- Qt 계정 (오픈소스 무료 라이선스, 이메일 인증 필요)
- 충분한 디스크 여유 공간: **약 15GB** (Qt SDK 약 10GB + 도구 약 5GB)

---

## 3. Qt 6 SDK 설치

### 3.1 Qt Online Installer 다운로드

1. https://www.qt.io/download-qt-installer 접속
2. **"Download the Qt Online Installer"** 버튼 → Windows용 `.exe` 다운로드 (약 40MB)
3. Qt 회원가입 (오픈소스 사용 시 무료, 이메일 인증)

### 3.2 설치 마법사 진행

1. 다운로드한 인스톨러 실행
2. Qt 계정 로그인
3. 라이선스: **Open Source** 선택 (무료)
4. 설치 경로: `C:\Qt` 권장 (한글·공백 없는 경로)
5. **컴포넌트 선택** — 다음 항목 모두 체크:
   - **Qt 6.7.x** (또는 **6.8.x** LTS)
     - ☑ **MinGW 13.x 64-bit** ← Qt 동봉 컴파일러 (별도 설치 불필요)
     - ☑ **Qt HTTP Server** ⭐ (PoC HTTP 서버 핵심 모듈, Qt 6.4+ 부터 포함)
     - ☑ **Qt Quick 3D** (확장 시 QML 사용 위해)
   - **Developer and Designer Tools**:
     - ☑ **Qt Creator** (IDE)
     - ☑ **CMake**
     - ☑ **Ninja**
6. "다음" → 약관 동의 → 설치 시작 (약 30분~1시간 소요)

### 3.3 설치 확인

설치 완료 후 **Qt Creator** 실행:

- 시작 메뉴 → "Qt Creator" 검색 → 실행
- Welcome 탭이 정상 표시되는지 확인
- **Edit → Preferences → Kits** 탭에서 `Desktop Qt 6.7.x MinGW 64-bit` 키트가 자동 등록되어 있는지 확인 (경고 ⚠️ 없음)

### 3.4 트러블슈팅

| 증상 | 원인 / 해결 |
| --- | --- |
| 설치 도중 다운로드 실패 | 사내·기관 네트워크의 프록시·방화벽 차단 가능. Qt Installer 재실행 후 "재시도" 또는 다른 네트워크에서 시도 |
| Kit에 노란 경고 ⚠️ 표시 | 컴파일러 누락 가능성. 설치 마법사를 다시 실행해 MinGW 13.x 64-bit 추가 설치 |
| Qt HTTP Server 항목이 안 보임 | Qt 6.4 미만 버전을 선택했을 가능성. 6.7+ 선택 필수 |

---

## 4. Android SDK Platform-Tools (ADB) 설치

### 4.1 도구 보관 폴더 생성

```powershell
mkdir C:\Tools
```

> 시스템 도구를 `C:\Tools` 한 곳에 모아 PATH 관리·백업이 쉽게 함.

### 4.2 다운로드 + 압축 해제

1. https://developer.android.com/tools/releases/platform-tools 접속
2. **"Download SDK Platform-Tools for Windows"** 클릭
3. 약관 동의 → ZIP 파일 다운로드 (약 15MB)
4. ZIP을 `C:\Tools\` 안에 압축 해제
5. 결과: `C:\Tools\platform-tools\` 폴더 생성. 안에 `adb.exe`, `fastboot.exe` 등이 존재

### 4.3 설치 확인

```powershell
C:\Tools\platform-tools\adb.exe --version
```

→ `Android Debug Bridge version 1.0.41` 같은 결과가 나오면 OK.

---

## 5. scrcpy 설치

### 5.1 다운로드 + 압축 해제

1. https://github.com/Genymobile/scrcpy/releases 접속
2. 최신 릴리스에서 **`scrcpy-win64-v3.x.zip`** 다운로드 (약 40MB)
3. ZIP을 `C:\Tools\` 안에 압축 해제
4. 결과: `C:\Tools\scrcpy-win64-v3.x\` 폴더 생성

### 5.2 폴더명 정리

```powershell
cd C:\Tools
ren scrcpy-win64-v3.x scrcpy
```

> 폴더명을 짧게 정리해 PATH 등록·명령어 입력이 편하게 함. 실제 다운로드한 폴더명에 맞춰 첫 인자 입력.

### 5.3 설치 확인

```powershell
C:\Tools\scrcpy\scrcpy.exe --version
```

→ `scrcpy 3.x.x` 같은 결과가 나오면 OK.

> **참고**: scrcpy 압축 안에 `adb.exe`가 함께 들어있지만, 4장에서 받은 Platform-Tools `adb`를 권장(더 최신 + 공식). PATH는 Platform-Tools 쪽을 우선 등록.

---

## 6. PATH 환경변수 등록 (필수 권장)

PATH에 등록하면 어느 폴더에서든 `adb`, `scrcpy` 명령을 곧바로 사용할 수 있다.

### 6.1 환경변수 편집기 열기

1. **시작** → "환경 변수" 검색 → **"시스템 환경 변수 편집"** 클릭
2. 시스템 속성 창 하단 **"환경 변수(N)..."** 클릭

### 6.2 사용자 PATH에 추가

1. **상단 "사용자 변수"** 영역에서 **`Path`** 클릭 → **"편집(E)"**
2. **"새로 만들기(N)"** → 다음 경로 추가:
   ```
   C:\Tools\platform-tools
   ```
3. 다시 **"새로 만들기(N)"** → 추가:
   ```
   C:\Tools\scrcpy
   ```
4. 모든 창 **"확인"** 으로 닫기

### 6.3 PATH 적용 확인

⚠️ 기존 PowerShell 창은 PATH 갱신 전 — **반드시 새 PowerShell 창** 열기.

```powershell
adb --version
scrcpy --version
```

→ 둘 다 버전 정보가 떠야 OK.

### 6.4 PowerShell 실행 정책 (해당 시)

가상환경 등 스크립트 실행 시 보안 정책 에러가 뜰 수 있다:

```
about_Execution_Policies ... 실행할 수 없습니다.
```

해결: PowerShell **관리자 권한** 으로 실행 후:

```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```

→ "Y" 입력. 사용자 한정·로컬 스크립트 허용으로 보안상 안전.

---

## 7. 안드로이드 폰 연결

폰 측 설정은 [AndroidPhoneSetup.md](AndroidPhoneSetup.md) 의 4장 (USB 직결 ⭐ 권장 / 무선 디버깅) 절차를 먼저 진행한 후, PC에서 다음 명령으로 인식·연결한다.

### 7.0 채널 선택 — USB 직결 vs 무선

| 환경 | 권장 채널 |
| --- | --- |
| PC 유선 LAN ↔ 폰 WiFi가 **다른 네트워크** (사내망 vs 외부 WiFi 등) | **7.A USB 직결** ⭐ |
| PC ↔ 폰이 **같은 라우터/공유기** | 7.B 무선 디버깅 |
| 안정성·시연 영상 녹화 우선 | **7.A USB 직결** ⭐ |

본 프로젝트 MVP의 기본 채널은 **USB 직결**로 결정 (2026-05-06).

### 7.A USB 직결 (권장 채널)

#### 7.A.1 PC가 폰을 인식하는지 확인

USB 케이블로 폰 ↔ PC 연결 후:

```powershell
adb devices
```

→ `<디바이스 시리얼>     device` 표시되면 OK.

`unauthorized` 표시 시: 폰 화면의 "USB 디버깅 허용?" 다이얼로그를 확인하고 "이 컴퓨터에서 항상 허용" 체크 후 허용.

#### 7.A.2 scrcpy 실행

```powershell
scrcpy
```

→ 폰 화면이 PC 창에 미러링됨. 마우스로 폰 조작 가능.

#### 7.A.3 PWA 통신용 포트 포워딩 (PoC 단계에서 사용) ⭐

PWA 페이지를 폰에서 띄우려면 폰이 PC의 HTTP 서버에 접근해야 하는데, 같은 네트워크가 아니어도 **`adb reverse`** 한 줄로 해결 가능:

```powershell
adb reverse tcp:8000 tcp:8000
```

**의미**: 폰이 자신의 `localhost:8000` 으로 요청 → ADB가 USB 경유하여 PC의 `localhost:8000` 으로 포워딩.
- 같은 네트워크 필요 ❌
- 사내 방화벽·네트워크 정책 영향 ❌
- 폰 브라우저에서 `http://localhost:8000` 입력만 하면 PC 서버 응답 받음

**확인**:

```powershell
adb reverse --list
```

→ `(reverse) tcp:8000 tcp:8000` 표시되면 OK.

USB 케이블 분리 시 자동 해제. 재연결 시 다시 `adb reverse tcp:8000 tcp:8000` 실행 필요.

### 7.B 무선 디버깅 (같은 네트워크 환경에서만)

같은 라우터/공유기 환경일 때만 사용. 다른 네트워크면 7.A로.

#### 7.B.1 무선 페어링 (최초 1회)

```powershell
adb pair 192.168.0.42:43251
```

> IP·포트는 폰의 "무선 디버깅 → 페어링 코드로 디바이스 페어링" 화면에 뜬 값으로 교체. 6자리 페어링 코드 입력 시 폰 화면에 뜬 코드 입력.

#### 7.B.2 ADB 연결

```powershell
adb connect 192.168.0.42:5555
adb devices
```

> 페어링 포트(43251)와 디버깅 포트(5555)는 다름. 디버깅 포트는 폰의 "무선 디버깅" 메인 화면에 별도 표시됨.

#### 7.B.3 scrcpy 실행

```powershell
scrcpy
```

→ 폰 화면이 PC 창으로 미러링되면 성공.

#### 7.B 트러블슈팅: `protocol fault (couldn't read status message): No error`

PC와 폰이 다른 네트워크(`10.10.10.x` vs `192.168.0.x` 등)면 무선 페어링 자체가 동작하지 않는다. → 7.A USB 직결로 전환.

### 7.4 자주 쓰는 scrcpy 옵션

```powershell
:: 비트레이트 조정 (네트워크 약할 때)
scrcpy --video-bit-rate=4M

:: 시연 영상 녹화
scrcpy --record=demo.mp4

:: 폰 화면 끄고 PC만 표시 (시연용·배터리 절약)
scrcpy --turn-screen-off

:: 폰 마이크 소리 PC로 (Galaxy AI STT 디버깅 시)
scrcpy --audio-source=mic
```

---

## 8. Qt Creator 첫 빌드·실행

### 8.1 프로젝트 열기

1. Qt Creator 실행
2. **File → Open File or Project...** (또는 Ctrl+O)
3. `C:\Users\LMS\Desktop\Project\MediBridge\Client\CMakeLists.txt` 선택 → "Open"

### 8.2 Configure Project

자동으로 뜨는 Configure 화면에서:
- **Desktop Qt 6.11.0 MinGW 64-bit** 만 체크 (MSVC2022·Python 키트 해제)
- 우측 하단 **"Configure Project"** 클릭
- 빌드 디렉토리 자동 생성: `Client/build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug/`

### 8.3 빌드 + 실행

| 동작 | 단축키 | 아이콘 |
| --- | --- | --- |
| 빌드만 | **Ctrl+B** | 좌측 하단 망치 🔨 |
| 빌드 + 실행 | **Ctrl+R** | 좌측 하단 초록 ▶ |

성공 시 콘솔에 `[Main] MediBridge Client 시작됨` + GUI 윈도우 표시.

### 8.4 빌드 에러 트러블슈팅

| 증상 | 원인 / 해결 |
| --- | --- |
| `Unknown module(s) in QT: HttpServer` | Qt Maintenance Tool 으로 **Qt HTTP Server** 모듈 추가 설치 |
| `Cannot find -lQt6QuickControls2` | **Qt Quick Controls 2** 모듈 추가 설치 |
| `qrc:/Frontend/Main.qml: No such file` | Build → Clean & Rebuild |
| `'gmtime_s' was not declared` | `<time.h>` 추가 또는 Clean Rebuild (MinGW 헤더 캐시 문제) |
| 콘솔 창 안 뜸 | CMakeLists.txt 의 `WIN32_EXECUTABLE FALSE` 확인 |
| 폰 연결됐는데 LED 빨강 | `adb devices` 가 PATH 에 등록됐는지 확인 (6장 PATH 등록 절차) |
| `'QRegularExpression' is an incomplete type` | Qt 6 forward declaration 만 있고 본체 헤더 별도 — 사용 .cpp 에 `#include <QRegularExpression>` 추가. (`qstringfwd.h` 만으로는 부족) |
| `'QUuid' is an incomplete type` 또는 다른 Qt 클래스 incomplete type | 해당 헤더 직접 include — Qt 6 는 transitive include 가 줄어들어 명시 필요 |

### 8.5 빌드 산출물

| 항목 | 경로 |
| --- | --- |
| 실행 파일 | `Client/build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug/MediBridgeClient.exe` |
| Qt 런타임 DLL | 같은 폴더 (자동 복사) |
| 빌드 로그 | Qt Creator "Compile Output" |
| 실행 로그 | Qt Creator "Application Output" + 별도 콘솔 창 |

`build/` 는 `.gitignore` 처리됨.

---

## 9. 설치 완료 체크리스트

다음 항목이 모두 ✅ 면 클라 PC 셋업 완료.

### Qt 6 SDK
- [ ] Qt Online Installer 실행 + Qt 6.7.x 이상 설치 완료
- [ ] MinGW 13.x 64-bit 컴파일러 설치
- [ ] Qt HTTP Server 모듈 체크
- [ ] Qt Creator 실행 시 Welcome 탭 정상 표시
- [ ] Kit `Desktop Qt 6.7.x MinGW 64-bit` 자동 등록 (경고 없음)

### ADB Platform-Tools
- [ ] `C:\Tools\platform-tools\adb.exe` 존재
- [ ] PowerShell 새 창에서 `adb --version` 응답

### scrcpy
- [ ] `C:\Tools\scrcpy\scrcpy.exe` 존재
- [ ] PowerShell 새 창에서 `scrcpy --version` 응답

### PATH 등록
- [ ] 사용자 PATH에 `C:\Tools\platform-tools` 추가
- [ ] 사용자 PATH에 `C:\Tools\scrcpy` 추가
- [ ] 새 PowerShell 창에서 `adb`, `scrcpy` 명령이 풀 경로 없이 동작

### 폰 연결 (USB 직결, 폰 셋업 완료 후)
- [ ] USB 케이블 연결 + 폰 "USB 디버깅 허용" 승인
- [ ] `adb devices` 결과에 `<시리얼>     device` 표시
- [ ] `scrcpy` 실행 시 폰 화면이 PC 창에 미러링됨
- [ ] (PoC 단계에서) `adb reverse tcp:8000 tcp:8000` 등록 후 `adb reverse --list` 확인

---

## 10. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v1.0 | 2026-05-06 | 팀 (3인) | 초안 작성. Qt 6 SDK + ADB Platform-Tools + scrcpy + PATH 등록 + 폰 무선 연결 절차 정리 |
| v1.1 | 2026-05-06 | 팀 (3인) | 7장 채널 분기 추가 — **7.A USB 직결(권장, MVP 기본 채널) ↔ 7.B 무선 디버깅(같은 네트워크 시)**. **`adb reverse tcp:8000 tcp:8000`** 포트 포워딩 절차 추가. 7.B 트러블슈팅에 `protocol fault` 케이스 안내. 체크리스트의 폰 연결 항목을 USB 직결 기반으로 갱신. (PC 사내망 vs 폰 외부망 환경에서 무선 페어링 실패 케이스 → USB 단일화 결정 반영) |
| v1.2 | 2026-05-07 | 팀 (3인) | **8장 Qt Creator 첫 빌드·실행 절차 신규** — 프로젝트 열기 / Configure / 빌드(Ctrl+B) / 실행(Ctrl+R) / 빌드 에러 트러블슈팅 5종 / 빌드 산출물 위치 정리. 기존 9·10장 번호 한 칸씩 밀림. |
| **(2026-05-13 메모)** | | | 메인서버 측 인터페이스 확정 — 사진 흐름은 `/v1/media/intent` → 보관 PC 직접 PUT → `/v1/media/commit` 3단계 (MediaApi v0.2). 클라 `Client/MainServerClient/MediaApiClient` 갱신 필요. 상세: [Api/MediaApi.md](../Api/MediaApi.md). 시스템 시작 명령은 [system_prompt.md](../system_prompt.md) 정본 참조. |
| **(2026-05-13 클라 통합)** | | | `MediaApiClient` 에 `request_intent` / `put_to_storage` / `commit_upload` 3개 메소드 추가, `PillController::on_capture_succeeded` 콜백 체인 4단계 (intent → PUT 보관 PC → commit → identify) 교체 완료. 사진 본체가 메인서버를 통과하지 않는 정상 흐름 작동. 레거시 `upload_image` 는 fallback 으로 유지. |
