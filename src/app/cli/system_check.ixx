/**
 * @file system_check.ixx
 * @brief System environment check module
 * @see design.md Section 3.5.4 - --system-check Output Specification
 */
export module app.cli.system_check;

import <string>;
import <vector>;

export namespace app::cli {

enum class CheckStatus { Ok, Warn, Fail };

struct CheckResult {
    std::string name;
    CheckStatus status;
    std::string value;
    std::string message; // 仅 Warn/Fail 时填写
};

struct SystemCheckReport {
    std::vector<CheckResult> checks;
    int ok_count = 0;
    int warn_count = 0;
    int fail_count = 0;
};

/**
 * @brief Run all system checks
 */
SystemCheckReport run_all_checks();

/**
 * @brief Format report as human-readable text
 */
std::string format_text(const SystemCheckReport& report);

/**
 * @brief Format report as JSON
 */
std::string format_json(const SystemCheckReport& report);

} // namespace app::cli
