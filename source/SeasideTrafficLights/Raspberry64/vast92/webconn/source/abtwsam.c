/*------------------------------------------------------------------
 * abtwsam.c : VisualAge for Smalltalk, Web Connection,
 *             Web Server Interface, MS IIS Adapter
 *             (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#if defined(MEM_DEBUG)
#pragma strings(readonly)
#endif

/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#include "abtwsi.h"   /* Common include                             */
#include <httpext.h>  /* ISAPI include                              */
#include <sys/stat.h>

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Global Config pointer
 *------------------------------------------------------------------*/
AbtWsiConfig *Config = NULL;

/*------------------------------------------------------------------
 * Variable Strings
 *------------------------------------------------------------------*/
char * varStrings[] = {
          "AUTH_TYPE",
		  "AUTH_USER",
		  "LOCAL_ADDR",
		  "LOGON_USER",
          "REMOTE_ADDR",
          "REMOTE_HOST",
          "REMOTE_USER",
          "UNMAPPED_REMOTE_USER",
          "SCRIPT_NAME",
          "SERVER_NAME",
          "SERVER_PORT",
          "SERVER_PORT_SECURE",
          "SERVER_PROTOCOL",
          "SERVER_SOFTWARE",
          "HTTP_ACCEPT",

          "\0"};    /* NOTE: This last element is used to denote the end of the list */

/*------------------------------------------------------------------
 * External entry points
 *------------------------------------------------------------------*/
BOOL WINAPI DllMain(HINSTANCE module, DWORD flag, LPVOID dummy);

/*------------------------------------------------------------------
 * Internal function prototypes
 *------------------------------------------------------------------*/
BOOL   _AbtWsi_Linkage_ initConfig();
int    _AbtWsi_Linkage_ handleRequest(AbtWsiTransaction *trans);
void   _AbtWsi_Linkage_ handleError(AbtWsiTransaction *trans);
void   _AbtWsi_Linkage_ sendHeaders(AbtWsiTransaction *trans);
int    _AbtWsi_Linkage_ getProperties(AbtWsiTransaction *trans);
int    _AbtWsi_Linkage_ getTrans(AbtWsiTransaction **trans, EXTENSION_CONTROL_BLOCK *ecb);
FILE * _AbtWsi_Linkage_ openLogFile(void);

/*------------------------------------------------------------------
 * Helper macro
 *------------------------------------------------------------------*/
#define ECB  ((EXTENSION_CONTROL_BLOCK *)trans->adapterData)

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Entrypoint for ISAPI server DLL initialization and termination
 *------------------------------------------------------------------*/
