#!/usr/bin/env python3
"""식약처 공공데이터 CSV/Excel → MariaDB 일괄 적재.

POC 단계 — 메인서버 운영 중에는 호출되지 않음. 식약처에서 받아온 CSV 를
주기적으로(예: 월 1회) 본 스크립트로 import 해 DB 를 풍부하게 만든다.

식약처 OpenAPI 응답 또는 공공데이터포털 CSV 다운로드 둘 다 지원하도록
컬럼명 정규화(대소문자·공백 무시 + alias 매핑) 후 UPSERT 한다.

지원 대상 (DB ERD v4):
    pill_identification   — 낱알식별 (의약품 외관)
    dur_interaction_cache — DUR 병용금기
    drug_overview         — e약은요 (효능·사용법·주의·부작용 등)
    ingredient_info       — 성분 정보

사용:
    python3 import_pdma.py pill        <csv_path>
    python3 import_pdma.py dur         <csv_path>
    python3 import_pdma.py overview    <csv_path>
    python3 import_pdma.py ingredient  <csv_path>
    python3 import_pdma.py all <pill.csv> <dur.csv> <overview.csv> [<ingredient.csv>]

환경변수 (smoke 환경 기본값):
    MEDIBRIDGE_DB_HOST=127.0.0.1
    MEDIBRIDGE_DB_PORT=3306
    MEDIBRIDGE_DB_USER=medibridge_app
    MEDIBRIDGE_DB_PASSWORD=smoke_pw_change_me
    MEDIBRIDGE_DB_NAME=medibridge

종료 코드:
    0  성공
    1  파일 미존재 또는 인자 오류
    2  DB 연결 실패
    3  CSV 컬럼 매핑 실패 (필수 컬럼 누락)
"""
from __future__ import annotations

import argparse
import csv
import os
import sys
from pathlib import Path
from typing import Callable, Dict, Iterable, List, Optional, Tuple

# ----- DB 드라이버 (pymysql / mysql-connector 둘 다 지원) -----
_DB = None
try:
    import pymysql                               # 의존성 가장 가벼움
    _DB = "pymysql"
except ImportError:
    try:
        import mysql.connector as mysql_conn      # noqa: F401
        _DB = "mysql.connector"
    except ImportError:
        _DB = None


# =====================================================
# 컬럼 매핑 — 식약처 표준 + alias
# =====================================================
# key  : DB 컬럼명
# value: 식약처 CSV 의 가능한 컬럼명 목록 (대소문자·공백 무시 매칭)
#
# 식약처 데이터 출처별 컬럼명 차이 (포털 vs OpenAPI vs 엑셀 다운로드) 흡수.
# =====================================================

PILL_COLUMN_MAP: Dict[str, List[str]] = {
    "item_code":           ["ITEM_SEQ", "itemSeq", "품목기준코드", "품목일련번호"],
    "drug_name":           ["ITEM_NAME", "itemName", "품목명", "제품명"],
    "manufacturer":        ["ENTP_NAME", "entpName", "업체명", "제조업체"],
    "shape":               ["DRUG_SHAPE", "drugShape", "모양", "의약품모양"],
    "color_front":         ["COLOR_CLASS1", "colorClass1", "색상", "색상앞", "앞면색상"],
    "color_back":          ["COLOR_CLASS2", "colorClass2", "색상뒤", "뒷면색상"],
    "engraving_front":     ["PRINT_FRONT", "printFront", "표시앞", "앞면표시"],
    "engraving_back":      ["PRINT_BACK", "printBack", "표시뒤", "뒷면표시"],
    "pill_image_url":      ["ITEM_IMAGE", "itemImage", "큰제품이미지", "이미지URL"],
    "classification_no":   ["CLASS_NO", "classNo", "분류번호", "약효분류번호"],
    "classification_name": ["CLASS_NAME", "className", "분류명", "약효분류명"],
}

DUR_COLUMN_MAP: Dict[str, List[str]] = {
    "dur_id":           ["DUR_SEQ", "TYPE_CODE", "DUR_ID", "고유번호"],
    "base_item_code":   ["INGR_CODE", "ITEM_SEQ", "기준품목코드", "성분코드A"],
    "target_item_code": ["MIXTURE_INGR_CODE", "MIXTURE_ITEM_SEQ", "병용품목코드", "성분코드B"],
    "dur_type":         ["TYPE_NAME", "DUR_TYPE", "유형", "DUR유형"],
    "prohibit_reason":  ["PROHBT_CONTENT", "prohbtContent", "금기사유", "병용금기내용"],
}

