# 메디브릿지 DB ERD 정의서

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | DB ERD (MariaDB 스키마) |
| **버전** | v3.0 |
| **개정일** | 2026-05-07 |
| **이전 버전** | v2.0 (2026-05-06) → `Docs/Old/DB_ERD_ver2_2026-05-07.md` |

> ⭐ **v3.0 변경 핵심** (목업 「메디브릿지 목업.pptx」 분석 반영):
> 1. **`user_medication_pool.user_category`** 컬럼 신규 — 약 풀 화면(Slide 6)의 "종류: 해열진통제·항혈소판제·건강기능식품" 표시용. 사용자 등록 시 입력 또는 e약은요 효능에서 자동 추출.
> 2. **`pill_identification.classification_no`** + **`pill_identification.classification_name`** 컬럼 신규 — 식약처 약효분류 정보(예: "01140 해열, 진통, 소염제"). 식약처 낱알식별 CSV에 포함된 컬럼 매핑.
> 3. **`identify_narrow_sessions`** 테이블 신규 (선택) — 단계별 좁히기 흐름이 stateful 인 경우. **MVP는 stateless 흐름 채택** — 본 테이블 미사용 (자리만 정의, 확장 시 활성화).

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
    pill_image_url TEXT COMMENT '식약처 이미지 URL (DB에 직접 저장하지 않고 URL만 보관)',

    -- ⭐ v3.0 신규: 식약처 약효분류 (목업 약 목록 화면 카테고리 표시용)
    classification_no VARCHAR(20) COMMENT '약효분류번호 (예: 01140)',
    classification_name VARCHAR(100) COMMENT '약효분류명 (예: 해열, 진통, 소염제)',

    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '일괄 적재/갱신 일자',
    INDEX idx_classification (classification_no),
    INDEX idx_shape_color (shape, color_front)
);

-- 1-2. e약은요 — 사용자 요청 시점에 lazy 캐싱
CREATE TABLE drug_overview (
    item_code VARCHAR(100) PRIMARY KEY,
    efficacy_text TEXT COMMENT '효능 (목업 식별 결과 화면 표시)',
    usage_text TEXT COMMENT '사용법 (목업 식별 결과 화면 표시)',
    warning_text TEXT COMMENT '복용 전 경고',
    caution_text TEXT COMMENT '주의사항',
    interaction_text TEXT COMMENT '상호작용',
    side_effect_text TEXT COMMENT '부작용',
    storage_text TEXT COMMENT '보관법',
    cached_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT 'lazy 캐싱 일자 (TTL 정책 없음)',
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code)
);

-- 1-3. DUR 성분정보 — 식약처 CSV/스마트싱크로 일괄 적재
CREATE TABLE ingredient_info (
    ingredient_code VARCHAR(100) PRIMARY KEY COMMENT '성분코드',
    ingredient_name VARCHAR(255) COMMENT '성분명',
    safety_info TEXT COMMENT '성분 단위 안전정보 (DUR 성분정보 기반)',
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '일괄 적재/갱신 일자'
);

-- 1-4. 약품-성분 매핑
CREATE TABLE pill_ingredient_mapping (
    mapping_id INT AUTO_INCREMENT PRIMARY KEY,
    item_code VARCHAR(100) COMMENT '품목기준코드',
    ingredient_code VARCHAR(100) COMMENT '성분코드',
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code),
    FOREIGN KEY (ingredient_code) REFERENCES ingredient_info(ingredient_code),
    INDEX idx_item (item_code),
    INDEX idx_ingredient (ingredient_code)
) COMMENT '특정 약이 어떤 성분들로 이루어졌는지 역추적용 다리';

-- 1-5. DUR 품목정보 — 페어형 카테고리 (병용금기 등)
CREATE TABLE dur_interaction_cache (
    dur_id VARCHAR(100) PRIMARY KEY COMMENT 'DUR 레코드 고유키',
    base_item_code VARCHAR(100) COMMENT '기준 약물 (A)',
    target_item_code VARCHAR(100) COMMENT '금기 대상 약물 (B)',
    dur_type VARCHAR(50) COMMENT '금기 종류 (MVP: 병용금기)',
    prohibit_reason TEXT COMMENT '금기 사유 전문 (식약처 데이터 그대로 인용)',
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '일괄 적재/갱신 일자',
    FOREIGN KEY (base_item_code) REFERENCES pill_identification(item_code),
    INDEX idx_base (base_item_code),
    INDEX idx_target (target_item_code)
);

-- =====================================================
-- 2. 사용자 계정 및 등록 영역 (모듈 6 + 모듈 1 등록)
-- =====================================================

