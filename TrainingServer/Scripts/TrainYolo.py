"""
TrainYolo — YOLO26 알약 검출 모델 학습 스크립트

사용:
    python TrainYolo.py --data ../Datasets/pills.yaml --epochs 100 --imgsz 640

학습 결과:
    runs/detect/train/weights/best.pt → 추론 서버로 SCP 전송
"""
import argparse
from pathlib import Path
from loguru import logger

# from ultralytics import YOLO   # TODO 단계 활성화


def parse_args() -> argparse.Namespace:
    """명령행 인자 파싱"""
    parser = argparse.ArgumentParser(description="YOLO26 알약 검출 학습")
    parser.add_argument("--data", required=True, help="데이터셋 YAML 경로")
    parser.add_argument("--epochs", type=int, default=100)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--batch", type=int, default=16)
    parser.add_argument("--device", default="0", help="GPU 인덱스")
    parser.add_argument("--base-model", default="yolov8n.pt",
                        help="베이스 가중치 (YOLO26 출시 후 yolo26n.pt 등으로 교체)")
    return parser.parse_args()


def train(args: argparse.Namespace) -> None:
    """학습 메인 루틴"""
    # TODO (영역 A 분담):
    #   from ultralytics import YOLO
    #   model = YOLO(args.base_model)
    #   results = model.train(
    #       data=args.data,
    #       epochs=args.epochs,
    #       imgsz=args.imgsz,
    #       batch=args.batch,
    #       device=args.device,
    #       project="runs/detect",
    #       name="train",
    #   )
    #   logger.info(f"학습 완료 — best 가중치: {results.save_dir}/weights/best.pt")
    #
    #   추론 서버로 배포 (수동 또는 스크립트):
    #   scp runs/detect/train/weights/best.pt \
    #       <INF_USER>@<INF_IP>:~/medibridge-inference/Models/yolo26_pills.pt
    logger.info(f"[TrainYolo] TODO 구현 — args: {args}")


if __name__ == "__main__":
    args = parse_args()
    train(args)
