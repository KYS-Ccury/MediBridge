// =====================================================
// PhoneServer 구현 — 골격 (TODO 주석으로 남겨진 부분이 분담 작업)
// =====================================================
#include "PhoneServer.h"
#include "../MainServerClient/ApiClient.h"

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
    // 본 메소드의 html_path 는 컴파일 시점 고정 상수.
    //
    // TODO: Resources.qrc 사용 — qrc:/Frontend/Static/Index.html 로 변경 권장
    //   (CMakeLists.txt 에 Frontend/Resources.qrc 등록되어 있음)
    const QString html_path = QStringLiteral("PhoneAdapter/Static/Index.html");
    QFile html_file(html_path);
    if (!html_file.open(QIODevice::ReadOnly)) {
        qWarning() << "[PhoneServer] Index.html 로드 실패:" << html_path;
        return QHttpServerResponse(
            QByteArrayLiteral("text/plain"),
            QByteArray("PoC PhoneAdapter — Index.html 로드 실패")
        );
    }
    return QHttpServerResponse(
        QByteArrayLiteral("text/html; charset=utf-8"),
        html_file.readAll()
    );
}

// =====================================================
// POST /v1/media/image — 이미지 업로드
// =====================================================
QHttpServerResponse PhoneServer::handle_media_image(const QHttpServerRequest& request)
{
    // TODO (영역 C 분담):
    //   1. multipart/form-data 또는 JSON+base64 파싱
    //   2. (선택) WorkerPool::instance().submit(...) 으로 디코딩·검증 위임
    //   3. api_client_->media().upload_image(image_data, mime_type, intent_hint, callback) 호출
    //   4. 즉시 202 Accepted 응답 (request_id 포함)
    //
    // 참고:
    //   - MediaApi.md §1
    //   - QHttpServerRequest::body() 로 raw 바이너리 접근
    //   - multipart 파싱은 QHttpMultiPart 또는 수동 파싱 필요
    // ⚠ 보안: 이미지 바이너리 본체 로깅 ❌. 크기만.
    qInfo() << "[PhoneServer] POST /v1/media/image — body size:"
            << request.body().size() << "bytes (TODO)";

    // ⚠ 보안: request_id 는 예측 불가능한 UUID 사용 (timestamp 추측 회피).
    QJsonObject response_obj{
        {"status", "accepted"},
        {"request_id", QStringLiteral("req_") +
                       QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {"todo", true}
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
QHttpServerResponse PhoneServer::handle_speech_utterance(const QHttpServerRequest& request)
{
    // TODO (영역 C 분담):
    //   1. JSON 파싱 → text, stt_confidence, context, image_request_id 추출
    //   2. api_client_->speech().send_utterance(...) 호출
    //   3. MainServer 응답을 그대로 폰에 반환
    //
    // 참고:
    //   - SpeechApi.md §1
    //   - QJsonDocument::fromJson(request.body())
    // ⚠ 보안: 사용자 발화 본문(PII) 로깅 X. 크기·해시 일부만.
    const auto body_size = request.body().size();
    qInfo() << "[PhoneServer] POST /v1/speech/utterance — body size:"
            << body_size << "bytes (본문 마스킹) (TODO)";

    QJsonObject response_obj{
        {"status", "ok"},
        {"todo", true}
    };
    return QHttpServerResponse(
        QByteArrayLiteral("application/json; charset=utf-8"),
        QJsonDocument(response_obj).toJson(QJsonDocument::Compact)
    );
}

// =====================================================
// GET /health — 자체 헬스체크
// =====================================================
QHttpServerResponse PhoneServer::handle_health() const
{
    // TODO (영역 C 분담):
    //   1. ADB 연결 상태 확인 (외부 프로세스 실행 또는 별도 모듈)
    //   2. ApiClient의 최근 MainServer 핑 결과 조회
    //   3. status: ok / degraded / down 판정
    //
    // 참고: MonitoringApi.md §1
    QJsonObject response_obj{
        {"status", "ok"},
        {"service", "client_phone_adapter"},
        {"version", "0.1.0"},
        {"checked_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"todo", true}
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
    // TODO (영역 C 분담):
    //   1. ResourceMonitor 의 최근 측정값 조회
    //   2. JSON 직렬화 (MonitoringApi.md §2)
    QJsonObject response_obj{
        {"service", "client_phone_adapter"},
        {"collected_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"todo", true}
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
