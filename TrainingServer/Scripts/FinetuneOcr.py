"""
FinetuneOcr — PaddleOCR 인식(rec) 모델 알약 각인 fine-tuning

PrepareEngravingDataset.py 가 만든 데이터셋(_ocr_rec_dataset)으로
PP-OCRv4 영문 rec 모델을 fine-tune. 검출(det) 단계는 사용 안 함 —
YOLO 가 이미 알약을 crop 하므로 rec 만 필요.

흐름:
  1. PaddleOCR repo clone (tools/train.py 필요)
  2. 사전학습 rec 모델 다운로드 (en_PP-OCRv4_rec)
  3. 데이터셋 경로/char_dict 로 rec config YAML 생성
  4. tools/train.py 실행 (GPU)
  5. tools/export_model.py 로 추론용 inference model 추출
  6. 결과 → OcrEngine 가 MEDIBRIDGE_OCR_REC_DIR 로 로드

사용 (Vision PC, venv):
  python FinetuneOcr.py --data /.../DATA/_ocr_rec_dataset \
     --workdir /.../DATA/_ocr_train --epochs 60 [--smoke]
"""
from __future__ import annotations
import argparse
import os
import subprocess
import sys
from pathlib import Path

PADDLEOCR_REPO = "https://github.com/PaddlePaddle/PaddleOCR.git"
PRETRAIN_URL = ("https://paddleocr.bj.bcebos.com/PP-OCRv4/english/"
                "en_PP-OCRv4_rec_train.tar")

REC_CONFIG_TMPL = """
Global:
  use_gpu: true
  epoch_num: {epochs}
  log_smooth_window: 20
  print_batch_step: 20
  save_model_dir: {save_dir}
  save_epoch_step: 5
  eval_batch_step: [0, 500]
  cal_metric_during_train: true
  pretrained_model: {pretrained}
  checkpoints:
  save_inference_dir: {infer_dir}
  use_visualdl: false
  character_dict_path: {char_dict}
  max_text_length: 25
  use_space_char: true
  save_res_path: {save_dir}/predicts.txt
  distributed: false

Optimizer:
  name: Adam
  beta1: 0.9
  beta2: 0.999
  lr:
    name: Cosine
    learning_rate: 0.0005
    warmup_epoch: 2
  regularizer:
    name: L2
    factor: 3.0e-05

Architecture:
  model_type: rec
  algorithm: SVTR_LCNet
  Transform:
  Backbone:
    name: PPLCNetV3
    scale: 0.95
  Head:
    name: MultiHead
    head_list:
      - CTCHead:
          Neck:
            name: svtr
            dims: 120
            depth: 2
            hidden_dims: 120
            kernel_size: [1, 3]
            use_guide: true
          Head:
            fc_decay: 0.00001
      - NRTRHead:
          nrtr_dim: 384
          max_text_length: 25

Loss:
  name: MultiLoss
  loss_config_list:
    - CTCLoss:
    - NRTRLoss:

PostProcess:
  name: CTCLabelDecode

Metric:
  name: RecMetric
  main_indicator: acc
  ignore_space: false

Train:
  dataset:
    name: SimpleDataSet
    data_dir: {data_dir}
    ext_op_transform_idx: 1
    label_file_list:
      - {train_list}
    transforms:
      - DecodeImage: {{img_mode: BGR, channel_first: false}}
      - RecAug:
      - MultiLabelEncode:
          gtc_encode: NRTRLabelEncode
      - RecResizeImg:
          image_shape: [3, 48, 320]
      - KeepKeys:
          keep_keys: [image, label_ctc, label_gtc, length, valid_ratio]
  loader:
    shuffle: true
    batch_size_per_card: 32
    drop_last: true
    num_workers: 2

Eval:
  dataset:
    name: SimpleDataSet
    data_dir: {data_dir}
    label_file_list:
      - {val_list}
    transforms:
      - DecodeImage: {{img_mode: BGR, channel_first: false}}
      - MultiLabelEncode:
          gtc_encode: NRTRLabelEncode
      - RecResizeImg:
          image_shape: [3, 48, 320]
      - KeepKeys:
          keep_keys: [image, label_ctc, label_gtc, length, valid_ratio]
  loader:
    shuffle: false
    drop_last: false
    batch_size_per_card: 32
    num_workers: 2
"""


