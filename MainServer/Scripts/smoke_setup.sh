#!/usr/bin/env bash
# 메인서버 스모크 테스트용 — DB 사용자/스키마/시드 일괄 적용 (WSL 환경 가정).
# 실행: wsl -d Ubuntu -u root -e bash MainServer/Scripts/smoke_setup.sh
set -e

DB_NAME=medibridge
DB_USER=medibridge_app
DB_PASS=smoke_pw_change_me

echo "[smoke] 1) DB / user 생성 (idempotent)"
mysql -u root <<SQL
CREATE DATABASE IF NOT EXISTS ${DB_NAME} CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS '${DB_USER}'@'localhost' IDENTIFIED BY '${DB_PASS}';
GRANT SELECT, INSERT, UPDATE, DELETE ON ${DB_NAME}.* TO '${DB_USER}'@'localhost';
FLUSH PRIVILEGES;
SQL

echo "[smoke] 2) 스키마 적용"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mysql -u root ${DB_NAME} < "${SCRIPT_DIR}/../Database/Migrations/001_init_schema.sql"

echo "[smoke] 2-b) 마이그레이션 002 (photo_storage intent 컬럼)"
# 002 는 ALTER 라 멱등이 아님 — 이미 적용된 경우 안전하게 무시
mysql -u root ${DB_NAME} < "${SCRIPT_DIR}/../Database/Migrations/002_photo_storage_intent.sql" 2>/dev/null \
    || echo "[smoke]   (이미 적용됨 또는 컬럼 중복 — 무시)"

echo "[smoke] 3) 시드 적용 (TestMode 용 더미)"
mysql -u root ${DB_NAME} < "${SCRIPT_DIR}/../Database/Seeds/dev_seed.sql"

echo "[smoke] 4) 적용 검증"
mysql -u root -e "SELECT COUNT(*) AS pills      FROM pill_identification;
                  SELECT COUNT(*) AS users      FROM users;
                  SELECT COUNT(*) AS pseudonyms FROM pseudonym_map;
                  SELECT COUNT(*) AS pool_rows  FROM user_medication_pool WHERE is_active;
                  SELECT COUNT(*) AS intake_logs FROM medication_intake_logs;" ${DB_NAME}

echo "[smoke] DONE — 메인서버 실행 시 다음 환경변수 사용:"
echo "  MEDIBRIDGE_DB_PASSWORD='${DB_PASS}'"
echo "  MEDIBRIDGE_TEST_MODE='true'"
echo "  MEDIBRIDGE_JWT_SECRET='at_least_32_bytes_long_secret_for_smoke_test_xx'"
echo "  MEDIBRIDGE_STORAGE_SECRET='at_least_32_bytes_long_storage_secret_xxx_yy'"
echo "  MEDIBRIDGE_STORAGE_BASE_URL='http://10.10.10.122:8004'   # 데이터 보관 PC"
