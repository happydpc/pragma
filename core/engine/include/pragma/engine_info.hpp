#ifndef __ENGINE_INFO_HPP__
#define __ENGINE_INFO_HPP__

#include "pragma/definitions.h"
#include <string>
#include <vector>

namespace engine_info
{
	DLLENGINE std::string get_identifier();
	DLLENGINE std::string get_name();
	DLLENGINE std::string get_executable_name();
	DLLENGINE std::string get_server_executable_name();
	DLLENGINE std::string get_author_mail_address();
	DLLENGINE std::string get_website_url();
	DLLENGINE std::string get_modding_hub_url();
	DLLENGINE std::string get_wiki_url();
	DLLENGINE std::string get_forums_url();
	DLLENGINE std::string get_patreon_url();

	// Returns the extensions for the supported audio formats
	DLLENGINE const std::vector<std::string> get_supported_audio_formats();
};

#endif