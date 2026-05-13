// =====================================================
// ReportHtmlRenderer — 구현
// =====================================================
// 정책:
//   - PDF 변환(WeasyPrint·wkhtmltopdf 등) 시 그대로 인쇄 가능하도록 @media print 적용
//   - 부작용 인용은 식약처 본문 그대로 (LLM 변환 ❌)
//   - 사용자 식별자(user_id, email) 는 상단 안내에만 표기, anonymous_id 는 노출 X
//   - HTML escape — XSS 방어 (drug_name·memo·식약처 본문 모두)
// =====================================================
#include "ReportHtmlRenderer.h"

#include <sstream>
#include <string>

namespace medibridge::services::report {

namespace {

/// HTML special chars escape (& < > " ')
std::string esc(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;
        }
    }
    return out;
}

/// "2026-05-13T15:30:00Z" → "2026-05-13 15:30" (UI 가독성)
std::string fmt_dt(const std::string& iso)
{
    if (iso.size() < 16) return esc(iso);
    return esc(iso.substr(0, 10) + " " + iso.substr(11, 5));
}

/// 식약처 본문 줄바꿈 → <br> 변환 (escape 후)
std::string nl2br(const std::string& s)
{
    const std::string e = esc(s);
    std::string r;
    r.reserve(e.size());
    for (char c : e) {
        if (c == '\n')      r += "<br>";
        else if (c == '\r') {}    // skip
        else                 r += c;
    }
    return r;
}

