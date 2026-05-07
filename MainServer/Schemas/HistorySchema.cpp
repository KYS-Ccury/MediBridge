#include "HistorySchema.h"

namespace medibridge::schemas {

DurSnapshot DurSnapshot::from_json(const Json::Value& json)
{
    DurSnapshot s;
    s.checked_at = json.get("checked_at", "").asString();
    s.result     = json.get("result", "").asString();
    s.details    = json.get("details", Json::Value(Json::arrayValue));
    return s;
}

Json::Value DurSnapshot::to_json() const
{
    Json::Value v;
    v["checked_at"] = checked_at;
    v["result"]     = result;
    v["details"]    = details;
    return v;
}

HistoryRecordRequest HistoryRecordRequest::from_json(const Json::Value& json)
{
    HistoryRecordRequest r;
    r.item_code       = json.get("item_code", "").asString();
    r.intake_datetime = json.get("intake_datetime", "").asString();
    r.quantity        = json.get("quantity", 1).asInt();
    if (json.isMember("memo") && !json["memo"].isNull())
        r.memo = json["memo"].asString();
    if (json.isMember("confidence_score") && !json["confidence_score"].isNull())
        r.confidence_score = json["confidence_score"].asDouble();
    if (json.isMember("dur_snapshot") && !json["dur_snapshot"].isNull())
        r.dur_snapshot = DurSnapshot::from_json(json["dur_snapshot"]);
    return r;
}

bool HistoryRecordRequest::is_valid(std::string& f, std::string& c) const
{
    if (item_code.empty()) { f = "item_code"; c = "MISSING_FIELD"; return false; }
    if (quantity <= 0)     { f = "quantity";  c = "INVALID_QUANTITY"; return false; }
    return true;
}

Json::Value HistoryRecordResponse::to_json() const
{
    Json::Value v;
    v["intake_id"]       = intake_id;
    v["user_id"]         = user_id;
    v["item_code"]       = item_code;
    v["intake_datetime"] = intake_datetime;
    v["quantity"]        = quantity;
    v["memo"]            = memo;
    v["created_at"]      = created_at;
    return v;
}

Json::Value HistoryItem::to_json() const
{
    Json::Value v;
    v["intake_id"]       = intake_id;
    v["item_code"]       = item_code;
    v["drug_name"]       = drug_name;
    v["intake_datetime"] = intake_datetime;
    v["quantity"]        = quantity;
    v["memo"]             = memo;
    v["time_slot"]       = time_slot;
    return v;
}

Json::Value HistoryListResponse::to_json() const
{
    Json::Value v;
    Json::Value arr(Json::arrayValue);
    for (const auto& it : items) arr.append(it.to_json());
    v["items"]       = arr;
    v["page"]        = page;
    v["page_size"]   = page_size;
    v["total_count"] = total_count;
    v["total_pages"] = total_pages;
    return v;
}

} // namespace medibridge::schemas
