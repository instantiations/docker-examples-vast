/*------------------------------------------------------------------
 * abtwsac.c : VisualAge for Smalltalk, Web Connection,
 *             Web Server Interface, CGI Adapter
 *             (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#if defined(MEM_DEBUG)
#pragma strings(readonly)
#endif

/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#define ABT_INCLUDE_SOCKETS
#include "abtwsi.h"   /* Common include                             */
#include <sys/stat.h>

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * CGI data structure
 *------------------------------------------------------------------*/
typedef struct AbtWsaCgiData
 {
  FILE  *stdinPtr;
  FILE  *stdoutPtr;
  char **envv;
 } AbtWsaCgiData;

/*------------------------------------------------------------------
 * Global Config pointer
 *------------------------------------------------------------------*/
AbtWsiConfig *Config = NULL;

/*------------------------------------------------------------------
 * Internal function prototypes
 *------------------------------------------------------------------*/
int    _AbtWsi_Linkage_ cgiInit(AbtWsiTransaction **transPtr, AbtWsaCgiData **dataPtr, char *envv[]);
int    _AbtWsi_Linkage_ handleRequest(AbtWsiTransaction *trans, AbtWsaCgiData * data, char * progName);
void   _AbtWsi_Linkage_ handleError(AbtWsiTransaction *trans);
void   _AbtWsi_Linkage_ setheaders(AbtWsiTransaction *trans);
int    _AbtWsi_Linkage_ getProperties(AbtWsiTransaction *trans);
FILE * _AbtWsi_Linkage_ openLogFile(void);

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * main program for CGI
 *------------------------------------------------------------------*/
