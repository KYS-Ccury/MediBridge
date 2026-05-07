#include "PillController.h"
#include "../../MainServerClient/ApiClient.h"

#include <QLoggingCategory>

namespace medibridge::controllers {

PillController::PillController(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

bool    PillController::is_loading() const { return is_loading_; }
QString PillController::last_error() const { return last_error_; }
QString PillController::confidence_tier() const { return confidence_tier_; }
QString PillController::tts_text() const { return tts_text_; }
QString PillController::dur_result() const { return dur_result_; }
QString PillController::last_request_id() const { return last_request_id_; }

void PillController::set_loading(bool loading)
{
    if (is_loading_ == loading) return;
    is_loading_ = loading;
    emit is_loading_changed();
}

void PillController::set_error(const QString& error_code)
{
    last_error_ = error_code;
    emit last_error_changed();
}

// =====================================================
// 식별 + DUR
// =====================================================
void PillController::identify(const QString& image_request_id,
                              const QString& utterance_request_id)
{
    set_loading(true);
    set_error("");
    last_request_id_ = image_request_id;
    emit last_request_id_changed();

    api_client_->pill().identify(image_request_id, utterance_request_id, true,
        [this](const QByteArray& response, int status_code) {
            set_loading(false);

            if (status_code != 200) {
                set_error("IDENTIFY_FAILED");
                emit identify_failed(last_error_);
                return;
            }

            // TODO (영역 C 분담):
            //   1. JSON 파싱 → confidence_tier, guidance.tts_text, dur_check
            //   2. 후보 리스트 → PillCandidateListModel 갱신
            //   3. DUR detail 리스트 → DurDetailListModel 갱신
            confidence_tier_ = "MEDIUM";
            tts_text_ = "식별 결과 후보가 여러 개입니다.";
            dur_result_ = "no_risk_found";
            emit confidence_tier_changed();
            emit tts_text_changed();
            emit dur_result_changed();
            emit identify_succeeded();
        });
}

// =====================================================
// 약 풀 CRUD
// =====================================================
void PillController::load_pool(bool include_inactive)
{
    set_loading(true);
    api_client_->pill().get_pool(include_inactive,
        [this](const QByteArray& response, int status_code) {
            set_loading(false);
            if (status_code != 200) {
                set_error("POOL_LOAD_FAILED");
                emit pool_load_failed(last_error_);
                return;
            }
            // TODO: PoolItemListModel 에 데이터 채우기
            emit pool_loaded();
        });
}

void PillController::add_to_pool(const QString& item_code, const QString& reg_method)
{
    set_loading(true);
    api_client_->pill().add_to_pool(item_code, reg_method,
        [this](const QByteArray& /*response*/, int status_code) {
            set_loading(false);
            if (status_code == 201) {
                emit pool_changed();
            } else {
                set_error("ADD_POOL_FAILED");
            }
        });
}

void PillController::remove_from_pool(int pool_id)
{
    set_loading(true);
    api_client_->pill().remove_from_pool(pool_id,
        [this](const QByteArray& /*response*/, int status_code) {
            set_loading(false);
            if (status_code == 204) {
                emit pool_changed();
            } else {
                set_error("REMOVE_POOL_FAILED");
            }
        });
}

void PillController::reset_pool()
{
    set_loading(true);
    api_client_->pill().reset_pool(
        [this](const QByteArray& /*response*/, int status_code) {
            set_loading(false);
            if (status_code == 204) {
                emit pool_changed();
            } else {
                set_error("RESET_POOL_FAILED");
            }
        });
}

} // namespace medibridge::controllers
