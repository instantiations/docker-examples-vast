/*------------------------------------------------------------------
 * abtwsai.c : VisualAge for Smalltalk, Web Connection,
 *             Web Server Interface, IBM ICS Adapter
 *             (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#if defined(MEM_DEBUG)
#pragma strings(readonly)
#endif


/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#include "abtwsi.h"   /* Common include                             */
#include "HTAPI.h"    /* IBM ICS include                            */
#include <sys/stat.h>

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * ICS Data
 *------------------------------------------------------------------*/
typedef unsigned char * ICS_Handle;

/*------------------------------------------------------------------
 * Global Config pointer
 *------------------------------------------------------------------*/
AbtWsiConfig *Config = NULL;

/*------------------------------------------------------------------
 * Variable Strings
 *------------------------------------------------------------------*/
char * varStrings[] = {

/*** Standard CGI variables; excluding CONTENT_LENGTH  ****/

          "AUTH_TYPE",
          "CONTENT_TYPE",
          "CONTENT_ENCODING",
          "GATEWAY_INTERFACE",
          "PATH_INFO",
          "PATH_TRANSLATED",
          "QUERY_STRING",
          "REFERER_URL",
          "REMOTE_ADDR",
          "REMOTE_HOST",
          "REMOTE_IDENT",
          "REMOTE_USER",
          "REQUEST_METHOD",
          "SCRIPT_NAME",
          "SERVER_NAME",
          "SERVER_PORT",
          "SERVER_PROTOCOL",
          "SERVER_SOFTWARE",
          "HTTP_ACCEPT",
          "HTTP_COOKIE",
          "HTTP_LOCATION",
          "HTTP_USER_AGENT",

/*** NOTE: This last element is used to denote the end of the list ***/
          "\0"};

/*------------------------------------------------------------------
 * External entry points
 *------------------------------------------------------------------*/
void HTTPD_LINKAGE AbtWsiServerInit(ICS_Handle handle, unsigned long *major_version, unsigned long *minor_version, long *return_code);
void HTTPD_LINKAGE AbtWsiServerTerm(ICS_Handle handle, long * return_code);
void HTTPD_LINKAGE AbtWsiService(ICS_Handle handle, long * return_code);

/*------------------------------------------------------------------
 * Internal function prototypes
 *------------------------------------------------------------------*/
int  _AbtWsi_Linkage_ handleRequest(AbtWsiTransaction *trans, ICS_Handle handle);
void _AbtWsi_Linkage_ handleError(AbtWsiTransaction *trans);
void _AbtWsi_Linkage_ setHeaders(AbtWsiTransaction *trans);
void _AbtWsi_Linkage_ setProperties(AbtWsiTransaction *trans);
int  _AbtWsi_Linkage_ getProperties(AbtWsiTransaction *trans);
int  _AbtWsi_Linkage_ getTrans(AbtWsiTransaction **trans, ICS_Handle handle);

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Entrypoint for ICS server initialization notice
 *------------------------------------------------------------------*/
