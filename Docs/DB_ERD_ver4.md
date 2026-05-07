# 메디브릿지 DB ERD 정의서

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | DB ERD (MariaDB 스키마) |
| **버전** | v4.0 |
| **개정일** | 2026-05-07 |
| **이전 버전** | v3.0 → `Docs/Old/DB_ERD_ver3_2026-05-07.md` |

> ⭐ **v4.0 변경 핵심** (가명화 정책 도입 + 데이터 보관 PC 메타):
> 1. **`pseudonym_map`** 신규 — 단일 끊기 매핑 (`user_id` 가 유일하게 user 와 연결되는 컬럼)
> 2. 기존 데이터 테이블의 `user_id` FK → **`anonymous_id`** FK 로 일괄 교체
> 3. **`photo_storage`** 신규 — 데이터 보관 PC 의 사진 메타데이터 (경로·MIME·크기 등)
> 4. 익명화 시점 자유 텍스트(`memo`, `report_notes`) NULL 처리 정책 코드 주석 명시

> 📌 **본 v4.0 은 「시스템_연결구조_ver2.md」 의 §8 가명화·익명화 정책을 직접 매핑**.

```sql
-- =====================================================
-- 1. 식약처 데이터 (낱알식별 + e약은요 + DUR + 성분)
-- =====================================================

-- 1-1. 낱알식별 — 식약처 CSV/스마트싱크로 일괄 적재
CREATE TABLE pill_identification (
    item_code VARCHAR(100) PRIMARY KEY COMMENT '품목기준코드',
    drug_name VARCHAR(255) NOT NULL COMMENT '약명',
    manufacturer VARCHAR(255) COMMENT '제조사',
    shape VARCHAR(50) COMMENT '모양',
    color_front VARCHAR(50) COMMENT '앞면 색상',
    color_back VARCHAR(50) COMMENT '뒷면 색상',
    engraving_front VARCHAR(100) COMMENT '앞면 각인',
    engraving_back VARCHAR(100) COMMENT '뒷면 각인',
    pill_image_url TEXT COMMENT '식약처 이미지 URL (DB 직접 저장 X)',
    classification_no VARCHAR(20) COMMENT '약효분류번호 (예: 01140)',
    classification_name VARCHAR(100) COMMENT '약효분류명 (예: 해열, 진통, 소염제)',
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '일괄 적재/갱신 일자',
    INDEX idx_classification (classification_no),
    INDEX idx_shape_color (shape, color_front)
);

-- 1-2. e약은요 — lazy 캐싱
CREATE TABLE drug_overview (
    item_code VARCHAR(100) PRIMARY KEY,
    efficacy_text TEXT,
    usage_text TEXT,
    warning_text TEXT,
    caution_text TEXT,
    interaction_text TEXT,
    side_effect_text TEXT,
    storage_text TEXT,
    cached_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code)
);

-- 1-3. DUR 성분
CREATE TABLE ingredient_info (
    ingredient_code VARCHAR(100) PRIMARY KEY,
    ingredient_name VARCHAR(255),
    safety_info TEXT,
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 1-4. 약품-성분 매핑
CREATE TABLE pill_ingredient_mapping (
    mapping_id INT AUTO_INCREMENT PRIMARY KEY,
    item_code VARCHAR(100),
    ingredient_code VARCHAR(100),
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code),
    FOREIGN KEY (ingredient_code) REFERENCES ingredient_info(ingredient_code),
    INDEX idx_item (item_code),
    INDEX idx_ingredient (ingredient_code)
);

-- 1-5. DUR 품목 (페어형, MVP 병용금기)
CREATE TABLE dur_interaction_cache (
    dur_id VARCHAR(100) PRIMARY KEY,
    base_item_code VARCHAR(100),
    target_item_code VARCHAR(100),
    dur_type VARCHAR(50),
    prohibit_reason TEXT COMMENT '식약처 본문 그대로 (가공 X)',
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (base_item_code) REFERENCES pill_identification(item_code),
    INDEX idx_base (base_item_code),
    INDEX idx_target (target_item_code)
);

-- =====================================================
-- 2. 사용자 계정 (모듈 6)
-- =====================================================

CREATE TABLE users (
    user_id VARCHAR(100) PRIMARY KEY COMMENT '사용자 고유 식별자 (직접 식별자)',
    email VARCHAR(255) UNIQUE NOT NULL COMMENT '이메일 (로그인 ID)',
    password_hash VARCHAR(255) NOT NULL COMMENT 'bcrypt 등 표준 해시',
    user_name VARCHAR(100) NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- =====================================================
-- 3. ⭐ v4.0 신규: 가명 매핑 (단일 끊기)
-- =====================================================
-- 본 테이블이 user 와 데이터 테이블 사이의 유일한 연결고리.
-- 익명화 절차: UPDATE pseudonym_map SET user_id = NULL WHERE created_at < NOW() - INTERVAL 1 YEAR;

CREATE TABLE pseudonym_map (
    anonymous_id VARCHAR(64) PRIMARY KEY COMMENT '가명 ID (UUID v4 또는 hash)',
    user_id VARCHAR(100) NULL COMMENT '⭐ 익명화 시 NULL 처리되는 단일 컬럼',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    severed_at DATETIME NULL COMMENT '끊어진 시점 (감사 로그)',
    FOREIGN KEY (user_id) REFERENCES users(user_id)
        ON DELETE SET NULL,                      -- 사용자 탈퇴 시에도 anonymous_id 보존
    INDEX idx_user_active (user_id, severed_at)  -- 활성 매핑 조회 성능
) COMMENT '가명화 단일 끊기 매핑 — 시스템_연결구조 ver2 §8 직접 매핑';

-- =====================================================
-- 4. 사용자 등록 약 풀 (모듈 1) — anonymous_id FK 사용
-- =====================================================

CREATE TABLE user_medication_pool (
    pool_id INT AUTO_INCREMENT PRIMARY KEY,
    anonymous_id VARCHAR(64) COMMENT '가명 ID (user_id 직접 연결 X)',
    item_code VARCHAR(100),
    reg_method ENUM('VOICE', 'MANUAL', 'IMAGE'),
    user_category VARCHAR(100) COMMENT '사용자 카테고리 (예: 해열진통제)',
    is_active BOOLEAN DEFAULT TRUE COMMENT 'soft delete',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    deactivated_at DATETIME NULL,
    FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id),
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code),
    INDEX idx_anon_active (anonymous_id, is_active)
);

-- =====================================================
-- 5. 복약 이력 (모듈 2) — anonymous_id FK
-- =====================================================

CREATE TABLE medication_intake_logs (
    intake_id VARCHAR(100) PRIMARY KEY,
    anonymous_id VARCHAR(64) COMMENT '가명 ID',
    item_code VARCHAR(100),
    intake_datetime DATETIME COMMENT '분 단위 / 미입력 시 시스템 시각',
    quantity INT,
    memo TEXT COMMENT '⚠ PII 가능성 자유 텍스트 — 익명화 시 NULL 처리',
    confidence_score DECIMAL(5,4) NULL,
    dur_snapshot JSON NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id),
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code),
    INDEX idx_anon_datetime (anonymous_id, intake_datetime)
);

-- =====================================================
-- 6. 통합 보고서 (모듈 5) — anonymous_id FK
-- =====================================================

CREATE TABLE user_reports (
    report_id VARCHAR(100) PRIMARY KEY,
    anonymous_id VARCHAR(64) COMMENT '가명 ID',
    period_start DATE,
    period_end DATE,
    consumed_summary JSON,
    side_effect_quotes JSON COMMENT '식약처 e약은요 인용 (LLM 변환 X)',
    report_notes TEXT COMMENT '⚠ PII 가능성 자유 텍스트 — 익명화 시 NULL',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id),
    INDEX idx_anon_period (anonymous_id, period_start, period_end)
);

-- =====================================================
-- 7. ⭐ v4.0 신규: 사진 메타데이터 (데이터 보관 PC 매핑)
-- =====================================================
-- 데이터 보관 PC 에 저장된 사진의 인덱스.
-- 클라/추론/학습 PC 가 본 테이블의 storage_path 로 데이터 보관 PC 에 직접 접근.

CREATE TABLE photo_storage (
    photo_id VARCHAR(64) PRIMARY KEY COMMENT '사진 고유 ID (UUID v4)',
    anonymous_id VARCHAR(64) COMMENT '가명 ID (소유자 추적용, 끊기 가능)',
    storage_path TEXT NOT NULL COMMENT '데이터 보관 PC 절대 경로',
    mime_type VARCHAR(50) COMMENT 'image/png, image/jpeg 등',
    file_size_bytes BIGINT COMMENT '바이트 단위',
    taken_at DATETIME COMMENT '촬영 시각 (클라 측 캡쳐 시점)',
    request_id VARCHAR(100) COMMENT '클라 요청 추적 ID (UUID)',
    purpose ENUM('IDENTIFY', 'TRAIN', 'OTHER') DEFAULT 'IDENTIFY',
    uploaded_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id),
    INDEX idx_anon_uploaded (anonymous_id, uploaded_at),
    INDEX idx_request (request_id)
) COMMENT '데이터 보관 PC 사진 인덱스 — 파일명: <anonymous_id>_<photo_id>.<ext>';

-- =====================================================
-- 8. (선택, 확장) 단계별 좁히기 세션 — MVP 미사용
-- =====================================================
-- MVP 는 stateless 흐름 채택. 본 테이블은 자리만 정의.
--
-- CREATE TABLE identify_narrow_sessions (
--     session_id VARCHAR(100) PRIMARY KEY,
--     anonymous_id VARCHAR(64),
--     attributes_json JSON,
--     candidates_count INT,
--     created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
--     expires_at DATETIME COMMENT '세션 TTL (예: 5분)',
--     FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id)
-- );
```

