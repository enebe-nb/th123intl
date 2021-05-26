#include <windows.h>

#include "main.hpp"

#ifdef _DEBUG
    #define UTFERROR(handle, buffer) { \
        char errmsg[80]; \
        sprintf_s(errmsg, "Utf-8 error with handle %08X and data %08X.", (unsigned int)handle, (unsigned int)buffer); \
        MessageBoxA(GetActiveWindow(), errmsg, "th123intl Error", MB_OK); }
#else
    #define UTFERROR(handle, buffer)
#endif

namespace {
    typedef void (*appendDataPackage_t)(const char*);
    appendDataPackage_t orig_appendDataPackage;
    typedef int (__fastcall *parseHtmlTag_t)(void*, void*, const char*, int*);
    parseHtmlTag_t orig_parseHtmlTag = (parseHtmlTag_t) 0x412240;
    typedef void (__stdcall *parseIChar_t)(unsigned int, int*, int*);
    parseIChar_t orig_parseIChar = (parseIChar_t) 0x00411e20;
    typedef void (__fastcall *orig_text_func_t)(unsigned int ecx, void* edx, void *a, void *b, void *c, unsigned int d);
    orig_text_func_t orig_text_func = (orig_text_func_t) 0x40fc00;

    typedef void (__fastcall *copyInFilelist_t)(void*, void*, unsigned int);
    copyInFilelist_t orig_copyInFilelist = (copyInFilelist_t) 0x0043caa0;
    typedef void (__fastcall *copyInProfile_t)(unsigned int, void*, unsigned int, unsigned int, int);
    copyInProfile_t orig_copyInProfile = (copyInProfile_t) 0x004020d0;
    typedef void (__fastcall *stringFromCstr_t)(void*, void*, const char*, unsigned int);
    stringFromCstr_t orig_stringFromCstr = (stringFromCstr_t) 0x00402200;
    typedef unsigned int (__fastcall *getintbl_t)(void*, void*, unsigned int);
    getintbl_t orig_getintbl = (getintbl_t) 0x0043cbc0;
    typedef int (__fastcall *createTextTexture_t)(void*, void*, void*, const char*, void*, int, int, int*, int*);
    createTextTexture_t orig_createTextTexture = (createTextTexture_t) 0x004050a0;

    typedef int (__fastcall *loadImage_t)(int*, char*, void**, int*);
    loadImage_t orig_loadImage = (loadImage_t) 0x00408cf0;

