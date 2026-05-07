#include "AdbDeviceMonitor.h"

#include <QLoggingCategory>
#include <QStringList>
#include <QRegularExpression>

namespace medibridge::phonelink {

AdbDeviceMonitor::AdbDeviceMonitor(int interval_ms, QObject* parent)
    : QObject(parent)
    , interval_ms_(interval_ms)
{
    timer_.setInterval(interval_ms_);
    connect(&timer_, &QTimer::timeout, this, &AdbDeviceMonitor::on_poll_timer);
    connect(&process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AdbDeviceMonitor::on_process_finished);
}

AdbDeviceMonitor::~AdbDeviceMonitor()
{
    stop();
}

void AdbDeviceMonitor::start()
{
    timer_.start();
    qInfo() << "[AdbDeviceMonitor] 시작 — 주기:" << interval_ms_ << "ms";
    refresh_now();
}

void AdbDeviceMonitor::stop()
{
    timer_.stop();
}

void AdbDeviceMonitor::refresh_now()
{
    if (process_.state() != QProcess::NotRunning) {
        return;     // 진행 중이면 스킵
    }
    process_.start("adb", {"devices"});
}

QString AdbDeviceMonitor::last_serial() const { return last_serial_; }
QString AdbDeviceMonitor::last_state()  const { return last_state_; }
bool    AdbDeviceMonitor::is_connected() const { return last_state_ == "device"; }

void AdbDeviceMonitor::on_poll_timer()
{
    refresh_now();
}

void AdbDeviceMonitor::on_process_finished(int /*exit_code*/, QProcess::ExitStatus /*status*/)
{
    const QString output = QString::fromUtf8(process_.readAllStandardOutput());
    const auto [serial, state] = parse_adb_output(output);

    // 상태 변경 시에만 시그널 발신
    if (serial != last_serial_ || state != last_state_) {
        last_serial_ = serial;
        last_state_ = state;
        qInfo().noquote()
            << "[AdbDeviceMonitor] device_changed — serial:"
            << (serial.isEmpty() ? "(없음)" : serial)
            << "state:" << (state.isEmpty() ? "(없음)" : state);
        emit device_changed(serial, state);
    }
}

QPair<QString, QString> AdbDeviceMonitor::parse_adb_output(const QString& output)
{
    // 예시 출력:
    //   List of devices attached
    //   R3CXXXXXXX     device
    //
    // 또는:
    //   List of devices attached
    //   R3CXXXXXXX     unauthorized
    //
    // 또는 (연결 없음):
    //   List of devices attached
    //
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& raw : lines) {
        const QString line = raw.trimmed();
        if (line.startsWith("List of devices") || line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split(QRegularExpression("\\s+"),
                                             Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            return {parts[0], parts[1]};
        }
    }
    return {"", ""};
}

} // namespace medibridge::phonelink
