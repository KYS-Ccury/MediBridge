-- =====================================================
-- 002_photo_storage_intent — ⑤+⑥ 사진 흐름 지원
-- =====================================================
-- photo_storage 에 의향(intent) → 업로드 완료 (READY) 상태 트래킹용 컬럼 추가.
--
-- 흐름:
--   1) POST /v1/media/intent  → INSERT (status=PENDING, storage_path 예약)
--   2) 클라가 보관 PC 에 직접 PUT
--   3) 메인이 보관 PC HEAD 로 확인 또는 클라가 /v1/media/commit 호출 → status=READY
--   4) 만료 (예: 5분 PENDING 이상) → 청소 잡으로 status=EXPIRED
-- =====================================================

ALTER TABLE photo_storage
    ADD COLUMN status ENUM('PENDING','READY','EXPIRED','FAILED') NOT NULL DEFAULT 'READY'
        AFTER purpose,
    ADD COLUMN expires_at DATETIME NULL
        AFTER status,
    ADD COLUMN committed_at DATETIME NULL
        AFTER expires_at,
    ADD INDEX idx_status_expires (status, expires_at);

-- 기존 row 는 모두 READY 로 간주 (TestMode 시드 보존).
-- 신규 row 는 라우터에서 명시적으로 PENDING 시작.
