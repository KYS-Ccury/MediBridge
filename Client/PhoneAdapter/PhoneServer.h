// =====================================================
// PhoneServer — 폰(S24) PWA 수신용 HTTP 서버
// =====================================================
// 폰 브라우저가 adb reverse 또는 LAN으로 접속.
//
// 라우트:
//   GET  /                         → PWA 페이지(Index.html) 서빙
//   POST /v1/media/image           → 폰 카메라 이미지 수신 (multipart)
//   POST /v1/speech/utterance      → 폰 STT 결과 텍스트 수신 (JSON)
//   GET  /health                   → 본 어댑터 헬스체크 (Monitoring API)
//   GET  /metrics                  → 본 어댑터 메트릭 (Monitoring API)
//
// 본 서버는 폰 ↔ Client 어댑터 역할만이며,
// 실제 비즈니스 로직은 ApiClient를 통해 MainServer로 전달.
// =====================================================
#pragma once

#include <QObject>
#include <QHttpServer>
#include <memory>

class QTcpServer;

namespace medibridge::network {
class ApiClient;
}
namespace medibridge::services {
class UtteranceForwarder;
}

namespace medibridge::phone {

/**
 * @brief 폰 PWA를 위한 HTTP 서버.
 *
 * 메인 스레드 소유. 요청 처리는 QHttpServer 내부 스레드 풀이 자동 분산.
 * 무거운 작업(이미지 디코딩 등)은 Threading::WorkerPool 으로 위임 권장.
 */
class PhoneServer : public QObject
{
    Q_OBJECT
public:
    /**
     * @param api_client MainServer 호출용 클라이언트 (생명 주기는 외부 관리)
     * @param utterance_forwarder STT 텍스트를 메인서버 forward + GUI 시그널 발신 (외부 소유, nullptr 가능 — 없으면 ApiClient 직접 호출 fallback)
     * @param parent QObject 부모
     */
    explicit PhoneServer(network::ApiClient* api_client,
                         services::UtteranceForwarder* utterance_forwarder = nullptr,
                         QObject* parent = nullptr);
    ~PhoneServer() override;

    /**
     * @brief 지정 포트로 HTTP 서버 시작.
     * @param port 리스닝 포트 (기본 8000)
     * @return 성공 시 true
     */
    bool start_server(quint16 port);

    /**
     * @brief 서버 정지 + TCP 소켓 해제.
     */
    void stop_server();

    /**
     * @brief 현재 리스닝 중인 포트 (0이면 미시작).
     */
    quint16 listening_port() const;

private:
    /// 라우트 등록 (생성자에서 호출)
    void register_routes();

    /// GET / 핸들러 — PWA 페이지 서빙
    QHttpServerResponse handle_root() const;

    /// POST /v1/media/image 핸들러 — 이미지 업로드
    QHttpServerResponse handle_media_image(const QHttpServerRequest& request);

    /// POST /v1/speech/utterance 핸들러 — STT 텍스트
    QHttpServerResponse handle_speech_utterance(const QHttpServerRequest& request);

    /// GET /health 핸들러 — 자체 헬스체크
    QHttpServerResponse handle_health() const;

    /// GET /metrics 핸들러 — 자체 메트릭
    QHttpServerResponse handle_metrics() const;

    network::ApiClient* api_client_;          // MainServer 호출용 (외부 소유, 비-소유 포인터)
    services::UtteranceForwarder* utterance_forwarder_;   // STT 텍스트 forward + GUI 시그널 (외부 소유, nullable)
    std::unique_ptr<QHttpServer> http_server_;
    QTcpServer* tcp_server_;                  // QHttpServer가 소유
    quint16 listening_port_;
};

} // namespace medibridge::phone
