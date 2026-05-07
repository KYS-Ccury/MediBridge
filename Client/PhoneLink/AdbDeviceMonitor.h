// =====================================================
// AdbDeviceMonitor — `adb devices` 주기 폴링으로 폰 USB 연결 상태 감지
// =====================================================
// 주기적으로 외부 프로세스(adb.exe)를 실행해 출력 파싱.
// 결과 변경 시 device_changed 시그널 발신 → PhoneLinkController 가 수신.
// =====================================================
#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QProcess>

namespace medibridge::phonelink {

class AdbDeviceMonitor : public QObject
{
    Q_OBJECT
public:
    explicit AdbDeviceMonitor(int interval_ms, QObject* parent = nullptr);
    ~AdbDeviceMonitor() override;

    /// 주기 폴링 시작
    void start();
    /// 정지
    void stop();
    /// 즉시 1회 폴링 (수동 재연결 시도)
    void refresh_now();

    QString last_serial() const;
    QString last_state() const;
    bool is_connected() const;

signals:
    /**
     * 폰 연결 상태 변경 시 발신.
     *
     * @param serial 디바이스 시리얼 (없으면 빈 문자열)
     * @param state "device" / "unauthorized" / "offline" / "" (연결 없음)
     */
    void device_changed(const QString& serial, const QString& state);

private slots:
    void on_poll_timer();
    void on_process_finished(int exit_code, QProcess::ExitStatus status);

private:
    /// `adb devices` 출력 파싱 → (serial, state)
    static QPair<QString, QString> parse_adb_output(const QString& output);

    QTimer timer_;
    int interval_ms_;
    QProcess process_;
    QString last_serial_;
    QString last_state_;
};

} // namespace medibridge::phonelink
