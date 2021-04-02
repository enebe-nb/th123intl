#include <windows.h>
#include <fstream>
#include <stack>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include "main.hpp"

// TODO use SAX for parsing and const char* for storing
namespace {
    std::unordered_map<std::string, std::string> keyMap;
}

typedef std::stack<std::pair<std::string, const nlohmann::json&>> parserStack_t;
void LoadKeyMap(const std::filesystem::path& filename) {
    std::ifstream data(filename);
    nlohmann::json json;
    data >> json;
    parserStack_t stack;
    stack.push(parserStack_t::value_type::pair("", json));

    while(!stack.empty()) {
        std::string parent = stack.top().first;
        if (!parent.empty()) parent += ".";
        auto& node = stack.top().second;
        stack.pop();

        for(auto& iter = node.begin(); iter != node.end(); ++iter) {
            if (iter.value().is_object()) {
                stack.push(parserStack_t::value_type::pair(parent + iter.key(), iter.value()));
            } else if (iter.value().is_string()) {
                keyMap[parent+iter.key()] = iter.value().get<std::string>();
            }
        }
    }
}

template <std::size_t S>
static bool checkLangExistsAndSet(const wchar_t* lang, wchar_t (&language)[S]) {
    std::filesystem::path tpath = modulePath / L"locale" / lang;
    tpath.replace_extension(L".json");
    if (std::filesystem::exists(tpath)) {
        if (language != lang) wcscpy_s(language, lang);
        return true;
    }
    return false;
}

static const wchar_t* FindLanguage() {
    static wchar_t language[256];
    GetPrivateProfileStringW(L"Locale", L"Lang", L"auto", language, 256, (modulePath / L"th123intl.ini").c_str());
    if (wcscmp(language, L"auto") == 0) {
        ULONG numLanguages; ULONG bufferSize;
        if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, 0, &bufferSize)) {
            wchar_t* buffer = new wchar_t[bufferSize];
            if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, buffer, &bufferSize)) {
                wchar_t* lang = buffer;
                while(*lang) {
                    size_t len = wcslen(lang);
                    if (checkLangExistsAndSet(lang, language)) break;
                    else {
                        wchar_t* bc = wcschr(lang, L'-');
                        if (bc) {
                            *bc = '\0';
                            if (checkLangExistsAndSet(lang, language)) break;
                        }
                    }
                    lang += len + 1;
                }
            }
            delete[] buffer;
        }
    }

    if (!checkLangExistsAndSet(language, language)) {
        wcscpy_s(language, L"en");
    }

    return language;
}

void LoadLanguage() {
    const wchar_t* language = FindLanguage();
    languagePack = modulePath / L"locale" / language;
    languagePack.replace_extension(L".dat");
    if (!std::filesystem::exists(languagePack)) {
        languagePack = modulePath / L"locale" / L"en.dat";
        if (!std::filesystem::exists(languagePack)) languagePack.clear();
    }

    std::filesystem::path languageData = modulePath / L"locale" / language;
    languageData.replace_extension(L".json");
    if (std::filesystem::exists(languagePack)) {
        LoadKeyMap(languageData);
    }
}

extern "C" __declspec(dllexport) const char* GetTranslation(const char* key) {
    auto iter = keyMap.find(key);
    if (iter != keyMap.end()) return iter->second.c_str();
    else return 0;
}

extern "C" __declspec(dllexport) void SetTranslation(const char* key, const char* value) {
    keyMap[key] = value;
}
