/*
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "tinyxml/XMLUtils.h"
#include "PVRMovistarData.h"
#include "MovistarTVRPC.h"
#include "MovistarTVDVBSTP.h"
#include <string>
#include <fstream>

using namespace std;
using namespace ADDON;

PVRMovistarData::PVRMovistarData(void)
{
	m_iEpgStart = -1;
	m_strDefaultIcon =  "http://3.bp.blogspot.com/-YmOxQO5QR1E/U_2J07e6eXI/AAAAAAAAAhw/_PWsrnm9xzU/s1600/movistartv.jpg";
	m_strDefaultMovie = "";

	if (!LoadMovistarData()) {
		/* Load last known configuration */
		//XBMC->Log(LOG_DEBUG, "Loading fallback data\n");
		//LoadFallbackData();
	}

	XBMC->Log(LOG_NOTICE, "Channels: %d\n", m_channels.size());
}

PVRMovistarData::~PVRMovistarData(void)
{
	m_channels.clear();
	m_groups.clear();
}

std::string PVRMovistarData::GetSettingsFile() const
{
  string settingFile = g_strClientPath;
  if (settingFile.at(settingFile.size() - 1) == '\\' ||
      settingFile.at(settingFile.size() - 1) == '/')
    settingFile.append("PVRMovistarAddonSettings.xml");
  else
    settingFile.append("/PVRMovistarAddonSettings.xml");
  return settingFile;
}

bool PVRMovistarData::LoadMovistarData(void)
{
	MovistarTV::ClientProfile client_profile;
	MovistarTV::PlatformProfile platform_profile;
	std::string xml_raw;

	if (MovistarTV::GetClientProfile(client_profile) != 0) {
		XBMC->Log(LOG_ERROR, "Error getting the client profile\n");
		return false;
	}
	XBMC->Log(LOG_DEBUG, "User demarcation: %d\n", client_profile.demarcation);
	
	if (MovistarTV::GetPlatformProfile(platform_profile) != 0) {
		XBMC->Log(LOG_ERROR, "Error getting the platform profile\n");
		return false;
	}
	XBMC->Log(LOG_DEBUG, "DVB Entry Point: %s\n", platform_profile.dvbConfig.dvbEntryPoint.c_str());

	/* Split host and port */
	int dvb_entry_point_port = 3937;
	std:string dvb_entry_point_host;
	size_t colonPos = platform_profile.dvbConfig.dvbEntryPoint.find(':');

	if(colonPos != std::string::npos)
	{
		dvb_entry_point_host = platform_profile.dvbConfig.dvbEntryPoint.substr(0, colonPos);
		std::string portPart = platform_profile.dvbConfig.dvbEntryPoint.substr(colonPos + 1);

		dvb_entry_point_port = atoi(portPart.c_str());
	}

	/* We should now do the multicast stuff on the DVB Entry Point */
	std::vector<MovistarTV::DVBSTP::DVBSTPPushOffering> service_provider_offerings;
	service_provider_offerings = MovistarTV::DVBSTP::GetServiceProviderDiscovery(dvb_entry_point_host, dvb_entry_point_port, client_profile, platform_profile);
	if (service_provider_offerings.size() <= 0) {
		XBMC->Log(LOG_NOTICE, "GetServiceProviderDiscovery() returned no results\n");
		return false;
	}

	for(int i = 0 ; i < service_provider_offerings.size() ; i++)
	{
		std::string offering_response;
		std::vector<MovistarTV::DVBSTP::DVBSTPChannel> channels = MovistarTV::DVBSTP:: GetBroadcastDiscovery(service_provider_offerings[i].address, service_provider_offerings[i].port);

		for(int j = 0 ; j < channels.size() ; j++) {
			PVRMovistarChannel stdChannel;

			stdChannel.bRadio = 0;
			stdChannel.iChannelNumber = j + 1;
			stdChannel.iEncryptionSystem = 0;
			stdChannel.iSubChannelNumber = 0;
			stdChannel.iUniqueId = channels[j].serviceName;
			//stdChannel.iUniqueId = j + 1;
			stdChannel.strChannelName = channels[j].name;
			stdChannel.strIconPath = channels[j].logoUri;
			stdChannel.strStreamURL = channels[j].streamUrl;

			XBMC->Log(LOG_NOTICE, "Adding channel '%s' with stream %s ", stdChannel.strChannelName.c_str(), stdChannel.strStreamURL.c_str());
			m_channels.push_back(stdChannel);
		}

		std::string raw_response;
		//MovistarTV::DVBSTP::GetXmlFile(service_provider_offerings[i].address, service_provider_offerings[i].port, 5, raw_response);
		//std::ofstream out("K:\output_05.txt");
		//out << raw_response;
		//out.close();
		
		//XBMC->Log(LOG_NOTICE, "Payload 2: %s\n", offering_response.c_str());
    }
	

	

	/* TODO: Until we finish the code we shall return false */
	//return true;
	return false;
}

