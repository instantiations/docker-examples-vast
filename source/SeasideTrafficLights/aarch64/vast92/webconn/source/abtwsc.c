/*--------------
 * abtwsc.c :  VisualAge for Smalltalk, Web Connection,
 *             Web Server Interface, Core Functions
 *             (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
/****
#if defined(MEM_DEBUG)
#pragma strings(readonly)
#endif
****/

/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#define ABT_INCLUDE_GETCWD
#define ABT_INCLUDE_LOAD
#include "abtwsi.h"   /* Common include                             */
#include <sys/stat.h> /* Needed for fstat function                  */

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Global info'
 *------------------------------------------------------------------*/
AbtWsiConfig *Config = NULL;

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Configuration File Strings and characters
 *------------------------------------------------------------------*/
#define MSG_STRING_LOG                       "Log"
#define MSG_STRING_VERBOSE                   "Verbose"
#define MSG_STRING_LINK                      "Link"
#define MSG_STRING_TRANSPORT                 "Transport"
#define MSG_STRING_CODEPAGE                  "CodePage"
#define MSG_STRING_ERRORFORMAT               "ErrorFormat"
#define MSG_STRING_DEBUG                     "Debug"
#define MSG_STRING_TRUE                      "True"
#define MSG_STRING_FALSE                     "False"
#define MSG_STRING_PLUS                      "+"
#define MSG_STRING_MINUS                     "-"
#define MSG_STRING_ONE                       "1"
#define MSG_STRING_ZERO                      "0"
#define MSG_STRING_ON                        "on"
#define MSG_STRING_OFF                       "off"
#define MSG_STRING_WILDCARD                  "*"

#define MSG_COMMENT_CHAR                     '#' /* NOTE: This is a character,
                                                          not a string         */

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Internal function prototypes
 *------------------------------------------------------------------*/
int  _AbtWsi_Linkage_ handleConfigLine(AbtWsiTransaction *trans, char *line);
int  _AbtWsi_Linkage_ keywordLog(AbtWsiTransaction *trans, char *data);
int  _AbtWsi_Linkage_ keywordVerbose(AbtWsiTransaction *trans, char *data);
int  _AbtWsi_Linkage_ keywordLink(AbtWsiTransaction *trans, char *data);
int  _AbtWsi_Linkage_ keywordTransport(AbtWsiTransaction *trans, char *data);
int  _AbtWsi_Linkage_ keywordCodePage(AbtWsiTransaction *trans, char *data);
int  _AbtWsi_Linkage_ keywordErrorFormat(AbtWsiTransaction *trans, char *data);
long _AbtWsi_Linkage_ getBooleanValue(char *string);
void _AbtWsi_Linkage_ addLink(AbtWsiTransaction *trans, AbtWsiTransportRecord *transport, char *pathInfo);
void _AbtWsi_Linkage_ addTransport(AbtWsiTransaction *trans, char *transportName, char *loadLib, char *initData);
AbtWsiLinkRecord * _AbtWsi_Linkage_ findLinkRecord(char *pathInfo);
unsigned long _AbtWsi_Linkage_ getNextID(void);
int  _AbtWsi_Linkage_ loadTransportFunctions(AbtWsiTransaction *trans, AbtWsiTransportRecord *transRecord);
AbtWsiTransportRecord * _AbtWsi_Linkage_ findTransportByName(char *transName, AbtWsiConfig *config);
AbtWsiTransportRecord * _AbtWsi_Linkage_ findTransportByLib(char *libName, AbtWsiConfig *config);
void _AbtWsi_Linkage_ freeProperties(AbtWsiTransaction *trans, AbtWsiElement *properties);
void _AbtWsi_Linkage_ freeProperty(AbtWsiTransaction *trans, AbtWsiElement *propElement);
void _AbtWsi_Linkage_ freeConfig(AbtWsiTransaction *trans, AbtWsiConfig *config);
void _AbtWsi_Linkage_ freeTransport(AbtWsiTransaction *trans, AbtWsiTransportRecord *transport);
void _AbtWsi_Linkage_ freeLink(AbtWsiTransaction *trans, AbtWsiLinkRecord *link);
char * _AbtWsi_Linkage_ getDefaultMessageFormat(AbtWsiTransaction *trans, char *errorBuffer);

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Initialize the core data and read the config file
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWscInit(AbtWsiTransaction *trans, char* baseName, AbtWsiConfig *config)
 {
  int rc = -1;
  AbtWsiTransportRecord *next;

  Config = config;

  rc = AbtWscLoadConfig(trans, baseName);

  if (!rc)
   {
    AbtWscDebugMessage(trans, "<--------------- Start --------------->", "");

    next  = Config->firstTransportRecord;

/* Modified for defect 12084.  Code will now return '0' unconditionally.  */
/* Old code 
		 while (next && !rc)   */

   while ( next )
     {
     ((PF_ABTWSTRESETTRANSPORT)(next->resetTransportPtr))(trans);
      rc = ((PF_ABTWSTINIT)(next->initPtr))(trans, Config, next);
      next = next->nextRecord;
     }
   }

/* Always return 0.  Defect 12084  */
  return 0;
 }

