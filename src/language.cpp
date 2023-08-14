#include <fstream>

#include <SokuLib.hpp>
#include <nlohmann/json.hpp>
#include "main.hpp"

struct LangConfig langConfig;
LangConfig::~LangConfig() {
    if (locale) _free_locale(locale);
    for (auto& i : this->fontOverrides) {
        for (auto& j : i.second) {
            delete j.data;
        }
    }
}

namespace {
    std::unordered_map<std::string_view, unsigned int> fontMap = {
        {"unknown01", 0x4126ad},
        {"profile", 0x434de1},
        {"deck", 0x438611},
        {"gui", 0x43d883},
        {"popup", 0x444142},
        {"unknown02", 0x44ba52},
        {"number", 0x450b52},
        {"unknown03", 0x453cd1},
        {"unknown04", 0x453dbd},
        {"unknown05", 0x45c5ac},
        {"unknown06", 0x45c6f6},
        {"unknown07", 0x45f110},
        {"unknown08", 0x46202a},
        {"story", 0x462990},
    };

    std::unordered_map<std::string_view, unsigned int> tilesMap = {
        {"deck", 0x450c0f},
        {"ipport", 0x4488b3},
    };

    LangConfig::FontOverride setString(nlohmann::json& value) {
        std::string name; value.get_to(name);
        char* buffer = new char[name.size()+1];
        memcpy(buffer, name.data(), name.size());
        buffer[name.size()] = '\0';
        return {buffer, offsetof(SokuLib::FontDescription, faceName), name.size()+1};
    }

    template <typename T, size_t offset>
    LangConfig::FontOverride setAny(nlohmann::json& value) {
        T* buffer = new T;
        value.get_to(*buffer);
        return {buffer, offset, sizeof(T)};
    }

    using setter_t = LangConfig::FontOverride (*)(nlohmann::json&);
    std::unordered_map<std::string_view, setter_t> fontAttr = {
        {"name", setString},
        {"r1", setAny<BYTE, offsetof(SokuLib::FontDescription, r1)>},
        {"r2", setAny<BYTE, offsetof(SokuLib::FontDescription, r2)>},
        {"g1", setAny<BYTE, offsetof(SokuLib::FontDescription, g1)>},
        {"g2", setAny<BYTE, offsetof(SokuLib::FontDescription, g2)>},
        {"b1", setAny<BYTE, offsetof(SokuLib::FontDescription, b1)>},
        {"b2", setAny<BYTE, offsetof(SokuLib::FontDescription, b2)>},
        {"height", setAny<LONG, offsetof(SokuLib::FontDescription, height)>},
        {"weight", setAny<LONG, offsetof(SokuLib::FontDescription, weight)>},
        {"italic", setAny<BYTE, offsetof(SokuLib::FontDescription, italic)>},
        {"shadow", setAny<BYTE, offsetof(SokuLib::FontDescription, shadow)>},
        {"wrap", setAny<BYTE, offsetof(SokuLib::FontDescription, useOffset)>},
        {"paddingX", setAny<DWORD, offsetof(SokuLib::FontDescription, offsetX)>},
        {"paddingY", setAny<DWORD, offsetof(SokuLib::FontDescription, offsetY)>},
        {"spacingX", setAny<DWORD, offsetof(SokuLib::FontDescription, charSpaceX)>},
        {"spacingY", setAny<DWORD, offsetof(SokuLib::FontDescription, charSpaceY)>},
    };

    std::unordered_map<std::string_view, unsigned int> tilesAttr = {
        {"offsetX", 0},
        {"offsetY", 1},
        {"width", 2},
        {"height", 3},
        {"spacing", 4},
    };
}

static inline void SetOverrides(std::vector<LangConfig::FontOverride>& out, nlohmann::json& node) {
    for (auto iter = node.begin(); iter != node.end(); ++iter) {
        if (!fontAttr.count(iter.key())) continue;
        out.push_back(fontAttr.at(iter.key())(iter.value()));
    }
}
#include <iostream>
static void LoadConfig(const std::filesystem::path& filename) {
    std::ifstream data(filename);
    try {
        nlohmann::json json = nlohmann::json::parse(data);
        std::cout << json << std::endl;

        langConfig.locale = _create_locale(LC_ALL, json.at("locale").get<std::string>().c_str());
        if (!langConfig.locale) throw std::exception("invalid locale");

        json.at("charset").get_to(langConfig.charset);

        auto array = json["packs"];
        if (array.is_array()) for (auto iter = array.begin(); iter != array.end(); ++ iter) {
            langConfig.packFiles.push_back(modulePath / (char8_t*)iter->get<std::string>().c_str());
        }

        array = json["font"]["files"];
        if (array.is_array()) for (auto iter = array.begin(); iter != array.end(); ++ iter) {
            AddFontResourceExW((modulePath / (char8_t*)iter->get<std::string>().c_str()).c_str(), FR_PRIVATE, 0);
        }

        array = json["font"];
        if (array.is_object()) for (auto iter = array.begin(); iter != array.end(); ++ iter) {
            if (!iter->is_object()) continue;
            auto font = fontMap.find(iter.key());
            if (font == fontMap.end()) continue;
            SetOverrides(langConfig.fontOverrides[font->second], *iter);
        }

        array = json["tiles"];
        if (array.is_object()) for (auto iter = array.begin(); iter != array.end(); ++ iter) {
            if (!iter->is_object()) continue;
            auto tile = tilesMap.find(iter.key());
            if (tile == tilesMap.end()) continue;
            for (auto attr = iter->begin(); attr != iter->end(); ++attr) {
                if (!tilesAttr.count(attr.key())) continue;
                langConfig.tileOverrides[tile->second]
                    .push_back({attr.value().get<unsigned int>(), tilesAttr.at(attr.key())});
            }
        }
    } catch (const std::exception& e) {
#ifdef _DEBUG
        logging << "Failure loading json File" << std::endl;
        logging << e.what() << std::endl;
#endif
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
        ULONG numLanguages; ULONG bufferSize = 0;
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
    if (!std::filesystem::exists(modulePath / L"th123intl.ini"))
        WritePrivateProfileStringW(L"Locale", L"Lang", L"auto", (modulePath / L"th123intl.ini").c_str());
    const wchar_t* language = FindLanguage();

    std::filesystem::path languageData = modulePath / L"locale" / language;
    languageData.replace_extension(L".json");
    if (std::filesystem::exists(languageData)) {
        LoadConfig(languageData);
    }
}