OVERVIEW_COLUMN_MAP: Dict[str, List[str]] = {
    "item_code":        ["ITEM_SEQ", "itemSeq", "품목기준코드"],
    "efficacy_text":    ["efcyQesitm", "EFCY_QESITM", "효능", "효능효과"],
    "usage_text":       ["useMethodQesitm", "USE_METHOD_QESITM", "사용법", "용법용량"],
    "warning_text":     ["atpnWarnQesitm", "ATPN_WARN_QESITM", "경고", "경고문구"],
    "caution_text":     ["atpnQesitm", "ATPN_QESITM", "주의사항", "사용상주의"],
    "interaction_text": ["intrcQesitm", "INTRC_QESITM", "상호작용"],
    "side_effect_text": ["seQesitm", "SE_QESITM", "부작용", "이상반응"],
    "storage_text":     ["depositMethodQesitm", "DEPOSIT_METHOD_QESITM", "보관법", "저장방법"],
}

INGREDIENT_COLUMN_MAP: Dict[str, List[str]] = {
    "ingredient_code": ["INGR_CODE", "ingrCode", "성분코드"],
    "ingredient_name": ["INGR_NAME", "ingrName", "성분명"],
    "safety_info":     ["SAFETY_INFO", "safetyInfo", "안전정보"],
}


# =====================================================
# 유틸
# =====================================================

def _norm(s: str) -> str:
    """컬럼명 비교용 — 공백·언더스코어 제거 + 소문자."""
    return s.replace(" ", "").replace("_", "").replace("-", "").lower() if s else ""


def _resolve_columns(header: List[str], mapping: Dict[str, List[str]]) -> Dict[str, Optional[int]]:
    """CSV 헤더에서 DB 컬럼별 인덱스 매핑. 못 찾으면 None."""
    norm_header = [_norm(h) for h in header]
    out: Dict[str, Optional[int]] = {}
    for db_col, aliases in mapping.items():
        idx: Optional[int] = None
        for a in aliases:
            na = _norm(a)
            if na in norm_header:
                idx = norm_header.index(na)
                break
        out[db_col] = idx
    return out


def _connect():
    """DB 연결 — pymysql 또는 mysql.connector 자동 분기."""
    host = os.getenv("MEDIBRIDGE_DB_HOST",     "127.0.0.1")
    port = int(os.getenv("MEDIBRIDGE_DB_PORT", "3306"))
    user = os.getenv("MEDIBRIDGE_DB_USER",     "medibridge_app")
    pwd  = os.getenv("MEDIBRIDGE_DB_PASSWORD", "smoke_pw_change_me")
    db   = os.getenv("MEDIBRIDGE_DB_NAME",     "medibridge")

    if _DB is None:
        print("ERROR: pymysql 또는 mysql-connector-python 둘 다 설치되지 않았습니다.\n"
              "  pip install pymysql", file=sys.stderr)
        sys.exit(2)

    print(f"[import] DB 접속: {user}@{host}:{port}/{db} (driver={_DB})")
    if _DB == "pymysql":
        return pymysql.connect(host=host, port=port, user=user, password=pwd,
                               database=db, charset="utf8mb4", autocommit=False)
    return mysql_conn.connect(host=host, port=port, user=user, password=pwd,
                              database=db, charset="utf8mb4", autocommit=False)


