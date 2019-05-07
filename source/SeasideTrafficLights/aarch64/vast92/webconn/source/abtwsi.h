/*------------------------------------------------------------------
 * abtwsi.h :  VisualAge for Smalltalk, Web Connection,
 *             Web Server Interface, Common Header
 *             (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#ifndef _ABTWSI_H
#define _ABTWSI_H

/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

/*------------------------------------------------------------------
 * OS Specifc defs and includes
 *------------------------------------------------------------------*/
#include "abtwscos.h"

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Common memory macros
 *------------------------------------------------------------------*/
#define AbtWscMalloc(trans,size) Config->adapter.cmalloc(trans,size)
#define AbtWscCalloc(trans,size) Config->adapter.ccalloc(trans,size)
#define AbtWscFree(trans,ptr)    Config->adapter.cfree(trans,ptr)

/*------------------------------------------------------------------
 * Types
 *------------------------------------------------------------------*/
typedef struct AbtWsiTransportRecord
 {
  char                         *name;
  char                         *lib;
  char                         *initData;
  AbtModuleHandle               moduleHandle;
  AbtFunctionPointer            initPtr;
  AbtFunctionPointer            sendRequestPtr;
  AbtFunctionPointer            getResponsePtr;
  AbtFunctionPointer            resetTransportPtr;
  void                         *data;
  struct AbtWsiTransportRecord *nextRecord;
 } AbtWsiTransportRecord;

typedef struct AbtWsiProperty
 {
  char *name;
  char *value;
 } AbtWsiProperty;

typedef struct AbtWsiElement
 {
  AbtWsiProperty        prop;
  struct AbtWsiElement *next;
 } AbtWsiElement;

typedef struct AbtWsiRequestHeader
 {
  unsigned long  cookie;
  unsigned long  major;
  unsigned long  minor;
  unsigned long  id;
  unsigned long  type;
  char          *codePage;
 } AbtWsiRequestHeader;

typedef struct AbtWsiResponseHeader
 {
  unsigned long  id;
  unsigned long  type;
 } AbtWsiResponseHeader;

typedef struct AbtWsiMessageData
 {
  unsigned long   propertyCount;
  AbtWsiElement  *firstProperty;
  AbtWsiElement  *lastProperty;
  unsigned long   contentLength;
  char           *contentData;
 } AbtWsiMessageData;

typedef struct AbtWsiRequestMessage
 {
  AbtWsiRequestHeader  hdr;
  AbtWsiMessageData    data;
 } AbtWsiRequestMessage;

typedef struct AbtWsiResponseMessage
 {
  AbtWsiResponseHeader hdr;
  AbtWsiMessageData    data;
 } AbtWsiResponseMessage;


typedef struct AbtWsiTransaction
 {
  AbtWsiRequestMessage      req;
  AbtWsiResponseMessage     resp;
  void                     *adapterData;
  AbtWsiTransportRecord    *transport;
  void                     *transData;
  char                     *error;
 } AbtWsiTransaction;

typedef void * _AbtWsi_Linkage_ AbtMalloc(AbtWsiTransaction *trans, int size);
typedef void * _AbtWsi_Linkage_ AbtCalloc(AbtWsiTransaction *trans, int size);
typedef void   _AbtWsi_Linkage_ AbtFree(AbtWsiTransaction *trans, void *ptr);

typedef struct  AbtWsiAdapterRecord
 {
  AbtFunctionPointer  debugMessage;
  AbtMalloc          *cmalloc;
  AbtCalloc          *ccalloc;
  AbtFree            *cfree;
 } AbtWsiAdapterRecord;

typedef struct AbtWsiLinkRecord
 {
  char                    *pathInfo;
  AbtWsiTransportRecord   *transport;
  struct AbtWsiLinkRecord *nextRecord;
 } AbtWsiLinkRecord;

