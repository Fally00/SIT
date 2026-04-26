#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace Integrity {
    inline constexpr const char kManifestFileName[] = ".sit_integrity.manifest";

    struct Issue {
        std::string path;
        std::string status;
        std::string detail;
    };

    struct CheckResult {
        int total = 0;
        int ok = 0;
        int changed = 0;
        int missing = 0;
        int added = 0;
        int errors = 0;
        std::vector<Issue> issues;
    };

    bool createManifest(const std::filesystem::path& root, std::string* error);
    bool updateManifest(const std::filesystem::path& root, std::string* error);
    bool checkManifest(const std::filesystem::path& root, CheckResult& result, std::string* error);
}