CREATE TABLE users (
    user_id VARCHAR(100) PRIMARY KEY COMMENT '사용자 고유 식별자',
    email VARCHAR(255) UNIQUE NOT NULL COMMENT '이메일 (로그인 ID)',
    password_hash VARCHAR(255) NOT NULL COMMENT 'bcrypt 등 표준 해시 (평문 저장 절대 금지)',
    user_name VARCHAR(100) NOT NULL COMMENT '사용자 이름 또는 닉네임',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '가입일'
);

CREATE TABLE user_medication_pool (
    pool_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(100),
    item_code VARCHAR(100),
    reg_method ENUM('VOICE', 'MANUAL', 'IMAGE') COMMENT '사전 등록 방법',

    -- ⭐ v3.0 신규: 약 풀 표시용 카테고리 (목업 Slide 6 "종류: 해열진통제")
    -- 사용자가 명시 입력하거나, 등록 시점에 pill_identification.classification_name
    -- 또는 drug_overview.efficacy_text 에서 자동 추출
    user_category VARCHAR(100) COMMENT '사용자 표시용 카테고리 (예: 해열진통제, 항혈소판제, 건강기능식품)',

    is_active BOOLEAN DEFAULT TRUE COMMENT '활성 상태 (개별 삭제 시 FALSE 로 soft delete)',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '등록일시',
    deactivated_at DATETIME NULL COMMENT '비활성(삭제) 일시',
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code),
    INDEX idx_user_active (user_id, is_active)
) COMMENT '사용자별 등록 약 풀. 전체 리셋(수동)은 별도 트리거로 일괄 deactivate 처리';

-- =====================================================
-- 3. 복약 이력 및 보고서 영역 (모듈 2 + 모듈 5)
-- =====================================================

CREATE TABLE medication_intake_logs (
    intake_id VARCHAR(100) PRIMARY KEY COMMENT '복약 기록 고유키',
    user_id VARCHAR(100) COMMENT '사용자 식별자',
    item_code VARCHAR(100) COMMENT '복용한 약품 코드',
    intake_datetime DATETIME COMMENT '실제 약 복용 일시 (분 단위 / 미입력 시 시스템 시각)',
    quantity INT COMMENT '복용 개수',
    memo TEXT COMMENT '사용자 메모',
    confidence_score DECIMAL(5,4) NULL COMMENT '식별 결과 신뢰도 스냅샷 (선택)',
    dur_snapshot JSON NULL COMMENT 'DUR 위험 검출 결과 스냅샷 (선택)',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '시스템에 기록된 시각',
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code),
    INDEX idx_user_datetime (user_id, intake_datetime)
);

CREATE TABLE user_reports (
    report_id VARCHAR(100) PRIMARY KEY COMMENT '보고서 고유키',
    user_id VARCHAR(100) COMMENT '사용자 식별자',
    period_start DATE COMMENT '보고서 시작일',
    period_end DATE COMMENT '보고서 종료일',
    consumed_summary JSON COMMENT '먹은 약들의 통계 (종류별 개수·일자 등 JSON)',
    side_effect_quotes JSON COMMENT '약별 부작용 식약처 e약은요 인용 (LLM 자연어 생성 미사용)',
    report_notes TEXT COMMENT '해당 보고서 종합 메모',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '보고서 생성 일시',
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    INDEX idx_user_period (user_id, period_start, period_end)
);

-- =====================================================
-- 4. (선택, 확장) 단계별 좁히기 세션 — MVP 미사용
-- =====================================================
-- 본 테이블은 stateful 단계별 좁히기 흐름이 필요할 때 활성화.
-- MVP는 stateless 흐름 (클라가 누적 attributes 매번 송신) 채택.
--
-- CREATE TABLE identify_narrow_sessions (
--     session_id VARCHAR(100) PRIMARY KEY,
--     user_id VARCHAR(100),
--     attributes_json JSON COMMENT '누적 입력 (color, shape, has_engraving, ...)',
--     candidates_count INT,
--     created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
--     expires_at DATETIME COMMENT '세션 TTL (예: 5분)',
--     FOREIGN KEY (user_id) REFERENCES users(user_id)
-- );
```

---

## 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v1.0 | 2026-04-30 | 팀 (3인) | 초안 작성 |
| v2.0 | 2026-05-06 | 팀 (3인) | `medication_intake_logs.symptoms` → `memo`, `confidence_score`·`dur_snapshot` 추가, `user_medication_pool.is_active` soft delete 등 |
| v3.0 | 2026-05-07 | 팀 (3인) | 목업 분석 반영 — ① `pill_identification.classification_no/name` 추가 (식약처 약효분류, Slide 6 "종류" 표시) ② `user_medication_pool.user_category` 추가 (사용자 등록 시 입력) ③ 인덱스 추가 (조회 성능: `idx_classification`, `idx_shape_color`, `idx_user_active`, `idx_user_period`, `idx_user_datetime`) ④ 단계별 좁히기 세션 테이블 자리 정의 (MVP 미사용, 확장 자리) |