typedef struct AbtWsiConfig
 {
  char                  *errorFile;
  char                  *logFile;
  char                  *configFile;
  long                   configFileDate;
  unsigned long          verbose;
  unsigned long          debug;
  unsigned long          log;
  AbtWsiAdapterRecord    adapter;
  AbtWsiTransportRecord *firstTransportRecord;
  AbtWsiTransportRecord *lastTransportRecord;
  AbtWsiLinkRecord      *firstLinkRecord;
  AbtWsiLinkRecord      *lastLinkRecord;
 } AbtWsiConfig;

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * constants
 *------------------------------------------------------------------*/
#if !defined(TRUE)
   #define TRUE 1
#endif

#if !defined(FALSE)
   #define FALSE 0
#endif

#define MAX_LINE_LEN              1024

#define WSI_MAJOR_VERSION         2
#define WSI_MINOR_VERSION         0

#define WSI_REQUEST_TYPE_SERVICE  1L
#define WSI_REQUEST_TYPE_HELLO    2L

#define PROGRAM_LONGNAME          "VisualAge Web Connection"
#define CONFIG_FILE_EXT           ".cnf"
#define ERROR_FILE_EXT            ".htm"
#define LOG_FILE_EXT              ".log"
#define DEFAULT_NAME              "abtwsi"
#define DEFAULT_CONFIG_FILE       DEFAULT_NAME ".cnf"
#define DEFAULT_ERROR_FILE        DEFAULT_NAME ".htm"
#define DEFAULT_LOG_FILE          DEFAULT_NAME ".log"
#define ABTWSI_BASENAME           "ABTWSI_BASENAME" 

#define WSI_MAGIC_COOKIE          "\x04" "\x28" "\x19" "\x53"
#define WHITESPACE                " \a\b\t\n\v\f\r"

#define WSI_STATUS_CODE           "_X_ABTWSI_STATUS_CODE"
#define WSI_STATUS_TEXT           "_X_ABTWSI_STATUS_TEXT"
#define WSI_LOCATION              "_X_ABTWSI_LOCATION"
#define WSI_CONTENT_TYPE          "_X_ABTWSI_CONTENT_TYPE"

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Error Messages
 *------------------------------------------------------------------*/
#define ERR_STRING_NOMEMORY                  "The operating system reported an 'out of memory' condition."
#define ERR_STRING_NOCONFIGFILE              "The configuration file '%s' could not be found or was unreadable."
#define ERR_STRING_READCONFIGFILE            "An error occurred reading the configuration file %s."
#define ERR_STRING_PATHINFONOTSET            "The 'PATH_INFO' environment variable was not set by the calling program."
#define ERR_STRING_INVALIDKEYWORD            "An invalid keyword '%s' was found in the configuration file."
#define ERR_STRING_INVALIDKEYDATA            "Invalid data '%s' was defined for the keyword '%s' in the configuration file."
#define ERR_STRING_NOTENOUGHPARMS            "Not enough parameters for the keyword '%s' were defined in the configuration file."
#define ERR_STRING_NOPARMS                   "No parameters for the keyword '%s' were defined in the configuration file."
#define ERR_STRING_INVALIDLINKTRANSPORT      "An invalid link transport '%s' was specified in the configuration file."
#define ERR_STRING_ERRORLOADINGLIBRARY       "An error occurred loading the link transport library '%s'; error = %s'."
#define ERR_STRING_ERRORLOADINGFUNCTION      "An error occurred loading the function '%s' from transport library '%s'; error = %s."
#define ERR_STRING_BADCONTENTLENGTH          "Bad data was found in the 'CONTENT_LENGTH' variable; data = %s."

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * HTTP Message Strings
 *------------------------------------------------------------------*/
#define MSG_STRING_INSERTERROR               "#_Insert_Error_Message_Here_#"
#define MSG_STRING_UNAVAILABLE               "Unavailable"
#define MSG_STRING_ERROR                     "Error"
#define MSG_STRING_TITLE                     "Web Server Interface Error"
#define MSG_STRING_ERROR_HTTP                "Error processing an HTTP server transaction."
#define MSG_STRING_VERSION                   "Web Server Interface, version:"
#define MSG_STRING_COMPILED                  "Compiled on:"
#define MSG_STRING_DIRECTORY                 "Current directory:"

