# Media API — 이미지 송수신

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.1 |
| **작성일** | 2026-05-06 |
| **모듈** | 미디어 송수신 (모듈 1 식별의 입력) |
| **관련 문서** | [프로토콜 v2 §2.3](../프로토콜_ver2.md), [요구사항 분석서 v2 §6.2 FR-C2](../요구사항_분석서_ver2.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> 폰(S24)이 카메라로 촬영한 이미지를 PWA → Client → MainServer 경로로 전달. **음성은 [SpeechApi.md](SpeechApi.md)** 참조 (폰 온디바이스 STT로 텍스트 변환 후 별도 엔드포인트 사용).

---

## 통신 흐름

```
[ 폰 PWA: <input capture> ]
   ↓ multipart/form-data (HTTP)
[ Client/PhoneAdapter (localhost:8000) ]
   ↓ multipart/form-data 또는 JSON+base64 (REST)
[ MainServer (8001/v1) ]
```

→ **두 단계 모두 본 명세에서 정의**. Client/PhoneAdapter도 같은 인터페이스 사용 (포트만 다름).

---

## 엔드포인트 목록

| HTTP | URL | 설명 | 인증 | 위치 |
| --- | --- | --- | --- | --- |
| POST | `/v1/media/image` | 이미지 1장 업로드 (식별 트리거) | ✓ (MainServer) / X (Client) | MainServer + Client |

> Client/PhoneAdapter는 폰 ↔ Client 어댑터라 별도 인증 없음. MainServer는 JWT 필수.

---

## 1. POST /v1/media/image — 이미지 업로드

### 요청 (multipart/form-data 권장)
- 헤더:
  - `Content-Type: multipart/form-data; boundary=...`
  - `Authorization: Bearer <JWT>` ✓ (MainServer만)
- 폼 필드:

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| `image` | file (binary) | ✓ | JPEG/PNG, 최대 10MB |
| `intent_hint` | string | – | 폰 STT 직전 발화 텍스트와 결합할 의도 힌트 (예: `"identify"`, `"register"`) |
| `request_id` | string | – | 클라가 부여한 요청 ID (UUID 권장) — 사진+음성 결합용 |

### 요청 (JSON + base64 대안)
파일 업로드가 곤란한 경우 사용. 비권장 (페이로드 33% 증가).
```json
{
  "image_base64": "iVBORw0KGgoAAAANSUhEUgAA...",
  "mime_type": "image/jpeg",
  "intent_hint": "identify",
  "request_id": "req_abc123"
}
```

### 응답 (Client/PhoneAdapter ← 폰)
- **202 Accepted** — 폰에 즉시 응답하고 비동기 처리
```json
{
  "request_id": "req_abc123",
  "status": "accepted",
  "next_poll_url": "/v1/media/result/req_abc123"
}
```
> Client는 받은 즉시 MainServer로 forward. 결과는 별도 폴링 또는 WebSocket으로 (확장).

### 응답 (MainServer ← Client)
- **200 OK** — 식별·DUR 결과까지 동기 반환 (참고: [PillApi.md](PillApi.md) `POST /v1/pill/identify` 와 동일 형식)
```json
{
  "request_id": "req_abc123",
  "candidates": [
    {
      "item_code": "201801234",
      "drug_name": "타이레놀정500mg",
      "confidence": 0.94,
      "match_keys": ["engraving", "shape", "color"]
    }
  ],
  "confidence_tier": "HIGH",
  "guidance": {
    "tts_text": "타이레놀정으로 추정됩니다. 화면을 확인해주세요.",
    "active_guide": null,
    "interactive_guide": null
  },
  "dur_check": {
    "result": "no_risk_found",
    "details": []
  }
}
```

### `confidence_tier` (3단계)
| 값 | 신뢰도 범위 | 화면 표시 분기 |
| --- | --- | --- |
| `HIGH` | ≥ 95% | 단일 결과 표시 |
| `MEDIUM` | 70~95% | Top-3 후보 비교 |
| `LOW` | < 70% | "식별 어려움. 약사·의사 확인 권유" |

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `INVALID_IMAGE_FORMAT` | JPEG/PNG 외 형식 |
| 413 | `IMAGE_TOO_LARGE` | 10MB 초과 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 (MainServer만) |
| 422 | `NO_PILL_DETECTED` | YOLO26이 알약 검출 실패 → guidance 응답에 능동 가이드 텍스트 포함 |
| 503 | `INFERENCE_SERVER_UNAVAILABLE` | 추론 서버 연결 끊김 |

### Schemas/ 매핑
- `MainServer/Schemas/MediaSchema.h::ImageUploadRequest`
- `MainServer/Schemas/MediaSchema.h::ImageUploadResponse`
- `Client/PhoneAdapter` 측은 multipart 파싱·forward만 (Schema는 MainServer와 공유 또는 별도 정의)

---

## Threading 정책

이미지 디코딩·인코딩이 무거우면:
- Client: `Client/Threading/WorkerPool` 에 디코드 작업 위임
- MainServer: 추론 서버 호출은 Drogon 코루틴(I/O 비동기)으로 자동 처리, 별도 워커 불필요

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — multipart/form-data 우선, JSON+base64 대안, confidence_tier 3단계, 에러 코드 정리 |
