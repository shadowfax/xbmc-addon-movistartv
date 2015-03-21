#pragma once

#include <string>
#include <json/json.h>
#include <cstdlib>

#define E_SUCCESS 0
#define E_FAILED -1
#define E_EMPTYRESPONSE -2


namespace MovistarTV
{
	/* TODO: complete as many fields are missing */
	struct ClientProfile
	{
		int             version;
		int             demarcation;
		std::string     clientVersion;
	};

	struct DVBConfig
	{
		std::string dvbServiceProvider;
		std::string dvbEntryPoint;
	};

	/* TODO: complete as many fields are missing */
	struct PlatformProfile
	{
		DVBConfig dvbConfig;
		std::string clientVersion;
	};

	int GetClientProfile(ClientProfile &profile);
	int GetPlatformProfile(PlatformProfile &profile);
}