    static struct {
        uint32_t addr;
        const char *string;
    } text_table[] = {
        // DINPUT
        { 0x40d66d, "\\system.dinput.failCreate0" },
        { 0x40d6a5, "\\system.dinput.failCreate1" },
        { 0x40d867, "\\system.dinput.failKeyboard" },
        { 0x40d898, "\\system.dinput.failDataFormat" },
        { 0x40d8ce, "\\system.d3d.failCoopLevel" },
        { 0x40d930, "\\system.dinput.failQueue" },
        { 0x40db1a, "\\system.dinput.failMouse" },
        { 0x40db4e, "\\system.dinput.failDataFormat" },
        { 0x40db87, "\\system.d3d.failCoopLevel" },
        { 0x40dbd8, "\\system.dinput.failProps" },

        // SOCKET
        { 0x41312c, "\\system.socket.failInit" },
        { 0x413331, "\\system.socket.failPort" },

        // DIRECT3D
        { 0x414f05, "\\system.d3d.failObject" },
        { 0x414f35, "\\system.d3d.failDisplay" },
        { 0x415079, "\\system.d3d.failDevice" },

        // DSOUND
        { 0x418392, "\\system.dsound.failBuffer" },
        { 0x4183d1, "\\system.dsound.failBuffer3D" },
        { 0x41851e, "\\system.dsound.failBuffer" },
        { 0x4185ca, "\\system.dsound.failBufferLock" },
        { 0x4186f2, "\\system.dsound.failBufferLock" },
        { 0x41884c, "\\system.dsound.failCreateObject" },
        { 0x41888b, "\\system.dsound.failInit" },
        { 0x4188e1, "\\system.d3d.failCoopLevel" },
        { 0x4189ab, "\\system.dsound.failCreatePrimary" },
        { 0x4189f7, "\\system.dsound.failCreateListener" },
        { 0x418af1, "\\system.dsound.failCreateTemporary" },

        // ERROR
        { 0x442e81, "\\system.profile.failFolder" },
        { 0x442eac, "\\system.replay.failFolder" },
        { 0x7fb88d, "\\system.failInit" },

        // Replay
        { 0x444d50, "<key system.replay.returnSelect>" },
        { 0x444d6f, "<key system.replay.returnTitle>" },
        { 0x444e75, "<key system.replay.saved>" },
        { 0x4492e4, "<key system.replay.saved>" },
        { 0x4496b0, "<key system.replay.saved>" },
        { 0x449298, "<key system.replay.save>" },
        { 0x449682, "<key system.replay.save>" },
        { 0x44b57e, "<key system.replay.deleted>" },
        { 0x44b598, "<key system.replay.failDelete>" },
        { 0x44b654, "<key system.replay.delete>" },

        // Netplay
        { 0x448f37, "<key system.netplay.selectSaved>" },
        { 0x448f5b, "<key system.netplay.selectPrevious>" },
        { 0x446842, "<key system.netplay.selectPort>" },
        { 0x4468e2, "<key system.netplay.selectAddress>" },
        { 0x446e8f, "<key system.netplay.allowSpec>" },
        { 0x446e3c, "<key system.netplay.waithost>" },
        { 0x448c92, "<key system.netplay.failSpec>" },
        { 0x448c76, "<key system.netplay.failStart>" },
        { 0x448b73, "<key system.netplay.failConn>" },
        { 0x448c5a, "<key system.netplay.failConn>" },
        { 0x448cae, "<key system.netplay.failConn>" },
        { 0x448afa, "<key system.netplay.waitClient>" },
        { 0x448ab0, "<key system.netplay.confirmMatch>" },
        { 0x44902c, "<key system.netplay.confirmAddress>" },
        { 0x448b6a, "<key system.netplay.failPlaying>" },
        { 0x448b5a, "<key system.netplay.failPlayingSpec>" },

        // Replay/continue/etc
        { 0x449319, "<key system.story.continueConfirm>%d<key system.story.continueLeft>" },

        // Profile stuff
        { 0x450256, "<key system.profile.confirmSave>" },
        { 0x4503c9, "<key system.profile.confirmSave>" },
        { 0x450188, "<key system.profile.fail20Cards>" },
        { 0x452090, "<key system.profile.failCreate>" },
        { 0x4520f8, "<key system.profile.failCreate>" },
        { 0x452009, "<key system.profile.created>" },
        { 0x452972, "<key system.profile.confirmOverwrite>" },
        { 0x452f6c, "<key system.profile.confirmOverwrite>" },
        { 0x452ae1, "<key system.profile.confirmCopy>" },
        { 0x452c71, "<key system.profile.confirmDelete>" },

        // stupid spellcard clip fix
        //{ 0x43780e, "<br><br>" },
        //{ 0x437ff4, "<br><br>" },

        { 0x448872, "0 1 2 3 4 5 6 7 8 9<br>^<br>     .      .      .      : " },
        { 0x44c90b, "%s - %s  %03d/%03d  Time %02d.%02d" },
        { 0x44c947, "<color 808080>? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? </color>" },
        { 0x450b7d, "%d    <color FF8080>%d</color>    /%d<color FF8080>  /%d</color><br>" },

        { 0, 0 }
    };
}

inline DWORD TamperNearJmpOpr(DWORD addr, DWORD target) {
    DWORD old = *reinterpret_cast<PDWORD>(addr + 1) + (addr + 5);
    *reinterpret_cast<PDWORD>(addr + 1) = target - (addr + 5);
    return old;
}

template <std::size_t S>
inline void TamperCode(uint32_t address, const uint8_t (&code)[S]) {
    memcpy((void*)address, code, S);
}

template <std::size_t S>
inline void TamperStringWithSize(uint32_t addrStr, uint32_t addrLen, const char (&code)[S]) {
    *(uint8_t*)addrLen = S - 1;
    *(const char**)addrStr = code;
}

