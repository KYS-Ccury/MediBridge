#include "ReportController.h"
#include "ApiClient.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QUrl>

namespace medibridge::controllers {

ReportController::ReportController(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

bool    ReportController::is_loading() const { return is_loading_; }
QString ReportController::last_error() const { return last_error_; }
QString ReportController::last_pdf_path() const { return last_pdf_path_; }

// =====================================================
// 응답 형식별 처리
//   pdf  : 응답 바이너리를 <Documents>/MediBridge/reports/ 에 저장
//   html : 임시 .html 로 저장 후 시스템 기본 브라우저로 열기
//   json : 모델에 채움 (현재는 last_pdf_path_ 빈값 + generated 시그널만)
// =====================================================
void ReportController::generate(const QString& from_date,
                                const QString& to_date,
                                const QString& format)
{
    is_loading_ = true;
    emit is_loading_changed();

    api_client_->report().generate(from_date, to_date, format,
        [this, format, from_date, to_date]
        (const QByteArray& response, int status_code) {
            is_loading_ = false;
            emit is_loading_changed();

            if (status_code != 200) {
                QString code = QStringLiteral("REPORT_GENERATE_FAILED_") + QString::number(status_code);
                const auto err_doc = QJsonDocument::fromJson(response);
                if (err_doc.isObject() && err_doc.object().contains("error")) {
                    code = err_doc.object().value("error").toObject()
                                  .value("code").toString(code);
                }
                last_error_ = code;
                emit last_error_changed();
                emit generate_failed(last_error_);
                qWarning() << "[ReportController] generate 실패 status=" << status_code
                           << "code=" << code;
                return;
            }

            // 저장 경로 — 사용자 Documents/MediBridge/reports/
            const QString docs_dir = QStandardPaths::writableLocation(
                QStandardPaths::DocumentsLocation);
            const QString reports_dir = docs_dir + QStringLiteral("/MediBridge/reports");
            QDir().mkpath(reports_dir);

            const QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            const QString fmt_l = format.toLower();

            if (fmt_l == "pdf") {
                const QString path = QStringLiteral("%1/medibridge_%2_%3_%4.pdf")
                    .arg(reports_dir, from_date, to_date, ts);
                QFile f(path);
                if (!f.open(QIODevice::WriteOnly)) {
                    last_error_ = "PDF_WRITE_FAILED";
                    emit last_error_changed();
                    emit generate_failed(last_error_);
                    qWarning() << "[ReportController] PDF 쓰기 실패:" << path;
                    return;
                }
                f.write(response);
                f.close();
                last_pdf_path_ = path;
                emit last_pdf_path_changed();
                qInfo() << "[ReportController] PDF 저장:" << path << "(" << response.size() << "B)";

                // 시스템 PDF 뷰어로 열기 (선택)
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                emit generated(format, last_pdf_path_);
                return;
            }
            if (fmt_l == "html") {
                const QString path = QStringLiteral("%1/medibridge_%2_%3_%4.html")
                    .arg(reports_dir, from_date, to_date, ts);
                QFile f(path);
                if (!f.open(QIODevice::WriteOnly)) {
                    last_error_ = "HTML_WRITE_FAILED";
                    emit last_error_changed();
                    emit generate_failed(last_error_);
                    return;
                }
                f.write(response);
                f.close();
                last_pdf_path_ = path;
                emit last_pdf_path_changed();
                qInfo() << "[ReportController] HTML 저장:" << path << "(" << response.size() << "B)";

                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                emit generated(format, last_pdf_path_);
                return;
            }

            // json — 파일 저장 안 함. 응답을 모델로 노출 (Phase 2-B 에서 모델 추가 가능)
            last_pdf_path_.clear();
            emit last_pdf_path_changed();
            qInfo() << "[ReportController] JSON 응답 수신 (" << response.size() << "B)";
            emit generated(format, last_pdf_path_);
        });
}

} // namespace medibridge::controllers
