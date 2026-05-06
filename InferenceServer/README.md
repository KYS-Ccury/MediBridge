# InferenceServer — 영역 A (Python FastAPI, Ubuntu 24.04 + GPU)

> 메디브릿지 AI 추론 서버. YOLO26·PaddleOCR·OpenCV·LLM 호스팅. Whisper STT는 확장 fallback.

---

## 빠른 시작

1. [Docs/Install/InferenceServerInstall.md](../Docs/Install/InferenceServerInstall.md) 의 NVIDIA 드라이버·CUDA·cuDNN 셋업
2. Python 3.12 가상환경:
   ```bash
   python3 -m venv ~/medibridge-inference-venv
   source ~/medibridge-inference-venv/bin/activate
   pip install -r requirements.txt
   pip install torch torchvision --index-url https://download.pytorch.org/whl/cu121
   ```
3. 학습 서버에서 SCP로 모델 가중치 받아 `Models/` 에 배치
4. 실행:
   ```bash
   python Main.py
   ```
5. 기본 포트: **8002**, 베이스 URL: `http://<host>:8002/v1`

---

## 폴더 구조

| 폴더 | 책임 |
| --- | --- |
| `Routers/` | FastAPI 엔드포인트 (얇음) |
| `Schemas/` | Pydantic 모델 |
| `Vision/` | YOLO26 검출 + PaddleOCR + OpenCV |
| `Llm/` | 의도 분류기(Stage 0.5) + 등록·일반 안내 보조 |
| `Speech/` | (확장 fallback) Whisper STT |
| `Threading/` | concurrent.futures 워커 풀 |
| `Monitoring/` | /health, /metrics |
| `Models/` | 모델 가중치 (.pt 등 — `.gitignore`로 제외) |

---

## TODO 마커별 분담 가능 작업

| 영역 | 파일 | 작업 |
| --- | --- | --- |
| Vision | `Vision/YoloDetector.py` | Ultralytics YOLO 통합, GPU 추론 |
| Vision | `Vision/OcrEngine.py` | PaddleOCR + 알약 fine-tuning 가중치 사용 |
| Vision | `Vision/ColorShape.py` | OpenCV HSV/윤곽선 분석 |
| LLM | `Llm/IntentClassifier.py` | LLM 모델 선정·통합, JSON 스키마 강제 검증, 인젝션 패턴 확장 |
| LLM | `Llm/OnboardingHelper.py` | RAG 인덱스 구축 (식약처 데이터 임베딩), 약명 정규화·분기 질문·요약 |
| Speech | `Speech/WhisperFallback.py` | (확장 단계) faster-whisper 통합 |
| Monitoring | `Monitoring/ResourceMonitor.py` | pynvml GPU 메트릭 수집 |
| Routers | 6개 라우터 모두 본체 구현 | Vision/Intent/Onboarding/Summary/Speech/Monitoring |

---

## API 엔드포인트

| 라우터 | 엔드포인트 | MVP/확장 |
| --- | --- | --- |
| Vision | POST /vision/detect, /vision/analyze | MVP |
| Intent | POST /intent/classify | MVP |
| Onboarding | POST /onboarding/normalize, /disambiguate | MVP |
| Summary | POST /summary/non-medical | MVP |
| Speech | POST /speech/stt | 확장 fallback |
| Monitoring | GET /health, /metrics | MVP |

---

## 핵심 정책 (절대 준수)

1. **Stage 0.5 의도 분류기는 분류만** — JSON 스키마 강제 (`category` + `confidence` + `injection_flag`)
2. **DUR 위험 안내 영역에 LLM 호출 코드 자체가 부재** — Onboarding/Summary 라우터만 LLM 사용
3. **인젝션 패턴 1차 필터** — 정규식 기반 즉시 OTHER 반환
4. **Summary/non-medical 입력 검증** — 의료 안내 키워드 차단
5. **출력 검증** — 단정 표현 reject ("복용 가능합니다" 등)

---

## 외부 의존성

| 라이브러리 | 용도 |
| --- | --- |
| FastAPI + Uvicorn | HTTP 프레임워크 |
| Pydantic | 검증·직렬화 |
| PyTorch (CUDA) | YOLO26 / Whisper |
| Ultralytics | YOLO26 추론 |
| PaddleOCR (paddlepaddle-gpu) | 각인 인식 |
| OpenCV | 색·모양 분석 |
| psutil + pynvml | 자원 측정 |
| loguru | 로깅 |

---

## 관련 문서

- [Docs/요구사항_분석서_ver2.md](../Docs/요구사항_분석서_ver2.md) §4 영역 A
- [Docs/Api/](../Docs/Api/) Vision/Intent/Onboarding/Summary 등
- [Docs/Install/InferenceServerInstall.md](../Docs/Install/InferenceServerInstall.md) 환경 셋업
