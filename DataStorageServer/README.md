# DataStorageServer — 데이터 보관 PC (Drogon C++ 미니 서버)

> 메디브릿지 사진 보관 전용. 메인서버 발급 HMAC 토큰 검증 후 PUT/GET 만 처리.

---

## 책임

- **PUT** `/storage/photos/{anonymous_id}/{photo_id}.{ext}` — 클라PC 가 사진 본체 업로드
- **GET** `/storage/photos/{anonymous_id}/{photo_id}.{ext}` — Vision PC 가 추론용으로 다운로드
- **GET** `/health` — 메인서버 모니터링용

**메인서버 통과 X.** 모든 인증·메타·정책은 메인서버가 발급한 토큰에 박혀 있고, 본 서버는 그 토큰을 검증만 한다.

---

## 빠른 시작 (시연/스모크)

```bash
# 1) 빌드
cd DataStorageServer
mkdir -p build-wsl && cd build-wsl
cmake .. && make -j$(nproc)
cd ..

# 2) 띄우기
bash Scripts/datastorage-up.sh

# 3) 메인서버랑 함께 풀 시나리오 테스트
bash Scripts/smoke_test.sh
```

---

## 환경변수

| 이름 | 기본값 | 비고 |
| --- | --- | --- |
| `DATASTORAGE_PORT` | `8004` | HTTP 포트 |
| `DATASTORAGE_THREAD_NUM` | `0` (CPU 코어 수) | Drogon 워커 스레드 |
| `DATASTORAGE_ROOT` | `/var/lib/medibridge_storage` | 파일 저장 루트 |
| `DATASTORAGE_MAX_BYTES` | `10485760` | 업로드 최대 바이트 (10MB) |
| `MEDIBRIDGE_STORAGE_SECRET` | (없음) | **메인서버와 동일 시크릿 (HMAC-SHA256, 32+ 바이트)** |
| `MEDIBRIDGE_ENV` | `development` | `production` 시 시크릿 미설정 거부 |

---

## 토큰 형식 — 메인서버 발급, 본 서버 검증

JWT 호환 (HS256). payload 예시:

```json
{
  "iss":  "medibridge-main",
  "aud":  "datastorage",
  "sub":  "anon_test_001",
  "jti":  "ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d",
  "op":   "put",
  "mime": "image/jpeg",
  "max":  10485760,
  "iat":  1778639483,
  "exp":  1778639783
}
```

### 검증 항목 (모두 통과해야 200)

| # | 항목 | 위치 |
|---|---|---|
| 1 | HMAC 서명 일치 | `Services/TokenVerifier` |
| 2 | `iss="medibridge-main"`, `aud="datastorage"` | TokenVerifier |
| 3 | `exp > now` (만료 안 됨) | TokenVerifier |
| 4 | `op` 일치 (PUT 토큰으로 GET 못함) | TokenVerifier |
| 5 | URL 의 `anon` ↔ 토큰 `sub` 일치 | `Routers/Photo` |
| 6 | URL 의 `photo_id` ↔ 토큰 `jti` 일치 | Routers/Photo |
| 7 | URL 확장자 ↔ 토큰 `mime` 일치 | Routers/Photo |
| 8 | Content-Type ↔ 토큰 `mime` 일치 (있으면) | Routers/Photo |
| 9 | Content-Length ≤ `min(토큰 max, 서버 max_bytes)` | Routers/Photo |
| 10 | `anon`, `photo_id` 화이트리스트 `[A-Za-z0-9_-]{1,64}` | `Services/StorageManager` |
| 11 | 확장자 화이트리스트 `{jpg, png}` | StorageManager |

---

## 디스크 구조

```
<DATASTORAGE_ROOT>/
├─ anon_test_001/
│  ├─ ph_3f0a4e30bd6cd9e0a5f2ba4de68b1c4d.jpg
│  └─ ph_5d97cb...png
├─ anon_test_002/
│  └─ ...
```

- 파일명 = `<photo_id>.<ext>` (1:1 photo_storage PK)
- atomic write — `<photo_id>.<ext>.tmp` 작성 후 `rename` (반쪽 파일 방지)

---

## 폴더 구조

```
DataStorageServer/
├─ Main.cpp                    # drogon::app().run()
├─ Config.cpp/.h               # 포트·storage_root·공유 시크릿
├─ CMakeLists.txt
├─ README.md
├─ Routers/
│  ├─ Photo.cpp/.h             # PUT/GET (요청 검증 → 서비스 위임)
│  └─ Monitoring.cpp/.h        # /health
├─ Services/
│  ├─ TokenVerifier.cpp/.h     # 토큰 HMAC 검증 (메인 issuer 의 짝)
│  └─ StorageManager.cpp/.h    # 파일 I/O — atomic write·MIME 매핑
└─ Scripts/
   ├─ datastorage-up.sh        # 빌드 후 부팅 스크립트
   └─ smoke_test.sh            # 메인 + 보관 풀 시나리오 검증
```

---

## 보안 정책

1. **시크릿 분리** — 메인서버 JWT 시크릿과 다른 32+ 바이트 별도 시크릿
2. **op 분리** — PUT 토큰으로 GET 불가, GET 토큰으로 PUT 불가
3. **TTL 짧음** — 메인서버에서 300초 발급 (Config 로 조정 가능)
4. **경로 traversal 방어** — 모든 ID 화이트리스트 정규식 통과
5. **IP rate limit** — Drogon `setMaxConnectionNumPerIP(20)`

---

## 관련 문서

- 메인서버 측 발급: `MainServer/Services/Media/StorageTokenIssuer.cpp`
- 메인서버 라우터: `MainServer/Routers/Media.cpp` (`/v1/media/intent`, `/v1/media/commit`)
- 아키텍처: `Docs/Architecture/MediBridge_Architecture_MVP.pptx` 슬라이드 2 ⑤⑥
