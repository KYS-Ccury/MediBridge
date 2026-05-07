#include "HistoryController.h"
#include "../../MainServerClient/ApiClient.h"

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
        [this](const QByteArray& /*response*/, int status_code) {
            set_loading(false);
            if (status_code == 201) {
                emit record_succeeded();
            } else {
                last_error_ = "RECORD_FAILED";
                emit last_error_changed();
                emit record_failed(last_error_);
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
                last_error_ = "LIST_LOAD_FAILED";
                emit last_error_changed();
                emit list_load_failed(last_error_);
                return;
            }
            // TODO: HistoryListModel 갱신, total_count_ 추출
            emit list_loaded();
        });
}

} // namespace medibridge::controllers