/*======================================================================*
 =                                                                      =
 =  COMMON CORE FUNCTION DEFINITIONS                                    =
 =                                                                      =
 *======================================================================*/

/*------------------------------------------------------------------
 * Other external function prototypes
 *------------------------------------------------------------------*/
int    _AbtWsi_Linkage_ AbtWscInit(AbtWsiTransaction *trans, char* baseName, AbtWsiConfig *config);
void   _AbtWsi_Linkage_ AbtWscEnd(AbtWsiTransaction *trans);
int    _AbtWsi_Linkage_ AbtWscSendRequest(AbtWsiTransaction *trans);
int    _AbtWsi_Linkage_ AbtWscGetResponse(AbtWsiTransaction *trans);
void   _AbtWsi_Linkage_ AbtWscResetTransport(AbtWsiTransaction *trans);
int    _AbtWsi_Linkage_ AbtWscAddProperty(AbtWsiTransaction *trans, char *name, char *value, AbtWsiMessageData *data);
int    _AbtWsi_Linkage_ AbtWscCreateProperty(AbtWsiTransaction *trans, char *name, char *value, AbtWsiMessageData *data);
char * _AbtWsi_Linkage_ AbtWscFindProperty(AbtWsiTransaction *trans, char *name, AbtWsiMessageData *data);
void   _AbtWsi_Linkage_ AbtWscAllocateTransaction(AbtWsiTransaction **transPtr, AbtCalloc mem_alloc);
void   _AbtWsi_Linkage_ AbtWscFreeTransaction(AbtWsiTransaction *trans);
int    _AbtWsi_Linkage_ AbtWscLoadConfig(AbtWsiTransaction *trans, char* baseName);
void   _AbtWsi_Linkage_ AbtWscError(AbtWsiTransaction *trans, char *str1, char *str2, char *str3, char *str4);
char * _AbtWsi_Linkage_ AbtWscFormatHtmlError(AbtWsiTransaction *trans);
char * _AbtWsi_Linkage_ AbtWscAddToString(AbtWsiTransaction *trans, char **oldStringPtr, char *addString);
char * _AbtWsi_Linkage_ AbtWscAddTimestamp(AbtWsiTransaction *trans, char ** buffPtr);
long   _AbtWsi_Linkage_ AbtWscMatchWildcardString(char *str1, char* str2);
int    _AbtWsi_Linkage_ AbtWscSetStatus(AbtWsiTransaction *trans, int code, char * text);
int    _AbtWsi_Linkage_ AbtStricmp(char *op1, char *op2);
int    _AbtWsi_Linkage_ AbtStrnicmp(char *op1, char *op2, int maxLength);

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/
/*======================================================================*
 =                                                                      =
 =  COMMON ADAPTER FUNCTION DEFINITIONS                                 =
 =                                                                      =
 *======================================================================*/

/*------------------------------------------------------------------
 * External function types
 *------------------------------------------------------------------*/
#define ABTWSTINIT_FUNCTION             "AbtWstInit"
#define ABTWSTSENDREQUEST_FUNCTION      "AbtWstSendRequest"
#define ABTWSTGETRESPONSE_FUNCTION      "AbtWstGetResponse"
#define ABTWSTRESETTRANSPORT_FUNCTION   "AbtWstResetTransport"

#if defined(OPSYS_OS2)
  typedef int (* _AbtWsi_Linkage_ PF_ABTWSTINIT)(AbtWsiTransaction *trans, AbtWsiConfig *config, AbtWsiTransportRecord *transRecord);
  typedef int (* _AbtWsi_Linkage_ PF_ABTWSTSENDREQUEST)(AbtWsiTransaction *trans);
  typedef int (* _AbtWsi_Linkage_ PF_ABTWSTGETRESPONSE)(AbtWsiTransaction *trans);
  typedef int (* _AbtWsi_Linkage_ PF_ABTWSTRESETTRANSPORT)(AbtWsiTransaction *trans);
