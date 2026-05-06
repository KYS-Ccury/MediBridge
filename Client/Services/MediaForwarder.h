// =====================================================
// MediaForwarder — 폰 입력을 MainServer로 중계하는 비즈니스 로직
// =====================================================
// PhoneAdapter가 받은 raw 데이터(이미지·STT 텍스트)를
// MainServer 호출 가능한 형태로 가공·전달하는 책임.
//
// 분리 이유:
//   - PhoneAdapter는 HTTP 라우팅·파싱만 담당 (얇게 유지)
//   - 이미지 검증·재인코딩 같은 가공은 본 클래스에 격리
//   - 추후 라우팅 정책 변경(예: 일부는 MainServer, 일부는 직접 추론서버) 시
//     본 클래스만 수정 → 다른 모듈 영향 없음
// =====================================================
#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <functional>

namespace medibridge::network { class ApiClient; }

namespace medibridge::services {

/**
 * @brief 폰 입력을 MainServer로 중계.
 *
 * 본 객체는 메인 스레드에 머무르며,
 * 무거운 가공은 Threading::WorkerPool 으로 위임.
 */
class MediaForwarder : public QObject
{
    Q_OBJECT
public:
    using ForwardCallback = std::function<void(const QByteArray& main_server_response,
                                               int status_code)>;

    /**
     * @param api_client MainServer 호출 클라이언트 (외부 소유)
     * @param parent QObject 부모
     */
    explicit MediaForwarder(network::ApiClient* api_client,
                            QObject* parent = nullptr);

    /**
     * @brief 폰에서 받은 이미지를 MainServer로 전달.
     *
     * 흐름:
     *   1. (선택) WorkerPool에서 이미지 검증·리사이즈
     *   2. ApiClient::upload_image 호출
     *   3. 응답을 callback으로 반환
     *
     * @param image_data raw 이미지 바이트
     * @param mime_type 예: "image/jpeg"
     * @param intent_hint "identify" / "register" 등
     * @param callback MainServer 응답 수신 콜백
     */
    void forward_image(const QByteArray& image_data,
                       const QString& mime_type,
                       const QString& intent_hint,
                       ForwardCallback callback);

    /**
     * @brief 폰에서 받은 STT 텍스트를 MainServer로 전달.
     *
     * @param text 폰 STT 결과
     * @param stt_confidence 신뢰도 (없으면 -1.0)
     * @param context "daily_use" / "onboarding" / "confirmation" 등
     * @param image_request_id 결합할 이미지 ID (없으면 빈 문자열)
     * @param callback MainServer 응답 수신 콜백
     */
    void forward_utterance(const QString& text,
                           double stt_confidence,
                           const QString& context,
                           const QString& image_request_id,
                           ForwardCallback callback);

private:
    network::ApiClient* api_client_;   // 외부 소유, 비-소유 포인터
};

} // namespace medibridge::services
