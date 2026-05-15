#!/usr/bin/env bash
# =====================================================
# sync-llm-pc.sh — 본 PC InferenceServer/ → LLM PC 단방향 동기화
# =====================================================
# 사용 (WSL Bash):
#   bash Scripts/sync-llm-pc.sh            # 동기화만
#   bash Scripts/sync-llm-pc.sh --restart  # 동기화 + FastAPI 재시작
#
# 비전PC 와 동일 패턴 (sync-vision-pc.sh 참조).
# git 사용 안 함 — rsync over SSH 단방향.
# =====================================================
set -e

REMOTE_USER="llm-server"
REMOTE_HOST="10.10.10.128"
REMOTE_BASE="/home/llm-server/Desktop/MediBridge"

LOCAL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOCAL_INFERENCE="${LOCAL_ROOT}/InferenceServer"

if [ ! -d "${LOCAL_INFERENCE}" ]; then
    echo "ERROR: ${LOCAL_INFERENCE} 가 없습니다." >&2
    exit 1
fi

echo "[sync] 본 PC : ${LOCAL_INFERENCE}"
echo "[sync] LLM PC: ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/InferenceServer/"
echo ""

# 사전 SSH 도달성 점검
if ! ssh -o BatchMode=yes -o ConnectTimeout=3 "${REMOTE_USER}@${REMOTE_HOST}" 'true' 2>/dev/null; then
    echo "ERROR: SSH 인증 실패. ssh-copy-id ${REMOTE_USER}@${REMOTE_HOST} 먼저 실행하세요." >&2
    exit 2
fi

# rsync — 단방향 (본 PC → LLM PC). --delete 로 본 PC 삭제분도 반영.
# .env / Models / chroma_db / venv / __pycache__ 는 LLM PC 의 것 유지.
rsync -avz --delete \
    --exclude='__pycache__' \
    --exclude='.venv' \
    --exclude='venv' \
    --exclude='chroma_db' \
    --exclude='Models' \
    --exclude='.env' \
    --exclude='*.pyc' \
    --exclude='hf_cache' \
    "${LOCAL_INFERENCE}/" \
    "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/InferenceServer/"

echo ""
echo "[sync] 동기화 완료. LLM PC 의 .env / Models / venv 는 그대로 유지."
echo ""

# 동일성 검증
LOCAL_HASH=$(cd "${LOCAL_INFERENCE}" && find . -type f \( -name '*.py' -o -name '*.txt' -o -name '*.md' -o -name '.env.sample' \) \
    -not -path './__pycache__/*' -not -path './.venv/*' -not -path './chroma_db/*' -not -path './Models/*' \
    | sort | xargs sha256sum 2>/dev/null | sha256sum | awk '{print $1}')
REMOTE_HASH=$(ssh "${REMOTE_USER}@${REMOTE_HOST}" \
    "cd '${REMOTE_BASE}/InferenceServer' && find . -type f \( -name '*.py' -o -name '*.txt' -o -name '*.md' -o -name '.env.sample' \) \
    -not -path './__pycache__/*' -not -path './.venv/*' -not -path './chroma_db/*' -not -path './Models/*' \
    | sort | xargs sha256sum 2>/dev/null | sha256sum | awk '{print \$1}'")

echo "[sync] 본 PC  hash: ${LOCAL_HASH}"
echo "[sync] LLM PC hash: ${REMOTE_HASH}"
if [ "${LOCAL_HASH}" = "${REMOTE_HASH}" ]; then
    echo "[sync] ✅ 코드 완전 동일"
else
    echo "[sync] ⚠ 차이 있음 (제외 패턴 — Models / .env / venv 는 정상)"
fi

# --restart 옵션: FastAPI 재시작
if [ "$1" = "--restart" ]; then
    echo ""
    echo "[sync] LLM PC FastAPI 재시작 중..."
    ssh "${REMOTE_USER}@${REMOTE_HOST}" bash <<'REMOTE'
cd ~/Desktop/MediBridge/InferenceServer
pkill -f 'InferenceServer.*Main.py' 2>/dev/null || true
sleep 1
set -a; source .env; set +a
nohup .venv/bin/python Main.py < /dev/null > /tmp/llm-pc.log 2>&1 &
disown
echo "PID: $!"
sleep 5
curl -sS http://127.0.0.1:8002/health | head -1
REMOTE
fi
