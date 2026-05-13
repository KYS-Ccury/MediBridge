# 메디브릿지 설치 매뉴얼 인덱스

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 설치 매뉴얼 인덱스 (전체 환경 한눈에) |
| **버전** | v1.1 |
| **개정일** | 2026-05-13 |
| **이전 버전** | v1.0 (2026-05-06) |
| **작성자** | 팀 (3인) |

> 본 폴더(`Docs/Install/`)는 메디브릿지 프로젝트의 **각 PC/디바이스별 설치 절차를 매뉴얼화**하여 관리한다. 환경 재구축, 신규 팀원 온보딩, 트러블슈팅 시 1차 참고 문서.

---

## 1. 환경 구성 한눈에

| # | 디바이스 / 서버 | OS | 역할 | 매뉴얼 |
| --- | --- | --- | --- | --- |
| ① | **클라이언트 PC (GUI)** | Windows 10/11 | Qt6 + C++ + QML 기반 GUI, 폰 입력 수신, 운용 서버 통신 | [ClientPcInstall.md](ClientPcInstall.md) ✅ |
| ② | **메인 서버 PC** | Ubuntu 24.04 | **Drogon C++** + MariaDB, JWT, 식약처 데이터 적재, 청소 잡 | [MainServerInstall.md](MainServerInstall.md) ✅ |
| ③ | **데이터 보관 PC** ⭐ | Ubuntu 24.04 | **Drogon 미니 서버 (port 8004)**, 사진 PUT/GET + HMAC 토큰 검증 | [DataStorageInstall.md](DataStorageInstall.md) ✅ |
| ④ | **추론 서버 PC (LLM)** | Ubuntu 24.04 (GPU) | LLM·RAG·ChromaDB·Stage 0.5 의도 분류 (10.10.10.120 : 8002) | [InferenceServerInstall.md](InferenceServerInstall.md) ⏳ |
| ⑤ | **추론 서버 PC (Vision)** | Ubuntu 24.04 (GPU) | YOLO·PaddleOCR·OpenCV (10.10.10.128 : 8003) | [InferenceServerInstall.md](InferenceServerInstall.md) ⏳ |
| ⑥ | **학습 서버** | Ubuntu 24.04 (GPU) | 데이터 수집·모델 학습·검증·배포 (PC 부족 시 ④⑤ 와 동거) | [TrainingServerInstall.md](TrainingServerInstall.md) ⏳ |
| (입력) | **안드로이드 S24 폰** | Android 14+ | 카메라 + 마이크 + 온디바이스 STT (Galaxy AI / `SpeechRecognizer`) | [AndroidPhoneSetup.md](AndroidPhoneSetup.md) ✅ |

✅ = 작성 완료 / ⏳ = 추후 작성 (해당 서버 셋업 진입 시 채움)

> ⭐ **시스템 시연 시작 명령** 은 [system_prompt.md](../system_prompt.md) 정본 참조.

---

## 2. 통신 구간 요약

```
[ S24 폰 ] ─ USB ─→ [ 클라 PC ]
                       │   REST 8001 (JWT)
                       ▼
                  [ 메인 서버 PC :8001 ] ──── REST 8002 ── [ LLM PC ]
                       │  │  │                            (10.10.10.120)
                       │  │  └── REST 8003 ─── [ Vision PC ]
                       │  │                    (10.10.10.128)
                       │  │                          │
                       │  └── HTTPS 443 ─→ [ 식약처 OpenAPI ]
                       │
                       │ ① /v1/media/intent (토큰 발급)
                       │ ③ /v1/media/commit (READY 마킹)
                       │
              ┌────────┘
              │  ② PUT (메인 우회, put_token 검증)
              │  ⑥ Vision PC GET (get_token 검증)
              ▼
       [ 데이터 보관 PC :8004 ] (10.10.10.122)
       <storage_root>/<anon>/<photo_id>.<ext>
```

→ 사진 본체는 메인서버를 통과하지 않음 (컨트롤 평면 ↔ 데이터 평면 분리, AWS S3+RDS 패턴).

---

## 3. 매뉴얼 작성 규칙

본 폴더의 매뉴얼은 다음 규칙으로 통일 작성한다.

- **각 파일 상단**: 메타 정보 박스(버전·작성일·OS·역할) + 변경 이력
- **명령어**: 코드 블록 + 한국어 주석 (각 명령이 무엇을 하는지 설명)
- **트러블슈팅**: 자주 마주칠 에러와 해결 방법을 별도 절로 정리
- **체크리스트**: 매뉴얼 마지막에 셋업 완료 여부를 점검할 수 있는 항목 리스트
- **명명 규칙**: 파일명은 PascalCase (`ClientPcInstall.md`), 폴더 경로는 한글·공백·특수문자 회피

---

## 4. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v1.0 | 2026-05-06 | 팀 (3인) | 초안 작성. 클라 PC + S24 폰 매뉴얼 본문 작성 완료, 서버군 3종 스켈레톤 작성 |
| **v1.1** | **2026-05-13** | 팀 (3인) | **데이터 보관 PC 추가** — Drogon 미니 서버 port 8004, [DataStorageInstall.md](DataStorageInstall.md) 신규. 메인 서버 매뉴얼 작성 완료(✅). 통신 구간도에 사진 흐름 ⑤+⑥ (메인 우회 PUT/GET) 반영. LLM/Vision 추론 서버 2대 분리. 시스템 시작 명령은 [system_prompt.md](../system_prompt.md) 정본으로 분리. |