---

## 익명화 운영 SQL (참고)

### 매핑 끊기 — 1년 경과
```sql
UPDATE pseudonym_map
   SET user_id = NULL,
       severed_at = NOW()
 WHERE created_at < NOW() - INTERVAL 1 YEAR
   AND user_id IS NOT NULL;
```

### 자유 텍스트 PII 마스킹 — 매핑 끊긴 행
```sql
UPDATE medication_intake_logs il
   JOIN pseudonym_map pm ON il.anonymous_id = pm.anonymous_id
    SET il.memo = NULL
 WHERE pm.severed_at IS NOT NULL
   AND il.memo IS NOT NULL;

UPDATE user_reports r
   JOIN pseudonym_map pm ON r.anonymous_id = pm.anonymous_id
    SET r.report_notes = NULL
 WHERE pm.severed_at IS NOT NULL
   AND r.report_notes IS NOT NULL;
```

### 사용자 탈퇴 (즉시 익명화)
```sql
START TRANSACTION;

UPDATE pseudonym_map
   SET user_id = NULL,
       severed_at = NOW()
 WHERE user_id = ?;        -- 탈퇴 사용자

DELETE FROM users WHERE user_id = ?;

COMMIT;
```

---

## v3 → v4 마이그레이션 가이드 (참고)

