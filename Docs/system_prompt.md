# MediBridge — 시스템 운영 명령어 모음

> 메인서버 / 데이터 보관 PC / DB / 빌드 / 모니터링 등 자주 쓰는 명령어 정본.
> 모든 명령어는 **WSL Ubuntu 24.04** 환경 기준. Windows PowerShell 에서 호출할 때는 `wsl -d Ubuntu -e bash -c "..."` 로 감싸면 됨.

---

## 1. 빠른 시작 — 처음 보는 사람용 단계 가이드

전체 시스템을 처음 띄우는 순서. **단계마다 어디서 실행하는지** 명시.

| # | 단계 | 어디서 | 빈도 |
|---|---|---|---|
| 0 | DB 셋업 | WSL Bash (root) | **최초 1회만** |
| 1 | 메인서버 시작 | WSL Bash | 매번 |
| 2 | 데이터 보관 PC 시작 | WSL Bash | 매번 |
| 3 | WSL → LAN 포트 노출 (portproxy) | Windows PowerShell (관리자) | **최초 1회만** |
| 4 | LAN 동작 확인 | 같은 PC 또는 다른 PC | (선택) |
| 5 | 클라이언트 실행 | 다른 PC (또는 같은 PC) | 매번 |

---

### 단계 0 — DB 셋업 (최초 1회만)

> 처음 이 프로젝트를 받아서 셋업할 때만. 다음부터는 건너뛰어도 됨.

**WSL Ubuntu 터미널** 에서:

```bash
wsl -d Ubuntu -u root -e bash -c "bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/Scripts/smoke_setup.sh"
```

→ DB·사용자·스키마(`001_init_schema.sql`)·마이그레이션(`002_photo_storage_intent.sql`)·시드(`dev_seed.sql`) 모두 적용.

성공 메시지 예:
```
[smoke] 4) 적용 검증
pills: 10  users: 2  pseudonyms: 2  pool_rows: 4  intake_logs: 6
[smoke] DONE
```

---

### 단계 1 — 메인서버 시작

**WSL Ubuntu 터미널** 에서:

```bash
bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/Scripts/medibridge-up.sh
```

성공 메시지:
```
[up] /health OK (HH:MM:SS)
============================================================
  메인 서버 가동 완료
  로컬 접근  : http://127.0.0.1:8001/health
  LAN 접근   : http://10.10.10.97:8001/health
============================================================
```

> `mysqladmin: connect to server at 'localhost' failed` 경고가 떠도 무시 — MariaDB ping 체크 부분이고 실제 DB 는 정상 연결됩니다.

---

### 단계 2 — 데이터 보관 PC 시작

**같은 WSL 터미널** (메인서버 띄운 직후) 에서:

```bash
bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/DataStorageServer/Scripts/datastorage-up.sh
```

성공 메시지:
```
[ds-up] /health OK (HH:MM:SS)
============================================================
  데이터 보관 PC 가동 완료
  포트          : 8004
  storage_root  : /tmp/medibridge_storage_smoke
============================================================
```

---

### 단계 3 — WSL → LAN 포트 노출 (최초 1회만)

> WSL 의 메인서버·보관 PC 는 기본적으로 LAN 다른 PC 에서 접근 불가.
> portproxy 한 번 등록하면 재부팅 후에도 유지됨 (다음부터는 건너뛰어도 됨).

**Windows PowerShell 을 관리자 권한으로 열기**:
- 시작 메뉴 → "PowerShell" 검색 → **우클릭 → 관리자 권한으로 실행**

처음 한 번만 실행 정책 풀기:
```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```

그 다음 portproxy 등록:
```powershell
cd C:\Users\LMS\Desktop\Project\MediBridge\MainServer\Scripts
.\medibridge-portproxy.ps1
```

→ **8001 (메인서버) + 8004 (보관 PC) 두 포트를 한 번에 등록** + 방화벽 허용.

성공하면 마지막에 등록 결과 표시:
```
0.0.0.0  8001  172.x.x.x  8001
0.0.0.0  8004  172.x.x.x  8004
```

> WSL IP 가 매번 부팅마다 바뀌면 portproxy 도 재등록 필요. 그땐 같은 스크립트 한 번 더 실행.

---

### 단계 4 — LAN 동작 확인 (선택)

다른 PC 띄우기 전 동작 확인. **같은 PC 의 어디서든 실행 가능**.

WSL Bash:
```bash
curl -s http://10.10.10.97:8001/health | jq
curl -s http://10.10.10.97:8004/health | jq
```

