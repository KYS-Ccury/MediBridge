#!/usr/bin/env bash
# =====================================================
# datastorage-up.sh — 데이터 보관 PC 미니 서버 띄우기
# =====================================================
# 사용:
#   bash MediBridge/DataStorageServer/Scripts/datastorage-up.sh
#
# 동작:
#   1) storage_root 디렉터리 확인/생성 (WSL: /tmp/medibridge_storage_smoke)
#   2) 환경변수 설정 (메인서버와 동일한 STORAGE_SECRET)
#   3) 백그라운드 실행, 로그 → /tmp/datastorage.log
#   4) /health 1초 간격 ping
# =====================================================
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build-wsl/MediBridgeDataStorageServer"
LOG="/tmp/datastorage.log"

# ----- 0. 빌드 산출물 점검 -----
if [[ ! -x "${BIN}" ]]; then
  echo "ERROR: ${BIN} 없음. 먼저 빌드하세요:" >&2
  echo "  cd ${ROOT} && mkdir -p build-wsl && cd build-wsl && cmake .. && make -j\$(nproc)" >&2
  exit 1
fi

# ----- 1. 기존 인스턴스 정리 -----
if pgrep -f MediBridgeDataStorageServer >/dev/null 2>&1; then
  echo "[ds-up] 기존 인스턴스 종료..."
  pkill -f MediBridgeDataStorageServer
  sleep 0.5
fi

# ----- 2. 환경변수 + 디렉터리 -----
# WSL 시연용 — /tmp 하위 (재부팅 시 초기화). 실 PC 배포 시 /var/lib/medibridge_storage
export DATASTORAGE_PORT='8004'
export DATASTORAGE_ROOT='/tmp/medibridge_storage_smoke'
export MEDIBRIDGE_STORAGE_SECRET='at_least_32_bytes_long_storage_secret_xxx_yy'
export MEDIBRIDGE_ENV='development'

mkdir -p "${DATASTORAGE_ROOT}"

# ----- 3. 백그라운드 실행 -----
cd "${ROOT}"
nohup "${BIN}" > "${LOG}" 2>&1 &
PID=$!
echo "[ds-up] DataStorageServer 시작 — PID ${PID}, 로그: ${LOG}"

# ----- 4. /health 확인 -----
for i in $(seq 1 20); do
  if curl -fsS http://127.0.0.1:8004/health -o /dev/null 2>/dev/null; then
    echo "[ds-up] /health OK ($(date +%T))"
    break
  fi
  sleep 0.5
done

echo
echo "============================================================"
echo "  데이터 보관 PC 가동 완료"
echo "  포트          : 8004"
echo "  storage_root  : ${DATASTORAGE_ROOT}"
echo "  로그          : tail -f ${LOG}"
echo "  종료          : pkill -f MediBridgeDataStorageServer"
echo "============================================================"
