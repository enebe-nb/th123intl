#include <windows.h>
#include <SokuLib.hpp>
#include <mbstring.h>

#include "main.hpp"

namespace {
    SokuLib::CSVParser* systemStrings = 0;
    std::unordered_map<SokuLib::SWRFont*, int> fontObjects;
    std::unordered_map<int, const char*> fontNames;
    std::unordered_map<int, const char*> tilesNames;

    typedef void (*appendDataPackage_t)(const char*);
    appendDataPackage_t orig_appendDataPackage;
    typedef int (__fastcall *parseHtmlTag_t)(void*, void*, const char*, int*);
    parseHtmlTag_t orig_parseHtmlTag = (parseHtmlTag_t) 0x412240;
    typedef void (__stdcall *parseIChar_t)(unsigned int, int*, int*);
    parseIChar_t orig_parseIChar = (parseIChar_t) 0x00411e20;

    typedef void (__fastcall *copyInFilelist_t)(void*, void*, SokuLib::String&);
    copyInFilelist_t orig_copyInFilelist = (copyInFilelist_t) 0x0043caa0;
    typedef void (__fastcall *copyInProfile_t)(SokuLib::String&, void*, SokuLib::String&, unsigned int, int);
    copyInProfile_t orig_copyInProfile = (copyInProfile_t) 0x004020d0;
    typedef int (__fastcall *createTextTexture_t)(void*, void*, void*, const char*, void*, int, int, int*, int*);
    createTextTexture_t orig_createTextTexture = (createTextTexture_t) 0x004050a0;

    typedef int (__fastcall *loadImage_t)(int*, char*, void**, int*);
    loadImage_t orig_loadImage = (loadImage_t) 0x00408cf0;

    const std::unordered_multimap<std::string_view, const char**> strMapSimple = {
        { "dinput.failCreate0", (const char**)0x40d66d },
        { "dinput.failCreate1", (const char**)0x40d6a5 },
        { "dinput.failKeyboard", (const char**)0x40d867 },
        { "dinput.failDataFormat", (const char**)0x40d898 },
        { "dinput.failQueue", (const char**)0x40d930 },
        { "dinput.failMouse", (const char**)0x40db1a },
        { "dinput.failDataFormat", (const char**)0x40db4e },
        { "dinput.failProps", (const char**)0x40dbd8 },

        // SOCKET
        { "socket.failInit", (const char**)0x41312c },
        { "socket.failPort", (const char**)0x413331 },

        // DIRECT3D
        { "d3d.failCoopLevel", (const char**)0x40d8ce },
        { "d3d.failCoopLevel", (const char**)0x40db87 },
        { "d3d.failObject", (const char**)0x414f05 },
        { "d3d.failDisplay", (const char**)0x414f35 },
        { "d3d.failDevice", (const char**)0x415079 },
        { "d3d.failCoopLevel", (const char**)0x4188e1 },

        // DSOUND
        { "dsound.failBuffer", (const char**)0x418392 },
        { "dsound.failBuffer3D", (const char**)0x4183d1 },
        { "dsound.failBuffer", (const char**)0x41851e },
        { "dsound.failBufferLock", (const char**)0x4185ca },
        { "dsound.failBufferLock", (const char**)0x4186f2 },
        { "dsound.failCreateObject", (const char**)0x41884c },
        { "dsound.failInit", (const char**)0x41888b },
        { "dsound.failCreatePrimary", (const char**)0x4189ab },
        { "dsound.failCreateListener", (const char**)0x4189f7 },
        { "dsound.failCreateTemporary", (const char**)0x418af1 },

        // ERROR
        { "failInit", (const char**)0x7fb88d },

        // Replay
        { "replay.failFolder", (const char**)0x442eac },
        { "replay.returnSelect", (const char**)0x444d50 },
        { "replay.returnTitle", (const char**)0x444d6f },
        { "replay.saved", (const char**)0x444e75 },
        { "replay.save", (const char**)0x449298 },
        { "replay.saved", (const char**)0x4492e4 },
        { "replay.save", (const char**)0x449682 },
        { "replay.saved", (const char**)0x4496b0 },
        { "replay.deleted", (const char**)0x44b57e },
        { "replay.failDelete", (const char**)0x44b598 },
        { "replay.delete", (const char**)0x44b654 },

        // Netplay
        { "netplay.selectPort", (const char**)0x446842 },
        { "netplay.selectAddress", (const char**)0x4468e2 },
        { "netplay.waithost", (const char**)0x446e3c },
        { "netplay.allowSpec", (const char**)0x446e8f },
        { "netplay.confirmMatch", (const char**)0x448ab0 },
        { "netplay.waitClient", (const char**)0x448afa },
        { "netplay.failPlayingSpec", (const char**)0x448b5a },
        { "netplay.failPlaying", (const char**)0x448b6a },
        { "netplay.failConn", (const char**)0x448b73 },
        { "netplay.failStart", (const char**)0x448c76 },
        { "netplay.failSpec", (const char**)0x448c92 },
        { "netplay.failConn", (const char**)0x448c5a },
        { "netplay.failConn", (const char**)0x448cae },
        { "netplay.selectSaved", (const char**)0x448f37 },
        { "netplay.selectPrevious", (const char**)0x448f5b },
        { "netplay.confirmAddress", (const char**)0x44902c },

        // Replay/continue/etc
        { "story.continue", (const char**)0x449319 },

        // Profile stuff
        { "profile.failFolder", (const char**)0x442e81 },
        { "profile.fail20Cards", (const char**)0x450188 },
        { "profile.confirmSave", (const char**)0x450256 },
        { "profile.confirmSave", (const char**)0x4503c9 },
        { "profile.created", (const char**)0x452009 },
        { "profile.failCreate", (const char**)0x452090 },
        { "profile.failCreate", (const char**)0x4520f8 },
        { "profile.confirmOverwrite", (const char**)0x452972 },
        { "profile.confirmCopy", (const char**)0x452ae1 },
        { "profile.confirmDelete", (const char**)0x452c71 },
        { "profile.confirmOverwrite", (const char**)0x452f6c },

        // stupid spellcard clip fix
        //{ 0x43780e, "<br><br>" },
        //{ 0x437ff4, "<br><br>" },

        { "pattern.0", (const char**)0x448872 },
        { "pattern.1", (const char**)0x44c90b },
        { "pattern.2", (const char**)0x44c947 },
        { "pattern.3", (const char**)0x450b7d },
    };

