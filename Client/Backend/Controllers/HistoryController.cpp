#include "HistoryController.h"
#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

namespace medibridge::controllers {

HistoryController::HistoryController(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

bool    HistoryController::is_loading() const { return is_loading_; }
QString HistoryController::last_error() const { return last_error_; }
int     HistoryController::total_count() const { return total_count_; }

void HistoryController::set_loading(bool loading)
{
    if (is_loading_ == loading) return;
    is_loading_ = loading;
    emit is_loading_changed();
}

void HistoryController::record(const QString& item_code,
                               int quantity,
                               const QString& memo)
{
    set_loading(true);
    api_client_->history().record(item_code, quantity, memo,
        [this](const QByteArray& response, int status_code) {
            set_loading(false);
            if (status_code == 201) {
                qInfo() << "[HistoryController] record OK";
                emit record_succeeded();
            } else {
                QString code = QStringLiteral("RECORD_FAILED_") + QString::number(status_code);
                const auto err_doc = QJsonDocument::fromJson(response);
                if (err_doc.isObject() && err_doc.object().contains("error")) {
                    code = err_doc.object().value("error").toObject()
                                  .value("code").toString(code);
                }
                last_error_ = code;
                emit last_error_changed();
                emit record_failed(last_error_);
                qWarning() << "[HistoryController] record 실패 status=" << status_code
                           << "code=" << code;
            }
        });
}

void HistoryController::load_list(const QString& from_date,
                                  const QString& to_date,
                                  int page,
                                  int page_size)
{
    set_loading(true);
    api_client_->history().list(from_date, to_date, page, page_size,
        [this](const QByteArray& response, int status_code) {
            set_loading(false);
            if (status_code != 200) {
                QString code = QStringLiteral("LIST_LOAD_FAILED_") + QString::number(status_code);
                const auto err_doc = QJsonDocument::fromJson(response);
                if (err_doc.isObject() && err_doc.object().contains("error")) {
                    code = err_doc.object().value("error").toObject()
                                  .value("code").toString(code);
                }
                last_error_ = code;
                emit last_error_changed();
                emit list_load_failed(last_error_);
                return;
            }
            // 응답 schema:
            //   { "items": [{ intake_id, item_code, drug_name, intake_datetime,
            //                 quantity, memo, ... }], "total_count": N }
            const auto doc = QJsonDocument::fromJson(response);
            if (!doc.isObject()) {
                last_error_ = "LIST_INVALID_JSON";
                emit last_error_changed();
                emit list_load_failed(last_error_);
                return;
            }
            const auto items = doc.object().value("items").toArray();
            total_count_ = doc.object().value("total_count").toInt(items.size());
            emit total_count_changed();
            qInfo().nospace() << "[HistoryController] list 로드 OK — items=" << items.size()
                              << " total=" << total_count_;
            // TODO Phase 2-B: HistoryListModel 갱신
            emit list_loaded();
        });
}

} // namespace medibridge::controllers
