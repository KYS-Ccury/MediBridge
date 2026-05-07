# 메디브릿지 시스템 연결 구조 (v2.2)

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 시스템 연결 구조 정의 (역할 분류 + 디바이스 간 연결 흐름 + 가명화 정책 + TTS 정책) |
| **버전** | v2.2 |
| **작성일** | 2026-05-07 |
| **이전 버전** | v2.1 (in-place 갱신) / v2.0 / v1.0 → `Docs/Old/시스템_연결구조_ver1_2026-05-07.md` |
| **상태** | 🟡 **WIP — 사용자가 단계별 흐름을 추가하며 검토 진행 중** |

> ⚠️ **본 문서는 기존 v2/v3 문서(시스템 흐름 정리본·기획서 등)의 수정 제안임**.
> 핵심 차이: ① **데이터 보관 PC** 신규 도입 — 기존 4대 구성 → **5대 + 폰** ② **가명화 매핑(pseudonym_map)** 도입 ③ **추론 PC 를 LLM 전용 / Vision 전용 2대로 분리** (학습+추론 동거, 향후 분리 가능 설계).
> 검토 완료 후 확정 시 기존 v2 문서들을 v3 로 일괄 갱신해야 함.

> ⭐ **v2.2 변경 핵심** (사용자 확정 — Onboarding RAG round-trip + TTS 정책):
> - **§2.8 Onboarding RAG round-trip 흐름** 신규 — 음성 등록 시 의료용어를 모르는 사용자를 위한 약명 정규화 (RAG → 식약처 매칭 → 동명·동성분 분기 질문 → TTS 안내 → 사용자 응답 다회 반복)
> - **§2.1 TTS 음성 안내 정책 명확화** — 클라 자체 TTS (Qt6 `QTextToSpeech` + Windows SAPI 한국어) 우선, 메인 서버 TTS fallback 어댑터 구조
> - **§2.9 TTS 어댑터 정책** 신규 — 데이터량·품질·환경 비교, 어댑터 인터페이스
> - **§2.8 남용 방어** — `max_rounds=5`, `max_input_tokens=200`, rate_limit 명시 (구체 임계값은 구현 시 측정 후 확정)
>
> ⭐ **v2.1 변경 핵심** (사용자 확정):
> - **§1.2 IP·호스트 매트릭스** 신규 — 실제 운용 IP 확정
> - **추론 PC 역할 재분배** — ⑤ 추론 + ⑥ 학습 → ⑤ LLM(학습+추론) / ⑥ Vision(학습+추론) 의 **카테고리별 분리**, 각 PC 내에서 학습+추론 동거 (네트워크 분리 가능 설계)
> - **데이터 보관 PC = 사진 전용** 명시 (음성 데이터 원본은 폰 외부로 나가지 않음 → 보관 X. 추출 텍스트 구조화 데이터는 메인 서버 MariaDB 보관 OK)
> - **§2.6 학습 데이터 일괄 다운로드 흐름** 신규 — Vision 학습 시 Data Storage PC 에서 SCP/일괄
>
> ⭐ **v2.0 변경 핵심**:
> - **§8 가명화·익명화 정책** 신규 — `pseudonym_map` 단일 끊기 매핑
> - **§2.5 데이터 보관 PC 메타 (photo_storage)** 정의 추가
> - DB ERD v3 → v4 로 동시 갱신 ([DB_ERD_ver4.md](DB_ERD_ver4.md) 참조)

---

## 1. 디바이스 / 서버 역할 분류

### 1.1 디바이스 일람

| # | 디바이스 | OS / 하드웨어 | 핵심 역할 (한 줄) |
| --- | --- | --- | --- |
| ① | **핸드폰 (안드로이드 S24)** | Android 14+ | 카메라·마이크·온디바이스 STT 자원 제공 (입력 디바이스) |
| ② | **클라이언트 PC** | Windows 10/11 | 사용자 GUI + 폰 조작 + 메인서버 통신 + 화면 캡쳐 |
| ③ | **메인 서버 PC** | Ubuntu 24.04 | 비즈니스 로직 + MariaDB(메타+가명매핑) + 라우팅 컨트롤 |
| ④ | **데이터 보관 PC** ⭐ | Ubuntu 24.04 (대용량 디스크) | **사진 전용** 영속 저장 (음성 원본 보관 X) |
| ⑤ | **LLM PC (학습+추론 동거)** ⭐ | Ubuntu 24.04 (GPU) | LLM 학습 + LLM 추론 (Stage 0.5 의도 분류 / Onboarding RAG / 비의료 일반 안내). **네트워크 분리 가능 설계** |
| ⑥ | **Vision PC (학습+추론 동거)** ⭐ | Ubuntu 24.04 (GPU) | YOLO·PaddleOCR·OpenCV 학습 + 추론. **네트워크 분리 가능 설계** |

