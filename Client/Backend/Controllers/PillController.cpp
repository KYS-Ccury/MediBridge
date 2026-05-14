#include "PillController.h"
#include "ApiClient.h"
#include "PhoneCaptureService.h"

#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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

// =====================================================
// ⭐ 정상 사진 흐름 (MediaApi v0.2) — 4단계 콜백 체인
// =====================================================
//   ① POST /v1/media/intent       → photo_id, storage_url, put_token, request_id
//   ② PUT  <storage_url>           → 보관 PC(10.10.10.122:8004) 직접 업로드
//   ③ POST /v1/media/commit       → status PENDING → READY
//   ④ POST /v1/pill/identify      → request_id 로 추론 요청
//
// 사진 본체는 메인서버를 통과하지 않음 — 메인은 토큰만 발급.
// =====================================================
void PillController::on_capture_succeeded(const QByteArray& png_data)
{
    // 💡 사용자가 촬영 버튼을 눌렀을 때(is_loading_ == true)만 서버로 업로드합니다.
    if (!is_loading_) return;

    qInfo() << "[PillController] 스트리밍 프레임 캡처 성공 — ① /v1/media/intent 요청";

    const QString mime    = QStringLiteral("image/png");
    const qint64  size_b  = png_data.size();
    const QString purpose = QStringLiteral("IDENTIFY");

    // ① POST /v1/media/intent
    api_client_->media().request_intent(mime, size_b, purpose,
        [this, png_data, mime](const QByteArray& intent_resp, int intent_status) {
            if (intent_status != 201) {
                set_loading(false);
                set_error(QStringLiteral("INTENT_FAILED_") + QString::number(intent_status));
                emit identify_failed(last_error_);
                return;
            }

            const auto j = QJsonDocument::fromJson(intent_resp).object();
            const QString photo_id    = j.value("photo_id").toString();
            const QString storage_url = j.value("storage_url").toString();
            const QString put_token   = j.value("put_token").toString();
            const QString req_id      = j.value("request_id").toString();

            if (photo_id.isEmpty() || storage_url.isEmpty()
                || put_token.isEmpty() || req_id.isEmpty()) {
                set_loading(false);
                set_error("INTENT_INVALID_RESPONSE");
                emit identify_failed(last_error_);
                return;
            }

            qInfo() << "[PillController] ① intent OK photo_id=" << photo_id
                    << "→ ② PUT" << storage_url;

            // ② PUT <storage_url> — 보관 PC 직접
            api_client_->media().put_to_storage(storage_url, put_token, png_data, mime,
                [this, photo_id, req_id](const QByteArray& put_resp, int put_status) {
                    Q_UNUSED(put_resp);
                    if (put_status != 201) {
                        set_loading(false);
                        set_error(QStringLiteral("STORAGE_PUT_FAILED_")
                                  + QString::number(put_status));
                        emit identify_failed(last_error_);
                        return;
                    }

                    qInfo() << "[PillController] ② PUT 보관 PC OK → ③ /v1/media/commit";

                    // ③ POST /v1/media/commit
                    api_client_->media().commit_upload(photo_id,
                        [this, req_id](const QByteArray& commit_resp, int commit_status) {
                            Q_UNUSED(commit_resp);
                            if (commit_status != 200) {
                                set_loading(false);
                                set_error(QStringLiteral("COMMIT_FAILED_")
                                          + QString::number(commit_status));
                                emit identify_failed(last_error_);
                                return;
                            }

                            qInfo() << "[PillController] ③ commit OK request_id=" << req_id
                                    << "→ ④ /v1/pill/identify";

                            // ④ POST /v1/pill/identify (기존 흐름)
                            api_client_->pill().identify(req_id, /*utterance_id=*/QString(), true,
                                [this](const QByteArray& resp, int status) {
                                    handle_identify_response(resp, status);
                                });
                        });
                });
        });
}

void PillController::on_capture_failed(const QString& error_message)
{
    set_loading(false);
    set_error(QStringLiteral("CAPTURE_FAILED: ") + error_message);
    qWarning() << "[PillController] 캡쳐 실패 —" << error_message;
    emit identify_failed(last_error_);
}

