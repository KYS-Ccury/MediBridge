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
#include "../Services/Media/StorageTokenIssuer.h"
#include "../Utils/TimeUtil.h"

#include <drogon/HttpResponse.h>
#include <drogon/MultiPart.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <random>
#include <sstream>
#include <regex>

using medibridge::database::Connection;
using medibridge::Config;
namespace AuthSvc    = medibridge::services::auth;
namespace MediaSvc   = medibridge::services::media;
namespace orm        = drogon::orm;

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

// anonymous_id / photo_id 정규식 (경로 traversal 방어).
// 보관 PC URL 경로에 직접 끼어들어가므로 엄격하게 화이트리스트.
static bool is_safe_id(const std::string& s)
{
    if (s.empty() || s.size() > 64) return false;
    static const std::regex re("^[A-Za-z0-9_-]+$");
    return std::regex_match(s, re);
}

// MIME → 확장자
static std::string mime_to_ext(const std::string& mime)
{
    if (mime == "image/png")  return "png";
    if (mime == "image/jpeg") return "jpg";
    return "bin";
}

// JWT 인증 + anonymous_id 조회 — Pill.cpp 와 동일 패턴
static std::optional<std::string>
auth_and_resolve_anon(const drogon::HttpRequestPtr& req, std::string& err_code)
{
    auto h = req->getHeader("Authorization");
    auto user_id_opt = AuthSvc::JwtIssuer::extract_user_id_from_header(h);
    if (!user_id_opt) { err_code = "INVALID_TOKEN"; return std::nullopt; }

    auto db = Connection::instance().client();
    if (!db) { err_code = "DB_UNAVAILABLE"; return std::nullopt; }

    try {
        auto rows = db->execSqlSync(
            "SELECT anonymous_id FROM pseudonym_map "
            "WHERE user_id = ? AND severed_at IS NULL LIMIT 1",
            *user_id_opt);
        if (rows.empty()) { err_code = "INVALID_TOKEN"; return std::nullopt; }
        return rows[0]["anonymous_id"].as<std::string>();
    } catch (const orm::DrogonDbException&) {
        err_code = "DB_ERROR"; return std::nullopt;
    }
}

