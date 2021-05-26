#include "main.hpp"
#include <mutex>

namespace {
    typedef struct { int pitch; void* data; } D3DLOCKED_RECT;
    typedef void (*clearTexture_t)(void*, int, int);
    clearTexture_t orig_clearTexture = (clearTexture_t) 0x8201b0;
    typedef void (__fastcall* renderText_t)(int, void*, const char*, int*, int*);
    renderText_t orig_renderText = (renderText_t) 0x412910;

    void* fontHandle = 0;
    typedef struct Font {
        char fontName[0x100];
        unsigned char baseColors[0x8];
        int fontHeight, fontWeight;
        char flags[0x4];
        int unknown;
        int paddingX, paddingY;
        int spacingX, spacingY;

        Font(const char* fName, unsigned int c0, unsigned int c1, bool italic = false, bool shadow = false, bool wrap = false) {
            memset(this, 0, sizeof(this));
            strcpy(fontName, fName);
            baseColors[0] = c0>>16;
            baseColors[1] = c1>>16;
            baseColors[2] = c0>>8;
            baseColors[3] = c1>>8;
            baseColors[4] = c0;
            baseColors[5] = c1;
            flags[0] = italic;
            flags[1] = shadow;
            flags[2] = wrap;
        }
    } font_t;
}

#define dxLockRect(texture, ...) \
    (*((int (__stdcall**)(void*, int, D3DLOCKED_RECT*, int, int))(*(int*)texture + 0x4c)))(texture, __VA_ARGS__)
#define dxUnlockRect(texture, ...) \
    (*((int (__stdcall**)(void*, int))(*(int*)texture + 0x50)))(texture, __VA_ARGS__)

static void drawGrid(int* data, int size, int width, int height) {
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if ((x%size == 0 || y%size == 0) && (data[y*width + x] >> 24) == 0) {
                data[y*width + x] = 0xff555555;
            }
        }
    }
}

static void selectFont(font_t* font) {
    if (!fontHandle) {
        fontHandle = new char[0x1a4];
        ((void (__fastcall*)(void*))0x4116d0)(fontHandle); // FontCreate
    }

    if (*(void**)((int)fontHandle+0x158) != 0x0) {
      ((void (*)(void*))0x8224b6)(*(void**)((int)fontHandle+0x158));
      *(void**)((int)fontHandle+0x154) = *(void**)((int)fontHandle+0x158) = 0;
    }
    ((void (__fastcall*)(void*, int, int))0x411840)(fontHandle, 0, (int)font); // FontSetIndirect
}

static void drawText(int font, void* dxTex, const char* text, int width, int height, bool clear = true) {
    D3DLOCKED_RECT rect;
    if (dxLockRect(dxTex, 0, &rect, 0, 0) == 0) {
        if (clear) std::memset(rect.data, 0, rect.pitch * height);
        //orig_clearTexture(rect.data, 0, rect.pitch * 24);
        *(void**)(font+0x140) = *(void**)(font+0x144) = rect.data;
        *(int*)(font+0x150) = rect.pitch / 4;
        *(int*)(font+0x14c) = width;
        *(int*)(font+0x148) = height;
        orig_renderText(font, 0, text, 0, 0);
#ifdef _DEBUG
        drawGrid((int*)rect.data, 4, rect.pitch / 4, height);
#endif
        dxUnlockRect(dxTex, 0);
    }
}

std::mutex textRenderLock;
std::unordered_map<std::string, Command*> texRedraw;
class RedrawSimple : public Command {
public:
    font_t* font;
    const char* text;
    const int x, y, w, h, sy;
    bool clear;
    inline RedrawSimple(font_t* font, const char* text, int x, int y, int w, int h, int sy = 0, bool clear = true)
        : font(font), text(text), x(x), y(y), w(w), h(h), sy(sy), clear(clear) {}

    void execute(void* texture) override {
        std::lock_guard guard(textRenderLock);
        font->paddingX = x;
        font->paddingY = y;
        font->spacingY = sy;
        selectFont(font);
        drawText((int)fontHandle, texture, text, w, h, clear);
    }
};

class RedrawMultiple : public Command {
public:
    struct Item {
        font_t* font;
        const char* text;
        const int x, y;
    };

    int w, h;
    bool clear;
    std::vector<Item> items;
    inline RedrawMultiple(int w, int h, bool clear = true) : w(w), h(h), clear(clear) {}

    void addItem(font_t* font, const char* text, int x, int y) {
        items.emplace_back(Item{font, text, x, y});
    }

