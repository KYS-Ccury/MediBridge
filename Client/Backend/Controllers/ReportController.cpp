#include "ReportController.h"
#include "../../MainServerClient/ApiClient.h"

namespace medibridge::controllers {

ReportController::ReportController(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

bool    ReportController::is_loading() const { return is_loading_; }
QString ReportController::last_error() const { return last_error_; }
QString ReportController::last_pdf_path() const { return last_pdf_path_; }

void ReportController::generate(const QString& from_date,
                                const QString& to_date,
                                const QString& format)
{
    is_loading_ = true;
    emit is_loading_changed();

    api_client_->report().generate(from_date, to_date, format,
        [this, format](const QByteArray& response, int status_code) {
            is_loading_ = false;
            emit is_loading_changed();

            if (status_code != 200) {
                last_error_ = "REPORT_GENERATE_FAILED";
                emit last_error_changed();
                emit generate_failed(last_error_);
                return;
            }

            // TODO (영역 C 분담):
            //   - format == "pdf" → 응답 바이너리를 ./reports/ 에 저장
            //   - format == "html" → 임시 .html 파일로 저장 후 시스템 브라우저로 열기
            //   - format == "json" → 데이터 모델에 채우기
            last_pdf_path_ = "TODO_PDF_PATH";
            emit last_pdf_path_changed();
            emit generated(format, last_pdf_path_);
        });
}

} // namespace medibridge::controllers
