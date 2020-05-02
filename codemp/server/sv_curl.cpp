// sv_curl.cpp -- server code for libcurl based operations

#include "server.h"
#include "sv_gameapi.h"

#include <vector>
#include <chrono>
#include <curl/curl.h>

// http useragent used to identify the client
#define HTTP_USERAGENT				"openjkded/1.0"

// hard limits on the transfers
#define MAX_PARALLEL_CONNECTIONS	5L		// max active handles in the multi, the rest is dormant
#define MAX_HOST_CONNECTIONS		2L		// max handlers for a given host so we don't DoS them
#define CONNECT_TIMEOUT_SEC			60L		// timeout in seconds for the connection phase alone
#define UPLOAD_RATE_LIMIT			1000000	// transfer upload rate limit in bytes/second
#define DOWNLOAD_RATE_LIMIT			1000000	// transfer download rate limit in bytes/second
//CURLOPT_TIMEOUT TODO: this timeout is bad to use in a multi, no total timeout for now
//CURLOPT_LOW_SPEED_LIMIT: TODO: a companion to implement with the timeout as well
//CURLOPT_MAXFILESIZE: if file download is implemented (not always transmitted)

// global pointer to the multi interface, null if uninitialized
static CURLM* globalmcurl = nullptr;

// used to keep track of how many easy handles are alive in the multi
static int numActiveTransfers = 0;

// set to true when want to abort all running handles in the multi
static bool abortAll = false;

class CurlTransfer {
	// The base class that represents a transfer and which can be extended to customize
	// its behavior, such as a read/write backend, callbacks...
	// This is a wrapper around the easy handle, and its purpose is to be stored in private
	// handle info to be retrieved later without keeping a list of all running transfers.
	// The class manages the easy handle lifecycle entirely except its allocation and cleanup
	// which must be done outside of the class.

private:
	static trsfHandle_t nextHandle;		// incremented every time a handle is allocated

private:
	trsfHandle_t		handle;			// used in callbacks to identify transfer results
	CURL*				curl;			// the curl easy handle of this transfer
	bool				active;			// qtrue if the easy handle is currently attached to the multi handle

	struct curl_slist*	httpHeaders;	// pointer to the block of memory of custom http headers, if used
	struct curl_httppost* postForm;		// pointer to the block of memory of multipart form data, if used

