# TrainingServer — 영역 A (Python, Ubuntu 24.04 + GPU)

> 메디브릿지 학습 서버. 데이터 수집·전처리, YOLO26 / PaddleOCR fine-tuning, 검증, 추론 서버로 SCP 배포.
>
> 운용과 격리됨 — 운용 중에는 비활성. 모델 배포 시에만 추론 서버로 SCP.

---

## 빠른 시작

1. [Docs/Install/TrainingServerInstall.md](../Docs/Install/TrainingServerInstall.md) 의 NVIDIA·CUDA 셋업
2. Python 가상환경:
   ```bash
   python3 -m venv ~/medibridge-train-venv
   source ~/medibridge-train-venv/bin/activate
   pip install -r requirements.txt
   pip install torch torchvision --index-url https://download.pytorch.org/whl/cu121
   ```
3. AI Hub 경구약제 5,000종 데이터셋 신청·다운로드 → `Datasets/` 에 배치
4. YOLO26 학습:
   ```bash
   cd Scripts
   python TrainYolo.py --data ../Datasets/pills.yaml --epochs 100
   ```
5. 결과를 추론 서버로 배포:
   ```bash
   scp runs/detect/train/weights/best.pt \
       <INF_USER>@<INF_IP>:~/medibridge-inference/Models/yolo26_pills.pt
   ```

---

## 폴더 구조

| 폴더 | 책임 |
| --- | --- |
| `Scripts/` | 학습 스크립트 (TrainYolo, FinetuneOcr 등) |
| `Datasets/` | 학습 데이터 (`.gitignore` 로 제외 — 수십 GB 가능) |
| `Models/` | 학습 결과 가중치 (`.gitignore` 로 제외) |

---

## TODO 마커별 분담 작업

| 영역 | 파일 | 작업 |
| --- | --- | --- |
| YOLO 학습 | `Scripts/TrainYolo.py` | Ultralytics YOLO API 통합, 데이터 YAML 작성 |
| OCR 학습 | `Scripts/FinetuneOcr.py` | PaddleOCR 학습 도구 통합, 알약 데이터셋 변환 |
| 데이터 준비 | (신규) `Scripts/PreparePillDataset.py` | AI Hub 5,000종 → YOLO 형식 변환, augmentation |
| 평가 | (신규) `Scripts/EvaluateModel.py` | Top-K 정확도 측정, 베이스라인(Top-5 10%) 대비 |
| 배포 | (신규) `Scripts/DeployToInference.sh` | SCP + 모델 hot reload 트리거 |

---

## 데이터셋

| 출처 | 내용 | 용량 |
| --- | --- | --- |
| AI Hub 경구약제 5,000종 | 전문의약품 3,143종 + 일반의약품 1,857종 | 수십 GB |
| 식약처 낱알식별 정보 | 모양·색·각인 매핑 (메인서버 DB와 공유) | 수백 MB |

---

## 모델 배포 흐름

```
[ TrainingServer ]
  학습 완료 → runs/detect/train/weights/best.pt
       ↓ SCP / 파일 전송
[ InferenceServer ]
  Models/yolo26_pills.pt 로 배치
       ↓ 서버 재시작 또는 hot reload
  추론 라우터에서 사용
```

운용 중에는 학습 서버 ↔ 추론 서버 통신 없음 (모델 배포 시에만).

---

## 관련 문서

- [Docs/Install/TrainingServerInstall.md](../Docs/Install/TrainingServerInstall.md)
- [Docs/시스템 흐름 정리본_ver2.md](../Docs/시스템%20흐름%20정리본_ver2.md) §7.1 학습 데이터
- [Docs/요구사항_분석서_ver2.md](../Docs/요구사항_분석서_ver2.md) §4 영역 A
