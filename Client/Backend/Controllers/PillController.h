// =====================================================
// PillController — 알약 식별 + DUR + 약 풀 관리 (ViewModel)
// =====================================================
// 폰에서 받은 사진은 PhoneServer가 직접 처리하고,
// 본 컨트롤러는 식별 결과 표시·약 풀 CRUD UI를 담당.
// =====================================================
#pragma once

#include <QObject>
#include <QString>

namespace medibridge::network { class ApiClient; }

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
    explicit PillController(network::ApiClient* api_client, QObject* parent = nullptr);

    // 접근자
    bool is_loading() const;
    QString last_error() const;
    QString confidence_tier() const;
    QString tts_text() const;
    QString dur_result() const;
    QString last_request_id() const;

    // QML 호출
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
    void pool_changed();      // 약 풀 변경 (추가/삭제/리셋) 시 발신

private:
    void set_loading(bool loading);
    void set_error(const QString& error_code);

    network::ApiClient* api_client_;
    bool is_loading_ = false;
    QString last_error_;
    QString confidence_tier_;
    QString tts_text_;
    QString dur_result_;
    QString last_request_id_;
};

} // namespace medibridge::controllers