> 📌 **5대 + 폰 구성**. ⑤·⑥ 은 본디 학습 서버와 추론 PC 를 분리하는 것이 운영상 안전하나, **이번 프로젝트는 여유 PC 부족**으로 학습+추론 한 PC 동거. 단, **포트·네트워크 정책상 분리 가능하도록 설계**해 향후 PC 증설 시 즉시 분리 가능.

### 1.2 ⭐ IP·호스트 매트릭스 (신규)

| # | 디바이스 | IP / 호스트 | OS | 비고 |
| --- | --- | --- | --- | --- |
| ① | 핸드폰 | (USB → 클라 PC `adb reverse`) | Android | `localhost:5037` adb 데몬 경유 |
| ② | 클라이언트 PC | **로컬 (localhost)** | Windows 10/11 | Qt6 + QML, 폰 USB 직결 |
| ③ | 메인 서버 PC | **10.10.10.97** | Ubuntu 24.04 | Drogon C++ + MariaDB 8001/tcp |
| ④ | 데이터 보관 PC | **10.10.10.122** | Ubuntu 24.04 | 사진 전용 저장소 |
| ⑤ | LLM PC (학습+추론) | **10.10.10.120** | Ubuntu 24.04 (GPU) | FastAPI 8002/tcp |
| ⑥ | Vision PC (학습+추론) | **10.10.10.128** | Ubuntu 24.04 (GPU) | FastAPI 8003/tcp |

> 📌 **사내 LAN 대역 `10.10.10.0/24`** 일괄. 외부 노출 없음 (식약처 OpenAPI 호출만 메인서버에서 HTTPS 아웃바운드).
> 📌 **추론 서버 포트 분리** — LLM 8002, Vision 8003. 향후 PC 분리 시에도 포트 그대로 유지.

### 1.3 역할별 핵심 보유 자원

| 디바이스 | DB | GPU | 디스크 (대용량) | 카메라/마이크 |
| --- | --- | --- | --- | --- |
| ① 핸드폰 | — | (NPU/Galaxy AI) | — | ✓ |
| ② 클라이언트 PC | — | — | — | — |
| ③ 메인 서버 | **MariaDB (메타+가명매핑+이력+텍스트 추출 결과)** | — | (보통) | — |
| ④ 데이터 보관 PC | — | — | **✓ 대용량 ✓ (사진 전용)** | — |
| ⑤ LLM PC | — | ✓ | (모델 가중치 + LLM 학습셋) | — |
| ⑥ Vision PC | — | ✓ | (모델 가중치 + 학습 시 일괄 다운로드) | — |

> 📌 **음성 데이터 처리 정책**: 폰 온디바이스 STT 결과 텍스트는 클라 PC → 메인 서버 → LLM PC 로 흐르며, **음성 바이너리 원본은 폰 외부로 나가지 않음** → Data Storage PC 에 보관 X.
> 단, **추출된 구조화 데이터** (예: "타이레놀 2알" → 약명·개수)는 메인 서버 MariaDB (`medication_intake_logs` 등) 에 보관 OK.

---

## 2. 디바이스 간 연결 1:1 (사용자 정의 흐름)

### 2.1 ① 핸드폰 ↔ ② 클라이언트 PC (USB)

| 동작 | 흐름 |
| --- | --- |
| **촬영 조작** | 클라 PC → 폰 (ADB/scrcpy) → 폰 카메라 앱 제어 |
| **촬영 화면 보기** | 폰 → 클라 PC (scrcpy 미러링) |
| **사진 데이터 확보** | 클라 PC 가 폰 화면 캡쳐 (`adb exec-out screencap -p`) → PNG 직접 수신 |
| **음성 인식 시작** | 클라 PC → 폰 (음성 인식 ON 신호) |
| **음성 인식 결과** | 폰 내부 STT → 텍스트 변환 → 클라 PC 로 텍스트만 송신 (음성 바이너리 X) |
| **음성 안내 (TTS)** | **클라 PC 자체 TTS 우선** (Qt6 `QTextToSpeech` + Windows SAPI 한국어 보이스). 환경 미지원·품질 부족 시 **메인 서버 TTS fallback** (어댑터). 폰 스피커 출력은 클라 → 폰 USB 채널로 위임 가능. 상세 §2.9 |
| **사용 시점 (음성)** | 약품 등록 시 — STT 결과를 클라 PC 경유해 LLM 보정 |

