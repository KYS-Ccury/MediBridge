# 메인(운용) 서버 설치 매뉴얼 (Ubuntu 24.04)

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 메인(운용) 서버 PC 설치 매뉴얼 |
| **대상 OS** | Ubuntu 24.04 LTS |
| **역할** | FastAPI 백엔드, MariaDB(사용자 계정 + 약 풀 + 복약 이력 + 식약처 데이터), 위험 안내 템플릿, 프롬프트 인젝션 방어 |
| **버전** | v0.1 (스켈레톤) |
| **작성일** | 2026-05-06 |
| **작성자** | 팀 (3인) |

> ⏳ **본 매뉴얼은 스켈레톤 상태.** 메인 서버 셋업 진입 시 각 절을 채워나간다.

---

## 1. 설치 예정 항목 (체크리스트)

| # | 도구 / 컴포넌트 | 용도 | 상태 |
| --- | --- | --- | --- |
| 1 | Python 3.12 + `python3-venv` | FastAPI 실행 환경 | ⏳ |
| 2 | FastAPI + Uvicorn | REST API 백엔드 | ⏳ |
| 3 | MariaDB 11.x | 메인 DB (사용자/약 풀/복약 이력/식약처 데이터) | ⏳ |
| 4 | `python-multipart` | 파일 업로드 처리 | ⏳ |
| 5 | `mariadb` Python 커넥터 | 파이썬 ↔ MariaDB | ⏳ |
| 6 | `passlib[bcrypt]` | 비밀번호 해시 (모듈 6 인증) | ⏳ |
| 7 | `python-jose[cryptography]` | JWT 토큰 발급/검증 | ⏳ |
| 8 | `httpx` | 추론 PC + 식약처 OpenAPI 호출 클라이언트 | ⏳ |
| 9 | `nginx` (선택) | 리버스 프록시·정적 파일 서빙 | ⏳ |
| 10 | `ufw` 방화벽 설정 | 외부 노출 포트 통제 | ⏳ |
| 11 | systemd 서비스 등록 | FastAPI·MariaDB 자동 시작 | ⏳ |

---

## 2. 사전 준비

- Ubuntu 24.04 LTS 설치 완료
- 인터넷 연결 + sudo 권한 사용자
- LAN 환경 — 클라 PC, 추론 서버와 같은 네트워크
- 고정 IP 권장 (클라 PC가 명시적 IP로 접속)

---

## 3. 설치 절차 (예정)

### 3.1 시스템 패키지 업데이트

```bash
# (예정) sudo apt update && sudo apt upgrade -y
```

### 3.2 Python + 가상환경

```bash
# (예정) sudo apt install -y python3.12 python3.12-venv python3-pip
# (예정) python3 -m venv ~/medibridge-main-venv
# (예정) source ~/medibridge-main-venv/bin/activate
```

### 3.3 FastAPI 의존성 설치

```bash
# (예정) pip install fastapi 'uvicorn[standard]' python-multipart \
#        mariadb 'passlib[bcrypt]' 'python-jose[cryptography]' httpx
```

### 3.4 MariaDB 설치 + 초기 설정

```bash
# (예정) sudo apt install -y mariadb-server mariadb-client
# (예정) sudo mysql_secure_installation
# (예정) sudo systemctl enable --now mariadb
```

### 3.5 DB 스키마 적용

DB ERD v2 ([../DB_ERD_ver2.md](../DB_ERD_ver2.md)) 의 `CREATE TABLE` 문을 적용.

```bash
# (예정) sudo mysql -u root -p
# (예정) > CREATE DATABASE medibridge CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
# (예정) > CREATE USER 'medibridge_app'@'localhost' IDENTIFIED BY '<강력한 비밀번호>';
# (예정) > GRANT ALL ON medibridge.* TO 'medibridge_app'@'localhost';
# (예정) 이후 DB_ERD_ver2.md 의 모든 CREATE TABLE 실행
```

### 3.6 식약처 데이터 일괄 적재

낱알식별·DUR 품목·DUR 성분 CSV/EXCEL 파일을 식약처 OpenData에서 다운로드 후 MariaDB에 적재.

```bash
# (예정) 적재 스크립트 위치: 본 프로젝트 코드 저장소의 scripts/ImportPdmaData.py 등
```

### 3.7 방화벽 설정

```bash
# (예정) sudo ufw allow 8000/tcp  # FastAPI
# (예정) sudo ufw enable
```

### 3.8 systemd 서비스 등록

```bash
# (예정) /etc/systemd/system/medibridge-api.service 작성 (uvicorn 실행)
# (예정) sudo systemctl enable --now medibridge-api
```

---

## 4. 설치 완료 체크리스트 (예정)

- [ ] Python 3.12 + venv 환경 구성
- [ ] FastAPI 의존성 설치 완료
- [ ] MariaDB 설치 + `mysql_secure_installation` 완료
- [ ] DB 스키마 (DB_ERD v2) 적용
- [ ] 식약처 일괄 적재 데이터 (낱알식별·DUR 품목·DUR 성분) 정상 적재
- [ ] FastAPI 서버 시작 → 클라 PC에서 health check 응답 확인
- [ ] 추론 PC와의 통신 테스트 (REST API)
- [ ] systemd 자동 시작 동작
- [ ] ufw 방화벽 8000/tcp 허용 외 차단

---

## 5. 트러블슈팅 (예정)

| 증상 | 원인 / 해결 |
| --- | --- |
| (해당 시 채움) | (해당 시 채움) |

---

## 6. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v0.1 | 2026-05-06 | 팀 (3인) | 스켈레톤 작성. 설치 예정 항목 + 절차 골격만 명시. 실 셋업 진입 시 명령어·트러블슈팅 채워나감 |