	std::vector<void*>	cache;			// all the other cached data that libcurl can't store for us

protected:
	TrsfResultCallback	callback;		// the callback to send the finalized data to
	void*				userdata;		// a user defined pointer to pass to callback

public:
	CurlTransfer(CURL* curl, const char* url) {
		curl_easy_reset(curl);

		this->handle = nextHandle++;
		this->curl = curl;
		this->active = qfalse;

		this->httpHeaders = nullptr;
		this->postForm = nullptr;

		this->callback = nullptr;
		this->userdata = nullptr;

		SetOpt(CURLOPT_URL, url);

#if 0
		SetOpt(CURLOPT_VERBOSE, 1L);
#endif

		// this trick will help us retrieve this class easily later
		SetOpt(CURLOPT_PRIVATE, this);

		// write callback, no-op by default
		SetOpt(CURLOPT_WRITEFUNCTION, NullWriteFunction);
		SetOpt(CURLOPT_WRITEDATA, NULL);

		// progress callback
		SetOpt(CURLOPT_NOPROGRESS, 0L);
		SetOpt(CURLOPT_XFERINFOFUNCTION, AbortProgressCallback);
		SetOpt(CURLOPT_XFERINFODATA, &abortAll);

		// hard limits
		SetOpt(CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT_SEC);
		SetOpt(CURLOPT_MAX_SEND_SPEED_LARGE, (curl_off_t)UPLOAD_RATE_LIMIT);
		SetOpt(CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)DOWNLOAD_RATE_LIMIT);
	}

	virtual ~CurlTransfer() {
		if (httpHeaders) {
			curl_slist_free_all(httpHeaders);
		}

		if (postForm) {
			curl_formfree(postForm);
		}

		// free everything cached
		for (std::vector<void*>::iterator it = cache.begin(); it != cache.end(); ++it) {
			free(*it);
		}
	}

	trsfHandle_t GetHandle() {
		return handle;
	}

	CURL* GetCurl() {
		return curl;
	}

	bool IsActive() {
		return active;
	}

	void SetResultCallback(TrsfResultCallback callback) {
		this->callback = callback;
	}

	void SetUserdata(void* userdata) {
		this->userdata = userdata;
	}

	void InvokeCallback(trsfErrorInfo_t* errorInfo) {
		if (callback) {
			// we might have got a protocol response code, so retrieve it
			int responseCode = 0;
			GetInfo(CURLINFO_RESPONSE_CODE, &responseCode);

			Internal_InvokeCallback(handle, errorInfo, responseCode);
		}
	}

	template<typename... Args>
	CURLcode SetOpt(CURLoption option, Args... args) {
		return curl_easy_setopt(curl, option, args...);
	}

	template<typename... Args>
	CURLcode GetInfo(CURLINFO info, Args... args) {
		return curl_easy_getinfo(curl, info, args...);
	}

	CURLcode SetupHTTP(const char* accept, const char* contentType) {
		SetOpt(CURLOPT_FOLLOWLOCATION, 1L);
		SetOpt(CURLOPT_USERAGENT, HTTP_USERAGENT);

		// for specific header information, we have to build a linked list and manage
		// our own memory, which is why we store it inside the curl transfer struct

		if (VALIDSTRING(accept)) {
			httpHeaders = curl_slist_append(httpHeaders, va("Accept: %s", accept));
		}

		if (VALIDSTRING(contentType)) {
			httpHeaders = curl_slist_append(httpHeaders, va("Content-Type: %s", contentType));
		}

		if (httpHeaders) {
			return SetOpt(CURLOPT_HTTPHEADER, httpHeaders);
		}

		return CURLE_OK;
	}

	CURLcode SetupMultipart(const trsfFormPart_t* multiPart, size_t numParts) {
		struct curl_httppost* last = nullptr;

		int i;
		for (i = 0; i < numParts; ++i) {
			const trsfFormPart_t* part = &multiPart[i];

			if (!part->isFile) {
				if (!part->fromDisk) {
					// let libcurl copy the buffer so we don't need to keep it around
					curl_formadd(&postForm, &last,
						CURLFORM_COPYNAME, part->partName,
						CURLFORM_CONTENTSLENGTH, part->bufSize,
						CURLFORM_COPYCONTENTS, part->buf,
						CURLFORM_END
					);
				} else {
					// not implemented yet
				}
			} else {
				if (!part->fromDisk) {
					// copy the buffer ourselves since in this case it's not supported by libcurl
					void* cachedData = malloc(part->bufSize);
					memcpy(cachedData, part->buf, part->bufSize);
					cache.push_back(cachedData);

					curl_formadd(&postForm, &last,
						CURLFORM_COPYNAME, part->partName,
						CURLFORM_BUFFER, part->filename,
						CURLFORM_BUFFERLENGTH, part->bufSize,
						CURLFORM_BUFFERPTR, cachedData,
						CURLFORM_END
					);
				} else {
					// not implemented yet
				}
			}
		}

		return SetOpt(CURLOPT_HTTPPOST, postForm);
	}

	bool Attach(CURLM* mcurl) {
		CURLMcode mc = curl_multi_add_handle(mcurl, curl);

		if (mc != CURLM_OK) {
			Com_Printf("(CurlTransfer::Attach) curl_multi_add_handle: %s\n", curl_multi_strerror(mc));
			return false;
		}

		active = qtrue;

		return qtrue;
	}

	void Detach(CURLM* mcurl) {
		CURLMcode mc = curl_multi_remove_handle(mcurl, curl);

		if (mc != CURLM_OK) {
			Com_Printf("(CurlTransfer::Detach) curl_multi_remove_handle: %s\n", curl_multi_strerror(mc));
		}

		active = qfalse;
	}

protected:
	virtual void Internal_InvokeCallback(trsfHandle_t handle, trsfErrorInfo_t* errorInfo, int responseCode) {
		// in the default callback, we don't have any data to pass along with the result
		callback(handle, errorInfo, responseCode, NULL, 0, userdata);
	}

private:
	static size_t NullWriteFunction(void* contents, size_t size, size_t nmemb, void* userp) {
		// the default function doesn't do anything
		return size * nmemb;
	}

	static int AbortProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
		// the only purpose of this callback is to abort if told to
		// clientp contains a pointer to a bool that is true if we should abort
		// TODO: we can also implement proper timeouts here
		if (clientp && *((bool*)clientp)) {
			return 1;
		}

		return 0;
	}

};

trsfHandle_t CurlTransfer::nextHandle = 0;

