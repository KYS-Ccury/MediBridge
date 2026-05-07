#include "PhoneCaptureService.h"

#include <QLoggingCategory>

namespace medibridge::phonelink {

// PNG 시그니처 (8바이트) — 캡쳐 결과 검증용
static const QByteArray PNG_SIGNATURE =
    QByteArray::fromHex("89504E470D0A1A0A");

PhoneCaptureService::PhoneCaptureService(QObject* parent)
    : QObject(parent)
{
    // QProcess 시그널 연결
    connect(&process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PhoneCaptureService::on_finished);
    connect(&process_, &QProcess::errorOccurred,
            this, &PhoneCaptureService::on_error_occurred);
}

PhoneCaptureService::~PhoneCaptureService()
{
    if (process_.state() != QProcess::NotRunning) {
        process_.kill();
        process_.waitForFinished(1000);
    }
}

bool PhoneCaptureService::is_capturing() const
{
    return process_.state() != QProcess::NotRunning;
}

void PhoneCaptureService::capture_screen()
{
    if (process_.state() != QProcess::NotRunning) {
        emit capture_failed(QStringLiteral("이전 캡쳐 진행 중"));
        return;
    }

    qInfo() << "[PhoneCaptureService] adb exec-out screencap -p 실행";

    // ⚠ 보안: 인자는 고정값 (사용자 입력 X) — 커맨드 인젝션 방지
    // exec-out 모드: stdout 으로 raw PNG 출력 (개행 변환 X — Windows 안전)
    process_.start("adb", {"exec-out", "screencap", "-p"});
}

void PhoneCaptureService::on_finished(int exit_code, QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit || exit_code != 0) {
        const QString stderr_text = QString::fromUtf8(process_.readAllStandardError());
        const QString msg = QStringLiteral("ADB 종료 코드 %1 — %2")
                                .arg(exit_code).arg(stderr_text.left(200));
        qWarning() << "[PhoneCaptureService]" << msg;
        emit capture_failed(msg);
        return;
    }

    QByteArray png_data = process_.readAllStandardOutput();

    // PNG 시그니처 검증 (폰 미연결 시 빈 데이터 또는 텍스트 반환 가능)
    if (png_data.size() < 8 || !png_data.startsWith(PNG_SIGNATURE)) {
        const QString msg = QStringLiteral("PNG 헤더 검증 실패 (크기 %1, 폰 미연결?)")
                                .arg(png_data.size());
        qWarning() << "[PhoneCaptureService]" << msg;
        emit capture_failed(msg);
        return;
    }

    qInfo() << "[PhoneCaptureService] 캡쳐 완료 —" << png_data.size() << "bytes";
    emit capture_succeeded(png_data);
}

void PhoneCaptureService::on_error_occurred(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
        case QProcess::FailedToStart:
            msg = QStringLiteral("adb 실행 파일을 찾을 수 없음 (PATH 등록 확인)");
            break;
        case QProcess::Crashed:
            msg = QStringLiteral("adb 프로세스 비정상 종료");
            break;
        case QProcess::Timedout:
            msg = QStringLiteral("adb 응답 타임아웃");
            break;
        default:
            msg = QStringLiteral("adb 알 수 없는 에러: ") + QString::number(error);
    }
    qWarning() << "[PhoneCaptureService]" << msg;
    emit capture_failed(msg);
}

} // namespace medibridge::phonelink
