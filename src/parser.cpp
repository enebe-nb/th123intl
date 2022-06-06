#include <SokuLib.hpp>
#include <mbstring.h>
#include "main.hpp"

namespace {
    typedef std::pair<SokuLib::String*, char*> StringIterator;

    typedef void (__fastcall *appendChar_t)(SokuLib::String&, int unused, StringIterator&, SokuLib::String&, char*, char);
    appendChar_t orig_appendChar = (appendChar_t) 0x0040fc00;
    typedef void (__fastcall *orig_text_func_t)(unsigned int ecx, void* edx, void *a, void *b, void *c, unsigned int d);
    orig_text_func_t orig_text_func = (orig_text_func_t) 0x40fc00;
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

static char* __cdecl scriptFixTokenize(char* str, const char* delim, char** context) {
    if (!context) { _set_errno(EINVAL); return nullptr; }
    if (!delim) { _set_errno(EINVAL); return nullptr; }
    if (!str) str = *context;
    if (!str) { _set_errno(EINVAL); return nullptr; }

    str += _mbsspn_l((const uint8_t*)str, (const uint8_t*)delim, langConfig.locale);
    size_t index = _mbscspn_l((const uint8_t*)str, (const uint8_t*)delim, langConfig.locale);
    if (!index) return nullptr;
    if (!str[index]) {
        *context = &str[index];
        return str;
    } *context = (char*)_mbsinc_l((const uint8_t*)&str[index], langConfig.locale);

    unsigned char* prev = _mbsdec_l((const uint8_t*)str, (const uint8_t*)&str[index], langConfig.locale);
    if (!prev) { _set_errno(EINVAL); return nullptr; }
    if (*prev == '\\') {
        scriptFixTokenize(0, delim, context);
        return str;
    } else {
        str[index] = '\0';
        return str;
    }
}

// we do still need escape character for other mods TODO any locale
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

    // escape codes on scripts TODO currently hooks _strtok_s
    TamperCode(0x82112a, {
        0x52,                   // PUSH edx
        0x55,                   // PUSH ebp
        0xe8, 0, 0, 0, 0,       // CALL xxxx
        0x90, 0x90, 0x90, 0x90, // NOPs
        0x90, 0x90, 0x90, 0x90, // NOPs
        0x90, 0x90, 0x90, 0x90, // NOPs
        0x84, 0xc0              // TEST al, al
    });
    SokuLib::TamperNearJmpOpr(0x82112c, script_passthrough);

    VirtualProtect((LPVOID)0x00401000, 0x00456000, old, &old);
}