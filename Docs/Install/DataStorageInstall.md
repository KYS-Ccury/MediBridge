# 데이터 보관 PC 설치 매뉴얼 (Drogon C++ 미니 서버)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.1 |
| **작성일** | 2026-05-13 |
| **대상** | 데이터 보관 PC (10.10.10.122 : 8004) |
| **OS** | Ubuntu 24.04 LTS |
| **역할** | 사진 PUT/GET + HMAC 토큰 검증 — 메인서버 통과 X (컨트롤 평면 ↔ 데이터 평면 분리) |
| **관련 문서** | [Api/MediaApi v0.2](../Api/MediaApi.md), [시스템 흐름 v3.1 §17](../시스템%20흐름%20정리본_ver3.md), [system_prompt.md](../system_prompt.md) |

---

## 1. 책임 범위

**할 일**:
- `PUT /storage/photos/{anon}/{photo_id}.{ext}` — 클라 PC 또는 Vision PC 가 사진 본체 업로드 (메인 발급 put_token 검증)
- `GET /storage/photos/{anon}/{photo_id}.{ext}` — Vision PC 가 추론용 사진 다운로드 (메인 발급 get_token 검증)
- `GET /health` — 디스크 여유 확인 (메인서버 모니터링)
- atomic write (.tmp → rename) — 반쪽 파일 방지
- 11가지 보안 검증 — [MediaApi.md §2](../Api/MediaApi.md) 참조

**안 하는 일**:
- DB 직접 접근 (메인 서버가 `photo_storage` 관리)
- 토큰 발급 (메인 서버 `StorageTokenIssuer` 가 발급, 본 서버는 검증만)
- 외부 인터넷 통신
- 인증·세션 관리 (토큰이 self-contained)

---

## 2. 의존성 설치

### 2.1 시스템 패키지

```bash
sudo apt update
sudo apt install -y \
    build-essential cmake git pkg-config \
    libssl-dev libdrogon-dev libjsoncpp-dev \
    libc-ares-dev libyaml-cpp-dev uuid-dev zlib1g-dev libbrotli-dev
```

> 보관 PC 는 DB 미사용이라 `libmariadb-dev` 등 DB 라이브러리는 **선택**. Drogon 의존성으로 자동 설치될 수 있음.

### 2.2 검증

```bash
cmake --version    # ≥ 3.16
dpkg -s libdrogon-dev | grep Version    # 1.8.x 이상
```

---

## 3. 소스 가져오기 + 빌드

```bash
cd /path/to/MediBridge

# 빌드
cd DataStorageServer
mkdir -p build-wsl && cd build-wsl
cmake ..
make -j$(nproc)
```

성공 시 `MediBridgeDataStorageServer` 실행 파일 생성.

### 3.1 빌드 트러블슈팅

| 증상 | 해결 |
| --- | --- |
| `Drogon::Drogon 찾을 수 없음` | `sudo apt install libdrogon-dev` |
| `openssl/hmac.h 없음` | `sudo apt install libssl-dev` |
| `JsonCpp 링크 실패` | `sudo apt install libjsoncpp-dev` |

---

## 4. 환경변수 (필수)

| 이름 | 기본값 | 비고 |
| --- | --- | --- |
| `DATASTORAGE_PORT` | `8004` | HTTP 포트 |
| `DATASTORAGE_THREAD_NUM` | `0` (CPU 코어 수) | Drogon 워커 |
| `DATASTORAGE_ROOT` | `/var/lib/medibridge_storage` | 파일 저장 루트 |
| `DATASTORAGE_MAX_BYTES` | `10485760` | 업로드 최대 (10MB) |
| **`MEDIBRIDGE_STORAGE_SECRET`** | **(필수)** | **메인서버와 반드시 동일한 32+ 바이트 시크릿** |
| `MEDIBRIDGE_ENV` | `development` | `production` 시 시크릿 미설정 거부 (abort) |

### 4.1 시크릿 보안

- **JWT 시크릿 (`MEDIBRIDGE_JWT_SECRET`) 과 절대 동일 X.** 별도 시크릿 사용.
- 32 바이트 이상 (256 bit HS256 권장).
- production 환경에서는 반드시 환경변수 또는 systemd EnvironmentFile 로 주입.
- 노출 시 임의 사진 위조 가능 — 즉시 회전.

### 4.2 권장 systemd EnvironmentFile

```ini
# /etc/medibridge/datastorage.env  (chmod 0600)
DATASTORAGE_PORT=8004
DATASTORAGE_ROOT=/var/lib/medibridge_storage
DATASTORAGE_MAX_BYTES=10485760
MEDIBRIDGE_STORAGE_SECRET=<32+ 바이트 무작위 시크릿>
MEDIBRIDGE_ENV=production
```

---

## 5. 디스크 준비

```bash
sudo mkdir -p /var/lib/medibridge_storage
sudo chown $USER:$USER /var/lib/medibridge_storage
sudo chmod 0750 /var/lib/medibridge_storage
```

권장 디스크 — **대용량** (POC: 100GB+, 운영: 1TB+). SSD 권장 (PUT/GET 빈도 높음).

