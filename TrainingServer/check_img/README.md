# MediBridge POC — 알약 식별 가능성 검증 (`check_img/`)

| 항목 | 내용 |
| --- | --- |
| **목적** | YOLO/PaddleOCR/OpenCV 기반 다중 알약 식별이 **가능한지** 사전 검증. 본 시스템(MainServer/Client) 과 무관한 독립 POC |
| **위치** | `TrainingServer/check_img/` |
| **검증할 3가지** | ① 여러 알약 **검출** ② 각인 **OCR** ③ 색·모양 **정량화** |
| **대상 OS** | Ubuntu 20.04 / 22.04 / 24.04 (또는 macOS / WSL2 Ubuntu). Windows native 도 작동 가능 (PaddleOCR 호환성 주의) |
| **Python** | **3.10 / 3.11 / 3.12** 모두 가능. 단 3.12 는 **PaddlePaddle 3.0+** 필요 (2.6.x 는 3.11 까지) |

---

## 0. 기술 스택과 POC 한계

| 컴포넌트 | 본 POC 사용 모듈 | 한계 |
| --- | --- | --- |
| 검출 | **OpenCV 윤곽선** (학습 불필요) | 단순 배경(흰 종이 등)에서만 잘 동작. 복잡 배경은 YOLO fine-tuning 필요 — 본 POC 범위 외 |
| 각인 OCR | **PaddleOCR** 사전학습 한글·영문 모델 | 또렷한 각인만 인식. 흐릿한 미세 각인은 fine-tuning 필요 |
| 색 분석 | **OpenCV HSV 분포** | 조명에 영향받음. 식약처 카테고리(흰색·노란색 등) 매핑은 휴리스틱이므로 실 데이터로 보정 권장 |
| 모양 분석 | **OpenCV 윤곽선 + 회전사각형** | 원형/타원형/캡슐형/장방형/다각형 분류. 겹친 알약 분리 어려움 |

> ⚠ "YOLO26" 은 공식 출시된 모델명이 아닙니다. 본 POC 는 사전학습 YOLO 가 알약 클래스를 모르는 한계 때문에 **검출에 OpenCV** 를 사용. 운영 단계 진입 시 AI Hub 5,000종 데이터로 YOLOv8/v11 fine-tuning 검토.

---

## 1. 사전 준비 — 시스템 패키지

### Ubuntu

이미 시스템에 Python (3.10/3.11/3.12) 이 있으면 그걸 사용. 본 매뉴얼은 3.12 기준으로도 동작하도록 작성.

```bash
sudo apt update

# (이미 시스템 Python 있으면) — venv 패키지만 보강
sudo apt install -y python3-venv python3-dev python3-pip \
                    libgl1 libglib2.0-0     # OpenCV 동작용

# (특정 버전 명시 설치 예: 3.11)
# sudo apt install -y python3.11 python3.11-venv python3.11-dev

# 시스템 Python 버전 확인
python3 --version       # 예: Python 3.12.3
```

> Python 3.12 는 PaddlePaddle 3.0+ 와 함께 사용 (§3.3 참조). 3.10/3.11 은 paddlepaddle 2.6.x 도 가능.

### macOS

```bash
brew install python@3.11
```

### Windows

Python 3.11 공식 인스톨러로 설치 + "Add Python to PATH" 체크. 본 매뉴얼의 셸 명령은 Linux/macOS 기준이라 PowerShell 에선 일부 다름 (가상환경 activate 가 `.\venv\Scripts\Activate.ps1`).

---

## 2. 가상환경 (venv) 만들기

**모든 명령은 `check_img/` 폴더에서 실행.**

```bash
cd ~/MediBridge/TrainingServer/check_img    # 본인 경로로 조정

# 1) venv 생성 (이 폴더 안에 venv/ 디렉토리 생김)
python3 -m venv venv
# (특정 버전 명시: python3.12 -m venv venv  /  python3.11 -m venv venv)

# 2) venv 활성화
source venv/bin/activate     # Linux/macOS
# (Windows PowerShell: .\venv\Scripts\Activate.ps1)
# (Windows cmd:        venv\Scripts\activate.bat)

# 3) 활성화 확인 — 프롬프트 앞에 (venv) 가 붙고, 아래가 venv 안 경로를 가리키면 OK
which python   # → .../check_img/venv/bin/python
which pip      # → .../check_img/venv/bin/pip
python --version    # → 시스템 Python 버전 그대로
```

