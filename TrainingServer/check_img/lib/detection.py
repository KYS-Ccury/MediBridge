"""알약 검출 — OpenCV 윤곽선 기반 (학습 불필요).

POC 가정: 알약을 단색 배경(흰 종이·접시·검은 매트 등)에 놓고 촬영.
복잡한 배경에서는 YOLO fine-tuning 이 필요하지만 본 단계에선 검증 X.

⭐ v2 개선:
  - 양방향 이진화 — '어두운 약 + 밝은 배경' / '밝은 약 + 어두운 배경'
    둘 다 시도해서 결과가 좋은 쪽 선택 (또는 합집합)
  - 각인이 있는 약의 solidity 가 낮아지는 점 고려 → 임계 0.85→0.75 완화
  - HoughCircles fallback — 윤곽선 검출 실패 시 원형 약 보조 검출
  - debug=True 시 이진화 마스크 + 윤곽선 시각화 반환
"""

from __future__ import annotations
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

import cv2
import numpy as np


@dataclass
class PillBox:
    """검출된 알약 1개의 위치·크기 정보."""
    index:    int
    bbox:     Tuple[int, int, int, int]   # (x, y, w, h) 외접 사각형
    center:   Tuple[int, int]
    area:     float
    contour:  np.ndarray


# 검출 파라미터
MIN_AREA_RATIO   = 0.0005   # 이미지 면적 대비 (≈0.05%) — 작은 알약 허용
MAX_AREA_RATIO   = 0.40     # ≈40%
ASPECT_RATIO_RANGE = (0.25, 4.0)
MIN_SOLIDITY     = 0.75     # 각인 있는 약의 오목 부분 허용


def _binarize(gray: np.ndarray, invert: bool) -> np.ndarray:
    """Otsu 이진화 — 두 방향 중 하나."""
    flag = cv2.THRESH_BINARY_INV if invert else cv2.THRESH_BINARY
    _, bw = cv2.threshold(gray, 0, 255, flag + cv2.THRESH_OTSU)
    return bw


def _filter_contours(contours, img_area: float,
                     img_w: int, img_h: int) -> List[np.ndarray]:
    """면적·종횡비·solidity·경계닿음 으로 알약 후보 추림."""
    out = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        ratio = area / img_area
        if ratio < MIN_AREA_RATIO or ratio > MAX_AREA_RATIO:
            continue
        x, y, bw, bh = cv2.boundingRect(cnt)
        aspect = bw / max(bh, 1)
        if aspect < ASPECT_RATIO_RANGE[0] or aspect > ASPECT_RATIO_RANGE[1]:
            continue
        hull = cv2.convexHull(cnt)
        ha = cv2.contourArea(hull)
        if ha > 0 and area / ha < MIN_SOLIDITY:
            continue

        # 이미지 경계 닿은 거대 contour 제거 — 검은 테두리·프레임 false-positive 방지.
        # 박스 4 변 중 2 변 이상이 경계에 닿고 + 면적 12% 이상이면 제외.
        edge_touch = 0
        if x <= 1:                edge_touch += 1
        if y <= 1:                edge_touch += 1
        if x + bw >= img_w - 1:   edge_touch += 1
        if y + bh >= img_h - 1:   edge_touch += 1
        if edge_touch >= 2 and ratio > 0.12:
            continue

        out.append(cnt)
    return out


def _contours_from(image_bgr: np.ndarray, invert: bool) -> List[np.ndarray]:
    """그레이스케일 Otsu 이진화 기반 검출."""
    h, w = image_bgr.shape[:2]
    gray = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    bw = _binarize(blurred, invert=invert)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    cleaned = cv2.morphologyEx(bw, cv2.MORPH_OPEN,  kernel, iterations=2)
    cleaned = cv2.morphologyEx(cleaned, cv2.MORPH_CLOSE, kernel, iterations=1)
    contours, _ = cv2.findContours(cleaned, cv2.RETR_EXTERNAL,
                                   cv2.CHAIN_APPROX_SIMPLE)
    return _filter_contours(contours, float(h * w), w, h)


