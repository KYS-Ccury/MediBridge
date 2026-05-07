#include "AdbReverseManager.h"

#include <QLoggingCategory>

namespace medibridge::phonelink {

AdbReverseManager::AdbReverseManager(quint16 port, QObject* parent)
    : QObject(parent)
    , port_(port)
{
    connect(&process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AdbReverseManager::on_setup_finished);
}

AdbReverseManager::~AdbReverseManager() = default;

void AdbReverseManager::setup_reverse()
{
    if (process_.state() != QProcess::NotRunning) {
        return;
    }

    // adb reverse tcp:8000 tcp:8000
    const QString port_str = QString("tcp:%1").arg(port_);
    process_.start("adb", {"reverse", port_str, port_str});
    qInfo() << "[AdbReverseManager] adb reverse 실행 — port:" << port_;
}

void AdbReverseManager::remove_all_reverses()
{
    QProcess::execute("adb", {"reverse", "--remove-all"});
    is_active_ = false;
    qInfo() << "[AdbReverseManager] adb reverse --remove-all";
}

bool AdbReverseManager::is_active() const { return is_active_; }

void AdbReverseManager::on_setup_finished(int exit_code, QProcess::ExitStatus status)
{
    const bool success = (status == QProcess::NormalExit && exit_code == 0);
    const QString stderr_output = QString::fromUtf8(process_.readAllStandardError());

    if (success) {
        is_active_ = true;
        qInfo() << "[AdbReverseManager] reverse 등록 성공 — port:" << port_;
        emit reverse_setup_succeeded();
    } else {
        is_active_ = false;
        qWarning() << "[AdbReverseManager] reverse 등록 실패 — "
                   << "exit:" << exit_code
                   << "stderr:" << stderr_output;
        emit reverse_setup_failed(stderr_output);
    }
}

} // namespace medibridge::phonelink