> 📌 **음성 흐름 핵심**: 음성 바이너리는 폰 외부로 나가지 않음. 텍스트만 클라 PC → LLM(메인/추론서버) → 보정된 텍스트 응답.

### 2.2 ② 클라이언트 PC ↔ ③ 메인 서버 (LAN)

| 동작 | 흐름 |
| --- | --- |
| **사진 업로드 신호** | 클라 PC → 메인 서버 ("사진 업로드 의향" 통지 + 메타데이터: 크기·MIME·요청자) |
| **저장 가능 판정** | 메인 서버 → 데이터 보관 PC ("디스크·메모리 여유 있나?") |
| **저장 경로 발급** | 데이터 보관 PC → 메인 서버 (`OK + 경로`) |
| **클라에게 응답** | 메인 서버 → 클라 PC (`OK + 데이터 보관 PC 경로 + 단기 서명 토큰`) |
| **메타데이터 저장** | 메인 서버 → MariaDB (`photo_storage` 1행 INSERT — anonymous_id, 경로, 메타) |
| **실제 사진 전송** | 클라 PC → 데이터 보관 PC 직접 (메인서버 우회, 발급된 경로로 PUT, 토큰 검증) |

> 📌 **컨트롤 평면(메인) ↔ 데이터 평면(데이터 보관) 분리**. AWS S3+RDS 패턴과 유사.

### 2.3 ③ 메인 서버 ↔ ④ 데이터 보관 PC (사진 데이터 평면)

| 동작 | 흐름 |
| --- | --- |
| **사전 점검** | 메인 서버 → 데이터 보관 PC (디스크·메모리 상태 질의) |
| **경로 사전 발급** | 메인 서버 ← 데이터 보관 PC (저장 가능 경로 응답) |
| **사진 본체 저장** | 클라/추론/학습 PC → 데이터 보관 PC (직접 PUT, 경로는 메인서버에서 받음) |
| **사진 본체 조회** | 클라/추론/학습 PC → 메인서버 (경로 질의) → 데이터 보관 PC (직접 GET) |
| **메타데이터 일관성** | 메인 서버 MariaDB 의 `photo_storage` 행 ↔ 데이터 보관 PC 실제 파일 1:1 매칭 |

### 2.4 추론 PC 의 사진 접근 (사용자 명시)

> "이후 이 사진데이터는 추론PC나 학습PC에서도 불러올 수 있도록 할 것, 이 때도 경로는 메인서버를 통해 물어보고 실제 데이터는 데이터보관PC로부터 받을 것"

| 디바이스 | 흐름 |
| --- | --- |
| ⑥ Vision PC (10.10.10.128) | 1. 메인서버에 "request_id 사진 경로?" 질의 → 2. 메인서버가 MariaDB 조회 → 경로 응답 → 3. Vision PC 가 데이터 보관 PC 에서 직접 GET → 4. YOLO·OCR·OpenCV 추론 |
| ⑤ LLM PC (10.10.10.120) | 사진 직접 접근 거의 없음. **텍스트 입력 추론** 중심 (Stage 0.5 의도 분류 / Onboarding RAG / 일반 안내). 필요 시 동일 패턴(메인서버 경유 경로 질의 → 직접 GET) 적용 가능 |

### 2.5 ⭐ 데이터 보관 PC 메타 (photo_storage)

데이터 보관 PC 에 저장된 모든 사진은 **메인 서버 MariaDB 의 `photo_storage` 테이블**에 메타데이터가 등록됨. 본 테이블이 클라/추론/학습 PC 와 데이터 보관 PC 사이의 **인덱스 역할**.

```
photo_storage (
  photo_id        PK    -- UUID
  anonymous_id          -- 가명 매핑 (FK to pseudonym_map)
  storage_path          -- 데이터 보관 PC 경로
  mime_type
  file_size_bytes
  taken_at
  request_id            -- 클라 요청 추적용
  uploaded_at
)
```

> **파일명 규칙**: `<storage_path>/YYYY/MM/DD/<anonymous_id>_<photo_id>.jpg`
> → 파일 시스템 자체에 user 식별자 노출 ❌

### 2.6 ⭐ Vision PC 학습 시 — 학습 데이터 일괄 다운로드 (사용자 확정)

> 사용자 확정: "학습 데이터셋은 학습 시 일괄 다운로드".