Windows PowerShell:
```powershell
curl http://10.10.10.97:8001/health
curl http://10.10.10.97:8004/health
```

둘 다 `{"status":"ok",...}` JSON 응답이 떨어지면 정상.

다른 PC 에서 사전 진단 (선택):
```powershell
Test-NetConnection -ComputerName 10.10.10.97 -Port 8001
Test-NetConnection -ComputerName 10.10.10.97 -Port 8004
```
→ `TcpTestSucceeded : True` 두 번 뜨면 OK.

---

### 단계 5 — 클라이언트 실행

**다른 PC (또는 같은 PC)** 에서 Qt 클라이언트 빌드 및 실행. 클라는 `Client/Main.cpp` 에 `http://10.10.10.97:8001/` 가 하드코딩되어 있어 자동으로 메인서버를 찾아갑니다.

자세한 클라 셋업: `Docs/Install/ClientPcInstall.md`.

---

### 종료 (모두 끄기)

**WSL Bash**:
```bash
pkill -x MediBridgeMainServer
pkill -x MediBridgeDataStorageServer
```

portproxy 와 방화벽 규칙은 그대로 둬도 무방 — 다음에 서버 켤 때 그대로 재활용.

---

## 1-A. 다음 번부터 — 매번 띄울 때 (두 줄)

단계 0, 3 은 이미 셋업된 상태라면 매번 켤 때는 **WSL Bash 두 줄** 이면 끝:

```bash
bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/Scripts/medibridge-up.sh
bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/DataStorageServer/Scripts/datastorage-up.sh
```

> 단, **WSL 자체를 재시작했다면** WSL IP 가 바뀌므로 단계 3 의 portproxy 스크립트 한 번 더 실행 필요.

---

## 2. 메인서버 (Drogon C++, 8001)

### 시작 / 재시작

```bash
bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/Scripts/medibridge-up.sh
```

스크립트가 기존 인스턴스를 자동 종료 후 새로 띄움 (idempotent).

### 종료

```bash
pkill -x MediBridgeMainServer
```

### 로그

| 용도 | 명령 |
|---|---|
| 실시간 스트리밍 | `tail -f /tmp/medibridge.log` |
| 마지막 100줄 | `tail -100 /tmp/medibridge.log` |
| 전체 (스크롤) | `less /tmp/medibridge.log` |
| 에러만 추적 | `tail -f /tmp/medibridge.log \| grep -E "ERROR\|FATAL"` |
| 요청·응답만 | `tail -f /tmp/medibridge.log \| grep -E "REQ\|RES"` |
| 청소 잡 동작 | `tail -f /tmp/medibridge.log \| grep "Cleanup"` |
| 특정 API | `tail -f /tmp/medibridge.log \| grep "/v1/pill"` |

### 빌드

```bash
cd /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer
mkdir -p build-wsl && cd build-wsl
cmake ..                    # 최초 1회
make -j$(nproc)
```

### 코드 수정 후 — 빌드 + 재실행 한 줄

```bash
cd /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/build-wsl && \
  make -j$(nproc) && \
  bash ../Scripts/medibridge-up.sh
```

### 동작 확인

| 위치 | URL | 기대 |
|---|---|---|
| 같은 PC | `http://127.0.0.1:8001/health` | `{"status":"ok",...}` |
| LAN 다른 PC | `http://10.10.10.97:8001/health` | 같은 JSON (portproxy 등록 후) |

```bash
curl -s http://127.0.0.1:8001/health | jq
```

---

## 3. 데이터 보관 PC (Drogon C++ 미니 서버, 8004)

### 시작

```bash
bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/DataStorageServer/Scripts/datastorage-up.sh
```

저장 경로 기본: `/tmp/medibridge_storage_smoke/<anonymous_id>/<photo_id>.<ext>`
(`DATASTORAGE_ROOT` 환경변수로 변경 가능. 실 배포 시 `/var/lib/medibridge_storage`)

### 종료

```bash
pkill -x MediBridgeDataStorageServer
```

### 로그

```bash
tail -f /tmp/datastorage.log
```

### 빌드

```bash
cd /mnt/c/Users/LMS/Desktop/Project/MediBridge/DataStorageServer
mkdir -p build-wsl && cd build-wsl
cmake ..                    # 최초 1회
make -j$(nproc)
```

### 동작 확인

```bash
curl -s http://127.0.0.1:8004/health | jq
```

→ `status: ok`, `disk_free_bytes`, `storage_root_exists: true` 응답.

### 저장 파일 확인

```bash
ls -la /tmp/medibridge_storage_smoke/anon_test_001/
```