def _iter_csv(csv_path: Path) -> Iterable[Tuple[List[str], List[List[str]]]]:
    """CSV 또는 TSV 또는 Excel(.xlsx) 자동 감지 후 (header, rows) yield.

    엑셀은 openpyxl 이 있을 때만 지원.
    """
    suffix = csv_path.suffix.lower()
    if suffix == ".xlsx":
        try:
            from openpyxl import load_workbook
        except ImportError:
            print("ERROR: .xlsx 파일은 openpyxl 필요. pip install openpyxl", file=sys.stderr)
            sys.exit(1)
        wb = load_workbook(filename=str(csv_path), data_only=True, read_only=True)
        ws = wb.active
        rows = [[str(c.value) if c.value is not None else "" for c in row] for row in ws.iter_rows()]
        if not rows:
            return
        yield rows[0], rows[1:]
        return

    # CSV / TSV — encoding 자동: utf-8-sig 우선, 실패 시 cp949 (한국어 데이터)
    for enc in ("utf-8-sig", "utf-8", "cp949", "euc-kr"):
        try:
            with open(csv_path, "r", encoding=enc, newline="") as f:
                # 구분자 추정
                sample = f.read(4096)
                f.seek(0)
                delim = "\t" if sample.count("\t") > sample.count(",") else ","
                reader = csv.reader(f, delimiter=delim)
                rows = list(reader)
                if not rows:
                    return
                yield rows[0], rows[1:]
                return
        except UnicodeDecodeError:
            continue
    print(f"ERROR: 인코딩 자동 감지 실패: {csv_path}", file=sys.stderr)
    sys.exit(1)


def _value(row: List[str], idx: Optional[int]) -> Optional[str]:
    if idx is None or idx >= len(row):
        return None
    v = row[idx].strip()
    return v if v else None


# =====================================================
# 적재 핵심 — UPSERT
# =====================================================

def _upsert(conn, table: str, columns: List[str], values: List[Tuple]):
    """ON DUPLICATE KEY UPDATE 패턴으로 일괄 적재. PK 충돌 시 갱신."""
    if not values:
        return 0
    placeholders = ",".join(["%s"] * len(columns))
    updates = ",".join([f"{c}=VALUES({c})" for c in columns if c != columns[0]])
    sql = (f"INSERT INTO {table} ({','.join(columns)}) VALUES ({placeholders}) "
           f"ON DUPLICATE KEY UPDATE {updates}")
    cur = conn.cursor()
    BATCH = 500
    total = 0
    for i in range(0, len(values), BATCH):
        chunk = values[i:i + BATCH]
        cur.executemany(sql, chunk)
        total += cur.rowcount
        conn.commit()
        print(f"  ... {min(i + BATCH, len(values))}/{len(values)} 행 처리")
    cur.close()
    return total


# =====================================================
# 각 도메인별 import 함수
# =====================================================

def import_pill(conn, csv_path: Path) -> int:
    print(f"[pill] {csv_path}")
    for header, rows in _iter_csv(csv_path):
        idx = _resolve_columns(header, PILL_COLUMN_MAP)
        if idx["item_code"] is None or idx["drug_name"] is None:
            print(f"ERROR: 필수 컬럼(item_code/drug_name) 매핑 실패. 헤더={header}", file=sys.stderr)
            sys.exit(3)
        values: List[Tuple] = []
        for row in rows:
            code = _value(row, idx["item_code"])
            name = _value(row, idx["drug_name"])
            if not code or not name:
                continue
            values.append((
                code, name,
                _value(row, idx["manufacturer"]),
                _value(row, idx["shape"]),
                _value(row, idx["color_front"]),
                _value(row, idx["color_back"]),
                _value(row, idx["engraving_front"]),
                _value(row, idx["engraving_back"]),
                _value(row, idx["pill_image_url"]),
                _value(row, idx["classification_no"]),
                _value(row, idx["classification_name"]),
            ))
        cols = ["item_code", "drug_name", "manufacturer", "shape",
                "color_front", "color_back", "engraving_front", "engraving_back",
                "pill_image_url", "classification_no", "classification_name"]
        n = _upsert(conn, "pill_identification", cols, values)
        print(f"[pill] UPSERT {n} 행 (입력 {len(values)})")
        return n
    return 0


