# Crop 이미지 영속화 설계 — 식별 이력에 박싱된 사진 표시

| 항목 | 값 |
|---|---|
| 작성일 | 2026-05-15 |
| 작성자 | (팀 검토용 초안) |
| 상태 | **설계만 — 구현은 Phase 4 (Vision PC YOLO 안정화 이후)** |
| 관련 문서 | [Api/MediaApi.md](Api/MediaApi.md) · [Api/PillApi.md](Api/PillApi.md) · [Api/HistoryApi.md](Api/HistoryApi.md) · [DB_ERD_ver4.md](DB_ERD_ver4.md) |

---

## 1. 목적

사용자가 **식별 결과 화면** 과 **복약 이력 화면** 에서 알약의 박싱된 crop 사진을 함께 보고, 어떤 알약이 어떤 이름인지 시각적으로 즉시 구별할 수 있게 한다.

### 사용자 가치
- "타이레놀500" 이라는 텍스트만 보던 것 → 실제 본인 약의 모양·각인·색을 사진으로 확인
- 복약 이력 일주일 분 표시 시 — 텍스트 list 가 아니라 약 사진 갤러리처럼 식별 용이
- 고령층 사용자에게 큰 도움 (한자 약명·작은 글자 부담 ↓)

---

## 2. 현재 흐름 (crop 미보존)

```
폰 → 클라 → 메인 /v1/media/intent → 보관 PC PUT (원본) → /v1/media/commit → /v1/pill/identify
                                                                                   ↓
                                                              Vision PC YOLO 검출
                                                                                   ↓
                                                              crop N개 추출 (메모리만, 휘발)
                                                                                   ↓
                                                              키값 4종 (각인·색·모양·크기) 반환
                                                                                   ↓
                                                              메인서버: 식별 후보 응답
                                                                                   ↓
                                                              클라 결과 화면: 약명 텍스트만
```

→ 결과 화면 / 이력 화면 모두 **사진 0장**.

---

## 3. 제안 흐름 (crop 영속, AWS S3+RDS 패턴 그대로 확장)

```
폰 → 클라 → 메인 intent (원본) → 보관 PC PUT (원본) → commit → identify
                                                                ↓
                                              Vision PC YOLO 검출
                                                                ↓
                                              crop N개 + 4종 키값 추출
                                                                ↓
                          ⭐ Vision PC → 메인 intent(IDENTIFY_CROP, parent_photo_id=원본) × N회
                                       → 보관 PC PUT (crop)                          × N회
                                       → commit (crop)                               × N회
                                                                ↓
                                              Vision PC 응답: 후보별 crop_photo_id 포함
                                                                ↓
                              메인서버: candidates[].crop_photo_id 응답
                                                                ↓
                              클라 결과 화면: <Image source="<crop_url>" />
                                                                ↓
                              복약 기록 시 intake_logs.crop_photo_id 저장
                                                                ↓
                              이력 화면: 항목별 썸네일 + 약명
```

---

## 4. DB 마이그레이션 (`003_crop_photo_storage.sql`)

```sql
-- 1) photo_storage 에 parent_photo_id 추가 — crop ↔ 원본 추적
ALTER TABLE photo_storage
    ADD COLUMN parent_photo_id VARCHAR(64) NULL AFTER photo_id,
    ADD INDEX idx_parent_photo (parent_photo_id);

-- 2) purpose ENUM 확장 — IDENTIFY_CROP 추가
ALTER TABLE photo_storage
    MODIFY COLUMN purpose ENUM('IDENTIFY','REGISTER','EVIDENCE','IDENTIFY_CROP') NOT NULL;

-- 3) intake_logs 에 crop_photo_id 추가 — 복약 이력 ↔ crop 매핑
ALTER TABLE intake_logs
    ADD COLUMN crop_photo_id VARCHAR(64) NULL AFTER memo,
    ADD CONSTRAINT fk_intake_crop
        FOREIGN KEY (crop_photo_id) REFERENCES photo_storage(photo_id)
        ON DELETE SET NULL;

-- 4) photo_storage retention 분기 (원본은 5분 PENDING, crop 은 영속)
--    cleanup 잡이 IDENTIFY_CROP 은 청소 안 하도록 WHERE 절 변경.
```

### 영향
- `photo_storage` 신규 컬럼 1개 + ENUM 확장 + 인덱스 1개
- `intake_logs` 신규 컬럼 1개 + FK 제약
- 청소 잡 SQL 수정 (`purpose != 'IDENTIFY_CROP'` 조건 추가)

---

## 5. API 변경

### 5.1 MediaApi v0.2 → v0.3

`POST /v1/media/intent` 요청 페이로드 확장:
```json
{
  "mime_type": "image/jpeg",
  "size_bytes": 50000,
  "purpose": "IDENTIFY_CROP",
  "parent_photo_id": "ph_xxx"
}
```

