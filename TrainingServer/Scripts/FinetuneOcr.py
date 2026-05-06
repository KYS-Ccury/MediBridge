"""
FinetuneOcr — PaddleOCR 알약 각인 fine-tuning 스크립트

사용:
    python FinetuneOcr.py --data ../Datasets/pill_engravings/ --epochs 50

PaddleOCR fine-tuning은 yaml 설정 + 별도 도구 사용 — 본 스크립트는 진입점만.
"""
import argparse
from loguru import logger


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="PaddleOCR 알약 각인 fine-tuning")
    parser.add_argument("--data", required=True, help="알약 각인 데이터셋 경로")
    parser.add_argument("--config", default="configs/rec_korean_lite_train.yml")
    parser.add_argument("--epochs", type=int, default=50)
    parser.add_argument("--pretrained", default="rec_korean_lite/best_accuracy.pdparams")
    return parser.parse_args()


def finetune(args: argparse.Namespace) -> None:
    """fine-tuning 진입점"""
    # TODO (영역 A 분담):
    #   1. PaddleOCR 학습 도구 설치 (PaddleOCR 저장소 clone 후 tools/train.py 사용)
    #   2. 알약 데이터셋을 PaddleOCR 형식으로 변환
    #   3. config YAML 수정 (data_dir, label_file_list)
    #   4. python tools/train.py -c <config> -o Global.pretrained_model=<base> 실행
    #   5. 결과 가중치 → 추론 서버 SCP 배포
    logger.info(f"[FinetuneOcr] TODO 구현 — args: {args}")


if __name__ == "__main__":
    args = parse_args()
    finetune(args)
