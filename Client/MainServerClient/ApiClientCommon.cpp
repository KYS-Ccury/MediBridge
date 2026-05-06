#include "ApiClientCommon.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QLoggingCategory>
#include <QUuid>

namespace medibridge::network {

ApiClientCommon::ApiClientCommon(const QString& base_url, QObject* parent)
    : QObject(parent)
    , base_url_(base_url)
    , network_manager_(new QNetworkAccessManager(this))
{
    if (base_url_.endsWith('/')) {
        base_url_.chop(1);
    }
}

ApiClientCommon::~ApiClientCommon() = default;

void ApiClientCommon::set_access_token(const QString& token)
{
    access_token_ = token;
    qInfo() << "[ApiClientCommon] JWT 등록됨 (length:" << token.length() << ")";
}

void ApiClientCommon::clear_access_token()
{
    access_token_.clear();
    qInfo() << "[ApiClientCommon] JWT 삭제됨";
}

QString ApiClientCommon::access_token() const { return access_token_; }
bool ApiClientCommon::is_authenticated() const { return !access_token_.isEmpty(); }
QString ApiClientCommon::base_url() const { return base_url_; }
QNetworkAccessManager* ApiClientCommon::network_manager() const { return network_manager_; }

QNetworkRequest ApiClientCommon::build_request(const QString& path,
                                               const QString& content_type) const
{
    const QUrl url(base_url_ + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, content_type);

    if (!access_token_.isEmpty()) {
        request.setRawHeader("Authorization",
                             ("Bearer " + access_token_).toUtf8());
    }
    request.setRawHeader("X-Request-ID",
                         QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());
    return request;
}

void ApiClientCommon::send_request(const QString& method,
                                   const QString& path,
                                   const QByteArray& body,
                                   const QString& content_type,
                                   JsonCallback callback,
                                   const QList<QPair<QByteArray, QByteArray>>& extra_headers)
{
    // TODO (영역 C 분담):
    //   1. build_request(path, content_type) 으로 요청 생성
    //   2. extra_headers 추가 첨부
    //   3. method 분기: post/get/deleteResource() 등
    //   4. QNetworkReply::finished → 콜백 호출
    //   5. 401 감지 시 token_expired emit
    //   6. 네트워크 에러 시 network_error emit
    Q_UNUSED(method); Q_UNUSED(path); Q_UNUSED(body);
    Q_UNUSED(content_type); Q_UNUSED(extra_headers);
    qInfo() << "[ApiClientCommon] send_request — TODO" << method << path;
    if (callback) callback(QByteArray("{\"todo\":true}"), 200);
}

} // namespace medibridge::network
