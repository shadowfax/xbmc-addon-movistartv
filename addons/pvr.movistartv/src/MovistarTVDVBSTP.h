#pragma once

#include "MovistarTVRPC.h"
#include "PVRMovistarData.h"
#include <string>
#include <cstdlib>
#include <vector>

namespace MovistarTV
{
	namespace DVBSTP
	{
		/* Service Discovery & Selection protocol (SD&S) header */
		struct sd_s_hdr_t {
			unsigned int version : 2;
			unsigned int resrv : 3;
			unsigned int encryption : 2;
			unsigned int crc_flag : 1;
			unsigned int total_segment_size : 24;
			unsigned int payload_id : 8;
			unsigned int segment_id : 16;
			unsigned int segment_version : 8;
			unsigned int section_number : 12;
			unsigned int last_section_number : 12;
			unsigned int compression : 3;
			unsigned int provider_id_flag : 1;
			unsigned int hdr_len : 4;
		};

		struct DVBSTPPushOffering {
			std::string address;
			unsigned short port;
		};

		/* Missing data */
		struct DVBSTPChannel
		{
			int                     serviceName;	/* Unique identifier */
			int						logicalChannelNumber;
			std::string             name;			/* Channel name */
			std::string				description;
			std::string             streamUrl;		/* Join several fields */
			std::string	            logoUri;		/* ie. http://172.26.22.23:2001/appclient/incoming/epg/MAY_1/imSer/2.jpg */
		};

		int GetXmlFile(const std::string& address, unsigned short port, int payload_id, std::string& response);

		std::vector<DVBSTPPushOffering> GetServiceProviderDiscovery(const std::string& address, unsigned short port, const MovistarTV::ClientProfile& client, const MovistarTV::PlatformProfile& platform);

		std::vector<DVBSTPChannel> GetBroadcastDiscovery(const std::string& address, unsigned short port);
	}
}