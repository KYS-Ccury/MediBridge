// =====================================================
// ImageForwarder — 폰 카메라 이미지를 MainServer로 중계
// =====================================================
#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <functional>

namespace medibridge::network { class ApiClient; }

namespace medibridge::services {

class ImageForwarder : public QObject
{
    Q_OBJECT
public:
    using ForwardCallback = std::function<void(const QByteArray& main_server_response,
                                               int status_code)>;

    explicit ImageForwarder(network::ApiClient* api_client,
                            QObject* parent = nullptr);

    /**
     * 폰에서 받은 이미지를 MainServer로 전달.
     *   1. (선택) WorkerPool — 이미지 검증·리사이즈
     *   2. ApiClient::media().upload_image(...)
     */
    void forward(const QByteArray& image_data,
                 const QString& mime_type,
                 const QString& intent_hint,
                 ForwardCallback callback);

private:
    network::ApiClient* api_client_;   // 외부 소유
};

} // namespace medibridge::services
