#!/usr/bin/env bash
# =====================================================
# medibridge-up-prod.sh — 메인서버 Production 모드 가동
# =====================================================
# medibridge-up.sh 의 production 버전:
#   - MEDIBRIDGE_TEST_MODE='false'  ← 핵심 차이
#   - 추론서버 URL 명시 export
#   - Pdma 키는 선택 (미설정 시 e약은요 lazy 호출 graceful skip)
#
# 사용:
#   bash MainServer/Scripts/medibridge-up-prod.sh
# =====================================================
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build-wsl/MediBridgeMainServer"
LOG="/tmp/medibridge.log"

if [[ ! -x "${BIN}" ]]; then
  echo "ERROR: ${BIN} 가 없습니다. 먼저 빌드: cd ${ROOT}/build-wsl && make -j" >&2
  exit 1
fi

# MariaDB
if ! mysqladmin ping >/dev/null 2>&1; then
  echo "[prod-up] MariaDB 시작..."
  sudo service mariadb start
  sleep 1
fi

# 기존 인스턴스 정리
if pgrep -f MediBridgeMainServer >/dev/null 2>&1; then
  echo "[prod-up] 기존 MediBridgeMainServer 종료..."
  pkill -f MediBridgeMainServer || true
  sleep 0.5
fi

# Production 환경변수
export MEDIBRIDGE_DB_PASSWORD='smoke_pw_change_me'
export MEDIBRIDGE_TEST_MODE='false'             # ★ Production
export MEDIBRIDGE_JWT_SECRET='at_least_32_bytes_long_secret_for_smoke_test_xx'
export MEDIBRIDGE_STORAGE_SECRET='at_least_32_bytes_long_storage_secret_xxx_yy'
export MEDIBRIDGE_STORAGE_BASE_URL='http://10.10.10.122:8004'
export MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL='10'
export MEDIBRIDGE_ENV='development'

# 추론서버 URL — 명시 (Config.h 기본값 동일하지만 안전)
export MEDIBRIDGE_INFERENCE_LLM_BASE='http://10.10.10.128:8002'
export MEDIBRIDGE_INFERENCE_VISION_BASE='http://10.10.10.120:8003'

# Pdma 키 (식약처 OpenAPI) — 미설정 시 e약은요 lazy 호출 graceful skip
# export MEDIBRIDGE_PDMA_KEY='발급받은_식약처_OpenAPI_키'

cd "${ROOT}"
nohup "${BIN}" > "${LOG}" 2>&1 &
PID=$!
echo "[prod-up] Production 가동 — PID ${PID}, 로그: ${LOG}"

# 부팅 확인
for i in $(seq 1 20); do
  if curl -fsS http://127.0.0.1:8001/health -o /dev/null 2>/dev/null; then
    echo "[prod-up] /health OK ($(date +%T))"
    break
  fi
  sleep 0.5
done

# TestMode 비활성 검증
HEALTH=$(curl -s http://127.0.0.1:8001/health)
if echo "$HEALTH" | grep -q "test_mode_active"; then
  echo "[prod-up] ⚠ /health 에 test_mode_active reason 여전히 존재 — 코드 확인 필요"
  echo "    $HEALTH"
else
  echo "[prod-up] ✅ Production 모드 확인 — test_mode_active reason 없음"
fi

WSL_IP=$(hostname -I | awk '{print $1}')
echo ""
echo "============================================================"
echo "  메인 서버 Production 모드 가동"
echo "  TEST_MODE     : false"
echo "  LLM PC URL    : http://10.10.10.128:8002"
echo "  Vision PC URL : http://10.10.10.120:8003"
echo "  보관 PC URL   : http://10.10.10.122:8004"
echo "  WSL IP        : ${WSL_IP}"
echo "  LAN           : http://10.10.10.97:8001/health"
echo "============================================================"
