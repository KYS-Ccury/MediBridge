# 추론 서버 설치 매뉴얼 (Ubuntu 24.04, GPU)

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 추론 서버 PC 설치 매뉴얼 |
| **대상 OS** | Ubuntu 24.04 LTS |
| **하드웨어** | NVIDIA GPU 권장 (CUDA 호환) |
| **역할** | YOLO26 알약 검출, PaddleOCR 각인 인식, OpenCV 색·모양 분석, **(확장 fallback) Whisper STT**, LLM 의도 분류기·등록 LLM·RAG |
| **버전** | v0.1 (스켈레톤) |
| **작성일** | 2026-05-06 |
| **작성자** | 팀 (3인) |

> ⏳ **본 매뉴얼은 스켈레톤 상태.** 추론 서버 셋업 진입 시 각 절을 채워나간다.
>
> 본 프로젝트 MVP에서는 **음성 STT를 폰의 온디바이스 STT(Galaxy AI)로 처리**하므로 추론 서버에 Whisper는 **확장 fallback** 용도로만 설치한다 (선택).

---

## 1. 설치 예정 항목 (체크리스트)

| # | 도구 / 컴포넌트 | 용도 | 상태 |
| --- | --- | --- | --- |
| 1 | NVIDIA Driver (570 등) | GPU 사용 기반 | ⏳ |
| 2 | CUDA Toolkit (12.x) + cuDNN | GPU 가속 | ⏳ |
| 3 | Python 3.12 + venv | 추론 환경 | ⏳ |
| 4 | PyTorch (CUDA 빌드) | YOLO26 / Whisper / LLM 추론 | ⏳ |
| 5 | **YOLO26** (Ultralytics 또는 해당 패키지) | 알약 검출 | ⏳ |
| 6 | **PaddleOCR** + 알약 fine-tuning 모델 | 각인 인식 | ⏳ |
| 7 | OpenCV (`opencv-python`) | 색·모양·크기 분석 | ⏳ |
| 8 | FastAPI + Uvicorn | 추론 REST API 서버 | ⏳ |
| 9 | (확장) Whisper STT — `openai-whisper` 또는 `faster-whisper` | 폰 STT fallback | ⏳ |
| 10 | LLM (의도 분류기·등록 RAG) — 모델 추후 선정 | Stage 0.5 + Onboarding | ⏳ |
| 11 | **ChromaDB** 벡터 DB — `pip install chromadb` | 비의료 영역 RAG (Onboarding 약명 정규화·일반 안내). LLM PC `10.10.10.128` 동거 | ⏳ |
| 12 | systemd 서비스 등록 | 자동 시작 | ⏳ |

---

## 2. 사전 준비

- Ubuntu 24.04 LTS + sudo 권한
- NVIDIA GPU (`lspci | grep -i nvidia` 로 확인)
- 메인 서버와 같은 LAN, 고정 IP 권장
- GPU 메모리: 모델 동시 로딩 가능한 용량 (16GB+ 권장, 추후 모델 확정 후 산정)

---

## 3. 설치 절차 (예정)

### 3.1 NVIDIA Driver + CUDA

```bash
# (예정) sudo apt update
# (예정) sudo ubuntu-drivers autoinstall
# (예정) sudo reboot
# (예정) nvidia-smi  # 드라이버 동작 확인

# (예정) CUDA Toolkit 설치 (PyTorch 호환 버전)
# (예정) https://developer.nvidia.com/cuda-downloads 에서 .deb 다운로드 + 설치
```

### 3.2 Python + 가상환경

```bash
# (예정) sudo apt install -y python3.12 python3.12-venv python3-pip
# (예정) python3 -m venv ~/medibridge-inference-venv
# (예정) source ~/medibridge-inference-venv/bin/activate
```

### 3.3 PyTorch (CUDA 빌드)

```bash
# (예정) PyTorch 공식 설치 명령 — CUDA 12.x 호환:
# (예정) pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
# (예정) python -c "import torch; print(torch.cuda.is_available())"  # True 확인
```

### 3.4 Vision 모듈

```bash
# (예정) pip install ultralytics opencv-python paddlepaddle-gpu paddleocr
# (예정) YOLO26 가중치 다운로드 + 알약 fine-tuning 모델 적용
```

### 3.5 FastAPI 추론 서버

```bash
# (예정) pip install fastapi 'uvicorn[standard]' python-multipart httpx
# (예정) 코드 저장소의 inference_server/ 배포
```

### 3.6 (확장) Whisper STT

```bash
# (예정) pip install faster-whisper
# (예정) MVP에선 호출되지 않음 — 확장 fallback 시점에만 사용
```

### 3.7 LLM (모델 선정 후 채움)

```bash
# (예정) 모델 선정에 따라 결정:
# (예정)   - Ollama 설치 + 로컬 LLM
# (예정)   - vLLM
# (예정)   - 외부 API 호출 (별도 설치 없음)
```

### 3.8 systemd 등록

```bash
# (예정) /etc/systemd/system/medibridge-inference.service
# (예정) sudo systemctl enable --now medibridge-inference
```

---

## 4. 설치 완료 체크리스트 (예정)

- [ ] `nvidia-smi` 정상 출력 (드라이버·GPU 인식)
- [ ] PyTorch CUDA 사용 가능 (`torch.cuda.is_available() == True`)
- [ ] YOLO26 단일 이미지 추론 동작 (예제 이미지로 검증)
- [ ] PaddleOCR 한국어 텍스트 인식 동작
- [ ] OpenCV HSV·윤곽선 분석 동작
- [ ] FastAPI 추론 서버 시작 → 메인 서버에서 health check 응답
- [ ] (확장 시) Whisper STT 동작
- [ ] LLM 의도 분류 동작 (모델 선정 후)
- [ ] systemd 자동 시작

---

## 5. 트러블슈팅 (예정)

| 증상 | 원인 / 해결 |
| --- | --- |
| (해당 시 채움) | (해당 시 채움) |

---

## 6. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v0.1 | 2026-05-06 | 팀 (3인) | 스켈레톤 작성. NVIDIA·CUDA·Vision·FastAPI·LLM 설치 예정 항목 명시. Whisper는 확장 fallback으로 분리 |
| **(2026-05-13 메모)** | | | 메인서버 측 인터페이스 확정 — Vision PC 는 `POST /vision/detect_remote` 받음 (페이로드: `{photo_id, storage_url, get_token, mime, purpose}`), 응답 `{candidates:[{item_code,drug_name,confidence,match_keys}], confidence_tier}`. 사진은 `get_token` 으로 데이터 보관 PC(`10.10.10.122:8004`) GET. 자세한 schema: [Api/PillApi.md](../Api/PillApi.md), [Api/MediaApi.md](../Api/MediaApi.md). |
