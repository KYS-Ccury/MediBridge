#include "MediaSchema.h"

namespace medibridge::schemas {

ImageUploadRequest ImageUploadRequest::from_request(const Json::Value& meta)
{
    ImageUploadRequest r;
    r.mime_type   = meta.get("mime_type", "").asString();
    r.intent_hint = meta.get("intent_hint", "identify").asString();
    r.request_id  = meta.get("request_id", "").asString();
    return r;
}

bool ImageUploadRequest::is_valid(std::string& f, std::string& c) const
{
    if (mime_type != "image/jpeg" && mime_type != "image/png") {
        f = "mime_type"; c = "INVALID_MIME_TYPE"; return false;
    }
    return true;
}

Json::Value ImageUploadResponse::to_json() const
{
    Json::Value v;
    v["request_id"]    = request_id;
    v["status"]        = status;
    v["next_poll_url"] = next_poll_url;
    return v;
}

} // namespace medibridge::schemas