class CurlBufTransfer : public CurlTransfer {
	// This class keeps a write buffer fully in memory that automatically resizes itself to hold
	// the curl response, so it may not be appropriate for large file transfers. Upon transfer
	// completion, the buffer is sent to the callback along with the usual data

private:
	std::vector<char>	writebuf; // response buffer
	qboolean			nullTerminate; // whether or not to append a null terminator

public:
	CurlBufTransfer(CURL* curl, const char* url, TrsfResultCallback callback) : CurlTransfer(curl, url) {
		this->writebuf = std::vector<char>();
		this->nullTerminate = qfalse;

		// override the write functions with the vector buffer ones
		SetOpt(CURLOPT_WRITEFUNCTION, VectorBufferWriteFunction);
		SetOpt(CURLOPT_WRITEDATA, &this->writebuf);

		// callback is NOT optional for this, so enforce it in the constructor
		SetResultCallback(callback);
	}

	void SetNullTerminate(qboolean nullTerminate) {
		this->nullTerminate = nullTerminate;
	}

protected:
	void Internal_InvokeCallback(trsfHandle_t handle, trsfErrorInfo_t* errorInfo, int responseCode) override {
		// here, we send the address of the buffer and its size, or null if there is no data

		size_t size = writebuf.size();

		if (nullTerminate) {
			writebuf.push_back('\0');
			++size;
		}

		char* bufPtr = size > 0 ? &writebuf[0] : NULL;

		callback(handle, errorInfo, responseCode, bufPtr, size, userdata);

		if (nullTerminate) {
			writebuf.pop_back();
		}
	}

private:
	static size_t VectorBufferWriteFunction(void* contents, size_t size, size_t nmemb, void* userp) {
		std::vector<char>* writebuf = reinterpret_cast<std::vector<char>*>(userp);
		size_t realsize = size * nmemb;

		if (realsize) {
			char* buf = (char*)contents;
			writebuf->insert(writebuf->end(), buf, buf + realsize);
		}

		return realsize;
	}

};

static CURL* CreateEasyHandle() {
	CURL* curl = curl_easy_init();

	if (!curl) {
		Com_Printf("(CreateEasyHandle) curl_easy_init failed\n");
		return nullptr;
	}

	return curl;
}

static void CleanupEasyHandle(CURL* curl) {
	curl_easy_cleanup(curl);
}

/*
==================
SV_CurlInit

Initializes libcurl globally and the transfer system
==================
*/
void SV_CurlInit() {
	if (globalmcurl) {
		return;
	}

	CURLcode c;

	// initialize libcurl globally
	c = curl_global_init(CURL_GLOBAL_DEFAULT);
	
	if (c != CURLE_OK) {
		Com_Printf("(SV_CurlInit) curl_global_init: %s\n", curl_easy_strerror(c));
		return;
	}

	// get a multi handle for all future non-blocking operations
	globalmcurl = curl_multi_init();

	if (!globalmcurl) {
		curl_global_cleanup();
		Com_Printf("(SV_CurlInit) curl_multi_init failed\n");
		return;
	}

	// setup some multi options
	curl_multi_setopt(globalmcurl, CURLMOPT_MAX_TOTAL_CONNECTIONS, MAX_PARALLEL_CONNECTIONS);
	curl_multi_setopt(globalmcurl, CURLMOPT_MAX_HOST_CONNECTIONS, MAX_HOST_CONNECTIONS);

	// print the version string since the dynamic library could be replaced
	curl_version_info_data* data = curl_version_info(CURLVERSION_NOW);
	Com_Printf("libcurl version: %s\n", data->version);
}

