-- =====================================================
-- MediBridge — 개발용 더미 데이터 (TestMode)
-- =====================================================
-- 정책: Docs/TestMode.md
-- 적용: mysql -u medibridge_app -p medibridge < dev_seed.sql
--
-- ⚠ 식별 prefix:
--   - item_code:    999800xxx  (식약처 실 코드와 충돌 X)
--   - user_id:      test_user_xxx
--   - anonymous_id: anon_test_xxx
--   - storage_path: /test/dev_seed/...
-- ⚠ production 환경에서 절대 적용 금지.
-- =====================================================

-- =====================================================
-- 1. 식약처 데이터 더미
-- =====================================================

INSERT INTO pill_identification
  (item_code, drug_name, manufacturer, shape, color_front, color_back,
   engraving_front, engraving_back, classification_no, classification_name)
VALUES
  ('999800001', '타이레놀정500mg(테스트)',         '한국얀센', '장방형', '흰색', '흰색', 'TYL', '500',  '01140', '해열, 진통, 소염제'),
  ('999800002', '타이레놀이알서방정650mg(테스트)', '한국얀센', '장방형', '흰색', '흰색', 'TY8', '650',  '01140', '해열, 진통, 소염제'),
  ('999800003', '어린이타이레놀정80mg(테스트)',    '한국얀센', '원형',   '분홍', '분홍', 'TYK', '',     '01140', '해열, 진통, 소염제'),
  ('999800004', '게보린정(테스트)',                '삼진제약', '원형',   '흰색', '흰색', 'GB',  '',     '01140', '해열, 진통, 소염제'),
  ('999800005', '이부프로펜정200mg(테스트)',       '대원제약', '원형',   '주황', '주황', 'IB2', '',     '01140', '해열, 진통, 소염제'),
  ('999800006', '판콜에이내복액(테스트)',          '동화약품', '캡슐형', '갈색', '갈색', '',    '',     '01400', '진해거담제'),
  ('999800007', '베아제정(테스트)',                '대웅제약', '원형',   '연두', '연두', 'BE',  '',     '02300', '소화제'),
  ('999800008', '훼스탈플러스정(테스트)',          '한독',     '원형',   '연두', '연두', 'FE',  '+',    '02300', '소화제'),
  ('999800009', '센텔라정100mg(테스트)',           '동국제약', '원형',   '연두', '연두', 'CT',  '100',  '13900', '기타순환계용약'),
  ('999800010', '아스피린프로텍트정100mg(테스트)', '바이엘',   '원형',   '주황', '주황', 'BAY', 'A100', '01140', '해열, 진통, 소염제');

-- e약은요 캐시 (Onboarding RESOLVED 단계 표시용)
INSERT INTO drug_overview (item_code, efficacy_text, usage_text, warning_text, side_effect_text)
VALUES
  ('999800001',
   '이 약은 발열, 두통, 근육통, 생리통, 치통, 관절통의 완화에 사용합니다.',
   '성인 1회 1~2정, 1일 3~4회 식사 후 30분에 복용합니다. 1일 최대 4g(8정)을 초과하지 마세요.',
   '간 질환 환자는 의사와 상의하세요. 음주 후 복용을 피하세요.',
   '드물게 발진, 가려움, 구역, 구토 등이 나타날 수 있습니다.'),
  ('999800004',
   '이 약은 발열, 두통, 치통, 생리통의 완화에 사용합니다.',
   '성인 1회 1정, 1일 3회 식사 후 30분에 복용합니다.',
   '아세트아미노펜 함유 다른 약과 병용 시 간 손상 위험이 있습니다.',
   '발진, 가려움, 위장 장애가 나타날 수 있습니다.'),
  ('999800005',
   '이 약은 발열, 두통, 근육통, 관절통의 완화에 사용합니다.',
   '성인 1회 1~2정, 1일 3회 식사 후 복용합니다.',
   '위·십이지장 궤양 환자는 의사와 상의하세요.',
   '위장 장애, 발진이 나타날 수 있습니다.');

-- 성분 정보
INSERT INTO ingredient_info (ingredient_code, ingredient_name, safety_info)
VALUES
  ('ING001', '아세트아미노펜',     '1일 최대 4g(성인). 간 질환 환자 주의.'),
  ('ING002', '이부프로펜',         '위·십이지장 궤양 환자 주의.'),
  ('ING003', '아스피린',           '소아 라이 증후군 위험. 위장관 출혈 주의.'),
  ('ING004', '구아이페네신',       '진해거담 보조 성분.'),
  ('ING005', '판크레아틴',         '소화 효소 보충.'),
  ('ING006', '센텔라아시아티카',   '정맥 부전 보조.');

