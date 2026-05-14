// =====================================================
// PhoneServer 구현 — 폰 PWA ↔ PC ↔ 메인서버 중계
// =====================================================
// /media/image  : 폰 카메라 raw binary 수신 → ImageForwarder (intent→PUT→commit→identify)
// /speech/utter : 폰 STT 텍스트 수신 → UtteranceForwarder (메인서버 utterance 전송)
// /health,/metrics : 폰 PWA 가 PC 상태 점검 시 폴링
// /           : PWA Index.html 서빙 (7-path fallback)
// =====================================================
#include "PhoneServer.h"
#include "ApiClient.h"

#include <QCoreApplication>
#include <QFile>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QTcpServer>
#include <QHostAddress>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QUuid>
#include <QStringList>

namespace medibridge::phone {

// =====================================================
// 생성자 / 소멸자
// =====================================================
PhoneServer::PhoneServer(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
    , http_server_(std::make_unique<QHttpServer>())
    , tcp_server_(nullptr)
    , listening_port_(0)
{
    // 라우트 등록 (생성자에서 1회)
    register_routes();
}

PhoneServer::~PhoneServer()
{
    stop_server();
}

// =====================================================
// 라우트 등록
// =====================================================
void PhoneServer::register_routes()
{
    // GET /
    http_server_->route("/",
        [this](const QHttpServerRequest&) {
            return handle_root();
        });

    // POST /v1/media/image
    http_server_->route("/v1/media/image",
        QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& req) {
            return handle_media_image(req);
        });

    // POST /v1/speech/utterance
    http_server_->route("/v1/speech/utterance",
        QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& req) {
            return handle_speech_utterance(req);
        });

    // GET /health
    http_server_->route("/health",
        [this](const QHttpServerRequest&) {
            return handle_health();
        });

    // GET /metrics
    http_server_->route("/metrics",
        [this](const QHttpServerRequest&) {
            return handle_metrics();
        });
}

// =====================================================
// GET / — PWA 페이지 서빙
// =====================================================
QHttpServerResponse PhoneServer::handle_root() const
{
    // ⚠ 보안: 경로는 절대 사용자 입력으로 만들지 않음 (path traversal 방지).
    // 모든 후보 경로는 컴파일 시점 고정 상수.
    //
    // 빌드 환경에 따라 working directory 가 다를 수 있어 여러 후보 시도:
    //   - Qt Creator 기본: build/Desktop_Qt_*/  (한 단계 위가 프로젝트 루트)
    //   - 직접 실행:     현재 디렉터리 또는 .exe 옆
    //   - 배포:          .exe 옆에 PhoneAdapter/Static 복사
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QStringLiteral("PhoneAdapter/Static/Index.html"),                   // working dir
        appDir + QStringLiteral("/PhoneAdapter/Static/Index.html"),         // .exe 옆
        appDir + QStringLiteral("/../PhoneAdapter/Static/Index.html"),      // build/ 한 단계 위
        appDir + QStringLiteral("/../../PhoneAdapter/Static/Index.html"),   // build/Desktop_*/  두 단계 위
        appDir + QStringLiteral("/../Client/PhoneAdapter/Static/Index.html"),
        appDir + QStringLiteral("/../../Client/PhoneAdapter/Static/Index.html"),
        QStringLiteral(":/PhoneAdapter/Static/Index.html"),                 // qrc (등록 시)
    };

    for (const QString& path : candidates) {
        QFile f(path);
        if (f.exists() && f.open(QIODevice::ReadOnly)) {
            qInfo() << "[PhoneServer] Index.html 로드 OK:" << path;
            return QHttpServerResponse(
                QByteArrayLiteral("text/html; charset=utf-8"),
                f.readAll()
            );
        }
    }

    qWarning() << "[PhoneServer] Index.html 모든 후보 경로 실패. appDir=" << appDir;
    return QHttpServerResponse(
        QByteArrayLiteral("text/plain; charset=utf-8"),
        QByteArray("PoC PhoneAdapter — Index.html 로드 실패. "
                   "PhoneAdapter/Static/Index.html 을 실행 디렉터리 또는 .exe 옆에 두세요.")
    );
}