    void execute(void* texture) override {
        std::lock_guard guard(textRenderLock);
        bool first = true;
        for (auto& item : items) {
            item.font->paddingX = item.x;
            item.font->paddingY = item.y;
            selectFont(item.font);
            drawText((int)fontHandle, texture, item.text, w, h, clear && first);
            first = false;
        }
    }
};

font_t menuFont("MonoSpatialModSWR", 0xffffff, 0xffffff, false, true, false);
font_t menuFontGray("MonoSpatialModSWR", 0x555555, 0x555555, false, true, false);
font_t flavorFont("MonoSpatialModSWR", 0xffffff, 0xffff40, false, true, false);
font_t characterFont("MonoSpatialModSWR", 0xffffff, 0xffffff, false, true, false);
font_t backgroundFont("MonoSpatialModSWR", 0xffffff, 0xffffff, false, true, false);
font_t bgmFont("MonoSpatialModSWR", 0xffffff, 0xffffff, false, true, false);
font_t skillCostFont("MonoSpatialModSWR", 0xffffff, 0xffffff, false, true, false);
font_t weatherFont0("MonoSpatialModSWR", 0xffffff, 0xffffff, false, true, false);
font_t weatherFont1("MonoSpatialModSWR", 0xffd0d0, 0xffffff, false, true, false);
font_t weatherFont2("MonoSpatialModSWR", 0xe04040, 0xd0d0d0, false, true, false);

