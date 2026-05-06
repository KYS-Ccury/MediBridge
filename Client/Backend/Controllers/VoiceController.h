#pragma once
#include <QObject>
#include <QString>

namespace medibridge::services { class UtteranceForwarder; }

namespace medibridge::controllers {

class VoiceController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool is_listening READ is_listening NOTIFY is_listening_changed)
    Q_PROPERTY(QString current_text READ current_text NOTIFY current_text_changed)

public:
    // PhoneServer 매개변수 제거됨
    explicit VoiceController(services::UtteranceForwarder* forwarder, QObject* parent = nullptr);

    bool is_listening() const;
    QString current_text() const;

    Q_INVOKABLE void start_listening();
    Q_INVOKABLE void stop_listening();
    Q_INVOKABLE void clear_text();

signals:
    void is_listening_changed();
    void current_text_changed();
    void speech_error(const QString& message);
    void speech_finished(const QString& final_text);

private slots:
    void on_utterance_received(const QString& text, bool is_final);

private:
    services::UtteranceForwarder* forwarder_;
    bool is_listening_ = false;
    QString current_text_;
};

} // namespace medibridge::controllers