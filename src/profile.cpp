#include <SokuLib.hpp>
#include <windows.h>
#include <mbstring.h>

#include "main.hpp"
#include "th123intl.hpp"

using namespace th123intl;
_locale_t u8locale = 0;

namespace {
    typedef int (__fastcall *server_incomingConn_t)(int, int, int, size_t&, char*, size_t, char*&, size_t&);
    server_incomingConn_t orig_server_incomingConn = (server_incomingConn_t) 0x004555c0;
    typedef void (__fastcall *client_incomingConn_t)(int, int, unsigned int, char*, size_t);
    client_incomingConn_t orig_client_incomingConn = (client_incomingConn_t) 0x00454760;
    typedef DWORD (WINAPI *client_requestConn_t)(unsigned int*);
    client_requestConn_t orig_client_requestConn = (client_requestConn_t) 0x00416030;
    typedef int (__fastcall *replay_getReplayPath_t)(void*, int, const char*, const char*, const char*);
    replay_getReplayPath_t orig_replay_getReplayPath = (replay_getReplayPath_t) 0x0042cb30;
    typedef int (__fastcall *createTextTexture_t)(void*, void*, void*, const char*, void*, int, int, int*, int*);
    createTextTexture_t orig_replay_createTexture = (createTextTexture_t) 0x004050a0;
    typedef SokuLib::String& (__fastcall *replay_appendDir_t)(SokuLib::String&, int, SokuLib::String&, int, int);
    replay_appendDir_t orig_replay_appendDir = (replay_appendDir_t) 0x004087b0;

    std::vector<int> detectOrder;
}

static inline unsigned int GetTCP() {
    return ((__crt_locale_data_public*)(langConfig.locale)->locinfo)->_locale_lc_codepage;
}

static bool __validAndConvert(int CP, char* buffer, size_t bSize, size_t& len) {
    if (CP == CP_UTF8) return true;
    std::string tmp; if (bSize < len) len = bSize;
    if (!validInCodePage(CP, std::string_view(buffer, len))) return false;
    if (!ConvertCodePage(CP, std::string_view(buffer, len), CP_UTF8, tmp)) return false;

    if (tmp.size() >= bSize) {
        const size_t charLen = _mbsnccnt_l((unsigned char*)tmp.c_str(), 31, u8locale);
        tmp.resize(_mbsnbcnt_l((unsigned char*)tmp.c_str(), charLen, u8locale));
    }

    if (strncpy_s(buffer, bSize, tmp.c_str(), tmp.size())) {
        throw std::runtime_error("th123intl: This should never happen.");
    } else {
        len = tmp.size();
        return true;
    }
}

static size_t __guessAndConvert(char* buffer, size_t bSize, size_t len) {
    if (validInCodePage(CP_UTF8, std::string_view(buffer, len))) return len;
    for (auto cp : detectOrder) if (__validAndConvert(cp, buffer, 32, len)) return len;
    for (int i = 0; i < len; ++i) if (buffer[i] < 0) buffer[i] = '?';
    return len;
}

static int __fastcall repl_server_incomingConn(int self, int unused, int index, size_t& connType, char* data, size_t dataSize, char*& replyData, size_t& replyDataSize) {
    std::string orig_clientName;
    if (data[0] == 1 && dataSize >= 0x28) {
        orig_clientName.assign(&data[2], data[1] < 31 ? data[1] : 31);
        data[1] = __guessAndConvert(&data[2], 32, data[1] < 32 ? data[1] : 32);
#ifdef _DEBUG
        logging << "Server.IncomingConn: ";
        logging << "01 " << std::string(&data[2], data[1]).c_str() << std::endl;
#endif
    }

    auto ret = orig_server_incomingConn(self, unused, index, connType, data, dataSize, replyData, replyDataSize);

    if (data[0] == 1 && connType == 0 && replyDataSize >= 0x44) {
        strncpy_s(&data[32], 32, orig_clientName.c_str(), orig_clientName.size());
    }

    return ret;
}

static void __fastcall repl_client_incomingConn(int self, int unused, unsigned int specInfo, char* data, size_t dataSize) {
    if (dataSize >= 0x44) {
        __guessAndConvert(data, 32, strnlen_s(data, 32));
        __guessAndConvert(&data[32], 32, strnlen_s(&data[32], 32));
    }

#ifdef _DEBUG
    logging << "Client.IncomingConn: ";
    logging << std::string(&data[0], 32).c_str() << " vs ";
    logging << std::string(&data[0x20], 32).c_str() << std::endl;
#endif
    return orig_client_incomingConn(self, unused, specInfo, data, dataSize);
}

