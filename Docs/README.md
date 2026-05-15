# MediBridge — Docs 인덱스

| 항목 | 내용 |
| --- | --- |
| **프로젝트** | 메디브릿지 (MediBridge) — 복약·건강 통합 케어 플랫폼 |
| **본 폴더** | `Docs/` — 프로젝트 설계·명세 문서 일체 |
| **변경 이력** | [CHANGELOG.md](CHANGELOG.md) |

---

## 1. 문서 위계 (정본 일원화)

각 주제는 **단일 정본 문서**가 있으며, 다른 문서는 정본을 참조한다 (DRY).

| 주제 | 정본 | 비고 |
| --- | --- | --- |
| **시장성·시나리오·핵심 설계 원칙** | [아이템_ver3.md](아이템_ver3.md) | 모듈 1 설계 사상 중심 |
| **프로젝트 개요·모듈 구조·포트폴리오 어필** | [기획서_ver3.md](기획서_ver3.md) | 6 모듈 표는 본 문서가 정본 |
| **시스템 흐름·MVP vs 확장 매트릭스·기술 스택** | [시스템 흐름 정리본_ver3.md](시스템%20흐름%20정리본_ver2.md) | 모듈별 상세 매트릭스 정본 |
| **PC·서버 연결 구조·가명화 정책·TTS 정책** | [시스템_연결구조_ver2.md](시스템_연결구조_ver2.md) (v2.2) | 5대 + 폰 + IP 확정 + LLM/Vision 분리 + Onboarding RAG round-trip + TTS 어댑터 |
| **기능 요구사항·표현 톤 정책** | [요구사항_분석서_ver2.md](요구사항_분석서_ver2.md) | 비기능 포함 |
| **DB 스키마** | [DB_ERD_ver4.md](DB_ERD_ver4.md) | `pseudonym_map` 단일-컷 매핑 도입 |
| **REST API 계약 (변경 불가)** | [Api/ApiOverview.md](Api/ApiOverview.md) | API 명세서 8종 인덱스 |
| **통신 프로토콜 골격** | [프로토콜_ver2.md](프로토콜_ver2.md) (v2.1) | API 명세서 우선, 본 문서는 골격 + §3.3 TTS 정책 |
| **개발 일정·역할 분담** | [개발계획서_ver2.md](개발계획서_ver2.md) | 16일 일정 |
| **보안 점검 체크리스트** | [SecurityChecklist.md](SecurityChecklist.md) | OWASP 기반 |
| **TestMode 설계서** | [TestMode.md](TestMode.md) | 추론·파일저장 우회 + DB seed 응답 (개발용) |
| **시스템 운영 명령어** | [system_prompt.md](system_prompt.md) | 메인·보관 PC 켜기/끄기/로그/포트 — 단계 가이드 |
| **설치 매뉴얼** | [Install/](Install/) | OS·기기별 설정. MainServer v0.3 + **DataStorage 신규** |
| **시연 테스트 결과** ⭐ | [SmokeTest_2026-05-15.md](SmokeTest_2026-05-15.md) | GUI 시연 결과 (통과·보류·실패·피드백 + 재시연 가이드) |
| **팀 회의록** ⭐ | [Meeting_2026-05-15.md](Meeting_2026-05-15.md) | 2026-05-15 — LLM·RAG 진입 결정 + 이미지 추론서버 진단 |
| **LLM 모델 후보 비교** ⭐ | [LLM_Model_Candidates.md](LLM_Model_Candidates.md) | **v2** — VRAM 16 GB 한도 + 한국어 임베딩 (KURE-v1) + LLM (Gemma 4 E4B 시작) + 외부 API fallback (gpt-4o-mini) + `LlmProvider` 추상화 |

---

## 2. API 명세서 (`Docs/Api/`)

