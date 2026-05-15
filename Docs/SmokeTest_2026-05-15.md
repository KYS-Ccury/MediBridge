# 클라이언트 GUI 시연 테스트 결과 — 2026-05-15

| 항목 | 값 |
|---|---|
| 시연 일자 | 2026-05-15 |
| 클라 빌드 | `MediBridgeClient.exe` (커밋 `0593288` 시점) |
| 메인서버 | `10.10.10.97:8001` (TestMode = true) |
| 보관 PC | `10.10.10.122:8004` (선택 기동) |
| 시드 계정 | `test@medibridge.local` / `test1234` |
| 폰 | 다른 담당자 폰 → 본인 폰 (8000번 충돌 이슈 별도 처리) |

---

## ✅ 통과 (실측 확인 완료)

### 1. Login 화면
- 정상 로그인 + 토큰 발급
- 잘못된 비번 → `INVALID_CREDENTIALS` 토스트
- 회원가입 링크 → SignupPage → 가입 성공 토스트 → LoginPage 자동 복귀
- 비밀번호 마스킹 (•••)

### 2. HomePage 진입 직후
- 상단 헤더 로그아웃 버튼 → LoginPage 복귀
- 폰 연결 상태 PhoneStatusIndicator
- 액션 카드 클릭 가능 (사용자 메모: "6개 카드"로 확인됨)

### 3. 자동 로그인 (NEW)
- 로그인 → 종료 → 재실행 → **HomePage 직행** (LoginPage 안 거침)
- 로그아웃 → 종료 → 재실행 → LoginPage 표시 (세션 비워짐)

### 4. 복약 이력 (HistoryPage)
- 빠른 기간 버튼 (7/30/90일·전체) 동작
- 직접 날짜 입력 (`2025-01-01 ~ 2026-12-31`) 조회 동작
- 수량·메모·시간 표시

### 5. 통합 보고서 (ReportPage)
- 빠른 기간 버튼 (7/30/90/365일) 동작
- 📄 PDF 저장 + 시스템 뷰어 자동 열림
- 🌐 HTML 미리보기 + 브라우저 자동 열림
- 저장 경로 카드 표시

### 6. 식별 결과 (IdentifyResultPage)
- 신뢰도 배지 (HIGH/MEDIUM/LOW)
- 복용 기록 남기기 다이얼로그 → 수량·메모 입력 → history 저장

### 7. 음성 입력 (VoiceInputPage)
- 마이크 펄스 애니메이션
- **폰 → PC 실시간 텍스트 표시** (`utterance_received` 버그 수정 후 확인됨)
- 초기화 버튼

### 8. 폰 PWA 시연
- ADB 디바이스 자동 감지 → 인디케이터 초록
- `adb reverse tcp:8000 tcp:8000` 자동 등록
- 폰 브라우저 → `http://localhost:8000` → PWA 진입
- 폰 STT 텍스트 → PC GUI 실시간 표시
- **폰 카메라 촬영 → PC → 메인서버 → 보관 PC PUT 까지 확인** (이후 식별→결과 화면 자동 표시는 미확인)

---

## ⏸ 미확인 (보류 — 다음 시연 때 확인)

확인 어려움 또는 시연 동선상 안 들어간 신규 기능들.

| 항목 | 사유 |
|---|---|
| 🔊 HomePage 환영 TTS 발화 (Heami) | 발화 청취 미실시 |
| 음성 안내 ON/OFF Switch + QSettings 영속 | 토글 클릭 미실시 |
| QSettings 위치 `HKCU\Software\MediBridge\MediBridgeClient` | 레지스트리 미확인 |
| PillPoolPage ✕ soft delete | (약 풀 표시 자체 실패로 진입 불가) |
| PillPoolPage "음성으로 등록" → VoiceInputPage 라우팅 | 동상 |
| PillPoolPage 전체 리셋 | 동상 |
| HistoryPage 시간대 색상 바 (아침🟧/점심🟩/저녁🟦/취침🟪) | 시각 확인 미실시 |
| ReportPage 로딩 인디케이터 (wkhtmltopdf) | 짧아서 확인 어려움 |
| IdentifyResultPage TTS 자동 발화 (`tts_text`) | 발화 청취 미실시 |
| MEDIUM/LOW 추가 안내 발화 | 동상 |
| DUR risk 검출 발화 | 동상 |
| 후보 카드 "내 약 풀" 초록 배지 | 약 풀 실패로 검증 불가 |
| 폰 촬영 → 보관 PC PUT 이후 → 식별 → 결과 화면 자동 표시 | PUT 까지만 확인 |

---

## ❌ 실패

### PillPoolPage — 시드 4건 미표시

- 증상: "내 약 목록" 진입 시 시드 4건 (타이레놀500 / 이부프로펜200 / 베아제 / 아스피린) 안 보임
- 영향:
  - 직접 입력으로 등록 검증 불가
  - 중복 등록 거부 검증 불가
  - ✕ soft delete · 전체 리셋 · 식별 결과 "내 약 풀" 배지 모두 차단됨
- 원인 후보 (조사 필요):
  1. 백엔드 `/v1/pill/pool` 응답이 비어있음 (시드 적용 안 됨 또는 anonymous_id 매칭 안 됨)
  2. `PoolItemListModel` 의 JSON 파싱 키 미스매치
  3. QML 바인딩 누락 (model 객체 vs 데이터 갱신)

---

## 🔧 추가 요청 사항 (사용자 피드백)