#else
  typedef int (_AbtWsi_Linkage_ * PF_ABTWSTINIT)(AbtWsiTransaction *trans, AbtWsiConfig *config, AbtWsiTransportRecord *transRecord);
  typedef int (_AbtWsi_Linkage_ * PF_ABTWSTSENDREQUEST)(AbtWsiTransaction *trans);
  typedef int (_AbtWsi_Linkage_ * PF_ABTWSTGETRESPONSE)(AbtWsiTransaction *trans);
  typedef int (_AbtWsi_Linkage_ * PF_ABTWSTRESETTRANSPORT)(AbtWsiTransaction *trans);
#endif

/*------------------------------------------------------------------
 * External Function prototypes
 *------------------------------------------------------------------*/
int     _AbtWsi_Linkage_ AbtWsaSendRequest(AbtWsiTransaction *trans);
int     _AbtWsi_Linkage_ AbtWsaGetResponse(AbtWsiTransaction *trans);
void   _AbtWsi_Linkage_ AbtWsaDebugMessage(AbtWsiTransaction *trans, char *str1, char *str2);
void * _AbtWsi_Linkage_ AbtWsaMalloc(AbtWsiTransaction *trans, int size);
void * _AbtWsi_Linkage_ AbtWsaCalloc(AbtWsiTransaction *trans, int size);
void   _AbtWsi_Linkage_ AbtWsaFree(AbtWsiTransaction *trans, void *ptr);

/*------------------------------------------------------------------
 * External function types
 *------------------------------------------------------------------*/
#if defined(OPSYS_OS2)
  typedef void (* _AbtWsi_Linkage_  PF_ABTWSADEBUGMESSAGE)(struct AbtWsiTransaction *trans, char *str1, char *str2);
#else
  typedef void (_AbtWsi_Linkage_ * PF_ABTWSADEBUGMESSAGE)(struct AbtWsiTransaction *trans, char *str1, char *str2);
#endif

/*------------------------------------------------------------------
 * macro to write a debug message using the current adapter function
 *------------------------------------------------------------------*/
#define AbtWscDebugMessage(trans,str1,str2) \
  if (Config->debug)                  \
    ((PF_ABTWSADEBUGMESSAGE)(Config->adapter.debugMessage))(trans,str1,str2)

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*======================================================================*
 =                                                                      =
 =  COMMON TRANSPORT FUNCTION DEFINITIONS                               =
 =                                                                      =
 *======================================================================*/

/*------------------------------------------------------------------
 * External function prototypes
 *------------------------------------------------------------------*/
int  _AbtWsi_Linkage_ AbtWstInit(AbtWsiTransaction *trans, AbtWsiConfig *config, AbtWsiTransportRecord *transRecord);
void _AbtWsi_Linkage_ AbtWstResetTransport(AbtWsiTransaction *trans);
int  _AbtWsi_Linkage_ AbtWstSendRequest(AbtWsiTransaction *trans);
int  _AbtWsi_Linkage_ AbtWstGetResponse(AbtWsiTransaction *trans);

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Transport function macros
 *------------------------------------------------------------------*/
#define ABTWSTINIT(trans,config,transport)     (((PF_ABTWSTINIT)(trans->transport->initPtr))(trans,config,transport))
#define ABTWSTSENDREQUEST(trans)    (((PF_ABTWSTSENDREQUEST)(trans->transport->sendRequestPtr))(trans))
#define ABTWSTGETRESPONSE(trans)    (((PF_ABTWSTGETRESPONSE)(trans->transport->getResponsePtr))(trans))
#define ABTWSTRESETTRANSPORT(trans) (((PF_ABTWSTRESETTRANSPORT)(trans->transport->resetTransportPtr))(trans))

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

#endif
