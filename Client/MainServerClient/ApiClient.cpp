#include "ApiClient.h"
#include "ApiClientCommon.h"

namespace medibridge::network {

ApiClient::ApiClient(const QString& base_url, QObject* parent)
    : QObject(parent)
    , common_(std::make_shared<ApiClientCommon>(base_url, this))
    , auth_(common_, this)
    , pill_(common_, this)
    , history_(common_, this)
    , report_(common_, this)
    , media_(common_, this)
    , speech_(common_, this)
    , monitoring_(common_, this)
{
    // Common의 시그널을 ApiClient의 시그널로 전파
    connect(common_.get(), &ApiClientCommon::token_expired,
            this, &ApiClient::token_expired);
    connect(common_.get(), &ApiClientCommon::network_error,
            this, &ApiClient::network_error);
}

ApiClient::~ApiClient() = default;

void ApiClient::set_access_token(const QString& token)
{
    common_->set_access_token(token);
}

void ApiClient::clear_access_token()
{
    common_->clear_access_token();
}

QString ApiClient::access_token() const
{
    return common_->access_token();
}

bool ApiClient::is_authenticated() const
{
    return common_->is_authenticated();
}

} // namespace medibridge::network