BOOL WINAPI DllMain(HINSTANCE module, DWORD flag, LPVOID dummy)
{
	switch (flag)
	{
	case DLL_PROCESS_DETACH:
		{
#if defined(MEM_DEBUG)
			{
				int fh; 
				char buff1[] = "\n#######  FINAL MEMORY CHECK - SHOULD BE NONE !!!  ##########\n\n";
				char buff2[] = "\n#################\n";
				FILE *filePtr = freopen("C:\\DEBUG.MEM", "w+", stderr);

				if (filePtr)
				{
					fh = fileno(filePtr);
					_set_crt_msg_handle(fh);    
					write(fh, buff1, strlen(buff1));
					_dump_allocated(200);
					write(fh, buff2, strlen(buff2));
					close(fh);
				}
			}
#endif
			break;
		}

	default:
		break;
	}
	return TRUE;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * First ISAPI entrypoint for registration and resource allocation
 *------------------------------------------------------------------*/
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *ver)
{
	ver->dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

	sprintf(ver->lpszExtensionDesc,"%s, %s  %d.%d", PROGRAM_LONGNAME,
		MSG_STRING_VERSION, WSI_MAJOR_VERSION, WSI_MINOR_VERSION);

	if(!initConfig()) {
		return FALSE;
	}

	return TRUE;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Main ISAPI entrypoint
 *------------------------------------------------------------------*/
DWORD WINAPI HttpExtensionProc(EXTENSION_CONTROL_BLOCK *ecb)
{
	AbtWsiTransaction *trans;
	DWORD ret = HSE_STATUS_SUCCESS;

	if ( (getTrans(&trans, ecb)) || (handleRequest(trans)) )
		ret = HSE_STATUS_ERROR;

	AbtWscFreeTransaction(trans);

	return ret;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Unload ISAPI DLL entrypoint.  Free resources
 *------------------------------------------------------------------*/
BOOL WINAPI TerminateExtension(DWORD dwFlags) {
	AbtWscEnd(NULL);
	return TRUE;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Send a request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWsaSendRequest(AbtWsiTransaction *trans)
{
	unsigned long rc;
	unsigned long size;
	char bytesString[20];
	unsigned long bytesTransferred;

	rc = getProperties(trans);

	if (!rc)
	{
		/*---------------------------------------------------------------
		* Check for the content data
		*---------------------------------------------------------------*/
		trans->req.data.contentLength = ECB->cbTotalBytes;

		if (trans->req.data.contentLength)
		{
			trans->req.data.contentData = AbtWsaMalloc(trans, trans->req.data.contentLength);
			if (trans->req.data.contentData)
			{
				memcpy(trans->req.data.contentData, ECB->lpbData, ECB->cbAvailable);
				if (ECB->cbTotalBytes != ECB->cbAvailable)
				{
					bytesTransferred = ECB->cbAvailable;         
					rc = 1;

					// Read bytes from the body of the HTTP request until all bytes are read 
					while (( bytesTransferred < trans->req.data.contentLength)  && (rc))
					{	
						size = trans->req.data.contentLength - bytesTransferred ;	
						rc = ECB->ReadClient(ECB->ConnID,
							trans->req.data.contentData + bytesTransferred,
							&size);	
						bytesTransferred = bytesTransferred + size ;
					}

					if (!rc)  
					{
						rc = -1;
						sprintf(bytesString,"%i",GetLastError());
						AbtWscDebugMessage(trans, "Error invoking ReadClient API.  ErrorText=", bytesString);
					}
					else
						rc = 0;    // Success!
				}
			}
			else
				rc = -1;
		}

		/*---------------------------------------------------------------
		* Send the request on through...
		*---------------------------------------------------------------*/
		if (!rc)
			rc =  AbtWscSendRequest(trans);
	}

	return (int)rc;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Handle the response to a request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWsaGetResponse(AbtWsiTransaction *trans)
{
	long  rc;
	unsigned long size;

	/*---------------------------------------------------------------
	* Get the response...
	*---------------------------------------------------------------*/
	rc = AbtWscGetResponse(trans);

	if ((!rc) && !(trans->error))
	{
		sendHeaders(trans);

		size = trans->resp.data.contentLength;

		if (!(ECB->WriteClient(ECB->ConnID,
			trans->resp.data.contentData,
			&size,
			HSE_IO_SYNC)))
			rc = -1;
	}

	return (int)rc;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Write a debug message; this is only called if debugging is on
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ AbtWsaDebugMessage(AbtWsiTransaction *trans, char *str1, char *str2)
{
	FILE *logFile;
	char *buffer = NULL;

	logFile = openLogFile();

	if (logFile)
	{
		AbtWscAddToString(trans, &buffer, "\n");
		AbtWscAddTimestamp(trans,&buffer);
		AbtWscAddToString(trans, &buffer, "Debug - ");
		AbtWscAddToString(trans, &buffer, str1);
		AbtWscAddToString(trans, &buffer, str2);

		fputs(buffer, logFile);

		fclose(logFile);

		AbtWsaFree(trans, buffer);
	}
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*----------------------------------------------------------------------------
 * Define common malloc, calloc, and free routines to ensure allocs
 * and frees are done from the same library when using multiple DLLs.
 * Also does error checking and handling.
 *---------------------------------------------------------------------------*/
void * _AbtWsi_Linkage_ AbtWsaMalloc(AbtWsiTransaction *trans, int size)
{
	void * ptr;

	ptr = malloc(size);

	if (!ptr)
		AbtWscError(trans, ERR_STRING_NOMEMORY, "", "", "");

	return ptr;
}

void * _AbtWsi_Linkage_ AbtWsaCalloc(AbtWsiTransaction *trans, int size)
{
	void * ptr;

	ptr = calloc(1, size);

	if (!ptr)
		AbtWscError(trans, ERR_STRING_NOMEMORY, "", "", "");

	return ptr;
}

void _AbtWsi_Linkage_ AbtWsaFree(AbtWsiTransaction *trans, void *ptr)
{
	if (ptr)
		free(ptr);
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Initialize the Global Config
 *------------------------------------------------------------------*/
BOOL _AbtWsi_Linkage_ initConfig()
{
	BOOL ret = TRUE;

	/* Config requires initialization */
	if(!Config) {
		AbtWsiTransaction *trans = NULL;

		Config = (AbtWsiConfig *)AbtWsaCalloc(trans, sizeof(AbtWsiConfig));
		if (!Config) {
			ret = FALSE;
		} else {
			Config->adapter.debugMessage = (AbtFunctionPointer)AbtWsaDebugMessage;
			Config->adapter.cmalloc       = AbtWsaMalloc;
			Config->adapter.ccalloc       = AbtWsaCalloc;
			Config->adapter.cfree         = AbtWsaFree;

			if (getTrans(&trans, NULL) != 0) {
				ret = FALSE;
			} else if (AbtWscInit(trans, getenv(ABTWSI_BASENAME), Config) != 0) {
				ret = FALSE;
			}

			AbtWscFreeTransaction(trans);
		}
	}
	return ret;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Handle a request
 *------------------------------------------------------------------*/
int  _AbtWsi_Linkage_ handleRequest(AbtWsiTransaction *trans)
{
	int rc = -1;

	if (!(trans->error))
	{
		rc = AbtWsaSendRequest(trans);

		if (!rc && !(trans->error))
			rc = AbtWsaGetResponse(trans);
	}

	if (trans->error)
		handleError(trans);

	return rc;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle an error condition
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ handleError(AbtWsiTransaction *trans)
{
	char *errorPage;
	unsigned long  size;

	if (trans->error)
	{
		errorPage = AbtWscFormatHtmlError(trans);

		if (errorPage)
		{
			sendHeaders(trans);

			size = strlen(errorPage);

			ECB->WriteClient(ECB->ConnID,
				errorPage,
				&size,
				HSE_IO_SYNC);

			AbtWsaFree(trans, errorPage);
		}

		/* log error... if logging is on */
		if (Config && Config->log)
		{
			if (strlen(trans->error) > (HSE_LOG_BUFFER_LEN - 1))
				trans->error[HSE_LOG_BUFFER_LEN] = (char)NULL;

			strcpy(ECB->lpszLogData, trans->error);
		}

		AbtWsaFree(trans, trans->error);
	}
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Setup the headers and returned status code
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ sendHeaders(AbtWsiTransaction *trans)
{
	char *statusCode;
	char *statusText;
	char *statusBuffer = NULL;
	char *headerBuffer = NULL;
	unsigned long length = 0;
	AbtWsiElement *elem = trans->resp.data.firstProperty;

	if (trans)
	{
		statusCode  = AbtWscFindProperty(trans, WSI_STATUS_CODE, &(trans->resp.data));

		if (statusCode)
		{
			statusText = AbtWscFindProperty(trans, WSI_STATUS_TEXT, &(trans->resp.data));

			AbtWscAddToString(trans, &statusBuffer, statusCode);

			if (statusText)
			{
				AbtWscAddToString(trans, &statusBuffer, " ");
				AbtWscAddToString(trans, &statusBuffer, statusText);
			}

			ECB->dwHttpStatusCode = atoi(statusCode);
		}
		else
			ECB->dwHttpStatusCode = 200;

		while (elem)
		{
			if (strcmp(elem->prop.name, WSI_STATUS_CODE) &&
				strcmp(elem->prop.name, WSI_STATUS_TEXT) )
			{
				if (!(strcmp(elem->prop.name, WSI_CONTENT_TYPE)))
					AbtWscAddToString(trans, &headerBuffer, "Content-type: ");
				else if (!(strcmp(elem->prop.name, WSI_LOCATION))) 
					AbtWscAddToString(trans, &headerBuffer, "Location: ");
				else  
				{
					AbtWscAddToString(trans, &headerBuffer, elem->prop.name);
					AbtWscAddToString(trans, &headerBuffer, ": ");
				}
				AbtWscAddToString(trans, &headerBuffer, elem->prop.value);
				AbtWscAddToString(trans, &headerBuffer, "\n");
			}
			elem = elem->next;
		}

		if (headerBuffer)
		{
			AbtWscAddToString(trans, &headerBuffer, "\n");
			length = strlen(headerBuffer) + 1;
		}

		ECB->ServerSupportFunction(ECB->ConnID,
			HSE_REQ_SEND_RESPONSE_HEADER,
			statusBuffer,
			&length,
			(void *)headerBuffer);

		AbtWsaFree(trans, statusBuffer);
		AbtWsaFree(trans, headerBuffer);
	}
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Build the property list
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ getProperties(AbtWsiTransaction *trans)
{
	int   i;
	unsigned long size;
	char  readBuff[4096];
	char *header;
	char *value;

	AbtWscCreateProperty(trans, "REQUEST_METHOD", ECB->lpszMethod, &(trans->req.data));
	AbtWscCreateProperty(trans, "QUERY_STRING", ECB->lpszQueryString, &(trans->req.data));
	AbtWscCreateProperty(trans, "PATH_INFO", ECB->lpszPathInfo, &(trans->req.data));
	AbtWscCreateProperty(trans, "PATH_TRANSLATED", ECB->lpszPathTranslated, &(trans->req.data));
	AbtWscCreateProperty(trans, "CONTENT_TYPE", ECB->lpszContentType, &(trans->req.data));

	for (i=0; *varStrings[i]; i++)
	{
		size = sizeof(readBuff);
		if (ECB->GetServerVariable(ECB->ConnID, varStrings[i], readBuff, &size)) {
			if (AbtWscCreateProperty(trans, varStrings[i], readBuff, &(trans->req.data)) != 0) {
				return -1;
			}
		} else {
			int reason = GetLastError();
			AbtWscDebugMessage(trans, "Failed to retrieve IIS Server variable: ", varStrings[i]);
			switch(reason) {
			case ERROR_INVALID_PARAMETER:
				AbtWscDebugMessage(trans, "IIS Server variable failure reason: ", "ERROR_INVALID_PARAMETER");
				break;
			case ERROR_INVALID_INDEX:
				AbtWscDebugMessage(trans, "IIS Server variable failure reason: ", "ERROR_INVALID_INDEX");
				break;
			case ERROR_INSUFFICIENT_BUFFER:
				AbtWscDebugMessage(trans, "IIS Server variable failure reason: ", "ERROR_INSUFFICIENT_BUFFER");
				break;
			case ERROR_NO_DATA:
				AbtWscDebugMessage(trans, "IIS Server variable failure reason: ", "ERROR_NO_DATA");
				break;
			default:
				break;
			}
		}
	}

	size = sizeof(readBuff);

	if (ECB->GetServerVariable(ECB->ConnID, "ALL_HTTP", readBuff, &size))
	{
		header = strtok(readBuff, "\n");

		if (header) {
			do
			{
				value = strchr(header, ':');

				if (value)
				{
					*value = (char)NULL;
					value++;
					if (AbtWscCreateProperty(trans, header, value, &(trans->req.data))) {
						return -1;
					}
				}

				header = strtok(NULL, "\n");

			} while (header);
		}   		//    If (header) end
	}

	return 0;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Get a new Transaction object
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ getTrans(AbtWsiTransaction **trans, EXTENSION_CONTROL_BLOCK *ecb)
{
	AbtWscAllocateTransaction(trans, AbtWsaCalloc);

	if (*trans)
	{
		(*trans)->adapterData = ecb;
		return 0;
	}
	else
		return -1;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Return a FILE pointer for the error log file
 *------------------------------------------------------------------*/
FILE * _AbtWsi_Linkage_ openLogFile(void)
{
	FILE *file = NULL;

	if (Config && Config->log && Config->logFile)
		file = fopen(Config->logFile, "a+");

	if (!file)
		file = fopen(DEFAULT_LOG_FILE, "a+");

	return file;
}

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
