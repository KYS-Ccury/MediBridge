#!/usr/bin/env bash
# =====================================================
# sync-vision-pc.sh — 본 PC InferenceServer/ → 비전 PC 단방향 동기화
# =====================================================
# 사용 (WSL Bash):
#   bash Scripts/sync-vision-pc.sh
#
# 동작:
#   1) InferenceServer/ 의 코드만 rsync
#   2) .env / Models/ / chroma_db/ / venv / __pycache__ 는 제외
#      → 비전PC 의 .env (VISION 모드) 와 학습 가중치 유지
#   3) 변경 사항 요약 출력
#
# 그 외 디렉토리 (MainServer, Client, Docs 등) 는 본 PC 에서만 있으면 됨.
# =====================================================
set -e

REMOTE_USER="ai-trainer"
REMOTE_HOST="10.10.10.120"
REMOTE_BASE="/media/ai-trainer/fd234fd8-cefc-4354-bc18-b8babbcf4f31/home/yesom/Desktop/MediBridge"

LOCAL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOCAL_INFERENCE="${LOCAL_ROOT}/InferenceServer"

if [ ! -d "${LOCAL_INFERENCE}" ]; then
    echo "ERROR: ${LOCAL_INFERENCE} 가 없습니다." >&2
    exit 1
fi

echo "[sync] 본 PC : ${LOCAL_INFERENCE}"
echo "[sync] 비전PC: ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/InferenceServer/"
echo ""

# 사전 SSH 도달성 점검
if ! ssh -o BatchMode=yes -o ConnectTimeout=3 "${REMOTE_USER}@${REMOTE_HOST}" 'true' 2>/dev/null; then
    echo "ERROR: SSH 인증 실패. ssh-copy-id ${REMOTE_USER}@${REMOTE_HOST} 먼저 실행하세요." >&2
    exit 2
fi

# rsync — 단방향 (본 PC → 비전PC). --delete 로 본 PC 삭제분도 반영.
# 단, .env / Models / chroma_db / venv / __pycache__ 는 비전PC 의 것 유지 (--exclude).
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
echo "[sync] 동기화 완료. 비전PC 의 .env / Models / venv 는 그대로 유지."
echo ""

# 동일성 검증 (sha256 비교)
LOCAL_HASH=$(cd "${LOCAL_INFERENCE}" && find . -type f \( -name '*.py' -o -name '*.txt' -o -name '*.md' -o -name '.env.sample' \) \
    -not -path './__pycache__/*' -not -path './.venv/*' -not -path './chroma_db/*' -not -path './Models/*' \
    | sort | xargs sha256sum 2>/dev/null | sha256sum | awk '{print $1}')
REMOTE_HASH=$(ssh "${REMOTE_USER}@${REMOTE_HOST}" \
    "cd '${REMOTE_BASE}/InferenceServer' && find . -type f \( -name '*.py' -o -name '*.txt' -o -name '*.md' -o -name '.env.sample' \) \
    -not -path './__pycache__/*' -not -path './.venv/*' -not -path './chroma_db/*' -not -path './Models/*' \
    | sort | xargs sha256sum 2>/dev/null | sha256sum | awk '{print \$1}'")

echo "[sync] 본 PC  hash: ${LOCAL_HASH}"
echo "[sync] 비전PC hash: ${REMOTE_HASH}"
if [ "${LOCAL_HASH}" = "${REMOTE_HASH}" ]; then
    echo "[sync] ✅ 코드 완전 동일"
else
    echo "[sync] ⚠ 차이 있음 (제외 패턴이 다를 수 있음 — Models / .env / venv 는 정상)"
fi

# (선택) 비전PC FastAPI 재시작 — 코드 변경 후 반드시 필요
if [ "$1" = "--restart" ]; then
    echo ""
    echo "[sync] 비전PC FastAPI 재시작 중..."
    ssh "${REMOTE_USER}@${REMOTE_HOST}" bash <<'REMOTE'
TARGET="/media/ai-trainer/fd234fd8-cefc-4354-bc18-b8babbcf4f31/home/yesom/Desktop/MediBridge"
VENV="$TARGET/check_img_ih/venv"
cd "$TARGET/InferenceServer"
pkill -f 'InferenceServer.*Main.py' 2>/dev/null || true
sleep 1
set -a; source .env; set +a
nohup "$VENV/bin/python" Main.py < /dev/null > /tmp/vision-pc.log 2>&1 &
disown
echo "PID: $!"
sleep 3
curl -sS http://127.0.0.1:8003/health | head -1
REMOTE
fi