| 파일 | 모듈 | 본 버전 |
| --- | --- | --- |
| [ApiOverview.md](Api/ApiOverview.md) | 인덱스 + 공통 규칙 (호스트 IP 확정, 추론 LLM/Vision 분리) | **v0.4** |
| [AuthApi.md](Api/AuthApi.md) | 모듈 6 — 인증 | v0.1 |
| [HistoryApi.md](Api/HistoryApi.md) | 모듈 2 — 복약 이력 | v0.1 |
| [ReportApi.md](Api/ReportApi.md) | 모듈 5 — 통합 보고서 (HTML/PDF) | **v0.2** |
| [MediaApi.md](Api/MediaApi.md) | 미디어 송수신 — intent/commit/get_token + 보관 PC 직접 PUT ⭐ | **v0.2** |
| [SpeechApi.md](Api/SpeechApi.md) | 음성 텍스트 (폰 STT) — UtteranceForwarder 경유 + dev 로깅 | **v0.2** |
| [PillApi.md](Api/PillApi.md) | 모듈 1 — 식별·DUR + 약 풀 + 단계별 좁히기 + Onboarding 정규화 ⭐ | **v0.3** |
| [MonitoringApi.md](Api/MonitoringApi.md) | 자원 모니터링 | v0.1 |

---

## 3. 폴더 구조

```
Docs/
├── README.md                       # 본 파일 (인덱스)
├── CHANGELOG.md                    # 코드·문서 변경 이력
├── system_prompt.md                # ⭐ 시스템 운영 명령어 정리
├── 기획서_ver3.md
├── 아이템_ver3.md
├── 시스템 흐름 정리본_ver3.md
├── 시스템_연결구조_ver2.md
├── 요구사항_분석서_ver2.md
├── DB_ERD_ver4.md
├── 프로토콜_ver2.md
├── 개발계획서_ver2.md
├── SecurityChecklist.md
├── TestMode.md
├── SmokeTest_2026-05-15.md           # ⭐ GUI 시연 결과
├── Meeting_2026-05-15.md             # ⭐ 팀 회의록
├── LLM_Model_Candidates.md           # ⭐ LLM 모델 비교 (Gemma 4 등)
├── 메디브릿지 목업.pptx
├── Architecture/
│   └── MediBridge_Architecture_MVP.pptx   # ⭐ 5대 + 폰 다이어그램 (10 슬라이드)
├── Api/                            # REST API 명세서 8종
│   ├── ApiOverview.md              # v0.4
│   ├── AuthApi.md
│   ├── HistoryApi.md
│   ├── ReportApi.md                # v0.2 — HTML/PDF
│   ├── MediaApi.md                 # v0.2 — intent/commit/get_token
│   ├── SpeechApi.md
│   ├── PillApi.md                  # v0.3
│   └── MonitoringApi.md
├── Install/                        # 설치 매뉴얼
│   ├── InstallIndex.md
│   ├── MainServerInstall.md        # Drogon + TestMode + Storage 시크릿
│   ├── DataStorageInstall.md       # ⭐ 신규 — 보관 PC 미니 서버
│   ├── ClientPcInstall.md
│   ├── AndroidPhoneSetup.md
│   ├── InferenceServerInstall.md
│   └── TrainingServerInstall.md
└── Old/                            # 이전 버전 보관 (날짜 suffix)
    ├── 기획서_ver1_2026-05-06.md
    ├── 기획서_ver2_2026-05-07.md
    ├── 아이템_ver1_2026-05-06.md
    ├── 아이템_ver2_2026-05-07.md
    ├── 시스템 흐름 정리본_ver1_2026-05-06.md
    ├── 시스템_연결구조_ver1_2026-05-07.md
    ├── 요구사항_분석서_ver1_2026-05-06.md
    ├── DB_ERD_ver1_2026-05-06.md
    ├── DB_ERD_ver2_2026-05-07.md
    ├── DB_ERD_ver3_2026-05-07.md
    ├── 개발계획서_ver1_2026-05-06.md
    ├── 프로토콜_ver1_2026-05-06.md
    └── Api/                        # API 이전 버전
```

---

## 4. 신규 문서 작성 규칙

### 4.1 문서 갱신 시
1. 기존 문서를 `Docs/Old/<파일명>_ver<N>_<YYYY-MM-DD>.md` 로 이동
2. 새 버전 작성 (버전 번호 +1, 「이전 버전」 링크 포함)
3. 「변경 이력」 절에 사유 한 줄 추가
4. **[CHANGELOG.md](CHANGELOG.md) 에 항목 추가** (Added / Changed / Moved + 사유)
5. 본 README 의 정본 표·폴더 구조 갱신

### 4.2 신규 문서 작성 시
1. 기존 문서와 **주제 중복 없는지** 확인 (정본 표 1번 참조)
2. 중복이라면 **기존 문서 갱신** 으로 처리
3. 분리 가능하다면 신규 작성 + 본 README 정본 표 추가