// =====================================================
// POST /v1/media/image — 이미지 업로드
// =====================================================
// =====================================================
// POST /v1/media/image — 폰 PWA 로부터 raw 바이너리 수신
// =====================================================
// PWA Index.html 가 multipart 대신 raw binary 로 보내므로 단순 처리:
//   request.body()  → image bytes 그대로
//   Content-Type   → mime_type
//
// 받은 즉시 메인서버 신규 사진 흐름 (intent → 보관 PC PUT → commit) 을 트리거.
// 폰 PWA 에는 즉시 202 Accepted + request_id 응답 (fire-and-forget).
// 결과 (식별까지) 는 PC GUI 측에서 별도 진행 또는 추후 polling.
// =====================================================
QHttpServerResponse PhoneServer::handle_media_image(const QHttpServerRequest& request)
{
    const QByteArray image_data = request.body();
    if (image_data.isEmpty()) {
        QJsonObject err{{"error", "EMPTY_BODY"}};
        return QHttpServerResponse(
            QByteArrayLiteral("application/json; charset=utf-8"),
            QJsonDocument(err).toJson(QJsonDocument::Compact),
            QHttpServerResponse::StatusCode::BadRequest);
    }

    // Content-Type 헤더에서 mime 추출 (기본 image/jpeg)
    QString mime_type = QStringLiteral("image/jpeg");
    for (const auto& [name, value] : request.headers()) {
        if (QByteArray(name.data(), int(name.size())).toLower() == "content-type") {
            const QString v = QString::fromUtf8(QByteArray(value.data(), int(value.size())));
            if (v.contains("png", Qt::CaseInsensitive))       mime_type = "image/png";
            else if (v.contains("jpeg", Qt::CaseInsensitive)
                  || v.contains("jpg",  Qt::CaseInsensitive)) mime_type = "image/jpeg";
            break;
        }
    }

    const QString req_id = QStringLiteral("req_phone_")
                         + QUuid::createUuid().toString(QUuid::WithoutBraces).left(16);

    qInfo().nospace() << "[PhoneServer] POST /v1/media/image — bytes=" << image_data.size()
                      << " mime=" << mime_type << " req_id=" << req_id;

    // ⭐ 신규 흐름 트리거 (intent → PUT → commit)
    //   비동기 콜백 체인. 폰 PWA 에는 즉시 응답 후 백그라운드 진행.
    if (api_client_) {
        const qint64 size_b = image_data.size();
        api_client_->media().request_intent(mime_type, size_b, "IDENTIFY",
            [this, image_data, mime_type, req_id]
            (const QByteArray& intent_resp, int intent_status) {
                if (intent_status != 201) {
                    qWarning() << "[PhoneServer] phone-flow intent 실패 status=" << intent_status;
                    return;
                }
                const auto j = QJsonDocument::fromJson(intent_resp).object();
                const QString photo_id    = j.value("photo_id").toString();
                const QString storage_url = j.value("storage_url").toString();
                const QString put_token   = j.value("put_token").toString();
                if (photo_id.isEmpty() || storage_url.isEmpty() || put_token.isEmpty()) {
                    qWarning() << "[PhoneServer] phone-flow intent 응답 불완전";
                    return;
                }
                qInfo() << "[PhoneServer] phone-flow ① intent OK photo_id=" << photo_id;

                api_client_->media().put_to_storage(
                    storage_url, put_token, image_data, mime_type,
                    [this, photo_id](const QByteArray&, int put_status) {
                        if (put_status != 201) {
                            qWarning() << "[PhoneServer] phone-flow PUT 실패 status=" << put_status;
                            return;
                        }
                        qInfo() << "[PhoneServer] phone-flow ② PUT OK → commit";

                        api_client_->media().commit_upload(photo_id,
                            [photo_id](const QByteArray&, int commit_status) {
                                if (commit_status == 200) {
                                    qInfo() << "[PhoneServer] phone-flow ③ commit OK photo_id="
                                            << photo_id << "— PC GUI 가 identify 트리거 가능";
                                } else {
                                    qWarning() << "[PhoneServer] phone-flow commit 실패 status="
                                               << commit_status;
                                }
                            });
                    });
            });
    }

    QJsonObject response_obj{
        {"status",     "accepted"},
        {"request_id", req_id},
        {"bytes",      image_data.size()},
        {"mime_type",  mime_type}
    };
    return QHttpServerResponse(
        QByteArrayLiteral("application/json; charset=utf-8"),
        QJsonDocument(response_obj).toJson(QJsonDocument::Compact),
        QHttpServerResponse::StatusCode::Accepted
    );
}

