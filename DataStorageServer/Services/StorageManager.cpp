// =====================================================
// StorageManager — 구현
// =====================================================
#include "StorageManager.h"
#include "../Config.h"

#include <filesystem>
#include <fstream>
#include <regex>
#include <cstdio>
#include <iostream>

namespace fs = std::filesystem;

namespace datastorage::services {

bool StorageManager::is_safe(const std::string& id)
{
    if (id.empty() || id.size() > 64) return false;
    static const std::regex re("^[A-Za-z0-9_-]+$");
    return std::regex_match(id, re);
}

bool StorageManager::is_allowed_ext(const std::string& ext)
{
    return ext == "jpg" || ext == "jpeg" || ext == "png";
}

static fs::path file_path(const std::string& anon,
                          const std::string& photo_id,
                          const std::string& ext)
{
    return fs::path(Config::instance().storage_root()) / anon / (photo_id + "." + ext);
}

bool StorageManager::put_atomic(const std::string& anonymous_id,
                                const std::string& photo_id,
                                const std::string& ext,
                                const std::string& body,
                                std::string&       err_msg)
{
    if (!is_safe(anonymous_id) || !is_safe(photo_id) || !is_allowed_ext(ext)) {
        err_msg = "invalid id/ext";
        return false;
    }

    const fs::path final_path = file_path(anonymous_id, photo_id, ext);
    const fs::path dir        = final_path.parent_path();
    const fs::path tmp_path   = dir / (photo_id + "." + ext + ".tmp");

    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        err_msg = "mkdir failed: " + ec.message();
        return false;
    }

    // 1) tmp 에 쓰기
    {
        std::ofstream ofs(tmp_path, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            err_msg = "open tmp failed";
            return false;
        }
        ofs.write(body.data(), static_cast<std::streamsize>(body.size()));
        if (!ofs) {
            err_msg = "write failed";
            ofs.close();
            std::remove(tmp_path.string().c_str());
            return false;
        }
    } // ofs close → flush

    // 2) atomic rename
    fs::rename(tmp_path, final_path, ec);
    if (ec) {
        err_msg = "rename failed: " + ec.message();
        std::remove(tmp_path.string().c_str());
        return false;
    }

    std::cout << "[Storage] PUT OK: " << final_path << " (" << body.size() << " B)" << std::endl;
    return true;
}

std::optional<std::string>
StorageManager::get(const std::string& anonymous_id,
                    const std::string& photo_id,
                    const std::string& ext)
{
    if (!is_safe(anonymous_id) || !is_safe(photo_id) || !is_allowed_ext(ext)) {
        return std::nullopt;
    }
    const fs::path p = file_path(anonymous_id, photo_id, ext);
    std::error_code ec;
    if (!fs::exists(p, ec) || !fs::is_regular_file(p, ec)) return std::nullopt;

    std::ifstream ifs(p, std::ios::binary | std::ios::ate);
    if (!ifs) return std::nullopt;
    const auto size = ifs.tellg();
    if (size <= 0) return std::string{};
    ifs.seekg(0, std::ios::beg);
    std::string buf;
    buf.resize(static_cast<size_t>(size));
    ifs.read(buf.data(), size);
    if (!ifs) return std::nullopt;
    return buf;
}

bool StorageManager::exists(const std::string& anonymous_id,
                            const std::string& photo_id,
                            const std::string& ext)
{
    if (!is_safe(anonymous_id) || !is_safe(photo_id) || !is_allowed_ext(ext)) return false;
    std::error_code ec;
    return fs::exists(file_path(anonymous_id, photo_id, ext), ec);
}

} // namespace datastorage::services