static void repl_appendDataPackage(const char* filename) {
    orig_appendDataPackage(filename);
    orig_appendDataPackage((modulePath / L"strkeys.dat").string().c_str());
    if (!languagePack.empty()) orig_appendDataPackage(languagePack.string().c_str());
}

static bool applyWordBreak(void* handle, const char* word) {
    wchar_t wstr[512];
    size_t len = strcspn(&word[1], " <") + 1;
    size_t wlen = MultiByteToWideChar(CP_UTF8, 0, word, len, wstr, 512);
    if (wlen) {
        SIZE textSize;
        if (GetTextExtentPoint32W(*(HDC*)handle, wstr, wlen, &textSize)) {
            int width = textSize.cx + *(int*)((int)handle + 0x134) + *(int*)((int)handle + 0x12c) * wlen;
            if (width > *(int*)((int)handle + 0x14c)) {
                *(int*)((int)handle + 0x134) = *(int*)((int)handle + 0x124) + *(char*)((int)handle + 0x11d);
                *(int*)((int)handle + 0x138) += *(int*)((int)handle + 0x130) + *(int*)((int)handle + 0x114);
                return true;
            }
        }
    }
    return false;
}

static int parseHtmlTag(void* handle, const char* text, int* out1, int* out2);
static int printNextChar(void* handle, const char* buffer, int index, int len, int* out1, int* out2) {
#ifdef _DEBUG
    static const char* lastBuffer = 0;
    if (lastBuffer != buffer) {
        logging << "renderText: " << (void*)buffer << " handle: " << handle << std::endl;
        if (buffer > (const char*)0x400000) logging << "content: " << buffer << std::endl;
        logging.flush();
        lastBuffer = buffer;
    }
#endif

    uint32_t lc = (uint8_t)buffer[index];
    if (!lc) return 0;
    if (lc == '<') return 1 + parseHtmlTag(handle, &buffer[index+1], out1, out2);
    if (lc == ' ' && *((char*)handle + 0x11e))
        if(applyWordBreak(handle, &buffer[index])) return 1;

    if (lc < 0x80) { // 1byte
        __asm {
            push out2;
            push out1;
            push lc;
            mov eax, handle;
            call orig_parseIChar;
        }; // fuck eax
        return 1;
    } else if (lc < 0xc0) {
        // error: received continuation byte on leading byte
        UTFERROR(handle, buffer);
        return 0;
    } else if (lc >= 0xf4) {
        // error: leading byte specifies more than 4 bytes
        UTFERROR(handle, buffer);
        return 0;
    } else {
        int bytes; for(bytes = 2; lc & (0x80 >> bytes); ++bytes);
        uint32_t c = lc & (0xff >> bytes+1);
        for (int i = 1; i < bytes; ++i) {
            if (buffer[index+i] == '\0') {
                //error: end of string 
                UTFERROR(handle, buffer);
                return 0;
            }
            if ((buffer[index+i] & 0xc0) != 0x80) {
                //error: expecting continuation byte
                UTFERROR(handle, buffer);
                return 0;
            }
            c = (c << 6) | (buffer[index+i] & 0x3f);
        }
        __asm {
            push out2;
            push out1;
            push c;
            mov eax, handle;
            call orig_parseIChar;
        }; // fuck eax
        return bytes;
    }
}

static int parseHtmlTag(void* handle, const char* text, int* out1, int* out2) {
    const char* end = strchr(text, '>');
    if (end && strncmp(text, "key ", 4) == 0) {
        const char* value = GetTranslation(std::string(text+4, end).c_str());
        if (value) {
            size_t len = strlen(value);
            for (size_t i = 0, j = -1; i < len && j != 0; i += j) {
                j = printNextChar(handle, value, i, len, out1, out2);
            }
        }
        return end - text + 1;
    }

    return orig_parseHtmlTag(out2, handle, text, out1);
}

