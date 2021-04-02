#pragma once
#include <filesystem>

#ifdef _DEBUG
extern std::ofstream logging;
#endif

extern std::filesystem::path modulePath;
extern std::filesystem::path languagePack;
void LoadHooks();
void LoadLanguage();

extern "C" __declspec(dllexport) const char* GetTranslation(const char* key);
extern "C" __declspec(dllexport) void SetTranslation(const char* key, const char* value);