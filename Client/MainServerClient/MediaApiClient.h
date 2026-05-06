// =====================================================
// MediaApiClient — /v1/media/* 호출 (MediaApi.md)
// =====================================================
#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <memory>
#include <functional>

namespace medibridge::network {

class ApiClientCommon;

class MediaApiClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray&, int)>;

    explicit MediaApiClient(std::shared_ptr<ApiClientCommon> common,
                            QObject* parent = nullptr);

    /// POST /v1/media/image — multipart/form-data
    void upload_image(const QByteArray& image_data,
                      const QString& mime_type,
                      const QString& intent_hint,
                      JsonCallback callback);

private:
    std::shared_ptr<ApiClientCommon> common_;
};

} // namespace medibridge::network