def run(cmd, cwd=None, env=None):
    print(f"[run] {' '.join(str(c) for c in cmd)}", flush=True)
    subprocess.check_call([str(c) for c in cmd], cwd=cwd, env=env)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", required=True, help="_ocr_rec_dataset 경로")
    ap.add_argument("--workdir", required=True, help="학습 작업 디렉토리")
    ap.add_argument("--epochs", type=int, default=60)
    ap.add_argument("--smoke", action="store_true",
                    help="2 epoch — 파이프라인 검증용")
    args = ap.parse_args()

    data = Path(args.data).resolve()
    work = Path(args.workdir).resolve()
    work.mkdir(parents=True, exist_ok=True)
    repo = work / "PaddleOCR"
    save_dir = work / "rec_pill"
    infer_dir = work / "rec_pill_infer"

    if not (data / "train_list.txt").exists():
        print(f"[err] {data}/train_list.txt 없음 — 데이터셋 먼저 준비")
        return 1

    if not repo.exists():
        run(["git", "clone", "--depth", "1", PADDLEOCR_REPO, repo])

    pre_tar = work / "en_PP-OCRv4_rec_train.tar"
    pre_dir = work / "en_PP-OCRv4_rec_train"
    if not pre_dir.exists():
        if not pre_tar.exists():
            run(["wget", "-q", "-O", pre_tar, PRETRAIN_URL])
        run(["tar", "-xf", pre_tar, "-C", work])
    pretrained = pre_dir / "best_accuracy"

    epochs = 2 if args.smoke else args.epochs
    cfg = REC_CONFIG_TMPL.format(
        epochs=epochs, save_dir=save_dir, infer_dir=infer_dir,
        pretrained=pretrained, char_dict=data / "char_dict.txt",
        data_dir=data, train_list=data / "train_list.txt",
        val_list=data / "val_list.txt")
    cfg_path = work / "rec_pill.yml"
    cfg_path.write_text(cfg, encoding="utf-8")
    print(f"[cfg] {cfg_path}", flush=True)

    env = dict(os.environ)
    env.setdefault("CUDA_VISIBLE_DEVICES", "0")

    # ⭐ 라이브 추론과 GPU 공유 → 데모 스파이크로 OOM 사망 가능.
    #   체크포인트에서 자동 resume + 재시도로 15h 투자 보호.
    #   PaddleOCR 는 save_dir/latest.pdparams 를 주기 저장.
    import time
    max_retries = 0 if args.smoke else 8
    attempt = 0
    while True:
        cmd = [sys.executable, "tools/train.py", "-c", cfg_path]
        latest = save_dir / "latest.pdparams"
        if latest.exists():
            # resume: checkpoints 지정 시 그 지점부터 이어서 학습
            cmd += ["-o", f"Global.checkpoints={save_dir}/latest"]
            print(f"[resume] {latest} 에서 재개 (attempt {attempt})",
                  flush=True)
        try:
            run(cmd, cwd=repo, env=env)
            break                       # 정상 종료
        except subprocess.CalledProcessError as e:
            attempt += 1
            if attempt > max_retries:
                print(f"[fail] 재시도 {max_retries} 초과 — 중단", flush=True)
                raise
            print(f"[retry] 학습 비정상 종료(OOM 등) — "
                  f"{attempt}/{max_retries}, 60s 후 체크포인트 재개",
                  flush=True)
            time.sleep(60)              # GPU 점유 프로세스 해소 대기

    run([sys.executable, "tools/export_model.py", "-c", cfg_path,
         "-o", f"Global.pretrained_model={save_dir}/best_accuracy",
         f"Global.save_inference_dir={infer_dir}"],
        cwd=repo, env=env)

    print(f"\n[done] 추론 모델: {infer_dir}", flush=True)
    print(f"[deploy] OcrEngine 에 MEDIBRIDGE_OCR_REC_DIR={infer_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