디스크 구조:
```
/var/lib/medibridge_storage/
├── anon_test_001/
│   ├── ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d.jpg
│   └── ph_5d97cb02d7e4f9...png
├── anon_test_002/
│   └── ...
```

> 파일명 패턴 `<anonymous_id>/<photo_id>.<ext>` — **user_id 노출 0**.

---

## 6. 빠른 시작 (시연/스모크)

### 6.1 부팅 스크립트로 띄우기

```bash
bash /path/to/MediBridge/DataStorageServer/Scripts/datastorage-up.sh
```

성공:
```
[ds-up] /health OK (HH:MM:SS)
============================================================
  데이터 보관 PC 가동 완료
  포트          : 8004
  storage_root  : /tmp/medibridge_storage_smoke
============================================================
```

### 6.2 동작 확인

```bash
curl -s http://127.0.0.1:8004/health | jq
```

→ `status: ok`, `disk_free_bytes`, `storage_root_exists: true` 응답.

### 6.3 풀 시나리오 스모크 (메인 + 보관 모두 가동)

```bash
bash /path/to/MediBridge/DataStorageServer/Scripts/smoke_test.sh
```

→ 로그인 → intent → 더미 JPG 생성 → PUT → commit → 디스크 확인 → 보안 검증(만료 토큰, jti mismatch) 까지 8단계 자동 실행.

### 6.4 종료

```bash
pkill -x MediBridgeDataStorageServer
```

---

## 7. LAN 노출 (WSL 환경 — 시연 PC 가 WSL 안에서 보관 PC 도 띄우는 경우)

같은 PC 에서 WSL 안에 보관 PC 를 띄운다면 Windows 가 8004 포트도 LAN 으로 노출 해야 다른 PC 클라가 접근 가능.

[`MainServer/Scripts/medibridge-portproxy.ps1`](../../MainServer/Scripts/medibridge-portproxy.ps1) 가 **8001 + 8004 두 포트를 일괄 등록** 해줌. 한 번만 실행:

```powershell
# Windows 관리자 PowerShell
cd C:\Users\LMS\Desktop\Project\MediBridge\MainServer\Scripts
.\medibridge-portproxy.ps1
```

> 본 PC 의 LAN IP 가 `10.10.10.97` 이면 다른 PC 에서 `http://10.10.10.97:8004/health` 로 접근 가능.

---

## 8. 운영 systemd 서비스 등록 (production)

```ini
# /etc/systemd/system/medibridge-datastorage.service
[Unit]
Description=MediBridge Data Storage Server
After=network-online.target

[Service]
Type=simple
User=medibridge
WorkingDirectory=/opt/medibridge/DataStorageServer
EnvironmentFile=/etc/medibridge/datastorage.env
ExecStart=/opt/medibridge/DataStorageServer/build-wsl/MediBridgeDataStorageServer
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now medibridge-datastorage
sudo systemctl status medibridge-datastorage
```

---

## 9. 트러블슈팅

| 증상 | 원인 / 해결 |
| --- | --- |
| PUT 요청 401 `INVALID_TOKEN` | 메인서버와 `MEDIBRIDGE_STORAGE_SECRET` 가 다름 → 동일하게 맞추기. 또는 토큰 TTL(300s) 초과. |
| PUT 요청 403 `PHOTO_ID_MISMATCH` | URL 의 photo_id 가 토큰 jti 와 다름 — 클라가 메인 응답의 storage_url 그대로 쓰는지 확인 |
| PUT 요청 413 `SIZE_EXCEEDED` | Content-Length > `DATASTORAGE_MAX_BYTES`. 환경변수 조정 또는 사진 압축 |
| `storage_root 생성 실패` | 권한 부족 → `chown medibridge:medibridge /var/lib/medibridge_storage` |
| `address already in use` | 다른 프로세스가 8004 사용. `lsof -i :8004` 로 확인 후 종료 |
| 디스크 가득 참 | `df -h /var/lib/medibridge_storage` 확인 — 청소 잡으로 EXPIRED 마킹된 row 의 실제 파일은 별도 디스크 청소 필요 (선택) |

### 로그

```bash
tail -f /tmp/datastorage.log                # 시연 환경
journalctl -u medibridge-datastorage -f      # systemd 환경
```

---

## 10. 셋업 완료 체크리스트

- [ ] `MediBridgeDataStorageServer` 빌드 성공
- [ ] `MEDIBRIDGE_STORAGE_SECRET` 환경변수 설정 (32+ 바이트, 메인서버와 동일)
- [ ] `DATASTORAGE_ROOT` 디렉터리 존재 + 쓰기 권한
- [ ] `curl http://127.0.0.1:8004/health` → 200 OK
- [ ] 메인서버와 동시 가동 + `smoke_test.sh` 풀 시나리오 통과
- [ ] (LAN 노출 시) `curl http://10.10.10.97:8004/health` → 200 OK
- [ ] (production) systemd 등록 + 재부팅 후 자동 시작 확인

---

## 11. 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-13 | 초안 — Drogon 미니 서버 빌드·환경변수·portproxy·systemd. 11가지 보안 검증 + atomic write + HMAC 토큰 검증 명시. |