/*
==================
SV_CurlShutdown

Proper libcurl cleanup that takes care of aborting transfers still in progress
==================
*/
void SV_CurlShutdown() {
	if (globalmcurl) {
		CURLMcode mc;

		// abort and cleanup all the remaining transfers

		int stillRunning;
		abortAll = true; // this will cause all handles to fail with CURLE_ABORTED_BY_CALLBACK

		// a bug in some versions of libcurl can cause an infinite loop if using CURLMOPT_MAX_HOST_CONNECTIONS
		// i haven't seen the bug happen, but make sure we can't get stuck in a loop here
		std::chrono::steady_clock::time_point clockBegin = std::chrono::steady_clock::now();

		do {
			mc = curl_multi_perform(globalmcurl, &stillRunning);

			if (mc != CURLM_OK) {
				Com_Printf("(SV_CurlShutdown) curl_multi_perform: %s\n", curl_multi_strerror(mc));
				break;
			}

			// wait at most a second for something to happen at shutdown
			mc = curl_multi_wait(globalmcurl, NULL, 0, 1000, NULL);

			if (mc != CURLM_OK) {
				Com_Printf("(SV_CurlShutdown) curl_multi_wait: %s\n", curl_multi_strerror(mc));
				break;
			}

			CURLMsg* msg = nullptr;
			int msgsLeft;
			while ((msg = curl_multi_info_read(globalmcurl, &msgsLeft))) {
				if (msg->msg == CURLMSG_DONE) {
					// this transfer was aborted successfully, free memory without doing any callback
					CURL* curl = msg->easy_handle;
					CurlTransfer* transfer = nullptr;
					curl_easy_getinfo(curl, CURLINFO_PRIVATE, &transfer);

					if (transfer) {
						transfer->Detach(globalmcurl);
						Com_Printf("Transfer handle %d cancelled\n", transfer->GetHandle());
						delete transfer;
					}

					curl_easy_cleanup(curl);
				}
			}

			// this should never happened unless using a bugged libcurl, but in any case don't let this last
			// infinitely (more than 10 secs). This would cause memory leaks on application shutdown
			std::chrono::steady_clock::time_point clockNow = std::chrono::steady_clock::now();
			int secondsElapsed = std::chrono::duration_cast<std::chrono::seconds>(clockNow - clockBegin).count();
			if (secondsElapsed > 10) {
				Com_Printf("(SV_CurlShutdown) cleanup took too long, exiting\n");
				break;
			}

		} while (stillRunning > 0);

		// cleanup the multi handle
		mc = curl_multi_cleanup(globalmcurl);

		if (mc != CURLM_OK) {
			Com_Printf("(SV_CurlShutdown) curl_multi_cleanup: %s\n", curl_multi_strerror(mc));
		}

		// reset globals for re-usage
		globalmcurl = nullptr;
		numActiveTransfers = 0;
		abortAll = false;

		// global libcurl cleanup
		curl_global_cleanup();
	}
}

/*
==================
SV_RunTransfers

Performs as much work as possible on queued transfers in a non blocking way.
Triggers callbacks and cleanups finished transfers.
This is meant to be called every frame.
==================
*/
void SV_RunTransfers() {
	if (numActiveTransfers > 0) {
		CURLMcode mc;

		int stillRunning;
		mc = curl_multi_perform(globalmcurl, &stillRunning);

		if (mc > CURLM_OK) {
			Com_Printf("(SV_RunCurlTransfers) curl_multi_perform: %s\n", curl_multi_strerror(mc));
			return;
		}

		if (stillRunning < numActiveTransfers) {
			// at least one or more transfers less running, read all the pending messages
			numActiveTransfers = stillRunning;

			CURLMsg* msg = nullptr;
			int msgsLeft;
			while ((msg = curl_multi_info_read(globalmcurl, &msgsLeft))) {
				// right now only this value is possible, so add a warning for future proofing
				if (msg->msg != CURLMSG_DONE) {
					Com_Printf("(SV_RunCurlTransfers) unknown message number %d\n", msg->msg);
					continue;
				}

				CURL* curl = msg->easy_handle;

				// retrieve the transfer object we stored earlier
				CurlTransfer* transfer = nullptr;
				curl_easy_getinfo(curl, CURLINFO_PRIVATE, &transfer);

				if (transfer) {
					// info about the error if one occured (in which case code > 0)
					trsfErrorInfo_t errorInfo;
					errorInfo.code = msg->data.result; // the curl_easy_perform return code that the multi got
					errorInfo.desc = curl_easy_strerror(msg->data.result); // this memory is safe to pass around

					transfer->InvokeCallback(&errorInfo);
					transfer->Detach(globalmcurl);

					double totalTimeSeconds;
					transfer->GetInfo(CURLINFO_TOTAL_TIME, &totalTimeSeconds);
					if (!errorInfo.code) {
						Com_Printf("Transfer handle %d completed in %.3f seconds\n", transfer->GetHandle(), totalTimeSeconds);
					} else {
						Com_Printf("Transfer handle %d failed after %.3f seconds\n", transfer->GetHandle(), totalTimeSeconds);
					}

					delete transfer; // this will invalidate any memory that was passed to the callbacks
				} else {
					// this should never happen, but if it did it would likely mean memory leaks, so log it
					Com_Printf("(SV_RunCurlTransfers) easy handle has no transfer pointer\n");
				}

				// in any case, we MUST cleanup the handle since the transfer wrapper leaves us that memory to manage
				curl_easy_cleanup(curl);
			}
		}
	}
}

