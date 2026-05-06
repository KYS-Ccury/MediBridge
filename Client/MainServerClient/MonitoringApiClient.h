// =====================================================
// MonitoringApiClient — /health, /metrics 호출 (MonitoringApi.md)
// =====================================================
#pragma once

#include <QObject>
#include <memory>
#include <functional>

namespace medibridge::network {

class ApiClientCommon;

class MonitoringApiClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray&, int)>;

    explicit MonitoringApiClient(std::shared_ptr<ApiClientCommon> common,
                                 QObject* parent = nullptr);

    void check_health(JsonCallback callback);
    void check_metrics(JsonCallback callback);

private:
    std::shared_ptr<ApiClientCommon> common_;
};

} // namespace medibridge::network