| # | 요청 | 위치 | 우선순위 |
|---|---|---|---|
| 1 | 복용기록 남기기 다이얼로그에서 약 데이터 **선택 + 직접 입력** 가능하게 | IdentifyResultPage `record_dialog` | 높음 |
| 2 | 촬영 후 식별 결과 페이지의 **"처음으로" 버튼이 HomePage 로 안 가고 카메라 페이지로 복귀** | IdentifyResultPage `stack.pop()` | 높음 (UX 버그) |
| 3 | 복약 기록 시 풀에 **미등록 약**은 → "풀에 등록 + 기록" / "기록만 남기기" 선택 가능 | IdentifyResultPage `record_dialog` | 중간 |
| 4 | 메모 (선택) **placeholder 예시 변경** | IdentifyResultPage `memo_input` | 낮음 |
| 5 | HomePage "최근 식별 결과" 카드 클릭 시 **이력 없으면 안내 팝업** | HomePage SubMenuCard | 중간 |

### 확인된 정상 동작 (피드백 중)
- 복약 기록 시 시간 자동 등록 ✅

---

## 다음 단계 (실행 계획)

1. **🔍 진단** — PillPoolPage 시드 4건 미표시 원인 파악 (curl 로 백엔드 응답 확인 + 클라 모델 파싱 검증)
2. **🔧 버그 수정**
   - 추가 요청 #2: "처음으로" 버튼 라우팅 → `stack.replace("HomePage.qml")` 또는 `pop(null)` 패턴
3. **➕ 기능 추가**
   - 추가 요청 #1: 약 선택 콤보박스 + 수동 입력 필드
   - 추가 요청 #3: 미등록 약 분기 UI (RadioButton "풀에 등록 + 기록" / "기록만")
   - 추가 요청 #5: HomePage 카드 클릭 → `pill_controller.last_request_id` 비면 토스트
4. **🎨 사소한 수정**
   - 추가 요청 #4: 메모 placeholder 변경
5. **🧪 재시연** — 보류된 ⏸ 항목들 (TTS / 음성 토글 / DUR 발화 등) 동선 짜서 확인

---

## 부가 — 폰 USB 8000 충돌 이슈

본인 폰에서 `adb reverse tcp:8000 tcp:8000` 실패 → 해결 방법:

1. `adb reverse --remove-all` + `adb kill-server` + `adb start-server` + USB 재연결
2. 폰 측 8000 점유 앱 종료 (Termux / KSWEB 등)
3. 환경변수 `MEDIBRIDGE_PHONE_PORT=18000` 으로 포트 변경 (커밋 `262f7ff` 에서 지원)

---

## 관련 커밋

### 사용자 피드백 반영 (오늘)
- `f671c8c` fix(client/pill): 약 풀 미표시 가설 보강 — 인증 가드 + 진단 로그 + 재시도
- `1dea3fa` fix(client/qml): 식별 결과 '처음으로' 버튼 → HomePage 직행
- `19f3e94` feat(client/qml): 복용 기록 다이얼로그 개편 — 약 선택 3가지 + 풀 등록 분기
- `ac93cbf` feat(client/qml): HomePage '최근 식별 결과' 카드 — 항상 활성 + 클릭 안내

### 그 외 같은 세션 작업
- `0593288` feat(dev-log): TestMode 한정 utterance 본문 미리보기 로깅
- `1b87fa4` fix(client/phone): 폰 STT 텍스트 GUI 표시 — UtteranceForwarder 경유
- `262f7ff` feat(client/phone): MEDIBRIDGE_PHONE_PORT 환경변수 오버라이드
- `5b3ccae` feat(client): TTS 어댑터 + 자동 로그인 + 능동/대화형 가이드

---

## 재시연 가이드 (오늘 변경 검증)

### 1. 약 풀 표시 — 콘솔 로그 동시 확인
클라이언트 실행 시 콘솔(또는 Qt Creator 출력 패널) 에 다음 로그 차례로 떨어져야 정상:
```
[PillPoolPage] load_pool 호출 — authed=true
[PillController] load_pool 시작 — include_inactive=false
[PillController] /v1/pill/pool 응답 status=200 body=NNN B
[PillController] pool 로드 OK — items=4 total=4 model_rows=4
[PillPoolPage] pool_loaded — model rows: 4
```
- `authed=false` 가 보이면 → 자동 로그인 토큰 set 타이밍 문제 (auth 변경 시 자동 재시도 트리거됨)
- `status=401` 이면 → 저장된 토큰 만료 (재로그인 필요)
- `items=4 model_rows=4` 인데 ListView 가 비면 → QML 바인딩 문제 (추가 조사)

### 2. '처음으로' 버튼
카메라로 촬영 → 식별 결과 → '처음으로' 클릭 → **HomePage 로 직행** (CameraPage 안 거침)

### 3. 복용 기록 다이얼로그 개편
식별 결과 → "복용 기록 남기기" 클릭. 새 다이얼로그에서:
- RadioButton 3개 — '식별 결과' / '내 약 풀' / '직접 입력'
- 선택 요약 카드 — 초록 ✓ (풀 멤버) / 주황 ⚠ (풀 미등록)
- 미등록 약 → 분기 RadioButton 자동 표시
- 메모 placeholder — "예: 점심 식후 / 가벼운 두통"

### 4. 최근 식별 결과 카드
HomePage → "최근 식별 결과" 카드 (회색 아닌 흰색·활성) 클릭:
- 이력 없으면 → "최근 식별 이력이 없습니다. 먼저 카메라로 약을 촬영해주세요." 토스트
- 이력 있으면 → IdentifyResultPage 진입
