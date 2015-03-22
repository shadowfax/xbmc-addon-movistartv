#include "client.h"
#include "PVRMovistarData.h"
#include "MovistarTVDVBSTP.h"
#include "rapidxml/rapidxml.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace ADDON;
using namespace rapidxml;

namespace MovistarTV
{

	//Include platform specific datatypes, header files, defines and constants:
#if defined TARGET_WINDOWS
  #define WIN32_LEAN_AND_MEAN           // Enable LEAN_AND_MEAN support
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
 
  #ifndef NI_MAXHOST
    #define NI_MAXHOST 1025
  #endif

  #ifndef socklen_t
    typedef int socklen_t;
  #endif
  #ifndef ipaddr_t
    typedef unsigned long ipaddr_t;
  #endif
  #ifndef port_t
    typedef unsigned short port_t;
  #endif
#elif defined TARGET_LINUX || defined TARGET_DARWIN || defined TARGET_FREEBSD
#ifdef SOCKADDR_IN
#undef SOCKADDR_IN
#endif
  #include <sys/types.h>     /* for socket,connect */
  #include <sys/socket.h>    /* for socket,connect */
  #include <sys/un.h>        /* for Unix socket */
  #include <arpa/inet.h>     /* for inet_pton */
  #include <netdb.h>         /* for gethostbyname */
  #include <netinet/in.h>    /* for htons */
  #include <unistd.h>        /* for read, write, close */
  #include <errno.h>
  #include <fcntl.h>

  typedef int SOCKET;
  typedef sockaddr SOCKADDR;
  typedef sockaddr_in SOCKADDR_IN;
  #ifndef INVALID_SOCKET
  #define INVALID_SOCKET (-1)
  #endif
  #define SOCKET_ERROR (-1)
  #define closesocket close
#else
  #error Platform specific socket support is not yet available on this platform!
#endif

	namespace DVBSTP
	{

		int GetXmlFile(const std::string& address, unsigned short port, int payload_id, std::string& response)
		{
			SOCKET sd;
			int reuse = 1;
			struct sockaddr_in addr;
			struct ip_mreq mreq;
			int retval = -1;
#if defined(TARGET_WINDOWS)
			int timeout = 3000;
#elif defined TARGET_LINUX || defined TARGET_DARWIN || defined TARGET_FREEBSD
			struct timeval timeout;
			timeout.tv_sec = 3;
			timeout.tv_usec = 0;
#endif

			/* Create socket */
			if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
				XBMC->Log(LOG_ERROR, "Opening datagram socket error\n");
				return -1;
			}

			/* allow multiple sockets to use the same PORT number */
			if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
				XBMC->Log(LOG_ERROR, "Can not reuse socket address\n");
				closesocket(sd);
				return -1;
			}

