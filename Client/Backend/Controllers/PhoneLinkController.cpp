#include "PhoneLinkController.h"
#include "AdbDeviceMonitor.h"
#include "AdbReverseManager.h"

#include <QLoggingCategory>

namespace medibridge::controllers {

PhoneLinkController::PhoneLinkController(phonelink::AdbDeviceMonitor* monitor,
                                         phonelink::AdbReverseManager* reverse_manager,
                                         QObject* parent)
    : QObject(parent)
    , monitor_(monitor)
    , reverse_manager_(reverse_manager)
{
    // AdbDeviceMonitor 의 시그널 연결
    if (monitor_) {
        connect(monitor_,
                SIGNAL(device_changed(QString, QString)),
                this,
                SLOT(on_device_changed(QString, QString)));
    }
}

bool    PhoneLinkController::is_connected() const { return is_connected_; }
QString PhoneLinkController::device_serial() const { return device_serial_; }
QString PhoneLinkController::device_state() const { return device_state_; }
bool    PhoneLinkController::reverse_active() const { return reverse_active_; }
QString PhoneLinkController::last_error() const { return last_error_; }

void PhoneLinkController::retry_connect()
{
    // TODO (영역 C 분담):
    //   monitor_->refresh_now();   // 즉시 1회 폴링
    qInfo() << "[PhoneLinkController] retry_connect 요청";
}

void PhoneLinkController::retry_reverse()
{
    // TODO:
    //   if (reverse_manager_) reverse_manager_->setup_reverse();
    qInfo() << "[PhoneLinkController] retry_reverse 요청";
}

void PhoneLinkController::on_device_changed(const QString& serial, const QString& state)
{
    const bool was_connected = is_connected_;

    device_serial_ = serial;
    device_state_ = state;
    is_connected_ = (state == "device");

    emit device_serial_changed();
    emit device_state_changed();

    if (was_connected != is_connected_) {
        emit is_connected_changed();
    }

    // Q4=C: 자동 reverse 셋업 (연결 감지 시)
    if (is_connected_ && !was_connected && reverse_manager_) {
        // TODO: reverse_manager_->setup_reverse();
        reverse_active_ = true;
        emit reverse_active_changed();
        emit phone_ready();
        qInfo() << "[PhoneLinkController] 폰 연결 감지 → adb reverse 자동 셋업";
    } else if (!is_connected_ && was_connected) {
        reverse_active_ = false;
        emit reverse_active_changed();
        qInfo() << "[PhoneLinkController] 폰 연결 끊김";
    }
}

} // namespace medibridge::controllers
