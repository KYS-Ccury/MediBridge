"""
BuildRagIndex — 메인서버 MariaDB → Chroma DB RAG 인덱스 빌드

사용:
    python TrainingServer/Scripts/BuildRagIndex.py \\
        --db-host 10.10.10.97 --db-user medibridge_app \\
        --db-pass $MEDIBRIDGE_DB_PASSWORD --db-name medibridge \\
        --embedding nlpai-lab/KURE-v1 --device cuda \\
        --out /home/medibridge/InferenceServer/chroma_db

빌드 후 적용:
    LLM PC 의 InferenceServer/chroma_db/ 에 결과 복사 (학습+추론 동거면 같은 디렉토리)
    FastAPI 재시작

생성 컬렉션:
    pdma_overview     — e약은요 본문 (섹션 단위 청크). 비의료 영역(efficacy/usage/storage) 만.
    dur_interactions  — DUR 페어. 메타데이터만 (LLM 컨텍스트 주입 X).

⚠ 정책:
    - 비의료 영역만 LLM 컨텍스트로 노출 가능
    - 의료 안내 영역(warning/caution/interaction/side_effect) 은 메타 저장만, 검색 시 필터링됨
    - DUR 위험 본문은 메인서버 DurChecker 가 직접 처리 — 본 인덱스는 메타만
"""
from __future__ import annotations

import argparse
import os
import sys
from typing import Iterable

from loguru import logger


SECTIONS_NON_MEDICAL = ("efficacy", "usage", "storage")
SECTIONS_MEDICAL = ("warning", "caution", "interaction", "side_effect")
ALL_SECTIONS = SECTIONS_NON_MEDICAL + SECTIONS_MEDICAL


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="MediBridge RAG 인덱스 빌드")
    p.add_argument("--db-host", default=os.getenv("DB_HOST", "10.10.10.97"))
    p.add_argument("--db-port", type=int, default=int(os.getenv("DB_PORT", "3306")))
    p.add_argument("--db-user", default=os.getenv("DB_USER", "medibridge_app"))
    p.add_argument("--db-pass", default=os.getenv("MEDIBRIDGE_DB_PASSWORD", ""))
    p.add_argument("--db-name", default=os.getenv("DB_NAME", "medibridge"))
    p.add_argument("--embedding", default=os.getenv("EMBEDDING_MODEL", "nlpai-lab/KURE-v1"))
    p.add_argument("--device", default=os.getenv("EMBEDDING_DEVICE", "cuda"))
    p.add_argument("--out", default=os.getenv("CHROMA_PERSIST_DIR", "./chroma_db"))
    p.add_argument("--batch-size", type=int, default=64)
    return p.parse_args()


def connect_db(args: argparse.Namespace):
    """MariaDB 연결."""
    try:
        import pymysql
    except ImportError:
        logger.error("pymysql 미설치 — pip install pymysql")
        sys.exit(1)

    if not args.db_pass:
        logger.error("DB 비밀번호 미설정. --db-pass 또는 MEDIBRIDGE_DB_PASSWORD env 필요")
        sys.exit(1)

    return pymysql.connect(
        host=args.db_host,
        port=args.db_port,
        user=args.db_user,
        password=args.db_pass,
        database=args.db_name,
        charset="utf8mb4",
        cursorclass=pymysql.cursors.DictCursor,
    )


def load_drug_overview(conn) -> list[dict]:
    """drug_overview + pill_identification 조인 — drug_name 동행."""
    with conn.cursor() as cur:
        cur.execute("""
            SELECT
                d.item_code,
                d.efficacy_text, d.usage_text, d.warning_text,
                d.caution_text, d.interaction_text, d.side_effect_text,
                d.storage_text,
                p.drug_name
            FROM drug_overview d
            LEFT JOIN pill_identification p ON p.item_code = d.item_code
        """)
        return cur.fetchall()


def load_dur_pairs(conn) -> list[dict]:
    """DUR 페어."""
    with conn.cursor() as cur:
        cur.execute("""
            SELECT
                dur_id,
                base_item_code,
                target_item_code,
                dur_type,
                prohibit_reason
            FROM dur_interaction_cache
        """)
        return cur.fetchall()


