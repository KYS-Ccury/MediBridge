#include "VoiceController.h"
#include "UtteranceForwarder.h"
#include <QLoggingCategory>

namespace medibridge::controllers {

VoiceController::VoiceController(services::UtteranceForwarder* forwarder, QObject* parent)
    : QObject(parent), forwarder_(forwarder)
{
    if (forwarder_) {
        // forwarder에서 텍스트가 오면 on_utterance_received 실행
        connect(forwarder_, SIGNAL(utterance_received(QString,bool)),
                this, SLOT(on_utterance_received(QString,bool)));
    }
}

bool VoiceController::is_listening() const { return is_listening_; }
QString VoiceController::current_text() const { return current_text_; }

void VoiceController::start_listening()
{
    if (is_listening_) return;
    current_text_.clear();
    emit current_text_changed();
    
    // PC 화면에서 마이크가 켜진 상태로만 전환 (실제 STT 작동은 폰에서 시작)
    is_listening_ = true;
    emit is_listening_changed();
    qInfo() << "[VoiceController] PC 마이크 대기 상태 켜짐";
}

void VoiceController::stop_listening()
{
    if (!is_listening_) return;
    is_listening_ = false;
    emit is_listening_changed();
}

void VoiceController::clear_text()
{
    current_text_.clear();
    emit current_text_changed();
}

void VoiceController::on_utterance_received(const QString& text, bool is_final)
{
    current_text_ = text;
    emit current_text_changed();

    if (is_final) {
        emit speech_finished(text);
        qInfo() << "[VoiceController] STT 텍스트 수신:" << text;
    }
}

} // namespace medibridge::controllers