학습 시점에 Vision PC (⑥, 10.10.10.128) 가 데이터 보관 PC (④, 10.10.10.122) 로부터 학습용 사진 일괄 수집:

| 단계 | 흐름 |
| --- | --- |
| 1. 학습용 데이터셋 경로 일괄 질의 | Vision PC → 메인 서버 (`GET /v1/training/dataset/manifest?from=...&to=...&label=...`) |
| 2. 메인 서버 MariaDB 조회 | `photo_storage` 테이블 SELECT (라벨·기간 필터) |
| 3. 경로 매니페스트 응답 | 메인 서버 → Vision PC (path 리스트 + 단기 서명 토큰) |
| 4. 학습 데이터 일괄 SCP/HTTP | Vision PC ← 데이터 보관 PC (경로 리스트 일괄 PULL, 메인서버 우회) |
| 5. 학습 진행 | Vision PC GPU 에서 YOLO·OCR fine-tuning |
| 6. 모델 가중치 산출 | Vision PC 자체 보관 (운용 중 추론도 같은 PC) |

> 📌 **학습+추론 동거** 이므로 모델 가중치 SCP 배포 단계 생략 (같은 PC 안에서 즉시 사용).
> 📌 **네트워크 분리 시 (향후 PC 증설)** — 학습 PC ↔ 추론 PC 사이에 모델 가중치 SCP 단계 부활.

### 2.7 음성 텍스트 추론 흐름 (Stage 0.5 / Onboarding RAG)

> **음성 원본은 폰 외부로 나가지 않음** — 데이터 보관 PC·메인 서버 모두에서 보관 X. 텍스트만 흐름.

| 단계 | 흐름 |
| --- | --- |
| 1. 폰 STT | 폰 내장 STT (Galaxy AI / `SpeechRecognizer`) → 텍스트 변환 |
| 2. 텍스트 전송 | 폰 → 클라 PC (USB ADB) → 메인 서버 10.10.10.97 (`POST /v1/speech/utterance`) |
| 3. LLM 추론 요청 | 메인 서버 → LLM PC 10.10.10.120 (텍스트 입력, FastAPI 8002/tcp) |
| 4. LLM 응답 (분류 결과 또는 보정 텍스트) | LLM PC → 메인 서버 |
| 5. 추출 구조화 데이터 저장 | 메인 서버 → MariaDB (예: `medication_intake_logs`, `user_medication_pool`) — **추출 결과만 보관** |
| 6. 클라 응답 | 메인 서버 → 클라 PC → (필요 시) TTS 음성 안내 |

> 📌 **STT 원본 텍스트** 는 처리 중 메모리에서만 사용, **DB 영속 저장 X** (구조화 추출 결과만 저장).
> 📌 **개인정보 이중 최적화** — 음성 바이너리 비송신 + STT 원본 텍스트 비저장.

### 2.8 ⭐ Onboarding RAG round-trip 흐름 (음성 등록 약명 정규화)

> 사용자가 **의료용어를 모르는 상태**에서 음성으로 약을 등록하려 할 때 (예: "빨간 알약 진통제 등록할게"), 식약처 의약품 정보 RAG 로 **약명 후보 추출 + 동명·동성분 분기 질문 + TTS 음성 안내 + 사용자 응답** 을 다회 round-trip 으로 반복해 정확한 `item_code` 를 확정한 뒤 풀에 등록.
> 본 흐름의 API 명세: [PillApi v0.3 §3 `POST /v1/pill/onboarding/normalize`](Api/PillApi.md).

| 단계 | 흐름 |
| --- | --- |
| 1. 폰 STT | 폰 내장 STT → "타이레놀 등록할게" 텍스트 |
| 2. 텍스트 전송 (round=1) | 폰 → 클라 PC → 메인 서버 10.10.10.97 (`POST /v1/pill/onboarding/normalize` body: `utterance_text`, `round=1`) |
| 3. RAG 요청 | 메인 서버 → LLM PC 10.10.10.120 (약명 후보 추출 NER + 식약처 의약품 **ChromaDB 벡터 검색**) |
| 4. RAG 응답 | LLM PC → 메인 서버 (후보 N개 + hint) |
| 5. state 분기 | 메인 서버: 후보 1건 → `RESOLVED` / 2~5건 → `NEED_DISAMBIGUATION` / 0건 → `NOT_FOUND` |
| 6. TTS 안내 | 메인 서버 → 클라 PC (응답 `question.tts_text` 또는 `confirmation.tts_text` 또는 `tts_text` 포함) → **클라 자체 TTS 합성·재생** (어댑터, §2.9) |
| 7. 사용자 응답 수신 | 음성(폰 STT) 또는 화면 터치 → 클라 PC (선택값 + `prev_choice_token`) |
| 8. 다음 라운드 (round=k+1) | 클라 PC → 메인 서버 (round=k+1, `prev_choice_token`, `prev_selection`) → 3 단계 반복 |
| 9. RESOLVED + 사용자 OK | 클라 PC → 메인 서버 (`POST /v1/pill/pool` body: 정규화된 `item_code`, `reg_method=VOICE`) → MariaDB INSERT |

