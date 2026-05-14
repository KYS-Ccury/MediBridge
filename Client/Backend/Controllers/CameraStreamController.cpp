#include "CameraStreamController.h"
#include "PhoneCaptureService.h"

namespace medibridge::controllers {

CameraStreamController::CameraStreamController(medibridge::phonelink::PhoneCaptureService* service, QObject* parent)
    : QObject(parent), m_service(service)
{
    connect(&m_timer, &QTimer::timeout, this, &CameraStreamController::request_frame);
    
    if (m_service) {
        connect(m_service, &medibridge::phonelink::PhoneCaptureService::capture_succeeded,
                this, &CameraStreamController::handle_captured_frame);
    }
}

void CameraStreamController::setVideoSink(QVideoSink* sink) {
    if (m_sink != sink) {
        m_sink = sink;
        emit videoSinkChanged();
    }
}

void CameraStreamController::start_stream() {
    m_timer.start(150); // 약 7 FPS (시스템 부하 고려)
}

void CameraStreamController::stop_stream() {
    m_timer.stop();
}

void CameraStreamController::request_frame() {
    if (m_service && !m_service->is_capturing()) {
        m_service->capture_screen();
    }
}

void CameraStreamController::handle_captured_frame(const QByteArray& png_data) {
    if (!m_sink) return;

    QImage img = QImage::fromData(png_data);
    if (!img.isNull()) {
        // 이미지를 비디오 프레임으로 변환하여 전달
        m_sink->setVideoFrame(QVideoFrame(img));
    }
}

} // namespace medibridge::controllers