응답은 기존과 동일 (`photo_id`, `storage_url`, `put_token` 발급).

### 5.2 PillApi v0.3 → v0.4

`POST /v1/pill/identify` 응답에 후보별 `crop` 정보 추가:
```json
{
  "candidates": [
    {
      "item_code": "999800001",
      "drug_name": "타이레놀500",
      "confidence": 0.93,
      "in_user_pool": true,
      "match_keys": ["각인:T", "색:WHITE", "모양:ROUND"],
      "crop_photo_id": "ph_crop_abc123",
      "crop_url": "http://10.10.10.122:8004/storage/photos/anon_xxx/ph_crop_abc123.jpg?get_token=...",
      "crop_url_expires_at": "2026-05-15T14:00:00Z"
    },
    ...
  ],
  ...
}
```

`crop_url` 은 메인서버가 발급한 short-lived GET 토큰 포함 (TTL 5-10분). 클라가 만료되면 별도 `GET /v1/media/get_token?photo_id=...` 로 재발급.

### 5.3 HistoryApi v0.1 → v0.2

`POST /v1/history/record` 요청에 `crop_photo_id` 옵션 추가:
```json
{
  "item_code": "999800001",
  "quantity": 1,
  "memo": "아침 식후",
  "crop_photo_id": "ph_crop_abc123"
}
```

`GET /v1/history/list` 응답에 `crop_url` 포함:
```json
{
  "items": [
    {
      "intake_id": 42,
      "item_code": "999800001",
      "drug_name": "타이레놀500",
      "quantity": 1,
      "memo": "...",
      "intake_at": "2026-05-15T08:30:00",
      "time_slot": "MORNING",
      "crop_photo_id": "ph_crop_abc123",
      "crop_url": "http://10.10.10.122:8004/storage/photos/anon_xxx/ph_crop_abc123.jpg?get_token=...",
      "crop_url_expires_at": "..."
    }
  ],
  "total_count": 6
}
```

---

## 6. Vision PC 측 흐름 (인효 담당과 협의 필요)

YOLO 검출 후 crop N개를 보관 PC 에 PUT 하는 과정:

```python
# Vision PC pseudocode
def detect_remote(photo_id, storage_url, get_token, mime, purpose):
    # 1) 원본 GET
    img = http_get(storage_url, headers={"Authorization": f"Bearer {get_token}"})

    # 2) YOLO 검출
    detections = yolo.detect(img)   # list of (bbox, crop_image, confidence)

    # 3) 각 crop 을 보관 PC 에 PUT
    candidates = []
    for det in detections:
        crop_bytes = encode_jpeg(det.crop_image)

        # 메인서버에 intent 요청 (Vision PC 도 JWT 필요 — Vision 전용 서비스 계정 또는 토큰 패스스루)
        intent = http_post_main("/v1/media/intent", {
            "mime_type": "image/jpeg",
            "size_bytes": len(crop_bytes),
            "purpose": "IDENTIFY_CROP",
            "parent_photo_id": photo_id,
        })
        crop_photo_id = intent["photo_id"]

        # 보관 PC 에 직접 PUT
        http_put(intent["storage_url"], crop_bytes,
                 headers={"Authorization": f"Bearer {intent['put_token']}"})

        # commit
        http_post_main(f"/v1/media/commit", {"photo_id": crop_photo_id})

        # OCR + 색·모양·크기
        keys = analyze_keys(det.crop_image)
        candidates.append({
            "match_keys": keys,
            "confidence": det.confidence,
            "crop_photo_id": crop_photo_id,
        })

    # 4) 키값 매칭 (메인서버 측 식약처 캐시) — 후보 → 약 item_code 결정
    # (이미 기존 흐름 — Vision PC 또는 메인서버 측 매칭 단계에서 처리)

    return {"candidates": candidates, "confidence_tier": "HIGH"}
```

**문제점·검토**:
- Vision PC 의 메인서버 호출 인증 — 사용자 JWT pass-through vs 서비스 계정 토큰
- crop N 개당 intent+PUT+commit 3회 호출 → N=5 면 15회 호출. 응답 시간 가산
- 대안: **batch intent** API 신설 (`POST /v1/media/intent_batch` — N개 photo_id 한 번에 발급)

---

## 7. 메인서버 변경

| 영역 | 변경 |
|---|---|
| `MainServer/Schemas/PillSchema` | `IdentifyResponse.Candidate` 에 `crop_photo_id` · `crop_url` · `crop_url_expires_at` 필드 |
| `MainServer/Schemas/HistorySchema` | `IntakeRecord` 에 `crop_photo_id`, `IntakeListItem` 에 `crop_url` |
| `MainServer/Schemas/MediaSchema` | `IntentRequest` 에 `parent_photo_id` · `purpose=IDENTIFY_CROP` |
| `MainServer/Routers/Pill.cpp` | identify 응답 매핑 — crop_photo_id 별 get_token 발급 + crop_url 조립 |
| `MainServer/Routers/History.cpp` | record 시 crop_photo_id 저장 · list 시 get_token 발급 + crop_url 조립 |
| `MainServer/Services/Media` | get_token batch 발급 함수 (N개 photo_id 한 번에) |
| `MainServer/Database/Migrations/003_crop_photo_storage.sql` | 신규 |
| 청소 잡 | `purpose != 'IDENTIFY_CROP'` 조건 추가 (crop 은 영속) |

