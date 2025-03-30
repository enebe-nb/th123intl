#pragma once
#include <filesystem>
#include <unordered_map>
#include <vector>

#ifdef _DEBUG
#include <fstream>
extern std::ofstream logging;
#endif

extern std::filesystem::path modulePath;
extern std::filesystem::path languagePack;
void LoadHooks();
void LoadLanguage();
void LoadParser();

struct LangConfig {
	struct FontOverride {
		void* data;
		size_t offset;
		size_t size;
	};

	struct TileOverride {
		unsigned int data;
		unsigned int param;
	};

	_locale_t locale = 0;
	std::string localeName = "";
	unsigned int charset = 128;
	std::vector<std::filesystem::path> packFiles;
	std::unordered_map<unsigned int, std::vector<FontOverride>> fontOverrides;
	std::unordered_map<unsigned int, std::vector<TileOverride>> tileOverrides;

	~LangConfig();
}; extern struct LangConfig langConfig;