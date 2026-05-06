# 학습 서버 설치 매뉴얼 (Ubuntu 24.04, GPU)

| 항목 | 내용 |
| --- | --- |
| **문서 종류** | 학습 서버 PC 설치 매뉴얼 |
| **대상 OS** | Ubuntu 24.04 LTS |
| **하드웨어** | NVIDIA GPU 권장 (CUDA 호환, 가능하면 추론 서버보다 큰 VRAM) |
| **역할** | 데이터 수집·전처리, 모델 학습(YOLO26 fine-tuning, PaddleOCR fine-tuning 등), 검증, **추론 PC로 모델 배포(SCP)** |
| **버전** | v0.1 (스켈레톤) |
| **작성일** | 2026-05-06 |
| **작성자** | 팀 (3인) |

> ⏳ **본 매뉴얼은 스켈레톤 상태.** 학습 서버 셋업 진입 시 각 절을 채워나간다.
>
> 학습 서버는 **운용과 격리**된다 (기획서 v2 / 시스템 흐름 정리본 v2). 운용 중에는 비활성, 모델 배포는 **SCP/파일 전송으로 추론 서버에만 전달**.

---

## 1. 설치 예정 항목 (체크리스트)

| # | 도구 / 컴포넌트 | 용도 | 상태 |
| --- | --- | --- | --- |
| 1 | NVIDIA Driver + CUDA Toolkit + cuDNN | GPU 학습 기반 | ⏳ |
| 2 | Python 3.12 + venv | 학습 환경 | ⏳ |
| 3 | PyTorch (CUDA 빌드) | 모델 학습 프레임워크 | ⏳ |
| 4 | **Ultralytics YOLO** (YOLO26 fine-tuning) | 알약 검출 모델 학습 | ⏳ |
| 5 | **PaddleOCR 학습 환경** | 각인 OCR fine-tuning | ⏳ |
| 6 | OpenCV, NumPy, Pandas, scikit-learn | 데이터 처리·평가 | ⏳ |
| 7 | Jupyter / TensorBoard | 학습 모니터링·실험 | ⏳ |
| 8 | OpenSSH 서버 (`openssh-server`) | SCP로 추론 서버에 모델 전송 | ⏳ |
| 9 | (선택) MLflow / Weights & Biases | 실험 추적 | ⏳ |
| 10 | AI Hub 5,000종 데이터셋 | 학습 데이터 (출처: aihub.or.kr) | ⏳ |

---

## 2. 사전 준비

- Ubuntu 24.04 LTS + sudo 권한
- NVIDIA GPU (학습용, VRAM 16GB+ 권장)
- 데이터 저장용 디스크 — AI Hub 5,000종 데이터셋이 수십 GB 가능, 별도 SSD 권장
- 추론 서버와의 SSH 키 페어 (모델 배포용)

---

## 3. 설치 절차 (예정)

### 3.1 NVIDIA Driver + CUDA

추론 서버와 동일 절차. 다만 **CUDA 버전을 추론 서버와 일치시키는 것을 권장** (학습 ↔ 추론 호환성).

```bash
# (예정) sudo apt update
# (예정) sudo ubuntu-drivers autoinstall
# (예정) sudo reboot
# (예정) nvidia-smi
# (예정) CUDA Toolkit 설치
```

### 3.2 Python + 가상환경

```bash
# (예정) sudo apt install -y python3.12 python3.12-venv python3-pip
# (예정) python3 -m venv ~/medibridge-train-venv
# (예정) source ~/medibridge-train-venv/bin/activate
```

### 3.3 PyTorch (CUDA 빌드)

```bash
# (예정) pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
# (예정) python -c "import torch; print(torch.cuda.is_available())"
```

### 3.4 학습 프레임워크

```bash
# (예정) pip install ultralytics opencv-python numpy pandas scikit-learn
# (예정) pip install paddlepaddle-gpu  # PaddleOCR 학습용
# (예정) pip install jupyterlab tensorboard
```

### 3.5 OpenSSH 서버 (모델 배포용)

```bash
# (예정) sudo apt install -y openssh-server
# (예정) sudo systemctl enable --now ssh

# (예정) 추론 서버에 SSH 공개키 등록 — 학습 서버에서 추론 서버로 SCP 무비밀번호 전송:
# (예정) ssh-keygen -t ed25519
# (예정) ssh-copy-id <inference_user>@<inference_server_ip>
```

### 3.6 데이터셋 준비

```bash
# (예정) AI Hub 경구약제 5,000종 데이터셋 다운로드:
#         https://aihub.or.kr/aihubdata/data/view.do?dataSetSn=576
# (예정) 신청·승인 후 다운로드. 압축 해제 후 ~/datasets/ai-hub-pills/ 에 배치
```

### 3.7 학습 파이프라인 설정

```bash
# (예정) 코드 저장소의 training/ 폴더 사용
# (예정) YOLO26 학습 예: yolo train data=pills.yaml model=yolo26n.pt epochs=100 ...
# (예정) PaddleOCR fine-tuning은 별도 절차
```

### 3.8 모델 배포 (SCP 절차)

```bash
# (예정) 학습 완료 후 추론 서버로 모델 전송:
# (예정) scp ~/runs/detect/train/weights/best.pt <inf_user>@<inf_ip>:~/medibridge-inference/models/yolo26.pt
```

---

## 4. 설치 완료 체크리스트 (예정)

- [ ] `nvidia-smi` 정상 출력
- [ ] PyTorch CUDA 사용 가능
- [ ] YOLO26 학습 환경 준비 (예제 데이터로 1 epoch 동작 확인)
- [ ] PaddleOCR 학습 환경 준비
- [ ] OpenSSH 서버 동작 + 추론 서버 SSH 키 등록 (무비밀번호 SCP)
- [ ] AI Hub 데이터셋 신청·다운로드 완료
- [ ] 학습 결과를 추론 서버로 SCP 전송 테스트 통과

---

## 5. 트러블슈팅 (예정)

| 증상 | 원인 / 해결 |
| --- | --- |
| (해당 시 채움) | (해당 시 채움) |

---

## 6. 변경 이력

| 버전 | 일자 | 작성자 | 변경 사항 |
| --- | --- | --- | --- |
| v0.1 | 2026-05-06 | 팀 (3인) | 스켈레톤 작성. NVIDIA·PyTorch·Ultralytics·PaddleOCR·OpenSSH·데이터셋 설치 예정 항목 명시 |