			/* Set timeout */
#if defined(TARGET_WINDOWS)
			if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int)) < 0) {
#elif defined TARGET_LINUX || defined TARGET_DARWIN || defined TARGET_FREEBSD
			if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval)) < 0) {
#endif
				XBMC->Log(LOG_ERROR, "Can not set the socket receive timeout\n");
				closesocket(sd);
				return - 1;
			}

			/* bind port */
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(port);
			if (bind(sd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
				XBMC->Log(LOG_ERROR, "Can not bind por %d\n", port);
				closesocket(sd);
				return -1;
			}

			/* join multicast group */
			mreq.imr_multiaddr.s_addr = inet_addr(address.c_str());
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);

			if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
				XBMC->Log(LOG_ERROR, "Can not join multicast group %s\n", address.c_str());
				closesocket(sd);
				return -1;
			}

			char databuf[4096];
			int next_section = 0;
			std::string result;
			result.clear();
			int lost_packets = 0;

			while(1)
			{
				struct sd_s_hdr_t header;
				int addrlen = sizeof(addr);
				int nBytes = recvfrom(sd, databuf, 4096, 0, (struct sockaddr *) &addr, &addrlen);
				if (nBytes <= 0) {
					// TODO: set a timeout for the function
					lost_packets++;
					if (lost_packets >= 4) {
						XBMC->Log(LOG_ERROR, "Can not receive data from %s\n", address.c_str());
						break;
					}
					continue;
				}

				lost_packets = 0;
				memset(&header, 0, sizeof(header));

				header.version = ((int)databuf[0] >> 6) & 0x03;
				header.encryption = ((int)databuf[0] >> 1) & 0x03;
				header.crc_flag = (int)databuf[0] & 0x03;
				header.total_segment_size = ((unsigned int)databuf[1] << 16) + ((unsigned int)databuf[2] << 8) + ((unsigned int)databuf[3]);
				header.payload_id = (unsigned int)databuf[4];
				header.segment_id = ((unsigned int)databuf[5] << 8) + ((unsigned int)databuf[6]);

				header.section_number = (databuf[8] << 4) | ((databuf[9] >> 4) &0x0F);
				header.last_section_number = ((databuf[9] & 0x0f) << 8) + databuf[10];
		
				header.compression = ((unsigned int)databuf[11] >> 6) & 0x03;
				header.provider_id_flag = ((unsigned int)databuf[11] >> 4) & 0x01;
				header.hdr_len = (unsigned int)databuf[11] & 0x0F;

				if ((header.payload_id == payload_id) && (header.section_number == next_section)) {
					// Payload
					int payload_offset = 12;
					int payload_length = nBytes - (12 + header.hdr_len);
					if (header.crc_flag) {
						payload_length -= 4;
					}
					if (header.provider_id_flag) {
						payload_length -= 4;
						payload_offset += 4;
					}
					payload_offset += header.hdr_len;

					char *payload_data = (char *)malloc(payload_length + 1);
					memset(payload_data, 0, payload_length + 1);
					strncpy(payload_data, databuf + payload_offset, payload_length);

					result.append(payload_data);

					free(payload_data);

					/* Finally */
					if (header.last_section_number > next_section) {
						next_section++;
					} else {
						response = result;
						retval=0;
						break;
					}
				}
			}

			return retval;
		}

		std::vector<DVBSTPPushOffering> GetServiceProviderDiscovery(const std::string& address, unsigned short port, const MovistarTV::ClientProfile& client, const MovistarTV::PlatformProfile& platform)
		{
			std::vector<DVBSTPPushOffering> m_offerings;
			std::string raw_xml;
			if (GetXmlFile(address, port, 1, raw_xml) != 0) {
				XBMC->Log(LOG_ERROR, "Error fetching DVBSTP data\n");
				return m_offerings;
			}

			/* Cache */
			std::string settingFile = g_strClientPath;
			if (settingFile.at(settingFile.size() - 1) == '\\' ||
				settingFile.at(settingFile.size() - 1) == '/')
				settingFile.append("ServiceProviderDiscovery.xml");
			else
				settingFile.append("/ServiceProviderDiscovery.xml");
  
			XBMC->Log(LOG_NOTICE, "Writing to %s\n", settingFile.c_str());
			std::ofstream out(settingFile);
			out << raw_xml;
			out.close();

			xml_document<> xmlDoc;
			try 
			{
				xmlDoc.parse<0>((char *)raw_xml.c_str());
			} 
			catch(parse_error p) 
			{
				XBMC->Log(LOG_ERROR, "Unable parse SD&S XML: %s", p.what());
				return m_offerings;
			}

			xml_node<> *pRootElement = xmlDoc.first_node("ServiceDiscovery");
			if (!pRootElement)
			{
				XBMC->Log(LOG_ERROR, "Invalid SD&S XML: no <ServiceDiscovery> tag found");
				return m_offerings;
			}

			xml_node<> *pMainElement = pRootElement->first_node("ServiceProviderDiscovery");
			if (!pMainElement)
			{
				XBMC->Log(LOG_ERROR, "Invalid SD&S XML: no <ServiceProviderDiscovery> tag found");
				return m_offerings;
			}

			bool demarcation_found = false;
			bool got_push = false;
			std::string my_domain_name;
			my_domain_name.clear();
			my_domain_name.append("DEM_");
			my_domain_name.append(std::to_string(client.demarcation));
			my_domain_name.append(".");
			my_domain_name.append(platform.dvbConfig.dvbServiceProvider);
			XBMC->Log(LOG_DEBUG, "Searching for %s\n", my_domain_name.c_str());

			xml_node<> *pServiceProviderNode = NULL;
			for(pServiceProviderNode = pMainElement->first_node("ServiceProvider"); pServiceProviderNode; pServiceProviderNode = pServiceProviderNode->next_sibling("ServiceProvider"))
			{
				std::string domain_name;
				xml_attribute<> *pAttribute = pServiceProviderNode->first_attribute("DomainName");
				if (pAttribute == NULL)
				{
					XBMC->Log(LOG_ERROR, "Invalid SD&S XML: no 'DomainName' attribute inside <ServiceProvider> tag");
					return m_offerings;
				}
				domain_name = pAttribute->value();

				if (my_domain_name == domain_name) {
					demarcation_found = true;
			
					xml_node<> *pOfferingNode = pServiceProviderNode->first_node("Offering");
					if (!pOfferingNode)
					{
						XBMC->Log(LOG_ERROR, "No offerings for your service provider\n");
						return m_offerings;
					}
					for(pOfferingNode; pOfferingNode; pOfferingNode = pOfferingNode->next_sibling("Offering"))
					{
						xml_node<> *pPushNode = pOfferingNode->first_node("Push");
						if (!pPushNode)
						{
							XBMC->Log(LOG_ERROR, "No <Push> tag found for the offering\n");
							/* Do not exits, maybe there is another offer */
						} else {
							for(pPushNode; pPushNode; pPushNode = pPushNode->next_sibling("Push"))
							{
								struct DVBSTPPushOffering push_offering;
								memset(&push_offering, 0, sizeof(push_offering));
								xml_attribute<> *pPushAddressAttribute = pPushNode->first_attribute("Address");
								if (pPushAddressAttribute == NULL)
								{
									continue;
								}
								push_offering.address = pPushAddressAttribute->value();

								xml_attribute<> *pPushPortAttribute = pPushNode->first_attribute("Port");
								if (pPushPortAttribute == NULL)
								{
									push_offering.port = 3937;
								} 
								else 
								{
									/*push_offering.port = std::stoi(pPushPortAttribute->value());*/
									std::string port_value = pPushPortAttribute->value();
									push_offering.port = atoi(port_value.c_str());
								}

								got_push = true;

								m_offerings.push_back(push_offering);
							}
						}
					}

					/* exit for loop */
					break;
				}
		
			}

			if (!demarcation_found) {
				XBMC->Log(LOG_ERROR, "Unable to find a suitable service provider for your demarcation\n");
				return m_offerings;
			}

			if (!got_push) {
				XBMC->Log(LOG_ERROR, "Unable to find a suitable offering for your demarcation\n");
				return m_offerings;
			}

			return m_offerings;
		}

		std::vector<DVBSTPChannel> GetBroadcastDiscovery(const std::string& address, unsigned short port)
		{
			std::vector<DVBSTPChannel> channels;
			std::string raw_xml;
			if (GetXmlFile(address, port, 2, raw_xml) != 0) {
				XBMC->Log(LOG_ERROR, "Error fetching DVBSTP data\n");
				return channels;
			}

			/* Cache */
			/* Cache */
			std::string settingFile = g_strClientPath;
			if (settingFile.at(settingFile.size() - 1) == '\\' ||
				settingFile.at(settingFile.size() - 1) == '/')
				settingFile.append("BroadcastDiscovery.xml");
			else
				settingFile.append("/BroadcastDiscovery.xml");
  
			XBMC->Log(LOG_NOTICE, "Writing to %s\n", settingFile.c_str());
			std::ofstream out(settingFile);
			out << raw_xml;
			out.close();

			xml_document<> xmlDoc;
			try 
			{
				xmlDoc.parse<0>((char *)raw_xml.c_str());
			} 
			catch(parse_error p) 
			{
				XBMC->Log(LOG_ERROR, "Unable parse SD&S XML: %s", p.what());
				return channels;
			}

			xml_node<> *pRootElement = xmlDoc.first_node("ServiceDiscovery");
			if (!pRootElement)
			{
				XBMC->Log(LOG_ERROR, "Invalid SD&S XML: no <ServiceDiscovery> tag found");
				return channels;
			}

			xml_node<> *pMainElement = pRootElement->first_node("BroadcastDiscovery");
			if (!pMainElement)
			{
				XBMC->Log(LOG_ERROR, "Invalid SD&S XML: no <BroadcastDiscovery> tag found");
				return channels;
			}

			xml_node<> *pServiceListElement = pMainElement->first_node("ServiceList");
			if (!pServiceListElement)
			{
				XBMC->Log(LOG_ERROR, "Invalid SD&S XML: no <ServiceList> tag found");
				return channels;
			}

			xml_node<> *pSingleServiceNode = NULL;
			for(pSingleServiceNode = pServiceListElement->first_node("SingleService"); pSingleServiceNode; pSingleServiceNode = pSingleServiceNode->next_sibling("SingleService"))
			{
				struct DVBSTPChannel channel;

				/* Stream Address */
				xml_node<> *pServiceLocationElement = pSingleServiceNode->first_node("ServiceLocation");
				if (!pServiceLocationElement)
				{
					XBMC->Log(LOG_DEBUG, "No <ServiceLocation> tag found");
					continue;
				}
				xml_node<> *pIPMulticastAddressElement = pServiceLocationElement->first_node("IPMulticastAddress");
				if (!pIPMulticastAddressElement)
				{
					XBMC->Log(LOG_DEBUG, "No <IPMulticastAddress> tag found");
					continue;
				}

				xml_attribute<> *pMulticastAddressAttribute = pIPMulticastAddressElement->first_attribute("Address");
				if (!pMulticastAddressAttribute)
				{
					XBMC->Log(LOG_DEBUG, "No <Address> tag found");
					continue;
				}
				channel.streamUrl = "rtp://@";
				channel.streamUrl.append(pMulticastAddressAttribute->value());
				channel.streamUrl.append(":");

				xml_attribute<> *pMulticastPortAttribute = pIPMulticastAddressElement->first_attribute("Port");
				if (!pMulticastPortAttribute)
				{
					channel.streamUrl.append("8208");
				} else {
					channel.streamUrl.append(pMulticastPortAttribute->value());
				}

				/* Textual Identifier */
				xml_node<> *pTextualIdentifierElement = pSingleServiceNode->first_node("TextualIdentifier");
				if (!pTextualIdentifierElement)
				{
					XBMC->Log(LOG_DEBUG, "No <TextualIdentifier> tag found");
					continue;
				}

				xml_attribute<> *pServiceNameAttribute = pTextualIdentifierElement->first_attribute("ServiceName");
				if (!pServiceNameAttribute)
				{
					continue;
				}
				/*channel.serviceName = std::stoi(pServiceNameAttribute->value());*/
				std::string service_name_value = pServiceNameAttribute->value();
				channel.serviceName = atoi(service_name_value.c_str());

				xml_attribute<> *pLogoUriAttribute = pTextualIdentifierElement->first_attribute("logoURI");
				if (!pLogoUriAttribute)
				{
					channel.logoUri = "";
				} else {
					channel.logoUri = "http://172.26.22.23:2001/appclient/incoming/epg/";
					channel.logoUri.append(pLogoUriAttribute->value());
				}

				/* Service Information */
				xml_node<> *pSIElement = pSingleServiceNode->first_node("SI");
				if (!pSIElement)
				{
					XBMC->Log(LOG_DEBUG, "No <SI> tag found");
					continue;
				}

				/* TODO: Maybe there are other languages */
				xml_node<> *pNameNode = pSIElement->first_node("Name");
				if (!pNameNode)
				{
					XBMC->Log(LOG_DEBUG, "No <Name> tag found");
					continue;
				}
				channel.name = pNameNode->value();

				xml_node<> *pDescriptionNode = pSIElement->first_node("Name");
				if (!pDescriptionNode)
				{
					continue;
				}
				channel.description = pDescriptionNode->value();

				channels.push_back(channel);
			}

			return channels;
		}
	}
}