def _contours_from_saturation(image_bgr: np.ndarray) -> List[np.ndarray]:
    """HSV S 채널 기반 — 컬러 알약 ↔ 무채색/유사 명도 배경 분리.

    예: 빨간 알약 + 크림 배경 / 주황 알약 + 푸른 회색 배경.
    그레이스케일 Otsu 가 실패하는 케이스에 보강.
    """
    h, w = image_bgr.shape[:2]
    hsv = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2HSV)
    s = hsv[:, :, 1]
    blurred = cv2.GaussianBlur(s, (5, 5), 0)
    # Otsu 로 채도 임계 자동 결정. 알약이 더 채도 높다고 가정 (정상 케이스).
    _, bw = cv2.threshold(blurred, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    cleaned = cv2.morphologyEx(bw, cv2.MORPH_OPEN,  kernel, iterations=2)
    cleaned = cv2.morphologyEx(cleaned, cv2.MORPH_CLOSE, kernel, iterations=1)
    contours, _ = cv2.findContours(cleaned, cv2.RETR_EXTERNAL,
                                   cv2.CHAIN_APPROX_SIMPLE)
    return _filter_contours(contours, float(h * w), w, h)


def _estimate_bg_lab(image_bgr: np.ndarray) -> np.ndarray:
    """모서리 4 영역에서 배경색 robust 추정 — median of corner medians.

    한 코너에 알약이 걸쳐 있어도 나머지 3 코너의 median 으로 안정 동작.
    """
    h, w = image_bgr.shape[:2]
    cs = max(6, min(h, w) // 40)        # 더 작은 corner — 알약 침범 위험 감소
    corners = [
        image_bgr[:cs, :cs],
        image_bgr[:cs, -cs:],
        image_bgr[-cs:, :cs],
        image_bgr[-cs:, -cs:],
    ]
    corner_medians = []
    for c in corners:
        if c.size == 0:
            continue
        c_lab = cv2.cvtColor(c, cv2.COLOR_BGR2LAB).astype(np.float32)
        corner_medians.append(np.median(c_lab.reshape(-1, 3), axis=0))
    if not corner_medians:
        return np.array([128.0, 128.0, 128.0], dtype=np.float32)
    return np.median(np.stack(corner_medians, axis=0), axis=0)


def _contours_from_chroma(image_bgr: np.ndarray) -> List[np.ndarray]:
    """LAB 색도(chromaticity = sqrt((a-128)² + (b-128)²)) 기반 검출.

    명도 L 무시하고 색도만 봄. 알약이 무채색이 아닌 한 (흰·회·검 제외)
    중성 배경(밝은 회색·연한 파랑 등) 과 강하게 분리됨.

    saturation 보다 LAB 가 인지적으로 더 균일하고 작은 채도 차이도 잘 잡음.
    Saturation 트랙이 놓치는 케이스(주황 알약 + 옅은 푸른 회색 배경 등) 보강.
    """
    h, w = image_bgr.shape[:2]
    img_area = float(h * w)

    lab = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2LAB).astype(np.int16)
    a = lab[:, :, 1] - 128
    b = lab[:, :, 2] - 128
    chroma = np.sqrt(a * a + b * b).astype(np.float32)
    if chroma.max() <= 0:
        return []
    chroma8 = (chroma / chroma.max() * 255).astype(np.uint8)

    blurred = cv2.GaussianBlur(chroma8, (5, 5), 0)
    _, bw = cv2.threshold(blurred, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    cleaned = cv2.morphologyEx(bw, cv2.MORPH_OPEN,  kernel, iterations=1)
    cleaned = cv2.morphologyEx(cleaned, cv2.MORPH_CLOSE, kernel, iterations=1)

    contours, _ = cv2.findContours(cleaned, cv2.RETR_EXTERNAL,
                                   cv2.CHAIN_APPROX_SIMPLE)
    return _filter_contours(contours, img_area, w, h)


def _contours_from_bg_distance(image_bgr: np.ndarray) -> List[np.ndarray]:
    """배경색 거리 기반 — 투톤 캡슐·무채색 알약·복잡 케이스 보강.

    모서리 4 영역에서 robust 배경색 추정 → 각 픽셀의 **LAB 색거리** 마스크.
    명도(그레이스케일) 와 채도(HSV S) 모두 비슷해도 색 자체가 다르면 분리됨.

    + 적당한 morphological close 로 캡슐 두 절반 통합
    + close 후 erode 1px 로 가장자리 halo 타이트하게 정리 (LC 500 같은 케이스)
    """
    h, w = image_bgr.shape[:2]
    img_area = float(h * w)

    bg_lab = _estimate_bg_lab(image_bgr)
    lab_image = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2LAB).astype(np.float32)
    diff = lab_image - bg_lab
    dist = np.sqrt(np.sum(diff * diff, axis=2))
    if dist.max() <= 0:
        return []
    dist8 = (dist / dist.max() * 255).astype(np.uint8)

    blurred = cv2.GaussianBlur(dist8, (5, 5), 0)
    _, bw = cv2.threshold(blurred, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

    # 스케일 적응 close — 너무 크면 인접 알약 합쳐짐, 너무 작으면 캡슐 양쪽 못 잇음.
    # 짧은 변의 1.0% — 이전 1.5% 보다 좀 더 보수적.
    k = max(7, int(min(h, w) * 0.010))
    if k % 2 == 0: k += 1
    big_kernel   = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (k, k))
    small_kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))

    cleaned = cv2.morphologyEx(bw,      cv2.MORPH_CLOSE, big_kernel,   iterations=1)
    cleaned = cv2.morphologyEx(cleaned, cv2.MORPH_OPEN,  small_kernel, iterations=1)
    # close 후 가장자리 halo·그라디언트 타이트 정리 — bbox 과대 추정 방지.
    erode_kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    cleaned = cv2.erode(cleaned, erode_kernel, iterations=1)

    contours, _ = cv2.findContours(cleaned, cv2.RETR_EXTERNAL,
                                   cv2.CHAIN_APPROX_SIMPLE)
    return _filter_contours(contours, img_area, w, h)


