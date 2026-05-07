// =====================================================
// AdbReverseManager — adb reverse tcp:N tcp:N 자동 셋업
// =====================================================
// 폰 연결 감지 시 호출되어 폰의 localhost:port → PC localhost:port 포워딩 등록.
// USB 케이블만 있으면 폰 브라우저가 PC PhoneAdapter 에 접속 가능.
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <QProcess>

namespace medibridge::phonelink {

class AdbReverseManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @param port 포워딩할 포트 (PhoneAdapter 와 동일)
     */
    explicit AdbReverseManager(quint16 port, QObject* parent = nullptr);
    ~AdbReverseManager() override;

    /// adb reverse 등록 (폰 연결 감지 시 자동 호출)
    void setup_reverse();

    /// adb reverse --remove-all (폰 연결 해제 시)
    void remove_all_reverses();

    bool is_active() const;

signals:
    void reverse_setup_succeeded();
    void reverse_setup_failed(const QString& error);

private slots:
    void on_setup_finished(int exit_code, QProcess::ExitStatus status);

private:
    quint16  port_;
    QProcess process_;
    bool     is_active_ = false;
};

} // namespace medibridge::phonelink
