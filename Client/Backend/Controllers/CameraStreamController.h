#pragma once
#include <QObject>
#include <QTimer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QPointer>

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
    // ⚠ QML VideoOutput 의 QVideoSink — 페이지 파괴 시 객체가 삭제되므로
    //   raw 포인터면 dangling → setVideoFrame 시 액세스 위반(크래시).
    //   QPointer 는 객체 파괴 시 자동으로 nullptr 가 되어 안전.
    QPointer<QVideoSink> m_sink = nullptr;
};

} // namespace medibridge::controllers