> 📌 **다회 round-trip 허용** — 단, **남용 방어** 필수: `max_rounds=5` 기본, `max_input_tokens=200`, 분당 호출 횟수 제한. 구체 임계값은 구현 시 운영 측정 후 확정.
> 📌 **stateless** — 메인 서버는 세션 보관 X. 클라가 매 round 마다 `prev_choice_token` 송신해 컨텍스트 복원 (`identify/narrow` 와 동일 패턴).
> 📌 **RAG 영역 정책 준수** — Onboarding RAG ✅, 의료 안내(DUR) X ([아이템 v3 §6.3](아이템_ver3.md) 영역 한정 규칙).
> 📌 **표현 톤** — `efficacy_text`/`usage_text` 는 식약처 e약은요 본문 그대로 인용. 단정 표현 금지 ("이 약 드세요" X, "이 약 맞으세요?" ✓).

```
[폰 STT "타이레놀 등록할게"]
   ↓ 텍스트 (USB ADB)
[② 클라 PC] (round=1)
   ↓ POST /v1/pill/onboarding/normalize
[③ 메인 서버 10.10.10.97]
   ↓ NER + RAG
[⑤ LLM PC 10.10.10.120]  ← ChromaDB 벡터 검색 (식약처 낱알식별 + e약은요 임베딩)
   ↓ candidates[]
[③ 메인 서버] state 결정
   ├─ RESOLVED      → confirmation.tts_text
   ├─ NEED_DISAMBIG → question.tts_text + choice_token
   └─ NOT_FOUND     → fallback_action (RECAPTURE_OR_MANUAL / NARROW_DOWN)
   ↓ Response
[② 클라 PC]
   ↓ TTS (Qt6 QTextToSpeech, fallback 시 메인서버 TTS — §2.9)
[사용자: 음성/터치 응답]
   ↓ (NEED_DISAMBIG 인 경우 round=2 재호출)
   ↓ (RESOLVED + OK 인 경우)
[② 클라 PC] POST /v1/pill/pool { item_code, reg_method:"VOICE" }
   ↓
[③ 메인 서버] MariaDB INSERT user_medication_pool
```

### 2.9 ⭐ TTS 어댑터 정책 — 클라 우선 + 메인 서버 fallback

#### 비교

| 방식 | 데이터량 (안내 1회 5~15초) | 장점 | 단점 |
| --- | --- | --- | --- |
| **클라 자체 TTS (MVP 기본)** | 텍스트만 (수백 바이트) | 무료, 지연 0, 네트워크 부담 0, 운영 비용 0 | 음성 품질 OS·보이스 의존 (Microsoft Heami / Sun-Hi 한국어 적정) |
| 메인 서버 TTS (MP3 96kbps) | 60~180KB | 음성 품질 균일·자연 (Google TTS / Naver Clova Voice) | 네트워크 트래픽 ↑ (클라 TTS 대비 100~300배), 클라우드 비용·지연 |
| 메인 서버 TTS (WAV) | 600KB ~ 1.5MB | 무손실 | 사진 1장 수준 부담 |

#### 어댑터 인터페이스

```
ITtsProvider {
    + speak(text: string, language: "ko-KR") -> void  // 비동기, 재생 완료 콜백
    + cancel() -> void
    + is_available() -> bool
}

구현체:
  - QtSpeechProvider           (MVP 기본: Qt6 QTextToSpeech + Windows SAPI)
  - PhoneTtsProvider            (옵션: 폰 USB 채널로 위임, 폰 스피커 출력)
  - ServerTtsProvider           (fallback: 메인서버 → Google TTS / Clova 호출 → MP3 수신·재생)
```

#### Fallback 발동 조건

| 조건 | 동작 |
| --- | --- |
| `QtSpeechProvider.is_available() == false` (한국어 보이스 미설치) | → `ServerTtsProvider` 자동 전환 |
| 사용자 환경설정 "고품질 음성" 선택 | → `ServerTtsProvider` 사용 |
| 메인서버 TTS API 실패 | → `QtSpeechProvider` 로 재시도 (이중 fallback) |