int PVRMovistarData::GetChannelsAmount(void)
{
  return m_channels.size();
}

PVR_ERROR PVRMovistarData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
	for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
	{
		PVRMovistarChannel &channel = m_channels.at(iChannelPtr);
		if (channel.bRadio == bRadio)
		{
			PVR_CHANNEL xbmcChannel;
			memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

			xbmcChannel.iUniqueId         = channel.iUniqueId;
			xbmcChannel.bIsRadio          = channel.bRadio;
			xbmcChannel.iChannelNumber    = channel.iChannelNumber;
			xbmcChannel.iSubChannelNumber = channel.iSubChannelNumber;
			strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName) - 1);
			strncpy(xbmcChannel.strStreamURL, channel.strStreamURL.c_str(), sizeof(xbmcChannel.strStreamURL) - 1);
			xbmcChannel.iEncryptionSystem = channel.iEncryptionSystem;
			strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath) - 1);
			xbmcChannel.bIsHidden         = false;

			PVR->TransferChannelEntry(handle, &xbmcChannel);
		}
	}

	return PVR_ERROR_NO_ERROR;
}

bool PVRMovistarData::GetChannel(const PVR_CHANNEL &channel, PVRMovistarChannel &myChannel)
{
	for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
	{
		PVRMovistarChannel &thisChannel = m_channels.at(iChannelPtr);
		if (thisChannel.iUniqueId == (int) channel.iUniqueId)
		{
			myChannel.iUniqueId         = thisChannel.iUniqueId;
			myChannel.bRadio            = thisChannel.bRadio;
			myChannel.iChannelNumber    = thisChannel.iChannelNumber;
			myChannel.iSubChannelNumber = thisChannel.iSubChannelNumber;
			myChannel.iEncryptionSystem = thisChannel.iEncryptionSystem;
			myChannel.strChannelName    = thisChannel.strChannelName;
			myChannel.strIconPath       = thisChannel.strIconPath;
			myChannel.strStreamURL      = thisChannel.strStreamURL;

			return true;
		}
	}

	return false;
}

int PVRMovistarData::GetChannelGroupsAmount(void)
{
  return m_groups.size();
}

PVR_ERROR PVRMovistarData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    PVRMovistarChannelGroup &group = m_groups.at(iGroupPtr);
    if (group.bRadio == bRadio)
    {
      PVR_CHANNEL_GROUP xbmcGroup;
      memset(&xbmcGroup, 0, sizeof(PVR_CHANNEL_GROUP));

      xbmcGroup.bIsRadio = bRadio;
      strncpy(xbmcGroup.strGroupName, group.strGroupName.c_str(), sizeof(xbmcGroup.strGroupName) - 1);

      PVR->TransferChannelGroup(handle, &xbmcGroup);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRMovistarData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    PVRMovistarChannelGroup &myGroup = m_groups.at(iGroupPtr);
    if (!strcmp(myGroup.strGroupName.c_str(),group.strGroupName))
    {
      for (unsigned int iChannelPtr = 0; iChannelPtr < myGroup.members.size(); iChannelPtr++)
      {
        int iId = myGroup.members.at(iChannelPtr) - 1;
        if (iId < 0 || iId > (int)m_channels.size() - 1)
          continue;
        PVRMovistarChannel &channel = m_channels.at(iId);
        PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember;
        memset(&xbmcGroupMember, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

        strncpy(xbmcGroupMember.strGroupName, group.strGroupName, sizeof(xbmcGroupMember.strGroupName) - 1);
        xbmcGroupMember.iChannelUniqueId  = channel.iUniqueId;
        xbmcGroupMember.iChannelNumber    = channel.iChannelNumber;

        PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
      }
    }
  }
 
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRMovistarData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{

  if (m_iEpgStart == -1)
    m_iEpgStart = iStart;

  time_t iLastEndTime = m_iEpgStart + 1;
  int iAddBroadcastId = 0;

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    PVRMovistarChannel &myChannel = m_channels.at(iChannelPtr);
    if (myChannel.iUniqueId != (int) channel.iUniqueId)
      continue;

    while (iLastEndTime < iEnd && myChannel.epg.size() > 0)
    {
      time_t iLastEndTimeTmp = 0;
      for (unsigned int iEntryPtr = 0; iEntryPtr < myChannel.epg.size(); iEntryPtr++)
      {
        PVRMovistarEpgEntry &myTag = myChannel.epg.at(iEntryPtr);

        EPG_TAG tag;
        memset(&tag, 0, sizeof(EPG_TAG));

        tag.iUniqueBroadcastId = myTag.iBroadcastId + iAddBroadcastId;
        tag.strTitle           = myTag.strTitle.c_str();
        tag.iChannelNumber     = myTag.iChannelId;
        tag.startTime          = myTag.startTime + iLastEndTime;
        tag.endTime            = myTag.endTime + iLastEndTime;
        tag.strPlotOutline     = myTag.strPlotOutline.c_str();
        tag.strPlot            = myTag.strPlot.c_str();
        tag.strIconPath        = myTag.strIconPath.c_str();
        tag.iGenreType         = myTag.iGenreType;
        tag.iGenreSubType      = myTag.iGenreSubType;

        iLastEndTimeTmp = tag.endTime;

        PVR->TransferEpgEntry(handle, &tag);
      }

      iLastEndTime = iLastEndTimeTmp;
      iAddBroadcastId += myChannel.epg.size();
    }
  }
  
  return PVR_ERROR_NO_ERROR;
}