int main(int argc, char *argv[], char *envv[])
 {
  int rc;
  AbtWsaCgiData     *data;
  AbtWsiTransaction *trans;

  #if defined(OPSYS_WIN32)
    WSADATA wsaData;
    WSAStartup(WS_VERSION_REQD, &wsaData); 
  #else
    #if defined(OPSYS_OS2)
      res_init();
    #endif
  #endif

  rc = cgiInit(&trans, &data, envv);

  if (!rc)
    rc = handleRequest(trans, data, argv[0]);

#if defined(MEM_DEBUG)
 {
  char buff1[] = "\n#######  FINAL MEMORY CHECK - SHOULD BE NONE !!!  ##########\n\n";
  char buff2[] = "\n#################\n";
  int fh = open("C:\\DEBUG.MEM", O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);

  if (fh != -1)
    _set_crt_msg_handle(fh);
  else
    fh = 1;

  write(fh, buff1, strlen(buff1));
  _dump_allocated(200);
  write(fh, buff2, strlen(buff2));

  if (fh != 1)
    close(fh);
 }
#endif

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Send a CGI request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWsaSendRequest(AbtWsiTransaction *trans)
 {
  int rc;
  char *errString;
  char *contentLengthStr = NULL;

  rc = getProperties(trans);

  if (!rc)
   {
  /*---------------------------------------------------------------
   * Check for the content data
   *---------------------------------------------------------------*/
    contentLengthStr = AbtWscFindProperty(trans, "CONTENT_LENGTH", &(trans->req.data));

    if (contentLengthStr)
     {
      trans->req.data.contentLength = strtoul(contentLengthStr, &errString,10);
      if (*errString)
        AbtWscError(trans, ERR_STRING_BADCONTENTLENGTH, contentLengthStr, "", "");
     }
    else
      trans->req.data.contentLength = 0;

    if (trans->req.data.contentLength)
     {
      trans->req.data.contentData = AbtWsaCalloc(trans, trans->req.data.contentLength+1);
      if (!trans->req.data.contentData)
        rc = -1;
      else
       {
        if ((fread(trans->req.data.contentData,
                   sizeof(char),
                   trans->req.data.contentLength,
                   ((AbtWsaCgiData *)trans->adapterData)->stdinPtr)) != trans->req.data.contentLength)
        rc = -1;
       }
     }
   }
  /*---------------------------------------------------------------
   * Send the request on through...
   *---------------------------------------------------------------*/
  if (!rc)
    rc =  AbtWscSendRequest(trans);

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Handle the response to a CGI request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWsaGetResponse(AbtWsiTransaction *trans)
 {
  int    rc;

  /*---------------------------------------------------------------
   * Get the response...
   *---------------------------------------------------------------*/
  rc = AbtWscGetResponse(trans);

  if ((!rc) && !(trans->error))
   {
    setheaders(trans);

    fwrite(trans->resp.data.contentData,
           sizeof(char),
           trans->resp.data.contentLength,
           ((AbtWsaCgiData *)trans->adapterData)->stdoutPtr);

   }
  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * write a debug message; this is only called if debugging is on
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ AbtWsaDebugMessage(AbtWsiTransaction *trans, char *str1, char *str2)
 {
  FILE *logFile;
  char *buffer = NULL;

  logFile = openLogFile();

  if (logFile)
   {
    AbtWscAddToString(trans, &buffer, "\r\n");
    AbtWscAddTimestamp(trans,&buffer);
    AbtWscAddToString(trans, &buffer, "Debug - ");
    AbtWscAddToString(trans, &buffer, str1);
    AbtWscAddToString(trans, &buffer, str2);

    fputs(buffer, logFile);

    fclose(logFile);
   }
  AbtWsaFree(trans, buffer);
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

int _AbtWsi_Linkage_ cgiInit(AbtWsiTransaction **transPtr, AbtWsaCgiData **dataPtr, char *envv[])
 {
  int  ret = -1;

/*------------------------------------------------------------------
 * Allocate the Config, trans and CGI data objecsts
 *------------------------------------------------------------------*/
  Config = AbtWsaCalloc(*transPtr, sizeof(AbtWsiConfig));
  if (Config)
   {
    Config->adapter.debugMessage = (AbtFunctionPointer)AbtWsaDebugMessage;
    Config->adapter.cmalloc       = AbtWsaMalloc;
    Config->adapter.ccalloc       = AbtWsaCalloc;
    Config->adapter.cfree         = AbtWsaFree;

    AbtWscAllocateTransaction(transPtr, AbtWsaCalloc);
    *dataPtr = AbtWsaMalloc(NULL, sizeof(AbtWsaCgiData));

    if (*transPtr && *dataPtr)
     {
     /*---------------------------------------------------------------
      * set stdout to binary (OS/2 and Windows only)
      *---------------------------------------------------------------*/

   #if (defined(OPSYS_OS2) || defined(OPSYS_WIN32))
      freopen("", "rb", stdin);
      freopen("", "wb", stdout);
   #endif

      (*dataPtr)->stdinPtr  = stdin;
      (*dataPtr)->stdoutPtr = stdout;
      (*dataPtr)->envv = envv;

       ret = 0;
     }
   }

  return ret;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Handle a CGI request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ handleRequest(AbtWsiTransaction *trans, AbtWsaCgiData * data, char * progName)
 {
  int rc = -1;

  char * extPtr;
  char * baseName;

  trans->adapterData = data;

  baseName = getenv(ABTWSI_BASENAME);

  if ( !baseName && progName)
   {
    baseName = AbtWsaMalloc(trans, strlen(progName)+1);
    if (baseName)
     {
      strcpy(baseName, progName);
      extPtr = strrchr(baseName, '.');

      if (extPtr)
       {
        if (!(AbtStricmp(extPtr, ".exe")))
          *extPtr = '\0';
       }
     }
    rc = AbtWscInit(trans, baseName, Config);
    AbtWsaFree(trans, baseName);
   }
  else
    rc = AbtWscInit(trans, baseName, Config);

  if (!rc && !(trans->error))
   {
    rc = AbtWsaSendRequest(trans);

    if (!rc && !(trans->error))
      rc = AbtWsaGetResponse(trans);
   }

  if (trans->error)
    handleError(trans);

  AbtWsaFree(trans, data);
  AbtWscFreeTransaction(trans);

  AbtWscEnd(NULL);

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle an error condition
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ handleError(AbtWsiTransaction *trans)
 {
  char *buffer;
  char *errorPage;
  FILE *logFile;

  if (trans->error)
   {
    errorPage = AbtWscFormatHtmlError(trans);
    if (errorPage)
     {
      setheaders(trans);
      fputs(errorPage, ((AbtWsaCgiData *)trans->adapterData)->stdoutPtr);
      AbtWsaFree(trans, errorPage);
     }

    /* log error to stderror too... if logging is on                                  */
    logFile = openLogFile();

    if (logFile)
     {
      buffer = NULL;
      AbtWscAddToString(trans, &buffer, "\r\n=======================================================================\r\n");
      AbtWscAddTimestamp(trans, &buffer);
      AbtWscAddToString(trans, &buffer, trans->error);
      AbtWscAddToString(trans, &buffer, "\r\n=======================================================================\r\n");

      fputs(buffer, logFile);
      fclose(logFile);
      AbtWsaFree(trans, buffer);
     }
    AbtWsaFree(trans, trans->error);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Build the CGI headers
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ setheaders(AbtWsiTransaction *trans)
 {
  char  *buffer       = NULL;
  char  *statusText   = NULL;
  char  *contentType  = "text/html";
  AbtWsiElement *elem;

  if (trans)
   {
    elem = trans->resp.data.firstProperty;

    while (elem)
     {
      if (!(strcmp(elem->prop.name, WSI_STATUS_CODE)))
       {
        AbtWscAddToString(trans, &buffer, "Status: ");
        AbtWscAddToString(trans, &buffer, elem->prop.value);
        statusText = AbtWscFindProperty(trans, WSI_STATUS_TEXT, &(trans->resp.data));
        if (statusText)
         {
          AbtWscAddToString(trans, &buffer, " "); 
          AbtWscAddToString(trans, &buffer, statusText);
         }
        AbtWscAddToString(trans, &buffer, "\r\n");
       }

      else if (!(strcmp(elem->prop.name, WSI_LOCATION)))
       {
        AbtWscAddToString(trans, &buffer, "Location: ");
        AbtWscAddToString(trans, &buffer, elem->prop.value);
        AbtWscAddToString(trans, &buffer, "\r\n");
       }

      else if (!(strcmp(elem->prop.name, WSI_CONTENT_TYPE)))
       { 
        contentType = elem->prop.value;
       }

      else if (strcmp(elem->prop.name, WSI_STATUS_TEXT))
       {
        AbtWscAddToString(trans, &buffer, elem->prop.name);
        AbtWscAddToString(trans, &buffer, ": ");
        AbtWscAddToString(trans, &buffer, elem->prop.value);
        AbtWscAddToString(trans, &buffer, "\r\n");
       }
      elem = elem->next;
     }
    AbtWscAddToString(trans, &buffer, "Content-type: ");
    AbtWscAddToString(trans, &buffer, contentType);
    AbtWscAddToString(trans, &buffer, "\r\n\r\n");
   }
  fputs(buffer, ((AbtWsaCgiData *)trans->adapterData)->stdoutPtr);
  AbtWsaFree(trans, buffer);
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Build the property list
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ getProperties(AbtWsiTransaction *trans)
 {
  int  i = 0;
  int  rc = 0;
  char *key;
  char *val;
  char *keyPtr;
  char *valPtr;
  char dummy[] = " ";
  char **envv = ((AbtWsaCgiData *)trans->adapterData)->envv;

  for (i=0; envv[i] && !rc; i++)
   {
    if (!(*envv[i]))
      continue;

    keyPtr = envv[i];
    valPtr = strchr(keyPtr,'=');
    if (valPtr)
      *valPtr = '\0';
    key = AbtWsaCalloc(trans, (strlen(keyPtr) + 1)); 

    if ( !valPtr || !(*(valPtr+1)) )
     valPtr = dummy;
    else  
     valPtr++;

    val = AbtWsaMalloc(trans, (strlen(valPtr)+1));

    if (key && val)
     {
      strcpy(key, keyPtr);
      strcpy(val, valPtr);  
      AbtWscAddProperty(trans, key, val, &(trans->req.data));
     }  
    else
     rc = -1;
   }
  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Return a FILE pointer for the error log file, IFF
 * error logging is enabled.
 *------------------------------------------------------------------*/
FILE * _AbtWsi_Linkage_ openLogFile(void)
 {
  FILE *file = NULL;

  if (Config && Config->log && Config->logFile)
    file = fopen(Config->logFile, "a+");

  if (!file && (!Config || Config->log))
   {
    file = fopen(DEFAULT_LOG_FILE, "a+");

    if (!file)
      file = stdout;
   }
  return file;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
