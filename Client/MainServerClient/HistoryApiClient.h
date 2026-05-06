// =====================================================
// HistoryApiClient — /v1/history/* 호출 (HistoryApi.md)
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <functional>

namespace medibridge::network {

class ApiClientCommon;

class HistoryApiClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray&, int)>;

    explicit HistoryApiClient(std::shared_ptr<ApiClientCommon> common,
                              QObject* parent = nullptr);

    void record(const QString& item_code,
                int quantity,
                const QString& memo,
                JsonCallback callback);

    void list(const QString& from_date,
              const QString& to_date,
              int page,
              int page_size,
              JsonCallback callback);

private:
    std::shared_ptr<ApiClientCommon> common_;
};

} // namespace medibridge::network