> 📌 **venv 종료**: `deactivate`. 종료 후엔 시스템 Python 으로 돌아감.

---

## 3. 의존성 설치

### 3.1 pip 자체를 최신화

```bash
pip install --upgrade pip wheel setuptools
```

### 3.2 requirements.txt 설치 (PaddlePaddle 제외)

```bash
pip install -r requirements.txt
```

설치되는 것: `opencv-python`, `numpy`, `paddleocr`, 기타 OCR 의존 패키지.

> ⚠ **PaddlePaddle (PaddleOCR 의 추론 엔진)** 은 `requirements.txt` 에 포함하지 않았습니다 — CPU/GPU 분기 + CUDA 버전별 wheel 이 달라 별도 설치가 필요.

### 3.3 PaddlePaddle (CPU) — 가장 단순

GPU 없거나 처음 검증만 하면 CPU 로 충분 (이미지 1장에 1~3초).

**Python 버전에 따라 명령이 다릅니다**:

```bash
# Python 3.12 (PaddlePaddle 3.0+ 필요)
pip install "paddlepaddle>=3.0.0"

# Python 3.10 / 3.11 — 2.6.x 도 OK (안정성 검증된 버전)
pip install paddlepaddle==2.6.1
# 또는 최신:
# pip install "paddlepaddle>=3.0.0"
```

> 📌 PaddleOCR 의 paddle 버전 호환:
>   - Python 3.12 + paddlepaddle 3.0 → paddleocr 2.7~2.10 모두 호환
>   - Python 3.10/3.11 + paddlepaddle 2.6 → paddleocr 2.7.x 권장

설치 검증:
```bash
python -c "import paddle; print('paddle:', paddle.__version__)"
python -c "from paddleocr import PaddleOCR; print('PaddleOCR import OK')"
```

### 3.4 PaddlePaddle (GPU, 선택) + PyTorch + CUDA 격리 설치

> 🛡 **시스템 영향 우려에 대한 답**: `pip install` 을 venv 활성화 상태에서 실행하면 패키지가 **`venv/lib/python3.x/site-packages/`** 안에만 들어갑니다. 시스템 site-packages 에 영향 X. PyPI 의 PyTorch/Paddle wheel 은 **CUDA 런타임을 자체 번들** 하므로 시스템 CUDA 도 안 건드림 (단, NVIDIA 드라이버는 시스템에 미리 있어야 함).

#### 사전 확인 — venv 활성화 상태인가?

```bash
echo "$VIRTUAL_ENV"
# 결과가 .../check_img/venv 면 OK. 빈 문자열이면 venv 미활성화 — 멈추고 venv/bin/activate 부터.
```

#### NVIDIA 드라이버·CUDA 호환성

```bash
nvidia-smi    # 출력의 CUDA Version: 항목 확인 (드라이버가 지원하는 최대 CUDA)
```

CUDA 11.8 / 12.1 / 12.4 가 가장 흔한 옵션. PyTorch·Paddle 모두 지원.

#### PyTorch (venv 안만, CUDA 12.1 예시)

```bash
# 반드시 venv 활성화 상태에서!
pip install torch==2.4.1 torchvision==0.19.1 \
    --index-url https://download.pytorch.org/whl/cu121
```

CPU only 라면:
```bash
pip install torch==2.4.1 torchvision==0.19.1 \
    --index-url https://download.pytorch.org/whl/cpu
```

#### PaddlePaddle GPU

**Python 3.12 (PaddlePaddle 3.0+, CUDA 11.8 / 12.x)**:
```bash
# CUDA 11.8 wheel
pip install "paddlepaddle-gpu>=3.0.0" \
    -i https://www.paddlepaddle.org.cn/packages/stable/cu118/

# CUDA 12.x wheel (NVIDIA 드라이버가 12 대응이면 권장)
pip install "paddlepaddle-gpu>=3.0.0" \
    -i https://www.paddlepaddle.org.cn/packages/stable/cu126/
```