// =====================================================
// POST /v1/media/intent — ⑤+⑥ 정상 흐름의 1단계
// =====================================================
// 클라가 사진 본체를 보내기 전 호출. 메인이:
//   1. JWT 검증 → anonymous_id 확인
//   2. 디스크 여유 / 메타 검증
//   3. photo_storage INSERT (status=PENDING, expires_at=now+TTL)
//   4. PUT 토큰 (HMAC) 발급
//   5. storage_url + put_token 반환
// 클라는 응답의 storage_url 에 직접 PUT 한다 (메인 우회).
// =====================================================
void Media::handle_intent(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    std::string err_code_auth;
    auto anon_opt = auth_and_resolve_anon(req, err_code_auth);
    if (!anon_opt) {
        const auto code = (err_code_auth == "DB_UNAVAILABLE")
            ? drogon::k503ServiceUnavailable
            : (err_code_auth == "DB_ERROR")
                ? drogon::k500InternalServerError
                : drogon::k401Unauthorized;
        callback(error_response(code, err_code_auth, "인증/식별 실패."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::MediaIntentRequest::from_json(*json_ptr);
    std::string ef, ec;
    if (!reqobj.is_valid(ef, ec, Config::instance().storage_max_bytes())) {
        callback(error_response(drogon::k400BadRequest, ec, "필드 검증 실패: " + ef));
        return;
    }

    if (!is_safe_id(*anon_opt)) {
        callback(error_response(drogon::k500InternalServerError, "INVALID_ANONYMOUS_ID",
                                "내부 식별자 형식이 잘못됨."));
        return;
    }

    // 식별자 생성
    const std::string photo_id   = "ph_" + gen_uuid_like();
    const std::string request_id = reqobj.request_id.empty()
        ? std::string("req_") + gen_uuid_like()
        : reqobj.request_id;
    const std::string ext        = mime_to_ext(reqobj.mime_type);
    const std::string storage_path = std::string("/photos/") + *anon_opt + "/" + photo_id + "." + ext;

    // photo_storage INSERT (PENDING)
    const int  ttl = Config::instance().storage_token_ttl_seconds();
    const auto db  = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }
    try {
        db->execSqlSync(
            "INSERT INTO photo_storage "
            "(photo_id, anonymous_id, storage_path, mime_type, file_size_bytes, "
            " taken_at, request_id, purpose, status, expires_at) "
            "VALUES (?, ?, ?, ?, ?, NOW(), ?, ?, 'PENDING', DATE_ADD(NOW(), INTERVAL ? SECOND))",
            photo_id, *anon_opt, storage_path, reqobj.mime_type, reqobj.size_bytes,
            request_id, reqobj.purpose, ttl);
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
        return;
    }

    // PUT 토큰 발급
    MediaSvc::StorageTokenClaims claims;
    claims.anonymous_id = *anon_opt;
    claims.photo_id     = photo_id;
    claims.operation    = "put";
    claims.mime_type    = reqobj.mime_type;
    claims.max_bytes    = Config::instance().storage_max_bytes();
    const std::string put_token = MediaSvc::StorageTokenIssuer::issue(claims, ttl);

    // storage_url 구성 — 보관 PC 가 라우팅할 경로
    // 형식: <base>/storage/photos/<anon>/<photo_id>.<ext>
    const std::string storage_url =
        Config::instance().storage_base_url() +
        "/storage/photos/" + *anon_opt + "/" + photo_id + "." + ext;

    // expires_at ISO 8601 (TTL 추가)
    const auto now_t  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const std::string expires_at_iso = utils::to_iso8601_utc(now_t + ttl);

    schemas::MediaIntentResponse resp;
    resp.photo_id    = photo_id;
    resp.request_id  = request_id;
    resp.storage_url = storage_url;
    resp.put_token   = put_token;
    resp.expires_at  = expires_at_iso;
    resp.max_bytes   = Config::instance().storage_max_bytes();

    auto http = drogon::HttpResponse::newHttpJsonResponse(resp.to_json());
    http->setStatusCode(drogon::k201Created);
    callback(http);
}

// =====================================================
// POST /v1/media/commit — PUT 완료 통지 → status PENDING → READY
// =====================================================
// 클라가 보관 PC PUT 성공 후 호출. 미호출 시 청소 잡이 EXPIRED 처리.
// 본 핸들러는 idempotent — 이미 READY 인 row 도 200 으로 응답.
// =====================================================
void Media::handle_commit(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    std::string err_code_auth;
    auto anon_opt = auth_and_resolve_anon(req, err_code_auth);
    if (!anon_opt) {
        const auto code = (err_code_auth == "DB_UNAVAILABLE")
            ? drogon::k503ServiceUnavailable
            : (err_code_auth == "DB_ERROR")
                ? drogon::k500InternalServerError
                : drogon::k401Unauthorized;
        callback(error_response(code, err_code_auth, "인증/식별 실패."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::MediaCommitRequest::from_json(*json_ptr);
    std::string ef, ec;
    if (!reqobj.is_valid(ef, ec)) {
        callback(error_response(drogon::k400BadRequest, ec, "필드 검증 실패: " + ef));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }
    try {
        // PENDING → READY 만 갱신. 다른 상태는 SELECT 로 확인 후 분기.
        // (MariaDB affectedRows() 는 변경된 행만 카운트하므로
        //  READY→READY 는 0 — idempotent 처리를 위해 별도 분기)
        auto r = db->execSqlSync(
            "UPDATE photo_storage "
            "   SET status='READY', committed_at=NOW() "
            " WHERE photo_id = ? AND anonymous_id = ? AND status='PENDING'",
            reqobj.photo_id, *anon_opt);
        if (r.affectedRows() == 0) {
            auto check = db->execSqlSync(
                "SELECT status FROM photo_storage "
                " WHERE photo_id = ? AND anonymous_id = ? LIMIT 1",
                reqobj.photo_id, *anon_opt);
            if (check.empty()) {
                callback(error_response(drogon::k404NotFound, "NOT_FOUND", "photo_id 없음."));
                return;
            }
            const std::string cur = check[0]["status"].as<std::string>();
            if (cur == "READY") {
                // idempotent — 이미 READY 면 정상 응답
            } else {
                // EXPIRED / FAILED
                callback(error_response(drogon::k409Conflict, "INVALID_STATE",
                                        "현재 상태(" + cur + ")에서 commit 불가."));
                return;
            }
        }
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
        return;
    }

    schemas::MediaCommitResponse resp;
    resp.photo_id = reqobj.photo_id;
    resp.status   = "READY";
    callback(drogon::HttpResponse::newHttpJsonResponse(resp.to_json()));
}

// =====================================================
// POST /v1/media/get_token — Vision PC 용 GET 토큰 발급
// =====================================================
// 메인이 /v1/pill/identify 시점에 자동 발급하는 게 정상이지만,
// Vision PC 단독 호출/디버그/리커버리 시 별도 노출.
// 본인 소유 + status=READY 만 발급.
// =====================================================
void Media::handle_get_token(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    std::string err_code_auth;
    auto anon_opt = auth_and_resolve_anon(req, err_code_auth);
    if (!anon_opt) {
        const auto code = (err_code_auth == "DB_UNAVAILABLE")
            ? drogon::k503ServiceUnavailable
            : (err_code_auth == "DB_ERROR")
                ? drogon::k500InternalServerError
                : drogon::k401Unauthorized;
        callback(error_response(code, err_code_auth, "인증/식별 실패."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::MediaGetTokenRequest::from_json(*json_ptr);
    std::string ef, ec;
    if (!reqobj.is_valid(ef, ec)) {
        callback(error_response(drogon::k400BadRequest, ec, "필드 검증 실패: " + ef));
        return;
    }
    if (!is_safe_id(*anon_opt) || !is_safe_id(reqobj.photo_id)) {
        callback(error_response(drogon::k400BadRequest, "INVALID_ID", "식별자 형식 오류."));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }

    std::string mime_type, storage_path, status;
    try {
        auto rows = db->execSqlSync(
            "SELECT mime_type, storage_path, status "
            "  FROM photo_storage "
            " WHERE photo_id = ? AND anonymous_id = ? LIMIT 1",
            reqobj.photo_id, *anon_opt);
        if (rows.empty()) {
            callback(error_response(drogon::k404NotFound, "NOT_FOUND", "photo_id 없음 또는 권한 없음."));
            return;
        }
        mime_type    = rows[0]["mime_type"].as<std::string>();
        storage_path = rows[0]["storage_path"].as<std::string>();
        status       = rows[0]["status"].as<std::string>();
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
        return;
    }

    if (status != "READY") {
        callback(error_response(drogon::k409Conflict, "INVALID_STATE",
                                "상태(" + status + ")에서 GET 토큰 발급 불가. PUT/commit 먼저."));
        return;
    }

    // GET 토큰 발급
    const int ttl = Config::instance().storage_token_ttl_seconds();
    MediaSvc::StorageTokenClaims claims;
    claims.anonymous_id = *anon_opt;
    claims.photo_id     = reqobj.photo_id;
    claims.operation    = "get";
    claims.mime_type    = mime_type;
    claims.max_bytes    = Config::instance().storage_max_bytes();
    const std::string get_token = MediaSvc::StorageTokenIssuer::issue(claims, ttl);

    // storage_url 재구성 — handle_intent 의 storage_path 형식: "/photos/<anon>/<photo>.<ext>"
    // 보관 PC 라우트 형식:                            "/storage/photos/<anon>/<photo>.<ext>"
    std::string sp = storage_path;
    if (sp.rfind("/photos/", 0) == 0) sp = sp.substr(7);   // "/photos" 제거 → "/<anon>/<photo>.<ext>"
    const std::string storage_url_final =
        Config::instance().storage_base_url() + "/storage/photos" + sp;

    const auto now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const std::string expires_at_iso = utils::to_iso8601_utc(now_t + ttl);

    schemas::MediaGetTokenResponse resp;
    resp.photo_id    = reqobj.photo_id;
    resp.storage_url = storage_url_final;
    resp.get_token   = get_token;
    resp.mime_type   = mime_type;
    resp.expires_at  = expires_at_iso;
    callback(drogon::HttpResponse::newHttpJsonResponse(resp.to_json()));
}

// =====================================================
// POST /v1/media/image — 레거시/TestMode fallback
// =====================================================
// 보관 PC 미가동 시·시드 시나리오용. 본문은 메모리에서 discard, 메타만 INSERT.
// 운영 모드 정상 흐름은 /v1/media/intent → PUT → /v1/media/commit.
// =====================================================
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
        // 레거시 경로 — 메타만 INSERT, 즉시 READY (실 파일 없이도 식별 시드 응답 가능)
        db->execSqlSync(
            "INSERT INTO photo_storage "
            "(photo_id, anonymous_id, storage_path, mime_type, file_size_bytes, "
            " taken_at, request_id, purpose, status, committed_at) "
            "VALUES (?, ?, ?, ?, ?, NOW(), ?, 'IDENTIFY', 'READY', NOW())",
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
