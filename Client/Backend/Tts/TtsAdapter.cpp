// =====================================================
// TtsAdapter 구현 — Windows SAPI (PowerShell + System.Speech)
// =====================================================
#include "TtsAdapter.h"

#include <QProcess>
#include <QSettings>
#include <QLoggingCategory>

namespace medibridge::tts {

namespace {
constexpr auto kSettingsGroup = "tts";
constexpr auto kSettingsEnabled = "tts/enabled";

// 단정 표현 — 요구사항 §9.3 절대 규정.
// "복용 가능 / 불가" 단정 출력 금지. 다른 단정 어구도 함께 차단.
const QStringList kBannedPhrases = {
    QStringLiteral("복용 가능합니다"),
    QStringLiteral("복용 불가합니다"),
    QStringLiteral("복용해도 됩니다"),
    QStringLiteral("복용하시면 안 됩니다"),
    QStringLiteral("안전합니다"),       // 단정 "안전" 차단 — "확인되지 않았습니다" 톤만 허용
    QStringLiteral("위험합니다"),       // 단정 "위험" 차단 — "약사·의사 상담" 톤만 허용
    QStringLiteral("진단합니다"),
};
}  // namespace

TtsAdapter::TtsAdapter(QObject* parent)
    : QObject(parent)
    , banned_phrases_(kBannedPhrases)
{
    load_settings();

#ifdef Q_OS_WIN
    process_ = new QProcess(this);
    process_->setProgram(QStringLiteral("powershell.exe"));
    // QProcess::finished(int, QProcess::ExitStatus) — Qt6 시그니처
    connect(process_, &QProcess::finished,
            this, [this](int exit_code, QProcess::ExitStatus /*status*/) {
                on_process_finished(exit_code);
            });
#endif
}

TtsAdapter::~TtsAdapter()
{
    if (process_ && process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(500);
    }
}

bool    TtsAdapter::enabled() const     { return enabled_; }
bool    TtsAdapter::is_speaking() const { return is_speaking_; }
QString TtsAdapter::last_error() const  { return last_error_; }

void TtsAdapter::set_enabled(bool on)
{
    if (enabled_ == on) return;
    enabled_ = on;
    save_settings();
    if (!on && is_speaking_) stop();
    emit enabled_changed();
    qInfo() << "[TtsAdapter] enabled →" << on;
}

bool TtsAdapter::contains_banned_phrase(const QString& text) const
{
    for (const auto& phrase : banned_phrases_) {
        if (text.contains(phrase, Qt::CaseInsensitive)) return true;
    }
    return false;
}

void TtsAdapter::speak(const QString& text)
{
    if (text.trimmed().isEmpty()) return;

    if (!enabled_) {
        last_error_ = QStringLiteral("DISABLED");
        emit last_error_changed();
        emit rejected(text, last_error_);
        return;
    }

    if (contains_banned_phrase(text)) {
        last_error_ = QStringLiteral("BANNED_PHRASE");
        emit last_error_changed();
        emit rejected(text, last_error_);
        qWarning() << "[TtsAdapter] BANNED_PHRASE — 발화 거부:" << text;
        return;
    }

    emit will_speak(text);
    invoke_sapi(text);
}

void TtsAdapter::speak_active_guide(const QString& guide_text)
{
    // 능동 가이드는 단정 차단 없이 그대로 — 조작 안내만이라는 정책 (FR-C6-03)
    if (guide_text.trimmed().isEmpty()) return;
    if (!enabled_) return;
    emit will_speak(guide_text);
    invoke_sapi(guide_text);
}

void TtsAdapter::speak_conversational_guide(const QString& guide_text)
{
    if (guide_text.trimmed().isEmpty()) return;
    if (!enabled_) return;
    emit will_speak(guide_text);
    invoke_sapi(guide_text);
}

void TtsAdapter::stop()
{
#ifdef Q_OS_WIN
    if (process_ && process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(300);
    }
#endif
    if (is_speaking_) {
        is_speaking_ = false;
        emit is_speaking_changed();
    }
}

void TtsAdapter::invoke_sapi(const QString& text)
{
#ifdef Q_OS_WIN
    if (!process_) return;
    // 이전 발화 중단 — 큐잉 없이 즉시 교체
    if (process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(200);
    }

    // PowerShell 인용 처리 — single quote escaping
    QString escaped = text;
    escaped.replace(QStringLiteral("'"), QStringLiteral("''"));

    const QString script = QStringLiteral(
        "Add-Type -AssemblyName System.Speech; "
        "$s = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
        "$voice = $s.GetInstalledVoices() | "
        "Where-Object { $_.VoiceInfo.Culture.Name -eq 'ko-KR' } | "
        "Select-Object -First 1; "
        "if ($voice) { $s.SelectVoice($voice.VoiceInfo.Name) }; "
        "$s.Speak('%1');"
    ).arg(escaped);

    const QStringList args = {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-NonInteractive"),
        QStringLiteral("-Command"),
        script,
    };
    process_->setArguments(args);
    process_->start();

    is_speaking_ = true;
    emit is_speaking_changed();
    qInfo().noquote() << "[TtsAdapter] speak:" << text;
#else
    Q_UNUSED(text);
    // 비-Windows: no-op (단, did_speak 즉시 발신해 UI 흐름 유지)
    emit did_speak(text);
#endif
}

void TtsAdapter::on_process_finished(int exit_code)
{
    if (is_speaking_) {
        is_speaking_ = false;
        emit is_speaking_changed();
    }
    if (exit_code != 0) {
        last_error_ = QStringLiteral("PROCESS_FAILED");
        emit last_error_changed();
        qWarning() << "[TtsAdapter] SAPI process exit_code=" << exit_code;
        return;
    }
    emit did_speak(QString());
}

void TtsAdapter::load_settings()
{
    QSettings s;
    enabled_ = s.value(kSettingsEnabled, true).toBool();
}

void TtsAdapter::save_settings()
{
    QSettings s;
    s.setValue(kSettingsEnabled, enabled_);
}

} // namespace medibridge::tts
