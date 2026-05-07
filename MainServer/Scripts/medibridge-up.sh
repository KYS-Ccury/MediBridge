#!/usr/bin/env bash
# =====================================================
# medibridge-up.sh — WSL 안에서 메인 서버 시연 환경 띄우기
# =====================================================
# 사용:
#   bash /mnt/c/Users/LMS/Desktop/Project/MediBridge/MainServer/Scripts/medibridge-up.sh
#
# 동작:
#   1) MariaDB 서비스 시작 (이미 떠 있으면 무시)
#   2) 환경변수 설정 (TestMode + DB 비밀번호 + JWT 시크릿)
#   3) 메인 서버 백그라운드 실행, 로그 → /tmp/medibridge.log
#   4) /health 에 1초 간격 ping 해서 부팅 확인
# =====================================================
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build-wsl/MediBridgeMainServer"
LOG="/tmp/medibridge.log"

# ----- 0. 빌드 산출물 점검 -----
if [[ ! -x "${BIN}" ]]; then
  echo "ERROR: ${BIN} 가 없습니다. 먼저 다음을 실행하세요:" >&2
  echo "  cd ${ROOT} && mkdir -p build-wsl && cd build-wsl && cmake .. && make -j\$(nproc)" >&2
  exit 1
fi

# ----- 1. MariaDB 시작 -----
if ! mysqladmin ping >/dev/null 2>&1; then
  echo "[up] MariaDB 시작..."
  sudo service mariadb start
  sleep 1
fi
mysqladmin ping 2>&1 | head -1

# ----- 2. 기존 인스턴스 정리 -----
if pgrep -f MediBridgeMainServer >/dev/null 2>&1; then
  echo "[up] 기존 MediBridgeMainServer 종료..."
  pkill -f MediBridgeMainServer
  sleep 0.5
fi

# ----- 3. 환경변수 + 백그라운드 실행 -----
export MEDIBRIDGE_DB_PASSWORD='smoke_pw_change_me'
export MEDIBRIDGE_TEST_MODE='true'
export MEDIBRIDGE_JWT_SECRET='at_least_32_bytes_long_secret_for_smoke_test_xx'
export MEDIBRIDGE_ENV='development'

cd "${ROOT}"
nohup "${BIN}" > "${LOG}" 2>&1 &
PID=$!
echo "[up] MediBridgeMainServer 시작 — PID ${PID}, 로그: ${LOG}"

# ----- 4. 부팅 확인 (최대 10초) -----
for i in $(seq 1 20); do
  if curl -fsS http://127.0.0.1:8001/health -o /dev/null 2>/dev/null; then
    echo "[up] /health OK ($(date +%T))"
    break
  fi
  sleep 0.5
done

# ----- 5. WSL IP 안내 -----
WSL_IP=$(hostname -I | awk '{print $1}')
echo
echo "============================================================"
echo "  메인 서버 가동 완료"
echo "  WSL IP        : ${WSL_IP}"
echo "  로컬 접근     : http://127.0.0.1:8001/health"
echo "  LAN 접근(팀)  : http://10.10.10.97:8001/health"
echo "                  (Windows portproxy 가 등록돼 있어야 함)"
echo "  로그          : tail -f ${LOG}"
echo "  종료          : pkill -f MediBridgeMainServer"
echo "============================================================"
