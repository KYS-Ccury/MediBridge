#include "AppController.h"

#include <QLoggingCategory>

namespace medibridge::controllers {

AppController::AppController(QObject* parent)
    : QObject(parent)
    , current_page_("LoginPage")
{
}

QString AppController::current_page() const { return current_page_; }
bool    AppController::is_global_loading() const { return is_global_loading_; }
QString AppController::last_toast_message() const { return last_toast_message_; }

void AppController::navigate_to(const QString& page_name)
{
    if (current_page_ == page_name) return;
    current_page_ = page_name;
    qInfo() << "[AppController] navigate_to:" << page_name;
    emit current_page_changed();
}

void AppController::show_toast(const QString& message)
{
    last_toast_message_ = message;
    emit last_toast_message_changed();
    emit toast_requested(message);
}

void AppController::set_global_loading(bool loading)
{
    if (is_global_loading_ == loading) return;
    is_global_loading_ = loading;
    emit is_global_loading_changed();
}

} // namespace medibridge::controllers
