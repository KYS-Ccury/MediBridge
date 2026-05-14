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
            
    // 💡 [추가됨] 윈도우에서 adb를 찾지 못할 때 원인을 파악하기 위한 에러 처리
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            qCritical() << "[AdbDeviceMonitor] ❌ adb 실행 실패! (Windows 환경 변수 PATH에 platform-tools 경로가 등록되어 있는지 확인하세요)";
        } else {
            qWarning() << "[AdbDeviceMonitor] adb 프로세스 에러:" << error;
        }
    });
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
    // 윈도우에서는 "adb" (또는 "adb.exe")로 실행
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

// 💡 [수정됨] 데몬 시작 메시지 등 불필요한 출력을 무시하도록 견고하게 개선
QPair<QString, QString> AdbDeviceMonitor::parse_adb_output(const QString& output)
{
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    bool list_started = false;

    for (const QString& raw : lines) {
        const QString line = raw.trimmed();

        // "List of devices attached" 이후의 줄만 기기 정보로 취급
        if (line.startsWith("List of devices")) {
            list_started = true;
            continue;
        }

        // 데몬 시작 로그나 공백은 무시
        if (!list_started || line.startsWith("* daemon") || line.isEmpty()) {
            continue;
        }

        const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            // 시리얼 넘버가 '*' 같은 특수문자로 시작하지 않는지 한 번 더 가드
            if (!parts[0].startsWith('*')) {
                return {parts[0], parts[1]};
            }
        }
    }
    return {"", ""};
}

} // namespace medibridge::phonelink