    const std::unordered_multimap<std::string_view, std::pair<const char**, int*> > strMapSized = {
        {"profile.failCopy", {(const char**)0x4523d7, (int*)0x4523cf}},
        {"profile.copied", {(const char**)0x452469, (int*)0x452461}},
        {"profile.deleted", {(const char**)0x4525d4, (int*)0x4525d2}},
        {"profile.failDelete", {(const char**)0x452613, (int*)0x452611}},
        {"profile.nameChanged", {(const char**)0x45277c, (int*)0x45277a}},
        {"profile.failName", {(const char**)0x4527e1, (int*)0x4527df}},
        {"profile.failName", {(const char**)0x452840, (int*)0x452838}},

        {"pattern.4", {(const char**)0x44a12a, (int*)0x44a128}},
        {"pattern.5", {(const char**)0x450bbd, (int*)0x450bbb}},
    };
}

template <std::size_t S>
static inline void TamperCode(uint32_t address, const uint8_t (&code)[S]) {
    memcpy((void*)address, code, S);
}

static inline void LoadSystemStrings() {
    if (systemStrings == 0) systemStrings = new SokuLib::CSVParser("data/csv/system.cv1");
    DWORD old;
    VirtualProtect((LPVOID)0x00401000, 0x00456000, PAGE_WRITECOPY, &old);

    for(int i = 0; i < systemStrings->data.size(); ++i) {
        auto& pair = systemStrings->data.at(i);
        if (pair.size() < 2) continue;

        { auto iter = strMapSimple.equal_range(std::string_view(pair[0], pair[0].size));
        if (iter.first != strMapSimple.end()) for(auto addr = iter.first; addr != iter.second; ++addr) {
            *addr->second = (const char*)pair[1];
        } }

        { auto iter = strMapSized.equal_range(std::string_view(pair[0], pair[0].size));
        if (iter.first != strMapSized.end()) for(auto addr = iter.first; addr != iter.second; ++addr) {
            *addr->second.first = (const char*)pair[1];
            *(unsigned char*)addr->second.second = pair[1].size;
        } }
    }

    VirtualProtect((LPVOID)0x00401000, 0x00456000, old, &old);
}

static void repl_appendDataPackage(const char* filename) {
    orig_appendDataPackage(filename);
    if (filename == (const char*)0x861b18) {
        for (auto& pack : langConfig.packFiles) orig_appendDataPackage(pack.string().c_str());
        *(bool*)0x8a0048 = true; // hack
        LoadSystemStrings();
    }
}

