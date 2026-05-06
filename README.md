# 메디브릿지 (MediBridge)

> 환자와 의료진을 잇는 다리 — 알약 식별·복약 이력·증상 기록·헬스케어 데이터·통합 보고서로 구성되는 **복약·건강 통합 케어 플랫폼**.
>
> 본 저장소는 메디브릿지의 **모듈 1(알약 식별 + 안전 점검) + 모듈 2(복약 이력) + 모듈 5(통합 보고서) + 모듈 6(인증)** 을 MVP로 구현한다.

| 항목 | 내용 |
| --- | --- |
| **프로젝트명** | AI 머신비전 활용 프로젝트 — 메디브릿지 |
| **팀 구성** | 김윤식(팀장), 심동주, 인효 (3인 팀) |
| **담당 교수** | 이동녘 교수 |
| **개발 기간** | 2026-04-27(월) ~ 2026-05-12(화) |

---

## 폴더 구조

```
MediBridge/
├─ Docs/                  ← 모든 설계 문서 (v2 기준)
│  ├─ Api/                ← REST API 명세서 (분담의 기준)
│  ├─ Install/            ← PC/디바이스별 설치 매뉴얼
│  └─ Old/                ← 이전 버전 백업
├─ Client/                ← 영역 C: Qt6 + C++ + QML 클라이언트 (Windows)
├─ MainServer/            ← 영역 B: Drogon (C++) 메인 서버 (Ubuntu 24.04)
├─ InferenceServer/       ← 영역 A: FastAPI (Python) 추론 서버 (Ubuntu 24.04, GPU)
├─ TrainingServer/        ← 영역 A: 학습 서버 (Ubuntu 24.04, GPU)
└─ Tests/                 ← 통합 테스트
```

---

## 개발 환경

| 디바이스 / 서버 | OS | 역할 |
| --- | --- | --- |
| 클라이언트 PC | Windows 10/11 | Qt6 + C++ + QML GUI, 폰 입력 수신, 메인서버 통신 |
| 메인 서버 PC | Ubuntu 24.04 | Drogon 백엔드, MariaDB, 식약처 데이터 |
| 추론 서버 PC | Ubuntu 24.04 (GPU) | YOLO26·PaddleOCR·OpenCV·LLM |
| 학습 서버 | Ubuntu 24.04 (GPU) | 데이터 수집·모델 학습 |
| 입력 디바이스 | 안드로이드 S24 | 카메라 + 마이크 + 온디바이스 STT (Galaxy AI) |

설치 절차는 [Docs/Install/InstallIndex.md](Docs/Install/InstallIndex.md) 참조.

---

## 핵심 설계 원칙

- **사전 등록(Onboarding)** 으로 식별 범위 5,000종 → 사용자 N종 축소
- **LLM·RAG** 는 등록·의도 분류·비의료 일반 안내에만 사용. **의료 안내(DUR 위험 안내)에는 미사용** — 식약처 DUR을 정해진 템플릿으로만 출력
- **단정 문구 금지** — 모든 결과 "추정" + "약사·의사 상담 권유" 톤
- **외부 API 의존 최소화** — 식약처 데이터를 MariaDB에 일괄 적재 + e약은요는 lazy 캐싱
- **폰 온디바이스 STT** — 음성 바이너리가 서버에 흐르지 않아 대역폭 절감 + 개인정보 보호

---

## 영역 분담

| 영역 | 1차 책임 폴더 | 주요 책임 |
| --- | --- | --- |
| **영역 A** (AI 추론) | `InferenceServer/`, `TrainingServer/` | YOLO26·PaddleOCR·OpenCV·LLM 의도 분류기·등록 RAG |
| **영역 B** (서버/DB/통신) | `MainServer/` | Drogon 백엔드, MariaDB, DUR 템플릿, 인증, 복약 이력, 보고서 |
| **영역 C** (GUI/클라) | `Client/` | Qt6+C+++QML, 폰 입력 수신, 결과 표시, 회원가입·로그인 화면 |

> 1:1 매핑이 아닌 **1차 책임자**만 두고 작업은 협업 중심.
> 영역 간 인터페이스(REST API)는 [Docs/Api/](Docs/Api/) 의 명세서로 합의.

---

## 빠른 시작

1. 본 저장소 clone
2. [Docs/Install/InstallIndex.md](Docs/Install/InstallIndex.md) 에서 자기 환경(클라/서버/폰)에 맞는 매뉴얼 따라 셋업
3. 자기 영역 폴더(`Client/` 또는 `MainServer/` 등) 안의 `README.md` 참조하여 개발 시작

---

## 주요 문서

| 문서 | 내용 |
| --- | --- |
| [Docs/기획서_ver2.md](Docs/기획서_ver2.md) | 프로젝트 기획·시스템 요구사항 |
| [Docs/시스템 흐름 정리본_ver2.md](Docs/시스템%20흐름%20정리본_ver2.md) | 전체 시스템 흐름·PC 4대 구성 |
| [Docs/요구사항_분석서_ver2.md](Docs/요구사항_분석서_ver2.md) | 기능·비기능 요구사항 (FR-A~C) |
| [Docs/프로토콜_ver2.md](Docs/프로토콜_ver2.md) | 통신 프로토콜·REST API 개요 |
| [Docs/DB_ERD_ver2.md](Docs/DB_ERD_ver2.md) | MariaDB 스키마 |
| [Docs/개발계획서_ver2.md](Docs/개발계획서_ver2.md) | Phase별 개발 계획·리스크 |
| [Docs/아이템_ver2.md](Docs/아이템_ver2.md) | 시장성·활용 시나리오·기술 스택 |

---

## 변경 이력

| 일자 | 내용 |
| --- | --- |
| 2026-05-06 | 프로젝트 초기 구조 확정 — Drogon(C++) 메인서버 + Qt6 클라 + Python 추론서버 + 폰 온디바이스 STT |