int PVRMovistarData::GetRecordingsAmount(void)
{
  return m_recordings.size();
}

PVR_ERROR PVRMovistarData::GetRecordings(ADDON_HANDLE handle)
{
  for (std::vector<PVRMovistarRecording>::iterator it = m_recordings.begin() ; it != m_recordings.end() ; it++)
  {
    PVRMovistarRecording &recording = *it;

    PVR_RECORDING xbmcRecording;

    xbmcRecording.iDuration     = recording.iDuration;
    xbmcRecording.iGenreType    = recording.iGenreType;
    xbmcRecording.iGenreSubType = recording.iGenreSubType;
    xbmcRecording.recordingTime = recording.recordingTime;

    strncpy(xbmcRecording.strChannelName, recording.strChannelName.c_str(), sizeof(xbmcRecording.strChannelName) - 1);
    strncpy(xbmcRecording.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(xbmcRecording.strPlotOutline) - 1);
    strncpy(xbmcRecording.strPlot,        recording.strPlot.c_str(),        sizeof(xbmcRecording.strPlot) - 1);
    strncpy(xbmcRecording.strRecordingId, recording.strRecordingId.c_str(), sizeof(xbmcRecording.strRecordingId) - 1);
    strncpy(xbmcRecording.strTitle,       recording.strTitle.c_str(),       sizeof(xbmcRecording.strTitle) - 1);
    strncpy(xbmcRecording.strStreamURL,   recording.strStreamURL.c_str(),   sizeof(xbmcRecording.strStreamURL) - 1);
    strncpy(xbmcRecording.strDirectory,   recording.strDirectory.c_str(),   sizeof(xbmcRecording.strDirectory) - 1);

    PVR->TransferRecordingEntry(handle, &xbmcRecording);
  }

  return PVR_ERROR_NO_ERROR;
}

int PVRMovistarData::GetTimersAmount(void)
{
  return m_timers.size();
}

PVR_ERROR PVRMovistarData::GetTimers(ADDON_HANDLE handle)
{
  int i = 0;
  for (std::vector<PVRMovistarTimer>::iterator it = m_timers.begin() ; it != m_timers.end() ; it++)
  {
    PVRMovistarTimer &timer = *it;

    PVR_TIMER xbmcTimer;
    memset(&xbmcTimer, 0, sizeof(PVR_TIMER));

    xbmcTimer.iClientIndex      = ++i;
    xbmcTimer.iClientChannelUid = timer.iChannelId;
    xbmcTimer.startTime         = timer.startTime;
    xbmcTimer.endTime           = timer.endTime;
    xbmcTimer.state             = timer.state;

    strncpy(xbmcTimer.strTitle, timer.strTitle.c_str(), sizeof(timer.strTitle) - 1);
    strncpy(xbmcTimer.strSummary, timer.strSummary.c_str(), sizeof(timer.strSummary) - 1);

    PVR->TransferTimerEntry(handle, &xbmcTimer);
  }

  return PVR_ERROR_NO_ERROR;
}