/*------------------------------------------------------------------
 * Free any memory before exiting
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ AbtWscEnd(AbtWsiTransaction *trans)
 {
  freeConfig(trans, Config);
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Send a request to the appropriate transport services
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWscSendRequest(AbtWsiTransaction *trans)
 {
  int                rc = -1;
  char              *value;
  AbtWsiLinkRecord  *link;

  value = AbtWscFindProperty(trans, "PATH_INFO", &(trans->req.data));

  if (value)
   {
    link = findLinkRecord(value);
    if (link && link->transport)
     {
      trans->transport = link->transport;
      rc = ABTWSTSENDREQUEST(trans);
     }
    else
      AbtWscError(trans, ERR_STRING_INVALIDLINKTRANSPORT, value, "", "");
   }
  else
     AbtWscError(trans, ERR_STRING_PATHINFONOTSET, "", "", "");

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Get a response via the appropriate transport services
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWscGetResponse(AbtWsiTransaction *trans)
 {
  int rc = -1;

  if (trans->transport->getResponsePtr)
    rc = ABTWSTGETRESPONSE(trans);

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Call the Transport function to reset itself
*------------------------------------------------------------------*/
void _AbtWsi_Linkage_ AbtWscResetTransport(AbtWsiTransaction *trans)
 {
  ABTWSTRESETTRANSPORT(trans);
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*--------------------------------------------------------------------
 * Allocate memory and add a property name/value pair to list.
 * It is assumed that the name/value pair has already been malloc'd.
 *--------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWscAddProperty(AbtWsiTransaction *trans, char *name, char *value, AbtWsiMessageData *data)
 {
  AbtWsiElement  *elem;
 
/*--------------------------------------------------------------
 * Don't add empty, null, or duplicate properties
 *-------------------------------------------------------------*/
  if (name && *name && value && *value && !(AbtWscFindProperty(trans,name,data)))
   {
    /*-------------------------------------------------------------
     * allocate memory
     *-------------------------------------------------------------*/
    elem = (AbtWsiElement *)AbtWscMalloc(trans, sizeof(AbtWsiElement));
   
    if (!elem)
      return -1;

    /*-------------------------------------------------------------
     * update the new property and element
    *-------------------------------------------------------------*/
    elem->prop.name  = name;

    if (value && *value)
      elem->prop.value = value;
    else
      elem->prop.value = "";

    elem->next = NULL;

    /*-------------------------------------------------------------
     * link into the list
     *-------------------------------------------------------------*/
    if (!(data->firstProperty))
      data->firstProperty = elem;

    if (data->lastProperty)
      data->lastProperty->next = elem;

    data->lastProperty = elem;

    data->propertyCount++;
   }
  else
   {
    if (name && *name)
      AbtWscFree(trans, name);
    if (value && *value)
      AbtWscFree(trans, value);
   }
  return 0;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Create and add a new property name/value pair
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWscCreateProperty(AbtWsiTransaction *trans, char *name, char *value, AbtWsiMessageData *data)
 {
  char *newName;
  char *newValue;
  int   rc = 0;
  char dummy[] = " ";
  char *valuePtr = value;

  if (name && value)
   {
    if (!(*value))
      valuePtr = dummy;
 
    newName  = AbtWscMalloc(trans, strlen(name)+1);
    newValue = AbtWscMalloc(trans, strlen(valuePtr)+1);

    if (newName && newValue)
     {
      strcpy(newName, name);
      strcpy(newValue, valuePtr);

      rc = AbtWscAddProperty(trans, newName, newValue, data);
     }
    else
      rc = -1;
   }
  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/
/*---------------------------------------------------------------------
 * Find the value for a property matching a name in the message data
 *---------------------------------------------------------------------*/
char * _AbtWsi_Linkage_ AbtWscFindProperty(AbtWsiTransaction *trans, char *name, AbtWsiMessageData *data)
 {
  AbtWsiElement *elem = data->firstProperty;

  while (elem)
   {
    if (!(AbtStricmp(name, elem->prop.name)))
       break;
    elem = elem->next;
   }

  if (elem)
    return elem->prop.value;
  else
    return NULL;
 }


/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/
/*------------------------------------------------------------------
 * Allocate a transaction structure
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ AbtWscAllocateTransaction(AbtWsiTransaction **transPtr, AbtCalloc mem_alloc)
 {
  /*---------------------------------------------------------------
   * allocate memory
   *---------------------------------------------------------------*/
  *transPtr = mem_alloc(NULL, sizeof(AbtWsiTransaction));

  if (*transPtr)
   {
  /*---------------------------------------------------------------
   * Initialize request header with default values
   *---------------------------------------------------------------*/
    memcpy(&((*transPtr)->req.hdr.cookie), WSI_MAGIC_COOKIE, 4);

    (*transPtr)->req.hdr.major    = WSI_MAJOR_VERSION;
    (*transPtr)->req.hdr.minor    = WSI_MINOR_VERSION;
    (*transPtr)->req.hdr.id       = getNextID();
    (*transPtr)->req.hdr.type     = WSI_REQUEST_TYPE_SERVICE;
    (*transPtr)->req.hdr.codePage = NULL;
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Free a request/Response block
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ AbtWscFreeTransaction(AbtWsiTransaction *trans)
 {
  if (trans)
   {
    freeProperties(trans, trans->req.data.firstProperty);
    AbtWscFree(trans, trans->req.data.contentData);

    freeProperties(trans, trans->resp.data.firstProperty);
    AbtWscFree(trans, trans->resp.data.contentData);

    AbtWscFree(NULL, trans);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * read the config info' from file
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWscLoadConfig(AbtWsiTransaction *trans, char *baseName)
 {
  int          rc = -1;
  char        *str;
  char         fileLine[MAX_LINE_LEN + 1];
  char        *line       = fileLine;
  char        *configFile = NULL;
  char        *logFile    = NULL;
  char        *errorFile  = NULL;
  FILE        *file       = NULL;
  struct stat  fileStat;

  /*---------------------------------------------------------------
   * open file
   *---------------------------------------------------------------*/
  if (baseName)
   {
    configFile = (char *)AbtWscMalloc(trans, (strlen(baseName)) + (strlen(CONFIG_FILE_EXT))+1);
    errorFile = (char *)AbtWscMalloc(trans, (strlen(baseName)) + (strlen(ERROR_FILE_EXT))+1);
    logFile = (char *)AbtWscMalloc(trans, (strlen(baseName)) + (strlen(LOG_FILE_EXT))+1);

    if (configFile && errorFile && logFile)
     {
      strcpy(errorFile, baseName);
      strcat(errorFile, ERROR_FILE_EXT);

      strcpy(logFile, baseName);
      strcat(logFile, LOG_FILE_EXT);

      strcpy(configFile, baseName);
      strcat(configFile, CONFIG_FILE_EXT);

      file = fopen(configFile, "r");
     }
   }

  if (!file)
   {
    AbtWscFree(trans, configFile);
    configFile = (char *)AbtWscMalloc(trans, (strlen(DEFAULT_CONFIG_FILE)+1));
    strcpy(configFile, DEFAULT_CONFIG_FILE);
    file = fopen(configFile, "r");
   }

  if (!file)
   {
    AbtWscError(trans, ERR_STRING_NOCONFIGFILE, configFile, "", "");
 }
  else
   {
   /*---------------------------------------------------------------
    * initialize the config structure pointer
    *---------------------------------------------------------------*/
    rc = 0;

    Config->configFile = configFile;
    Config->errorFile  = errorFile;
    Config->logFile    = logFile;

    if (!(fstat(fileno(file), &fileStat)))
      Config->configFileDate = (long)fileStat.st_mtime;

   /*---------------------------------------------------------------
    * read lines from file
    *---------------------------------------------------------------*/
    while (!feof(file) && !rc)
     {
      if (!fgets(line, MAX_LINE_LEN, file))
          break;

      if (!*line) continue;

      str = strchr(line, MSG_COMMENT_CHAR);
      if (str)
        *str = '\0';

      rc = handleConfigLine(trans, fileLine);
     }

    /*---------------------------------------------------------------
     * check for valid data and return
     *---------------------------------------------------------------*/
    if (rc || !(Config->firstTransportRecord && Config->firstLinkRecord))
     {
      AbtWscError(trans, ERR_STRING_READCONFIGFILE, configFile, "", "");
      if (!rc)
        rc = -1;
     }
   /*---------------------------------------------------------------
    * close file
    *---------------------------------------------------------------*/
    fclose(file);
   }
  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle an error condition
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ AbtWscError(AbtWsiTransaction *trans, char *str1, char *str2, char *str3, char *str4)
 {
  char buffer[MAX_LINE_LEN];

  if (trans && !(trans->error))
   {
    sprintf(buffer, str1, str2, str3, str4);
    AbtWscAddToString(trans, &(trans->error), buffer);

    /*---------------------------------------------------------------
     * Set the status code and text
     *---------------------------------------------------------------*/
    AbtWscSetStatus(trans, 500, trans->error);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Format an HTML error message
 *------------------------------------------------------------------*/
char * _AbtWsi_Linkage_ AbtWscFormatHtmlError(AbtWsiTransaction *trans)
 {
  FILE          *file = NULL;
  char           line[MAX_LINE_LEN + 1];
  char           tempBuffer[MAX_LINE_LEN+1];
  char		*cwdResult = NULL;
  char          *msgBuffer = NULL;
  char          *errorBuffer = NULL;
  AbtWsiElement *elem;
  /*---------------------------------------------------------------
   * open file
   *---------------------------------------------------------------*/
  if (Config && Config->errorFile)
    file = fopen(Config->errorFile, "r");

  if (!file)
    file = fopen(DEFAULT_ERROR_FILE, "r");

  AbtWscAddToString(trans, &errorBuffer, "<P><B><FONT SIZE=+2>");
  AbtWscAddToString(trans, &errorBuffer, trans->error);
  AbtWscAddToString(trans, &errorBuffer, "</FONT></B></P>\n");

  if (Config && Config->verbose)
   {
    AbtWscAddToString(trans, &errorBuffer, "<P><B>Web Server Interface:</B></P><UL>\n");

    sprintf(tempBuffer,"%s %d.%d\n",
            MSG_STRING_VERSION, WSI_MAJOR_VERSION, WSI_MINOR_VERSION);
    AbtWscAddToString(trans, &errorBuffer, tempBuffer);

    sprintf(tempBuffer,"<BR>%s %s, %s\n",
            MSG_STRING_COMPILED, __DATE__, __TIME__);
    AbtWscAddToString(trans, &errorBuffer, tempBuffer);

    sprintf(tempBuffer,"<BR>%s ", MSG_STRING_DIRECTORY);
    AbtWscAddToString(trans, &errorBuffer, tempBuffer);

    cwdResult = getcwd(tempBuffer, sizeof(tempBuffer));
    AbtWscAddToString(trans, &errorBuffer, tempBuffer);
    AbtWscAddToString(trans, &errorBuffer, "</UL>\n");

    AbtWscAddToString(trans, &errorBuffer, "<P><B>Request parameters:</B></P><UL>");
    elem = trans->req.data.firstProperty;
    while (elem)
     {
      sprintf(tempBuffer, "<B>%s</B> = %s\n<BR>", elem->prop.name, elem->prop.value);
      AbtWscAddToString(trans, &errorBuffer, tempBuffer);
      elem = elem->next;
     }
    AbtWscAddToString(trans, &errorBuffer, "</P></UL><P><B>Response parameters:</B></P><UL compact>\n");
    elem = trans->resp.data.firstProperty;
    while (elem)
     {
      sprintf(tempBuffer, "<B>%s</B> = %s\n<BR>", elem->prop.name, elem->prop.value);
      AbtWscAddToString(trans, &errorBuffer, tempBuffer);
      elem = elem->next;
     }
    AbtWscAddToString(trans, &errorBuffer, "</P></UL>\n");
   }
  if (file)
   {
   /*---------------------------------------------------------------
    * read lines from file
    *---------------------------------------------------------------*/
    while (!feof(file))
     {
      if (!fgets(line, MAX_LINE_LEN, file))
        break;

      if (!*line) continue;

      if (AbtStrnicmp(MSG_STRING_INSERTERROR, line, strlen(MSG_STRING_INSERTERROR)))
        AbtWscAddToString(trans, &msgBuffer, line);
      else
        AbtWscAddToString(trans, &msgBuffer, errorBuffer);
     }
   }
  else
   msgBuffer = getDefaultMessageFormat(trans, errorBuffer);

  AbtWscFree(trans, errorBuffer);

  return msgBuffer;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*---------------------------------------------------------------------
 * This function concatenates two strings by allocating a memory for the
 * new string and copying the two strings into the new memory block.  It
 * is assumed that the first string (*oldStringPtr) was allocated via
 * AbtWscMalloc and if new the allocation is successful the oldString is
 * freed.  You can also pass in NULL as the first value and only the
 * second string will be added to the newly malloc'd string.  The value
 * pointed to by oldStringPtr is updated and returned.
 *---------------------------------------------------------------------*/
char * _AbtWsi_Linkage_ AbtWscAddToString(AbtWsiTransaction *trans,
                                             char **oldStringPtr,
                                             char *addString)
 {
  char * newString;
  int    stringSize = 0;

  if (oldStringPtr && *oldStringPtr && **oldStringPtr)
    stringSize = (strlen(*oldStringPtr));
  if (addString && *addString)
    stringSize += (strlen(addString));
  stringSize += 1;

  newString = (char *)AbtWscCalloc(trans, stringSize);

  if (newString)
   {
    if (oldStringPtr && *oldStringPtr && **oldStringPtr)
     {
      strcpy(newString, *oldStringPtr);
      AbtWscFree(trans, *oldStringPtr);
     }

    if (addString && *addString)
      strcat(newString, addString);

    *oldStringPtr = newString;
   }

  return *oldStringPtr;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Add a timestamp to the beginning of a string
 *------------------------------------------------------------------*/
char * _AbtWsi_Linkage_ AbtWscAddTimestamp(AbtWsiTransaction *trans, char ** buffPtr)
 {
  time_t   timeP;
  char    *timeString;

  time(&timeP);
  timeString = ctime(&timeP);
  timeString[strlen(timeString)-1] = '\0';

  AbtWscAddToString(trans, buffPtr, timeString);
  AbtWscAddToString(trans, buffPtr, ": ");

  return *buffPtr;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * match a wildcard string, up to the first asterisk in string 2
 *------------------------------------------------------------------*/
long _AbtWsi_Linkage_ AbtWscMatchWildcardString(char *str1, char* str2)
 {
  char copyStr1[MAX_LINE_LEN+1];
  char copyStr2[MAX_LINE_LEN+1];
  char *wildStr;

  if (!str1 || !str2)
   {
    if (!str1 && !str2)
      return TRUE;
    else
      return FALSE;
   }

  if ( !(strcmp(MSG_STRING_WILDCARD, str2)) )
    return TRUE;

  strcpy(copyStr1, str1);
  strcpy(copyStr2, str2);

  wildStr = strtok(copyStr2, MSG_STRING_WILDCARD);
  copyStr1[strlen(wildStr)] = '\0';

  if ( !(AbtStricmp(copyStr1, wildStr)) )
    return TRUE;
  else
    return FALSE;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Set the status properties
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWscSetStatus(AbtWsiTransaction *trans, int code, char * text)
 {
  char    intBuff[35];

  /*---------------------------------------------------------------
   * Set the appropriate status code and text
   *---------------------------------------------------------------*/
  if (!(AbtWscFindProperty(trans, WSI_STATUS_CODE, &(trans->resp.data))))
   {
    sprintf(intBuff, "%d", code);

    if ( (AbtWscCreateProperty(trans, WSI_STATUS_CODE, intBuff, &(trans->resp.data))) ||
         (AbtWscCreateProperty(trans, WSI_STATUS_TEXT, text, &(trans->resp.data))) )
      return -1;
   }
  return 0;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

 /*------------------------------------------------------------------
 * Common AbtStricmp 
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtStricmp(char *op1, char *op2)
 {
  for (; tolower(*op1) == tolower(*op2); op1++,op2++)
    if ((*op1 == 0) || (*op2 == 0))
      break;

  return(tolower(*op1) - tolower(*op2));
 }
 
 int _AbtWsi_Linkage_ AbtStrnicmp(char *op1, char *op2, int maxLength)
 {
  int count;
  
  for (count = 1; tolower(*op1) == tolower(*op2); op1++,op2++,count++)
    if ((*op1 == 0) || (*op2 == 0) || (count == maxLength))
      break;

  return(tolower(*op1) - tolower(*op2));
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle a configuration line
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ handleConfigLine(AbtWsiTransaction *trans, char *line)
 {
  char *keyword;
  char *data;
  int   rc = -1;

  /*---------------------------------------------------------------
   * parse it up
   *---------------------------------------------------------------*/
  keyword = strtok(line, WHITESPACE);

  if ( !keyword || !*keyword )
    return 0;

  /*---------------------------------------------------------------
   * check for errors
   *---------------------------------------------------------------*/
  data = strtok(NULL, "\n");

  if (!data || !*data)
    AbtWscError(trans, ERR_STRING_NOPARMS, keyword, "", "");
  else
   {
    /*---------------------------------------------------------------
     * handle the keyword
     *---------------------------------------------------------------*/
    if (!AbtStricmp(keyword, MSG_STRING_LOG))
      rc = keywordLog(trans, data);
    else if (!AbtStricmp(keyword, MSG_STRING_LINK))
      rc = keywordLink(trans, data);
    else if (!AbtStricmp(keyword, MSG_STRING_TRANSPORT))
      rc = keywordTransport(trans, data);
    else if (!AbtStricmp(keyword, MSG_STRING_VERBOSE))
      rc = keywordVerbose(trans, data);
    else if (!AbtStricmp(keyword, MSG_STRING_CODEPAGE))
      rc = keywordCodePage(trans, data);
    else if (!AbtStricmp(keyword, MSG_STRING_ERRORFORMAT))
      rc = keywordErrorFormat(trans, data);
    else
      AbtWscError(trans, ERR_STRING_INVALIDKEYWORD, keyword, "", "");
   }
  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle Log keyword
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ keywordLog(AbtWsiTransaction *trans, char *data)
 {
  char *value;
  char *fileName;
  int   rc = -1;
  char *lastChar; 

  /*---------------------------------------------------------------
   * parse up the data
   *---------------------------------------------------------------*/
  value = strtok(data,WHITESPACE);

  if (value)
   {
    fileName = strtok(NULL,"\n");
 
    /*---------------------------------------------------------------
     * set, as appropriate
     *---------------------------------------------------------------*/
    Config->log = getBooleanValue(value);

    if (Config->log == -1)
     {
      if (AbtStricmp(value, MSG_STRING_DEBUG))
       {
        Config->log = 0;
        Config->debug = 0;
        AbtWscError(trans, ERR_STRING_INVALIDKEYDATA, value , MSG_STRING_LOG, "");
       }
      else
       {
        Config->debug = 1;
        Config->log = 1;
        rc = 0;
       }
     }
    else
     rc = 0;

  if (!rc)
   {
    if (fileName)
     {
      for (lastChar = fileName + (strlen(fileName)-1); isspace(*lastChar); lastChar--)
         *lastChar = '\0';

      AbtWscFree(trans, Config->logFile);
      Config->logFile = AbtWscMalloc(trans, strlen(fileName)+1);
      if (Config->logFile)
        strcpy(Config->logFile, fileName);
      else
       rc = -1;
     }
    }
   }
  else
    AbtWscError(trans, ERR_STRING_NOTENOUGHPARMS, MSG_STRING_DEBUG, "", "");

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle Verbose keyword
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ keywordVerbose(AbtWsiTransaction *trans, char *data)
 {
  char *value;
  int   rc = -1;

  /*---------------------------------------------------------------
   * parse up the data
   *---------------------------------------------------------------*/
  value = strtok(data,WHITESPACE);

  if (value)
   {
    /*---------------------------------------------------------------
     * set, as appropriate
     *---------------------------------------------------------------*/
    Config->verbose = getBooleanValue(data);

    if (-1 == Config->verbose)
     {
      AbtWscError(trans, ERR_STRING_INVALIDKEYDATA, data, MSG_STRING_VERBOSE, "");
      Config->verbose = 0;
     }
    else
     rc = 0;
   }
  else
    AbtWscError(trans, ERR_STRING_NOTENOUGHPARMS, MSG_STRING_VERBOSE, "", "");

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle a Link keyword
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ keywordLink(AbtWsiTransaction *trans, char *data)
 {
  int   rc = -1;
  char *pathInfo;
  char *transName;
  AbtWsiTransportRecord *transport;

  /*---------------------------------------------------------------
   * parse it up
   *---------------------------------------------------------------*/
  pathInfo  = strtok(data,WHITESPACE);
  transName = strtok(NULL,WHITESPACE);

  /*---------------------------------------------------------------
   * check for errors, if none, add link info'
   *---------------------------------------------------------------*/
  if (pathInfo && transName && *pathInfo && *transName)
   {
    transport = findTransportByName(transName, Config);
    if (!transport)
      AbtWscError(trans, ERR_STRING_INVALIDLINKTRANSPORT, transName, "", "");
    else
     {
      addLink(trans, transport, pathInfo);
      rc = 0;
     }
   }
  else
    AbtWscError(trans, ERR_STRING_NOTENOUGHPARMS, MSG_STRING_LINK, "", "");

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle a Transport keyword
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ keywordTransport(AbtWsiTransaction *trans, char *data)
 {
  char *transport;
  char *loadLib;
  char *initData;
  int   rc = -1;

  /*---------------------------------------------------------------
   * parse it up
   *---------------------------------------------------------------*/
  transport = strtok(data, WHITESPACE);
  loadLib   = strtok(NULL, WHITESPACE);
  initData  = strtok(NULL, "\n");

  /*---------------------------------------------------------------
   * check for errors, if none, add info'
   *---------------------------------------------------------------*/
  if (transport  && loadLib  && initData &&
       *transport && *loadLib && *initData)
   {
    addTransport(trans, transport, loadLib, initData);
    rc = 0;
   }
  else
    AbtWscError(trans, ERR_STRING_NOTENOUGHPARMS, MSG_STRING_TRANSPORT, "", "");

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle CodePage keyword
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ keywordCodePage(AbtWsiTransaction *trans, char *data)
 {
  char *lastChar;

  strtok(data, "\n");

  if (data && *data && !trans->req.hdr.codePage)
   {
    for (lastChar = data + (strlen(data)-1); isspace(*lastChar); lastChar--)
       *lastChar = '\0';

    trans->req.hdr.codePage = AbtWscMalloc(trans, strlen(data)+1);
    if (trans->req.hdr.codePage)
      strcpy(trans->req.hdr.codePage, data);
  else
    return -1;    
   }
  return 0;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * handle ErrorFormat keyword
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ keywordErrorFormat(AbtWsiTransaction *trans, char *data)
 {
  char *fileName;
  int   rc = -1;

  /*---------------------------------------------------------------
   * parse up the data
   *---------------------------------------------------------------*/
  fileName = strtok(data,WHITESPACE);

  if (fileName)
   {
    AbtWscFree(trans, Config->errorFile);
    Config->errorFile = AbtWscMalloc(trans, strlen(fileName)+1);
    if (Config->errorFile)
     {
      strcpy(Config->errorFile, fileName);
       rc = 0;
     }
   }
  else
    AbtWscError(trans, ERR_STRING_NOTENOUGHPARMS, MSG_STRING_DEBUG, "", "");

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Answer a boolean value for a string.  Returns:
 *    1, if the string indicates positive
 *    0, if the string indicates negative
 *   -1, if the string isn't valid
 *------------------------------------------------------------------*/
long _AbtWsi_Linkage_ getBooleanValue(char *string)
 {
  if ((!AbtStricmp(string, MSG_STRING_TRUE)) ||
      (!AbtStricmp(string, MSG_STRING_ON))   ||
      (!AbtStricmp(string, MSG_STRING_ONE))  ||
      (!AbtStricmp(string, MSG_STRING_PLUS)))
    return 1;
  else
   {
    if ((!AbtStricmp(string, MSG_STRING_FALSE))  ||
        (!AbtStricmp(string, MSG_STRING_OFF))    ||
        (!AbtStricmp(string, MSG_STRING_ZERO))   ||
        (!AbtStricmp(string, MSG_STRING_MINUS)))
      return 0;
    else
      return -1;
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Add a link record to Config
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ addLink(AbtWsiTransaction *trans, AbtWsiTransportRecord *transport, char *pathInfo)
 {
  AbtWsiLinkRecord *linkRecord;
  char             *pathInfoCopy;

  /*---------------------------------------------------------------
   * allocate memory
   *---------------------------------------------------------------*/
  linkRecord = (AbtWsiLinkRecord *)AbtWscMalloc(trans, sizeof(AbtWsiLinkRecord));
  pathInfoCopy = (char *)AbtWscMalloc(trans, 1+strlen(pathInfo));

  if (!pathInfoCopy || !linkRecord)
    return;

  /*---------------------------------------------------------------
   * copy string
   *---------------------------------------------------------------*/
  strcpy(pathInfoCopy, pathInfo);

  /*---------------------------------------------------------------
   * update the new link record
   *---------------------------------------------------------------*/
  linkRecord->pathInfo   = pathInfoCopy;
  linkRecord->transport  = transport;
  linkRecord->nextRecord = NULL;

  /*---------------------------------------------------------------
   * link into the list
   *---------------------------------------------------------------*/
  if (!Config->firstLinkRecord)
    Config->firstLinkRecord = linkRecord;

  if (Config->lastLinkRecord)
    Config->lastLinkRecord->nextRecord = linkRecord;

  Config->lastLinkRecord = linkRecord;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Add a transport record to the global Config
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ addTransport(AbtWsiTransaction *trans, char *transportName, char *loadLib, char *initData)
 {
  char                  *transportCopy;
  char                  *loadLibCopy;
  char                  *initDataCopy;
  AbtWsiTransportRecord *transRecord;

  /*---------------------------------------------------------------
   * allocate memory
   *---------------------------------------------------------------*/
  transportCopy = AbtWscMalloc(trans, 1+strlen(transportName));
  loadLibCopy   = AbtWscMalloc(trans, 1+strlen(loadLib));
  initDataCopy  = AbtWscMalloc(trans, 1+strlen(initData));
  transRecord   = AbtWscMalloc(trans, sizeof(AbtWsiTransportRecord));

  if (transportCopy && loadLibCopy && initDataCopy && transRecord)
   {
    /*---------------------------------------------------------------
     * copy strings
     *---------------------------------------------------------------*/
    strcpy(transportCopy, transportName);
    strcpy(loadLibCopy, loadLib);
    strcpy(initDataCopy, initData);

    /*---------------------------------------------------------------
     * update the new transport record
     *---------------------------------------------------------------*/
    transRecord->name       = transportCopy;
    transRecord->lib        = loadLibCopy;
    transRecord->initData   = initDataCopy;
    transRecord->nextRecord = NULL;
	transRecord->data = NULL;

    if (loadTransportFunctions(trans, transRecord))
      freeTransport(trans, transRecord);
    else
     {
      /*---------------------------------------------------------------
       * link into the list
       *---------------------------------------------------------------*/
      if (!Config->firstTransportRecord)
        Config->firstTransportRecord = transRecord;

      if (Config->lastTransportRecord)
        Config->lastTransportRecord->nextRecord = transRecord;

      Config->lastTransportRecord = transRecord;
     }
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*----------------------------------------------------------------------
 * Find a link record matching this pathinfo and return the last match.
 * The pathInfo being matched in the link record can contain a single
 * trailing asterisk, '*', for wildcard matching.  The pathInfo being
 * passed in is a complete path name without wildcards.
 *----------------------------------------------------------------------*/
AbtWsiLinkRecord * _AbtWsi_Linkage_ findLinkRecord(char *pathInfo)
 {
  AbtWsiLinkRecord *next  = Config->firstLinkRecord;
  AbtWsiLinkRecord *match = NULL;

  while (next)
   {
    if (AbtWscMatchWildcardString(pathInfo, next->pathInfo))
      match = next;
    next = next->nextRecord;
   }
  return match;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

unsigned long _AbtWsi_Linkage_ getNextID(void)
 {
  static unsigned long ID = 0;

  return ++ID;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Load a transport DLL w/associated function pointers
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ loadTransportFunctions(AbtWsiTransaction *trans, AbtWsiTransportRecord *transport)
 {
  unsigned long          rc;
  int                    onceOnly = FALSE;
  char                   intBuff[35];

/*------------------------------------------------------------------
 * A do-once loop for easier handling of error conditions
 *------------------------------------------------------------------*/
  do
   {
    rc = AbtWscLoadModule(transport->lib,
                           &(transport->moduleHandle));
    if (rc)
     {
      sprintf(intBuff, "%ld", rc);
      AbtWscError(trans, ERR_STRING_ERRORLOADINGLIBRARY, transport->lib, intBuff, "");
      break;
     }

    rc = AbtWscLoadFunction(transport->moduleHandle,
                            ABTWSTINIT_FUNCTION,
                            &(transport->initPtr));

    if (rc)
     {
      sprintf(intBuff, "%ld", rc);
      AbtWscError(trans, ERR_STRING_ERRORLOADINGFUNCTION,
                  ABTWSTINIT_FUNCTION, transport->lib, intBuff);
      break;
     }

    rc = AbtWscLoadFunction(transport->moduleHandle,
                            ABTWSTSENDREQUEST_FUNCTION,
                            &(transport->sendRequestPtr));
    if (rc)
     {
      sprintf(intBuff, "%ld", rc);
      AbtWscError(trans, ERR_STRING_ERRORLOADINGFUNCTION,
                  ABTWSTSENDREQUEST_FUNCTION, transport->lib, intBuff);
      break;
     }

    rc = AbtWscLoadFunction(transport->moduleHandle,
                            ABTWSTGETRESPONSE_FUNCTION,
                            &(transport->getResponsePtr));
    if (rc)
     {
      sprintf(intBuff, "%ld", rc);
      AbtWscError(trans, ERR_STRING_ERRORLOADINGFUNCTION,
                  ABTWSTGETRESPONSE_FUNCTION, transport->lib, intBuff);
      break;
     }

    rc = AbtWscLoadFunction(transport->moduleHandle,
                            ABTWSTRESETTRANSPORT_FUNCTION,
                            &(transport->resetTransportPtr));
    if (rc)
     {
      sprintf(intBuff, "%ld", rc);
      AbtWscError(trans, ERR_STRING_ERRORLOADINGFUNCTION,
                  ABTWSTRESETTRANSPORT_FUNCTION, transport->lib, intBuff);
      break;
     }

   } while (onceOnly);

  return (int)rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*---------------------------------------------------------------------
 * Find a transport record matching this name and return the last match.
 * The transName passed in can contain a single trailing asterisk, '*',
 * for wildcard matching.  The transport name being matched in the
 * transport record is a complete path name without wildcards.
 *---------------------------------------------------------------------*/
AbtWsiTransportRecord * _AbtWsi_Linkage_ findTransportByName(char *transName, AbtWsiConfig *config)
 {
  AbtWsiTransportRecord *next;
  AbtWsiTransportRecord *match = NULL;

  if (config)
   {
    next  = config->firstTransportRecord;
    while (next)
     {
      if (AbtWscMatchWildcardString(next->name, transName))
        match = next;
      next = next->nextRecord;
     }
   }
  return match;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*---------------------------------------------------------------------
 * Find a transport record matching this library name and return the last match.
 * The libName passed in can contain a single trailing asterisk, '*',
 * for wildcard matching.  The library name being matched in the
 * transport record is a complete path name without wildcards.
 *---------------------------------------------------------------------*/
AbtWsiTransportRecord * _AbtWsi_Linkage_ findTransportByLib(char *libName, AbtWsiConfig *config)
 {
  AbtWsiTransportRecord *next;
  AbtWsiTransportRecord *match = NULL;

  if (config)
   {
    next  = config->firstTransportRecord;
    while (next)
     {
      if (AbtWscMatchWildcardString(next->lib, libName))
        match = next;
      next = next->nextRecord;
     }
   }
  return match;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*--------------------------------------------------------------------------
 * Free the property list; given a pointer to the first property in the list
 *--------------------------------------------------------------------------*/
void _AbtWsi_Linkage_ freeProperties(AbtWsiTransaction *trans, AbtWsiElement *properties)
 {
  AbtWsiElement *current = properties;
  AbtWsiElement *next;

  while (current)
   {
    next = current->next;
    freeProperty(trans, current);
    current = next;
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*--------------------------------------------------------------------------
 * Free a single property element
 *--------------------------------------------------------------------------*/
void _AbtWsi_Linkage_ freeProperty(AbtWsiTransaction *trans, AbtWsiElement *propElement)
 {
  if (propElement)
   {
    AbtWscFree(trans, propElement->prop.name);
    AbtWscFree(trans, propElement->prop.value);
    AbtWscFree(trans, propElement);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*--------------------------------------------------------------------------
 * Free a Config structure
 *--------------------------------------------------------------------------*/
void _AbtWsi_Linkage_ freeConfig(AbtWsiTransaction *trans, AbtWsiConfig *config)
 {
  AbtWsiTransportRecord *currentTransport;
  AbtWsiTransportRecord *nextTransport;
  AbtWsiLinkRecord *currentLink;
  AbtWsiLinkRecord *nextLink;

  if (config)
   {
    currentTransport = config->firstTransportRecord;
    while (currentTransport)
     {
      nextTransport = currentTransport->nextRecord;
      freeTransport(trans, currentTransport);
      currentTransport = nextTransport;
     }

    currentLink = config->firstLinkRecord;
    while (currentLink)
     {
      nextLink = currentLink->nextRecord;
      freeLink(trans, currentLink);
      currentLink = nextLink;
     }

    AbtWscFree(trans, config->configFile);
    AbtWscFree(trans, config->errorFile);
    AbtWscFree(trans, config->logFile);
    AbtWscFree(trans, config);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*--------------------------------------------------------------------------
 * Free a single transport record
 *--------------------------------------------------------------------------*/
void _AbtWsi_Linkage_ freeTransport(AbtWsiTransaction *trans, AbtWsiTransportRecord *transport)
 {
  if (transport)
   {
    AbtWscFree(trans, transport->name);
    AbtWscFree(trans, transport->lib);
    AbtWscFree(trans, transport->initData);
    AbtWscFree(trans, transport->data);
    AbtWscFree(trans, transport);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*--------------------------------------------------------------------------
 * Free a single link record
 *--------------------------------------------------------------------------*/
void _AbtWsi_Linkage_ freeLink(AbtWsiTransaction *trans, AbtWsiLinkRecord *link)
 {
  if (link)
   {
    AbtWscFree(trans, link->pathInfo);
    AbtWscFree(trans, link);
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Format the default HTML error message
 *------------------------------------------------------------------*/
char * _AbtWsi_Linkage_ getDefaultMessageFormat(AbtWsiTransaction *trans, char *errorBuffer)
 {
  char *buffer = NULL;

  AbtWscAddToString(trans, &buffer, "<html>\n");
  AbtWscAddToString(trans, &buffer, "<head>\n");
  AbtWscAddToString(trans, &buffer, "<title>");
  AbtWscAddToString(trans, &buffer, MSG_STRING_TITLE);
  AbtWscAddToString(trans, &buffer, "</title>\n");
  AbtWscAddToString(trans, &buffer, "</head>\n");
  AbtWscAddToString(trans, &buffer, "<body>\n");

  AbtWscAddToString(trans, &buffer, "<h1>");
  AbtWscAddToString(trans, &buffer, MSG_STRING_TITLE);
  AbtWscAddToString(trans, &buffer, "</h1>\n\n");

  AbtWscAddToString(trans, &buffer, "<p>");
  AbtWscAddToString(trans, &buffer, MSG_STRING_ERROR_HTTP);
  AbtWscAddToString(trans, &buffer, "\n\n<p>\n");

  AbtWscAddToString(trans, &buffer, errorBuffer);
  AbtWscAddToString(trans, &buffer, "\n");

  AbtWscAddToString(trans, &buffer, "</body>\n");
  AbtWscAddToString(trans, &buffer, "</html>\n");

  return buffer;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