/*
==================
SV_SendGETRequest

Queues a HTTP GET request.
* If handle is not NULL, it will be set to the handle of the transfer (used in callbacks to tell
  which one has just completed)
* url must be specified
* resultCallback is mandatory for GET requests
* userdata is optional and is the pointer passed to resultCallback
* headerAccept is optional, and if it's a valid string, it will set the Accept: header
* headerContentType is optional, and if it's a valid string, it will set the Content-Type: header
* nullTerminate is true by default, and if set it will add a null terminator to the response buffer
  sent to resultCallback. Unset if you don't expect a string response
==================
*/
qboolean SV_SendGETRequest(trsfHandle_t* handle,
	const char* url,
	TrsfResultCallback resultCallback,
	void* userdata,
	const char* headerAccept,
	const char* headerContentType,
	qboolean nullTerminate)
{
	if (!globalmcurl) {
		return qfalse;
	}

	CURL* curl = CreateEasyHandle();

	if (!curl) {
		return qfalse;
	}

	CurlBufTransfer* transfer = new CurlBufTransfer(curl, url, resultCallback);

	if (handle) {
		*handle = transfer->GetHandle();
	}

	transfer->SetUserdata(userdata);
	transfer->SetNullTerminate(nullTerminate);
	transfer->SetupHTTP(headerAccept, headerContentType);
	// a transfer is already a GET by default
	
	if (!transfer->Attach(globalmcurl)) {
		delete transfer;
		CleanupEasyHandle(curl);

		return qfalse;
	}

	Com_Printf("GET request handle %d queued [%s]\n", transfer->GetHandle(), url);

	++numActiveTransfers;

	return qtrue;
}

/*
==================
SV_SendPOSTRequest

Queues a HTTP POST request.
* If handle is not NULL, it will be set to the handle of the transfer (used in callbacks to tell
  which one has just completed)
* url must be specified
* data is the null terminated string of the POST data and must be specified
* resultCallback is optional for POST requests, and if unset no callback will be invoked
* userdata is optional and is the pointer passed to resultCallback
* headerAccept is optional, and if it's a valid string, it will set the Accept: header
* headerContentType is optional, and if it's a valid string, it will set the Content-Type: header
* nullTerminate is true by default, and if set it will add a null terminator to the response buffer
  sent to resultCallback. Unset if you don't expect a string response
==================
*/
qboolean SV_SendPOSTRequest(trsfHandle_t* handle,
	const char* url,
	const char* data,
	TrsfResultCallback resultCallback,
	void* userdata,
	const char* headerAccept,
	const char* headerContentType,
	qboolean nullTerminate)
{
	if (!globalmcurl) {
		return qfalse;
	}

	CURL* curl = CreateEasyHandle();

	if (!curl) {
		return qfalse;
	}

	CurlTransfer* transfer;

	// POST requests don't have to specify a callback, in which case the result is just ignored
	if (resultCallback) {
		transfer = new CurlBufTransfer(curl, url, resultCallback);
		static_cast<CurlBufTransfer*>(transfer)->SetNullTerminate(nullTerminate);
	} else {
		transfer = new CurlTransfer(curl, url);
	}

	if (handle) {
		*handle = transfer->GetHandle();
	}

	transfer->SetUserdata(userdata);
	transfer->SetupHTTP(headerAccept, headerContentType);
	// post specific stuff
	transfer->SetOpt(CURLOPT_POST, 1L);
	transfer->SetOpt(CURLOPT_COPYPOSTFIELDS, data);

	if (!transfer->Attach(globalmcurl)) {
		delete transfer;
		CleanupEasyHandle(curl);

		return qfalse;
	}

	Com_Printf("POST request handle %d queued [%s]\n", transfer->GetHandle(), url);

	++numActiveTransfers;

	return qtrue;
}

