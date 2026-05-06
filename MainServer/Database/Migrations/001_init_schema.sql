-- =====================================================
-- MediBridge — 001 초기 스키마
-- =====================================================
-- 정본: Docs/DB_ERD_ver4.md
-- 적용: mysql -u medibridge_app -p medibridge < 001_init_schema.sql
-- =====================================================

-- (선택) 깨끗한 재적용을 위한 DROP — 운영 시 주의
-- SET FOREIGN_KEY_CHECKS=0;
-- DROP TABLE IF EXISTS photo_storage, user_reports, medication_intake_logs,
--                      user_medication_pool, pseudonym_map, users,
--                      dur_interaction_cache, pill_ingredient_mapping,
--                      ingredient_info, drug_overview, pill_identification;
-- SET FOREIGN_KEY_CHECKS=1;

-- =====================================================
-- 1. 식약처 데이터
-- =====================================================

CREATE TABLE IF NOT EXISTS pill_identification (
    item_code           VARCHAR(100) PRIMARY KEY COMMENT '품목기준코드',
    drug_name           VARCHAR(255) NOT NULL    COMMENT '약명',
    manufacturer        VARCHAR(255)             COMMENT '제조사',
    shape               VARCHAR(50)              COMMENT '모양',
    color_front         VARCHAR(50)              COMMENT '앞면 색상',
    color_back          VARCHAR(50)              COMMENT '뒷면 색상',
    engraving_front     VARCHAR(100)             COMMENT '앞면 각인',
    engraving_back      VARCHAR(100)             COMMENT '뒷면 각인',
    pill_image_url      TEXT                     COMMENT '식약처 이미지 URL (DB 직접 저장 X)',
    classification_no   VARCHAR(20)              COMMENT '약효분류번호 (예: 01140)',
    classification_name VARCHAR(100)             COMMENT '약효분류명 (예: 해열, 진통, 소염제)',
    last_updated        DATETIME DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_classification (classification_no),
    INDEX idx_shape_color    (shape, color_front),
    INDEX idx_drug_name      (drug_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS drug_overview (
    item_code        VARCHAR(100) PRIMARY KEY,
    efficacy_text    TEXT,
    usage_text       TEXT,
    warning_text     TEXT,
    caution_text     TEXT,
    interaction_text TEXT,
    side_effect_text TEXT,
    storage_text     TEXT,
    cached_at        DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS ingredient_info (
    ingredient_code VARCHAR(100) PRIMARY KEY,
    ingredient_name VARCHAR(255),
    safety_info     TEXT,
    last_updated    DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS pill_ingredient_mapping (
    mapping_id      INT AUTO_INCREMENT PRIMARY KEY,
    item_code       VARCHAR(100),
    ingredient_code VARCHAR(100),
    FOREIGN KEY (item_code)       REFERENCES pill_identification(item_code) ON DELETE CASCADE,
    FOREIGN KEY (ingredient_code) REFERENCES ingredient_info(ingredient_code) ON DELETE CASCADE,
    INDEX idx_item       (item_code),
    INDEX idx_ingredient (ingredient_code)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS dur_interaction_cache (
    dur_id           VARCHAR(100) PRIMARY KEY,
    base_item_code   VARCHAR(100),
    target_item_code VARCHAR(100),
    dur_type         VARCHAR(50),
    prohibit_reason  TEXT COMMENT '식약처 본문 그대로 (가공 X)',
    last_updated     DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (base_item_code) REFERENCES pill_identification(item_code) ON DELETE CASCADE,
    INDEX idx_base   (base_item_code),
    INDEX idx_target (target_item_code)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 2. 사용자 계정
-- =====================================================

CREATE TABLE IF NOT EXISTS users (
    user_id       VARCHAR(100) PRIMARY KEY COMMENT '사용자 고유 식별자',
    email         VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL    COMMENT 'bcrypt / pbkdf2 표준 해시',
    user_name     VARCHAR(100) NOT NULL,
    created_at    DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 3. 가명 매핑 (단일 끊기)
-- =====================================================

CREATE TABLE IF NOT EXISTS pseudonym_map (
    anonymous_id VARCHAR(64) PRIMARY KEY COMMENT '가명 ID',
    user_id      VARCHAR(100) NULL       COMMENT '익명화 시 NULL',
    created_at   DATETIME DEFAULT CURRENT_TIMESTAMP,
    severed_at   DATETIME NULL,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL,
    INDEX idx_user_active (user_id, severed_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT '가명화 단일 끊기 매핑';

-- =====================================================
-- 4. 사용자 등록 약 풀
-- =====================================================

CREATE TABLE IF NOT EXISTS user_medication_pool (
    pool_id        INT AUTO_INCREMENT PRIMARY KEY,
    anonymous_id   VARCHAR(64),
    item_code      VARCHAR(100),
    reg_method     ENUM('VOICE', 'MANUAL', 'IMAGE'),
    user_category  VARCHAR(100),
    is_active      BOOLEAN DEFAULT TRUE COMMENT 'soft delete',
    created_at     DATETIME DEFAULT CURRENT_TIMESTAMP,
    deactivated_at DATETIME NULL,
    FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id) ON DELETE CASCADE,
    FOREIGN KEY (item_code)    REFERENCES pill_identification(item_code) ON DELETE RESTRICT,
    INDEX idx_anon_active (anonymous_id, is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 5. 복약 이력
-- =====================================================

CREATE TABLE IF NOT EXISTS medication_intake_logs (
    intake_id        VARCHAR(100) PRIMARY KEY,
    anonymous_id     VARCHAR(64),
    item_code        VARCHAR(100),
    intake_datetime  DATETIME,
    quantity         INT,
    memo             TEXT COMMENT 'PII 가능 자유 텍스트 — 익명화 시 NULL',
    confidence_score DECIMAL(5,4) NULL,
    dur_snapshot     JSON NULL,
    created_at       DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id) ON DELETE CASCADE,
    FOREIGN KEY (item_code)    REFERENCES pill_identification(item_code) ON DELETE RESTRICT,
    INDEX idx_anon_datetime (anonymous_id, intake_datetime)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 6. 통합 보고서
-- =====================================================

CREATE TABLE IF NOT EXISTS user_reports (
    report_id          VARCHAR(100) PRIMARY KEY,
    anonymous_id       VARCHAR(64),
    period_start       DATE,
    period_end         DATE,
    consumed_summary   JSON,
    side_effect_quotes JSON COMMENT '식약처 e약은요 인용 (LLM 변환 X)',
    report_notes       TEXT COMMENT 'PII 가능 자유 텍스트 — 익명화 시 NULL',
    created_at         DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id) ON DELETE CASCADE,
    INDEX idx_anon_period (anonymous_id, period_start, period_end)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 7. 사진 메타데이터 (데이터 보관 PC 인덱스)
-- =====================================================

CREATE TABLE IF NOT EXISTS photo_storage (
    photo_id        VARCHAR(64) PRIMARY KEY,
    anonymous_id    VARCHAR(64),
    storage_path    TEXT NOT NULL,
    mime_type       VARCHAR(50),
    file_size_bytes BIGINT,
    taken_at        DATETIME,
    request_id      VARCHAR(100),
    purpose         ENUM('IDENTIFY', 'TRAIN', 'OTHER') DEFAULT 'IDENTIFY',
    uploaded_at     DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (anonymous_id) REFERENCES pseudonym_map(anonymous_id) ON DELETE CASCADE,
    INDEX idx_anon_uploaded (anonymous_id, uploaded_at),
    INDEX idx_request       (request_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT '데이터 보관 PC 사진 인덱스';