def _hough_circles_v2(image_bgr: np.ndarray) -> List[np.ndarray]:
    """원형 알약 보조 검출 — fallback. 검출 결과를 가짜 contour 로 변환."""
    h, w = image_bgr.shape[:2]
    img_area = float(h * w)
    return _hough_circles(image_bgr, img_area)


def _hough_circles(image_bgr: np.ndarray, img_area: float) -> List[np.ndarray]:
    """원형 알약 보조 검출 — fallback. 검출 결과를 가짜 contour 로 변환."""
    gray = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2GRAY)
    blurred = cv2.medianBlur(gray, 5)
    h, w = gray.shape
    min_dim = min(h, w)
    circles = cv2.HoughCircles(
        blurred, cv2.HOUGH_GRADIENT, dp=1.2,
        minDist=int(min_dim * 0.08),
        param1=120, param2=30,
        minRadius=int(min_dim * 0.03),
        maxRadius=int(min_dim * 0.30),
    )
    out: List[np.ndarray] = []
    if circles is None:
        return out
    for cx, cy, r in circles[0]:
        cx, cy, r = float(cx), float(cy), float(r)
        # 원형 contour 근사 — 36 점 polygon
        pts = []
        for i in range(36):
            ang = 2 * np.pi * i / 36
            pts.append([[cx + r * np.cos(ang), cy + r * np.sin(ang)]])
        cnt = np.array(pts, dtype=np.int32)
        area = cv2.contourArea(cnt)
        if area / img_area < MIN_AREA_RATIO or area / img_area > MAX_AREA_RATIO:
            continue
        out.append(cnt)
    return out


