#include "PillController.h"
#include "ApiClient.h"
#include "PhoneCaptureService.h"

#include <QLoggingCategory>

namespace medibridge::controllers {

PillController::PillController(network::ApiClient* api_client,
                               phonelink::PhoneCaptureService* phone_capture_service,
                               QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
    , phone_capture_service_(phone_capture_service)
{
    // PhoneCaptureService 시그널 연결 (PC 트리거 캡쳐 흐름)
    if (phone_capture_service_) {
        connect(phone_capture_service_,
                &phonelink::PhoneCaptureService::capture_succeeded,
                this, &PillController::on_capture_succeeded);
        connect(phone_capture_service_,
                &phonelink::PhoneCaptureService::capture_failed,
                this, &PillController::on_capture_failed);
    }
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
// PC 트리거 캡쳐 흐름 — capture_and_identify
// =====================================================
void PillController::capture_and_identify()
{
    if (is_loading_) return;

    // 💡 직접 capture_screen()을 호출하지 않고 "로딩 상태"만 켭니다.
    // 그러면 스트리밍 중인 다음 프레임이 왔을 때 on_capture_succeeded에서 가로챕니다.
    set_loading(true); 
    set_error("");
    qInfo() << "[PillController] 촬영 요청 수락 — 다음 스트리밍 프레임을 대기합니다.";
}

void PillController::on_capture_succeeded(const QByteArray& png_data)
{
    // 💡 사용자가 촬영 버튼을 눌렀을 때(is_loading_ == true)만 서버로 업로드합니다.
    if (!is_loading_) return; 

    qInfo() << "[PillController] 스트리밍 프레임 캡처 성공 — 서버 업로드 시작";
    
    api_client_->media().upload_image(png_data, "image/png", "identify",
        [this](const QByteArray& response, int status_code) {
            handle_identify_response(response, status_code);
        });
}

void PillController::on_capture_failed(const QString& error_message)
{
    set_loading(false);
    set_error(QStringLiteral("CAPTURE_FAILED: ") + error_message);
    qWarning() << "[PillController] 캡쳐 실패 —" << error_message;
    emit identify_failed(last_error_);
}

void PillController::handle_identify_response(const QByteArray& response, int status_code)
{
    Q_UNUSED(response);
    set_loading(false);

    // MainServer TODO 단계엔 501 반환 가능 — 그때도 흐름 검증 위해 임시 결과 채움
    if (status_code != 200 && status_code != 202) {
        set_error(QStringLiteral("UPLOAD_FAILED_") + QString::number(status_code));
        emit identify_failed(last_error_);
        return;
    }

    // TODO (영역 C 분담):
    //   1. JSON 파싱 → request_id, candidates[], confidence_tier, guidance.tts_text, dur_check
    //   2. ListModel 갱신 (PillCandidateListModel, DurDetailListModel)
    //   3. 본 임시값 제거
    confidence_tier_ = "MEDIUM";
    tts_text_ = QStringLiteral("응답 수신 완료 (응답 파싱 TODO)");
    dur_result_ = "no_risk_found";
    last_request_id_ = QStringLiteral("req_pending");

    emit confidence_tier_changed();
    emit tts_text_changed();
    emit dur_result_changed();
    emit last_request_id_changed();
    emit identify_succeeded();
}

// =====================================================
// 폰 PWA 흐름 — identify (request_id 기반)
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
            handle_identify_response(response, status_code);
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
            Q_UNUSED(response);
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