void HTTPD_LINKAGE AbtWsiServerInit(ICS_Handle handle,
         unsigned long *major_version, unsigned long *minor_version, long *return_code)
 {
  AbtWsiTransaction *trans;

  *return_code = HTTP_SERVER_ERROR;

  Config = AbtWsaCalloc(trans, sizeof(AbtWsiConfig));

  if (Config)
   {
    Config->adapter.debugMessage = (AbtFunctionPointer)AbtWsaDebugMessage;
    Config->adapter.cmalloc       = AbtWsaMalloc;
    Config->adapter.ccalloc       = AbtWsaCalloc;
    Config->adapter.cfree         = AbtWsaFree;

    if (!(getTrans(&trans, handle)))
     {
      if (!(AbtWscInit(trans, getenv(ABTWSI_BASENAME), Config)))
        *return_code = HTTP_OK;

      AbtWscFreeTransaction(trans);
     }
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Entrypoint for ICS termination notice
 *------------------------------------------------------------------*/
void HTTPD_LINKAGE AbtWsiServerTerm(ICS_Handle handle, long * return_code)
 {
  AbtWscEnd(NULL);

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

  *return_code = HTTP_OK;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Main entrypoint for ICS service request
 *------------------------------------------------------------------*/
void HTTPD_LINKAGE AbtWsiService(ICS_Handle handle, long * return_code)
 {
  AbtWsiTransaction *trans;

  *return_code = HTTP_OK;

  if (!(getTrans(&trans, handle)))
   {
    handleRequest(trans, handle);

    AbtWscFreeTransaction(trans);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Send a request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWsaSendRequest(AbtWsiTransaction *trans)
 {
  long  rc;
  char *errString;
  unsigned char *buffPtr;
  unsigned char  intBuff[35];
  unsigned long  size;
  unsigned long  length;

  rc = getProperties(trans);

  if (!rc)
   {
  /*---------------------------------------------------------------
   * Check for the content data
   *---------------------------------------------------------------*/
    size = sizeof(intBuff);
    length = 14;

    HTTPD_extract(trans->adapterData, (unsigned char *)"CONTENT_LENGTH", &length,
                  intBuff, &size, &rc);
    intBuff[size] = NULL;

    if (rc == HTTPD_SUCCESS)
     {
      trans->req.data.contentLength = strtoul((const char*)intBuff, &errString,10);
      if (*errString)
       {
        AbtWscError(trans, ERR_STRING_BADCONTENTLENGTH, (char *)intBuff, "", "");
        rc = -1;
       }
     }
    else
     {
      trans->req.data.contentLength = 0;
      rc = 0;
     }

    if (!rc && trans->req.data.contentLength)
     {
      trans->req.data.contentData = AbtWsaMalloc(trans, trans->req.data.contentLength);
      if (trans->req.data.contentData)
       {
        size = trans->req.data.contentLength;
        buffPtr = (unsigned char *)trans->req.data.contentData;
        while ((rc != HTTPD_EOF) && size)
         {
          HTTPD_read(trans->adapterData, buffPtr, &size, &rc);
         
          if (size)
           { 
            buffPtr += size;
            size = trans->req.data.contentLength - ((unsigned long)buffPtr - (unsigned long)(trans->req.data.contentData));
           } 
         }
        if (rc == HTTPD_EOF)
          rc = 0;
       }
      else
        rc = -1;
     }
   }
  /*---------------------------------------------------------------
   * Send the request on through...
   *---------------------------------------------------------------*/
  if (!rc)
    rc =  AbtWscSendRequest(trans);

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
    setHeaders(trans);
    setProperties(trans);

    size = trans->resp.data.contentLength;
    HTTPD_write(trans->adapterData, (unsigned char *)trans->resp.data.contentData,
                &size, &rc);
   }
  return (int)rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * write a debug message; this is only called if debugging is on
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ AbtWsaDebugMessage(AbtWsiTransaction *trans, char *str1, char *str2)
 {
  char *buffer = NULL;
  unsigned long  size;
  long  rc;

  AbtWscAddToString(trans, &buffer, "Debug - ");
  AbtWscAddToString(trans, &buffer, str1);
  AbtWscAddToString(trans, &buffer, str2);

  size =  strlen(buffer);
  HTTPD_log_error(trans->adapterData, (unsigned char *)buffer, &size, &rc);
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

/*------------------------------------------------------------------
 * Handle a request
 *------------------------------------------------------------------*/
int  _AbtWsi_Linkage_ handleRequest(AbtWsiTransaction *trans, ICS_Handle handle)
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
  long  rc;
  char *headerEnd = "\r\n\r\n";

  if (trans->error)
   {
    errorPage = AbtWscFormatHtmlError(trans);
    if (errorPage)
     {
      setHeaders(trans);

      size = strlen(headerEnd);
      HTTPD_write(trans->adapterData, (unsigned char *)headerEnd,  &size, &rc);

      size = strlen(errorPage);
      HTTPD_write(trans->adapterData, (unsigned char *)errorPage,  &size, &rc);

      AbtWsaFree(trans, errorPage);
     }

    /* log error to stderror too... if logging is on */
    if (Config && Config->log)
     {
      size = strlen(trans->error);
      HTTPD_log_error(trans->adapterData, (unsigned char *)trans->error, &size, &rc);
     }
    AbtWsaFree(trans, trans->error);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Set the headers
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ setHeaders(AbtWsiTransaction *trans)
 {
  long  rc = 0;
  char *statusCode;
  char *statusText;
  char *location;
  char *contentType;
  unsigned long size;
  unsigned long length;

  if (trans)
   {
    statusCode   = AbtWscFindProperty(trans, WSI_STATUS_CODE, &(trans->resp.data));
    location     = AbtWscFindProperty(trans, WSI_LOCATION, &(trans->resp.data));
    contentType  = AbtWscFindProperty(trans, WSI_CONTENT_TYPE, &(trans->resp.data));

    if (statusCode)
     {
      size = strlen(statusCode);
      length = 13;
      HTTPD_set(trans->adapterData, (unsigned char *)"HTTP_RESPONSE", &length,
                (unsigned char *)statusCode, &size, &rc);

      statusText   = AbtWscFindProperty(trans, WSI_STATUS_TEXT, &(trans->resp.data));

      if (statusText)
        {
         size = strlen(statusText);
         length = 11;
         HTTPD_set(trans->adapterData, (unsigned char *)"HTTP_REASON", &length,
                   (unsigned char *)statusText, &size, &rc);
       }
     }

    if (location)
     {
      size = strlen(location);
      length = 13;
      HTTPD_set(trans->adapterData, (unsigned char *)"HTTP_LOCATION", &length,
                (unsigned char *)location, &size, &rc);
     }

    if (!contentType)
      contentType = "text/html";

    size = strlen(contentType);
    length = 17;
    HTTPD_set(trans->adapterData, (unsigned char *)"HTTP_CONTENT_TYPE", &length,
              (unsigned char *)contentType, &size, &rc);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/


/*------------------------------------------------------------------
 * Set any returned properties
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ setProperties(AbtWsiTransaction *trans)
 {
  long  rc;
  unsigned long nameLength;
  unsigned long valueLength;
  AbtWsiElement *elem = trans->resp.data.firstProperty;

  while (elem)
   {
    if (strcmp(elem->prop.name, WSI_STATUS_CODE) &&
        strcmp(elem->prop.name, WSI_STATUS_TEXT) &&
        strcmp(elem->prop.name, WSI_LOCATION)    &&
        strcmp(elem->prop.name, WSI_CONTENT_TYPE))
     {
      nameLength  = strlen(elem->prop.name);
      valueLength = strlen(elem->prop.value);

      HTTPD_set(trans->adapterData, (unsigned char *)elem->prop.name, &nameLength,
                (unsigned char *)elem->prop.value, &valueLength, &rc);
     }
    elem = elem->next;
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
  unsigned long length;
  long  rc = 0;
  char *var;
  char *value;
  char  readBuff[4096];
  char dummy[] = " ";

  for (i=0; *varStrings[i]; i++)
   {
    size = sizeof(readBuff);
    length = strlen(varStrings[i]);
    HTTPD_extract(trans->adapterData, (unsigned char *)varStrings[i], &length,
                  (unsigned char *)readBuff, &size, &rc);

    if (rc == HTTPD_SUCCESS)
     {
      var   = AbtWsaMalloc(trans, strlen(varStrings[i])+1);
      
      if (var)
       {
        strcpy(var, varStrings[i]);
        
        if (size && *readBuff)
         {
          value = AbtWsaMalloc(trans, size+1);
          if (value)
           {
            memcpy(value, readBuff, size);
            value[size] = NULL;
           }
          else
            return -1;
         }
        else
         {
          value = AbtWsaMalloc(trans, strlen(dummy)+1); 
          if (value)
            strcpy(value, dummy);
          else
            return -1;
         }
        AbtWscAddProperty(trans, var, value, &(trans->req.data));
       }
      else
        return -1;
     }
   }

  size = sizeof(readBuff) - 1;
  length = 13;

  HTTPD_extract(trans->adapterData, (unsigned char *)"ALL_VARIABLES", &length,
                (unsigned char *)readBuff, &size, &rc);

  if (rc == HTTPD_SUCCESS)
   {
    readBuff[size] = NULL;
    var = strtok(readBuff, "\n");

    do
     {
      value = strchr(var, '=');

      if (value)
       {
        *value = NULL;
        value ++;

        if (AbtWscCreateProperty(trans, var, value, &(trans->req.data)))
          return -1;
       }

      var = strtok(NULL, "\n");

     } while (var);
   }

  return 0;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Get a new Transaction object
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ getTrans(AbtWsiTransaction **trans, ICS_Handle handle)
 {
  AbtWscAllocateTransaction(trans, AbtWsaCalloc);

  if (*trans)
   {
    (*trans)->adapterData = handle;
    return 0;
   }
  else
    return -1;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