**Python 3.10 / 3.11 (PaddlePaddle 2.6, CUDA 11.8 예시)**:
```bash
pip install paddlepaddle-gpu==2.6.1.post118 \
    -f https://www.paddlepaddle.org.cn/whl/linux/mkl/avx/stable.html
```

- 자세한 wheel 매트릭스: <https://www.paddlepaddle.org.cn/install/quick>
- **시스템 영향 없음** 검증: 설치 후 `find venv/lib -name 'libcudnn*' 2>/dev/null` 에 결과가 잡히면 venv 안에 자체 번들된 증거.

#### 설치 검증

```bash
python -c "import torch; print('torch:', torch.__version__, 'cuda:', torch.cuda.is_available())"
python -c "import paddle; print('paddle:', paddle.__version__); paddle.utils.run_check()"
```

`cuda: True` 가 나오면 GPU 인식됨.

---

## 4. 테스트 사진 준비

`samples/` 폴더에 **여러 알약을 한 장에 담은 사진** 을 1~여러 장 넣어주세요. 촬영 가이드는 [samples/README.md](samples/README.md).

처음에는 **흰 종이 위에 약 2~3알** 만 올려서 정면 촬영하는 것으로 시작하면 검출 성공률 가장 높음.

---

## 5. 실행

```bash
# venv 활성화 상태에서
cd ~/MediBridge/TrainingServer/check_img
source venv/bin/activate    # 새 터미널이면 다시 활성화

# 단일 이미지
python detect_pills.py samples/your_photo.jpg

# 폴더 일괄
python detect_pills.py samples/

# 결과 폴더 변경
python detect_pills.py samples/ --out my_results/
```

> 🐌 **첫 실행 시 PaddleOCR 가 모델 가중치 자동 다운로드** (~수십 MB, 1회만). 약간 느리지만 두 번째부터는 빠름.

### 출력 예시

```
[two_pills.jpg] 검출 알약 수: 2
  # 0  bbox=(120, 88, 156, 162)  색=흰색(0.78)  모양=원형(circ 0.91, elong 1.05)  각인=[TYL(0.92) | 500(0.81)]
  # 1  bbox=(312, 102, 198, 86)  색=주황(0.66)  모양=캡슐형(circ 0.41, elong 2.81)  각인=[BAY(0.74)]
  → 저장: two_pills_annotated.jpg, two_pills_result.json

=== 전체 처리 ===
이미지 수: 1
총 검출 알약 수: 2
요약: results/_summary.json
```

`results/` 폴더에 다음이 생깁니다:

| 파일 | 내용 |
| --- | --- |
| `<image>_annotated.jpg` | 알약마다 초록색 박스 + 번호가 그려진 디버그 이미지 |
| `<image>_pillNN.png` | 각 알약을 crop 한 이미지 (OCR/색/모양 입력) |
| `<image>_result.json` | 알약별 좌표·색·모양·OCR 결과 JSON |
| `_summary.json` | 전체 이미지의 요약 |

---

## 5.A AI Hub 데이터셋 정량 평가 (`eval_aihub.py`)

직접 사진 찍어 검증하는 §5 와 별도로, **AI Hub 경구약제 데이터셋의 GT (Ground Truth) 와 비교** 하는 자동 평가가 가능합니다.

### 5.A.1 데이터셋 다운로드

1. AI Hub 계정 + 사용 신청: <https://aihub.or.kr/aihubdata/data/view.do?currMenu=115&topMenu=100&dataSetSn=576>
2. 전체 ~수백 GB → **빠른 검증 시 일부만 받아도 OK** (예: 단일 약 1,000장)
3. 받은 후 폴더 구조 (예시):
   ```
   AIHubData/
   ├── images/                    ← PNG 파일들 (약별 하위 폴더 가능)
   │   ├── 198400001/
   │   │   ├── 0001.png
   │   │   └── ...
   │   └── ...
   └── labels/
       └── annotation.json        ← COCO-like 어노테이션
   ```

### 5.A.2 실행

```bash
# 빠른 검증 — 처음 50장만
python eval_aihub.py \
    --json   /path/AIHubData/labels/annotation.json \
    --images /path/AIHubData/images \
    --limit  50

# 전체
python eval_aihub.py \
    --json   /path/AIHubData/labels/annotation.json \
    --images /path/AIHubData/images
```