def import_dur(conn, csv_path: Path) -> int:
    print(f"[dur] {csv_path}")
    for header, rows in _iter_csv(csv_path):
        idx = _resolve_columns(header, DUR_COLUMN_MAP)
        if idx["base_item_code"] is None or idx["target_item_code"] is None:
            print(f"ERROR: DUR 필수 컬럼(base/target item_code) 매핑 실패. 헤더={header}", file=sys.stderr)
            sys.exit(3)
        values: List[Tuple] = []
        for i, row in enumerate(rows):
            base   = _value(row, idx["base_item_code"])
            target = _value(row, idx["target_item_code"])
            if not base or not target:
                continue
            dur_id = _value(row, idx["dur_id"]) or f"dur_{base}_{target}"
            values.append((
                dur_id, base, target,
                _value(row, idx["dur_type"]) or "병용금기",
                _value(row, idx["prohibit_reason"]) or "",
            ))
        cols = ["dur_id", "base_item_code", "target_item_code", "dur_type", "prohibit_reason"]
        n = _upsert(conn, "dur_interaction_cache", cols, values)
        print(f"[dur] UPSERT {n} 행 (입력 {len(values)})")
        return n
    return 0


def import_overview(conn, csv_path: Path) -> int:
    print(f"[overview] {csv_path}")
    for header, rows in _iter_csv(csv_path):
        idx = _resolve_columns(header, OVERVIEW_COLUMN_MAP)
        if idx["item_code"] is None:
            print(f"ERROR: e약은요 item_code 컬럼 누락. 헤더={header}", file=sys.stderr)
            sys.exit(3)
        values: List[Tuple] = []
        for row in rows:
            code = _value(row, idx["item_code"])
            if not code:
                continue
            values.append((
                code,
                _value(row, idx["efficacy_text"]) or "",
                _value(row, idx["usage_text"]) or "",
                _value(row, idx["warning_text"]) or "",
                _value(row, idx["caution_text"]) or "",
                _value(row, idx["interaction_text"]) or "",
                _value(row, idx["side_effect_text"]) or "",
                _value(row, idx["storage_text"]) or "",
            ))
        cols = ["item_code", "efficacy_text", "usage_text", "warning_text",
                "caution_text", "interaction_text", "side_effect_text", "storage_text"]
        n = _upsert(conn, "drug_overview", cols, values)
        print(f"[overview] UPSERT {n} 행 (입력 {len(values)})")
        return n
    return 0


def import_ingredient(conn, csv_path: Path) -> int:
    print(f"[ingredient] {csv_path}")
    for header, rows in _iter_csv(csv_path):
        idx = _resolve_columns(header, INGREDIENT_COLUMN_MAP)
        if idx["ingredient_code"] is None:
            print(f"ERROR: 성분 코드 컬럼 누락. 헤더={header}", file=sys.stderr)
            sys.exit(3)
        values: List[Tuple] = []
        for row in rows:
            code = _value(row, idx["ingredient_code"])
            if not code:
                continue
            values.append((
                code,
                _value(row, idx["ingredient_name"]) or "",
                _value(row, idx["safety_info"]) or "",
            ))
        cols = ["ingredient_code", "ingredient_name", "safety_info"]
        n = _upsert(conn, "ingredient_info", cols, values)
        print(f"[ingredient] UPSERT {n} 행 (입력 {len(values)})")
        return n
    return 0


# =====================================================
# CLI
# =====================================================

def main():
    parser = argparse.ArgumentParser(
        description="식약처 CSV/Excel 일괄 적재",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__)
    parser.add_argument("kind", choices=["pill", "dur", "overview", "ingredient", "all"])
    parser.add_argument("paths", nargs="+", help="CSV / XLSX 파일 경로 (all 시 4개 순서)")
    args = parser.parse_args()

    paths = [Path(p) for p in args.paths]
    for p in paths:
        if not p.exists():
            print(f"ERROR: 파일 없음: {p}", file=sys.stderr)
            sys.exit(1)

    conn = _connect()
    try:
        if args.kind == "pill":
            import_pill(conn, paths[0])
        elif args.kind == "dur":
            import_dur(conn, paths[0])
        elif args.kind == "overview":
            import_overview(conn, paths[0])
        elif args.kind == "ingredient":
            import_ingredient(conn, paths[0])
        elif args.kind == "all":
            if len(paths) < 3:
                print("ERROR: all 은 최소 3개(pill, dur, overview) 필요", file=sys.stderr)
                sys.exit(1)
            import_pill(conn, paths[0])
            import_dur(conn, paths[1])
            import_overview(conn, paths[2])
            if len(paths) >= 4:
                import_ingredient(conn, paths[3])
    finally:
        conn.close()
    print("[import] DONE")


if __name__ == "__main__":
    main()
