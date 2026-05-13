#!/usr/bin/env bash
# =====================================================
# smoke_test.sh — 메인서버 + 보관 PC 풀 시나리오 검증
# =====================================================
# 전제:
#   - MainServer 8001 가동 (medibridge-up.sh)
#   - DataStorageServer 8004 가동 (datastorage-up.sh)
#   - 두 서버가 동일 MEDIBRIDGE_STORAGE_SECRET 사용
#
# 시나리오:
#   1) 로그인 → JWT
#   2) /v1/media/intent → photo_id + storage_url + put_token
#   3) 더미 JPG 생성 (1x1 픽셀)
#   4) PUT storage_url → 보관 PC 에 저장
#   5) /v1/media/commit → status=READY
#   6) GET 토큰을 메인이 발급해줘야 하므로 — 본 스모크는 PUT 까지만 검증
#      (GET 토큰 발급은 Vision PC 연동 단계에서 추가)
# =====================================================
set -e

MAIN_URL='http://127.0.0.1:8001'

echo "=== 1) 로그인 ==="
JWT=$(curl -s -X POST "${MAIN_URL}/v1/auth/login" \
  -H 'Content-Type: application/json' \
  -d '{"email":"test@medibridge.local","password":"test1234"}' \
  | python3 -c 'import sys,json;print(json.load(sys.stdin)["access_token"])')
echo "JWT 길이: ${#JWT}"

echo
echo "=== 2) /v1/media/intent ==="
RESP=$(curl -s -X POST "${MAIN_URL}/v1/media/intent" \
  -H "Authorization: Bearer ${JWT}" \
  -H 'Content-Type: application/json' \
  -d '{"mime_type":"image/jpeg","size_bytes":1000,"purpose":"IDENTIFY"}')
echo "${RESP}" | python3 -m json.tool

PHOTO_ID=$(echo "${RESP}" | python3 -c 'import sys,json;print(json.load(sys.stdin)["photo_id"])')
STORAGE_URL=$(echo "${RESP}" | python3 -c 'import sys,json;print(json.load(sys.stdin)["storage_url"])')
PUT_TOKEN=$(echo "${RESP}" | python3 -c 'import sys,json;print(json.load(sys.stdin)["put_token"])')

# 보관 PC 가 127.0.0.1:8004 로 가동된 경우 storage_url 의 호스트 부분만 교체
LOCAL_URL=$(echo "${STORAGE_URL}" | sed 's|http://10\.10\.10\.122:8004|http://127.0.0.1:8004|')
echo
echo "STORAGE_URL (원본) : ${STORAGE_URL}"
echo "STORAGE_URL (로컬) : ${LOCAL_URL}"

echo
echo "=== 3) 더미 JPG 생성 (1x1 픽셀, 약 125B) ==="
TMP_JPG=$(mktemp --suffix=.jpg)
# 최소 유효 JPEG (1x1 흰색)
printf '\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00\xff\xdb\x00\x43\x00\x08\x06\x06\x07\x06\x05\x08\x07\x07\x07\x09\x09\x08\x0a\x0c\x14\x0d\x0c\x0b\x0b\x0c\x19\x12\x13\x0f\x14\x1d\x1a\x1f\x1e\x1d\x1a\x1c\x1c\x20\x24\x2e\x27\x20\x22\x2c\x23\x1c\x1c\x28\x37\x29\x2c\x30\x31\x34\x34\x34\x1f\x27\x39\x3d\x38\x32\x3c\x2e\x33\x34\x32\xff\xc0\x00\x0b\x08\x00\x01\x00\x01\x01\x01\x11\x00\xff\xc4\x00\x14\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xc4\x00\x14\x10\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x37\xff\xd9' > "${TMP_JPG}"
echo "파일: ${TMP_JPG} (size=$(stat -c %s "${TMP_JPG}") B)"

echo
echo "=== 4) PUT 보관 PC ==="
PUT_RESP=$(curl -s -w '\nHTTP:%{http_code}' -X PUT "${LOCAL_URL}" \
  -H "Authorization: Bearer ${PUT_TOKEN}" \
  -H 'Content-Type: image/jpeg' \
  --data-binary "@${TMP_JPG}")
echo "${PUT_RESP}"

echo
echo "=== 5) /v1/media/commit ==="
curl -s -X POST "${MAIN_URL}/v1/media/commit" \
  -H "Authorization: Bearer ${JWT}" \
  -H 'Content-Type: application/json' \
  -d "{\"photo_id\":\"${PHOTO_ID}\"}" | python3 -m json.tool

echo
echo "=== 6) 보관 PC 디스크 확인 ==="
ROOT="${DATASTORAGE_ROOT:-/tmp/medibridge_storage_smoke}"
echo "검색: ${ROOT}/anon_test_001/${PHOTO_ID}.jpg"
ls -la "${ROOT}/anon_test_001/${PHOTO_ID}.jpg" 2>&1

echo
echo "=== 7) 보안 — 만료된 토큰으로 PUT 시도 (실패 예상) ==="
EXPIRED='eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjEwMDB9.AAAA'
curl -s -w '\nHTTP:%{http_code}' -X PUT "${LOCAL_URL}" \
  -H "Authorization: Bearer ${EXPIRED}" \
  -H 'Content-Type: image/jpeg' \
  --data-binary "@${TMP_JPG}"

echo
echo
echo "=== 8) 보안 — 토큰 jti 와 다른 photo_id 로 PUT 시도 (실패 예상) ==="
BAD_URL=$(echo "${LOCAL_URL}" | sed "s|${PHOTO_ID}|ph_attacker_photo_id|")
curl -s -w '\nHTTP:%{http_code}' -X PUT "${BAD_URL}" \
  -H "Authorization: Bearer ${PUT_TOKEN}" \
  -H 'Content-Type: image/jpeg' \
  --data-binary "@${TMP_JPG}"

echo
echo
rm -f "${TMP_JPG}"
echo "=== DONE ==="