inline unsigned int _div255(unsigned int v) { return (v+1+(v>>8))>>8; }
static void __fastcall repl_alphaBlend(unsigned int color, unsigned int alpha, unsigned int* out) {
    unsigned short a0 = ((alpha*0xffu) >> 4) & 0xff;
    unsigned short a1 = *out >> 24;
    unsigned short a2 = a0 + _div255(a1*(255-a0));

    unsigned int result = (unsigned int)a2 << 24;
    if (a2 != 0) for (int i = 0; i < 3; ++i) {
        unsigned short c0 = (color >> i*8) & 0xff;
        unsigned short c1 = (*out >> i*8) & 0xff;
        unsigned short c2 = (c0*a0 + _div255(c1*a1)*(255-a0)) / a2;
        result |= ((unsigned int)c2 & 0xff) << i*8;
    }

    *out = result;
}

static void __fastcall repl_textShadow(int height, int width, int line, unsigned int* input, unsigned int* output) {
    width -= 1;
    height -= 1;

    for (int j = 1; j < height; ++j) {
        for (int i = 1; i < width; ++i) {
            unsigned int c0 = input[j*line+i];
            if (c0>>24) {
                unsigned alpha = c0>>27;
                if (alpha > 16) alpha = 16;
                unsigned int c1 = 0xff000000;
                repl_alphaBlend(c0, alpha, &c1);
                repl_alphaBlend(c1, 16, &output[j*line+i]);
            } else {
                unsigned char current = input[(j-1)*line+i] >> 24;
                unsigned char next = input[(j+1)*line+i] >> 24;
                if (current < next) current = next;
                next = input[j*line+(i-1)] >> 24;
                if (current < next) current = next;
                next = input[j*line+(i+1)] >> 24;
                if (current < next) current = next;
                repl_alphaBlend(0, current >> 4, &output[j*line+i]);
            }
        }
    }
} __declspec(naked) static void _repl_textShadow() {
    __asm {
        mov ecx, [eax+0x148];
        mov edx, [eax+0x14c];
        mov eax, [eax+0x150];
        pop ebx;
        push eax;
        push ebx;
        jmp repl_textShadow;
    }
}

// we do still need escape character for other mods
static unsigned char* __stdcall text_passthrough(unsigned char *ebp, unsigned int esp, void *a, void *b, void *c, unsigned int d) {
    static bool kana = 0;
    int dh = d & 0xff;

    // do keep files as shiftjis?
    if ((dh >= 0x80 && dh <= 0xa0) || dh >= 0xe0) {
        kana = 1;
    } else if (kana) {
        kana = 0;
    } else {
        if (dh == '\\') {
            unsigned char c = *(ebp+1);
            if (c == '\'' || c == '\"' || c == '\\' || c == ',') {
                ebp++;
                d = (d&~0xff) | c;
            }
        }
    }

    orig_text_func(esp + 0x68, 0, a, b, c, d);
    return ebp;
}

// we do still need escape character for other mods
static int __stdcall script_passthrough(char *ebp, char *edx) {
    char *orig_edx = edx;
    int retval = 0;
    ebp -= 0x24;

    unsigned char ch = *edx;
    if (ch == '\\' && *(edx+1) == ',' && (ebp[','>>3]&(1<<(','&7)))) {
        do {
            *edx = *(edx+1);
            edx++;
        } while (*edx != '\0' && *edx != '\n' && *edx != '\r');
    } else {
        retval = ebp[ch>>3]&(1<<(ch&7));
    }

    __asm mov edx, orig_edx;
    return retval;
}

template <std::size_t S>
inline int ACP2Utf8(char (&dst)[S], const char* src, unsigned int len) {
    wchar_t wstr[S];
    int wlen = MultiByteToWideChar(GetACP(), 0, src, len, wstr, S);
    if (wlen) {
        int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, dst, S, 0, 0);
        dst[u8len] = '\0';
#ifdef _DEBUG
        logging << "str convert: \"" << src << "\" -> \"" << dst << "\"" <<std::endl;
#endif
        return u8len;
    }
    return 0;
}

static void __fastcall repl_copyInFilelist(void* ecx, void* edx, unsigned int str) {
    if (GetACP() == CP_UTF8) return orig_copyInFilelist(ecx, edx, str);
    const char* cstr = *(int*)(str + 0x18) >= 0x10 ? *(const char**)(str + 0x4) : (const char*)(str + 0x4);
    int len = *(int*)(str + 0x14);

    char buffer[512];
    len = ACP2Utf8(buffer, cstr, len);
    if (len) orig_stringFromCstr((void*)str, edx, buffer, len);

    return orig_copyInFilelist(ecx, edx, str);
}

