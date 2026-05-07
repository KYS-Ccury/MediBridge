// =====================================================
// PhoneLinkController — 폰 USB 연결 상태 (ViewModel)
// =====================================================
// AdbDeviceMonitor·AdbReverseManager 의 결과를 QML에 노출.
// QML의 PhoneStatusIndicator 컴포넌트가 본 컨트롤러를 바인딩.
// =====================================================
#pragma once

#include <QObject>
#include <QString>

namespace medibridge::phonelink {
    class AdbDeviceMonitor;
    class AdbReverseManager;
}

namespace medibridge::controllers {

class PhoneLinkController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool is_connected READ is_connected NOTIFY is_connected_changed)
    Q_PROPERTY(QString device_serial READ device_serial NOTIFY device_serial_changed)
    Q_PROPERTY(QString device_state READ device_state NOTIFY device_state_changed)
    Q_PROPERTY(bool reverse_active READ reverse_active NOTIFY reverse_active_changed)
    Q_PROPERTY(QString last_error READ last_error NOTIFY last_error_changed)

public:
    explicit PhoneLinkController(phonelink::AdbDeviceMonitor* monitor,
                                 phonelink::AdbReverseManager* reverse_manager,
                                 QObject* parent = nullptr);

    bool is_connected() const;
    QString device_serial() const;
    QString device_state() const;
    bool reverse_active() const;
    QString last_error() const;

    /// 수동 재연결 시도 (Q4=C 옵션)
    Q_INVOKABLE void retry_connect();

    /// 수동 adb reverse 재실행
    Q_INVOKABLE void retry_reverse();

signals:
    void is_connected_changed();
    void device_serial_changed();
    void device_state_changed();
    void reverse_active_changed();
    void last_error_changed();

    /// 폰 연결 → 자동 reverse 성공 시 발신 (사용자에게 토스트 표시)
    void phone_ready();

private slots:
    /// AdbDeviceMonitor의 device_changed 시그널 수신
    void on_device_changed(const QString& serial, const QString& state);

private:
    phonelink::AdbDeviceMonitor*  monitor_;
    phonelink::AdbReverseManager* reverse_manager_;
    bool is_connected_ = false;
    QString device_serial_;
    QString device_state_;       // "device" / "unauthorized" / "offline" / ""
    bool reverse_active_ = false;
    QString last_error_;
};

} // namespace medibridge::controllers
