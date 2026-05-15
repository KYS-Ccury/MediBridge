# Speech API — 음성 텍스트 (폰 온디바이스 STT)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.2 |
| **작성일** | 2026-05-06 (v0.1) / **개정일 2026-05-15 (v0.2)** |
| **모듈** | 음성 입력 (Stage 0.5 의도 분류 / 등록 LLM 입력) |
| **관련 문서** | [프로토콜 v2 §2.3 / §3.3](../프로토콜_ver2.md), [요구사항 분석서 v2 §4.4 FR-A4 / §6.3 FR-C3](../요구사항_분석서_ver2.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> ⭐ **v0.2 변경 핵심**: ① PhoneServer 가 **UtteranceForwarder 를 경유**해 utterance_received 시그널을 emit 하도록 수정 → VoiceController.current_text 갱신 → VoiceInputPage 실시간 표시 (commit `1b87fa4`). ② TestMode 한정 dev 로깅 추가 — `[Speech][DEV] len=N cat=X inj=Y text="앞80자"` (commit `0593288`). PII 보호 정책상 production 에서는 본문 미출력.

> 폰의 **온디바이스 STT(Galaxy AI / Android `SpeechRecognizer`)** 가 음성을 텍스트로 변환한 결과를 송신. **음성 바이너리는 본 API에 포함되지 않음** (대역폭 절감 + 개인정보 보호). 추론 PC Whisper STT는 확장 fallback (별도 명세).

---

## 통신 흐름

```
[ 폰 PWA ]
   ├─ 사용자가 텍스트 입력칸 탭 → 삼성 키보드 마이크 → Galaxy AI STT
   └─ 변환된 텍스트를 PWA가 받음
   ↓ application/json (HTTP)
[ Client/PhoneAdapter (localhost:8000) ]
   ↓ ⭐ UtteranceForwarder::forward()
   ├─ emit utterance_received(text, true)  ─→ VoiceController::current_text 갱신
   │                                         ─→ VoiceInputPage 실시간 표시
   └─ application/json (REST)
       ↓
[ MainServer (8001/v1) ]
   ├─ TestMode 시: 키워드 기반 의도 분류 + [Speech][DEV] 본문 미리보기 로그
   └─ Production 시: 추론서버로 의도 분류·등록 LLM 호출
       ↓
[ InferenceServer (10.10.10.128:8002) ]
```

---

## 엔드포인트 목록

| HTTP | URL | 설명 | 인증 | 위치 |
| --- | --- | --- | --- | --- |
| POST | `/v1/speech/utterance` | 폰 STT 결과 텍스트 송신 | ✓ (MainServer) / X (Client) | MainServer + Client |

---

## 1. POST /v1/speech/utterance — 발화 텍스트 송신

### 요청
- 헤더:
  - `Content-Type: application/json`
  - `Authorization: Bearer <JWT>` ✓ (MainServer만)
- 바디:
```json
{
  "text": "이 약 뭐야?",
  "stt_confidence": 0.93,
  "stt_engine": "galaxy_ai",
  "context": "daily_use",
  "image_request_id": "req_abc123",
  "language": "ko-KR"
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| `text` | string | ✓ | 폰 STT 결과 텍스트 (1~500자) |
| `stt_confidence` | float | – | 폰이 제공하는 신뢰도 (0.0~1.0). 미지원 시 생략 |
| `stt_engine` | string | – | `galaxy_ai` / `system_speech_recognizer` 등 |
| `context` | string | – | `daily_use` / `onboarding` / `confirmation` 등 |
| `image_request_id` | string | – | 같은 발화에 결합된 이미지의 request_id (사진+음성 동시 분석용) |
| `language` | string | – | BCP 47 언어 태그 (기본 `ko-KR`) |

### 응답
- **200 OK** (Stage 0.5 의도 분류 결과)
```json
{
  "intent": {
    "category": "PILL_IDENTIFY",
    "confidence": 0.91
  },
  "follow_up_action": {
    "type": "ROUTE_TO_VISION",
    "params": {
      "use_image_request_id": "req_abc123"
    }
  },
  "guidance": {
    "tts_text": "약을 카메라로 비춰주세요.",
    "active_guide": "PROMPT_CAMERA"
  },
  "injection_flag": false
}
```

### `intent.category` (Stage 0.5 카테고리)

[요구사항 분석서 v2 FR-A5-01](../요구사항_분석서_ver2.md) 정의:

| 값 | 의미 |
| --- | --- |
| `PILL_IDENTIFY` | 약 식별 요청 |
| `RISK_CHECK` | 위험 점검 요청 (DUR) |
| `INFO_LOOKUP` | 단순 정보 확인 (e약은요) |
| `REGISTER_REQUEST` | 등록 요청 |
| `HISTORY_QUERY` | 복약 이력 조회 요청 |
| `REPORT_REQUEST` | 보고서 출력 요청 |
| `OTHER` | 기타 (인젝션 의심 포함) |

### `injection_flag`

`true` 면 분류기 LLM이 **프롬프트 인젝션 시도 패턴을 감지**하여 강제로 `OTHER`로 분류한 경우. 운용 서버는 로깅·감시 강화.

### 에러
| 상태 | 코드 | 의미 |
| --- | --- | --- |
| 400 | `EMPTY_TEXT` | text가 빈 문자열 |
| 400 | `TEXT_TOO_LONG` | text 500자 초과 |
| 401 | `INVALID_TOKEN` / `EXPIRED_TOKEN` | 인증 실패 (MainServer만) |
| 503 | `INFERENCE_SERVER_UNAVAILABLE` | 추론 서버 연결 끊김 |

### Schemas/ 매핑
- `MainServer/Schemas/SpeechSchema.h::UtteranceRequest`
- `MainServer/Schemas/SpeechSchema.h::UtteranceResponse`
- `MainServer/Schemas/SpeechSchema.h::IntentResult`
- `MainServer/Schemas/SpeechSchema.h::FollowUpAction`

---

## (확장 fallback) POST /v1/speech/audio

폰 온디바이스 STT 미사용·실패 시 음성 바이너리를 추론 PC Whisper로 보내는 fallback. **MVP 미구현, 본 명세에서 자리만 정의.**

- 헤더: `Content-Type: audio/wav` 또는 `audio/m4a`
- 바디: 음성 바이너리
- 응답: `UtteranceRequest` 와 같은 구조 + 자동 STT 변환된 `text` 포함

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — 폰 온디바이스 STT 텍스트 송신, Stage 0.5 의도 카테고리, injection_flag, 확장 fallback 자리 정의 |
| **(2026-05-13 메모)** | [ApiOverview v0.4](ApiOverview.md) 호환. 본 명세 변경 없음. 메인서버 `Routers/Speech.cpp` TestMode 동작. 운영 모드는 LLM PC(`10.10.10.128:8002`) Stage 0.5 가동 필요. `Services/Inference/IntentInferenceClient` 호출 형태 정의됨. |
| **v0.2** | **2026-05-15** | ① 클라 PhoneServer 가 UtteranceForwarder 경유로 변경 — utterance_received 시그널 → VoiceController.current_text 갱신 → VoiceInputPage 실시간 표시 (commit `1b87fa4`). ② TestMode 한정 dev 로깅 추가 — `[Speech][DEV] len=N cat=X inj=Y text="앞80자"` (Config::test_mode() true 일 때만, commit `0593288`). API 페이로드·응답 스키마 변경 없음 (호환). |