### 풀 시나리오 스모크 테스트 (메인 + 보관 모두 가동된 상태에서)

```bash
bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/DataStorageServer/Scripts/smoke_test.sh
```

→ 로그인 → intent → PUT → commit → 보안 검증(만료 토큰·jti mismatch) 까지 8단계.

---

## 4. DB (MariaDB)

### MariaDB 시작 / 상태

```bash
sudo service mariadb start          # 시작
mysqladmin ping -u root             # 떠있는지 확인 (응답: mysqld is alive)
```

### DB 콘솔 진입

```bash
# 앱 계정 (제한 권한)
mysql -u medibridge_app -psmoke_pw_change_me medibridge

# root (관리자)
sudo mysql -u root medibridge
```

### 자주 보는 테이블

```sql
-- 시드 사용자
SELECT user_id, email, user_name FROM users;

-- 가명화 매핑
SELECT user_id, anonymous_id, severed_at FROM pseudonym_map;

-- 사용자 약 풀
SELECT ump.item_code, p.drug_name, ump.is_active
FROM user_medication_pool ump
JOIN pill_identification p ON p.item_code = ump.item_code
WHERE ump.anonymous_id = 'anon_test_001';

-- 사진 메타 (최근순)
SELECT photo_id, anonymous_id, status, expires_at, committed_at
FROM photo_storage ORDER BY uploaded_at DESC LIMIT 10;

-- DUR 페어
SELECT dur_id, base_item_code, target_item_code, dur_type
FROM dur_interaction_cache;
```

### 시드 사용자 (테스트용)

| email | password | user_id |
|---|---|---|
| `test@medibridge.local` | `test1234` | `test_user_001` |
| `demo@medibridge.local` | `demo1234` | `test_user_002` |

→ `test_user_001` 에 약 풀 4건 시드 (타이레놀500 / 이부프로펜200 / 베아제 / 아스피린).

### DB 초기화 (위험 — 시드 다시 적용)

```bash
sudo mysql -u root -e "DROP DATABASE medibridge; CREATE DATABASE medibridge CHARACTER SET utf8mb4;"
wsl -d Ubuntu -u root -e bash -c "bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/Scripts/smoke_setup.sh"
```

---

## 5. 식약처 CSV 일괄 적재

운영 중에는 호출되지 않음 — 식약처에서 CSV 받았을 때만 import.

### 의존성 설치 (최초 1회)

```bash
sudo apt install python3-pymysql
# .xlsx 도 import 한다면:
sudo apt install python3-openpyxl
```

### 적재 (UPSERT — PK 충돌 시 갱신)

```bash
cd /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/Scripts

# 환경변수 — DB 비밀번호 (기본값과 다르면 export)
export MEDIBRIDGE_DB_PASSWORD=smoke_pw_change_me

# 개별
python3 import_pdma.py pill       sample_pdma_pill.csv
python3 import_pdma.py dur        sample_pdma_dur.csv
python3 import_pdma.py overview   sample_pdma_overview.csv
python3 import_pdma.py ingredient ingredient.csv

# 한 번에 4종 (순서: pill, dur, overview, ingredient)
python3 import_pdma.py all pill.csv dur.csv overview.csv [ingredient.csv]
```

→ 식약처 OpenAPI 표준 컬럼명 + 공공데이터포털 한글 컬럼명 모두 자동 인식.

---

## 6. PPTX 아키텍처 도해

### 빌드

```bash
cd /mnt/c/Users/LMS/Desktop/Project/MediBridge/Docs/Architecture
NODE_PATH=$(npm root -g) node build_pptx.js
```

→ `MediBridge_Architecture_MVP.pptx` 생성.

### PDF + JPG 미리보기

```bash
cd /mnt/c/Users/LMS/Desktop/Project/MediBridge/Docs/Architecture
soffice --headless --convert-to pdf MediBridge_Architecture_MVP.pptx
rm -f slide-*.jpg
pdftoppm -jpeg -r 110 MediBridge_Architecture_MVP.pdf slide
```

→ `slide-01.jpg` ~ `slide-10.jpg`.

---

## 7. 네트워크 / LAN

### portproxy 등록 확인 (Windows PowerShell)

```powershell
netsh interface portproxy show v4tov4
```

→ 출력에 `0.0.0.0  8001  ...  8001` 같은 줄이 보여야 LAN 노출 정상.

### portproxy 수동 등록 (관리자 PowerShell)

