#include <windows.h>
#include <locale>

#ifndef TH123INTL_HPP
#define TH123INTL_HPP

namespace th123intl {
    typedef void* tlpack_t;

    inline unsigned int GetFileSystemCodePage() { return GetACP(); }
    inline unsigned int GetTextCodePage() {
        auto handle = GetModuleHandleA("th123intl.dll");
        FARPROC proc = 0;
        if (handle) proc = GetProcAddress(handle, "GetTextCodePage");
        if (!handle || !proc) return 932;
        return proc();
    }

    template <_locale_t& curLocale>
    inline _locale_t GetLocale() {
        if (curLocale) return curLocale;
        auto handle = GetModuleHandleA("th123intl.dll");
        FARPROC proc = 0;
        if (handle) proc = GetProcAddress(handle, "GetLocale");
        if (!handle || !proc) {
            if (!curLocale) curLocale = _create_locale(LC_ALL, "ja_JP");
            return curLocale;
        }
        return curLocale = (_locale_t)proc();
    }

    inline const char* GetLocaleName() {
        auto handle = GetModuleHandleA("th123intl.dll");
        FARPROC proc = 0;
        if (handle) proc = GetProcAddress(handle, "GetLocaleName");
        if (!handle || !proc) return "ja_JP";
        return (const char*) proc();
    }

    inline tlpack_t LoadTranslationPack(const char* name) {
        auto handle = GetModuleHandleA("th123intl.dll");
        FARPROC proc = 0;
        if (handle) proc = GetProcAddress(handle, "LoadTranslationPack");
        if (handle && proc) reinterpret_cast<tlpack_t (*)(const char*)>(proc)(name);
    }

    inline void FreeTranslationPack(tlpack_t pack) {
        auto handle = GetModuleHandleA("th123intl.dll");
        FARPROC proc = 0;
        if (handle) proc = GetProcAddress(handle, "FreeTranslationPack");
        if (handle && proc) reinterpret_cast<void (*)(tlpack_t)>(proc)(pack);
    }

    inline const char* GetTranslation(tlpack_t pack, const char* key, const char* defaultText) {
        auto handle = GetModuleHandleA("th123intl.dll");
        FARPROC proc = 0;
        if (handle) proc = GetProcAddress(handle, "GetTranslation");
        if (!handle || !proc) return defaultText;
        return reinterpret_cast<const char* (*)(tlpack_t, const char*, const char*)>(proc)(pack, key, defaultText);
    }

    inline bool LocalizeFont(const char* name, void* fontDescription) {
        auto handle = GetModuleHandleA("th123intl.dll");
        FARPROC proc = 0;
        if (handle) proc = GetProcAddress(handle, "LocalizeFont");
        if (!handle || !proc) return false;
        return reinterpret_cast<bool (*)(const char*, void* )>(proc)(name, fontDescription);
    }

    template<int flags = WC_COMPOSITECHECK|WC_DEFAULTCHAR>
    inline bool ConvertCodePage(const std::wstring_view& from, unsigned int toCP, std::string& to) {
        const DWORD dwFlags = toCP == CP_UTF8 || toCP == 54936 ? flags & WC_ERR_INVALID_CHARS : flags;
        size_t size = WideCharToMultiByte(toCP, dwFlags, from.data(), from.size(), NULL, NULL, NULL, NULL);
        to.resize(size);
        if (!size) return false;
        const bool ret = WideCharToMultiByte(toCP, dwFlags, from.data(), from.size(), to.data(), to.size(), NULL, NULL);
        if (!ret) to.resize(0);
        return ret;
    }

    template<int flags = MB_USEGLYPHCHARS>
    inline bool ConvertCodePage(unsigned int fromCP, const std::string_view& from, std::wstring& to) {
        const DWORD dwFlags = fromCP == CP_UTF8 || fromCP == 54936  ? flags & WC_ERR_INVALID_CHARS : flags;
        size_t size = MultiByteToWideChar(fromCP, dwFlags, from.data(), from.size(), NULL, NULL);
        to.resize(size);
        if (!size) return false;
        const bool ret = MultiByteToWideChar(fromCP, dwFlags, from.data(), from.size(), to.data(), to.size());
        if (!ret) to.resize(0);
        return ret;
    }

    template<int inFlags = MB_USEGLYPHCHARS, int outFlags = WC_COMPOSITECHECK|WC_DEFAULTCHAR>
    inline bool ConvertCodePage(unsigned int fromCP, const std::string_view& from, unsigned int toCP, std::string& to) {
        if (fromCP == toCP) { to = from; return true; }
        std::wstring buffer;
        if (!ConvertCodePage<inFlags>(fromCP, from, buffer)) { to.resize(0); return false; }
        return ConvertCodePage<outFlags>(buffer, toCP, to);
    }

    inline bool validInCodePage(int CP, const std::string_view& str) {
        if (str.empty()) return true;
        return MultiByteToWideChar(CP, MB_ERR_INVALID_CHARS, str.data(), str.size(), NULL, NULL);
    }
}

#endif
