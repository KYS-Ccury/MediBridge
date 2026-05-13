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

// =====================================================
// MediaIntentRequest / Response
// =====================================================
MediaIntentRequest MediaIntentRequest::from_json(const Json::Value& j)
{
    MediaIntentRequest r;
    r.mime_type   = j.get("mime_type", "").asString();
    r.size_bytes  = j.get("size_bytes", 0).asInt64();
    r.purpose     = j.get("purpose", "IDENTIFY").asString();
    r.request_id  = j.get("request_id", "").asString();
    return r;
}

bool MediaIntentRequest::is_valid(std::string& f, std::string& c, long max_bytes) const
{
    if (mime_type != "image/jpeg" && mime_type != "image/png") {
        f = "mime_type"; c = "INVALID_MIME_TYPE"; return false;
    }
    if (size_bytes <= 0) {
        f = "size_bytes"; c = "INVALID_SIZE"; return false;
    }
    if (size_bytes > max_bytes) {
        f = "size_bytes"; c = "SIZE_EXCEEDED"; return false;
    }
    if (purpose != "IDENTIFY" && purpose != "TRAIN" && purpose != "OTHER") {
        f = "purpose"; c = "INVALID_PURPOSE"; return false;
    }
    return true;
}

Json::Value MediaIntentResponse::to_json() const
{
    Json::Value v;
    v["photo_id"]    = photo_id;
    v["request_id"]  = request_id;
    v["storage_url"] = storage_url;
    v["put_token"]   = put_token;
    v["expires_at"]  = expires_at;
    v["max_bytes"]   = static_cast<Json::Int64>(max_bytes);
    return v;
}

// =====================================================
// MediaCommitRequest / Response
// =====================================================
MediaCommitRequest MediaCommitRequest::from_json(const Json::Value& j)
{
    MediaCommitRequest r;
    r.photo_id = j.get("photo_id", "").asString();
    r.etag     = j.get("etag", "").asString();
    return r;
}

bool MediaCommitRequest::is_valid(std::string& f, std::string& c) const
{
    if (photo_id.empty()) {
        f = "photo_id"; c = "MISSING_PHOTO_ID"; return false;
    }
    return true;
}

Json::Value MediaCommitResponse::to_json() const
{
    Json::Value v;
    v["photo_id"] = photo_id;
    v["status"]   = status;
    return v;
}

// =====================================================
// MediaGetTokenRequest / Response
// =====================================================
MediaGetTokenRequest MediaGetTokenRequest::from_json(const Json::Value& j)
{
    MediaGetTokenRequest r;
    r.photo_id = j.get("photo_id", "").asString();
    return r;
}

bool MediaGetTokenRequest::is_valid(std::string& f, std::string& c) const
{
    if (photo_id.empty()) {
        f = "photo_id"; c = "MISSING_PHOTO_ID"; return false;
    }
    return true;
}

Json::Value MediaGetTokenResponse::to_json() const
{
    Json::Value v;
    v["photo_id"]    = photo_id;
    v["storage_url"] = storage_url;
    v["get_token"]   = get_token;
    v["mime_type"]   = mime_type;
    v["expires_at"]  = expires_at;
    return v;
}

} // namespace medibridge::schemas