---

## 8. 클라이언트 변경

| 영역 | 변경 |
|---|---|
| `Client/Backend/Models/PillCandidateListModel` | `crop_url` 필드 + `CropUrlRole` |
| `Client/Backend/Models/HistoryListModel` | `crop_url` 필드 추가 |
| `Client/Frontend/Pages/IdentifyResultPage.qml` | 후보 카드에 `Image { source: model.crop_url; sourceSize: 64x64 }` |
| `Client/Frontend/Pages/HistoryPage.qml` | 항목 좌측에 썸네일 (64x64) + 약명 우측 |
| `Client/MainServerClient/PillApiClient` | identify 응답 파싱 시 crop_url 보존 |
| `Client/MainServerClient/HistoryApiClient` | list 응답 파싱 시 crop_url 보존 |
| `Client/Backend/Controllers/PillController` | candidates 파싱 시 crop_url 모델에 전달 |
| `Client/Backend/Controllers/HistoryController` | items 파싱 시 crop_url 모델에 전달 |

### QML 이미지 캐싱
Qt `Image { cache: true; asynchronous: true }` 로 동일 URL 재방문 시 메모리 캐시 활용. crop_url 이 short-lived 토큰을 포함하므로 만료 시 자동 재발급 로직 필요 (Connections + retry).

---

## 9. 정책·보안 검토 사항

| 항목 | 결정 필요 | 권장 |
|---|---|---|
| **crop 영속 기간** | 영원 vs N년 | 복약 이력 row 있는 한 영원, 이력 삭제 시 ON DELETE SET NULL → 청소 잡으로 고아 crop 30일 후 삭제 |
| **crop GET 인증** | short-lived 토큰 vs 평문 URL | short-lived 토큰 (TTL 10분, 기존 패턴 유지) |
| **사용자 삭제권** | GDPR-ish — 이력 삭제 시 crop 도 같이 | ON DELETE SET NULL + 청소 잡으로 30일 후 디스크 삭제 |
| **재식별 중복 crop** | 매번 저장 vs 같은 item_code 한 번만 | **매번 저장** (간단·일관성 ↑, 디스크 부담 무시 가능) |
| **디스크 누적** | 사용자 × 약 × crop 평균 | 1000 사용자 × 10약 × 평균 30 crop × 30 KB ≈ 9 GB. 1년 운영 가정도 30 GB 미만 |
| **PII** | crop 자체는 익명화 — anonymous_id 매핑 외 정보 없음 | 현재 매핑 그대로 |

---

## 10. 작업량 산정 (구현 시)

| 영역 | 시간 |
|---|---|
| DB 마이그레이션 003 + 청소 잡 분기 | 1시간 |
| Schemas (PillSchema · HistorySchema · MediaSchema) | 1시간 |
| MainServer Routers (Pill · History · Media) | 2시간 |
| MainServer get_token batch 발급 | 1시간 |
| Vision PC 측 crop PUT 흐름 (인효 협업) | 3-4시간 |
| 클라 PillCandidateListModel · HistoryListModel | 1시간 |
| 클라 IdentifyResultPage · HistoryPage QML 이미지 | 2시간 |
| 클라 ApiClient 응답 파싱 | 1시간 |
| 통합 테스트 + 디버깅 | 2-3시간 |
| **합계** | **약 14-16시간** |

---

## 11. 권장 진입 시점

```
Phase 1 (완료) — TTL 30일 회귀, LLM PC 본 작업
Phase 2 — Vision PC YOLO 재학습 (인효, 진행 중)
Phase 3 — Vision PC FastAPI /vision/detect_remote 구현 (인효)
Phase 4 ⭐ — crop 영속화 (본 문서)
            Vision PC 가 안정적으로 동작해야 mock 없이 통합 가능
```

→ **인효 님의 Vision PC YOLO 안정화 완료 + /vision/detect_remote 동작 확인 후 진입**.

---

## 12. 후속 확장 아이디어

- **crop thumbnail** — JPEG 64x64 또는 WebP 로 리사이즈 후 별도 저장 (목록 화면 로딩 속도 ↑)
- **crop 자동 회전 보정** — EXIF 또는 YOLO bbox 기반
- **crop OCR overlay** — 각인 텍스트를 crop 위에 표시 (사용자가 자기 약 인식 ↑)
- **crop 갤러리 페이지** — 사용자의 평생 crop 모음을 약별·시각별로 그리드 표시
