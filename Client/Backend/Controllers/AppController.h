// =====================================================
// AppController — 전역 상태·페이지 라우팅 (ViewModel)
// =====================================================
// QML에서 stack.push/pop 외 전역 알림(toast)·로딩 오버레이를 관리.
// 다른 컨트롤러의 결과를 통합하여 페이지 전환 트리거.
// =====================================================
#pragma once

#include <QObject>
#include <QString>

namespace medibridge::controllers {

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString current_page READ current_page NOTIFY current_page_changed)
    Q_PROPERTY(bool is_global_loading READ is_global_loading NOTIFY is_global_loading_changed)
    Q_PROPERTY(QString last_toast_message READ last_toast_message NOTIFY last_toast_message_changed)

public:
    explicit AppController(QObject* parent = nullptr);

    // 접근자
    QString current_page() const;
    bool is_global_loading() const;
    QString last_toast_message() const;

    // QML 호출
    Q_INVOKABLE void navigate_to(const QString& page_name);
    Q_INVOKABLE void show_toast(const QString& message);
    Q_INVOKABLE void set_global_loading(bool loading);

signals:
    void current_page_changed();
    void is_global_loading_changed();
    void last_toast_message_changed();
    void toast_requested(const QString& message);     // QML Connections에서 수신해 토스트 띄움

private:
    QString current_page_;
    bool is_global_loading_ = false;
    QString last_toast_message_;
};

} // namespace medibridge::controllers
