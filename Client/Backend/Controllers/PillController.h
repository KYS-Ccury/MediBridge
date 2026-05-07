// =====================================================
// PillController — 알약 식별 + DUR + 약 풀 관리 (ViewModel)
// =====================================================
// 두 가지 식별 흐름 지원:
//   1. PC 트리거 캡쳐 — capture_and_identify()
//      adb screencap → 메모리 PNG → 메인서버 업로드 → 결과 표시
//   2. 폰 PWA 흐름 — PhoneServer 가 받은 사진을 자동 처리 (TODO)
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

namespace medibridge::network { class ApiClient; }
namespace medibridge::phonelink { class PhoneCaptureService; }

namespace medibridge::controllers {

class PillController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool is_loading READ is_loading NOTIFY is_loading_changed)
    Q_PROPERTY(QString last_error READ last_error NOTIFY last_error_changed)

    // 식별 결과
    Q_PROPERTY(QString confidence_tier READ confidence_tier NOTIFY confidence_tier_changed)
    Q_PROPERTY(QString tts_text READ tts_text NOTIFY tts_text_changed)
    Q_PROPERTY(QString dur_result READ dur_result NOTIFY dur_result_changed)
    Q_PROPERTY(QString last_request_id READ last_request_id NOTIFY last_request_id_changed)

public:
    /**
     * @param api_client            메인서버 호출 클라이언트 (외부 소유)
     * @param phone_capture_service PC 트리거 화면 캡쳐 (외부 소유, nullptr 가능)
     */
    explicit PillController(network::ApiClient* api_client,
                            phonelink::PhoneCaptureService* phone_capture_service,
                            QObject* parent = nullptr);

    // 접근자
    bool is_loading() const;
    QString last_error() const;
    QString confidence_tier() const;
    QString tts_text() const;
    QString dur_result() const;
    QString last_request_id() const;

    // QML 호출
    /// ⭐ PC 에서 폰 화면 캡쳐 → 메인서버 업로드 → 식별 결과 자동 표시
    Q_INVOKABLE void capture_and_identify();

    Q_INVOKABLE void identify(const QString& image_request_id,
                              const QString& utterance_request_id);
    Q_INVOKABLE void load_pool(bool include_inactive);
    Q_INVOKABLE void add_to_pool(const QString& item_code, const QString& reg_method);
    Q_INVOKABLE void remove_from_pool(int pool_id);
    Q_INVOKABLE void reset_pool();   // X-Confirm-Reset 자동 첨부

signals:
    void is_loading_changed();
    void last_error_changed();
    void confidence_tier_changed();
    void tts_text_changed();
    void dur_result_changed();
    void last_request_id_changed();

    void identify_succeeded();
    void identify_failed(const QString& error_code);
    void pool_loaded();
    void pool_load_failed(const QString& error_code);
    void pool_changed();

private slots:
    /// PhoneCaptureService::capture_succeeded 수신
    void on_capture_succeeded(const QByteArray& png_data);
    /// PhoneCaptureService::capture_failed 수신
    void on_capture_failed(const QString& error_message);

private:
    void set_loading(bool loading);
    void set_error(const QString& error_code);
    /// 메인서버 업로드 응답 파싱·결과 채움 (공통)
    void handle_identify_response(const QByteArray& response, int status_code);

    network::ApiClient* api_client_;
    phonelink::PhoneCaptureService* phone_capture_service_;
    bool is_loading_ = false;
    QString last_error_;
    QString confidence_tier_;
    QString tts_text_;
    QString dur_result_;
    QString last_request_id_;
};

} // namespace medibridge::controllers