-- 약품-성분 매핑
INSERT INTO pill_ingredient_mapping (item_code, ingredient_code) VALUES
  ('999800001', 'ING001'),
  ('999800002', 'ING001'),
  ('999800003', 'ING001'),
  ('999800004', 'ING001'),
  ('999800005', 'ING002'),
  ('999800006', 'ING004'),
  ('999800007', 'ING005'),
  ('999800008', 'ING005'),
  ('999800009', 'ING006'),
  ('999800010', 'ING003');

-- DUR 위험 페어 (가짜 — 실제 위험 정보 X, 동작 검증용)
INSERT INTO dur_interaction_cache
  (dur_id, base_item_code, target_item_code, dur_type, prohibit_reason)
VALUES
  ('DUR_TEST_001',
   '999800001', '999800004',
   '효능군중복',
   '아세트아미노펜 함유 약을 동시 복용 시 간 손상 위험이 있어 주의가 필요한 것으로 식약처에 안내되어 있습니다. (테스트 데이터)'),
  ('DUR_TEST_002',
   '999800005', '999800010',
   '병용주의',
   '비스테로이드 항염증제와 아스피린을 동시 복용 시 위장관 출혈 위험이 증가할 수 있는 것으로 식약처에 안내되어 있습니다. (테스트 데이터)');

-- =====================================================
-- 2. 테스트 사용자 (Auth 동작 검증용)
-- =====================================================
-- 비밀번호: test1234 (해시값은 PasswordHasher 가 생성한 PBKDF2-SHA256 형식 가정).
-- TestMode 시 Auth 라우터가 평문 비교 fallback 도 허용 (PasswordHasher 미구현 단계)
-- 운영 모드 진입 시 본 행을 제거하고 정상 가입 절차로 등록.

INSERT INTO users (user_id, email, password_hash, user_name)
VALUES
  ('test_user_001', 'test@medibridge.local',
   '$pbkdf2$test_hash_placeholder$test1234',
   '테스트사용자'),
  ('test_user_002', 'demo@medibridge.local',
   '$pbkdf2$test_hash_placeholder$demo1234',
   '데모사용자');

-- =====================================================
-- 3. 가명 매핑
-- =====================================================
INSERT INTO pseudonym_map (anonymous_id, user_id) VALUES
  ('anon_test_001', 'test_user_001'),
  ('anon_test_002', 'test_user_002');

-- =====================================================
-- 4. 사용자 등록 약 풀 (test_user_001 이 4개 약 등록 상태)
-- =====================================================
INSERT INTO user_medication_pool
  (anonymous_id, item_code, reg_method, user_category, is_active)
VALUES
  ('anon_test_001', '999800001', 'MANUAL', '해열진통제',     TRUE),
  ('anon_test_001', '999800005', 'MANUAL', '해열진통제',     TRUE),
  ('anon_test_001', '999800007', 'VOICE',  '소화제',         TRUE),
  ('anon_test_001', '999800010', 'IMAGE',  '심혈관 보조',    TRUE);

-- =====================================================
-- 5. 복약 이력 (최근 7일 샘플)
-- =====================================================
INSERT INTO medication_intake_logs
  (intake_id, anonymous_id, item_code, intake_datetime, quantity, memo)
VALUES
  ('intake_test_001', 'anon_test_001', '999800001', NOW() - INTERVAL 6 DAY + INTERVAL 8  HOUR, 1, '아침 두통'),
  ('intake_test_002', 'anon_test_001', '999800001', NOW() - INTERVAL 5 DAY + INTERVAL 14 HOUR, 1, NULL),
  ('intake_test_003', 'anon_test_001', '999800007', NOW() - INTERVAL 4 DAY + INTERVAL 12 HOUR, 2, '점심 후 소화불량'),
  ('intake_test_004', 'anon_test_001', '999800005', NOW() - INTERVAL 3 DAY + INTERVAL 21 HOUR, 1, '근육통'),
  ('intake_test_005', 'anon_test_001', '999800010', NOW() - INTERVAL 2 DAY + INTERVAL 8  HOUR, 1, NULL),
  ('intake_test_006', 'anon_test_001', '999800001', NOW() - INTERVAL 1 DAY + INTERVAL 9  HOUR, 1, NULL);