```powershell
$WslIp = wsl hostname -I | ForEach-Object { $_.Trim().Split(' ')[0] }
netsh interface portproxy add v4tov4 listenport=8001 listenaddress=0.0.0.0 connectport=8001 connectaddress=$WslIp
netsh advfirewall firewall add rule name="MediBridge 8001" dir=in action=allow protocol=TCP localport=8001
```

→ 데이터 보관 PC (8004) 도 같은 패턴.

### portproxy 제거

```powershell
netsh interface portproxy delete v4tov4 listenport=8001 listenaddress=0.0.0.0
```

### 다른 PC 에서 reachable 확인

```powershell
# Windows
Test-NetConnection -ComputerName 10.10.10.97 -Port 8001
```

```bash
# Linux/Mac
curl -v http://10.10.10.97:8001/health
```

| 결과 | 진단 |
|---|---|
| `{"status":"ok",...}` | 정상 |
| Connection refused | portproxy 미등록 또는 메인서버 다운 |
| Timeout | 방화벽 차단 / IP 가 `10.10.10.97` 이 아님 |
| No route to host | LAN 라우팅 자체 불가 |

---

## 8. 환경변수 정본

### MainServer

| 이름 | 기본값 | 비고 |
|---|---|---|
| `MEDIBRIDGE_DB_PASSWORD` | (없음) | `medibridge_app` 계정 비밀번호 |
| `MEDIBRIDGE_JWT_SECRET` | (없음) | HS256 — 32바이트 이상 권장 |
| `MEDIBRIDGE_STORAGE_SECRET` | (없음) | 보관 PC 토큰 — JWT 와 **다른** 32+바이트 |
| `MEDIBRIDGE_STORAGE_BASE_URL` | `http://10.10.10.122:8004` | 데이터 보관 PC URL |
| `MEDIBRIDGE_STORAGE_TOKEN_TTL` | `300` | PUT/GET 토큰 만료 (초) |
| `MEDIBRIDGE_STORAGE_MAX_BYTES` | `10485760` | 업로드 최대 (10MB) |
| `MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL` | `300` | PENDING 청소 잡 인터벌 (초) · 0 = 비활성 |
| `MEDIBRIDGE_PDMA_KEY` | (없음) | 식약처 OpenAPI 키 (운영 시 e약은요 lazy 호출) |
| `MEDIBRIDGE_INFERENCE_LLM_BASE` | `http://10.10.10.120:8002` | LLM PC URL |
| `MEDIBRIDGE_INFERENCE_VISION_BASE` | `http://10.10.10.128:8003` | Vision PC URL |
| `MEDIBRIDGE_TEST_MODE` | `false` | `true` 시 추론·외부저장 우회 → DB seed 응답 |
| `MEDIBRIDGE_ENV` | `development` | `production` 시 시크릿 미설정 거부 (강제 abort) |

### DataStorageServer

| 이름 | 기본값 |
|---|---|
| `DATASTORAGE_PORT` | `8004` |
| `DATASTORAGE_ROOT` | `/var/lib/medibridge_storage` |
| `DATASTORAGE_MAX_BYTES` | `10485760` |
| `MEDIBRIDGE_STORAGE_SECRET` | (메인서버와 **반드시 동일**) |

### 설정 우선순위

```
Config.h 멤버 기본값  <  config.json  <  환경변수 (항상 최우선)
```

샘플: `MainServer/config.sample.json`. 시크릿은 절대 JSON 에 넣지 말 것 — 환경변수 전용 강제 정책.

---

## 9. 빠른 시연 — JWT 받아 식별 시나리오

```bash
# 1) 로그인
JWT=$(curl -s -X POST http://127.0.0.1:8001/v1/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"email":"test@medibridge.local","password":"test1234"}' | jq -r .access_token)
echo "JWT: ${JWT:0:40}..."

# 2) 약 풀 조회
curl -s http://127.0.0.1:8001/v1/pill/pool -H "Authorization: Bearer $JWT" | jq

# 3) 알약 식별 (TestMode 시드 응답)
curl -s -X POST http://127.0.0.1:8001/v1/pill/identify \
  -H "Authorization: Bearer $JWT" -H "Content-Type: application/json" \
  -d '{"image_request_id":"req_demo","include_dur_check":true}' | jq

# 4) 보고서 PDF 다운로드
curl -s -o report.pdf -H "Authorization: Bearer $JWT" \
  "http://127.0.0.1:8001/v1/report/generate?format=pdf&from_date=2025-01-01&to_date=2026-12-31"

# 5) 사진 업로드 흐름 (intent → PUT → commit)
RESP=$(curl -s -X POST http://127.0.0.1:8001/v1/media/intent \
  -H "Authorization: Bearer $JWT" -H 'Content-Type: application/json' \
  -d '{"mime_type":"image/jpeg","size_bytes":160,"purpose":"IDENTIFY"}')
PHOTO_ID=$(echo "$RESP" | jq -r .photo_id)
PUT_URL=$(echo "$RESP" | jq -r .storage_url | sed 's|10.10.10.122|127.0.0.1|')
PUT_TOKEN=$(echo "$RESP" | jq -r .put_token)

curl -s -X PUT "$PUT_URL" -H "Authorization: Bearer $PUT_TOKEN" \
  -H 'Content-Type: image/jpeg' --data-binary @/path/to/photo.jpg

curl -s -X POST http://127.0.0.1:8001/v1/media/commit \
  -H "Authorization: Bearer $JWT" -H 'Content-Type: application/json' \
  -d "{\"photo_id\":\"$PHOTO_ID\"}" | jq
```

