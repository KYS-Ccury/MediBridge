// =====================================================
// TtsAdapter — 클라 자체 TTS (Windows SAPI / Heami 한국어)
// =====================================================
// 기획서 §8 어필 포인트 정본:
//   "TTS 어댑터 — 클라 자체 TTS 우선 + 메인 서버 fallback
//    (Qt6 QTextToSpeech + Windows SAPI 한국어 / ITtsProvider 인터페이스)"
//
// 본 클라이언트의 Qt 6.11 mingw 빌드에는 Qt6::TextToSpeech 모듈이 함께
// 설치되어 있지 않다 (`C:/Qt/6.11.0/mingw_64/lib/cmake/` 에 없음). 따라서
// Windows 에서는 PowerShell + System.Speech.Synthesis.SpeechSynthesizer
// 를 QProcess 로 호출하는 방식으로 SAPI 한국어 음성(Heami) 을 사용한다.
// 비-Windows 빌드는 no-op (signal 만 emit).
//
// 요구사항 매핑:
//   - FR-C3-02 : 등록 확인 / 분기 질문 / 능동·대화형 가이드 / 식별 결과 /
//                위험 검출 / 안전 케이스 안내 TTS 출력
//   - FR-C3-03 : 사용자 환경에 따라 음성 ↔ 클릭 선택 (enabled 토글)
//   - FR-C6-01 : 능동 가이드 TTS 멘트 (speak_active_guide)
//   - FR-C6-02 : 대화형 가이드 TTS 멘트 (speak_conversational_guide)
//
// 단정 표현 차단 정책 (요구사항 §9.3):
//   speak() 진입 시 "복용 가능합니다 / 복용 불가합니다" 등 단정 어구가
//   포함되면 거부하고 last_error 에 BANNED_PHRASE 표시.
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;

namespace medibridge::tts {

class TtsAdapter : public QObject
{
    Q_OBJECT
    /// TTS 활성 여부 (사용자 음성 안내 ON/OFF — QSettings 영속)
    Q_PROPERTY(bool enabled READ enabled WRITE set_enabled NOTIFY enabled_changed)
    /// 현재 발화 중 여부
    Q_PROPERTY(bool is_speaking READ is_speaking NOTIFY is_speaking_changed)
    /// 마지막 에러 코드 (BANNED_PHRASE / PROCESS_FAILED / DISABLED / "")
    Q_PROPERTY(QString last_error READ last_error NOTIFY last_error_changed)

public:
    explicit TtsAdapter(QObject* parent = nullptr);
    ~TtsAdapter() override;

    bool enabled() const;
    bool is_speaking() const;
    QString last_error() const;

    /// 일반 발화 — 단정 표현 필터 통과 후 SAPI 호출
    Q_INVOKABLE void speak(const QString& text);

    /// 능동 가이드 (시스템이 먼저 안내, FR-C6-01)
    /// 예: "카메라로 약을 비춰주세요"
    Q_INVOKABLE void speak_active_guide(const QString& guide_text);

    /// 대화형 가이드 (인식 결과에 따른 추가 요청, FR-C6-02)
    /// 예: "약이 서 있어요. 눕혀주세요"
    Q_INVOKABLE void speak_conversational_guide(const QString& guide_text);

    /// 현재 발화 중단
    Q_INVOKABLE void stop();

public slots:
    void set_enabled(bool on);

signals:
    void enabled_changed();
    void is_speaking_changed();
    void last_error_changed();
    /// 발화 시작 직전 (분석·로깅용)
    void will_speak(const QString& text);
    /// 발화 완료 시
    void did_speak(const QString& text);
    /// 발화 거부 (단정 표현 / 비활성)
    void rejected(const QString& text, const QString& reason);

private slots:
    void on_process_finished(int exit_code);

private:
    /// 단정 표현 필터 (요구사항 §9.3 절대 규정)
    bool contains_banned_phrase(const QString& text) const;
    /// 실제 SAPI 호출 (Windows 만 동작, 기타 OS no-op)
    void invoke_sapi(const QString& text);
    /// QSettings 에서 enabled 상태 복원
    void load_settings();
    /// QSettings 에 enabled 저장
    void save_settings();

    bool enabled_ = true;
    bool is_speaking_ = false;
    QString last_error_;

    QProcess* process_ = nullptr;
    QStringList banned_phrases_;
};

} // namespace medibridge::tts