def _box_iou(a: Tuple[int, int, int, int], b: Tuple[int, int, int, int]) -> float:
    ax1, ay1, aw, ah = a; bx1, by1, bw, bh = b
    ax2, ay2 = ax1 + aw, ay1 + ah
    bx2, by2 = bx1 + bw, by1 + bh
    ix1 = max(ax1, bx1); iy1 = max(ay1, by1)
    ix2 = min(ax2, bx2); iy2 = min(ay2, by2)
    iw = max(0, ix2 - ix1); ih = max(0, iy2 - iy1)
    inter = iw * ih
    union = aw * ah + bw * bh - inter
    return inter / union if union > 0 else 0.0


def _to_pills(contours: List[np.ndarray]) -> List[PillBox]:
    pills = []
    for cnt in contours:
        x, y, bw, bh = cv2.boundingRect(cnt)
        m = cv2.moments(cnt)
        if m["m00"] == 0:
            continue
        cx = int(m["m10"] / m["m00"])
        cy = int(m["m01"] / m["m00"])
        pills.append(PillBox(
            index=len(pills),
            bbox=(x, y, bw, bh),
            center=(cx, cy),
            area=float(cv2.contourArea(cnt)),
            contour=cnt,
        ))
    return pills


def _dedupe_pills(pills: List[PillBox], iou_threshold: float = 0.5) -> List[PillBox]:
    """단순 IoU 중복 제거 — 작은 박스 우선 (타이트한 검출 보존)."""
    sorted_by_area = sorted(pills, key=lambda p: p.area)
    kept: List[PillBox] = []
    for p in sorted_by_area:
        dup = False
        for q in kept:
            if _box_iou(p.bbox, q.bbox) >= iou_threshold:
                dup = True; break
        if not dup:
            kept.append(p)
    for i, p in enumerate(kept):
        p.index = i
    return kept


def _consolidate_detections(pills: List[PillBox]) -> List[PillBox]:
    """그림자로 과대 추정된 박스 vs 캡슐 양쪽 통합 박스를 구분해 정리.

    핵심 알고리즘:
      큰 박스 X 안에 작은 박스들 S1, S2, ... 가 ≥80% 포함되어 있을 때:
        - S1, S2, ... 가 서로 안 겹치면 (disjoint) X 는 투톤 캡슐 통합 → X 보존, Si 제거
        - S1, S2, ... 가 서로 겹치면 (같은 약 중복 검출) X 는 그림자로 과대 추정
          → X 제거, Si 보존
      X 안에 작은 박스 1개만 있을 때:
        - X 의 추가 면적이 < 40%: X 가 살짝 과대 추정 → X 제거
        - 추가 면적이 ≥ 40%: X 가 진짜 큰 약 → 작은 박스 제거
    """
    if not pills:
        return []

    pills_desc = sorted(pills, key=lambda p: p.area, reverse=True)
    drop = [False] * len(pills_desc)

    for i in range(len(pills_desc)):
        if drop[i]:
            continue
        big = pills_desc[i]
        bx, by, bw, bh = big.bbox
        big_area = bw * bh
        if big_area <= 0:
            drop[i] = True; continue

        contained_idxs: List[int] = []
        for j in range(i + 1, len(pills_desc)):
            if drop[j]:
                continue
            small = pills_desc[j]
            ax, ay, aw, ah = small.bbox
            small_area = aw * ah
            if small_area <= 0:
                continue
            ix1 = max(ax, bx); iy1 = max(ay, by)
            ix2 = min(ax + aw, bx + bw); iy2 = min(ay + ah, by + bh)
            iw = max(0, ix2 - ix1); ih = max(0, iy2 - iy1)
            inter = iw * ih
            if inter / small_area >= 0.8:
                contained_idxs.append(j)

        if len(contained_idxs) >= 2:
            disjoint = False
            for k1 in range(len(contained_idxs)):
                for k2 in range(k1 + 1, len(contained_idxs)):
                    p1 = pills_desc[contained_idxs[k1]]
                    p2 = pills_desc[contained_idxs[k2]]
                    if _box_iou(p1.bbox, p2.bbox) < 0.2:
                        disjoint = True; break
                if disjoint:
                    break
            if disjoint:
                for j in contained_idxs:
                    drop[j] = True
            else:
                drop[i] = True
        elif len(contained_idxs) == 1:
            j = contained_idxs[0]
            small = pills_desc[j]
            small_area = small.bbox[2] * small.bbox[3]
            extra_ratio = (big_area - small_area) / big_area
            if extra_ratio < 0.40:
                drop[i] = True
            else:
                drop[j] = True

    survivors = [p for k, p in enumerate(pills_desc) if not drop[k]]

    # 마지막 IoU 안전망
    final: List[PillBox] = []
    for p in sorted(survivors, key=lambda p: p.area):
        if not any(_box_iou(p.bbox, q.bbox) >= 0.5 for q in final):
            final.append(p)

    for k, p in enumerate(final):
        p.index = k
    return final