### 4.3 표기 컨벤션
- 한국어 우선, 영문 약어는 한국어 옆 괄호 (예: 가명화(pseudonymization))
- 외부 출처는 [링크 텍스트](URL) 형태 + 발행일 명시
- 변경 핵심은 문서 상단 「⭐ vN.N 변경 핵심」 박스에 요약
- 본 프로젝트 범위 외 항목은 「본 프로젝트 범위 외」 절에 분리 기재

---

## 5. 빠른 참조

### 5.0 메인 서버 + 보관 PC 시연 환경 띄우기 (TestMode)

PC 부팅 후 **WSL Bash 두 줄**:
```bash
bash MainServer/Scripts/medibridge-up.sh              # 메인서버 :8001
bash DataStorageServer/Scripts/datastorage-up.sh      # 보관 PC  :8004
```

**Windows 관리자 PS** (LAN 노출 — 최초 1회만):
```powershell
MainServer\Scripts\medibridge-portproxy.ps1            # 8001 + 8004 한 번에 등록
```

확인:
- `http://10.10.10.97:8001/health` (메인서버)
- `http://10.10.10.97:8004/health` (보관 PC)
- 시드 계정: `test@medibridge.local` / `test1234`

전체 운영 명령은 [system_prompt.md](system_prompt.md) 정본.
상세 셋업은 [Install/MainServerInstall.md](Install/MainServerInstall.md) + [Install/DataStorageInstall.md](Install/DataStorageInstall.md) / [TestMode.md](TestMode.md).

### 5.1 약 식별 흐름 한눈에
- 사전 등록 (음성·직접 입력) → 식별 범위 5,000종 → N종 축소
- Stage 0.5 (음성 의도 분류) → Stage 1 (검출·OCR·매칭) → Stage 2 (DUR 위험 검출)
- LLM·RAG **의료 안내 영역 미사용** (식약처 DUR 정해진 템플릿)

### 5.2 시스템 구성 (확정)
**5대 + 폰** — 모두 LAN `10.10.10.0/24`:
- GUI 클라이언트 PC (로컬, Windows 10/11)
- 메인서버 PC `10.10.10.97` (Ubuntu 24.04 + MariaDB)
- 데이터 보관 PC `10.10.10.122` (Ubuntu 24.04, **사진 전용**)
- LLM 학습+추론 PC `10.10.10.128` (Ubuntu 24.04 + GPU)
- Vision 학습+추론 PC `10.10.10.120` (Ubuntu 24.04 + GPU)
- 안드로이드 S24 폰 (USB 직결)

> 📌 학습+추론 동거 (여유 PC 부족) — 네트워크 분리 가능 설계로 향후 PC 증설 시 즉시 분리.
> 📌 음성 원본은 폰 외부 비송신 — STT 텍스트만 흐름, 추출 구조화 데이터만 메인 MariaDB 보관.

### 5.3 표현 톤 정책 (필수)
- ❌ "복용 가능합니다 / 불가합니다"
- ✅ "추정" + "약사·의사 상담 권유"
- ✅ DUR 위험 안내 = 식약처 데이터 그대로 인용 (LLM 자연어 변환 금지)
- 🆕 클라이언트 TtsAdapter 가 단정 표현 binary-level 차단 (`복용 가능 / 불가 / 안전합니다 / 위험합니다 / 진단합니다`)

### 5.4 클라이언트 신규 기능 (2026-05-15)
- 🔊 **TTS 어댑터** — Windows SAPI ko-KR Heami 음성, QSettings 영속 ON/OFF Switch
- 🔐 **자동 로그인** — 로그인 → 종료 → 재실행 시 HomePage 직행. 토큰 만료 시 자동 LoginPage 복귀
- 🎤 **음성 안내 가이드** — 능동 가이드 (환영 멘트) + 대화형 가이드 (신뢰도별 / DUR risk)
- 📝 **복용 기록 다이얼로그 개편** — 약 선택 3소스 + 풀 미등록 약 분기 (등록+기록 / 기록만)
- 📱 **폰 측 8000 점유 시** `MEDIBRIDGE_PHONE_PORT=18000` 환경변수로 오버라이드

---

## 6. 연락 / 협업

- 팀: 김윤식(팀장), 심동주, 인효 (3인)
- 담당 교수: 이동녘
- 일정: 2026-04-27 ~ 2026-05-12 (16일)