static bool applyWordBreak(SokuLib::SWRFont* font, const char* word) {
    wchar_t wstr[512];
    const size_t index = _mbscspn_l((const uint8_t*)word+1, (const uint8_t*)" <", langConfig.locale)+1;
    const size_t len = _mbsnccnt_l((const uint8_t*)word, index, langConfig.locale);
    const size_t wlen = _mbstowcs_l(wstr, word, len, langConfig.locale);
    if (wlen) {
        SIZE textSize;
        if (GetTextExtentPoint32W(font->hdc, wstr, wlen, &textSize)) {
            int width = textSize.cx + font->cursorX + font->description.charSpaceX * wlen;
            if (width > font->maxWidth) {
                font->cursorX = font->description.offsetX + font->description.shadow;
                font->cursorY += font->description.charSpaceY + font->description.height;
                return true;
            }
        }
    }
    return false;
}

static int printNextChar(SokuLib::SWRFont* font, const char* buffer, int index, int bufferSize, int* out1, int* out2) {
#ifdef _DEBUG
    static const char* lastBuffer = 0;
    if (lastBuffer != buffer) {
        auto iter = fontNames.find(fontObjects[font]);
        const char* name = iter == fontNames.end() ? "not found" : iter->second;
        logging << "Rendering text from " << name << ": " << buffer << std::endl;
        lastBuffer = buffer;
    }
#endif

    wchar_t lc;
    const int len = _mbtowc_l(&lc, buffer+index, bufferSize-index, langConfig.locale);
#ifdef _DEBUG
    if (len == -1) logging << "invalid character in index: " << index << std::endl;
#endif
    if (len <= 0) return 0;

    if (lc == L'<') return 1 + orig_parseHtmlTag(out2, font, buffer+index+len, out1);
    if (lc == L' ' && font->description.useOffset)
        if(applyWordBreak(font, &buffer[index])) return 1;

    __asm {
        push out2;
        push out1;
        movzx eax, lc;
        push eax;
        mov eax, font;
        call orig_parseIChar;
    }; // fuck eax
    return len;
}

/*
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
*/

template <std::size_t S>
inline int ACP2Locale(char (&dst)[S], const char* src, unsigned int len) {
    wchar_t wstr[S];
    int wlen = MultiByteToWideChar(GetACP(), 0, src, len, wstr, S);
    if (wlen) {
        int llen = _wcstombs_l(dst, wstr, wlen, langConfig.locale);
        dst[llen] = '\0';
#ifdef _DEBUG
        logging << "str convert: \"" << src << "\" -> \"" << dst << "\"" <<std::endl;
#endif
        return llen;
    }
    return 0;
}

static void __fastcall repl_copyInFilelist(void* ecx, void* edx, SokuLib::String& str) {
    const unsigned int targetCP = ((__crt_locale_data_public*)(langConfig.locale)->locinfo)->_locale_lc_codepage;
    if (GetACP() == targetCP) return orig_copyInFilelist(ecx, edx, str);
    const char* cstr = str;

    char buffer[512];
    int len = ACP2Locale(buffer, cstr, str.size);
    if (len) str.assign(buffer, len);

    return orig_copyInFilelist(ecx, edx, str);
}

static void __fastcall repl_copyInProfile(SokuLib::String& dst, void* edx, SokuLib::String& src, unsigned int offset, int len) {
    const unsigned int targetCP = ((__crt_locale_data_public*)(langConfig.locale)->locinfo)->_locale_lc_codepage;
    if (GetACP() == targetCP) return orig_copyInProfile(dst, edx, src, offset, len);
    const char* cstr = src;
    cstr += offset;
    if (len == -1) len = *(int*)(src + 0x14) - offset;

    char buffer[512];
    len = ACP2Locale(buffer, cstr, len);
    if (len) dst.assign(buffer, len);
    else orig_copyInProfile(dst, edx, src, offset, len);
}

static int __fastcall repl_createTextTexture(void* ecx, void* edx, void* dxHandle, const char* text, void* fontHandle, int texWidth, int texHeight, int* outWidth, int* outHeight) {
    const unsigned int targetCP = ((__crt_locale_data_public*)(langConfig.locale)->locinfo)->_locale_lc_codepage;
    if (GetACP() == targetCP) return orig_createTextTexture(ecx, edx, dxHandle, text, fontHandle, texWidth, texHeight, outWidth, outHeight);
    char buffer[512];
    int len = strlen(text);

    len = ACP2Locale(buffer, text, len);
    if (len) text = buffer;
    return orig_createTextTexture(ecx, edx, dxHandle, text, fontHandle, texWidth, texHeight, outWidth, outHeight);
}