---

## 10. 트러블슈팅

### 메인서버 부팅 실패

```bash
# 로그 확인
tail -50 /tmp/medibridge.log

# 흔한 원인
# 1) DB 미가동
mysqladmin ping -u root
sudo service mariadb start

# 2) 포트 점유
lsof -i :8001
pkill -x MediBridgeMainServer

# 3) 빌드 산출물 없음
ls /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/build-wsl/MediBridgeMainServer
```

### `/health` 가 LAN 에서 안 보임

- WSL 환경이면 portproxy 등록 안 됨 → §7 참조
- Windows 방화벽 8001 인바운드 허용 확인
- 메인서버 PC 의 LAN IP 가 `10.10.10.97` 인지 확인 (`ipconfig`)

### 다른 PC 의 클라가 연결 못 함

다른 PC 에서:
```bash
curl -v http://10.10.10.97:8001/health
```
- Refused → 메인서버 다운 또는 portproxy 미등록
- Timeout → 방화벽 또는 IP 불일치
- OK 떨어지면 클라 코드 측 문제

### 보관 PC PUT 이 401 INVALID_TOKEN

- 메인서버와 보관 PC 의 `MEDIBRIDGE_STORAGE_SECRET` 가 **다름** → 동일하게 맞추기
- 토큰 발급 후 300초 (TTL) 지났을 수 있음 → intent 재호출

### DB 비밀번호 변경 후 메인서버 안 뜸

`Scripts/medibridge-up.sh` 의 `MEDIBRIDGE_DB_PASSWORD` 값을 새 비밀번호로 수정.

### 청소 잡이 동작 안 함

- `MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL=0` 이면 비활성
- 로그에서 `[Cleanup]` 라인 확인:
  ```bash
  grep Cleanup /tmp/medibridge.log
  ```
- 부팅 로그에 `청소 잡 등록 — N초 간격` 보이는지 확인

---

## 11. 파일 위치 빠른 참조

| 무엇 | 어디 |
|---|---|
| 메인서버 코드 | `MainServer/` |
| 보관 PC 코드 | `DataStorageServer/` |
| 클라이언트 코드 | `Client/` |
| DB 스키마 | `MainServer/Database/Migrations/` |
| 시드 데이터 | `MainServer/Database/Seeds/dev_seed.sql` |
| 식약처 import 도구 | `MainServer/Scripts/import_pdma.py` |
| 메인서버 부팅 스크립트 | `MainServer/Scripts/medibridge-up.sh` |
| 보관 PC 부팅 스크립트 | `DataStorageServer/Scripts/datastorage-up.sh` |
| portproxy 스크립트 | `MainServer/Scripts/medibridge-portproxy.ps1` |
| 메인서버 로그 | `/tmp/medibridge.log` |
| 보관 PC 로그 | `/tmp/datastorage.log` |
| 아키텍처 PPTX | `Docs/Architecture/MediBridge_Architecture_MVP.pptx` |
| Config 샘플 | `MainServer/config.sample.json` |

---

## 12. 관련 문서

- **API 명세**: `Docs/Api/`
- **시스템 흐름**: `Docs/시스템 흐름 정리본_ver3.md`
- **시스템 연결**: `Docs/시스템_연결구조_ver2.md`
- **DB ERD**: `Docs/DB_ERD_ver4.md`
- **TestMode**: `Docs/TestMode.md`
- **요구사항**: `Docs/요구사항_분석서_ver2.md`
- **개발계획**: `Docs/개발계획서_ver2.md`
- **보안 체크리스트**: `Docs/SecurityChecklist.md`
- **변경 이력**: `Docs/CHANGELOG.md`
