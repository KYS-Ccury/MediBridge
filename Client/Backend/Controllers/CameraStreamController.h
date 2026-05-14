#pragma once
#include <QObject>
#include <QTimer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>

namespace medibridge::phonelink { class PhoneCaptureService; }

namespace medibridge::controllers {

class CameraStreamController : public QObject
{
    Q_OBJECT
    // QML의 VideoOutput.videoSink와 연결될 속성
    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)

public:
    explicit CameraStreamController(medibridge::phonelink::PhoneCaptureService* service, QObject* parent = nullptr);

    QVideoSink* videoSink() const { return m_sink; }
    void setVideoSink(QVideoSink* sink);

    // QML에서 호출할 함수
    Q_INVOKABLE void start_stream();
    Q_INVOKABLE void stop_stream();

signals:
    void videoSinkChanged();

private slots:
    void request_frame();
    void handle_captured_frame(const QByteArray& png_data);

private:
    medibridge::phonelink::PhoneCaptureService* m_service;
    QTimer m_timer;
    QVideoSink* m_sink = nullptr;
};

} // namespace medibridge::controllers