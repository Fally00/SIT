// integrity.cpp integration manifest implementation
#include "integrity/integrity.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Integrity {
namespace fs = std::filesystem;

namespace {
    constexpr const char kManifestHeader[] = "# SIT-INTEGRITY-1 SHA256";

    struct ManifestEntry {
        uint64_t size = 0;
        int64_t mtime = 0;
        std::string hash;
        bool seen = false;
    };

    struct FileRecord {
        std::string relPath;
        uint64_t size = 0;
        int64_t mtime = 0;
        std::string hash;
    };

    void trimLineEndings(std::string& line) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
    }

    bool isExcludedDir(const fs::path& path) {
        return path.filename() == ".git";
    }

    int64_t toUnixSeconds(const fs::file_time_type& ftime) {
        using namespace std::chrono;
        auto sctp = time_point_cast<seconds>(
            ftime - fs::file_time_type::clock::now() + system_clock::now());
        return static_cast<int64_t>(system_clock::to_time_t(sctp));
    }

    bool normalizeManifestPath(const std::string& parsedPath, std::string& normalizedPath) {
        if (parsedPath.empty()) {
            return false;
        }

        fs::path normalized = fs::path(parsedPath).lexically_normal();
        if (normalized.empty() ||
            normalized == "." ||
            normalized.is_absolute() ||
            normalized.has_root_name() ||
            normalized.has_root_directory()) {
            return false;
        }

        for (const auto& part : normalized) {
            if (part == "..") {
                return false;
            }
        }

        normalizedPath = normalized.generic_string();
        return !normalizedPath.empty() && normalizedPath != ".";
    }

    std::string hashFileSha256(const fs::path& path, std::string* error) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            if (error) {
                *error = "Unable to open file";
            }
            return {};
        }

        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
            EVP_MD_CTX_new(), &EVP_MD_CTX_free);
        if (!ctx) {
            if (error) {
                *error = "Unable to allocate SHA-256 context";
            }
            return {};
        }

        if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
            if (error) {
                *error = "Unable to initialize SHA-256 context";
            }
            return {};
        }

        std::array<char, 8192> buffer{};
        while (file) {
            file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            std::streamsize count = file.gcount();
            if (count > 0 &&
                EVP_DigestUpdate(ctx.get(), buffer.data(), static_cast<size_t>(count)) != 1) {
                if (error) {
                    *error = "Error while hashing file";
                }
                return {};
            }
        }

        if (!file.eof() && file.fail()) {
            if (error) {
                *error = "Error while reading file";
            }
            return {};
        }

        std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
        unsigned int digestSize = 0;
        if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &digestSize) != 1) {
            if (error) {
                *error = "Unable to finalize SHA-256 digest";
            }
            return {};
        }

        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (unsigned int i = 0; i < digestSize; ++i) {
            oss << std::setw(2) << static_cast<unsigned int>(digest[i]);
        }
        return oss.str();
    }

    bool parseManifestLine(
        const std::string& line,
        std::string& path,
        uint64_t& size,
        int64_t& mtime,
        std::string& hash,
        std::string* parseError) {
        std::string sizeStr;
        std::string mtimeStr;
        std::istringstream iss(line);

        if (!std::getline(iss, path, '\t') ||
            !std::getline(iss, sizeStr, '\t') ||
            !std::getline(iss, mtimeStr, '\t') ||
            !std::getline(iss, hash)) {
            if (parseError) {
                *parseError = "expected path, size, mtime, and hash fields";
            }
            return false;
        }

        if (!normalizeManifestPath(path, path)) {
            if (parseError) {
                *parseError = "invalid relative path";
            }
            return false;
        }

        if (hash.empty()) {
            if (parseError) {
                *parseError = "missing hash";
            }
            return false;
        }

        try {
            size = std::stoull(sizeStr);
            mtime = std::stoll(mtimeStr);
        } catch (...) {
            if (parseError) {
                *parseError = "invalid numeric metadata";
            }
            return false;
        }

        return true;
    }

    bool loadManifest(
        const fs::path& manifestPath,
        std::unordered_map<std::string, ManifestEntry>& entries,
        std::string* error) {
        std::ifstream in(manifestPath, std::ios::binary);
        if (!in) {
            if (error) {
                *error = "Integrity manifest not found. Run 'integrity init' first.";
            }
            return false;
        }

        std::string header;
        if (!std::getline(in, header)) {
            if (error) {
                *error = "Integrity manifest is empty.";
            }
            return false;
        }
        trimLineEndings(header);
        if (header != kManifestHeader) {
            if (error) {
                *error = "Integrity manifest format not recognized.";
            }
            return false;
        }

        std::string line;
        int lineNumber = 1;
        while (std::getline(in, line)) {
            ++lineNumber;
            trimLineEndings(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::string path;
            uint64_t size = 0;
            int64_t mtime = 0;
            std::string hash;
            std::string parseError;
            if (!parseManifestLine(line, path, size, mtime, hash, &parseError)) {
                if (error) {
                    *error = "Integrity manifest parse error on line " +
                        std::to_string(lineNumber) + ": " + parseError + ".";
                }
                return false;
            }

            entries[path] = { size, mtime, hash, false };
        }
        return true;
    }

    bool writeManifest(const fs::path& root, bool allowOverwrite, std::string* error) {
        std::error_code ec;
        if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
            if (error) {
                *error = "Root path is not a directory.";
            }
            return false;
        }

        fs::path manifestPath = root / kManifestFileName;
        if (!allowOverwrite && fs::exists(manifestPath, ec)) {
            if (error) {
                *error = "Integrity manifest already exists. Use 'integrity update' to overwrite.";
            }
            return false;
        }

        std::vector<FileRecord> records;
        int errorCount = 0;

        fs::recursive_directory_iterator it(
            root,
            fs::directory_options::skip_permission_denied,
            ec);
        fs::recursive_directory_iterator end;
        for (; it != end; it.increment(ec)) {
            if (ec) {
                ++errorCount;
                ec.clear();
                continue;
            }
            if (it->is_directory(ec)) {
                if (isExcludedDir(it->path())) {
                    it.disable_recursion_pending();
                }
                continue;
            }
            if (!it->is_regular_file(ec)) {
                continue;
            }

            auto relPath = it->path().lexically_relative(root).generic_string();
            if (relPath == kManifestFileName) {
                continue;
            }

            uint64_t size = fs::file_size(it->path(), ec);
            if (ec) {
                ++errorCount;
                ec.clear();
                continue;
            }
            auto mtimeFs = fs::last_write_time(it->path(), ec);
            if (ec) {
                ++errorCount;
                ec.clear();
                continue;
            }
            int64_t mtime = toUnixSeconds(mtimeFs);
            std::string hashError;
            std::string hash = hashFileSha256(it->path(), &hashError);
            if (hash.empty()) {
                ++errorCount;
                continue;
            }
            records.push_back({ relPath, size, mtime, hash });
        }

        std::sort(records.begin(), records.end(),
            [](const FileRecord& a, const FileRecord& b) { return a.relPath < b.relPath; });

        std::ofstream out(manifestPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            if (error) {
                *error = "Unable to write integrity manifest.";
            }
            return false;
        }
        out << kManifestHeader << "\n";
        for (const auto& record : records) {
            out << record.relPath << "\t" << record.size << "\t" << record.mtime << "\t" << record.hash << "\n";
        }

        if (errorCount > 0 && error) {
            *error = "Manifest created with " + std::to_string(errorCount) + " unreadable files skipped.";
        }

        return true;
    }

    void addIssue(CheckResult& result, const std::string& path, const std::string& status, const std::string& detail) {
        result.issues.push_back({ path, status, detail });
    }
} // namespace

    bool createManifest(const fs::path& root, std::string* error) {
        return writeManifest(root, false, error);
    }

    bool updateManifest(const fs::path& root, std::string* error) {
        return writeManifest(root, true, error);
    }

    bool checkManifest(const fs::path& root, CheckResult& result, std::string* error) {
        std::error_code ec;
        if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
            if (error) {
                *error = "Root path is not a directory.";
            }
            return false;
        }

        fs::path manifestPath = root / kManifestFileName;
        std::unordered_map<std::string, ManifestEntry> manifest;
        if (!loadManifest(manifestPath, manifest, error)) {
            return false;
        }

        result = CheckResult{};
        result.total = static_cast<int>(manifest.size());

        fs::recursive_directory_iterator it(
            root,
            fs::directory_options::skip_permission_denied,
            ec);
        fs::recursive_directory_iterator end;
        for (; it != end; it.increment(ec)) {
            if (ec) {
                ++result.errors;
                addIssue(result, "", "ERROR", "Directory scan error: " + ec.message());
                ec.clear();
                continue;
            }

            if (it->is_directory(ec)) {
                if (isExcludedDir(it->path())) {
                    it.disable_recursion_pending();
                }
                continue;
            }
            if (!it->is_regular_file(ec)) {
                continue;
            }

            auto relPath = it->path().lexically_relative(root).generic_string();
            if (relPath == kManifestFileName) {
                continue;
            }

            auto found = manifest.find(relPath);
            if (found == manifest.end()) {
                ++result.added;
                addIssue(result, relPath, "NEW", "Not in manifest");
                continue;
            }

            ManifestEntry& entry = found->second;
            entry.seen = true;

            uint64_t size = fs::file_size(it->path(), ec);
            if (ec) {
                ++result.errors;
                addIssue(result, relPath, "ERROR", "Unable to read file size");
                ec.clear();
                continue;
            }
            auto mtimeFs = fs::last_write_time(it->path(), ec);
            if (ec) {
                ++result.errors;
                addIssue(result, relPath, "ERROR", "Unable to read last write time");
                ec.clear();
                continue;
            }
            int64_t mtime = toUnixSeconds(mtimeFs);

            if (size == entry.size && mtime == entry.mtime) {
                ++result.ok;
                continue;
            }

            std::string hashError;
            std::string hash = hashFileSha256(it->path(), &hashError);
            if (hash.empty()) {
                ++result.errors;
                addIssue(result, relPath, "ERROR", hashError.empty() ? "Unable to hash file" : hashError);
                continue;
            }
            if (hash == entry.hash) {
                ++result.ok;
                continue;
            }
            ++result.changed;
            addIssue(result, relPath, "CHANGED", "Hash mismatch");
        }

        for (const auto& entryPair : manifest) {
            if (!entryPair.second.seen) {
                ++result.missing;
                addIssue(result, entryPair.first, "MISSING", "File not found");
            }
        }

        return true;
    }
}
