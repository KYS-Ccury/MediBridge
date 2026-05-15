"""
RagClient — Chroma + KURE-v1 임베딩 기반 검색

컬렉션:
    pdma_overview      : 식약처 e약은요 본문 (섹션 단위 청크)
    dur_interactions   : DUR 페어 (위험 정보 — LLM 미사용, 메타만 검색)

⚠ 정책:
    - 의료 안내 영역(DUR 위험 본문) 은 본 클라이언트로 LLM 컨텍스트에 주입 X
    - DUR 검색은 메타데이터(item_code, dur_type)만 — 메인서버 DurChecker 가 처리
    - 비의료 영역(e약은요 효능·복용법·보관법) 은 본문 검색 OK

지연 로딩 — 모델은 첫 query() 호출 시 다운로드/로딩.
"""
from __future__ import annotations

import os
from typing import Optional

from loguru import logger

from Config import get_config


class RagClient:
    """Chroma + KURE-v1 검색 클라이언트 (싱글톤)"""

    _instance: Optional["RagClient"] = None

    @classmethod
    def instance(cls) -> "RagClient":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self) -> None:
        self._cfg = get_config()
        self._embedder = None       # SentenceTransformer
        self._client = None         # chromadb.PersistentClient
        self._loaded = False

    def load(self) -> bool:
        """모델 + DB 로딩. 실패 시 False."""
        if self._loaded:
            return True
        try:
            from sentence_transformers import SentenceTransformer
            import chromadb

            logger.info(f"[RagClient] 임베딩 모델 로딩: {self._cfg.embedding_model}")
            self._embedder = SentenceTransformer(
                self._cfg.embedding_model,
                device=self._cfg.embedding_device,
            )

            persist_dir = os.path.abspath(self._cfg.chroma_persist_dir)
            os.makedirs(persist_dir, exist_ok=True)
            self._client = chromadb.PersistentClient(path=persist_dir)
            logger.info(f"[RagClient] Chroma persist: {persist_dir}")

            # 컬렉션 미존재 시 생성 (BuildRagIndex.py 가 채워야 검색 결과 있음)
            self._client.get_or_create_collection("pdma_overview")
            self._client.get_or_create_collection("dur_interactions")

            self._loaded = True
            return True
        except ImportError as e:
            logger.warning(
                f"[RagClient] 의존성 미설치 — {e}. "
                "requirements.txt 확인: sentence-transformers, chromadb"
            )
            return False
        except Exception as e:
            logger.exception(f"[RagClient] 로딩 실패: {e}")
            return False

    def is_loaded(self) -> bool:
        return self._loaded

    def search_overview(
        self,
        query: str,
        *,
        n_results: int = 3,
        section_filter: Optional[list[str]] = None,
    ) -> list[dict]:
        """
        e약은요 본문 검색 (비의료 영역).

        section_filter 미지정 시 위험성 섹션(warning/caution/interaction/side_effect) 자동 제외.
        """
        if not self._loaded and not self.load():
            return []
        try:
            allowed_sections = section_filter or [
                "efficacy", "usage", "storage"
            ]  # 의료 안내 영역 섹션 제외 (warning/caution/interaction/side_effect)

            embedding = self._embedder.encode([query], normalize_embeddings=True).tolist()
            col = self._client.get_collection("pdma_overview")
            results = col.query(
                query_embeddings=embedding,
                n_results=n_results,
                where={"section": {"$in": allowed_sections}},
            )
            return self._format_results(results)
        except Exception as e:
            logger.exception(f"[RagClient] search_overview 실패: {e}")
            return []

    def search_dur_meta(
        self,
        query: str,
        *,
        n_results: int = 5,
    ) -> list[dict]:
        """
        DUR 페어 메타데이터 검색 (의료 안내 본문은 메인서버 DurChecker 가 처리).

        ⚠ 본 메서드의 결과는 LLM 컨텍스트에 주입 금지 — 메타만 활용.
        """
        if not self._loaded and not self.load():
            return []
        try:
            embedding = self._embedder.encode([query], normalize_embeddings=True).tolist()
            col = self._client.get_collection("dur_interactions")
            results = col.query(
                query_embeddings=embedding,
                n_results=n_results,
            )
            return self._format_results(results)
        except Exception as e:
            logger.exception(f"[RagClient] search_dur_meta 실패: {e}")
            return []

    def _format_results(self, raw) -> list[dict]:
        """Chroma 결과 → flat list[dict]"""
        out = []
        if not raw or not raw.get("ids"):
            return out
        ids = raw["ids"][0]
        docs = raw.get("documents", [[]])[0]
        metas = raw.get("metadatas", [[]])[0]
        dists = raw.get("distances", [[]])[0]
        for i, doc_id in enumerate(ids):
            out.append({
                "id": doc_id,
                "document": docs[i] if i < len(docs) else None,
                "metadata": metas[i] if i < len(metas) else {},
                "distance": dists[i] if i < len(dists) else None,
            })
        return out

    def health(self) -> dict:
        """헬스 — 컬렉션 카운트 반환"""
        if not self._loaded:
            return {"loaded": False}
        try:
            pdma = self._client.get_collection("pdma_overview").count()
            dur = self._client.get_collection("dur_interactions").count()
            return {
                "loaded": True,
                "embedding_model": self._cfg.embedding_model,
                "device": self._cfg.embedding_device,
                "pdma_overview_count": pdma,
                "dur_interactions_count": dur,
            }
        except Exception as e:
            return {"loaded": True, "error": str(e)}
