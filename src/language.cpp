#include <windows.h>
#include <fstream>
#include <list>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include "main.hpp"

namespace {
    std::unordered_map<std::string, const char*> keyMap;

    inline void keyMapInsert(const std::string& key, const std::string& val) {
        char* value = new char[val.size() + 1];
        val.copy(value, val.size());
        value[val.size()] = '\0';

        auto iter = keyMap.find(key);
        if (iter == keyMap.end()) {
            keyMap[key] = value;
        } else {
            delete[] iter->second;
            iter->second = value;
        }
    }

    class : public nlohmann::json::json_sax_t {
    public:
        std::list<std::string> keyStack;
        int arrayIndex = -1;

        // TODO implement unused non-strings??
        bool null() override { return true; }
        bool binary(nlohmann::json::binary_t& val) override { return true; }
        bool boolean(bool val) override { return true; }
        bool number_integer(number_integer_t val) override { return true; }
        bool number_unsigned(number_unsigned_t val) override { return true; }
        bool number_float(number_float_t val, const string_t& s) override { return true; }

        bool string(string_t& val) override {
            if (arrayIndex >= 0) keyStack.back() = arrayIndex++;

            std::string key;
            for (auto& k : keyStack) {
                if (!key.empty()) key += '.';
                key += k;
            }

            keyMapInsert(key, val);
            return true;
        }

        bool start_object(std::size_t elements) override { keyStack.emplace_back(); return true; }
        bool key(string_t& val) override { keyStack.back() = val; return true; }
        bool end_object() override { keyStack.pop_back(); return true; }
        bool start_array(std::size_t elements) override { keyStack.emplace_back(); arrayIndex = 0; return true; }
        bool end_array() override { keyStack.pop_back(); arrayIndex = -1; return true; }

        bool parse_error(std::size_t position, const std::string& last_token, const nlohmann::json::exception& ex) override { return false; }
    } saxParser;
}

void LoadKeyMap(const std::filesystem::path& filename) {
    // TODO make it thread with lock
    std::ifstream data(filename);
    nlohmann::json json;
    json.sax_parse(data, &saxParser);
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
    if (iter != keyMap.end()) return iter->second;
    else return 0;
}

extern "C" __declspec(dllexport) void SetTranslation(const char* key, const char* value) {
    keyMap[key] = value;
}