def chunks_from_overview_row(row: dict) -> list[dict]:
    """1 row → 섹션별 청크 list[{id, document, metadata}]"""
    out = []
    item_code = row["item_code"]
    drug_name = row.get("drug_name") or ""
    for section in ALL_SECTIONS:
        col = f"{section}_text"
        text = (row.get(col) or "").strip()
        if not text:
            continue
        # 너무 긴 텍스트는 자르기 (KURE-v1 max 8192 tokens — 안전하게 4000자)
        if len(text) > 4000:
            text = text[:4000]
        out.append({
            "id": f"{item_code}::{section}",
            "document": text,
            "metadata": {
                "item_code": item_code,
                "drug_name": drug_name,
                "section": section,
                # 정책 라벨 — 검색 시 LLM 컨텍스트 주입 가능 여부
                "is_medical_guidance": section in SECTIONS_MEDICAL,
            },
        })
    return out


def chunks_from_dur_pair(row: dict) -> dict:
    """DUR 페어 → 청크 1개 (검색 키 = '약A + 약B + 사유')"""
    base = row.get("base_item_code") or ""
    target = row.get("target_item_code") or ""
    reason = row.get("prohibit_reason") or ""
    document = f"{base} ↔ {target} 사유: {reason}"
    return {
        "id": f"dur::{row['dur_id']}",
        "document": document,
        "metadata": {
            "dur_id": int(row["dur_id"]),
            "base_item_code": base,
            "target_item_code": target,
            "dur_type": row.get("dur_type") or "",
        },
    }


def main() -> int:
    args = parse_args()

    try:
        import chromadb
        from sentence_transformers import SentenceTransformer
    except ImportError as e:
        logger.error(f"의존성 미설치: {e}. pip install -r TrainingServer/requirements.txt")
        return 1

    logger.info(f"[BuildRagIndex] 시작 — embedding={args.embedding} device={args.device} out={args.out}")
    os.makedirs(args.out, exist_ok=True)

    # 1) DB 로드
    conn = connect_db(args)
    try:
        overview_rows = load_drug_overview(conn)
        dur_rows = load_dur_pairs(conn)
    finally:
        conn.close()
    logger.info(f"[BuildRagIndex] DB rows: overview={len(overview_rows)} dur={len(dur_rows)}")

    # 2) 청크 생성
    overview_chunks = []
    for row in overview_rows:
        overview_chunks.extend(chunks_from_overview_row(row))
    dur_chunks = [chunks_from_dur_pair(r) for r in dur_rows]
    logger.info(f"[BuildRagIndex] chunks: overview={len(overview_chunks)} dur={len(dur_chunks)}")

    # 3) 임베딩 모델 로드
    embedder = SentenceTransformer(args.embedding, device=args.device)

    # 4) Chroma 컬렉션 (재빌드 — 기존 삭제 후 생성)
    client = chromadb.PersistentClient(path=args.out)
    for name in ("pdma_overview", "dur_interactions"):
        try:
            client.delete_collection(name)
            logger.info(f"[BuildRagIndex] 기존 컬렉션 삭제: {name}")
        except Exception:
            pass

    col_overview = client.create_collection("pdma_overview")
    col_dur = client.create_collection("dur_interactions")

    # 5) 일괄 임베딩 + 삽입
    def insert(col, chunks: Iterable[dict]):
        chunks = list(chunks)
        if not chunks:
            return
        ids = [c["id"] for c in chunks]
        docs = [c["document"] for c in chunks]
        metas = [c["metadata"] for c in chunks]
        # 배치 임베딩
        for i in range(0, len(docs), args.batch_size):
            batch_docs = docs[i:i + args.batch_size]
            batch_emb = embedder.encode(batch_docs, normalize_embeddings=True).tolist()
            col.add(
                ids=ids[i:i + args.batch_size],
                embeddings=batch_emb,
                documents=batch_docs,
                metadatas=metas[i:i + args.batch_size],
            )
            logger.info(f"  inserted {min(i + args.batch_size, len(docs))}/{len(docs)} into {col.name}")

    insert(col_overview, overview_chunks)
    insert(col_dur, dur_chunks)

    logger.info("[BuildRagIndex] 완료")
    logger.info(f"  pdma_overview     count = {col_overview.count()}")
    logger.info(f"  dur_interactions  count = {col_dur.count()}")
    logger.info(f"  → 출력 디렉토리: {os.path.abspath(args.out)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
