#include "platform/platform.h"

#ifdef _WIN32

#include <windows.h>

#include <string>
#include <vector>

namespace {
    bool readRegistryString(HKEY key, const wchar_t* valueName, std::wstring& value) {
        DWORD type = 0;
        DWORD size = 0;
        if (RegQueryValueExW(key, valueName, nullptr, &type, nullptr, &size) != ERROR_SUCCESS) {
            return false;
        }

        if ((type != REG_SZ && type != REG_EXPAND_SZ) || size < sizeof(wchar_t)) {
            return false;
        }

        std::vector<wchar_t> buffer(size / sizeof(wchar_t), L'\0');
        if (RegQueryValueExW(
                key,
                valueName,
                nullptr,
                &type,
                reinterpret_cast<LPBYTE>(buffer.data()),
                &size) != ERROR_SUCCESS) {
            return false;
        }

        buffer.back() = L'\0';
        value.assign(buffer.data());
        return !value.empty();
    }

    std::string wideToUtf8(const std::wstring& value) {
        if (value.empty()) {
            return {};
        }

        const int size = WideCharToMultiByte(
            CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) {
            return {};
        }

        std::string result(static_cast<size_t>(size), '\0');
        if (WideCharToMultiByte(
                CP_UTF8,
                0,
                value.c_str(),
                -1,
                result.data(),
                size,
                nullptr,
                nullptr) <= 0) {
            return {};
        }

        if (!result.empty() && result.back() == '\0') {
            result.pop_back();
        }
        return result;
    }
} // namespace

namespace Platform {

std::string getOsName() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            0,
            KEY_READ,
            &key) != ERROR_SUCCESS) {
        return "Windows (unknown)";
    }

    std::wstring buildString;
    std::wstring displayVersion;
    const bool gotBuild = readRegistryString(key, L"CurrentBuildNumber", buildString);
    const bool gotDisplayVersion = readRegistryString(key, L"DisplayVersion", displayVersion);
    RegCloseKey(key);

    if (!gotBuild || !gotDisplayVersion) {
        return "Windows (unknown)";
    }

    try {
        const int build = std::stoi(buildString);
        const std::wstring family = build >= 22000 ? L"Windows 11" : L"Windows 10";
        const std::wstring formatted =
            family + L" " + displayVersion + L" (Build " + buildString + L")";

        const std::string utf8 = wideToUtf8(formatted);
        return utf8.empty() ? "Windows (unknown)" : utf8;
    } catch (...) {
        return "Windows (unknown)";
    }
}

} // namespace Platform

#endif