qboolean SV_SendMultipartPOSTRequest(trsfHandle_t* handle,
	const char* url,
	trsfFormPart_t* multiPart,
	size_t numParts,
	TrsfResultCallback resultCallback,
	void* userdata,
	const char* headerAccept,
	const char* headerContentType,
	qboolean nullTerminate)
{
	if (!globalmcurl) {
		return qfalse;
	}

	CURL* curl = CreateEasyHandle();

	if (!curl) {
		return qfalse;
	}

	CurlTransfer* transfer;

	// POST requests don't have to specify a callback, in which case the result is just ignored
	if (resultCallback) {
		transfer = new CurlBufTransfer(curl, url, resultCallback);
		static_cast<CurlBufTransfer*>(transfer)->SetNullTerminate(nullTerminate);
	} else {
		transfer = new CurlTransfer(curl, url);
	}

	transfer->SetUserdata(userdata);
	transfer->SetupHTTP(headerAccept, headerContentType);
	// multipart post specific stuff
	transfer->SetupMultipart(multiPart, numParts);

	if (!transfer->Attach(globalmcurl)) {
		delete transfer;
		CleanupEasyHandle(curl);

		return qfalse;
	}

	Com_Printf("POST multipart request handle %d queued [%s]\n", transfer->GetHandle(), url);

	++numActiveTransfers;

	return qtrue;
}

// This callback is common to all VM API calls and is where trap calls are routed from.
// The reason for this is that we don't want to store the address of a module that can
// be unloaded dynamically (unless we invalidate everything on VM reload...)
// VMs must use the API knowing that if they get shut down prematurely or if the transfer
// takes excessively long, they might never get to know the result.
// VMs can't pass userdata either for the same reasons.
static void VMTransferResultCallback(trsfHandle_t handle, trsfErrorInfo_t* errorInfo, int responseCode, void* data, size_t size, void* userdata) {
	GVM_TransferResult(handle, errorInfo, responseCode, data, size);
}

/*
==================
SV_VM_SendGETRequest

Simplified SV_SendGETRequest wrapper for the VM. Because of dynamic addresses, the callback
is always the GVM_TransferResult trap call and it cannot be passed userdata. The received
data is also always null terminated.
* If handle is not NULL, it will be set to the handle of the transfer (used in callbacks to tell
  which one has just completed)
* url must be specified
* headerAccept is optional, and if it's a valid string, it will set the Accept: header
* headerContentType is optional, and if it's a valid string, it will set the Content-Type: header
==================
*/
qboolean SV_VM_SendGETRequest(trsfHandle_t* handle,
	const char* url,
	const char* headerAccept,
	const char* headerContentType)
{
	return SV_SendGETRequest(handle, url, VMTransferResultCallback, nullptr,
		headerAccept, headerContentType, qtrue);
}

/*
==================
SV_VM_SendPOSTRequest

Simplified SV_SendPOSTRequest wrapper for the VM. Because of dynamic addresses, the callback
is always the GVM_TransferResult trap call and it cannot be passed userdata. The received
data is also always null terminated.
* If handle is not NULL, it will be set to the handle of the transfer (used in callbacks to tell
  which one has just completed)
* url must be specified
* data is the null terminated string of the POST data and must be specified
* headerAccept is optional, and if it's a valid string, it will set the Accept: header
* headerContentType is optional, and if it's a valid string, it will set the Content-Type: header
* if receiveResult is unset, the VM callback will never be called.
==================
*/
qboolean SV_VM_SendPOSTRequest(trsfHandle_t* handle,
	const char* url,
	const char* data,
	const char* headerAccept,
	const char* headerContentType,
	qboolean receiveResult)
{
	return SV_SendPOSTRequest(handle, url, data, receiveResult ? VMTransferResultCallback : nullptr, nullptr,
		headerAccept, headerContentType, qtrue);
}

/*
==================
SV_VM_SendMultipartPOSTRequest

Simplified SV_SendMultipartPOSTRequest wrapper for the VM. Because of dynamic addresses, the callback
is always the GVM_TransferResult trap call and it cannot be passed userdata. The received
data is also always null terminated.
* If handle is not NULL, it will be set to the handle of the transfer (used in callbacks to tell
  which one has just completed)
* url must be specified
* multiPart is a pointer to an array of form part descriptors
* numParts is the number of elements in the multiPart array
* headerAccept is optional, and if it's a valid string, it will set the Accept: header
* headerContentType is optional, and if it's a valid string, it will set the Content-Type: header
* if receiveResult is unset, the VM callback will never be called.
==================
*/
qboolean SV_VM_SendMultipartPOSTRequest(trsfHandle_t* handle,
	const char* url,
	trsfFormPart_t* multiPart,
	size_t numParts,
	const char* headerAccept,
	const char* headerContentType,
	qboolean receiveResult)
{
	return SV_SendMultipartPOSTRequest(handle, url, multiPart, numParts, receiveResult ? VMTransferResultCallback : nullptr, nullptr,
		headerAccept, headerContentType, qtrue);
}
