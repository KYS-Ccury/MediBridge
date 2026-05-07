// =====================================================
// PhoneCaptureService — `adb exec-out screencap -p` 로 폰 화면 PNG 캡쳐
// =====================================================
// 폰 갤러리·파일 저장·SCP 회수 절차 ❌.
// stdout 으로 PNG 바이너리 직접 수신 → 메모리에서 즉시 사용.
//
// 사용 시나리오:
//   1. 사용자가 폰 카메라 앱을 띄워 알약을 비춤
//   2. PC GUI "촬영" 버튼 클릭
//   3. 본 서비스가 adb 호출 → PNG 바이트 → capture_succeeded 시그널
//   4. PillController 가 받아서 메인서버로 업로드
// =====================================================
#pragma once

#include <QObject>
#include <QByteArray>
#include <QProcess>
#include <QString>

namespace medibridge::phonelink {

class PhoneCaptureService : public QObject
{
    Q_OBJECT
public:
    explicit PhoneCaptureService(QObject* parent = nullptr);
    ~PhoneCaptureService() override;

    /// 캡쳐 시작 (비동기). 결과는 capture_succeeded / capture_failed 시그널.
    Q_INVOKABLE void capture_screen();

    /// 진행 중인지
    bool is_capturing() const;

signals:
    /// 캡쳐 성공 — PNG 바이너리 전달
    void capture_succeeded(const QByteArray& png_data);

    /// 캡쳐 실패
    void capture_failed(const QString& error_message);

private slots:
    void on_finished(int exit_code, QProcess::ExitStatus status);
    void on_error_occurred(QProcess::ProcessError error);

private:
    QProcess process_;
};

} // namespace medibridge::phonelink
