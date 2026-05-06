#include "ReportSchema.h"

namespace medibridge::schemas {

Json::Value ReportPeriod::to_json() const
{
    Json::Value v;
    v["from_date"] = from_date;
    v["to_date"]   = to_date;
    return v;
}

Json::Value ConsumedByDrug::to_json() const
{
    Json::Value v;
    v["item_code"]      = item_code;
    v["drug_name"]      = drug_name;
    v["total_quantity"] = total_quantity;
    v["first_intake"]   = first_intake;
    v["last_intake"]    = last_intake;
    return v;
}

Json::Value ConsumedSummary::to_json() const
{
    Json::Value v;
    v["total_intakes"] = total_intakes;
    Json::Value arr(Json::arrayValue);
    for (const auto& d : by_drug) arr.append(d.to_json());
    v["by_drug"] = arr;
    return v;
}

Json::Value SideEffectQuote::to_json() const
{
    Json::Value v;
    v["item_code"]        = item_code;
    v["drug_name"]        = drug_name;
    v["caution_text"]     = caution_text;
    v["side_effect_text"] = side_effect_text;
    v["source_note"]      = source_note;
    return v;
}

Json::Value IntakeLogEntry::to_json() const
{
    Json::Value v;
    v["intake_datetime"] = intake_datetime;
    v["drug_name"]       = drug_name;
    v["quantity"]        = quantity;
    v["memo"]            = memo;
    return v;
}

Json::Value ReportResponse::to_json() const
{
    Json::Value v;
    v["report_id"]    = report_id;
    v["user"]         = user;
    v["period"]       = period.to_json();
    v["consumed_summary"] = consumed_summary.to_json();
    Json::Value se(Json::arrayValue);
    for (const auto& q : side_effect_quotes) se.append(q.to_json());
    v["side_effect_quotes"] = se;
    Json::Value il(Json::arrayValue);
    for (const auto& it : intake_logs) il.append(it.to_json());
    v["intake_logs"]    = il;
    v["generated_at"]   = generated_at;
    return v;
}

} // namespace medibridge::schemas
