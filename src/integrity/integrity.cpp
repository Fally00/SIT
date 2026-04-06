// integrity.cpp integration manifest implementation
#include "integrity/integrity.h"

// Standard library includes for succesful implementation
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <array>

namespace Integrity {
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

    uint32_t rotr(uint32_t value, uint32_t bits) {
        return (value >> bits) | (value << (32 - bits));
    }

    class Sha256 {
    public:
        Sha256() {
            reset();
        }

        void update(const uint8_t* data, size_t length) {
            for (size_t i = 0; i < length; ++i) {
                buffer[bufferLen++] = data[i];
                if (bufferLen == buffer.size()) {
                    transform(buffer.data());
                    bitLen += 512;
                    bufferLen = 0;
                }
            }
        }

        std::string finalHex() {
            uint64_t totalBits = bitLen + static_cast<uint64_t>(bufferLen) * 8;
            buffer[bufferLen++] = 0x80;
            if (bufferLen > 56) {
                while (bufferLen < buffer.size()) {
                    buffer[bufferLen++] = 0;
                }
                transform(buffer.data());
                bufferLen = 0;
            }
            while (bufferLen < 56) {
                buffer[bufferLen++] = 0;
            }
            for (int i = 7; i >= 0; --i) {
                buffer[bufferLen++] = static_cast<uint8_t>((totalBits >> (i * 8)) & 0xFF);
            }
            transform(buffer.data());

            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            for (uint32_t word : state) {
                oss << std::setw(8) << word;
            }
            reset();
            return oss.str();
        }

    private:
        std::array<uint8_t, 64> buffer{};
        size_t bufferLen = 0;
        uint64_t bitLen = 0;
        std::array<uint32_t, 8> state{};

        void reset() {
            bufferLen = 0;
            bitLen = 0;
            state = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
        }

        void transform(const uint8_t* chunk) {
            static const uint32_t k[64] = {
                0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
            };

            uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<uint32_t>(chunk[i * 4]) << 24)
                    | (static_cast<uint32_t>(chunk[i * 4 + 1]) << 16)
                    | (static_cast<uint32_t>(chunk[i * 4 + 2]) << 8)
                    | (static_cast<uint32_t>(chunk[i * 4 + 3]));
            }
            for (int i = 16; i < 64; ++i) {
                uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }

            uint32_t a = state[0];
            uint32_t b = state[1];
            uint32_t c = state[2];
            uint32_t d = state[3];
            uint32_t e = state[4];
            uint32_t f = state[5];
            uint32_t g = state[6];
            uint32_t h = state[7];

            for (int i = 0; i < 64; ++i) {
                uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                uint32_t ch = (e & f) ^ (~e & g);
                uint32_t temp1 = h + s1 + ch + k[i] + w[i];
                uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = s0 + maj;

                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
            state[5] += f;
            state[6] += g;
            state[7] += h;
        }
    };

    bool isExcludedDir(const std::filesystem::path& path) {
        auto name = path.filename().string();
        return name == ".git";
    }

    int64_t toUnixSeconds(const std::filesystem::file_time_type& ftime) {
        using namespace std::chrono;
        auto sctp = time_point_cast<seconds>(ftime - std::filesystem::file_time_type::clock::now()
            + system_clock::now());
        return static_cast<int64_t>(system_clock::to_time_t(sctp));
    }

    std::string hashFileSha256(const std::filesystem::path& path, std::string* error) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            if (error) {
                *error = "Unable to open file";
            }
            return std::string();
        }
        Sha256 hasher;
        std::array<char, 8192> buffer;
        while (file) {
            file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            std::streamsize count = file.gcount();
            if (count > 0) {
                hasher.update(reinterpret_cast<const uint8_t*>(buffer.data()), static_cast<size_t>(count));
            }
        }
        if (!file.eof() && file.fail()) {
            if (error) {
                *error = "Error while reading file";
            }
            return std::string();
        }
        return hasher.finalHex();
    }

    bool parseManifestLine(const std::string& line, std::string& path, uint64_t& size, int64_t& mtime, std::string& hash) {
        std::string sizeStr;
        std::string mtimeStr;
        std::istringstream iss(line);
        if (!std::getline(iss, path, '\t')) {
            return false;
        }
        if (!std::getline(iss, sizeStr, '\t')) {
            return false;
        }
        if (!std::getline(iss, mtimeStr, '\t')) {
            return false;
        }
        if (!std::getline(iss, hash)) {
            return false;
        }
        try {
            size = std::stoull(sizeStr);
            mtime = std::stoll(mtimeStr);
        } catch (...) {
            return false;
        }
        return true;
    }

    bool loadManifest(const std::filesystem::path& manifestPath,
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
            if (line.empty() || line[0] == '#') {
                continue;
            }
            std::string path;
            uint64_t size = 0;
            int64_t mtime = 0;
            std::string hash;
            if (!parseManifestLine(line, path, size, mtime, hash)) {
                if (error) {
                    *error = "Integrity manifest parse error on line " + std::to_string(lineNumber) + ".";
                }
                return false;
            }
            entries[path] = { size, mtime, hash, false };
        }
        return true;
    }

    bool writeManifest(const std::filesystem::path& root, bool allowOverwrite, std::string* error) {
        std::error_code ec;
        if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
            if (error) {
                *error = "Root path is not a directory.";
            }
            return false;
        }

        std::filesystem::path manifestPath = root / kManifestFileName;
        if (!allowOverwrite && std::filesystem::exists(manifestPath, ec)) {
            if (error) {
                *error = "Integrity manifest already exists. Use 'integrity update' to overwrite.";
            }
            return false;
        }

        std::vector<FileRecord> records;
        int errorCount = 0;

        std::filesystem::recursive_directory_iterator it(
            root,
            std::filesystem::directory_options::skip_permission_denied,
            ec);
        std::filesystem::recursive_directory_iterator end;
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

            uint64_t size = std::filesystem::file_size(it->path(), ec);
            if (ec) {
                ++errorCount;
                ec.clear();
                continue;
            }
            auto mtimeFs = std::filesystem::last_write_time(it->path(), ec);
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

    bool createManifest(const std::filesystem::path& root, std::string* error) {
        return writeManifest(root, false, error);
    }

    bool updateManifest(const std::filesystem::path& root, std::string* error) {
        return writeManifest(root, true, error);
    }

    bool checkManifest(const std::filesystem::path& root, CheckResult& result, std::string* error) {
        std::error_code ec;
        if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
            if (error) {
                *error = "Root path is not a directory.";
            }
            return false;
        }

        std::filesystem::path manifestPath = root / kManifestFileName;
        std::unordered_map<std::string, ManifestEntry> manifest;
        if (!loadManifest(manifestPath, manifest, error)) {
            return false;
        }

        result = CheckResult{};
        result.total = static_cast<int>(manifest.size());

        std::filesystem::recursive_directory_iterator it(
            root,
            std::filesystem::directory_options::skip_permission_denied,
            ec);
        std::filesystem::recursive_directory_iterator end;
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

            uint64_t size = std::filesystem::file_size(it->path(), ec);
            if (ec) {
                ++result.errors;
                addIssue(result, relPath, "ERROR", "Unable to read file size");
                ec.clear();
                continue;
            }
            auto mtimeFs = std::filesystem::last_write_time(it->path(), ec);
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
