// =====================================================
// HistoryController — 복약 이력 기록·조회 (ViewModel)
// =====================================================
#pragma once

#include <QObject>
#include <QString>

#include "HistoryListModel.h"

namespace medibridge::network { class ApiClient; }

namespace medibridge::controllers {

class HistoryController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool is_loading READ is_loading NOTIFY is_loading_changed)
    Q_PROPERTY(QString last_error READ last_error NOTIFY last_error_changed)
    Q_PROPERTY(int total_count READ total_count NOTIFY total_count_changed)
    Q_PROPERTY(QObject* items READ items_model CONSTANT)   // ⭐ QML 노출 Model

public:
    explicit HistoryController(network::ApiClient* api_client, QObject* parent = nullptr);

    bool is_loading() const;
    QString last_error() const;
    int total_count() const;

    QObject* items_model() { return &items_; }

    Q_INVOKABLE void record(const QString& item_code,
                            int quantity,
                            const QString& memo);
    Q_INVOKABLE void load_list(const QString& from_date,
                               const QString& to_date,
                               int page,
                               int page_size);

signals:
    void is_loading_changed();
    void last_error_changed();
    void total_count_changed();

    void record_succeeded();
    void record_failed(const QString& error_code);
    void list_loaded();
    void list_load_failed(const QString& error_code);

private:
    void set_loading(bool loading);

    network::ApiClient* api_client_;
    bool is_loading_ = false;
    QString last_error_;
    int total_count_ = 0;

    medibridge::models::HistoryListModel items_;
};

} // namespace medibridge::controllers
