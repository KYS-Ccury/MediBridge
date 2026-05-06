// =====================================================
// ReportApiClient — /v1/report/* 호출 (ReportApi.md)
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <functional>

namespace medibridge::network {

class ApiClientCommon;

class ReportApiClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray&, int)>;

    explicit ReportApiClient(std::shared_ptr<ApiClientCommon> common,
                             QObject* parent = nullptr);

    /// GET /v1/report/generate?format={pdf|html|json}
    void generate(const QString& from_date,
                  const QString& to_date,
                  const QString& format,
                  JsonCallback callback);

private:
    std::shared_ptr<ApiClientCommon> common_;
};

} // namespace medibridge::network