static int __fastcall repl_replay_getReplayPath(void* handle, int unused, const char* format, const char* player2, const char* player1) {
    std::string p2buf, p1buf;
    bool hasp1 = ConvertCodePage(CP_UTF8, player1, GetACP(), p1buf);
    bool hasp2 = ConvertCodePage(CP_UTF8, player2, GetACP(), p2buf);
    return orig_replay_getReplayPath(handle, unused, format, hasp2 ? p2buf.c_str() : player2, hasp1 ? p1buf.c_str() : player1);
}

static errno_t repl_strcpy2utf8(char* dest, size_t dSize, const char* src) {
    size_t len = strlen(src); if (len > 3) len -= 3;
    if (GetACP() == CP_UTF8) {
        const size_t charLen = _mbsnccnt_l((unsigned char*)src, len < dSize-1 ? len : dSize-1, u8locale);
        len = _mbsnbcnt_l((unsigned char*)src, charLen, u8locale);
        return strncpy_s(dest, dSize, src, len);
    } else {
        std::string tmp;
        if (!ConvertCodePage(GetACP(), std::string_view(src, len), CP_UTF8, tmp)) return EINVAL;
        if (tmp.size() >= dSize) {
            const size_t charLen = _mbsnccnt_l((unsigned char*)tmp.c_str(), dSize-1, u8locale);
            tmp.resize(_mbsnbcnt_l((unsigned char*)tmp.c_str(), charLen, u8locale));
        }
        return strncpy_s(dest, dSize, tmp.c_str(), tmp.size());
    }
}

static void __fastcall repl_net_copyProfileName(SokuLib::String& dst, void* edx, const char* src, size_t len) {
    std::string tmp;
    if (ConvertCodePage<MB_USEGLYPHCHARS, WC_COMPOSITECHECK|WC_DEFAULTCHAR|WC_NO_BEST_FIT_CHARS>
        (CP_UTF8, std::string_view(src, len-1), GetTCP(), tmp)) dst = tmp;
    else dst.assign(src, len);
}

static void __fastcall repl_profile_updateName(SokuLib::Profile& profile) {
    std::string buffer; size_t len = profile.file.size >=3 ? profile.file.size - 3 : profile.file.size;
    if (GetACP() == GetTCP()
        || !ConvertCodePage(GetACP(), std::string_view((char*)profile.file, len), GetTCP(), buffer)) {
        buffer.assign((char*)profile.file, len);
    }

    if (buffer.size() > 23) {
        const size_t charLen = _mbsnccnt_l((unsigned char*)buffer.c_str(), 23, langConfig.locale);
        len = _mbsnbcnt_l((unsigned char*)buffer.c_str(), charLen, langConfig.locale);
    } else len = buffer.size();

    profile.name.assign(buffer.c_str(), len);
}

static void __fastcall repl_replay_formatName(SokuLib::CFileList& filelist, int unused, SokuLib::String& name) {
    std::string buffer; size_t len = name.size;
    if (filelist.extLength && len >= filelist.extLength) len -= filelist.extLength;
    if (GetACP() == GetTCP()
        || !ConvertCodePage(GetACP(), std::string_view((char*)name, len), GetTCP(), buffer)) {
        buffer.assign((char*)name, len);
    }

    if (filelist.maxLength && buffer.size() > filelist.maxLength) {
        const size_t charLen = _mbsnccnt_l((unsigned char*)buffer.c_str(), filelist.maxLength, langConfig.locale);
        len = _mbsnbcnt_l((unsigned char*)buffer.c_str(), charLen, langConfig.locale);
    }

    name.assign(buffer.c_str(), len);
}

static int __fastcall repl_CProfileListAppendLine(SokuLib::CProfileList& self, void* unused, SokuLib::String& out, void* unknown, SokuLib::Deque<SokuLib::String>& list, int index) {
    const unsigned int targetCP = GetTCP();
    std::string buffer;
    if (!ConvertCodePage(GetACP(), std::string_view(list[index], list[index].size), targetCP, buffer)) {
        out.append("!non-renderable!"); return 16;
    };

    if (self.extLength && buffer.size() >= self.extLength) buffer.resize(buffer.size() - 3);
    if (self.maxLength && buffer.size() >= self.maxLength) {
        const size_t charLen = _mbsnccnt_l((unsigned char*)buffer.c_str(), self.maxLength-1, langConfig.locale);
        buffer.resize(_mbsnbcnt_l((unsigned char*)buffer.c_str(), charLen, langConfig.locale));
    }
    if (buffer.empty()) return 0;

    out.append(buffer.c_str(), buffer.size());
    return buffer.size();
}

