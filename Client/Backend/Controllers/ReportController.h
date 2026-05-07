// =====================================================
// ReportController — 통합 보고서 생성 (ViewModel)
// =====================================================
#pragma once

#include <QObject>
#include <QString>

namespace medibridge::network { class ApiClient; }

namespace medibridge::controllers {

class ReportController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool is_loading READ is_loading NOTIFY is_loading_changed)
    Q_PROPERTY(QString last_error READ last_error NOTIFY last_error_changed)
    Q_PROPERTY(QString last_pdf_path READ last_pdf_path NOTIFY last_pdf_path_changed)

public:
    explicit ReportController(network::ApiClient* api_client, QObject* parent = nullptr);

    bool is_loading() const;
    QString last_error() const;
    QString last_pdf_path() const;

    /// PDF/HTML/JSON 형식으로 보고서 생성
    Q_INVOKABLE void generate(const QString& from_date,
                              const QString& to_date,
                              const QString& format);

signals:
    void is_loading_changed();
    void last_error_changed();
    void last_pdf_path_changed();

    void generated(const QString& format, const QString& output_path);
    void generate_failed(const QString& error_code);

private:
    network::ApiClient* api_client_;
    bool is_loading_ = false;
    QString last_error_;
    QString last_pdf_path_;
};

} // namespace medibridge::controllers
