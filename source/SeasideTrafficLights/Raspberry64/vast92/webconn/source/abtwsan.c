/*------------------------------------------------------------------
 * abtwsai.c : VisualAge for Smalltalk, Web Connection,
 *             Web Server Interface, Netscape API Adapter
 *             (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#if defined(MEM_DEBUG)
#pragma strings(readonly)
#endif

/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#include "abtwsi.h"         /* Common include                       */
#include <netsite.h>        /* NSAPI includes                       */
#include <base/pblock.h>
#include <base/session.h>
#include <base/daemon.h>
#include <frame/req.h>
#include <frame/http.h>
#include <frame/protocol.h>
#include <frame/log.h>
#include <frame/conf.h>

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Types
 *------------------------------------------------------------------*/
typedef struct AbtWsaNsapiData
 {
  pblock   *pb;
  Session  *sn;
  Request  *rq;
 } AbtWsaNsapiData;

/*------------------------------------------------------------------
 * Global Config pointer
 *------------------------------------------------------------------*/
AbtWsiConfig *Config = NULL;

/*------------------------------------------------------------------
 * External entry points
 *------------------------------------------------------------------*/
int  NSAPI_PUBLIC AbtWsiServerInit(pblock *pb, Session *sn, Request *rq);
int  NSAPI_PUBLIC AbtWsiService(pblock *pb, Session *sn, Request *rq);
void NSAPI_PUBLIC AbtWsiRestart(void *data);
int  NSAPI_PUBLIC AbtWsiSetPathInfo(pblock *pb, Session *sn, Request *rq);

/*------------------------------------------------------------------
 * Internal function prototypes
 *------------------------------------------------------------------*/
int  _AbtWsi_Linkage_ handleRequest(AbtWsiTransaction *trans);
void _AbtWsi_Linkage_ handleError(AbtWsiTransaction *trans);
void _AbtWsi_Linkage_ setHeaders(AbtWsiTransaction *trans);
void _AbtWsi_Linkage_ setProperties(AbtWsiTransaction *trans);
void _AbtWsi_Linkage_ addPbToProperties(AbtWsiTransaction *trans, pblock *pb, char * prefix, AbtWsiMessageData *data);
int  _AbtWsi_Linkage_ getTrans(AbtWsiTransaction **trans, pblock *pb, Session *sn, Request *rq);
void _AbtWsi_Linkage_ getProperties(AbtWsiTransaction *trans);

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Entrypoint for NSAPI server initialization notice
 *------------------------------------------------------------------*/
