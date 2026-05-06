# 메디브릿지 설치 매뉴얼 인덱스

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 설치 매뉴얼 인덱스 (전체 환경 한눈에) |
| **버전** | v1.0 |
| **작성일** | 2026-05-06 |
| **작성자** | 팀 (3인) |

> 본 폴더(`Docs/Install/`)는 메디브릿지 프로젝트의 **각 PC/디바이스별 설치 절차를 매뉴얼화**하여 관리한다. 환경 재구축, 신규 팀원 온보딩, 트러블슈팅 시 1차 참고 문서.

---

## 1. 환경 구성 한눈에

| # | 디바이스 / 서버 | OS | 역할 | 매뉴얼 |
| --- | --- | --- | --- | --- |
| ① | **클라이언트 PC (GUI)** | Windows 10/11 | Qt6 + C++ + QML 기반 GUI, 폰 입력 수신, 운용 서버 통신 | [ClientPcInstall.md](ClientPcInstall.md) ✅ |
| ② | **메인(운용) 서버 PC** | Ubuntu 24.04 | FastAPI 백엔드, MariaDB, 식약처 데이터 적재 | [MainServerInstall.md](MainServerInstall.md) ⏳ |
| ③ | **추론 서버 PC** | Ubuntu 24.04 (GPU) | YOLO26·PaddleOCR·OpenCV·LLM 추론, Whisper STT(확장 fallback) | [InferenceServerInstall.md](InferenceServerInstall.md) ⏳ |
| ④ | **학습 서버** | Ubuntu 24.04 (GPU) | 데이터 수집·모델 학습·검증·배포 | [TrainingServerInstall.md](TrainingServerInstall.md) ⏳ |
| (입력) | **안드로이드 S24 폰** | Android 14+ | 카메라 + 마이크 + 온디바이스 STT (Galaxy AI / `SpeechRecognizer`) | [AndroidPhoneSetup.md](AndroidPhoneSetup.md) ✅ |

✅ = 작성 완료 / ⏳ = 추후 작성 (해당 서버 셋업 진입 시 채움)

---

## 2. 통신 구간 요약

```
[ S24 폰 ] ─ USB or WiFi ─→ [ 클라 PC (Win10/11) ]
                                      │ TCP/IP (REST API on HTTP)
                                      │ + 미디어/STT 텍스트 별도 엔드포인트
                                      ▼
                            [ 메인 서버 (Ubuntu 24.04) ]
                                      │ TCP/IP (REST API on HTTP)
                                      ▼
                            [ 추론 서버 (Ubuntu 24.04 GPU) ]
                                      ▲ SCP / 파일 전송 (모델 배포 시에만)
                                      │
                            [ 학습 서버 (Ubuntu 24.04 GPU) ]

외부: 운용 서버 ↔ 식약처 OpenAPI (HTTPS, e약은요 캐시 미스 시에만)
```

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
