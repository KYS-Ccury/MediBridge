// =====================================================
// ApiClientCommon — 모든 카테고리 클라이언트가 공유하는 공통 송신·인증
// =====================================================
// 패턴 B (조합 패턴): 카테고리별 클라이언트(AuthApiClient, PillApiClient...)는
// 본 객체를 std::shared_ptr 로 공유하여 같은 base_url, JWT, network_manager 사용.
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <functional>

class QNetworkAccessManager;
class QNetworkRequest;

namespace medibridge::network {

class ApiClientCommon : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray& response_body, int status_code)>;

    explicit ApiClientCommon(const QString& base_url, QObject* parent = nullptr);
    ~ApiClientCommon() override;

    // ----- JWT 토큰 -----
    void set_access_token(const QString& token);
    void clear_access_token();
    QString access_token() const;
    bool is_authenticated() const;

    // ----- 공통 송신 -----
    /// 모든 하위 클라이언트가 본 메소드로 송신
    void send_request(const QString& method,
                      const QString& path,
                      const QByteArray& body,
                      const QString& content_type,
                      JsonCallback callback,
                      const QList<QPair<QByteArray, QByteArray>>& extra_headers = {});

    QString base_url() const;
    QNetworkAccessManager* network_manager() const;

signals:
    /// 401 응답 감지 시 발신 — UI에서 재로그인 유도
    void token_expired();
    /// 네트워크 자체 실패
    void network_error(const QString& reason);

private:
    QNetworkRequest build_request(const QString& path,
                                  const QString& content_type) const;

    QString base_url_;
    QString access_token_;
    QNetworkAccessManager* network_manager_;
};

} // namespace medibridge::network