> 📌 **MVP 단계**: `QtSpeechProvider` 만 구현·검증. 음질 평가 후 운용 단계에서 `ServerTtsProvider` 추가.
> 📌 **메인 서버 TTS 데이터량**은 사진 1장(1~3MB)보다 작아 절대량 부담은 X. 다만 **클라 TTS 대비 100~300배** 증가 + 클라우드 TTS API 비용·지연·네트워크 의존 ↑.

---

## 3. 미정의 / 추후 추가 흐름 (사용자 검토 진행 중)

다음 흐름들은 본 v2.x 에서 자리만 정의, 사용자 단계별 추가 예정:

- ✅ **§2.7 음성 LLM 보정 흐름** — v2.1 에서 추가됨
- ✅ **§2.6 Vision PC 학습 일괄 다운로드** — v2.1 에서 추가됨
- ✅ **§2.8 Onboarding RAG round-trip** — v2.2 에서 추가됨
- ✅ **§2.9 TTS 어댑터 정책** — v2.2 에서 추가됨
- 🔲 **③ 메인 서버 ↔ ⑥ Vision PC (이미지 추론 요청)** — Identify·Narrow 흐름 상세
- 🔲 **③ 메인 서버 ↔ 식약처 OpenAPI** — e약은요 lazy 캐싱
- 🔲 **클라 PC ↔ 메인 서버 (사진 외)** — 회원가입·로그인·복약이력·보고서 등 일반 API
- 🔲 **DUR 위험 안내 흐름** — 식별 후 메인서버의 DurChecker 호출
- 🔲 **데이터 보관 PC 보안 정책** — 단기 서명 토큰 (사전 발급) + IP allowlist

---

## 4. 데이터 흐름 시각화

### 4.1 사진 업로드 — 컨트롤 평면 + 데이터 평면 분리

```
[① 핸드폰 카메라]
   ↑↓ (USB ADB)
   │
[② 클라 PC]
   │ (1) 사진 업로드 의향 + 메타
   ├──────────────→ [③ 메인 서버]
   │                     │ (2) 여유? + 경로 질의
   │                     ├──────→ [④ 데이터 보관 PC]
   │                     │ (3) OK + 경로
   │                     ←──────┤
   │                     │ (4) MariaDB INSERT (photo_storage)
   │                     │     anonymous_id, 경로, 메타
   │ (5) OK + 경로 + 단기 토큰
   ←─────────────┤
   │ (6) PUT 사진 (직접, 토큰 검증)
   └──────────────────────────────────→ [④ 데이터 보관 PC]
```

### 4.2 추론 시 사진 접근

```
[⑤ 추론 PC]
   │ (1) request_id 의 사진 경로?
   ├──────────────→ [③ 메인 서버]
   │                     │ MariaDB SELECT (photo_storage)
   │ (2) 경로 응답
   ←─────────────┤
   │ (3) GET 사진 (직접)
   └──────────────────────────────────→ [④ 데이터 보관 PC]
   │ (4) YOLO·OCR
   ↓
[추론 결과 → 메인 서버 → 클라 PC]
```

---

## 5. 본 v2.x vs 기존 v2/v3 차이

| 항목 | 기존 v2/v3 | 본 v2.1 (제안) |
| --- | --- | --- |
| 디바이스 수 | 4대 + 폰 (메인+추론+학습+클라) | **5대 + 폰** (메인+데이터보관+**LLM**+**Vision**+클라) |
| 추론 / 학습 분리 | 추론 PC + 학습 서버 (분리 1+1) | **LLM PC (학습+추론 동거) + Vision PC (학습+추론 동거)**, 카테고리별 분리. 네트워크 분리 가능 설계 |
| 사진 저장 위치 | 메인 서버 `uploads/` | **데이터 보관 PC** (10.10.10.122, 사진 전용) |
| 음성 데이터 처리 | 명시 없음 | **음성 원본 폰 외부 비송신** + **STT 텍스트 DB 비저장**. 추출 구조화 결과만 보관 |
| 메인 서버 부담 | 비즈니스 + 파일 I/O + DB | 비즈니스 + DB(메타+가명+이력+텍스트 추출 결과) |
| 클라 → 메인 흐름 | 사진 직접 POST | 신호 → 경로/토큰 → 데이터 보관 PC 로 PUT |
| 추론 PC 사진 접근 | 메인서버 경유 | 메인서버에 경로만 질의 → 데이터 보관 PC 직접 GET (Vision PC) |
| 학습 데이터 수집 | 학습 서버 자체 보관 가정 | **학습 시점 Data Storage PC 일괄 PULL** (Vision PC) |
| **개인정보 식별 컬럼** | 4~5 테이블에 user_id 직접 FK | **pseudonym_map 1곳만** |