void LoadRedraw() {
    menuFont.fontHeight =
    menuFontGray.fontHeight = 18;
    menuFont.fontWeight =
    menuFontGray.fontWeight = 500;

    texRedraw["data/menu/22a_Yes.bmp"] = new RedrawSimple(&menuFontGray, "<key gui.yes>", 3, 2, 128, 24);
    texRedraw["data/menu/22b_Yes.bmp"] = new RedrawSimple(&menuFont, "<key gui.yes>", 3, 2, 128, 24);
    texRedraw["data/menu/23a_No.bmp"] = new RedrawSimple(&menuFontGray, "<key gui.no>", 3, 2, 128, 24);
    texRedraw["data/menu/23b_No.bmp"] = new RedrawSimple(&menuFont, "<key gui.no>", 3, 2, 128, 24);
    texRedraw["data/profile/deck2/050_Yes.bmp"] = new RedrawSimple(&menuFontGray, "<key gui.yes>", 3, 2, 128, 24);
    texRedraw["data/profile/deck2/051_Yes.bmp"] = new RedrawSimple(&menuFont, "<key gui.yes>", 3, 2, 128, 24);
    texRedraw["data/profile/deck2/060_No.bmp"] = new RedrawSimple(&menuFontGray, "<key gui.no>", 3, 2, 128, 24);
    texRedraw["data/profile/deck2/061_No.bmp"] = new RedrawSimple(&menuFont, "<key gui.no>", 3, 2, 128, 24);

    texRedraw["data/profile/keyconfig/020_On.bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.8>", 0, 0, 32, 16);
    texRedraw["data/profile/keyconfig/021_Off.bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.9>", 0, 0, 32, 16);

    texRedraw["data/menu/config/c1[01].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.label.0>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c1[02].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.label.1>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c1[03].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.label.2>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c1[04].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.label.3>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c1[05].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.label.4>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c1[06].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.label.5>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c1[07].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.label.6>", 3, 2, 128, 24);
    
    texRedraw["data/menu/config/c2[01].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.0>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[02].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.1>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[03].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.2>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[04].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.3>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[05].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.4>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[06].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.5>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[07].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.6>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[08].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.7>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[09].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.8>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[10].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.9>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[11].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.10>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[12].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.11>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[13].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.12>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[14].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.13>", 3, 2, 128, 24);
    texRedraw["data/menu/config/c2[15].bmp"] = new RedrawSimple(&menuFont, "<key gui.config.option.14>", 3, 2, 128, 24);

    texRedraw["data/menu/battle/000_Restart.bmp"] = new RedrawSimple(&menuFont, "<key gui.pause.0>", 3, 2, 160, 24);
    texRedraw["data/menu/battle/010_1pKeyConfig.bmp"] = new RedrawSimple(&menuFont, "<key gui.pause.1>", 3, 2, 160, 24);
    texRedraw["data/menu/battle/020_2pKeyConfig.bmp"] = new RedrawSimple(&menuFont, "<key gui.pause.2>", 3, 2, 160, 24);
    texRedraw["data/menu/battle/030_Chara.bmp"] = new RedrawSimple(&menuFont, "<key gui.pause.3>", 3, 2, 160, 24);
    texRedraw["data/menu/battle/040_Title.bmp"] = new RedrawSimple(&menuFont, "<key gui.pause.4>", 3, 2, 160, 24);

    texRedraw["data/menu/connect/00a_Server.bmp"] = new RedrawSimple(&menuFont, "<key gui.netplay.0>", 3, 2, 256, 24);
    texRedraw["data/menu/connect/01a_IP.bmp"] = new RedrawSimple(&menuFont, "<key gui.netplay.1>", 3, 2, 256, 24);
    texRedraw["data/menu/connect/02a_List.bmp"] = new RedrawSimple(&menuFont, "<key gui.netplay.2>", 3, 2, 256, 24);
    texRedraw["data/menu/connect/03a_History.bmp"] = new RedrawSimple(&menuFont, "<key gui.netplay.3>", 3, 2, 256, 24);
    texRedraw["data/menu/connect/04a_Clipboard.bmp"] = new RedrawSimple(&menuFont, "<key gui.netplay.4>", 3, 2, 256, 24);
    texRedraw["data/menu/connect/05a_Profile.bmp"] = new RedrawSimple(&menuFont, "<key gui.netplay.5>", 3, 2, 256, 24);

    texRedraw["data/menu/practice/GamePlay.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.0>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/Weather.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.1>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/SP.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.2>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/State.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.3>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/Position.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.4>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/Guard.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.5>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/Counter.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.6>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/Passive.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.7>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/CharactorSerect.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.8>", 3, 2, 168, 24);
    texRedraw["data/menu/practice/Title.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.label.9>", 3, 2, 168, 24);

    texRedraw["data/menu/practice/6.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.0>", 3, 2, 128, 24);
    texRedraw["data/menu/practice/Stand.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.1>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/Sit.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.2>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/Jump.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.3>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/2pPlay.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.4>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/On.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.5>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/Off.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.6>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/Up_guard.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.7>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/Down_guard.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.8>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/One.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.9>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/front.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.10>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/back.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.11>", 3, 2, 64, 24);
    texRedraw["data/menu/practice/random.bmp"] = new RedrawSimple(&menuFont, "<key gui.practice.option.12>", 3, 2, 64, 24);

    texRedraw["data/menu/practice/Weather_list.bmp"] = new RedrawSimple(&menuFont, 
        "<key gui.weather.0.name><br>"  "<key gui.weather.1.name><br>"  "<key gui.weather.2.name><br>"
        "<key gui.weather.3.name><br>"  "<key gui.weather.4.name><br>"  "<key gui.weather.5.name><br>"
        "<key gui.weather.6.name><br>"  "<key gui.weather.7.name><br>"  "<key gui.weather.8.name><br>"
        "<key gui.weather.9.name><br>"  "<key gui.weather.10.name><br>" "<key gui.weather.11.name><br>"
        "<key gui.weather.12.name><br>" "<key gui.weather.13.name><br>" "<key gui.weather.14.name><br>"
        "<key gui.weather.15.name><br>" "<key gui.weather.16.name><br>" "<key gui.weather.17.name><br>"
        "<key gui.weather.18.name><br>" "<key gui.weather.19.name><br>" "<key gui.weather.20.name>",
        0, 0, 200, 504, 6);

    texRedraw["data/profile/01a_DeckConstruct.bmp"] = new RedrawSimple(&menuFont, "<key gui.profile.0>", 3, 2, 128, 24);
    texRedraw["data/profile/02a_KeyConfig.bmp"] = new RedrawSimple(&menuFont, "<key gui.profile.1>", 3, 2, 128, 24);
    texRedraw["data/profile/00a_NewMaking.bmp"] = new RedrawSimple(&menuFont, "<key gui.profile.2>", 3, 2, 128, 24);
    texRedraw["data/profile/03a_Copy.bmp"] = new RedrawSimple(&menuFont, "<key gui.profile.3>", 3, 2, 128, 24);
    texRedraw["data/profile/04a_Delete.bmp"] = new RedrawSimple(&menuFont, "<key gui.profile.4>", 3, 2, 128, 24);
    texRedraw["data/profile/05a_Rename.bmp"] = new RedrawSimple(&menuFont, "<key gui.profile.5>", 3, 2, 128, 24);
    texRedraw["data/profile/20_Name.bmp"] = new RedrawSimple(&menuFont, "<key gui.profile.7>", 16, 20, 330, 74, 0, false);

    texRedraw["data/profile/deck1/00a_reimu.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.reimu.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/01a_marisa.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.marisa.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/02a_sakuya.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.sakuya.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/03a_alice.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.alice.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/04a_patchouli.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.patchouli.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/05a_youmu.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.youmu.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/06a_remilia.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.remilia.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/07a_yuyuko.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.yuyuko.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/08a_yukari.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.yukari.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/09a_suika.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.suika.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/10a_udonge.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.udonge.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/11a_aya.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.aya.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/12a_komachi.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.komachi.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/13a_iku.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.iku.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/14a_tenshi.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.tenshi.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/15a_sanae.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.sanae.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/16a_chirno.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.chirno.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/17a_meirin.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.meirin.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/18a_utsuho.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.utsuho.name>", 3, 2, 120, 24);
    texRedraw["data/profile/deck1/19a_suwako.bmp"] = new RedrawSimple(&menuFont, "<key gui.character.suwako.name>", 3, 2, 120, 24);

    skillCostFont.fontHeight = 14;
    skillCostFont.fontWeight = 500;

    texRedraw["data/profile/deck2/022_SkillCard000Cost.bmp"] = new RedrawSimple(&skillCostFont, "<key gui.profile.6>", 34, 15, 64, 32, 0, false);
    texRedraw["data/profile/deck2/022_SkillCard001Cost.bmp"] = new RedrawSimple(&skillCostFont, "<key gui.profile.6>", 34, 15, 64, 32, 0, false);
    texRedraw["data/profile/deck2/022_SkillCard002Cost.bmp"] = new RedrawSimple(&skillCostFont, "<key gui.profile.6>", 34, 15, 64, 32, 0, false);
    texRedraw["data/profile/deck2/022_SkillCard003Cost.bmp"] = new RedrawSimple(&skillCostFont, "<key gui.profile.6>", 34, 15, 64, 32, 0, false);
    texRedraw["data/profile/deck2/022_SkillCard004Cost.bmp"] = new RedrawSimple(&skillCostFont, "<key gui.profile.6>", 22, 15, 64, 32, 0, false);

    texRedraw["data/menu/select/000_1pProfile.bmp"] = new RedrawSimple(&menuFont, "<key gui.select.0>", 3, 2, 160, 24);
    texRedraw["data/menu/select/010_2pProfile.bmp"] = new RedrawSimple(&menuFont, "<key gui.select.1>", 3, 2, 160, 24);
    texRedraw["data/menu/select/020_1pDeck.bmp"] = new RedrawSimple(&menuFont, "<key gui.select.2>", 3, 2, 160, 24);
    texRedraw["data/menu/select/030_2pDeck.bmp"] = new RedrawSimple(&menuFont, "<key gui.select.3>", 3, 2, 160, 24);
    texRedraw["data/menu/select/040_1pKeyConfig.bmp"] = new RedrawSimple(&menuFont, "<key gui.select.4>", 3, 2, 160, 24);
    texRedraw["data/menu/select/050_2pKeyConfig.bmp"] = new RedrawSimple(&menuFont, "<key gui.select.5>", 3, 2, 160, 24);
    texRedraw["data/menu/select/060_Title.bmp"] = new RedrawSimple(&menuFont, "<key gui.select.6>", 3, 2, 160, 24);

    texRedraw["data/menu/replay/play.bmp"] = new RedrawSimple(&menuFont, "<key gui.replay.0>", 3, 2, 128, 24);
    texRedraw["data/menu/replay/Delete.bmp"] = new RedrawSimple(&menuFont, "<key gui.replay.1>", 3, 2, 128, 24);

    bgmFont.fontHeight = 14;
    bgmFont.fontWeight = 500;

    texRedraw["data/infoEffect/st00.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st00.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st01.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st01.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st02.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st02.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st03.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st03.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st04.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st04.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st05.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st05.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st06.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st06.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st10.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st10.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st11.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st11.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st12.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st12.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st13.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st13.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st14.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st14.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st15.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st15.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st16.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st16.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st17.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st17.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st18.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st18.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st19.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st19.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st20.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st20.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st21.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st21.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st22.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st22.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st30.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st30.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st31.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st31.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st32.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st32.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st33.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st33.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st34.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st34.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st35.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st35.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st40.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st40.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st41.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st41.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st42.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st42.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st43.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st43.name>", 3, 0, 256, 16);
    texRedraw["data/infoEffect/st99.bmp"] = new RedrawSimple(&bgmFont, "BGM <key csv.music.st99.name>", 3, 0, 256, 16);

    backgroundFont.fontHeight = 28;
    backgroundFont.fontWeight = 700;
    backgroundFont.spacingX = -2;

    texRedraw["data/scene/select/scenario/2_moji.bmp"] = new RedrawSimple(&menuFont, "<key gui.scenario.0><br><key gui.scenario.1>", 0, 6, 96, 64, 14);
    texRedraw["data/scene/select/bg/bg_name00.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.0>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name01.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.1>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name02.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.2>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name03.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.3>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name04.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.4>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name05.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.5>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name10.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.6>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name11.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.7>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name12.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.8>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name13.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.9>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name14.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.10>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name15.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.11>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name16.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.12>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name17.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.13>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name18.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.14>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name30.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.15>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name31.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.16>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name32.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.17>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name33.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.18>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name34.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.19>", 2, 2, 256, 64);
    texRedraw["data/scene/select/bg/bg_name99.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.background.20>", 2, 2, 256, 64);

    weatherFont0.fontHeight = 20;
    weatherFont0.fontWeight = 900;
    weatherFont0.spacingX = -2;

    texRedraw["data/infoEffect/weatherName000.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.0.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName001.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.1.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName002.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.2.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName003.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.3.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName004.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.4.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName005.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.5.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName006.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.6.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName007.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.7.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName008.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.8.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName009.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.9.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName010.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.10.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName012.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.11.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName013.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.12.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName014.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.13.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName015.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.14.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName016.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.20.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName017.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.21.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName018.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.15.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName019.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.16.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName020.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.17.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName021.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.18.name>", 2, 2, 64, 32);
    texRedraw["data/infoEffect/weatherName022.bmp"] = new RedrawSimple(&backgroundFont, "<key gui.weather.19.name>", 2, 2, 64, 32);

    weatherFont1.fontHeight = 72;
    weatherFont2.fontHeight = 24;
    weatherFont1.fontWeight = 900;
    weatherFont2.fontWeight = 500;
    weatherFont1.spacingX = 0;
    weatherFont2.spacingX = 0;

    RedrawMultiple* rm;
    texRedraw["data/infoEffect/weatherNameB000.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.1.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.1.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB001.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.2.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.2.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB002.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.3.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.3.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB003.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.4.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.4.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB004.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.5.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.5.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB005.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.6.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.6.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB006.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.7.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.7.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB007.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.8.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.8.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB008.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.9.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.9.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB009.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.10.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.10.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB010.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.11.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.11.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB011.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.12.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.12.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB012.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.13.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.13.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB013.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.14.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.14.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB014.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.20.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.20.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB015.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.15.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.15.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB016.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.16.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.16.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB017.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.17.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.17.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB018.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.18.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.18.flavor>", 2, 94);
    texRedraw["data/infoEffect/weatherNameB019.bmp"] = rm = new RedrawMultiple(256, 128);
    rm->addItem(&weatherFont1, "<key gui.weather.19.name>", 2, 2);
    rm->addItem(&weatherFont2, "<key gui.weather.19.flavor>", 2, 94);

    flavorFont.fontHeight = 14;
    characterFont.fontHeight = 42;
    flavorFont.fontWeight = 600;
    characterFont.fontWeight = 900;
    characterFont.spacingX = -3;

    texRedraw["data/scene/select/character/01b_name/name_00.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.reimu.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.reimu.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_01.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.marisa.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.marisa.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_02.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.sakuya.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.sakuya.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_03.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.alice.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.alice.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_04.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.patchouli.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.patchouli.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_05.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.youmu.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.youmu.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_06.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.remilia.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.remilia.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_07.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.yuyuko.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.yuyuko.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_08.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.yukari.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.yukari.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_09.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.suika.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.suika.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_10.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.udonge.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.udonge.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_11.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.aya.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.aya.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_12.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.komachi.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.komachi.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_13.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.iku.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.iku.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_14.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.tenshi.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.tenshi.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_15.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.sanae.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.sanae.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_16.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.chirno.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.chirno.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_17.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.meirin.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.meirin.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_18.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.utsuho.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.utsuho.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_19.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.suwako.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.suwako.flavor>", 2, 4);
    texRedraw["data/scene/select/character/01b_name/name_20.bmp"] = rm = new RedrawMultiple(256, 64);
    rm->addItem(&characterFont, "<key gui.character.random.name>", 2, 22);
    rm->addItem(&flavorFont, "<key gui.character.random.flavor>", 2, 4);

    /* TODO:
     * - data_guide_**
     * * data_infoEffect_**
     * * data_profile_20_Name
     * * data_profile_deck2_**
     * - data_profile_deck2_name_**
     * * data_profile_keyconfig_**
     * * data_scene_select_bg_**
     * * data_scene_select_character_**
     * * data_scene_select_scenario_**
     */
}