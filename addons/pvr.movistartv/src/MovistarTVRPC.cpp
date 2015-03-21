
#include "client.h"
#include "MovistarTVRPC.h"

using namespace ADDON;

namespace MovistarTV {

	int MovistarTVRPC(const std::string& url, std::string& json_response)
	{
		int retval = E_FAILED;

		XBMC->Log(LOG_DEBUG, "Fetching URL: %s\n", url.c_str());
		void* hFile = XBMC->OpenFileForWrite(url.c_str(), 0);
		if (hFile != NULL) {
			int rc = XBMC->WriteFile(hFile, "", 0);
			if (rc >= 0) {
				std::string result;
				result.clear();
				char buffer[1024];
				while (XBMC->ReadFileString(hFile, buffer, 1023))
					result.append(buffer);
				json_response = result;
				retval = E_SUCCESS;
			} else {
				XBMC->Log(LOG_ERROR, "can not write to %s", url.c_str());
			}
			XBMC->CloseFile(hFile);
		} else {
			XBMC->Log(LOG_ERROR, "can not open %s for write", url.c_str());
		}
		return retval;
	}

	int MovistarTVJSONRPC(const std::string& url, Json::Value& json_response)
	{
		std::string response;
		int retval = E_FAILED;
		retval = MovistarTVRPC(url, response);

		if (retval != E_FAILED) {
			// Print only the first 512 bytes, otherwise XBMC will crash...
			XBMC->Log(LOG_DEBUG, "Response: %s\n", response.substr(0,512).c_str());

			if (response.length() != 0) {
				Json::Reader reader;

				bool parsingSuccessful = reader.parse(response, json_response);

				if ( !parsingSuccessful ) {
					XBMC->Log(LOG_DEBUG, "Failed to parse %s: \n%s\n", response.c_str(), reader.getFormatedErrorMessages().c_str() );
					return E_FAILED;
				}

				if( json_response.type() != Json::objectValue) {
					XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
					return E_FAILED;
				}

				Json::Value result_code = json_response.get("resultCode", -1);
				if (result_code.type() != Json::intValue) {
					XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::intValue\n");
					return E_FAILED;
				}

				if (result_code.asInt() != 0) {
					Json::Value result_text = json_response.get("resultText", "Unknown error");
					if (result_code.type() != Json::stringValue) {
						XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::stringValue\n");
						return E_FAILED;
					}

					XBMC->Log(LOG_DEBUG, "%s\n", result_text.asString().c_str());
					return E_FAILED;
				}

				// TODO: Check the hashCode

				// Get result data and return that
				Json::Value result_data = json_response.get("resultData", NULL);
				if( result_data.type() != Json::objectValue) {
					XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
					return E_FAILED;
				}
				json_response = result_data;
			} else {
				XBMC->Log(LOG_DEBUG, "Empty response");
				return E_EMPTYRESPONSE;
			}
		}

		return retval;
	}

	int GetClientProfile(ClientProfile &profile)
	{
		Json::Value json_response;
		Json::Value t_value;
		int retval = E_FAIL;

		retval = MovistarTVJSONRPC("http://172.26.22.23:2001/appserver/mvtv.do?action=getClientProfile", json_response);
		if (retval != E_SUCCESS) {
			return retval;
		}

		/* Version */
		t_value = json_response.get("version", NULL);
		if (t_value.type() != Json::intValue) {
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::intValue\n");
			return E_FAILED;
		}
		profile.version = t_value.asInt();

		/* Client version */
		t_value = json_response.get("clientVersion", NULL);
		if (t_value.type() != Json::stringValue) {
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::stringValue\n");
			return E_FAILED;
		}
		profile.clientVersion = t_value.asString();

		/* Demarcation */
		t_value = json_response.get("demarcation", NULL);
		if (t_value.type() != Json::intValue) {
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::intValue\n");
			return E_FAILED;
		}
		profile.demarcation = t_value.asInt();

		/* Return */
		return E_SUCCESS;
	}

	int GetPlatformProfile(PlatformProfile &profile)
	{
		Json::Value json_response;
		Json::Value t_value;
		Json::Value tt_value;
		int retval = E_FAIL;

		retval = MovistarTVJSONRPC("http://172.26.22.23:2001/appserver/mvtv.do?action=getPlatformProfile", json_response);
		if (retval != E_SUCCESS) {
			return retval;
		}

		/* DVB Config */
		t_value = json_response.get("dvbConfig", NULL);
		if (t_value.type() != Json::objectValue) {
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
			return E_FAILED;
		}

		tt_value = t_value.get("dvbServiceProvider", "imagenio.es");
		if (tt_value.type() != Json::stringValue) {
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::stringValue\n");
			return E_FAILED;
		}
		profile.dvbConfig.dvbServiceProvider = tt_value.asString();

		tt_value = t_value.get("dvbEntryPoint", NULL);
		if (tt_value.type() != Json::stringValue) {
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::stringValue\n");
			return E_FAILED;
		}
		profile.dvbConfig.dvbEntryPoint = tt_value.asString();

		/* Client version */
		t_value = json_response.get("clientVersion", NULL);
		if (t_value.type() != Json::stringValue) {
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::stringValue\n");
			return E_FAILED;
		}
		profile.clientVersion = t_value.asString();

		/* Return */
		return E_SUCCESS;
	}

}