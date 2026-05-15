// =====================================================
// PillApiClient — /v1/pill/* 호출 (PillApi.md)
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <functional>

namespace medibridge::network {

class ApiClientCommon;

class PillApiClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray&, int)>;

    explicit PillApiClient(std::shared_ptr<ApiClientCommon> common,
                           QObject* parent = nullptr);

    void identify(const QString& image_request_id,
                  const QString& utterance_request_id,
                  bool include_dur_check,
                  JsonCallback callback);

    void get_pool(bool include_inactive, JsonCallback callback);

    void add_to_pool(const QString& item_code,
                     const QString& reg_method,
                     JsonCallback callback);

    void remove_from_pool(int pool_id, JsonCallback callback);

    /// X-Confirm-Reset 헤더 자동 첨부 (안전 가드)
    void reset_pool(JsonCallback callback);

    /// POST /v1/pill/onboarding/normalize — drug_name LIKE 검색
    /// round=1 첫 검색만 노출 (분기 확정 흐름은 별도 호출자 책임)
    void search_drug_name(const QString& utterance_text, JsonCallback callback);

private:
    std::shared_ptr<ApiClientCommon> common_;
};

} // namespace medibridge::network
