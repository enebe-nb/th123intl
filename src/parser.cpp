#include <SokuLib.hpp>
#include <mbstring.h>
#include "main.hpp"
#include <map>

namespace {
    typedef std::pair<SokuLib::String*, char*> StringIterator;

    typedef void (__fastcall *appendChar_t)(SokuLib::String&, int unused, StringIterator&, SokuLib::String&, char*, char);
    appendChar_t orig_appendChar = (appendChar_t) 0x0040fc00;
    typedef void (__fastcall *orig_text_func_t)(unsigned int ecx, void* edx, void *a, void *b, void *c, unsigned int d);
    orig_text_func_t orig_text_func = (orig_text_func_t) 0x40fc00;

    static const std::map<std::string, const char*> faceRemap = {
        {"confident", "\x97\x5d"},
        {"happy", "\x8A\xF0"},
        {"angry", "\x93\x7B"},
        {"confused", "\x98\x66"},
        {"normal", "\x95\x81"},
        {"sweatdrop", "\x8A\xBE"},
        {"determined", "\x8C\x88"},
        {"defeated", "\x95\x89"},
        {"surprised", "\x8B\xC1"},
    };
}

template <std::size_t S>
static inline void TamperCode(uint32_t address, const uint8_t (&code)[S]) {
    memcpy((void*)address, code, S);
}

// --- CSV args ---
// al = curCharValue
// ebx = 0;
// edx = bufferCapacity;
// ebp = curCharPtr
// edi = CSVParser
// [esp+58] = bufferString
// [esp+38] = iterator
// *note: calls pushes*

static void __fastcall csvAppendChar(StringIterator& iter, SokuLib::String& str, char c) {
    orig_appendChar(str, 0, iter, str, ((char*)str)+str.size, c);
}

static int __fastcall csvFixDefault(StringIterator& iter, SokuLib::String& out, const unsigned char* in) {
    const int len = _mbclen_l(in, langConfig.locale);
#ifdef _DEBUG
    if (len == -1) logging << "invalid character in csv parser." << std::endl;
#endif
    if (len <= 0) return 1;
    unsigned int c = _mbsnextc_l(in, langConfig.locale);

    if (c == '\\') {
        unsigned int next = _mbsnextc_l(_mbsinc_l(in, langConfig.locale), langConfig.locale);
        if (next == '\'' || next == '\"' || next == '\\' || next == ',') {
            csvAppendChar(iter, out, next);
            return 2;
        }
    }

    int i = len; while(i--) {
        csvAppendChar(iter, out, ((char*)&c)[i]);
    } return len;
}

static void __declspec(naked) csvFixQuotes() {
    __asm {
        cmp ds:[edi+0x2c], bl;  // test isInComment
        jnz __flipQuoteValue;

        mov al, ds:[ebp+1];
        cmp al, 0x22;           // test nextChar == '"'
        jne __flipQuoteValue;

        lea ecx, ds:[esp+0x3c];
        lea edx, ds:[esp+0x5c];
        push eax;
        call csvAppendChar;     // csvAppendChar('"')
        add ebp, 1;
        jmp __end;

    __flipQuoteValue:
        cmp ds:[edi+0x2d], bl;
        setz al;
        mov ds:[edi+0x2d], al;  // isInQuote = !isInQuote
    __end:
        ret;
    }
}

static void __declspec(naked) csvFixComma() {
    __asm {
        cmp ds:[edi+0x2c], bl;  // test isInComment
        jnz __ignore;

        cmp ds:[edi+0x2d], bl;  // test isInQuotes
        jz __pushCell;

        lea ecx, ds:[esp+0x3c];
        lea edx, ds:[esp+0x5c];
        push eax;
        call csvAppendChar;     // csvAppendChar(',')
        jmp __ignore;

    __ignore:
        pop eax;
        push 0x0040f608
    __pushCell:
        ret;
    }
}

static const unsigned char* repl_strchr(const unsigned char* str, unsigned int c) {
    return _mbschr_l(str, c, langConfig.locale);
}

template<class Command>
static int* __fastcall _factoryCreate(int* f) {
    static_assert(std::is_base_of<SokuLib::CommandParser::CommandBase, Command>::value);
    int* c = (int*)new Command();
    c[1] = f[1];
    return c;
}

template<class Command>
static inline void __fastcall registerCommand(SokuLib::CommandParser* parser, int unused, int id, const SokuLib::String name) {
    static const int vtable_ref[] = {0x456300, (int)&_factoryCreate<Command>};

    if ((name.*SokuLib::union_cast<bool(SokuLib::String::*)(int, int, int, int) const>(0x405c70))(0, name.size, 0x871030, 0)) {
        int* c = (int*)SokuLib::NewFct(8);
        c[0] = (int)vtable_ref;
        c[1] = id;
        parser->factoryMap[name] = c;
    } else {
        if (parser->factoryEmpty) SokuLib::DeleteFct(parser->factoryEmpty);
        int* c = (int*)SokuLib::NewFct(8);
        c[0] = (int)vtable_ref;
        c[1] = id;
        parser->factoryEmpty = c;
    }
}

namespace {
    struct CommandDialogue : public SokuLib::CommandParser::CommandBase {
        SokuLib::String str;
        void parseArgs(char* args) override {
            str.assign(args);
        }
    };

    struct CommandFace : public SokuLib::CommandParser::CommandBase {
        SokuLib::String type;
        SokuLib::String face;

        void parseArgs(char* args) override {
            char* token = strtok_s(args, ",", &args);
            type.assign(token ? token : "");
            token = strtok_s(0, ",", &args);
            if (token) {
                auto iter = faceRemap.find(token);
                face.assign(iter == faceRemap.end() ? token : iter->second);
            } else face.assign("");
        }
    };
}

void LoadParser() {
    DWORD old;
    VirtualProtect((LPVOID)0x00401000, 0x00456000, PAGE_WRITECOPY, &old);

    // CSV Parser
    SokuLib::TamperNearCall(0x40f4cd, csvFixComma);
    TamperCode(0x40f4d2, { 0xEB, 0x02, 0x90, 0x90 }); // skip

    SokuLib::TamperNearCall(0x40f4f2, csvFixQuotes);
    TamperCode(0x40f4f7, { 0xEB, 0x02, 0x90, 0x90 }); // skip

    TamperCode(0x40f5a9, {
        0x3E, 0x8D, 0x4C, 0x24, 0x38,   // lea ecx, iterator
        0x3E, 0x8D, 0x54, 0x24, 0x58,   // lea edx, string
        0x55,                           // push ebp
        0xE8, 0x00, 0x00, 0x00, 0x00,   // call xxx
        0x01, 0xC5,                     // add ebp, eax
        0xE9, 0x4B, 0x00, 0x00, 0x00,   // continue;
    }); SokuLib::TamperNearJmpOpr(0x40f5b4, csvFixDefault);

    // Remove patch from Soku2
    TamperCode(0x40f5ff, {
        0x8D, 0x4C, 0x24, 0x68,
        0xE8, 0xF8, 0x05, 0x00, 0x00,
    }); 

    // Patch argument parser in scripts
    SokuLib::TamperNearJmpOpr(0x4624ee, registerCommand<CommandDialogue>);
    SokuLib::TamperNearJmpOpr(0x46256e, registerCommand<CommandFace>);

    // Command separator using locale
    SokuLib::TamperNearJmpOpr(0x40595e, repl_strchr);

    VirtualProtect((LPVOID)0x00401000, 0x00456000, old, &old);
}