static SokuLib::String& __fastcall repl_replay_appendDir(SokuLib::String& dest, int unused, SokuLib::String& src, int offset, int length) {
    std::string buffer;
    if (ConvertCodePage(GetACP(), std::string_view((char*)src, src.size), GetTCP(), buffer)) {
        return dest.append(buffer.c_str()); // offset,length is always 0,-1 on this hook
    } else return orig_replay_appendDir(dest, unused, src, offset, length);
}

static int __fastcall repl_replay_createTexture(void* ecx, void* edx, void* dxHandle, const char* text, void* fontHandle, int texWidth, int texHeight, int* outWidth, int* outHeight) {
    const unsigned int targetCP = ((__crt_locale_data_public*)(langConfig.locale)->locinfo)->_locale_lc_codepage;
    if (GetACP() == targetCP) return orig_replay_createTexture(ecx, edx, dxHandle, text, fontHandle, texWidth, texHeight, outWidth, outHeight);
    std::string buffer; ConvertCodePage(GetACP(), std::string_view(text), targetCP, buffer);
    return orig_replay_createTexture(ecx, edx, dxHandle, buffer.c_str(), fontHandle, texWidth, texHeight, outWidth, outHeight);
}

void LoadProfile() {
    DWORD old;
    u8locale = _create_locale(LC_ALL, ".UTF8");

    { wchar_t orderStr[256]; orderStr[0] = L'\0';
        GetPrivateProfileStringW(L"Locale", L"DetectOrder", L"932,936", orderStr, 256, (modulePath / L"th123intl.ini").c_str());
        for (wchar_t *ctx, *token = wcstok(orderStr, L",", &ctx); token; token = wcstok(nullptr, L",", &ctx)) {
            auto val = wcstoul(token, nullptr, 10);
            if (!val) break;
            detectOrder.push_back(val);
        }
    }

    VirtualProtect((LPVOID)0x00401000, 0x00456000, PAGE_WRITECOPY, &old);
    *(int*)0x446a5b = 0x98;
    SokuLib::TamperNearJmpOpr(0x45366e, repl_strcpy2utf8); // server copy profile name
    *(int*)0x446c4b = 0x98;
    SokuLib::TamperNearJmpOpr(0x446c67, repl_strcpy2utf8); // client copy profile name
#ifndef SOKU_USE_NETUTF8
    SokuLib::TamperNearJmpOpr(0x453c56, repl_net_copyProfileName);
    SokuLib::TamperNearJmpOpr(0x453d42, repl_net_copyProfileName); // render copy network profile names
#endif
    SokuLib::TamperNearJmpOpr(0x435093, repl_profile_updateName);
    SokuLib::TamperNearJmpOpr(0x43543a, repl_profile_updateName); // on profile change update name from ACP

    orig_replay_getReplayPath = SokuLib::TamperNearJmpOpr(0x454365, repl_replay_getReplayPath); // network get path on replay save
    orig_replay_appendDir = SokuLib::TamperNearJmpOpr(0x42c804, repl_replay_appendDir); // on replay append dirname
    orig_replay_createTexture = SokuLib::TamperNearJmpOpr(0x0044b143, repl_replay_createTexture); // replay (big line)
    SokuLib::TamperNearJmpOpr(0x42c8c5, repl_replay_formatName);
    SokuLib::TamperNearJmpOpr(0x42c899, repl_replay_formatName); // on replay format regular file
    VirtualProtect((LPVOID)0x00401000, 0x00456000, old, &old);

    VirtualProtect((LPVOID)0x857000, 0x02b000, PAGE_WRITECOPY, &old);
    orig_server_incomingConn = SokuLib::TamperDword(0x858c9c, repl_server_incomingConn);
    orig_client_incomingConn = SokuLib::TamperDword(0x858cf4, repl_client_incomingConn); // network proccess incoming message

    SokuLib::TamperDword(0x8587a4, repl_CProfileListAppendLine); // render profile list
    VirtualProtect((LPVOID)0x857000, 0x02b000, old, &old);
}