def detect_pills(image_bgr: np.ndarray,
                 debug: bool = False) -> List[PillBox]:
    """이미지 입력 → 알약 후보 박스 리스트.

    검출 전략 (5-방향 합집합 + 중복 제거):
      1) 그레이 Otsu (INV/Normal) — 어두운 약 / 밝은 약
      2) HSV 채도 Otsu — 컬러 약 + 무채색/유사 명도 배경
      3) LAB 색도 (a-b 거리) Otsu — 명도 무시, 무채색 배경에서 컬러 약 분리
      4) LAB 배경거리 + 큰 close — 투톤 캡슐 통합 검출
      5) 합집합 후 IoU/contained 중복 제거
      6) 0개면 HoughCircles fallback
    """
    if image_bgr is None or image_bgr.size == 0:
        return []

    contours_a = _contours_from(image_bgr, invert=True)
    contours_b = _contours_from(image_bgr, invert=False)
    contours_s = _contours_from_saturation(image_bgr)
    contours_c = _contours_from_chroma(image_bgr)
    contours_d = _contours_from_bg_distance(image_bgr)

    pills_d = _to_pills(contours_d)
    pills_c = _to_pills(contours_c)
    pills_s = _to_pills(contours_s)
    pills_a = _to_pills(contours_a)
    pills_b = _to_pills(contours_b)

    merged = pills_d + pills_c + pills_s + pills_a + pills_b
    merged = _consolidate_detections(merged)

    if not merged:
        merged = _to_pills(_hough_circles_v2(image_bgr))
        merged = _consolidate_detections(merged)

    if debug:
        _save_debug_artifacts(image_bgr, contours_a, contours_b,
                              contours_s, contours_c, contours_d, merged)

    return merged


def _drop_contained(pills: List[PillBox]) -> List[PillBox]:
    """[deprecated] _consolidate_detections 로 대체됨. 호환을 위해 보존."""
    return _consolidate_detections(pills)


# =====================================================
# 디버그 마스크 시각화 (debug=True 시)
# =====================================================
_DEBUG_OUT_DIR: Optional[str] = None
_DEBUG_BASENAME: str = "img"


def set_debug_output(out_dir: str, basename: str):
    """디버그 출력 위치·파일명 prefix 지정."""
    global _DEBUG_OUT_DIR, _DEBUG_BASENAME
    _DEBUG_OUT_DIR = out_dir
    _DEBUG_BASENAME = basename


