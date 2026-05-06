# 메디브릿지 DB ERD 정의서

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | DB ERD (MariaDB 스키마) |
| **버전** | v2.0 |
| **개정일** | 2026-05-06 |
| **이전 버전** | v1.0 (2026-04-30) → `Docs/Old/DB_ERD_ver1_2026-05-06.md` |

> ⭐ **v2.0 변경 핵심**:
> 1. `medication_intake_logs.symptoms` → **`memo`** 로 컬럼명·주석 변경 (모듈 3 「증상/부작용 기록」은 본 MVP 범위 외, 컬럼 의미를 「메모」로 명확화).
> 2. `medication_intake_logs`에 **`confidence_score`**, **`dur_snapshot`** 컬럼 추가 (요구사항 분석서 FR-B9-02 「선택 항목: 식별 결과 신뢰도, DUR 위험 검출 결과 스냅샷」 반영).
> 3. `dur_interaction_cache`에 「확장 시 단일 약 단위 카테고리(임부금기·노인주의·연령대금기·용량주의 등)는 별도 테이블 분리 검토」 주석 추가.
> 4. `user_medication_pool`에 **`is_active`** 컬럼 추가 (개별 삭제 시 soft delete 정책 — 등록 이력 보존).

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
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '일괄 적재/갱신 일자'
);

-- 1-2. e약은요 — 사용자 요청 시점에 lazy 캐싱
CREATE TABLE drug_overview (
    item_code VARCHAR(100) PRIMARY KEY,
    efficacy_text TEXT COMMENT '효능',
    usage_text TEXT COMMENT '사용법',
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

-- 1-4. 약품-성분 매핑 (특정 약이 어떤 성분들로 이루어졌는지 역추적)
CREATE TABLE pill_ingredient_mapping (
    mapping_id INT AUTO_INCREMENT PRIMARY KEY,
    item_code VARCHAR(100) COMMENT '품목기준코드',
    ingredient_code VARCHAR(100) COMMENT '성분코드',
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code),
    FOREIGN KEY (ingredient_code) REFERENCES ingredient_info(ingredient_code)
) COMMENT '특정 약이 어떤 성분들로 이루어졌는지 역추적용 다리';

-- 1-5. DUR 품목정보 — 페어형 카테고리 (병용금기 등)
-- ※ MVP 범위: ① 병용금기 (페어형)
-- ※ 확장 시 검토: 단일 약 단위 카테고리(② 특정연령대금기, ③ 임부금기, ④ 용량주의,
--    ⑤ 투여기간주의, ⑥ 노인주의, ⑧ 서방정분할주의)는 본 테이블의 페어 구조로 표현이 어려움.
--    별도 테이블 `dur_single_drug_warning(item_code, dur_type, prohibit_reason)` 분리 검토 필요.
CREATE TABLE dur_interaction_cache (
    dur_id VARCHAR(100) PRIMARY KEY COMMENT 'DUR 레코드 고유키',
    base_item_code VARCHAR(100) COMMENT '기준 약물 (A)',
    target_item_code VARCHAR(100) COMMENT '금기 대상 약물 (B)',
    dur_type VARCHAR(50) COMMENT '금기 종류 (MVP: 병용금기 / 확장: 효능군중복주의 등 페어형)',
    prohibit_reason TEXT COMMENT '금기 사유 전문 (식약처 데이터 그대로 인용)',
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '일괄 적재/갱신 일자',
    FOREIGN KEY (base_item_code) REFERENCES pill_identification(item_code)
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
    reg_method ENUM('VOICE', 'MANUAL', 'IMAGE') COMMENT '사전 등록 방법 (VOICE: 폰 온디바이스 STT / MANUAL: 직접 입력 / IMAGE: 단일 촬영 — 확장)',
    is_active BOOLEAN DEFAULT TRUE COMMENT '활성 상태 (개별 삭제 시 FALSE로 soft delete — 등록 이력 보존)',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '등록일시',
    deactivated_at DATETIME NULL COMMENT '비활성(삭제) 일시',
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code)
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
    memo TEXT COMMENT '사용자 메모 (※ v1.0의 symptoms → memo로 변경. 「증상/부작용 기록」은 모듈 3 확장 범위)',
    confidence_score DECIMAL(5,4) NULL COMMENT '식별 결과 신뢰도 스냅샷 (선택 항목, FR-B9-02)',
    dur_snapshot JSON NULL COMMENT 'DUR 위험 검출 결과 스냅샷 (선택 항목, FR-B9-02)',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '시스템에 기록된 시각',
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (item_code) REFERENCES pill_identification(item_code)
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
    FOREIGN KEY (user_id) REFERENCES users(user_id)
);
```

---

## 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v1.0 | 2026-04-30 | 팀 (3인) | 초안 작성 |
| v2.0 | 2026-05-06 | 팀 (3인) | ① `medication_intake_logs.symptoms` → `memo`로 변경 (모듈 3 영역 침범 회피). ② `confidence_score`, `dur_snapshot` 컬럼 추가 (FR-B9-02). ③ `user_medication_pool.is_active` 컬럼 + soft delete 정책 명시. ④ `dur_interaction_cache` 주석에 단일 약 단위 카테고리 별도 테이블 분리 검토 가이드 추가. ⑤ `drug_overview.last_updated` → `cached_at`로 명확화 + TTL 정책 없음 명시. ⑥ `user_reports`를 `report_date` 단일 필드 → `period_start`/`period_end` 기간 필드로 확장 + `side_effect_quotes` JSON 컬럼 추가. ⑦ 식약처 데이터 적재 정책(일괄 적재 vs lazy 캐싱) 주석 명시. |
