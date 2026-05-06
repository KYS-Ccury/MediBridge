// =====================================================
// UserManager — 사용자 CRUD (회원가입·조회)
// =====================================================
#pragma once

#include <optional>
#include <string>
#include "../../Schemas/AuthSchema.h"
#include "../../Database/Models.h"

namespace medibridge::services::auth {

class UserManager
{
public:
    /// 회원가입 — 이메일 중복 체크, bcrypt 해시 저장
    static std::optional<schemas::SignupResponse>
    create_user(const schemas::SignupRequest& request,
                std::string& error_code_out);

    /// 이메일로 사용자 조회
    static std::optional<database::models::User>
    find_by_email(const std::string& email);

    /// user_id로 사용자 조회
    static std::optional<database::models::User>
    find_by_id(const std::string& user_id);
};

} // namespace medibridge::services::auth
