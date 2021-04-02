#include <windows.h>

#include "main.hpp"
#ifdef _DEBUG
#include <fstream>
#endif

#ifndef TH123INTL_DISABLE_UTFERROR
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

    static struct {
        uint32_t addr;
        const char *string;
    } text_table[] = {
        // DINPUT
        { 0x40d66d, "<key system.dinput.failCreate0>" },
        { 0x40d6a5, "<key system.dinput.failCreate1>" },
        { 0x40d867, "<key system.dinput.failKeyboard>" },
        { 0x40d898, "<key system.dinput.failDataFormat>" },
        { 0x40d8ce, "<key system.dinput.failCoopLevel>" },
        { 0x40d930, "<key system.dinput.failQueue>" },
        { 0x40db1a, "<key system.dinput.failMouse>" },
        { 0x40db4e, "<key system.dinput.failDataFormat>" },
        { 0x40dbd8, "<key system.dinput.failProps>" },

        // INTERNETS
        { 0x41312c, "<key system.socket.failInit>" },
        { 0x413331, "<key system.socket.failPort>" },

        // DIRECT3D
        { 0x414f05, "<key system.d3d.failObject>" },
        { 0x414f35, "<key system.d3d.failDisplay>" },
        { 0x415079, "<key system.d3d.failDevice>" },
        { 0x40db87, "<key system.d3d.failCoopLevel>" },
        { 0x4188e1, "<key system.d3d.failCoopLevel>" },

        // DSOUND
        { 0x418392, "<key system.dsound.failBuffer>" },
        { 0x4183d1, "<key system.dsound.failBuffer3D>" },
        { 0x41851e, "<key system.dsound.failBuffer>" },
        { 0x4185ca, "<key system.dsound.failBufferLock>" },
        { 0x4186f2, "<key system.dsound.failBufferLock>" },
        { 0x41884c, "<key system.dsound.failCreateObject>" },
        { 0x41888b, "<key system.dsound.failInit>" },
        { 0x4189ab, "<key system.dsound.failCreatePrimary>" },
        { 0x4189f7, "<key system.dsound.failCreateListener>" },
        { 0x418af1, "<key system.dsound.failCreateTemporary>" },

        // ERROR
        { 0x7fb88d, "Failed to initialize" },

        // Replay
        { 0x442eac, "<key system.replay.failCreateFolder>" },
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
        { 0x442e81, "<key system.profile.failFolder>" },
        { 0x452613, "<key system.profile.failDelete>" },
        { 0x4525d4, "<key system.profile.deleted>" },
        { 0x450256, "<key system.profile.confirmSave>" },
        { 0x4503c9, "<key system.profile.confirmSave>" },
        { 0x450188, "<key system.profile.fail20Cards>" },
        { 0x452090, "<key system.profile.failCreate>" },
        { 0x4520f8, "<key system.profile.failCreate>" },
        { 0x452009, "<key system.profile.created>" },
        { 0x452469, "<key system.profile.copied>" },
        { 0x4523d7, "<key system.profile.failCopy>" },
        { 0x4527e1, "<key system.profile.failName>" },
        { 0x452840, "<key system.profile.failName>" },
        { 0x45277c, "<key system.profile.nameChanged>" },
        { 0x452972, "<key system.profile.confirmOverwrite>" },
        { 0x452f6c, "<key system.profile.confirmOverwrite>" },
        { 0x452ae1, "<key system.profile.confirmCopy>" },
        { 0x452c71, "<key system.profile.confirmDelete>" },

        // stupid spellcard clip fix
        //{ 0x43780e, "<br><br>" },
        //{ 0x437ff4, "<br><br>" },

        { 0x448872, "0 1 2 3 4 5 6 7 8 9<br>^<br>     .      .      .      : " },
        { 0x44a12a, "<color 808080>? ? ? ? ? ? ? ? </color>" },
        { 0x44c90b, "%s - %s  %03d/%03d  Time %02d.%02d" },
        { 0x44c947, "<color 808080>? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? </color>" },
        { 0x450bbd, "<color 808080>? ? ?</color>" },
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
inline void TamperCode(void* address, const uint8_t (&code)[S]) {
    memcpy(address, code, S);
}

static void repl_appendDataPackage(const char* filename) {
    orig_appendDataPackage(filename);
    orig_appendDataPackage((modulePath / L"strkeys.dat").string().c_str());
    if (!languagePack.empty()) orig_appendDataPackage(languagePack.string().c_str());
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
        size_t len = strlen(value);
        for (size_t i = 0, j = -1; i < len && j != 0; i += j) {
            j = printNextChar(handle, value, i, len, out1, out2);
        }
        return end - text + 1;
    }

    return orig_parseHtmlTag(out2, handle, text, out1);
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

void LoadHooks() {
    DWORD old;
    VirtualProtect((LPVOID)0x00401000, 0x00456000, PAGE_WRITECOPY, &old);
    orig_appendDataPackage = (appendDataPackage_t) TamperNearJmpOpr(0x007FB85F, reinterpret_cast<DWORD>(repl_appendDataPackage));

    // Replace system strings
    for (int i = 0; text_table[i].addr; ++i) {
        *(const char**)text_table[i].addr = text_table[i].string;
    }

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
    }; TamperCode((void*)0x40f5ff, callTextPassThrough);
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
    }; TamperCode((void*)0x82112a, callScriptPassThrough);
    TamperNearJmpOpr(0x82112c, reinterpret_cast<DWORD>(script_passthrough));

    // rewrite character loop to use uft8 [0x4129b3]:[0x412a1b]
    //[esp+0x10]: len;
    //edi: offset;
    //ebx: buffer;
    //[ebp+0x0c]: out1;
    //[ebp+0x10]: out2;
    //esi: handle;
    const uint8_t callPrintNextChar[] = {
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
    }; TamperCode((void*)0x004129b3, callPrintNextChar);
    TamperNearJmpOpr(0x004129c2, reinterpret_cast<DWORD>(printNextChar));
    
    *(uint32_t*)0x450b3e = 0x00;    // deck numbers spacing
    *(uint8_t*)0x450bfd  = 0x0e;    // deck numbers slice size
    *(uint8_t*)0x450bff  = 0x12;    // deck numbers slice size
    *(uint32_t*)0x44e8e3 = 0x8c;    // not found card offset
    *(uint8_t*)0x4488a9  = 0x0a;    // ip numbers slice size
    *(uint32_t*)0x462966 = 1;       // story mode font spacing

    VirtualProtect((LPVOID)0x00401000, 0x00456000, old, &old);

    // Replace link to GetGlyphOutlineA with GetGlyphOutlineW
    VirtualProtect((LPVOID)0x857014, 4, PAGE_WRITECOPY, &old);
    *(uint32_t*)0x857014 = (uint32_t) GetProcAddress(GetModuleHandle(TEXT("gdi32.dll")), "GetGlyphOutlineW");
    VirtualProtect((LPVOID)0x857014, 4, old, &old);

    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
}