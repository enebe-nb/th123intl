#include <windows.h>
#include "main.hpp"

#include <nlohmann/json.hpp>

// global variable initialization
std::filesystem::path modulePath;
std::filesystem::path languagePack;
#ifdef _DEBUG
std::ofstream logging("th123intl.log");
#endif

static bool GetModulePath(HMODULE handle, std::filesystem::path& result) {
    // use wchar for better path handling
    std::wstring buffer;
    int len = MAX_PATH + 1;
    do {
        buffer.resize(len);
        len = GetModuleFileNameW(handle, buffer.data(), buffer.size());
    } while(len > buffer.size());

    if (len) result = std::filesystem::relative(std::filesystem::path(buffer.begin(), buffer.begin()+len).parent_path());
    return len;
}

const BYTE TARGET_HASH[16] = {0xdf, 0x35, 0xd1, 0xfb, 0xc7, 0xb5, 0x83, 0x31, 0x7a, 0xda, 0xbe, 0x8c, 0xd9, 0xf5, 0x3b, 0x2e};
extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
    return ::memcmp(TARGET_HASH, hash, sizeof TARGET_HASH) == 0;
}

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
    GetModulePath(hMyModule, modulePath);
#ifdef _DEBUG
    logging << "modulePath: " << modulePath << std::endl;
#endif

    LoadLanguage();
    if (langConfig.locale) {
        LoadHooks();
        LoadParser();
    }
    return true;
}

extern "C" __declspec(dllexport) void AtExit() {}

BOOL WINAPI DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    return true;
}