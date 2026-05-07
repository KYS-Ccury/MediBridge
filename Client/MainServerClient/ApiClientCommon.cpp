// =====================================================
// ApiClientCommon — QNetworkAccessManager 기반 실 HTTP 호출
// =====================================================
#include "ApiClientCommon.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QLoggingCategory>
#include <QUuid>
#include <QDebug>

namespace medibridge::network {

ApiClientCommon::ApiClientCommon(const QString& base_url, QObject* parent)
    : QObject(parent)
    , base_url_(base_url)
    , network_manager_(new QNetworkAccessManager(this))
{
    if (base_url_.endsWith('/')) {
        base_url_.chop(1);
    }
    qInfo() << "[ApiClientCommon] base_url =" << base_url_;
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
    if (!content_type.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, content_type);
    }

    if (!access_token_.isEmpty()) {
        request.setRawHeader("Authorization",
                             ("Bearer " + access_token_).toUtf8());
    }
    request.setRawHeader("X-Request-ID",
                         QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());
    // 자동 redirect 추적
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

void ApiClientCommon::send_request(const QString& method,
                                   const QString& path,
                                   const QByteArray& body,
                                   const QString& content_type,
                                   JsonCallback callback,
                                   const QList<QPair<QByteArray, QByteArray>>& extra_headers)
{
    QNetworkRequest request = build_request(path, content_type);
    for (const auto& kv : extra_headers) {
        request.setRawHeader(kv.first, kv.second);
    }

    qInfo().nospace() << "[ApiClientCommon] → " << method << " " << path
                      << " (body " << body.size() << "B)";

    QNetworkReply* reply = nullptr;
    const QString verb = method.toUpper();
    if (verb == "GET") {
        reply = network_manager_->get(request);
    } else if (verb == "POST") {
        reply = network_manager_->post(request, body);
    } else if (verb == "PUT") {
        reply = network_manager_->put(request, body);
    } else if (verb == "DELETE") {
        // body 가 비어있어도 sendCustomRequest 로 일관 처리
        reply = network_manager_->sendCustomRequest(request, "DELETE", body);
    } else {
        reply = network_manager_->sendCustomRequest(request, verb.toUtf8(), body);
    }

    connect(reply, &QNetworkReply::finished, this, [this, reply, method, path, callback]() {
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray response_body = reply->readAll();
        const auto net_err = reply->error();

        qInfo().nospace() << "[ApiClientCommon] ← " << method << " " << path
                          << "  status=" << status
                          << "  body=" << response_body.size() << "B";

        if (net_err != QNetworkReply::NoError && status == 0) {
            // 네트워크 자체 실패 (DNS / 연결 거부 / 타임아웃 등)
            const QString reason = reply->errorString();
            qWarning() << "[ApiClientCommon] 네트워크 오류:" << reason;
            emit network_error(reason);
            if (callback) callback(QByteArray(), 0);
        } else {
            if (status == 401) {
                emit token_expired();
            }
            if (callback) callback(response_body, status);
        }

        reply->deleteLater();
    });
}

} // namespace medibridge::network