static void __fastcall repl_copyInProfile(unsigned int dst, void* edx, unsigned int src, unsigned int offset, int len) {
    if (GetACP() == CP_UTF8) return orig_copyInProfile(dst, edx, src, offset, len);
    const char* cstr = *(int*)(src + 0x18) >= 0x10 ? *(const char**)(src + 0x4) : (const char*)(src + 0x4);
    cstr += offset;
    if (len == -1) len = *(int*)(src + 0x14) - offset;

    char buffer[512];
    len = ACP2Utf8(buffer, cstr, len);
    if (len) orig_stringFromCstr((void*)dst, edx, buffer, len);
    else orig_copyInProfile(dst, edx, src, offset, len);
}

static int __fastcall repl_createTextTexture(void* ecx, void* edx, void* dxHandle, const char* text, void* fontHandle, int texWidth, int texHeight, int* outWidth, int* outHeight) {
    if (GetACP() == CP_UTF8) return orig_createTextTexture(ecx, edx, dxHandle, text, fontHandle, texWidth, texHeight, outWidth, outHeight);
    char buffer[512];
    int len = strlen(text);

    len = ACP2Utf8(buffer, text, len);
    if (len) text = buffer;
    return orig_createTextTexture(ecx, edx, dxHandle, text, fontHandle, texWidth, texHeight, outWidth, outHeight);
}

static int __fastcall repl_loadImage(int* out2, char* filename, void** texture, int* out1) {
    int result = orig_loadImage(out2, filename, texture, out1);
    if (result >= 0) {
        auto iter = texRedraw.find(filename);
        if (iter != texRedraw.end()) iter->second->execute(*texture);
    } return result;
}

int __stdcall repl_MessageBoxUtf8(HWND window, const char* content, const char* title, unsigned int type) {
    if (content && *content == '\\') {
        const char * tn = GetTranslation(content+1);
        if (tn) content = tn;
    }

    if (title && *title == '\\') {
        const char * tn = GetTranslation(title+1);
        if (tn) title = tn;
    }

    bool cantConvert = false;
    int bufferSize;
    std::wstring wcontent;
    bufferSize = MultiByteToWideChar(CP_UTF8, 0, content, -1, 0, 0);
    if (bufferSize == 0) cantConvert = true;
    else {
        wcontent.resize(bufferSize);
        if (MultiByteToWideChar(CP_UTF8, 0, content, -1, wcontent.data(), wcontent.size()) == 0) cantConvert = true;
    }
    std::wstring wtitle;
    if (title) {
        bufferSize = MultiByteToWideChar(CP_UTF8, 0, title, -1, 0, 0);
        if (bufferSize == 0) cantConvert = true;
        else {
            wtitle.resize(bufferSize);
            if (MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle.data(), wtitle.size()) == 0) cantConvert = true;
        }
    }

    if (cantConvert) return MessageBoxA(window, content, title, type);
    else return MessageBoxW(window, wcontent.data(), title ? wtitle.data() : 0, type);
}

extern "C" __declspec(dllexport) int MessageBoxUtf8(void* window, const char* content, const char* title, unsigned int type) {
    return repl_MessageBoxUtf8((HWND)window, content, title, type);
}

