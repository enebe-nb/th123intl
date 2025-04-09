#include <windows.h>
#include "main.hpp"
#include "th123intl.hpp"

#include <nlohmann/json.hpp>
#include <SokuLib.hpp>

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

    if (len) result = std::filesystem::path(buffer.begin(), buffer.begin()+len).parent_path();
    return len;
}

const BYTE TARGET_HASH[16] = {0xdf, 0x35, 0xd1, 0xfb, 0xc7, 0xb5, 0x83, 0x31, 0x7a, 0xda, 0xbe, 0x8c, 0xd9, 0xf5, 0x3b, 0x2e};
extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
    return ::memcmp(TARGET_HASH, hash, sizeof TARGET_HASH) == 0;
}

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
    GetModulePath(hMyModule, modulePath);
#ifdef _DEBUG
    std::string pathText; th123intl::ConvertCodePage(modulePath.wstring(), CP_ACP, pathText);
    logging << "modulePath: " << pathText << std::endl;
#endif

    LoadLanguage();
    if (langConfig.locale) {
        LoadHooks();
        LoadParser();
        LoadProfile();
    }
    return true;
}

extern "C" __declspec(dllexport) void AtExit() {}

extern "C" __declspec(dllexport) unsigned int GetTextCodePage() {
    if (!langConfig.locale) return 932;
    return ((__crt_locale_data_public*)(langConfig.locale)->locinfo)->_locale_lc_codepage;
}

extern "C" __declspec(dllexport) void* GetLocale() {
    return (void*) langConfig.locale;
}

extern "C" __declspec(dllexport) const char* GetLocaleName() {
    return langConfig.localeName.c_str();
}

extern "C" __declspec(dllexport) int getPriority() {
    return -200;
}

namespace {
    struct tlpack_t {
        SokuLib::CSVParser parser;
        std::unordered_map<const char*, const char*> map;
        inline tlpack_t(const char* name) : parser(name) {}
    };
}

extern "C" __declspec(dllexport) void FreeTranslationPack(void* pack) {
    delete (tlpack_t*) pack;
}

extern "C" __declspec(dllexport) const char* GetTranslation(void* pack, const char* key, const char* defaultText) {
    if (!pack) return defaultText;
    auto& tltable = ((tlpack_t*)pack)->map;
    auto iter = tltable.find(key);
    if (iter == tltable.end()) return defaultText;
    return iter->second ? iter->second : defaultText;
}

extern "C" __declspec(dllexport) void* LoadTranslationPack(const char* name) {
    if (!name) return 0;
    char bName[50] = "data/intl_";
    strncat(bName, name, 32); strcat(bName, ".cv1");
    auto pack = new tlpack_t(bName);

    for(int i = 0; i < pack->parser.data.size(); ++i) {
        auto& pair = pack->parser.data.at(i);
        if (pair.size() < 2) continue;
        auto first = (const char*)pair[0]; auto second = (const char*)pair[1];
        if (!first || !second) continue;
        pack->map.insert_or_assign(first, second);
    }

    return (void*) pack;
}

extern "C" __declspec(dllexport) bool LocalizeFont(const char* name, SokuLib::FontDescription* desc) {
    auto iter = langConfig.customFonts.find(name);
    if (iter == langConfig.customFonts.end()) return false;

    for(auto& override : iter->second) {
        memcpy((char*)desc + override.offset, override.data, override.size);
    }
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    return true;
}