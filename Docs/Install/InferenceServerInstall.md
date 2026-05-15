# 추론 서버 설치 매뉴얼 (Ubuntu 24.04, GPU)

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 추론 서버 PC 설치 매뉴얼 |
| **대상 OS** | Ubuntu 24.04 LTS |
| **하드웨어** | NVIDIA GPU 권장 (CUDA 호환) |
| **역할** | YOLO26 알약 검출, PaddleOCR 각인 인식, OpenCV 색·모양 분석, **(확장 fallback) Whisper STT**, LLM 의도 분류기·등록 LLM·RAG |
| **버전** | v0.3 (LLM PC + Vision PC 양쪽 실 셋업 완성) |
| **작성일** | 2026-05-06 (v0.1) / **개정일 2026-05-15 (v0.3)** |
| **작성자** | 팀 (3인) |

> 📌 **2026-05-15 v0.2 변경**: LLM PC (`10.10.10.128:8002`) 측 실 셋업 절차 추가 — Python 가상환경 / OpenAI gpt-4.1-nano 기본 / Ollama gemma4:e4b fallback / KURE-v1 임베딩 / Chroma DB / 환경변수 / systemd 단위. Vision PC (`10.10.10.120:8003`) 측은 인효 담당 (스켈레톤 유지).
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

## 3.A LLM PC 실 셋업 — `10.10.10.128:8002` (2026-05-15 신규)

### 3.A.1 OS·Python 환경
```bash
# Ubuntu 24.04 LTS, sudo 권한
sudo apt update && sudo apt install -y python3.12 python3.12-venv git
cd /home/medibridge
git clone <repo> MediBridge   # 또는 SCP 로 코드 복사
cd MediBridge/InferenceServer
python3.12 -m venv .venv
source .venv/bin/activate
pip install -U pip wheel
pip install -r requirements.txt
```

### 3.A.2 환경변수 (`.env`)
```bash
cp .env.sample .env
# .env 수정 — 시크릿 입력
#   OPENAI_API_KEY=sk-...
#   MEDIBRIDGE_LLM_BACKEND=openai (기본)
#   MEDIBRIDGE_VISION_ENABLED=false   ← LLM PC 는 Vision 비활성
#   MEDIBRIDGE_LLM_ENABLED=true
```

### 3.A.3 (선택) Ollama fallback 설치
```bash
curl -fsSL https://ollama.com/install.sh | sh
ollama pull gemma4:e4b              # ~3 GB Q4
ollama serve &                      # :11434
# OpenAI 장애 시: MEDIBRIDGE_LLM_BACKEND=ollama 전환
```

### 3.A.4 KURE-v1 임베딩 모델 사전 다운로드 (선택, 첫 호출 시 자동 다운로드됨)
```bash
python -c "from sentence_transformers import SentenceTransformer; SentenceTransformer('nlpai-lab/KURE-v1', device='cuda')"
```

### 3.A.5 RAG 인덱스 빌드 (1회)
```bash
# 메인서버 MariaDB 에 접근 가능해야 함 (식약처 데이터가 채워진 상태)
export MEDIBRIDGE_DB_PASSWORD='...'
python ../TrainingServer/Scripts/BuildRagIndex.py \
  --db-host 10.10.10.97 --db-user medibridge_app \
  --db-name medibridge \
  --embedding nlpai-lab/KURE-v1 --device cuda \
  --out ./chroma_db
```

### 3.A.6 서버 실행
```bash
# 개발 모드
set -a; source .env; set +a
python Main.py

# 또는 uvicorn 직접
uvicorn Main:app --host 0.0.0.0 --port 8002

# 검증
curl http://127.0.0.1:8002/health | jq
curl -X POST http://127.0.0.1:8002/intent/classify \
  -H 'Content-Type: application/json' \
  -d '{"text":"이 약 뭐야?"}' | jq
# 기대: {"category":"PILL_IDENTIFY","confidence":0.9x,"injection_flag":false}
```

### 3.A.7 systemd 자동 시작 (운영)
```ini
# /etc/systemd/system/medibridge-inference.service
[Unit]
Description=MediBridge LLM Inference Server
After=network.target

[Service]
Type=simple
User=medibridge
WorkingDirectory=/home/medibridge/MediBridge/InferenceServer
EnvironmentFile=/home/medibridge/MediBridge/InferenceServer/.env
ExecStart=/home/medibridge/MediBridge/InferenceServer/.venv/bin/uvicorn Main:app --host 0.0.0.0 --port 8002
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```
```bash
sudo systemctl daemon-reload
sudo systemctl enable --now medibridge-inference
sudo systemctl status medibridge-inference
```

---

## 3.B Vision PC 실 셋업 — `10.10.10.120:8003` (2026-05-15 신규)

Vision PC 담당자가 `ai-trainer@10.10.10.120:/media/.../yesom/Desktop/MediBridge/check_img_ih/`
에서 YOLO + PaddleOCR + OpenCV 로 추론 완성. 본 절은 **본 프로젝트 구조로 코드 이식 후 FastAPI 가동** 절차.

### 3.B.1 본 InferenceServer 구조 배포 (rsync)

