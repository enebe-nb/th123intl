#include <windows.h>
#include <SokuLib.hpp>
#include <mbstring.h>
#include <usp10.h>

#include "main.hpp"
#include "th123intl.hpp"

namespace {
    SokuLib::CSVParser* systemStrings = 0;
    std::unordered_map<SokuLib::SWRFont*, int> fontObjects;
    std::unordered_map<int, const char*> fontNames;
    std::unordered_map<int, const char*> tilesNames;

    typedef int (__fastcall *parseHtmlTag_t)(void*, void*, const char*, int*);
    parseHtmlTag_t orig_parseHtmlTag = (parseHtmlTag_t) 0x412240;
    typedef void (__stdcall *parseIChar_t)(unsigned int, int*, int*);
    parseIChar_t orig_parseIChar = (parseIChar_t) 0x00411e20;
    constexpr int orig_applyTextShadow = 0x00412a60;

    typedef bool (__fastcall *csvParserInDeckInfo_t)(SokuLib::CSVParser&, void*, const char*);
    csvParserInDeckInfo_t orig_csvParserInDeckInfo = (csvParserInDeckInfo_t) 0x0040f370;

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
        { "netplay.waitHost", (const char**)0x446e3c },
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

static inline void LoadCustomPacks() {
    for (auto& pack : langConfig.packFiles) {
        // making it relative is the best solution, the game won't support path with nonASCII glyphs
        std::string systemPath; th123intl::ConvertCodePage(std::filesystem::relative(pack).wstring(), CP_ACP, systemPath);
        SokuLib::appendDatFile(systemPath.c_str());
    }
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

static void __declspec(naked) __onSokuSetup() {
    LoadCustomPacks();
    *(bool*)0x8a0048 = true; // hack
    LoadSystemStrings();
    __asm {
        lea eax, [ebx+8];
        mov edi, eax;
        ret
    }
}

static short repl_postProcessSoku2Packs(const char* filename) {
    LoadCustomPacks();
    return *reinterpret_cast<short*>(0x858da0);
}

static int findLineById(SokuLib::CSVParser& parser, int id) {
    for (int i = 0; i < parser.data.size(); ++i) {
        int lineId;
        auto& line = parser.data.at(i);
        if (!line.size()) continue;
        auto& idStr = line.at(0);
        if (std::from_chars(idStr, (char*)idStr + idStr.size, lineId).ec != std::errc()) continue;
        if (id == lineId) return i;
    }

    return -1;
}

static bool __fastcall repl_csvParserInDeckInfo(SokuLib::CSVParser& ecx, void* unused, const char* filename) {
    std::string_view name(filename);
    if (!orig_csvParserInDeckInfo(ecx, unused, filename)) return false;
    std::string intlfile("data/intl_"); intlfile += name.substr(9, name.size() - 9);
    std::replace(intlfile.begin()+10, intlfile.end(), '/', '_');
    SokuLib::CSVParser intlData(intlfile.c_str());
    if (!intlData.data.size()) return true;

#ifdef _DEBUG
    logging << "Replacing csv text in: " << name << std::endl;
#endif
    do {
        int i = intlData.getNextValue(); if(!i) continue;
        i = findLineById(ecx, i);
        if (i < 0) continue;
        intlData.getNextCell(ecx.data.at(i).at(1));
        intlData.getNextCell(ecx.data.at(i).at(4));
    } while (intlData.goToNextLine());

    return true;
}

static bool applyWordBreak(SokuLib::SWRFont* font, const char* word) {
    wchar_t wstr[512];
    const size_t index = _mbscspn_l((const uint8_t*)word+1, (const uint8_t*)" <", langConfig.locale)+1;
    const size_t len = _mbsnccnt_l((const uint8_t*)word, index, langConfig.locale);
    const size_t wlen = _mbstowcs_l(wstr, word, len, langConfig.locale);
    if (wlen) {
        SIZE textSize;
        if (GetTextExtentPoint32W(font->hdc, wstr, wlen, &textSize)) {
            int width = textSize.cx + font->cursor.x + font->description.charSpaceX * wlen;
            if (width > font->maxWidth) {
                font->cursor.x = font->description.offsetX + font->description.shadow;
                font->cursor.y += font->description.charSpaceY + font->description.height;
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

#ifdef SOKU_USE_UTF16
    wchar_t lc;
    const int len = _mbtowc_l(&lc, buffer+index, bufferSize-index, langConfig.locale);
#else 
    const int len = _mblen_l(buffer+index, bufferSize-index, langConfig.locale);
    unsigned int lc = len > 0 ? _mbsnextc_l((const uint8_t*)buffer+index, langConfig.locale) : 0;
#endif
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

static size_t* __fastcall repl_net_createTexture(SokuLib::TextureManager* manager, void* edx, size_t* texId, const char* text, SokuLib::SWRFont* font, int texWidth, int texHeight, int* outWidth, int* outHeight) {
    LPDIRECT3DTEXTURE9& texture = *manager->Allocate(*texId);
    texture = 0;

    const auto& dxCaps = *reinterpret_cast<D3DCAPS9*>(0x8a0e38);
    if (dxCaps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) {
        if (texWidth < texHeight) texWidth = texHeight;
        else texHeight = texWidth;
    }

    auto const renderLock = reinterpret_cast<SokuLib::CriticalSection*>(0x8a0e10);
    renderLock->lock();
    auto ret = reinterpret_cast<HRESULT (__stdcall*)(IDirect3DDevice9*, size_t, size_t, size_t, DWORD, D3DFORMAT, D3DPOOL, LPDIRECT3DTEXTURE9*)>
        (0x81f6ac)(*(IDirect3DDevice9**)0x8a0e30, texWidth, texHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture);
    renderLock->unlock();

    if (ret == D3D_OK) {
        renderLock->lock();
        LPDIRECT3DSURFACE9 surface = 0;
        texture->GetSurfaceLevel(0, &surface);
        surface->GetDC(&font->hdc);
        font->font = CreateFontA(font->description.height, 0, 0, 0,
            font->description.weight, font->description.italic, false, false, langConfig.charset,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_SWISS, font->description.faceName);
        font->gdiobj = SelectObject(font->hdc, font->font);

        font->cursor.x = font->description.offsetX + font->description.shadow;
        font->cursor.y = font->description.offsetY + font->description.shadow;
        font->maxWidth = texWidth;
        font->maxHeight = texHeight;
        font->gradient2;
        if (outWidth) *outWidth = 0;
        if (outHeight) *outHeight = 0;

        { std::wstring textBuffer; th123intl::ConvertCodePage(CP_UTF8, text, textBuffer);
        SCRIPT_STRING_ANALYSIS parsedString;
        const DWORD flags = SSA_FALLBACK|SSA_LINK|SSA_GCP|SSA_GLYPHS;
        RECT textRect = {font->cursor.x, font->cursor.y, font->maxWidth, font->maxHeight};
        FillRect(font->hdc, &textRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
        SetTextColor(font->hdc, 0x00ffffff);
        SetBkMode(font->hdc, TRANSPARENT);
        auto ret = ScriptStringAnalyse(font->hdc, textBuffer.data(), textBuffer.size(), 1.5f*textBuffer.size() + 16, -1, flags, font->maxWidth, 0, 0, 0, 0, 0, &parsedString);
        ret = ScriptStringOut(parsedString, font->cursor.x, font->cursor.y, ETO_CLIPPED, &textRect, 0, 0, false);
        ScriptStringFree(&parsedString); }

        DeleteObject(SelectObject(font->hdc, font->gdiobj));
        surface->ReleaseDC(font->hdc);
        surface->Release();
        font->gdiobj = nullptr;
        font->font = nullptr;
        font->hdc = nullptr;

        { D3DLOCKED_RECT texRect;
        texture->LockRect(0, &texRect, 0, 0);
        font->output1 = texRect.pBits;
        font->output2 = texRect.pBits;
        const size_t texSize = texRect.Pitch * font->maxHeight;
        font->lineWidth = texRect.Pitch / 4;
        char* shadowBuffer = 0;
        if (font->description.shadow) {
            memset(font->output1 = shadowBuffer = new char[texSize], 0, texSize);
        }

        for (int i = 0; i < font->maxWidth * font->maxHeight; ++i) {
            union Color { struct {BYTE b, g, r, a;}; UINT value; };
            Color mul; mul.value = font->gradient2 ? ((UINT*)font->gradient2)[((i/font->maxWidth-font->cursor.y)%font->description.height)] : font->singleColor;
            Color base; base.value = ((0xff&((int*)font->output2)[i]) << 24)|((int*)font->output2)[i];
            base.r = ((UINT)mul.r)*((UINT)base.r)/255;
            base.g = ((UINT)mul.g)*((UINT)base.g)/255;
            base.b = ((UINT)mul.b)*((UINT)base.b)/255;
            ((int*)font->output1)[i] = base.value;
        }

        if (shadowBuffer) {
            const auto arg2 = font->output2;
            __asm {
                push arg2;
                push shadowBuffer;
                mov eax, font;
                call orig_applyTextShadow;
            }; // fuck eax
            delete[] shadowBuffer;
        }
        texture->UnlockRect(0); }
        renderLock->unlock();
    } else {
        manager->Deallocate(*texId);
        *texId = 0;
    }

    return texId;
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

template <int ADDR> static void __fastcall setTiles2(SokuLib::CNumber* tile, int unused, int texture, int texOffsetX, int texOffsetY, int width, int height, int count, float textSpacing, int fontSpacing, int size, int floatSize) {
    auto iter = langConfig.tileOverrides.find(ADDR);
    if (iter != langConfig.tileOverrides.end()) {
        for(auto& override : iter->second) {
            switch (override.param) {
                case 0: texOffsetX = override.data; break;
                case 1: texOffsetY = override.data; break;
                case 2: width = override.data; break;
                case 3: height = override.data; break;
                case 4: textSpacing = override.data; break;
                default: break;
            }
        }
    }
#ifdef _DEBUG
    logging << "Slicing tile "<<tilesNames[ADDR]<<" in "<<(void*)texture<<" ("<<texOffsetX<<", "<<texOffsetY<<", "<<width<<", "<<height<<
        ", "<<count<<", "<<textSpacing<<", "<<fontSpacing<<", "<<size<<", "<<floatSize<<")" << std::endl;
#endif
    return (tile->*SokuLib::union_cast<void(SokuLib::CNumber::*)(int, int, int, int, int, int, float, int, int, int)>(0x414870))
        (texture, texOffsetX, texOffsetY, width, height, count, textSpacing, fontSpacing, size, floatSize);
}

template <int ADDR> static inline void addFont(const char* name) {
    SokuLib::TamperNearJmpOpr(ADDR, setFont<ADDR>);
    fontNames[ADDR] = name;
}

template <int ADDR> static inline void addTiles1(const char* name) {
    SokuLib::TamperNearJmpOpr(ADDR, setTiles1<ADDR>);
    tilesNames[ADDR] = name;
}

template <int ADDR> static inline void addTiles2(const char* name) {
    SokuLib::TamperNearJmpOpr(ADDR, setTiles2<ADDR>);
    tilesNames[ADDR] = name;
}

void LoadHooks() {
    DWORD old;
    VirtualProtect((LPVOID)0x00401000, 0x00456000, PAGE_WRITECOPY, &old);

    // if running nextsoku
    if (*(short*)0x7fb84b == (short)0xd0ff) {
        SokuLib::TamperNearCall(0x004417e3, repl_postProcessSoku2Packs);
        *((char*)0x004417e8) = 0x90; // NOP
    }
    SokuLib::TamperNearCall(0x0040797f, __onSokuSetup);

    // hook FontSetIndirect
    addFont<0x4126ad>("unknown01");
    addFont<0x434de1>("profile");
    addFont<0x438611>("deck");
    addFont<0x43d883>("gui");
    addFont<0x444142>("popup");
    addFont<0x44ba52>("replayPath");
    addFont<0x450b52>("number");
    addFont<0x453cd1>("netProfile");
    addFont<0x453dbd>("netProfile2");
    addFont<0x45c5ac>("unknown05");
    addFont<0x45c6f6>("unknown06");
    addFont<0x45f110>("unknown07");
    addFont<0x46202a>("unknown08");
    addFont<0x462990>("story");

    addTiles1<0x450c0f>("deck");
    addTiles2<0x4488b3>("ipport");

    // Patch font charset
    *(uint32_t*)0x411c64 = langConfig.charset;
    // Patch Spellcard text width
    // *(uint32_t*)0x4379e3 = 640;
    // *(uint32_t*)0x4381be = 640;

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
#ifdef SOKU_USE_NETUTF8
    SokuLib::TamperNearJmpOpr(0x453d01, repl_net_createTexture);
    SokuLib::TamperNearJmpOpr(0x453df1, repl_net_createTexture);
#endif

    // Convert ACP to langConfig.locale in replay and profile names
    //orig_copyInFilelist = SokuLib::TamperNearJmpOpr(0x0043cd89, repl_copyInFilelist);

    // Convert string inside Csv Files without changing numbers
    // 0x433029: story?
    // 0x4374f9: ingame spellcard/storyspell
    // 0x437de5: info spellcard/storyspell
    orig_csvParserInDeckInfo = SokuLib::TamperNearJmpOpr(0x437de5, repl_csvParserInDeckInfo);
    orig_csvParserInDeckInfo = SokuLib::TamperNearJmpOpr(0x4374f9, repl_csvParserInDeckInfo);

/*
    *(uint32_t*)0x450b3e = 0x00;    // deck numbers spacing
    *(uint8_t*)0x450bfd  = 0x0e;    // deck numbers slice size
    *(uint8_t*)0x450bff  = 0x12;    // deck numbers slice size
    *(uint32_t*)0x44e8e3 = 0x8c;    // not found card offset
    *(uint8_t*)0x4488a9  = 0x0a;    // ip numbers slice size
    *(uint32_t*)0x462966 = 1;       // story mode font spacing
*/
    VirtualProtect((LPVOID)0x00401000, 0x00456000, old, &old);

    VirtualProtect((LPVOID)0x857000, 0x02b000, PAGE_WRITECOPY, &old);
#ifdef SOKU_USE_UTF16
    // Replace link to GetGlyphOutlineA with GetGlyphOutlineW
    *(uint32_t*)0x857014 = (uint32_t) GetProcAddress(GetModuleHandle(TEXT("gdi32.dll")), "GetGlyphOutlineW");
#endif
    // Convert ACP to langConfig.locale in profile names
    // Replace link to MessageBoxA with custom function TODO convert to WCHAR?
    //*(uint32_t*)0x857250 = (uint32_t) repl_MessageBoxUtf8;
    VirtualProtect((LPVOID)0x857000, 0x02b000, old, &old);

    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
}