/// 인라인 CSS — PDF 변환 시 외부 자원 의존 0
constexpr const char* kStyle = R"CSS(
<style>
  @page { size: A4; margin: 18mm 16mm; }
  * { box-sizing: border-box; }
  body {
    font-family: 'Malgun Gothic', 'Apple SD Gothic Neo', sans-serif;
    color: #1A2238; margin: 0; padding: 24px;
    line-height: 1.55;
  }
  .report-header {
    border-bottom: 3px solid #0F4C81;
    padding-bottom: 12px; margin-bottom: 18px;
  }
  .report-header h1 {
    margin: 0 0 6px 0; font-size: 22pt; color: #0F4C81;
  }
  .report-header .meta { color: #5B6478; font-size: 10pt; }
  .report-header .meta b { color: #1A2238; }
  h2 {
    font-size: 14pt; color: #0F4C81; margin: 22px 0 8px 0;
    border-left: 4px solid #F18F01; padding-left: 8px;
  }
  .stat-row { display: flex; gap: 12px; margin: 8px 0 16px 0; }
  .stat-card {
    background: #F4F7FB; border: 1px solid #D5DCE4; border-radius: 6px;
    padding: 10px 14px; min-width: 110px;
  }
  .stat-card .label { font-size: 9pt; color: #5B6478; }
  .stat-card .value { font-size: 18pt; font-weight: 700; color: #0F4C81; }
  table { width: 100%; border-collapse: collapse; font-size: 10pt; margin: 6px 0 12px 0; }
  table th, table td {
    border: 1px solid #D5DCE4;
    padding: 6px 8px; text-align: left; vertical-align: top;
  }
  table th { background: #EEF2F8; font-weight: 600; color: #1A2238; }
  table tr:nth-child(even) td { background: #F8FAFC; }
  td.num { text-align: right; }
  .quote {
    border: 1px solid #D5DCE4; border-left: 4px solid #1C7293;
    background: #F8FAFC; padding: 10px 14px; margin: 8px 0;
    page-break-inside: avoid;
  }
  .quote .drug-name { font-weight: 700; color: #0F4C81; margin-bottom: 4px; font-size: 11pt; }
  .quote .section { margin: 4px 0; font-size: 10pt; }
  .quote .section .key {
    color: #5B6478; font-size: 9pt; display: inline-block; min-width: 60px; font-weight: 600;
  }
  .quote .source-note {
    margin-top: 6px; padding-top: 6px; border-top: 1px dashed #D5DCE4;
    font-size: 9pt; color: #5B6478; font-style: italic;
  }
  .footer {
    margin-top: 24px; padding-top: 10px;
    border-top: 1px solid #D5DCE4;
    color: #5B6478; font-size: 9pt;
  }
  @media print {
    body { padding: 0; }
    .quote { break-inside: avoid; }
    h2 { break-after: avoid; }
  }
</style>
)CSS";

} // anonymous

std::string ReportHtmlRenderer::render(const schemas::ReportResponse& r)
{
    std::ostringstream o;
    o << "<!DOCTYPE html><html lang=\"ko\"><head><meta charset=\"utf-8\">"
      << "<title>메디브릿지 복약 보고서</title>"
      << kStyle
      << "</head><body>";

    // ----- Header -----
    const std::string user_name = r.user.isMember("user_name")
        ? esc(r.user["user_name"].asString()) : std::string{"-"};
    const std::string email = r.user.isMember("email")
        ? esc(r.user["email"].asString()) : std::string{};
    o << "<div class=\"report-header\">"
      << "<h1>메디브릿지 복약 보고서</h1>"
      << "<div class=\"meta\">"
      << "<b>" << user_name << "</b>";
    if (!email.empty()) o << " &lt;" << email << "&gt;";
    o << " &nbsp;&middot;&nbsp; 기간 <b>" << esc(r.period.from_date)
      << "</b> ~ <b>" << esc(r.period.to_date) << "</b>"
      << " &nbsp;&middot;&nbsp; 생성 " << fmt_dt(r.generated_at)
      << " &nbsp;&middot;&nbsp; ID <code>" << esc(r.report_id) << "</code>"
      << "</div></div>";

    // ----- Summary stats -----
    o << "<h2>복약 요약</h2>"
      << "<div class=\"stat-row\">"
      << "<div class=\"stat-card\"><div class=\"label\">총 복용</div>"
      << "<div class=\"value\">" << r.consumed_summary.total_intakes << "</div></div>"
      << "<div class=\"stat-card\"><div class=\"label\">약품 수</div>"
      << "<div class=\"value\">" << r.consumed_summary.by_drug.size() << "</div></div>"
      << "<div class=\"stat-card\"><div class=\"label\">참고 인용</div>"
      << "<div class=\"value\">" << r.side_effect_quotes.size() << "</div></div>"
      << "</div>";

    if (!r.consumed_summary.by_drug.empty()) {
        o << "<table><thead><tr>"
          << "<th>약명</th><th>총 복용량</th><th>최초</th><th>최근</th>"
          << "</tr></thead><tbody>";
        for (const auto& d : r.consumed_summary.by_drug) {
            o << "<tr><td>" << esc(d.drug_name) << "</td>"
              << "<td class=\"num\">" << d.total_quantity << "</td>"
              << "<td>" << fmt_dt(d.first_intake) << "</td>"
              << "<td>" << fmt_dt(d.last_intake)  << "</td></tr>";
        }
        o << "</tbody></table>";
    } else {
        o << "<p style=\"color:#5B6478\">해당 기간 복용 기록이 없습니다.</p>";
    }

    // ----- Intake logs -----
    o << "<h2>복용 이력</h2>";
    if (r.intake_logs.empty()) {
        o << "<p style=\"color:#5B6478\">기록이 없습니다.</p>";
    } else {
        o << "<table><thead><tr>"
          << "<th style=\"width:18%\">일시</th><th>약</th>"
          << "<th style=\"width:10%\">개수</th><th>메모</th>"
          << "</tr></thead><tbody>";
        for (const auto& e : r.intake_logs) {
            o << "<tr><td>" << fmt_dt(e.intake_datetime) << "</td>"
              << "<td>" << esc(e.drug_name) << "</td>"
              << "<td class=\"num\">" << e.quantity << "</td>"
              << "<td>" << esc(e.memo) << "</td></tr>";
        }
        o << "</tbody></table>";
    }

    // ----- Side effect quotes (식약처 e약은요 — 그대로 인용) -----
    o << "<h2>약품별 주의·부작용 (식약처 e약은요 인용)</h2>";
    if (r.side_effect_quotes.empty()) {
        o << "<p style=\"color:#5B6478\">참고할 식약처 본문이 캐시되어 있지 않습니다.</p>";
    } else {
        for (const auto& q : r.side_effect_quotes) {
            o << "<div class=\"quote\">"
              << "<div class=\"drug-name\">" << esc(q.drug_name)
              << " <span style=\"color:#5B6478;font-weight:400;font-size:9pt\">("
              << esc(q.item_code) << ")</span></div>";
            if (!q.caution_text.empty()) {
                o << "<div class=\"section\"><span class=\"key\">주의</span>"
                  << nl2br(q.caution_text) << "</div>";
            }
            if (!q.side_effect_text.empty()) {
                o << "<div class=\"section\"><span class=\"key\">부작용</span>"
                  << nl2br(q.side_effect_text) << "</div>";
            }
            if (!q.source_note.empty()) {
                o << "<div class=\"source-note\">" << esc(q.source_note) << "</div>";
            }
            o << "</div>";
        }
    }

    // ----- Footer -----
    o << "<div class=\"footer\">"
      << "본 보고서의 의약품 안내 내용은 식약처 공공데이터(e약은요·DUR)를 그대로 인용한 것으로, "
      << "메디브릿지는 의료 판단을 제공하지 않습니다. 구체적 복약 결정은 반드시 약사·의사와 상담하십시오."
      << "</div></body></html>";

    return o.str();
}

} // namespace medibridge::services::report
