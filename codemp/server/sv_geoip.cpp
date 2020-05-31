#include "server.h"
extern "C" {
	#include "maxminddb.h"
}

namespace GeoIP {
	static MMDB_s *dbHandle = nullptr;

	void Init(void) {
		if (dbHandle)
			return; // already initialized

		dbHandle = new MMDB_s{};

		static const char *filename = "./GeoLite2-Country.mmdb";
		int status = MMDB_open(filename, MMDB_MODE_MMAP, dbHandle);
		if (status == MMDB_SUCCESS) {
			Com_Printf("Successfully loaded GeoIP database.\n");
		}
		else {
			Com_Printf("Error initializing GeoIP database %s: %s\n", filename, MMDB_strerror(status));
			delete dbHandle;
			dbHandle = nullptr;
		}
	}

	void Shutdown(void) {
		if (!dbHandle)
			return; // not initialized

		MMDB_close(dbHandle);
		delete dbHandle;
		dbHandle = nullptr;
		Com_Printf("GeoIP database unloaded.\n");
	}

	void GetCountry(const char *ipStr, char *outBuf, int outBufSize) {
		if (!outBuf || outBufSize <= 0) {
			assert(false);
			return;
		}

		*outBuf = '\0';

		if (!sv_countryDetection->integer || !VALIDSTRING(ipStr) || !isdigit((unsigned int)*ipStr))
			return;

		if (!dbHandle) {
			// not already initialized; could happen if the cvar was changed mid-game
			Init();
			if (!dbHandle)
				return;
		}

		// chop off port
		std::string filtered = std::string(ipStr);
		filtered = filtered.substr(0, filtered.find(":", 0));
		ipStr = filtered.c_str();

		// look up the ip
		int error = -1, gai_error = -1;
		MMDB_lookup_result_s result = MMDB_lookup_string(dbHandle, ipStr, &gai_error, &error);
		if (error != MMDB_SUCCESS || gai_error != 0) {
			Com_DPrintf("GeoIP lookup error for %s: %s\n", ipStr, MMDB_strerror(error));
			return;
		}
		if (!result.found_entry) {
			Com_DPrintf("GeoIP found no country for %s.\n", ipStr);
			return;
		}

		// convert it to a country name
		MMDB_entry_s entry = result.entry;
		MMDB_entry_data_s entry_data;
		if ((error = MMDB_get_value(&entry, &entry_data, "country", "names", "en", nullptr)) != MMDB_SUCCESS) {
			Com_DPrintf("GeoIP get_value error for %s: %s\n", ipStr, MMDB_strerror(error));
			return;
		}
		if (!entry_data.has_data) {
			Com_DPrintf("GeoIP found no country name for %s.\n", ipStr);
			return;
		}

		// write to the supplied buffer
		std::string countryStr = entry_data.utf8_string;
		countryStr.resize(entry_data.data_size);
		Q_strncpyz(outBuf, countryStr.c_str(), outBufSize);
	}
}