---

## 6. 장점·우려

### 장점
- **부하 분산**: 메인 서버는 메타·가명 매핑만
- **확장성**: 데이터 보관 PC 만 추가/교체로 저장 용량 확장
- **추론·학습 효율**: 메인 서버 거치지 않고 직접 데이터 접근
- **PIPA 가명처리 강함** (§8)

### 우려/검토 필요
- **PC 1대 추가**: 환경 구성 부담 ↑ (16일 일정 영향)
- **단일 장애점**: 데이터 보관 PC 다운 시 사진 접근 불가 (백업 정책 필요)
- **보안**: 데이터 보관 PC 직접 접근 통제 (메인서버 발급 단기 서명 토큰 권장)
- **메타-실제 일관성**: MariaDB 행과 실제 파일 동기화 (orphan 청소 배치)
- **임시 저장**: 메인-데이터보관 통신 실패 시 fallback (메인 임시 디스크?)

---

## 7. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v1.0 | 2026-05-07 | 팀 (3인) | 초안 — 사용자 제안 5대 + 폰 구조 정리 |
| v2.0 | 2026-05-07 | 팀 (3인) | **§8 가명화·익명화 정책** 신규 / **§2.5 데이터 보관 PC 메타 (photo_storage)** 정의 / DB ERD v3 → v4 동시 갱신 / 단기 서명 토큰 흐름 명시 / 본 v1·v2 변경 표 갱신 |
| v2.1 | 2026-05-07 | 팀 (3인) | **§1.2 IP·호스트 매트릭스** 신규(10.10.10.97 메인 / 10.10.10.122 데이터보관 / 10.10.10.120 LLM / 10.10.10.128 Vision) / **추론 PC 역할 재분배** — LLM(학습+추론 동거) + Vision(학습+추론 동거) 카테고리별 분리, 네트워크 분리 가능 설계 / **데이터 보관 PC = 사진 전용** 명시 (음성 원본 보관 X, 추출 구조화 데이터만 메인 MariaDB 보관 OK) / **§2.6 Vision 학습 일괄 다운로드 흐름** 신규 / **§2.7 음성 텍스트 추론 흐름** 신규 |
| v2.2 | 2026-05-07 | 팀 (3인) | **§2.8 Onboarding RAG round-trip 흐름** 신규 — 음성 등록 시 의료용어를 모르는 사용자를 위한 약명 정규화 (다회 round-trip 허용 + max_rounds·max_input_tokens·rate_limit 남용 방어). PillApi v0.3 `POST /v1/pill/onboarding/normalize` 와 1:1 매핑 / **§2.9 TTS 어댑터 정책** 신규 — 클라 자체 TTS 우선 (Qt6 QTextToSpeech + Windows SAPI), 메인 서버 TTS fallback 어댑터 (`ITtsProvider`) / §2.1 음성 안내 행 명확화 |

---

# 8. 가명화·익명화 정책 ⭐ 신규

## 8.1 설계 원칙 — "단일 끊기" 매핑

**목표**: 일정 기간 후 사진·이력·보고서 등 모든 데이터에서 사용자 식별 불가하도록 만들 수 있는 단일 SQL 끊기 절차 마련.

**원칙**: 모든 데이터 테이블에서 `user_id` FK 를 **제거**하고 **`anonymous_id`** 로 교체. user 와 anonymous 의 매핑은 **`pseudonym_map` 단일 테이블**에만 존재.

```
[ users (user_id, email, name, ...) ]
        │
        │ user_id (이 한 줄만 끊으면 익명화 완료)
        │
[ pseudonym_map (anonymous_id, user_id ⭐, severed_at) ]
        │
        │ anonymous_id (4곳 사용)
        ├─ user_medication_pool
        ├─ medication_intake_logs
        ├─ user_reports
        └─ photo_storage
```

## 8.2 익명화 절차 (운영)

1년 경과한 매핑 끊기 (단일 SQL):

```sql
UPDATE pseudonym_map
   SET user_id = NULL,
       severed_at = NOW()
 WHERE created_at < NOW() - INTERVAL 1 YEAR
   AND user_id IS NOT NULL;
```

→ 결과: 사진·이력·보고서 데이터는 그대로 보존. user 추적만 차단.