### 5.A.3 출력 — 정량 메트릭

```
============================================================
AI Hub 평가 결과 — 이미지 50장 / GT 박스 73개
============================================================
  검출 IoU≥0.5  :  88.4%   (65 / 73)
  검출 IoU≥0.7  :  76.7%
  검출 IoU≥0.8  :  64.4%    ← 가이드라인 합격선
  검출 IoU≥0.9  :  41.1%
  박스 수 일치  :  82.0%   (이미지 단위)

  OCR exact     :  18.2%   (대상 50)
  OCR substring :  54.5%
  색 일치       :  72.0%   (대상 50)
  모양 일치     :  81.0%   (대상 50)
============================================================
```

### 5.A.4 메트릭 의미

| 메트릭 | 의미 | 합격선 |
| --- | --- | --- |
| **IoU≥0.8** | 가이드라인 §4.2 의 검출 정답 기준 | **70%+** 면 OpenCV 검출 충분 (배경에 따라 변동 큼) |
| 박스 수 일치 | 이미지 단위로 GT 박스 수와 예측 수가 같음 | **80%+** 권장 |
| OCR exact | 정규화 후 완전 일치 | **20%+** 면 PaddleOCR 사전학습으로 충분 |
| OCR substring | 한쪽이 다른 쪽 포함 (부분 인식 OK) | **50%+** 권장 |
| 색 일치 | 식약처 카테고리(흰색/노란색/...) 일치 | **70%+** 권장 |
| 모양 일치 | 식약처 카테고리(원형/타원형/...) 일치 | **75%+** 권장 |

> ⚠ 본 합격선은 **POC 통과 여부 판단용** 휴리스틱입니다. 운영 단계엔 YOLO fine-tuning 으로 IoU 90%+ 가 정상이고, OCR 도 fine-tuning 으로 60%+ 도달이 목표.

### 5.A.5 결과 파일

| 파일 | 내용 |
| --- | --- |
| `eval_results/_summary.json` | 전체 합산 메트릭 |
| `eval_results/_detail.json` | 이미지별 상세 (각 박스 매칭 결과·OCR·색·모양) |

분석 활용:
```bash
# 검출 잘 안 된 이미지 추출
python -c "import json; d=json.load(open('eval_results/_detail.json'));
print('\n'.join(r['file_name'] for r in d if r['gt_box_count']>r['matched']))"
```

---

## 6. 검증 기준 — POC 통과 / 실패

| 결과 | 의미 | 다음 단계 |
| --- | --- | --- |
| ✅ 검출 ≥ 실제 알약 수의 80% & 각인 OCR confidence ≥ 0.7 ≥ 50% | **파이프라인 가능** | 본 시스템 (MainServer/InferenceServer) 진행 OK. 운영 단계엔 YOLO fine-tuning + 식약처 DB 매칭 추가 |
| 🟡 검출 OK, OCR 약함 | 검출은 OK, 각인은 fine-tuning 필요 | 검출 결과를 색·모양만 활용. OCR 은 후속 작업 |
| ❌ 검출 자체 안 됨 | 배경이 너무 복잡 또는 알약 너무 작음 | 단순 배경 사진으로 다시 시도 → 그래도 안 되면 YOLO 학습 데이터 확보 필요 |

---

## 7. 트러블슈팅