void LoadHooks() {
    DWORD old;
    VirtualProtect((LPVOID)0x00401000, 0x00456000, PAGE_WRITECOPY, &old);
    orig_appendDataPackage = (appendDataPackage_t) TamperNearJmpOpr(0x007FB85F, reinterpret_cast<DWORD>(repl_appendDataPackage));

    // Replace system strings
    for (int i = 0; text_table[i].addr; ++i) {
        *(const char**)text_table[i].addr = text_table[i].string;
    }
    TamperStringWithSize(0x452613, 0x452611, "<key system.profile.failDelete>");
    TamperStringWithSize(0x4525d4, 0x4525d2, "<key system.profile.deleted>");
    TamperStringWithSize(0x452469, 0x452461, "<key system.profile.copied>");
    TamperStringWithSize(0x4523d7, 0x4523cf, "<key system.profile.failCopy>");
    TamperStringWithSize(0x4527e1, 0x4527df, "<key system.profile.failName>");
    TamperStringWithSize(0x452840, 0x452838, "<key system.profile.failName>");
    TamperStringWithSize(0x45277c, 0x45277a, "<key system.profile.nameChanged>");
    TamperStringWithSize(0x44a12a, 0x44a128, "<color 808080>? ? ? ? ? ? ? ? </color>");
    TamperStringWithSize(0x450bbd, 0x450bbb, "<color 808080>? ? ?</color>");

    // Replace Fonts
    AddFontResourceExW((modulePath / L"MonoSpatialModSWR.ttf").c_str(), FR_PRIVATE, 0);
    *(const char**)0x434cf6 = "MonoSpatialModSWR"; // profiles
    *(const char**)0x438533 = "MonoSpatialModSWR"; // deck and spells
    *(const char**)0x43d7ac = "MonoSpatialModSWR"; // gui messages
    *(const char**)0x44406f = "MonoSpatialModSWR"; // popup
    *(const char**)0x44b987 = "MonoSpatialModSWR"; // loaded but unused?
    *(const char**)0x450a83 = "MonoSpatialModSWR"; // numbers
    *(const char**)0x453bfa = "MonoSpatialModSWR"; // TODO verify
    *(const char**)0x462926 = "MonoSpatialModSWR"; // story
    // Patch font type
    *(uint32_t*)0x411c64 = DEFAULT_CHARSET;
    
    // escape codes on csv
    const uint8_t callTextPassThrough[] = {
        0x54,                   // PUSH ebp
        0x55,                   // PUSH esp
        0xe8, 0, 0, 0, 0,       // CALL xxxx
        0x8b, 0xe8,             // MOV ebp, eax
    }; TamperCode(0x40f5ff, callTextPassThrough);
    TamperNearJmpOpr(0x40f601, reinterpret_cast<DWORD>(text_passthrough));

    // escape codes on scripts
    const uint8_t callScriptPassThrough[] = {
        0x52,                   // PUSH edx
        0x55,                   // PUSH ebp
        0xe8, 0, 0, 0, 0,       // CALL xxxx
        0x90, 0x90, 0x90, 0x90, // NOPs
        0x90, 0x90, 0x90, 0x90, // NOPs
        0x90, 0x90, 0x90, 0x90, // NOPs
        0x84, 0xc0              // TEST al, al
    }; TamperCode(0x82112a, callScriptPassThrough);
    TamperNearJmpOpr(0x82112c, reinterpret_cast<DWORD>(script_passthrough));

    // Change CreateTextTexture to accept utf8
    //[esp+0x10]: len;
    //edi: offset;
    //ebx: buffer;
    //[ebp+0x0c]: out1;
    //[ebp+0x10]: out2;
    //esi: handle;
     TamperCode(0x4129b3, {
        0x8b, 0x44, 0x24, 0x10, // MOV eax, [esp+0x10]
        0x53,                   // PUSH ebx
        0xff, 0x75, 0x10,       // PUSH [ebp+0x10]  -- out2
        0xff, 0x75, 0x0c,       // PUSH [ebp+0x0c]  -- out1
        0x50,                   // PUSH eax         -- length
        0x57,                   // PUSH edi         -- index 
        0x53,                   // PUSH ebx         -- buffer
        0x56,                   // PUSH esi         -- handle
        0xe8, 0, 0, 0, 0,       // CALL xxxx
        0x83, 0xc4, 0x18,       // ADD esp, 0x18
        0x5b,                   // POP ebx
        0x01, 0xc7,             // ADD edi, eax
        0x85, 0xc0,             // TEST eax, eax
        0x74, 0x42,             // JE +0x42         -- goto break
        0xeb, 0x48,             // JMP +0x48        -- goto continue
        0x90,                   // NOP              -- align
    }); // rewrite character loop to use uft8 [0x4129b3]:[0x412a1b]
    TamperNearJmpOpr(0x004129c2, reinterpret_cast<DWORD>(printNextChar));

    // Use utf8 in profile names
    orig_copyInFilelist = (copyInFilelist_t)
        TamperNearJmpOpr(0x0043cd89, reinterpret_cast<DWORD>(repl_copyInFilelist));
    orig_createTextTexture = (createTextTexture_t)
        TamperNearJmpOpr(0x0044b143, reinterpret_cast<DWORD>(repl_createTextTexture));
    TamperNearJmpOpr(0x0042c7a1, reinterpret_cast<DWORD>(repl_copyInProfile));
    orig_copyInProfile = (copyInProfile_t)
        TamperNearJmpOpr(0x00434ea9, reinterpret_cast<DWORD>(repl_copyInProfile));
    TamperCode(0x434eae, {
        0x8B, 0x45, 0x14,       // MOV eax, [ebp+0x14]
        0x90, 0x90, 0x90,       // NOP NOP NOP
    }); // use length of target instead of source

    // Overwrite texture loader to add text
    TamperNearJmpOpr(0x0040b4b2, reinterpret_cast<DWORD>(repl_loadImage));
    orig_loadImage = (loadImage_t) TamperNearJmpOpr(0x0040505c, reinterpret_cast<DWORD>(repl_loadImage));

    // Make the Text Renderer to do alpha blending
    TamperCode(0x411f80, {
        0x50,                           // PUSH eax
        0x51,                           // PUSH ecx
        0x52,                           // PUSH edx             -- save state
        0x0f, 0xb6, 0x17,               // MOVZX edx, byte[edi] -- alpha
        0x8b, 0x09,                     // MOV ecx, [ecx]       -- color
        0x50,                           // PUSH eax             -- output
        0xe8, 0, 0, 0, 0,               // CALL xxxx
        0x5a,                           // POP edx
        0x59,                           // POP ecx
        0x58,                           // POP eax              -- restore state
        0x83, 0xc7, 0x01,               // ADD edi, 1           -- advance loop
        0x90, 0x90,                     // NOP                  -- align
    });
    TamperCode(0x411fd0, {
        0x50,                           // PUSH eax
        0x51,                           // PUSH ecx
        0x52,                           // PUSH edx             -- save state
        0x0f, 0xb6, 0x17,               // MOVZX edx, byte[edi] -- alpha
        0x8b, 0x8e, 0x68, 0x01, 0, 0,   // MOV ecx, [esi+0x168] -- color
        0x50,                           // PUSH eax             -- output
        0xe8, 0, 0, 0, 0,               // CALL xxxx
        0x5a,                           // POP edx
        0x59,                           // POP ecx
        0x58,                           // POP eax              -- restore state
        0x83, 0xc7, 0x01,               // ADD edi, 1           -- advance loop
        0x90, 0x90,                     // NOP                  -- align
    });
    TamperNearJmpOpr(0x411f89, reinterpret_cast<DWORD>(repl_alphaBlend));
    TamperNearJmpOpr(0x411fdd, reinterpret_cast<DWORD>(repl_alphaBlend));

    // Make the Text Shadow to do alpha blending
    TamperNearJmpOpr(0x412a3f, reinterpret_cast<DWORD>(_repl_textShadow));

    *(uint32_t*)0x450b3e = 0x00;    // deck numbers spacing
    *(uint8_t*)0x450bfd  = 0x0e;    // deck numbers slice size
    *(uint8_t*)0x450bff  = 0x12;    // deck numbers slice size
    *(uint32_t*)0x44e8e3 = 0x8c;    // not found card offset
    *(uint8_t*)0x4488a9  = 0x0a;    // ip numbers slice size
    *(uint32_t*)0x462966 = 1;       // story mode font spacing

    VirtualProtect((LPVOID)0x00401000, 0x00456000, old, &old);

    VirtualProtect((LPVOID)0x857000, 0x02b000, PAGE_WRITECOPY, &old);
    // Replace link to GetGlyphOutlineA with GetGlyphOutlineW
    *(uint32_t*)0x857014 = (uint32_t) GetProcAddress(GetModuleHandle(TEXT("gdi32.dll")), "GetGlyphOutlineW");
    // Replace link to MessageBoxA with custom function
    *(uint32_t*)0x857250 = (uint32_t) repl_MessageBoxUtf8;
    VirtualProtect((LPVOID)0x857000, 0x02b000, old, &old);

    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
}