int NSAPI_PUBLIC AbtWsiServerInit(pblock *pb, Session *sn, Request *rq)
 {
  AbtWsiTransaction *trans;
  int rc = REQ_ABORTED;

  Config = AbtWsaCalloc(NULL, sizeof(AbtWsiConfig));

  if (Config)
   {
    Config->adapter.debugMessage = (AbtFunctionPointer)AbtWsaDebugMessage;
    Config->adapter.cmalloc       = AbtWsaMalloc;
    Config->adapter.ccalloc       = AbtWsaCalloc;
    Config->adapter.cfree         = AbtWsaFree;

    if (!(getTrans(&trans, pb, sn, rq)))
     {
      if (!(AbtWscInit(trans, getenv(ABTWSI_BASENAME), Config)))
        rc = REQ_PROCEED;
      else if (trans->error)
        pblock_nvinsert("error", trans->error, pb);

/*    magnus_atrestart(AbtWsiRestart, NULL); */
      daemon_atrestart(AbtWsiRestart, NULL);  

      AbtWscFreeTransaction(trans);
     }
    else
      pblock_nvinsert("error", "An error occurred allocating the WSI transaction object", pb);
   }

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Main entrypoint for NSAPI service request
 *------------------------------------------------------------------*/
int NSAPI_PUBLIC AbtWsiService(pblock *pb, Session *sn, Request *rq)
 {
  AbtWsiTransaction *trans;
  int rc = REQ_EXIT;

  if (!(getTrans(&trans, pb, sn, rq)))
   {
    if (!(handleRequest(trans)))
      rc = REQ_PROCEED;

    AbtWsaFree(trans, trans->adapterData);
    AbtWscFreeTransaction(trans);
   }
  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * callback for NSAPI restart
 *------------------------------------------------------------------*/
void NSAPI_PUBLIC AbtWsiRestart(void *data)
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
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

int NSAPI_PUBLIC AbtWsiSetPathInfo(pblock *pb, Session *sn, Request *rq)
 {
  char *seperator = pblock_findval("separator", pb);
  char *ppath     = pblock_findval("ppath", rq->vars);
  char *tempStr;

  if (!seperator)
   {
    log_error(LOG_MISCONFIG, "abtwsan - AbtWsiSetPathinfo", sn, rq,
              "Missing parameter - separator");
    return REQ_ABORTED;
   }

  tempStr = strstr(ppath, seperator);
  
  if (tempStr)
   {
    tempStr += strlen(seperator);

    if ( (*tempStr != '/') && (*(tempStr-1) == '/') )
      tempStr--;
  
    pblock_nvinsert("path-info", tempStr, rq->vars);
   }

  return REQ_NOACTION;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

void _AbtWsi_Linkage_ getProperties(AbtWsiTransaction *trans)
 {
  int   onceOnly = FALSE;
  char *value;
  char *uri_string;
  char *path_info;
  char  intBuff[35];
  AbtWsaNsapiData *data = (AbtWsaNsapiData *)trans->adapterData;

  value = pblock_findval("path-info", data->rq->vars);
  AbtWscCreateProperty(trans, "PATH_INFO", value, &(trans->req.data));
  value = pblock_findval("query", data->rq->vars);
  AbtWscCreateProperty(trans, "QUERY_STRING", value, &(trans->req.data));
  value = pblock_findval("user-agent", data->rq->headers);
  AbtWscCreateProperty(trans, "USER_AGENT", value, &(trans->req.data));
  value = pblock_findval("protocol", data->rq->reqpb);
  AbtWscCreateProperty(trans, "SERVER_PROTOCOL", value, &(trans->req.data));
  value = pblock_findval("method", data->rq->reqpb);
  AbtWscCreateProperty(trans, "REQUEST_METHOD", value, &(trans->req.data));
  value = http_uri2url_dynamic("", "", data->sn, data->rq);
  AbtWscCreateProperty(trans, "SERVER_HOSTNAME", value, &(trans->req.data));
  value = http_uri2url("", "");
  AbtWscCreateProperty(trans, "SERVER_URL", value, &(trans->req.data));
  value = pblock_findval("ip", data->sn->client);
  AbtWscCreateProperty(trans, "REMOTE_ADDR", value, &(trans->req.data));
  value = pblock_findval("auth-user", data->rq->vars);
  AbtWscCreateProperty(trans, "REMOTE_USER", value, &(trans->req.data));
  value = session_dns(data->sn);
  AbtWscCreateProperty(trans, "REMOTE_HOST", value, &(trans->req.data));
  value = pblock_findval("auth-type", data->rq->vars);
  AbtWscCreateProperty(trans, "AUTH_TYPE", value, &(trans->req.data));

#ifdef NET_SSL
  AbtWscCreateProperty(trans, "HTTPS", ( (security_active) ? "TRUE" : "FALSE"), &(trans->req.data));
  value = pblock_findval("keysize", data->sn->client);
  AbtWscCreateProperty(trans, "HTTPS_KEYSIZE", value, &(trans->req.data));
  value = pblock_findval("secret-keysize", data->sn->client);
  AbtWscCreateProperty(trans, "HTTPS_SECRETSIZE", value, &(trans->req.data));
#endif

  addPbToProperties(trans, data->pb, NULL, &(trans->req.data));
  addPbToProperties(trans, data->rq->vars, NULL, &(trans->req.data));
  addPbToProperties(trans, data->rq->reqpb, NULL, &(trans->req.data));
  addPbToProperties(trans, data->rq->headers, "HTTP_", &(trans->req.data));

  if (!(AbtWscFindProperty(trans, "PATH_INFO", &(trans->req.data))))
   {
    uri_string = AbtWscFindProperty(trans, "URI", &(trans->req.data));
    if (uri_string)
     {
      path_info = strrchr(uri_string, '/');
      if (path_info)
        AbtWscCreateProperty(trans, "PATH_INFO", path_info, &(trans->req.data));
     }
   }
 }

/*------------------------------------------------------------------
 * Send a request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWsaSendRequest(AbtWsiTransaction *trans)
 {
  int   rc = 0;
  int bytesInBuff;
  char *contentLenStr;
  AbtWsaNsapiData *data = (AbtWsaNsapiData *)trans->adapterData;

  getProperties(trans);

  /*---------------------------------------------------------------
   * Check for the content data
   *---------------------------------------------------------------*/
  trans->req.data.contentLength = 0;

  if (request_header("content-length", &contentLenStr, data->sn,
         data->rq) != REQ_ABORTED)
    if (contentLenStr)
      trans->req.data.contentLength = atoi(contentLenStr);

  trans->req.data.contentData = AbtWsaCalloc(trans, trans->req.data.contentLength +1);

  if (!(trans->req.data.contentData))
    rc = -1;
  else
   {
    bytesInBuff = data->sn->inbuf->cursize - data->sn->inbuf->pos;

    if (bytesInBuff)
      memcpy(trans->req.data.contentData,
             data->sn->inbuf->inbuf + data->sn->inbuf->pos, bytesInBuff);

    if (bytesInBuff < trans->req.data.contentLength)
      net_read(data->sn->csd,
               trans->req.data.contentData + bytesInBuff,
               trans->req.data.contentLength - bytesInBuff,
               100);

    /*---------------------------------------------------------------
     * Send the request on through...
     *---------------------------------------------------------------*/
    rc = AbtWscSendRequest(trans);
   }

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Handle the response to a request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWsaGetResponse(AbtWsiTransaction *trans)
 {
  int rc;

  /*---------------------------------------------------------------
   * Get the response...
   *---------------------------------------------------------------*/
  rc = AbtWscGetResponse(trans);

  if ((!rc) && !(trans->error))
   {
    setHeaders(trans);
    setProperties(trans);

    rc = protocol_start_response(((AbtWsaNsapiData *)trans->adapterData)->sn,
               ((AbtWsaNsapiData *)trans->adapterData)->rq);

    if ( (rc == REQ_PROCEED) || (rc == REQ_NOACTION) )
     {
      net_write(((AbtWsaNsapiData *)trans->adapterData)->sn->csd, trans->resp.data.contentData,
                trans->resp.data.contentLength);
      rc = REQ_PROCEED;
     }
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
  log_error(LOG_INFORM, "abtwsan", ((AbtWsaNsapiData *)trans->adapterData)->sn,
            ((AbtWsaNsapiData *)trans->adapterData)->rq, "%s %s %s", "Debug -", str1, str2);
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

  #if !defined(MEM_DEBUG)
    ptr = MALLOC(size);
  #else
    ptr = malloc(size);
  #endif

  if (!ptr)
    AbtWscError(trans, ERR_STRING_NOMEMORY, "", "", "");

  return ptr;
 }

void * _AbtWsi_Linkage_ AbtWsaCalloc(AbtWsiTransaction *trans, int size)
 {
  void * ptr;

  #if !defined(MEM_DEBUG)
    ptr = CALLOC(size);
  #else
    ptr = calloc(1, size);
  #endif

  if (!ptr)
    AbtWscError(trans, ERR_STRING_NOMEMORY, "", "", "");

  return ptr;
 }

void _AbtWsi_Linkage_ AbtWsaFree(AbtWsiTransaction *trans, void *ptr)
 {
  if (ptr)
  #if !defined(MEM_DEBUG)
    FREE(ptr);
  #else
    free(ptr);
  #endif
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Handle a request
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ handleRequest(AbtWsiTransaction *trans)
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

  if (trans->error)
   {
    errorPage = AbtWscFormatHtmlError(trans);
    if (errorPage)
     {
      setHeaders(trans);

      if(protocol_start_response(((AbtWsaNsapiData *)trans->adapterData)->sn,
          ((AbtWsaNsapiData *)trans->adapterData)->rq) != REQ_NOACTION)
        net_write(((AbtWsaNsapiData *)trans->adapterData)->sn->csd, errorPage,
                    strlen(errorPage));

      AbtWsaFree(trans, errorPage);
     }
    /* log error...  if logging is on */
    if (Config && Config->log)
     {
      log_error(LOG_FAILURE, "abtwsan", ((AbtWsaNsapiData *)trans->adapterData)->sn,
                ((AbtWsaNsapiData *)trans->adapterData)->rq, trans->error);
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
  char *statusCode  = NULL;
  char *statusText  = NULL;
  char *location    = NULL;
  char *contentType = NULL;

  if (trans)
   {
    statusCode   = AbtWscFindProperty(trans, WSI_STATUS_CODE, &(trans->resp.data));
    statusText   = AbtWscFindProperty(trans, WSI_STATUS_TEXT, &(trans->resp.data));
    location     = AbtWscFindProperty(trans, WSI_LOCATION, &(trans->resp.data));
    contentType  = AbtWscFindProperty(trans, WSI_CONTENT_TYPE, &(trans->resp.data));

    if (statusCode)
      protocol_status(((AbtWsaNsapiData *)trans->adapterData)->sn, ((AbtWsaNsapiData *)trans->adapterData)->rq,
                      atoi(statusCode), statusText);
    else
      protocol_status(((AbtWsaNsapiData *)trans->adapterData)->sn, ((AbtWsaNsapiData *)trans->adapterData)->rq,
                      PROTOCOL_OK, "OK");

    if (location)
      pblock_nvinsert("Location", location, ((AbtWsaNsapiData *)trans->adapterData)->rq->srvhdrs);

    if (contentType)
     {
      param_free(pblock_remove("content-type", ((AbtWsaNsapiData *)trans->adapterData)->rq->srvhdrs));
      pblock_nvinsert("content-type", contentType, ((AbtWsaNsapiData *)trans->adapterData)->rq->srvhdrs);
     }
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/


/*------------------------------------------------------------------
 * Set any returned properties
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ setProperties(AbtWsiTransaction *trans)
 {
  AbtWsiElement *elem = trans->resp.data.firstProperty;

  while (elem)
   {
    if (strcmp(elem->prop.name, WSI_STATUS_CODE) &&
        strcmp(elem->prop.name, WSI_STATUS_TEXT) &&
        strcmp(elem->prop.name, WSI_LOCATION)    &&
        strcmp(elem->prop.name, WSI_CONTENT_TYPE))
      pblock_nvinsert(elem->prop.name, elem->prop.value,
                      ((AbtWsaNsapiData *)trans->adapterData)->rq->srvhdrs);

    elem = elem->next;
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Add values in Pb to the property list
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ addPbToProperties(AbtWsiTransaction *trans, pblock *pb, char * prefix, AbtWsiMessageData *data)
 {
  int i, j;
  char  *namePtr = NULL;
  char  *nextDash;
  struct pb_entry *entry;

  for (i = 0; i < pb->hsize; i++)
   {
    for (entry = pb->ht[i]; entry; entry = entry->next)
     {
      if (prefix)
        namePtr = AbtWscAddToString(trans, &namePtr, prefix);

      namePtr = AbtWscAddToString(trans, &namePtr, entry->param->name);

      nextDash = strpbrk(namePtr, "-");
      while (nextDash)
       {
        *nextDash = '_';
        nextDash = strpbrk((nextDash+1), "-");
       }

      for (j = 0; namePtr[j]; namePtr[j] = toupper(namePtr[j]), j++);

      AbtWscCreateProperty(trans, namePtr, entry->param->value, data);

      AbtWsaFree(trans, namePtr);
      namePtr = NULL;
     }
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Get a new Transaction object
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ getTrans(AbtWsiTransaction **trans, pblock *pb, Session *sn, Request *rq)
 {
  AbtWsaNsapiData *dataPtr;

  *trans = NULL;

  dataPtr = (AbtWsaNsapiData *)AbtWsaMalloc(*trans, sizeof(AbtWsaNsapiData));

  if (dataPtr)
   {
    dataPtr->pb = pb;
    dataPtr->sn = sn;
    dataPtr->rq = rq;

    AbtWscAllocateTransaction(trans, AbtWsaCalloc);

    if (*trans)
     {
      (*trans)->adapterData = dataPtr;
      return 0;
     }
   }
  return -1;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