| 증상 | 원인 / 해결 |
| --- | --- |
| `ModuleNotFoundError: No module named 'paddle'` | venv 미활성화 또는 paddlepaddle 미설치. §3.3 또는 §3.4 |
| `import cv2` 시 `libGL.so.1: cannot open shared object` | 시스템 패키지 누락. `sudo apt install -y libgl1` |
| 검출 알약 수가 실제보다 적음 | 알약 사이가 너무 가까워 윤곽선이 합쳐짐. 사진에서 알약을 더 띄워 다시 촬영 |
| 검출 알약 수가 실제보다 많음 | 그림자/배경 잡음을 알약으로 오인. `lib/detection.py` 의 `MIN_AREA_RATIO` 를 높여 작은 노이즈 제외 |
| OCR 결과 빈 문자열만 나옴 | 각인이 너무 작거나 흐릿. 카메라 더 가깝게·초점 맞춰 다시 촬영. crop 이미지(`results/*_pillNN.png`)를 직접 확인 |
| 색 분류가 항상 "기타" | 조명이 너무 어둡거나 V(밝기) 가 낮음. 조명 개선 + `lib/color.py` 의 HSV 임계값 조정 |
| `paddle.utils.run_check()` 실패 (GPU 모드) | CUDA 버전 불일치. `nvidia-smi` 로 시스템 CUDA 버전 확인 후 §3.4 의 wheel 재설치 |
| 첫 실행 시 한참 멈춤 | PaddleOCR 모델 다운로드 중. 인터넷 연결 + 5분 정도 대기 |
| Python 3.12 에서 `pip install paddlepaddle` → 휠 못 찾음 | 2.6.x 가 3.12 미지원. `pip install "paddlepaddle>=3.0.0"` 으로 대체 |
| Python 3.12 + `numpy` 충돌 (`A module ... incompatible`) | requirements.txt 의 `numpy<2.0` 이 일부 신버전 패키지와 충돌 가능. 그때만 `pip install --upgrade numpy` 로 덮어쓰기 |
| `imgaug` / `shapely` 휠 못 찾음 (Python 3.12) | `pip install --upgrade imgaug shapely` 로 최신 휠 설치. 그래도 실패 시 `pip install --no-build-isolation imgaug` |

---

## 8. 팀원 분담 — 독립 검증

3명이 각자 다른 사진으로 검증해서 **공통적으로 통과** 하는지 확인하면 신뢰도 ↑:

| 팀원 | 사진 종류 | 검증 포인트 |
| --- | --- | --- |
| 1 | 시중 시판약 (정·캡슐 혼합) | 다양한 모양 분류 정확도 |
| 2 | 같은 색 다양한 약 (모두 흰색 등) | 모양·각인으로 구분되는가 |
| 3 | 어려운 케이스 (작은 약, 배경 복잡) | 검출 한계점 파악 |

각자 `results/_summary.json` 를 공유 → 통과율 합산 → 본 시스템 진행 결정.

---

## 9. 결과를 본 시스템과 연결하려면 (POC 통과 후)

1. `lib/detection.py` 의 OpenCV 검출 → `Services/Inference/VisionInferenceClient` 의 입력 형식과 매핑
2. `lib/ocr.py` 의 PaddleOCR 결과 → 식약처 낱알식별 검색 키 (engraving_front)
3. `lib/color.py` / `lib/shape.py` 의 라벨 → 식약처 검색 키 (color_front, shape)
4. **YOLO fine-tuning** 도입 단계: AI Hub 경구약제 데이터셋 다운로드 → ultralytics 로 학습 → 본 폴더의 OpenCV 검출을 `ultralytics.YOLO` 추론으로 교체

본 POC 는 시스템과 분리된 검증용 — 통과해도 본 시스템에 자동 통합 X. 통과 결과를 공유 후 별도 작업으로 통합.

---

## 10. 폴더 구조

```
check_img/
├── README.md             ← 본 문서
├── requirements.txt      ← Python 의존성
├── .gitignore            ← venv·결과·샘플 사진 추적 제외
├── detect_pills.py       ← 메인 실행 스크립트 (직접 찍은 사진)
├── eval_aihub.py         ← AI Hub 데이터셋 정량 평가 (§5.A)
├── lib/
│   ├── __init__.py
│   ├── detection.py      ← OpenCV 알약 검출
│   ├── ocr.py            ← PaddleOCR 각인 인식
│   ├── color.py          ← HSV 색 분석
│   ├── shape.py          ← 윤곽선 모양 분석
│   └── aihub_loader.py   ← AI Hub COCO-like JSON 로더 + IoU 매칭
├── samples/              ← 테스트 사진 (gitignore — 본인이 채움)
│   └── README.md
├── results/              ← detect_pills.py 출력 (gitignore)
│   ├── <image>_annotated.jpg
│   ├── <image>_pillNN.png
│   ├── <image>_result.json
│   └── _summary.json
└── eval_results/         ← eval_aihub.py 출력 (gitignore)
    ├── _summary.json
    └── _detail.json
```