// =====================================================
// POST /v1/speech/utterance — STT 텍스트
// =====================================================
// =====================================================
// POST /v1/speech/utterance — 폰 STT 텍스트 수신 → 메인서버 forward
// =====================================================
// 받은 즉시 메인서버에 비동기 forward (fire-and-forget). 폰에는 202 응답.
// ⚠ 보안: 사용자 발화 본문(PII) 로깅 X. 크기만.
// =====================================================
QHttpServerResponse PhoneServer::handle_speech_utterance(const QHttpServerRequest& request)
{
    const auto body = request.body();
    if (body.isEmpty()) {
        QJsonObject err{{"error", "EMPTY_BODY"}};
        return QHttpServerResponse(
            QByteArrayLiteral("application/json; charset=utf-8"),
            QJsonDocument(err).toJson(QJsonDocument::Compact),
            QHttpServerResponse::StatusCode::BadRequest);
    }

    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        QJsonObject err{{"error", "INVALID_JSON"}};
        return QHttpServerResponse(
            QByteArrayLiteral("application/json; charset=utf-8"),
            QJsonDocument(err).toJson(QJsonDocument::Compact),
            QHttpServerResponse::StatusCode::BadRequest);
    }
    const auto obj = doc.object();
    const QString text    = obj.value("text").toString();
    const double  conf    = obj.value("stt_confidence").toDouble(1.0);
    const QString context = obj.value("context").toString("daily_use");
    const QString img_id  = obj.value("image_request_id").toString();

    if (text.trimmed().isEmpty()) {
        QJsonObject err{{"error", "EMPTY_TEXT"}};
        return QHttpServerResponse(
            QByteArrayLiteral("application/json; charset=utf-8"),
            QJsonDocument(err).toJson(QJsonDocument::Compact),
            QHttpServerResponse::StatusCode::BadRequest);
    }

    qInfo().nospace() << "[PhoneServer] POST /v1/speech/utterance — len=" << text.size()
                      << " ctx=" << context;

    // Fire-and-forget — 메인서버에 forward, 응답은 클라 GUI 측 controller 가 처리
    if (api_client_) {
        api_client_->speech().send_utterance(text, conf, context, img_id,
            [](const QByteArray& resp, int status) {
                qInfo().nospace() << "[PhoneServer] speech forward status="
                                  << status << " body=" << resp.size() << "B";
            });
    }

    QJsonObject response_obj{
        {"status", "accepted"},
        {"chars",  text.size()}
    };
    return QHttpServerResponse(
        QByteArrayLiteral("application/json; charset=utf-8"),
        QJsonDocument(response_obj).toJson(QJsonDocument::Compact),
        QHttpServerResponse::StatusCode::Accepted
    );
}

// =====================================================
// GET /health — 자체 헬스체크
// =====================================================
QHttpServerResponse PhoneServer::handle_health() const
{
    // PhoneServer 가 살아있고 ApiClient 가 인증되어 있으면 ok.
    const bool authed = (api_client_ && api_client_->is_authenticated());
    QJsonObject response_obj{
        {"status",      "ok"},
        {"service",     "client_phone_adapter"},
        {"version",     "0.1.0"},
        {"checked_at",  QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"listening_port", static_cast<int>(listening_port_)},
        {"authenticated",  authed}
    };
    return QHttpServerResponse(
        QByteArrayLiteral("application/json; charset=utf-8"),
        QJsonDocument(response_obj).toJson(QJsonDocument::Compact)
    );
}

// =====================================================
// GET /metrics — 자체 메트릭
// =====================================================
QHttpServerResponse PhoneServer::handle_metrics() const
{
    // PhoneAdapter 자체는 무거운 자원 측정 안 함. 기본 카운터만.
    QJsonObject response_obj{
        {"service",        "client_phone_adapter"},
        {"collected_at",   QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"listening_port", static_cast<int>(listening_port_)}
    };
    return QHttpServerResponse(
        QByteArrayLiteral("application/json; charset=utf-8"),
        QJsonDocument(response_obj).toJson(QJsonDocument::Compact)
    );
}

// =====================================================
// 시작 / 정지 / 포트 조회
// =====================================================
bool PhoneServer::start_server(quint16 port)
{
    tcp_server_ = new QTcpServer(this);
    if (!tcp_server_->listen(QHostAddress::Any, port)) {
        qCritical() << "[PhoneServer] 포트 리스닝 실패:" << port
                    << "에러:" << tcp_server_->errorString();
        delete tcp_server_;
        tcp_server_ = nullptr;
        return false;
    }
    if (!http_server_->bind(tcp_server_)) {
        qCritical() << "[PhoneServer] HTTP 서버 바인딩 실패";
        tcp_server_->close();
        return false;
    }
    listening_port_ = tcp_server_->serverPort();
    qInfo() << "[PhoneServer] 시작됨 — 포트:" << listening_port_;
    return true;
}

void PhoneServer::stop_server()
{
    if (tcp_server_) {
        tcp_server_->close();
        listening_port_ = 0;
        qInfo() << "[PhoneServer] 정지됨";
    }
}

quint16 PhoneServer::listening_port() const
{
    return listening_port_;
}

} // namespace medibridge::phone
