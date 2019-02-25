#ifndef __LOCALIZATION_H__
#define __LOCALIZATION_H__

#include <pragma/networkdefinitions.h>
#include <string>
#include <vector>
#include <unordered_map>

class Locale;
struct DLLNETWORK Localization
{
public:
	friend Locale;
protected:
	Localization();
public:
	std::unordered_map<std::string,std::string> texts;
};

class DLLNETWORK Locale
{
private:
	static Localization m_localization;
	static std::vector<std::string> m_loadedFiles;
	static std::string m_language;
	static bool Load(const std::string &file,const std::string &lan,bool bReload);
public:
	static bool Load(const std::string &file,bool bReload=false);
	static void Initialize(std::string lan);
	static std::string GetText(const std::string &id);
	static std::string GetText(const std::string &id,const std::vector<std::string> &args);
	static bool GetText(const std::string &id,std::string &r);
	static const std::string &GetLanguage();
};

#endif