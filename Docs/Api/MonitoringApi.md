# Monitoring API — 자원 모니터링 (전 영역 공통)

| 항목 | 내용 |
| --- | --- |
| **버전** | v0.1 |
| **작성일** | 2026-05-06 |
| **모듈** | 전 영역 공통 (Client / MainServer / InferenceServer) |
| **관련 문서** | [요구사항 분석서 v2 §5.7 FR-B7 / §8.3](../요구사항_분석서_ver2.md), [시스템 흐름 정리본 v2 §5.1](../시스템%20흐름%20정리본_ver2.md), [기획서 v2 §2.3](../기획서_ver2.md) |
| **공통 규칙** | [ApiOverview.md](ApiOverview.md) |

> 각 서버·디바이스의 자원 사용률 + 헬스 상태를 노출. **MVP는 자체 구현(가벼운 `/health` + `/metrics`)**, 확장 시 Prometheus/Grafana 통합 가능. **Client는 자체 자원만 콘솔 로그**, UI 미연동 (별도 모니터링 시스템과 통합 예정).

---

## 적용 범위

| 서버 | `/health` | `/metrics` | 비고 |
| --- | --- | --- | --- |
| MainServer (8001) | ✓ | ✓ | DB·추론서버 연결 점검 포함 |
| InferenceServer (8002) | ✓ | ✓ | GPU 사용률·모델 로딩 상태 포함 |
| Client (8000, PhoneAdapter) | ✓ | ✓ | 자체 자원 + 서버 폴링 결과 |

> **공통 규칙과 다른 점**: `/health`, `/metrics` 는 **버전 prefix `/v1/` 없음**. 운영 표준 관례 (Kubernetes·Prometheus 등 호환).

---

## 1. GET /health — 단순 헬스체크

### 요청
- 헤더: 없음 (인증 불필요 — 외부 모니터링 도구 접근 허용)
- 메서드: GET

### 응답 — 정상
- **200 OK**
```json
{
  "status": "ok",
  "service": "main_server",
  "version": "0.1.0",
  "uptime_seconds": 12345,
  "checked_at": "2026-05-06T10:30:00+09:00"
}
```

### 응답 — 부분 장애 (degraded)
- **503 Service Unavailable**
```json
{
  "status": "degraded",
  "service": "main_server",
  "version": "0.1.0",
  "uptime_seconds": 12345,
  "checked_at": "2026-05-06T10:30:00+09:00",
  "reasons": [
    "db_connection_failed",
    "inference_server_unreachable"
  ]
}
```

### `status` 값
| 값 | HTTP | 의미 |
| --- | --- | --- |
| `ok` | 200 | 모든 의존 서비스 정상 |
| `degraded` | 503 | 일부 의존 서비스 장애 (서비스는 부분 동작) |
| `down` | 503 | 핵심 의존 서비스(DB 등) 장애로 동작 불가 |

### 점검 항목 (서버별)

**MainServer**:
- DB(MariaDB) 연결 점검 (`SELECT 1`)
- InferenceServer `/health` 호출 (3초 타임아웃)
- 디스크 여유 공간 (< 1GB 시 degraded)

**InferenceServer**:
- GPU 가용성 (`nvidia-smi` 또는 `pynvml`)
- 모델 로딩 상태 (YOLO26, PaddleOCR, LLM)
- 디스크 여유 공간

**Client (PhoneAdapter)**:
- MainServer `/health` 호출 결과
- 폰 ADB 연결 상태 (`adb devices` 출력 파싱)

---

## 2. GET /metrics — 자원 사용률 메트릭

### 요청
- 헤더: 없음
- 메서드: GET
- (선택) 쿼리: `format` — `json` (기본) / `prometheus` (확장)

### 응답 — JSON 형식 (기본)
- **200 OK**
```json
{
  "service": "main_server",
  "collected_at": "2026-05-06T10:30:00+09:00",
  "system": {
    "cpu_percent": 12.3,
    "memory_used_mb": 1234.5,
    "memory_total_mb": 16384.0,
    "memory_percent": 7.5,
    "disk_used_gb": 45.2,
    "disk_total_gb": 500.0,
    "disk_percent": 9.0,
    "uptime_seconds": 12345
  },
  "process": {
    "pid": 12345,
    "cpu_percent": 5.1,
    "memory_mb": 234.5,
    "thread_count": 16,
    "open_file_descriptors": 78
  },
  "service_specific": {
    "active_connections": 23,
    "request_rate_per_minute": 45,
    "db_connection_pool_used": 3,
    "db_connection_pool_total": 10
  }
}
```

### 응답 — InferenceServer 추가 필드
```json
{
  "service": "inference_server",
  "system": { /* 동일 */ },
  "process": { /* 동일 */ },
  "gpu": [
    {
      "index": 0,
      "name": "NVIDIA GeForce RTX 4090",
      "utilization_percent": 35.2,
      "memory_used_mb": 8192.0,
      "memory_total_mb": 24576.0,
      "memory_percent": 33.3,
      "temperature_c": 65
    }
  ],
  "models": [
    {
      "name": "yolo26",
      "loaded": true,
      "infer_count_total": 1234,
      "avg_latency_ms": 45.6,
      "last_infer_at": "2026-05-06T10:29:55+09:00"
    },
    {
      "name": "paddle_ocr",
      "loaded": true,
      "infer_count_total": 1100,
      "avg_latency_ms": 78.2
    }
  ]
}
```

### 응답 — Client 추가 필드
```json
{
  "service": "client_phone_adapter",
  "system": { /* Windows */ },
  "process": { /* Qt6 process */ },
  "phone_link": {
    "adb_connected": true,
    "device_serial": "R3CXXXXXXX",
    "last_health_check": "2026-05-06T10:29:50+09:00"
  },
  "main_server_ping": {
    "reachable": true,
    "latency_ms": 12,
    "last_checked": "2026-05-06T10:29:55+09:00"
  }
}
```

### 응답 — Prometheus 형식 (확장)
- **200 OK**
- Content-Type: `text/plain; version=0.0.4`
```
# HELP cpu_percent CPU usage percentage
# TYPE cpu_percent gauge
cpu_percent{service="main_server"} 12.3
memory_percent{service="main_server"} 7.5
gpu_utilization_percent{service="inference_server",gpu="0"} 35.2
...
```

> Prometheus 형식은 확장 단계 — MVP 미구현, 자리만 정의.

### Schemas/ 매핑
- `MainServer/Schemas/HealthSchema.h::HealthResponse`
- `MainServer/Schemas/HealthSchema.h::MetricsResponse`
- `MainServer/Schemas/HealthSchema.h::SystemMetrics`
- `MainServer/Schemas/HealthSchema.h::ProcessMetrics`
- `InferenceServer/Schemas/HealthSchema.py::GpuMetrics`, `ModelMetrics`
- `Client/Monitoring/Schemas` — Client 측은 콘솔 로그만이므로 Schemas 불필요, 내부 struct로 충분

---

## 수집 주기 / 폴링 정책

| 주체 | 주기 | 동작 |
| --- | --- | --- |
| Client/Monitoring | 30초 | MainServer `/health` 폴링, 결과 콘솔 로그 |
| Client/Monitoring | 10초 | 자체 자원 측정, 콘솔 로그 |
| 외부 모니터링 도구 (확장) | 임의 | `/metrics` 호출 |

---

## 변경 이력

| 버전 | 일자 | 변경 사항 |
| --- | --- | --- |
| v0.1 | 2026-05-06 | 초안 — `/health`, `/metrics` 정의, 서버별 추가 필드, Prometheus 확장 자리, Client는 콘솔 로그 정책 |