// =====================================================
// handle_identify_response — /v1/pill/identify 응답 파싱
// =====================================================
// 응답 schema (PillApi v0.3):
//   {
//     "request_id": "req_xxx",
//     "candidates": [{ item_code, drug_name, confidence, match_keys[], in_user_pool,
//                      classification_name, efficacy_text, usage_text }],
//     "confidence_tier": "HIGH|MEDIUM|LOW",
//     "guidance": { tts_text, fallback_action, ... },
//     "dur_check": { result: "no_risk_found|risk_found", details[] }
//   }
// =====================================================
void PillController::handle_identify_response(const QByteArray& response, int status_code)
{
    set_loading(false);

    if (status_code != 200 && status_code != 202) {
        // 에러 envelope 에서 code 추출
        QString code = QStringLiteral("IDENTIFY_FAILED_") + QString::number(status_code);
        const auto err_doc = QJsonDocument::fromJson(response);
        if (err_doc.isObject() && err_doc.object().contains("error")) {
            code = err_doc.object().value("error").toObject()
                          .value("code").toString(code);
        }
        set_error(code);
        emit identify_failed(last_error_);
        qWarning() << "[PillController] identify 실패 status=" << status_code
                   << "code=" << code;
        return;
    }

    const auto doc = QJsonDocument::fromJson(response);
    if (!doc.isObject()) {
        set_error("IDENTIFY_INVALID_JSON");
        emit identify_failed(last_error_);
        return;
    }
    const auto root = doc.object();

    // 기본 필드
    const QString req_id = root.value("request_id").toString();
    if (!req_id.isEmpty()) {
        last_request_id_ = req_id;
        emit last_request_id_changed();
    }

    // confidence_tier
    confidence_tier_ = root.value("confidence_tier").toString("MEDIUM");
    emit confidence_tier_changed();

    // guidance.tts_text
    const auto guidance = root.value("guidance").toObject();
    tts_text_ = guidance.value("tts_text").toString();
    if (tts_text_.isEmpty()) {
        tts_text_ = QStringLiteral("식별 결과를 확인해주세요.");
    }
    emit tts_text_changed();

    // dur_check
    const auto dur = root.value("dur_check").toObject();
    dur_result_ = dur.value("result").toString("no_risk_found");
    emit dur_result_changed();

    // candidates → PillCandidateListModel
    const auto cands_arr = root.value("candidates").toArray();
    QVector<models::PillCandidate> cands;
    cands.reserve(cands_arr.size());
    for (const auto& v : cands_arr) {
        const auto co = v.toObject();
        models::PillCandidate c;
        c.item_code    = co.value("item_code").toString();
        c.drug_name    = co.value("drug_name").toString();
        c.confidence   = co.value("confidence").toDouble();
        c.in_user_pool = co.value("in_user_pool").toBool(false);
        QStringList mk;
        for (const auto& m : co.value("match_keys").toArray()) mk << m.toString();
        c.match_keys = mk.join(", ");
        cands.push_back(c);
    }
    candidates_.set_candidates(cands);

    // dur_check.details → DurDetailListModel
    const auto details_arr = dur.value("details").toArray();
    QVector<models::DurDetail> details;
    details.reserve(details_arr.size());
    for (const auto& v : details_arr) {
        const auto d = v.toObject();
        models::DurDetail x;
        x.dur_type        = d.value("dur_type").toString();
        x.drug_a_name     = d.value("drug_a_name").toString();
        x.drug_b_name     = d.value("drug_b_name").toString();
        x.prohibit_reason = d.value("prohibit_reason").toString();
        x.action_message  = d.value("action_message").toString();
        details.push_back(x);
    }
    dur_details_.set_details(details);

    qInfo().nospace() << "[PillController] identify OK — candidates=" << cands.size()
                      << " dur_details=" << details.size()
                      << " tier=" << confidence_tier_
                      << " dur=" << dur_result_;

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
// =====================================================
// load_pool — /v1/pill/pool 응답 파싱
// =====================================================
// 응답 schema:
//   { "items": [{ pool_id, item_code, drug_name, reg_method, is_active,
//                 user_category, classification_name, created_at }],
//     "total_count": N }
// =====================================================
void PillController::load_pool(bool include_inactive)
{
    set_loading(true);
    api_client_->pill().get_pool(include_inactive,
        [this](const QByteArray& response, int status_code) {
            set_loading(false);
            if (status_code != 200) {
                QString code = "POOL_LOAD_FAILED";
                const auto err_doc = QJsonDocument::fromJson(response);
                if (err_doc.isObject() && err_doc.object().contains("error")) {
                    code = err_doc.object().value("error").toObject()
                                  .value("code").toString(code);
                }
                set_error(code);
                emit pool_load_failed(last_error_);
                return;
            }
            const auto doc = QJsonDocument::fromJson(response);
            if (!doc.isObject()) {
                set_error("POOL_INVALID_JSON");
                emit pool_load_failed(last_error_);
                return;
            }
            const auto items_arr = doc.object().value("items").toArray();
            const int total      = doc.object().value("total_count").toInt(items_arr.size());

            QVector<models::PoolItem> items;
            items.reserve(items_arr.size());
            for (const auto& v : items_arr) {
                const auto o = v.toObject();
                models::PoolItem p;
                p.pool_id    = o.value("pool_id").toInt();
                p.item_code  = o.value("item_code").toString();
                p.drug_name  = o.value("drug_name").toString();
                p.reg_method = o.value("reg_method").toString();
                p.is_active  = o.value("is_active").toBool(true);
                p.created_at = o.value("created_at").toString();
                items.push_back(p);
            }
            pool_items_.set_items(items);

            qInfo().nospace() << "[PillController] pool 로드 OK — items=" << items.size()
                              << " total=" << total;
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