### 추가 익명화 (자유 텍스트 PII 제거)

매핑 끊기와 동시에 자유 텍스트 컬럼도 마스킹 (PII 우연 포함 가능):

```sql
-- 메모·노트의 PII 마스킹 (1년 경과한 행)
UPDATE medication_intake_logs il
   JOIN pseudonym_map pm ON il.anonymous_id = pm.anonymous_id
    SET il.memo = NULL
 WHERE pm.severed_at IS NOT NULL;

UPDATE user_reports r
   JOIN pseudonym_map pm ON r.anonymous_id = pm.anonymous_id
    SET r.report_notes = NULL
 WHERE pm.severed_at IS NOT NULL;
```

## 8.3 PII 컬럼 분류 (참고)

| 분류 | 컬럼 | 익명화 후 처리 |
| --- | --- | --- |
| 직접 식별자 (`users` 테이블) | email, password_hash, user_name | 사용자 탈퇴 시 행 삭제 또는 별도 정책 |
| 가명 식별자 | pseudonym_map.user_id | **본 가명화 정책의 핵심 끊기 대상** |
| 자유 텍스트 PII 가능 | medication_intake_logs.memo, user_reports.report_notes | 끊기 시 동시 NULL |
| 사진 파일명 | `<anonymous_id>_<photo_id>.jpg` | 자체로 비식별 (추가 처리 X) |
| 음성 STT **원본 텍스트** | 송신만, DB 저장 X (메모리에서만 처리) | 영구 비저장 |
| 음성 → **추출 구조화 데이터** (약명·개수·시각 등) | 정상 보관 | `medication_intake_logs` 등에 저장. PII 가능 자유 텍스트 컬럼은 끊기 시 NULL |
| 음성 바이너리 원본 | 폰 외부로 나가지 않음 | 영구 비저장 (Data Storage PC·메인 서버 모두 X) |

## 8.4 데이터 보관 PC 측 정책

| 항목 | 정책 |
| --- | --- |
| **파일명 규칙** | `<storage_path>/YYYY/MM/DD/<anonymous_id>_<photo_id>.jpg` |
| **user_id 사용 금지** | 파일명·디렉토리 구조에 user_id, email, name 등장 X |
| **삭제 정책** | 사용자 탈퇴 시: photo_storage 의 row 와 데이터 보관 PC 의 파일 모두 삭제 (계약 시점 결정) |
| **익명화 시점**: 매핑 끊기 후 | 파일은 그대로 보존, 메타테이블의 `anonymous_id` 는 살아있어 데이터 무결성 유지 |

## 8.5 PIPA·GDPR 준수 관점

| 법적 요구 | 본 정책 |
| --- | --- |
| **PIPA 가명처리(§28-2)** | pseudonym_map 분리 → 결합 제3자 제공 안 함 (§28-2 ②) |
| **PIPA 통계·연구·공익 목적 활용** | 익명화 후 데이터 활용 가능 (§58-2) |
| **GDPR Pseudonymization (Art. 4, 32)** | 식별자와 데이터 분리 → 명시적 준수 |
| **GDPR Right to Erasure (Art. 17)** | 사용자 탈퇴 요청 시: pseudonym_map.user_id NULL + (선택) 자유 텍스트 마스킹 |

## 8.6 운영 자동화

권장: **systemd timer 또는 cron** 으로 매일 1회 익명화 절차 실행.

```bash
# /etc/systemd/system/medibridge-anonymize.timer
[Timer]
OnCalendar=daily
Persistent=true

# /etc/systemd/system/medibridge-anonymize.service
[Service]
ExecStart=/usr/local/bin/medibridge_anonymize.sh
```

스크립트 위치: `MainServer/Scripts/AnonymizeExpired.sh` (신규 — 본 구현 시 작성).

---

## 9. 다음 단계 (사용자 검토 흐름)

사용자가 다음 흐름들을 단계별로 추가하면, 본 문서에 이어 붙이고 v2.1, v2.2 로 갱신 진행:

1. ⑤ 추론 PC ↔ ③ 메인 서버 (추론 요청)
2. ⑥ 학습 서버 ↔ ④ 데이터 보관 PC (학습 데이터 일괄)
3. 음성 LLM 보정 흐름
4. DUR 검출 흐름
5. 데이터 보관 PC 단기 서명 토큰 정책 상세

검토 완료 후 본 v2.x 의 내용을:
- 「시스템 흐름 정리본 v3」으로 통합
- 「기획서 v3」/「프로토콜 v3」/「DB ERD v4」 로 파급 갱신