```bash
# LLM PC 또는 개발 PC 에서 (메인 코드 베이스가 있는 곳)
cd /path/to/MediBridge

# Vision PC 로 InferenceServer + TrainingServer 동기화
# (담당자 기존 check_img_ih/ 는 그대로 유지 — 모델·데이터셋 재사용)
rsync -avz --exclude='__pycache__' --exclude='.venv' --exclude='chroma_db' \
  InferenceServer/ \
  ai-trainer@10.10.10.120:'/media/ai-trainer/fd234fd8-cefc-4354-bc18-b8babbcf4f31/home/yesom/Desktop/MediBridge/InferenceServer/'
```

### 3.B.2 Vision PC 측 venv + 의존성

```bash
ssh ai-trainer@10.10.10.120
cd "/media/ai-trainer/fd234fd8-.../yesom/Desktop/MediBridge/InferenceServer"
python3.12 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
# 추가:
pip install ultralytics paddlepaddle-gpu paddleocr opencv-python
```

### 3.B.3 학습 가중치 매핑

담당자 학습 결과 `runs/detect/runs/detect/train_single_602020/weights/best.pt` 를 InferenceServer 가 보는 위치로:

```bash
mkdir -p Models
cp ../check_img_ih/runs/detect/runs/detect/train_single_602020/weights/best.pt \
   Models/yolo26_pills.pt
# 또는 환경변수로 지정:
# export MEDIBRIDGE_YOLO_WEIGHTS=/absolute/path/to/best.pt
```

### 3.B.4 환경변수 (`.env`)

```bash
cp .env.sample .env
# .env 수정 — Vision PC 모드:
#   MEDIBRIDGE_INFERENCE_PORT=8003
#   MEDIBRIDGE_VISION_ENABLED=true
#   MEDIBRIDGE_LLM_ENABLED=false       ← 핵심! Vision PC 는 LLM 비활성
#   MEDIBRIDGE_YOLO_WEIGHTS=Models/yolo26_pills.pt
```

### 3.B.5 실행 + 검증

```bash
set -a; source .env; set +a
python Main.py    # :8003

# 다른 셸에서:
curl http://127.0.0.1:8003/health | jq
# 기대: status=ok, reasons=[]

# 더미 이미지로 검출 테스트
curl -X POST http://127.0.0.1:8003/vision/detect \
  -F "image=@test_pill.jpg" | jq

# 보관 PC 통합 흐름 (메인서버 + 보관 PC 가 떠 있어야)
# 메인서버가 photo_id/storage_url/get_token 발급 후 호출:
curl -X POST http://127.0.0.1:8003/vision/detect_remote \
  -H 'Content-Type: application/json' \
  -d '{
    "photo_id": "ph_xxx",
    "storage_url": "http://10.10.10.122:8004/storage/photos/anon_xxx/ph_xxx.jpg",
    "get_token": "<HMAC>",
    "mime": "image/jpeg",
    "purpose": "IDENTIFY"
  }' | jq
```

### 3.B.6 systemd 자동 시작

```ini
# /etc/systemd/system/medibridge-vision.service
[Unit]
Description=MediBridge Vision Inference Server
After=network.target

[Service]
Type=simple
User=ai-trainer
WorkingDirectory=/media/ai-trainer/fd234fd8-cefc-4354-bc18-b8babbcf4f31/home/yesom/Desktop/MediBridge/InferenceServer
EnvironmentFile=/media/ai-trainer/fd234fd8-cefc-4354-bc18-b8babbcf4f31/home/yesom/Desktop/MediBridge/InferenceServer/.env
ExecStart=/media/ai-trainer/fd234fd8-cefc-4354-bc18-b8babbcf4f31/home/yesom/Desktop/MediBridge/InferenceServer/.venv/bin/uvicorn Main:app --host 0.0.0.0 --port 8003
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
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
| **v0.2** | **2026-05-15** | 팀 (3인) | LLM PC 측 실 셋업 §3.A 신규 — Python venv / OpenAI gpt-4.1-nano 기본 / Ollama gemma4:e4b fallback / KURE-v1 임베딩 / Chroma RAG 인덱스 빌드 (BuildRagIndex.py) / systemd 단위. `MEDIBRIDGE_VISION_ENABLED=false` 환경변수로 LLM PC ↔ Vision PC 코드 공유. Vision PC 측은 인효 담당 (스켈레톤 유지). 관련 설계: [LlmInferenceServer_Design.md](../LlmInferenceServer_Design.md). |
| **v0.3** | **2026-05-15** | 팀 (3인) | Vision PC §3.B 실 셋업 신규 — 담당자(인효) 의 `check_img_ih/` (YOLO + PaddleOCR + OpenCV) 추론 코드를 본 `InferenceServer/Vision/` 5종 모듈로 이식 완료 (`YoloDetector` · `OcrEngine` · `ColorClassifier` · `ShapeClassifier` · `SizeMeasurer`). `Routers/Vision.py` 의 `POST /vision/detect_remote` 신규 — 메인서버 호출 시 보관 PC 직접 GET → 검출 → 통합 분석 → `match_keys` 응답. rsync 배포·환경변수·systemd 단위·검증 명령 명시. |
