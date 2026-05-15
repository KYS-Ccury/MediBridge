#!/usr/bin/env bash
# =====================================================
# sync-storage-pc.sh — 본 PC DataStorageServer/ → 보관 PC 단방향 동기화
# =====================================================
# 사용 (WSL Bash):
#   bash Scripts/sync-storage-pc.sh             # 동기화만
#   bash Scripts/sync-storage-pc.sh --rebuild   # 동기화 + cmake/make
#   bash Scripts/sync-storage-pc.sh --restart   # 동기화 + 재시작
#   bash Scripts/sync-storage-pc.sh --rebuild --restart  # 모두
#
# Vision/LLM PC 와 다른 점:
#   - C++ Drogon 빌드 (python venv 아님)
#   - 대상은 DataStorageServer/ (InferenceServer 아님)
#   - 보관 PC 가 한글 경로 사용 (~/바탕화면/MediBridge/)
#   - 환경변수 MEDIBRIDGE_STORAGE_SECRET 가 메인서버와 동일해야 함
# =====================================================
set -e

REMOTE_USER="lms"
REMOTE_HOST="10.10.10.122"
REMOTE_BASE="/home/lms/바탕화면/MediBridge"

LOCAL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOCAL_DSS="${LOCAL_ROOT}/DataStorageServer"

if [ ! -d "${LOCAL_DSS}" ]; then
    echo "ERROR: ${LOCAL_DSS} 가 없습니다." >&2
    exit 1
fi

REBUILD=false
RESTART=false
for arg in "$@"; do
    case "$arg" in
        --rebuild) REBUILD=true ;;
        --restart) RESTART=true ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

echo "[sync] 본 PC : ${LOCAL_DSS}"
echo "[sync] 보관PC: ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/DataStorageServer/"
echo "[sync] rebuild=${REBUILD}  restart=${RESTART}"
echo ""

# 사전 SSH 도달성
if ! ssh -o BatchMode=yes -o ConnectTimeout=3 "${REMOTE_USER}@${REMOTE_HOST}" 'true' 2>/dev/null; then
    echo "ERROR: SSH 인증 실패. ssh-copy-id ${REMOTE_USER}@${REMOTE_HOST} 먼저." >&2
    exit 3
fi

# rsync — DataStorageServer/ 만. 빌드 결과·업로드 데이터·로그는 보관 PC 유지.
rsync -avz --delete \
    --exclude='build/' \
    --exclude='build-wsl/' \
    --exclude='build-*/' \
    --exclude='cmake-build-*/' \
    --exclude='uploads/' \
    --exclude='*.o' \
    --exclude='*.so' \
    --exclude='CMakeCache.txt' \
    --exclude='CMakeFiles/' \
    "${LOCAL_DSS}/" \
    "${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/DataStorageServer/"

echo ""
echo "[sync] 동기화 완료. 보관 PC 의 build-wsl/ / uploads/ 는 그대로 유지."
echo ""

# 동일성 검증
LOCAL_HASH=$(cd "${LOCAL_DSS}" && find . -type f \
    \( -name '*.cpp' -o -name '*.h' -o -name '*.cc' -o -name 'CMakeLists.txt' -o -name '*.sh' -o -name '*.md' -o -name '*.json' -o -name '*.sample' \) \
    -not -path './build*' | sort | xargs sha256sum 2>/dev/null | sha256sum | cut -d' ' -f1)

REMOTE_HASH=$(ssh "${REMOTE_USER}@${REMOTE_HOST}" bash -s <<'REMOTE'
cd ~/바탕화면/MediBridge/DataStorageServer
find . -type f \
    \( -name '*.cpp' -o -name '*.h' -o -name '*.cc' -o -name 'CMakeLists.txt' -o -name '*.sh' -o -name '*.md' -o -name '*.json' -o -name '*.sample' \) \
    -not -path './build*' | sort | xargs sha256sum 2>/dev/null | sha256sum | cut -d' ' -f1
REMOTE
)

echo "[sync] 본 PC  hash: ${LOCAL_HASH}"
echo "[sync] 보관PC hash: ${REMOTE_HASH}"
if [ "${LOCAL_HASH}" = "${REMOTE_HASH}" ]; then
    echo "[sync] ✅ 코드 완전 동일"
else
    echo "[sync] ⚠ 차이"
fi

# --rebuild
if [ "${REBUILD}" = "true" ]; then
    echo ""
    echo "[sync] 보관 PC 측 cmake + make 실행..."
    ssh "${REMOTE_USER}@${REMOTE_HOST}" bash <<'REMOTE'
set -e
cd ~/바탕화면/MediBridge/DataStorageServer
mkdir -p build-wsl
cd build-wsl
if [ ! -f Makefile ]; then
    cmake ..
fi
make -j$(nproc) 2>&1 | tail -5
REMOTE
fi

# --restart
if [ "${RESTART}" = "true" ]; then
    echo ""
    echo "[sync] 보관 PC FastAPI 재시작..."
    ssh "${REMOTE_USER}@${REMOTE_HOST}" bash <<'REMOTE'
pkill -x MediBridgeDataStorageServer 2>/dev/null || true
sleep 1
# 기존 datastorage-up.sh 활용
bash ~/바탕화면/MediBridge/DataStorageServer/Scripts/datastorage-up.sh 2>&1 | tail -10
REMOTE
fi
