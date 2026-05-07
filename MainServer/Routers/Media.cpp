// =====================================================
// Media Router — TestMode: 사진 수신 + photo_storage 메타만 INSERT
// =====================================================
// 실제 파일은 메모리에서 즉시 discard. storage_path 는 가짜 경로.
// 운영 모드: Data Storage PC (10.10.10.122) PUT 호출.
// =====================================================
#include "Media.h"
#include "../Schemas/MediaSchema.h"
#include "../Database/Connection.h"
#include "../Config.h"
#include "../Services/Auth/JwtIssuer.h"
#include "../Utils/TimeUtil.h"

#include <drogon/HttpResponse.h>
#include <drogon/MultiPart.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <random>
#include <sstream>

using medibridge::database::Connection;
using medibridge::Config;
namespace AuthSvc = medibridge::services::auth;
namespace orm = drogon::orm;

namespace medibridge::routers {

static drogon::HttpResponsePtr error_response(drogon::HttpStatusCode code,
                                              const std::string& err_code,
                                              const std::string& message)
{
    Json::Value body, err;
    err["code"] = err_code; err["message"] = message;
    body["error"] = err;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

static std::string gen_uuid_like()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::stringstream ss;
    ss << std::hex << gen() << gen();
    return ss.str().substr(0, 32);
}

void Media::handle_image_upload(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto h = req->getHeader("Authorization");
    auto user_id_opt = AuthSvc::JwtIssuer::extract_user_id_from_header(h);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }

    // anonymous_id 조회
    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }

    std::string anon_id;
    try {
        auto rows = db->execSqlSync(
            "SELECT anonymous_id FROM pseudonym_map WHERE user_id = ? AND severed_at IS NULL LIMIT 1",
            *user_id_opt);
        if (rows.empty()) {
            callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
            return;
        }
        anon_id = rows[0]["anonymous_id"].as<std::string>();
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
        return;
    }

    // multipart 파싱 — 파일이 있으면 메타만 INSERT, 본문은 discard
    drogon::MultiPartParser parser;
    int parse_ok = parser.parse(req);

    std::string mime_type = "image/jpeg";
    long file_size = 0;
    std::string request_id;

    if (parse_ok == 0) {
        const auto& files = parser.getFiles();
        if (!files.empty()) {
            const auto& f = files[0];
            file_size = static_cast<long>(f.fileLength());
            const auto fname = std::string(f.getFileName());
            const auto dot = fname.rfind('.');
            const auto ext = (dot != std::string::npos) ? fname.substr(dot) : "";
            if (ext == ".png" || ext == ".PNG") mime_type = "image/png";
        }
        const auto& params = parser.getParameters();
        auto it = params.find("request_id");
        if (it != params.end()) request_id = it->second;
        auto mt = params.find("mime_type");
        if (mt != params.end()) mime_type = mt->second;
    } else {
        // multipart 가 아니면 raw body 길이만 사용
        file_size = static_cast<long>(req->body().size());
        request_id = req->getHeader("X-Request-ID");
        const auto ct = req->getHeader("Content-Type");
        if (ct.find("png") != std::string::npos) mime_type = "image/png";
    }

    if (request_id.empty()) request_id = "req_" + gen_uuid_like();

    // photo_storage INSERT (TestMode: 가짜 storage_path)
    const std::string photo_id     = gen_uuid_like();
    const std::string storage_path = Config::instance().test_mode()
        ? std::string("/test/dev_seed/") + anon_id + "_" + photo_id + ".jpg"
        : std::string("/data/photos/")    + anon_id + "_" + photo_id + ".jpg";

    try {
        db->execSqlSync(
            "INSERT INTO photo_storage "
            "(photo_id, anonymous_id, storage_path, mime_type, file_size_bytes, "
            " taken_at, request_id, purpose) "
            "VALUES (?, ?, ?, ?, ?, NOW(), ?, 'IDENTIFY')",
            photo_id, anon_id, storage_path, mime_type, file_size, request_id);
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
        return;
    }

    schemas::ImageUploadResponse resp;
    resp.request_id    = request_id;
    resp.status        = "completed";
    resp.next_poll_url = "";

    callback(drogon::HttpResponse::newHttpJsonResponse(resp.to_json()));
}

} // namespace medibridge::routers
