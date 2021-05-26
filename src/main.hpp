#pragma once
#include <filesystem>
#include <unordered_map>

#ifdef _DEBUG
#include <fstream>
extern std::ofstream logging;
#endif

class Command {
public:
    inline virtual ~Command() {}
    virtual void execute(void*) = 0;
};

extern std::filesystem::path modulePath;
extern std::filesystem::path languagePack;
extern std::unordered_map<std::string, Command*> texRedraw;
void LoadHooks();
void LoadLanguage();
void LoadRedraw();

extern "C" __declspec(dllexport) const char* GetTranslation(const char* key);
extern "C" __declspec(dllexport) void SetTranslation(const char* key, const char* value);
extern "C" __declspec(dllexport) int MessageBoxUtf8(void* window, const char* content, const char* title, unsigned int type);