// =====================================================
// ReportPdfRenderer — 구현 (wkhtmltopdf subprocess)
// =====================================================
// 변환 도구: wkhtmltopdf (apt install wkhtmltopdf).
//   - 한국어 폰트 시스템 (Noto / Nanum / Malgun Gothic) 자동 사용
//   - CSS @page / @media print 정확히 적용
//   - 외부 프로세스라 fork+exec 비용 ~ 0.5~1.5초 — WorkerPool 위임 권장
//
// 보안:
//   - system() 대신 fork+execvp — argv 배열 직접 구성 (셸 escape 위험 0)
//   - 임시 파일은 mkstemp 로 race-free 생성 후 항상 unlink
//   - 본 함수는 caller 가 호출자 컨텍스트의 보안 검증 후 호출 (인증 통과 후)
//
// 폴백:
//   - wkhtmltopdf 미존재 → 빈 vector 반환 (라우터가 501 처리)
// =====================================================
#include "ReportPdfRenderer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

namespace medibridge::services::report {

namespace {

/// PATH 에서 wkhtmltopdf 찾기 (한 번 검사 후 캐시)
const char* find_wkhtmltopdf()
{
    static const char* cached = nullptr;
    static bool checked = false;
    if (checked) return cached;
    checked = true;
    // 일반 설치 위치
    constexpr const char* kCandidates[] = {
        "/usr/bin/wkhtmltopdf",
        "/usr/local/bin/wkhtmltopdf",
        "/opt/bin/wkhtmltopdf",
    };
    for (const auto* path : kCandidates) {
        if (access(path, X_OK) == 0) { cached = path; return cached; }
    }
    return nullptr;
}

/// mkstemps 로 임시 파일 생성. suffix 길이 자동 계산 (XXXXXX 위치 찾아 그 이후).
std::string make_tmp_file(const char* tmpl_in, std::string& err)
{
    std::string tmpl(tmpl_in);
    // XXXXXX 패턴 위치 → 그 뒤가 suffix
    const auto xpos = tmpl.find("XXXXXX");
    if (xpos == std::string::npos) {
        err = "template 에 XXXXXX 없음";
        return {};
    }
    const int suffix_len = static_cast<int>(tmpl.size() - (xpos + 6));

    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    int fd = mkstemps(buf.data(), suffix_len);
    if (fd < 0) {
        err = std::string("mkstemps 실패: ") + std::strerror(errno) +
              " (suffix_len=" + std::to_string(suffix_len) + ")";
        return {};
    }
    close(fd);
    return std::string(buf.data());
}

/// 파일에 내용 쓰기
bool write_file(const std::string& path, const std::string& content)
{
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    return static_cast<bool>(ofs);
}

/// 파일 전체 읽기 (바이너리)
std::vector<unsigned char> read_binary(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) return {};
    const auto sz = ifs.tellg();
    if (sz <= 0) return {};
    std::vector<unsigned char> out(static_cast<size_t>(sz));
    ifs.seekg(0);
    ifs.read(reinterpret_cast<char*>(out.data()), sz);
    if (!ifs) return {};
    return out;
}

/// fork + execvp — wkhtmltopdf 호출. 반환: child exit code (실패 시 음수)
int run_wkhtmltopdf(const std::string& bin, const std::string& html_path,
                    const std::string& pdf_path)
{
    pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        // child — stdout/stderr 를 /dev/null 로 (wkhtmltopdf 가 진행 메시지 많이 뱉음)
        int dn = open("/dev/null", O_WRONLY);
        if (dn >= 0) { dup2(dn, 1); dup2(dn, 2); close(dn); }

        // execvp 의 argv 는 modifiable C-string 들
        std::string s0 = bin;
        std::string s1 = "--quiet";
        std::string s2 = "--enable-local-file-access";
        std::string s3 = "--encoding";
        std::string s4 = "utf-8";
        std::string s5 = html_path;
        std::string s6 = pdf_path;
        char* argv[] = {
            s0.data(), s1.data(), s2.data(), s3.data(), s4.data(),
            s5.data(), s6.data(), nullptr
        };
        execvp(argv[0], argv);
        _exit(127);   // exec 실패
    }

    // parent — 종료 대기 (최대 30초 wall clock 가정 — alarm 미사용)
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -2;
    if (WIFEXITED(status))   return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return -3;
    return -4;
}

} // anonymous

std::vector<unsigned char> ReportPdfRenderer::render(const std::string& html)
{
    if (html.empty()) return {};

    const char* bin = find_wkhtmltopdf();
    if (!bin) {
        std::cerr << "[ReportPdfRenderer] wkhtmltopdf 미설치 — "
                  << "apt install wkhtmltopdf" << std::endl;
        return {};
    }

    std::string err;
    const std::string html_path = make_tmp_file("/tmp/medibridge_report_XXXXXX.html", err);
    if (html_path.empty()) {
        std::cerr << "[ReportPdfRenderer] " << err << std::endl;
        return {};
    }
    const std::string pdf_path = make_tmp_file("/tmp/medibridge_report_XXXXXX.pdf", err);
    if (pdf_path.empty()) {
        unlink(html_path.c_str());
        std::cerr << "[ReportPdfRenderer] " << err << std::endl;
        return {};
    }

    // 임시 파일 정리 가드 — 함수 종료 시 항상 unlink
    struct Cleanup {
        const std::string& html;
        const std::string& pdf;
        ~Cleanup() { unlink(html.c_str()); unlink(pdf.c_str()); }
    } guard{html_path, pdf_path};

    if (!write_file(html_path, html)) {
        std::cerr << "[ReportPdfRenderer] HTML 쓰기 실패: " << html_path << std::endl;
        return {};
    }

    const int rc = run_wkhtmltopdf(bin, html_path, pdf_path);
    if (rc != 0) {
        std::cerr << "[ReportPdfRenderer] wkhtmltopdf 종료 코드=" << rc << std::endl;
        return {};
    }

    auto bytes = read_binary(pdf_path);
    if (bytes.empty()) {
        std::cerr << "[ReportPdfRenderer] PDF 읽기 실패: " << pdf_path << std::endl;
        return {};
    }
    std::cout << "[ReportPdfRenderer] PDF 생성 " << bytes.size() << " B" << std::endl;
    return bytes;
}

} // namespace medibridge::services::report
