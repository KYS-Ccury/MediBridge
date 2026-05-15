# 알약 각인 OCR Fine-tuning 런북

> 작성: 2026-05-15
> 목적: PaddleOCR 인식(rec) 모델을 알약 각인으로 fine-tune 해
> 사전학습 모델이 못 읽던 음각 각인 인식률을 끌어올린다.
> 배경: `Vision_Tuning_Experiment.md` §6 — 사전학습 PaddleOCR
> det+rec 가 알약 음각을 전혀 못 잡음(9/9 none, 5전략 교차검증).

---

## 1. 핵심 설계

- **검출(det) 단계 미사용.** YOLO 가 이미 알약을 crop → rec 만
  필요. PaddleOCR 의 텍스트검출 실패(입증된 1차 병목)를 우회.
- **PP-OCRv4 영문 rec fine-tune.** backbone(PPLCNetV3+SVTR) 전이,
  분류 head 만 알약 각인 어휘(char_dict 43자)로 재학습.
- **현장열화 augmentation 필수.** AIHub 는 스튜디오 고해상.
  실제 입력은 폰 스크린샷 ~80px. 학습 crop 에 다운스케일·블러·
  JPEG열화·색캐스트 변형을 함께 생성해 저해상 전이 확보.

## 2. 데이터 현황 (정직)

| 항목 | 값 |
|---|---|
| 출처 | AIHub 경구약제 `DATA/01.데이터/2.Validation` (Vision PC) |
| 단일경구약제 JSON | 16,308 |
| **고유 약 종수** | **50종** (Validation 스플릿 한정) |
| 각인 유효(라벨 정제 후) | ~47종 |
| 생성 데이터셋 | train 55,907 / val 1,574 (이미지-split, 누수 0) |
| 문자 사전 | 43종 (영숫자 + 일부 한글 각인) |

⚠ **한계 (반드시 인지)**: AIHub `1.Training` 스플릿(나머지
~4,950종)은 미다운로드. 현 데이터는 **50종 파일럿**. 이 모델은
그 50종 각인은 잘 읽지만 **그 외 약엔 일반화 안 됨**. 운영
일반화는 Training 스플릿(AIHub 계정 다운로드, 대용량) 필요.
→ 본 작업의 목적은 **"도메인 fine-tune 이 사전학습 대비
각인 인식을 개선하는가" 가설 검증(파일럿)**.

## 3. 실행 절차 (Vision PC: ai-trainer@10.10.10.120)

```bash
B=/media/ai-trainer/.../Desktop/MediBridge
V=$B/check_img_ih/venv/bin/python

# (1) 데이터셋 준비 — bbox crop + 현장열화 aug + rec 라벨/사전
$V TrainingServer/Scripts/PrepareEngravingDataset.py \
   --src "$B/DATA/01.데이터/2.Validation" \
   --out "$B/DATA/_ocr_rec_dataset" \
   --aug 3 --split-mode image --val-ratio 0.10
#   split-mode image : 같은 50종의 held-out 사진으로 평가
#                       (각인 학습능력 vs 사전학습 비교)
#   split-mode drug  : 미학습 약으로 평가(일반화; 50종에선 부적합)

# (2) fine-tuning (clone→pretrained→config→train→export 자동)
$V TrainingServer/Scripts/FinetuneOcr.py \
   --data "$B/DATA/_ocr_rec_dataset" \
   --workdir "$B/DATA/_ocr_train" \
   --epochs 60
#   --smoke : 2 epoch 만(파이프라인 검증용)

# 산출물: $B/DATA/_ocr_train/rec_pill_infer/  (추론용 inference model)
```

### GPU 자원 주의 (사실)
- Vision PC GPU = RTX 3060 12GB, **추론서버와 공유**.
- batch_size 128 → OOM. **batch 32 = 약 6GB** 로 안정(검증됨).
- 학습 중 라이브 추론 요청이 오면 경합 → OOM 위험. 전체
  60 epoch(이 데이터·batch32 기준 약 15 시간 추정)은 시연
  비가동 시간대 백그라운드 실행 권장.

## 4. 배포 (학습 완료 후)

`InferenceServer/Vision/OcrEngine.py` 가 환경변수로 fine-tuned
rec 모델을 우선 로드하도록 이미 구현됨:

```bash
# Vision PC .env (InferenceServer)
MEDIBRIDGE_OCR_REC_DIR=/.../DATA/_ocr_train/rec_pill_infer
MEDIBRIDGE_OCR_REC_DICT=/.../DATA/_ocr_rec_dataset/char_dict.txt
```

미지정 시 사전학습 영문 모델로 자동 fallback (회귀 없음).
재가동: `pkill -f Main.py` 후 InferenceServer 재기동.

## 5. 검증 방법

학습 후 `Vision/_exp_harness.py`(있을 경우) 또는 동일 9알약
사진으로 OcrEngine 결과를 사전학습 대비 비교, `Vision_Tuning_
Experiment.md` 에 R7 회차로 수치 기록.

평가 지표(파일럿): val 1,574장(같은 47종 held-out 사진)에서
- exact match rate, normalized-edit-distance
- 사전학습 PP-OCRv4(en) 대비 Δ

## 6. 파일

| 파일 | 역할 |
|---|---|
| `TrainingServer/Scripts/PrepareEngravingDataset.py` | AIHub→rec 데이터셋 (crop+aug+라벨+사전) |
| `TrainingServer/Scripts/FinetuneOcr.py` | clone→pretrained→config→train→export |
| `InferenceServer/Vision/OcrEngine.py` | `MEDIBRIDGE_OCR_REC_DIR` 로 fine-tuned rec 로드 |

> 추측 표기: 학습시간(~15h)·일반화 효과는 추정. 50종 파일럿
> 결과가 사전학습 대비 개선되면 Training 스플릿 확보 후 전체
> 재학습 가치가 입증된다(가설).