template <int ADDR> static void __fastcall setFont(SokuLib::SWRFont* font, int unused, SokuLib::FontDescription* desc) {
    fontObjects[font] = ADDR;
    auto iter = langConfig.fontOverrides.find(ADDR);
    if (iter != langConfig.fontOverrides.end()) {
        for(auto& override : iter->second) {
            memcpy((char*)desc + override.offset, override.data, override.size);
        }
    }

#ifdef _DEBUG
    logging << "Init font: " << font << " in " << fontNames[ADDR] << " with " << (char*) desc << std::endl;
#endif
    return reinterpret_cast<void (__fastcall *)(SokuLib::SWRFont*, int, SokuLib::FontDescription*)>(0x411840)(font, unused, desc);
}

template <int ADDR> static void __fastcall setTiles1(SokuLib::CTile* tile, int unused, int texture, int texOffsetX, int texOffsetY, int width, int height) {
    auto iter = langConfig.tileOverrides.find(ADDR);
    if (iter != langConfig.tileOverrides.end()) {
        for(auto& override : iter->second) {
            switch (override.param) {
                case 0: texOffsetX = override.data; break;
                case 1: texOffsetY = override.data; break;
                case 2: width = override.data; break;
                case 3: height = override.data; break;
                default: break;
            }
        }
    }
#ifdef _DEBUG
    logging << "Slicing tile "<<tilesNames[ADDR]<<" in "<<(void*)texture<<" ("<<texOffsetX<<", "<<texOffsetY<<", "<<width<<", "<<height<<")" << std::endl;
#endif
    return tile->createSlices(texture, texOffsetX, texOffsetY, width, height);
}

template <int ADDR> static inline void addFont(const char* name) {
    SokuLib::TamperNearJmpOpr(ADDR, setFont<ADDR>);
    fontNames[ADDR] = name;
}

template <int ADDR> static inline void addTiles(const char* name) {
    SokuLib::TamperNearJmpOpr(ADDR, setTiles1<ADDR>);
    tilesNames[ADDR] = name;
}

void LoadHooks() {
    DWORD old;
    VirtualProtect((LPVOID)0x00401000, 0x00456000, PAGE_WRITECOPY, &old);
    orig_appendDataPackage = SokuLib::TamperNearJmpOpr(0x007FB85F, repl_appendDataPackage);

    // hook FontSetIndirect
    addFont<0x4126ad>("unknown01");
    addFont<0x434de1>("profile");
    addFont<0x438611>("deck");
    addFont<0x43d883>("gui");
    addFont<0x444142>("popup");
    addFont<0x44ba52>("unknown02");
    addFont<0x450b52>("number");
    addFont<0x453cd1>("unknown03");
    addFont<0x453dbd>("unknown04");
    addFont<0x45c5ac>("unknown05");
    addFont<0x45c6f6>("unknown06");
    addFont<0x45f110>("unknown07");
    addFont<0x46202a>("unknown08");
    addFont<0x462990>("story");

    addTiles<0x450c0f>("deck");

    // Patch font charset
    *(uint32_t*)0x411c64 = langConfig.charset;

    // Change CreateTextTexture to accept langConfig.locale
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
    }); // rewrite character loop to use langConfig.locale [0x4129b3]:[0x412a1b]
    SokuLib::TamperNearJmpOpr(0x004129c2, printNextChar);

    // Convert ACP to langConfig.locale in profile names
    orig_copyInFilelist = SokuLib::TamperNearJmpOpr(0x0043cd89, repl_copyInFilelist);
    orig_createTextTexture = SokuLib::TamperNearJmpOpr(0x0044b143, repl_createTextTexture);
    SokuLib::TamperNearJmpOpr(0x0042c7a1, repl_copyInProfile);
    orig_copyInProfile = SokuLib::TamperNearJmpOpr(0x00434ea9, repl_copyInProfile);
    TamperCode(0x434eae, {
        0x8B, 0x45, 0x14,       // MOV eax, [ebp+0x14]
        0x90, 0x90, 0x90,       // NOP NOP NOP
    }); // use length of target instead of source

/*
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
*/
    VirtualProtect((LPVOID)0x00401000, 0x00456000, old, &old);

    VirtualProtect((LPVOID)0x857000, 0x02b000, PAGE_WRITECOPY, &old);
    // Replace link to GetGlyphOutlineA with GetGlyphOutlineW
    *(uint32_t*)0x857014 = (uint32_t) GetProcAddress(GetModuleHandle(TEXT("gdi32.dll")), "GetGlyphOutlineW");
    // Replace link to MessageBoxA with custom function TODO convert to WCHAR?
    //*(uint32_t*)0x857250 = (uint32_t) repl_MessageBoxUtf8;
    VirtualProtect((LPVOID)0x857000, 0x02b000, old, &old);

    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
}