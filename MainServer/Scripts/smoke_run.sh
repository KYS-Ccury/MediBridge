#!/usr/bin/env bash
# 메인서버를 백그라운드로 띄우고 /health, /v1/auth/login, /v1/pill/pool 호출하여
# 응답을 확인한 뒤 종료한다. WSL 환경 가정.
#
# 실행 전 smoke_setup.sh 가 이미 적용되어 있어야 함.

set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/build-wsl/MediBridgeMainServer"
LOG="/tmp/medibridge-smoke.log"

if [[ ! -x "${BIN}" ]]; then
  echo "ERROR: ${BIN} 가 없습니다. 먼저 cmake/make 로 빌드하세요." >&2
  exit 1
fi

# 필수 환경변수
export MEDIBRIDGE_DB_PASSWORD='smoke_pw_change_me'
export MEDIBRIDGE_TEST_MODE='true'
export MEDIBRIDGE_JWT_SECRET='at_least_32_bytes_long_secret_for_smoke_test_xx'
export MEDIBRIDGE_ENV='development'

# 기존 인스턴스 청소
pkill -f MediBridgeMainServer 2>/dev/null || true
sleep 0.5

echo "[smoke] 서버 시작 (백그라운드, 로그: ${LOG})"
cd "${ROOT}"
"${BIN}" > "${LOG}" 2>&1 &
PID=$!

trap "kill ${PID} 2>/dev/null || true" EXIT

# 부팅 대기 — /health 가 응답할 때까지 최대 10초
for i in $(seq 1 20); do
  if curl -fsS http://127.0.0.1:8001/health -o /tmp/health.json 2>/dev/null; then
    break
  fi
  sleep 0.5
done

echo
echo "=== /health ==="
cat /tmp/health.json | python3 -m json.tool 2>/dev/null || cat /tmp/health.json
echo

echo "=== POST /v1/auth/login (test@medibridge.local / test1234) ==="
LOGIN=$(curl -s -X POST http://127.0.0.1:8001/v1/auth/login \
     -H "Content-Type: application/json" \
     -d '{"email":"test@medibridge.local","password":"test1234"}')
echo "${LOGIN}" | python3 -m json.tool 2>/dev/null || echo "${LOGIN}"
TOKEN=$(echo "${LOGIN}" | python3 -c 'import json,sys; print(json.load(sys.stdin).get("access_token",""))')

if [[ -z "${TOKEN}" ]]; then
  echo "WARN: access_token 미발급. 이후 인증 호출 생략."
else
  echo
  echo "=== GET /v1/pill/pool (Bearer ${TOKEN:0:30}…) ==="
  curl -s -H "Authorization: Bearer ${TOKEN}" \
       http://127.0.0.1:8001/v1/pill/pool \
       | python3 -m json.tool 2>/dev/null
fi

echo
echo "=== POST /v1/pill/onboarding/normalize (utterance='타이레놀 등록할게') ==="
if [[ -n "${TOKEN}" ]]; then
  curl -s -X POST http://127.0.0.1:8001/v1/pill/onboarding/normalize \
       -H "Authorization: Bearer ${TOKEN}" \
       -H "Content-Type: application/json" \
       -d '{"utterance_text":"타이레놀 등록할게","round":1}' \
       | python3 -m json.tool 2>/dev/null
fi

echo
echo "=== 서버 로그 마지막 10줄 ==="
tail -10 "${LOG}" || true

echo
echo "[smoke] 종료"