def _save_debug_artifacts(image_bgr, contours_a, contours_b,
                          contours_s, contours_c, contours_d, pills):
    if not _DEBUG_OUT_DIR:
        return
    import os
    os.makedirs(_DEBUG_OUT_DIR, exist_ok=True)
    h, w = image_bgr.shape[:2]
    gray = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    bw_inv = _binarize(blurred, invert=True)
    bw_nor = _binarize(blurred, invert=False)

    hsv = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2HSV)
    s_blur = cv2.GaussianBlur(hsv[:, :, 1], (5, 5), 0)
    _, bw_sat = cv2.threshold(s_blur, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

    # bg-distance mask
    cs = max(8, min(h, w) // 30)
    corners = [image_bgr[:cs, :cs], image_bgr[:cs, -cs:],
               image_bgr[-cs:, :cs], image_bgr[-cs:, -cs:]]
    lab_image = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2LAB).astype(np.float32)
    corner_pixels = np.concatenate([
        cv2.cvtColor(c, cv2.COLOR_BGR2LAB).astype(np.float32).reshape(-1, 3)
        for c in corners])
    bg_lab = np.median(corner_pixels, axis=0)
    diff = lab_image - bg_lab
    dist = np.sqrt(np.sum(diff * diff, axis=2))
    if dist.max() > 0:
        dist8 = (dist / dist.max() * 255).astype(np.uint8)
        bw_bg = cv2.threshold(dist8, 0, 255,
                              cv2.THRESH_BINARY + cv2.THRESH_OTSU)[1]
    else:
        bw_bg = np.zeros((h, w), dtype=np.uint8)

    # LAB chroma mask
    lab_int = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2LAB).astype(np.int16)
    aa = lab_int[:, :, 1] - 128
    bb = lab_int[:, :, 2] - 128
    chroma = np.sqrt(aa * aa + bb * bb).astype(np.float32)
    if chroma.max() > 0:
        chroma8 = (chroma / chroma.max() * 255).astype(np.uint8)
        bw_chr = cv2.threshold(chroma8, 0, 255,
                               cv2.THRESH_BINARY + cv2.THRESH_OTSU)[1]
    else:
        bw_chr = np.zeros((h, w), dtype=np.uint8)

    base = os.path.join(_DEBUG_OUT_DIR, _DEBUG_BASENAME)
    cv2.imwrite(f"{base}_dbg_thresh_inv.png",    bw_inv)
    cv2.imwrite(f"{base}_dbg_thresh_normal.png", bw_nor)
    cv2.imwrite(f"{base}_dbg_saturation.png",    bw_sat)
    cv2.imwrite(f"{base}_dbg_chroma.png",        bw_chr)
    cv2.imwrite(f"{base}_dbg_bg_distance.png",   bw_bg)

    overlay = image_bgr.copy()
    for cnt in contours_a:
        cv2.drawContours(overlay, [cnt], -1, (0, 0, 255),   2)   # red     — gray INV
    for cnt in contours_b:
        cv2.drawContours(overlay, [cnt], -1, (255, 0, 0),   2)   # blue    — gray Normal
    for cnt in contours_s:
        cv2.drawContours(overlay, [cnt], -1, (0, 255, 255), 2)   # yellow  — saturation
    for cnt in contours_c:
        cv2.drawContours(overlay, [cnt], -1, (255, 0, 255), 2)   # magenta — chroma
    for cnt in contours_d:
        cv2.drawContours(overlay, [cnt], -1, (0, 255, 0),   2)   # green   — bg-distance
    cv2.imwrite(f"{base}_dbg_contours.png", overlay)


# =====================================================
# 사용자 함수
# =====================================================
def annotate(image_bgr: np.ndarray, pills: List[PillBox]) -> np.ndarray:
    """검출 결과를 이미지에 그려 반환 (디버그용)."""
    out = image_bgr.copy()
    for p in pills:
        x, y, w, h = p.bbox
        cv2.rectangle(out, (x, y), (x + w, y + h), (0, 255, 0), 2)
        cv2.putText(out, f"#{p.index}", (x, max(y - 6, 12)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
    return out


def crop_pill(image_bgr: np.ndarray, pill: PillBox, pad: int = 6) -> np.ndarray:
    """알약 외접 사각형 + 약간의 padding 으로 crop."""
    x, y, w, h = pill.bbox
    H, W = image_bgr.shape[:2]
    x0 = max(0, x - pad); y0 = max(0, y - pad)
    x1 = min(W, x + w + pad); y1 = min(H, y + h + pad)
    return image_bgr[y0:y1, x0:x1]