```sql
-- 1. pseudonym_map 신규 + 기존 user_id 별로 anonymous_id 발급
INSERT INTO pseudonym_map (anonymous_id, user_id, created_at)
SELECT UUID(), user_id, NOW() FROM users;

-- 2. 데이터 테이블에 anonymous_id 컬럼 추가 (임시)
ALTER TABLE user_medication_pool ADD COLUMN anonymous_id_new VARCHAR(64);
UPDATE user_medication_pool ump
   JOIN pseudonym_map pm ON ump.user_id = pm.user_id
    SET ump.anonymous_id_new = pm.anonymous_id;

-- 3. 기존 user_id FK 제거 + anonymous_id_new 를 anonymous_id 로 rename
ALTER TABLE user_medication_pool
    DROP FOREIGN KEY <fk_name>,
    DROP COLUMN user_id,
    CHANGE COLUMN anonymous_id_new anonymous_id VARCHAR(64),
    ADD FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id);

-- (medication_intake_logs, user_reports 도 동일)
-- (photo_storage 는 신규 테이블이라 마이그레이션 X)
```

---

## 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v1.0 | 2026-04-30 | 팀 | 초안 |
| v2.0 | 2026-05-06 | 팀 | symptoms→memo, confidence_score·dur_snapshot, soft delete |
| v3.0 | 2026-05-07 | 팀 | 목업 분석 — classification_no/name + user_category + 인덱스 |
| v4.0 | 2026-05-07 | 팀 | 가명화 정책 도입 — ① `pseudonym_map` 신규 (단일 끊기 매핑) ② 데이터 테이블 `user_id` FK → `anonymous_id` FK 일괄 교체 (`user_medication_pool`, `medication_intake_logs`, `user_reports`) ③ `photo_storage` 신규 (데이터 보관 PC 사진 메타) ④ 익명화 운영 SQL 명시 (매핑 끊기 + 자유 텍스트 마스킹 + 사용자 탈퇴) ⑤ v3 → v4 마이그레이션 가이드. 관련 문서: 「시스템_연결구조 ver2 §8」 |
