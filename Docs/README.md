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
| **시스템 흐름·MVP vs 확장 매트릭스·기술 스택** | [시스템 흐름 정리본_ver2.md](시스템%20흐름%20정리본_ver2.md) | 모듈별 상세 매트릭스 정본 |
| **PC·서버 연결 구조·가명화 정책·TTS 정책** | [시스템_연결구조_ver2.md](시스템_연결구조_ver2.md) (v2.2) | 5대 + 폰 + IP 확정 + LLM/Vision 분리 + Onboarding RAG round-trip + TTS 어댑터 |
| **기능 요구사항·표현 톤 정책** | [요구사항_분석서_ver2.md](요구사항_분석서_ver2.md) | 비기능 포함 |
| **DB 스키마** | [DB_ERD_ver4.md](DB_ERD_ver4.md) | `pseudonym_map` 단일-컷 매핑 도입 |
| **REST API 계약 (변경 불가)** | [Api/ApiOverview.md](Api/ApiOverview.md) | API 명세서 8종 인덱스 |
| **통신 프로토콜 골격** | [프로토콜_ver2.md](프로토콜_ver2.md) (v2.1) | API 명세서 우선, 본 문서는 골격 + §3.3 TTS 정책 |
| **개발 일정·역할 분담** | [개발계획서_ver2.md](개발계획서_ver2.md) | 16일 일정 |
| **보안 점검 체크리스트** | [SecurityChecklist.md](SecurityChecklist.md) | OWASP 기반 |
| **TestMode 설계서** | [TestMode.md](TestMode.md) | 추론·파일저장 우회 + DB seed 응답 (개발용) |
| **설치 매뉴얼** | [Install/](Install/) | OS·기기별 설정. MainServerInstall v0.2 (Drogon + TestMode) |

---

## 2. API 명세서 (`Docs/Api/`)

| 파일 | 모듈 | 본 버전 |
| --- | --- | --- |
| [ApiOverview.md](Api/ApiOverview.md) | 인덱스 + 공통 규칙 (호스트 IP 확정, 추론 LLM/Vision 분리) | **v0.3** |
| [AuthApi.md](Api/AuthApi.md) | 모듈 6 — 인증 | v0.1 |
| [HistoryApi.md](Api/HistoryApi.md) | 모듈 2 — 복약 이력 | v0.1 |
| [ReportApi.md](Api/ReportApi.md) | 모듈 5 — 통합 보고서 | v0.1 |
| [MediaApi.md](Api/MediaApi.md) | 미디어 송수신 | v0.1 |
| [SpeechApi.md](Api/SpeechApi.md) | 음성 텍스트 (폰 STT) | v0.1 |
| [PillApi.md](Api/PillApi.md) | 모듈 1 — 식별·DUR + 약 풀 + 단계별 좁히기 + Onboarding 정규화 ⭐ | **v0.3** |
| [MonitoringApi.md](Api/MonitoringApi.md) | 자원 모니터링 | v0.1 |

---

## 3. 폴더 구조

```
Docs/
├── README.md                       # 본 파일 (인덱스)
├── CHANGELOG.md                    # 코드·문서 변경 이력
├── 기획서_ver3.md
├── 아이템_ver3.md
├── 시스템 흐름 정리본_ver2.md
├── 시스템_연결구조_ver2.md         # ⚠️ 검토 중
├── 요구사항_분석서_ver2.md
├── DB_ERD_ver4.md
├── 프로토콜_ver2.md
├── 개발계획서_ver2.md
├── SecurityChecklist.md
├── 메디브릿지 목업.pptx
├── Api/                            # REST API 명세서 8종
│   ├── ApiOverview.md
│   ├── AuthApi.md
│   ├── HistoryApi.md
│   ├── ReportApi.md
│   ├── MediaApi.md
│   ├── SpeechApi.md
│   ├── PillApi.md
│   └── MonitoringApi.md
├── Install/                        # 설치 매뉴얼
│   ├── ClientPcInstall.md
│   ├── AndroidPhoneSetup.md
│   └── ...
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

### 5.0 메인 서버 시연 환경 띄우기 (TestMode)

PC 부팅 후 두 명령:
- **WSL/Ubuntu**: `bash MainServer/Scripts/medibridge-up.sh`
- **Windows 관리자 PS** (WSL 노출 시): `MainServer\Scripts\medibridge-portproxy.ps1`
- 확인: `http://10.10.10.97:8001/health`
- 시드 계정: `test@medibridge.local` / `test1234`

상세는 [MainServer/README.md "빠른 시작"](../MainServer/README.md) / [Install/MainServerInstall.md](Install/MainServerInstall.md) / [TestMode.md](TestMode.md).

### 5.1 약 식별 흐름 한눈에
- 사전 등록 (음성·직접 입력) → 식별 범위 5,000종 → N종 축소
- Stage 0.5 (음성 의도 분류) → Stage 1 (검출·OCR·매칭) → Stage 2 (DUR 위험 검출)
- LLM·RAG **의료 안내 영역 미사용** (식약처 DUR 정해진 템플릿)

### 5.2 시스템 구성 (확정)
**5대 + 폰** — 모두 LAN `10.10.10.0/24`:
- GUI 클라이언트 PC (로컬, Windows 10/11)
- 메인서버 PC `10.10.10.97` (Ubuntu 24.04 + MariaDB)
- 데이터 보관 PC `10.10.10.122` (Ubuntu 24.04, **사진 전용**)
- LLM 학습+추론 PC `10.10.10.120` (Ubuntu 24.04 + GPU)
- Vision 학습+추론 PC `10.10.10.128` (Ubuntu 24.04 + GPU)
- 안드로이드 S24 폰 (USB 직결)

> 📌 학습+추론 동거 (여유 PC 부족) — 네트워크 분리 가능 설계로 향후 PC 증설 시 즉시 분리.
> 📌 음성 원본은 폰 외부 비송신 — STT 텍스트만 흐름, 추출 구조화 데이터만 메인 MariaDB 보관.

### 5.3 표현 톤 정책 (필수)
- ❌ "복용 가능합니다 / 불가합니다"
- ✅ "추정" + "약사·의사 상담 권유"
- ✅ DUR 위험 안내 = 식약처 데이터 그대로 인용 (LLM 자연어 변환 금지)

---

## 6. 연락 / 협업

- 팀: 김윤식(팀장), 심동주, 인효 (3인)
- 담당 교수: 이동녘
- 일정: 2026-04-27 ~ 2026-05-12 (16일)
