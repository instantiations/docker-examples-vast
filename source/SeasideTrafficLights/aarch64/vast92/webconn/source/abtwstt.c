/*------------------------------------------------------------------
 * abtwstt.c : VisualAge for Smalltalk, Web Connection,
 *             Web Server Interface, TCP/IP Transport
 *             (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#if defined(MEM_DEBUG)
#pragma strings(readonly)
#endif

/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#define ABT_INCLUDE_SOCKETS
#include "abtwsi.h"   /* common include                             */
#include "abtwstbs.h" /* common include                             */

#ifdef NS_NET
   #include <base/net.h>

  #define SOCKTYPE    SYS_NETFD
  #define SOCKPTR     SYS_NETFD *

  #define GET_SOCKET  net_socket
  #define SETSOCKOPT  net_setsockopt
  #define CONNECT     net_connect

  #define SOCKETCLOSE net_close

  #define READ(sock,buf,bufLen)  net_read(sock,buf,bufLen,300)
  #define WRITE(sock,buf,bufLen) net_write(sock,buf,bufLen)
#else
  #define SOCKTYPE    int
  #define SOCKPTR     int *

  #define GET_SOCKET  socket
  #define SETSOCKOPT  setsockopt
  #define CONNECT     connect

  #define READ(sock,buf,bufLen)  recv(sock,buf,bufLen,0)
  #define WRITE(sock,buf,bufLen) send(sock,buf,bufLen,0)
#endif

/*------------------------------------------------------------------
 * Define some WinSock-isms, if not already defined
 *------------------------------------------------------------------*/
#if !defined(INVALID_SOCKET)
   #define INVALID_SOCKET -1
#endif

#if !defined(SOCKET_ERROR)
   #define SOCKET_ERROR -1
#endif

#if !defined(INADDR_NONE)
   #define INADDR_NONE -1
#endif

/*------------------------------------------------------------------
 * Other definitions
 *------------------------------------------------------------------*/
#define MAX_BUFFER_SIZE       8192
#define MSG_STRING_SELF       "-"

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * TCP/IP Error Messages
 *------------------------------------------------------------------*/
#define ERR_STRING_CREATINGSOCKET        "An error occurred creating the TCP/IP socket; errno = %s."
#define ERR_STRING_RESOLVINGHOST         "An error occurred resolving the host name '%s'; errno = %s. "
#define ERR_STRING_ERRORONRECV           "An error occurred receiving data; rc = %s, errno = %s."
#define ERR_STRING_ERRORONSENDSOCKET     "An error occurred sending data on the TCP/IP socket; socket id = %s, errno = %s."
#define ERR_STRING_ERRORONRECVSOCKET     "An error occurred receiving data on the TCP/IP socket; socket id = %s, errno = %s."
#define ERR_STRING_SERVERCLOSEDSOCKET    "The server prematurely closed the TCP/IP socket; socket id = %s."
#define ERR_STRING_GETTINGLOCALHOSTNAME  "Unable to determine the local host name for this machine."
#define ERR_STRING_WINSOCKINIT           "An error occurred initializing WinSock; rc = %s, errno = %s."
#define ERR_STRING_WINSOCKVERSION        "An invalid WinSock version %s.%s was found;  version 1.1, or higher, is required."
#define ERR_STRING_BADPORTNUMBER         "A invalid port number '%s' was specified for the '%s' transport in the configuration file."
#define ERR_STRING_INVALIDTRANSDATA      "Invalid data for the '%s' transport was specified in the configuration file; hostname = %s, port = %s."
#define ERR_STRING_NOCONNECT_TCP         "Unable to connect to host at port %s; errno = %s."

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Global Config pointer
 *------------------------------------------------------------------*/
AbtWsiConfig *Config = NULL;

/*------------------------------------------------------------------
 * Internal Function prototypes
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ ReadSocket(void  *socket, char *buffer, int bufferLength);
int _AbtWsi_Linkage_ WriteSocket(void *socket, char *buffer, int bufferLength);

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

int _AbtWsi_Linkage_ AbtWstInit(AbtWsiTransaction *trans, AbtWsiConfig *config, AbtWsiTransportRecord *transRecord)
 {
  int                 rc = 0;
  char               *hostName = NULL;
  char               *sPort = NULL;
  char               *dummy;
  unsigned long       ulPort;
  unsigned short      port;
  char                localHostName[MAX_LINE_LEN+1];
  unsigned long       hostAddr;
  struct sockaddr_in *sockAddr;
  char               *initCopy = NULL;
  char                intBuff1[35];

  if (!Config) 
   {
    Config = config;

  /*---------------------------------------------------------------
  * winsock initialization (Win32 only)
  *---------------------------------------------------------------*/
#if defined(OPSYS_WIN32)
    {
     WSADATA wsaData;
     char    intBuff2[35];

     rc = WSAStartup(WS_VERSION_REQD, &wsaData);  

     if (rc)
      {
       sprintf(intBuff1, "%d", rc);
       sprintf(intBuff2, "%d", SOCKET_ERRNO);
       AbtWscError(trans, ERR_STRING_WINSOCKINIT, intBuff1, intBuff2, "");
       return -1;
      }

     if ((LOBYTE(wsaData.wVersion) < WS_VERSION_MAJOR) ||
       (LOBYTE(wsaData.wVersion) == WS_VERSION_MAJOR &&
        HIBYTE(wsaData.wVersion) < WS_VERSION_MINOR))
      {
       sprintf(intBuff1, "%d", LOBYTE(wsaData.wVersion));
       sprintf(intBuff2, "%d", HIBYTE(wsaData.wVersion));
       AbtWscError(trans, ERR_STRING_WINSOCKVERSION, intBuff1, intBuff2, "");
       return -1;
      }
    }
#else
  #if defined(OPSYS_OS2)
    sock_init();
  #endif
#endif
   }

  /*---------------------------------------------------------------
   * parse up the init data
   *---------------;------------------------------------------------*/
  AbtWscAddToString(trans, &initCopy, transRecord->initData);
  if (!initCopy)
    return -1;

  hostName = strtok(initCopy, WHITESPACE);
  sPort    = strtok(NULL, WHITESPACE);

  if (!hostName || !sPort ||!*hostName ||!*sPort)
   {
    AbtWscError(trans, ERR_STRING_INVALIDTRANSDATA, trans->transport->name, hostName, sPort);
    return -1;
   }

  ulPort = strtoul(sPort, &dummy, 10);

  if ( *dummy || (ulPort > 65536) )
   {
    AbtWscError(trans, ERR_STRING_BADPORTNUMBER, sPort, trans->transport->name, "");
    return -1;
   }

  port = (unsigned short) ulPort;

  /*---------------------------------------------------------------
   * resolve hostname
   *---------------------------------------------------------------*/
  if (!strcmp(MSG_STRING_SELF, hostName))
   {
    rc = gethostname(localHostName, MAX_LINE_LEN);
    if (SOCKET_ERROR == rc)
     {
      AbtWscError(trans, ERR_STRING_GETTINGLOCALHOSTNAME, "", "", "");
      return -1;
     }
    hostName = localHostName;
   }

  AbtWscDebugMessage(trans, "Resolving hostname for ", hostName);

  hostAddr = inet_addr(hostName);
  
  if (INADDR_NONE == hostAddr)
   {
#if defined(OPSYS_WIN32)
    struct hostent *hostEnt = gethostbyname(hostName);
    if (!hostEnt)
     {
      sprintf(intBuff1, "%d", SOCKET_ERRNO);
      AbtWscError(trans, ERR_STRING_RESOLVINGHOST, hostName, intBuff1, "");
      return -1;
     }
#else
  #if defined(OPSYS_SOLARIS)
    struct hostent  hostEntStruct;
    struct hostent  *hostEnt;
    int errnop = 0;
    char *dummy;
    
    hostEnt = gethostbyname(hostName);
    
    if (!hostEnt)
     {
      hostEnt = gethostbyname_r(hostName, &hostEntStruct, dummy, 0, &errnop);
    
      if (!hostEnt)
       {
        if (errnop)
          sprintf(intBuff1, "%d", errnop);
        else 
          sprintf(intBuff1, "%d", SOCKET_ERRNO);
      
        AbtWscError(trans, ERR_STRING_RESOLVINGHOST, hostName, intBuff1, "");
        return -1;
       }
     } 
#else
  #if defined(OPSYS_LINUX)
    struct hostent  hostEntStruct;
    struct hostent  *hostEnt;
    int res;
    int errnop = NULL;
    char *dummy;
    
    hostEnt = gethostbyname(hostName);
    
    if (!hostEnt)
     {
      res = gethostbyname_r(hostName, &hostEntStruct, dummy, 0, &hostEnt, &errnop);
    
      if (!hostEnt)
       {
        if (errnop)
          sprintf(intBuff1, "%d", errnop);
        else 
          sprintf(intBuff1, "%d", SOCKET_ERRNO);
      
        AbtWscError(trans, ERR_STRING_RESOLVINGHOST, hostName, intBuff1, "");
        return -1;
       }
     } 
#else
    struct hostent      hostEntStruct;
    struct hostent     *hostEnt;
    struct hostent_data data;
    
    hostEnt = gethostbyname(hostName);
    
    if (!hostEnt)
     {
      rc = gethostbyname_r(hostName, &hostEntStruct, &data);
       
      if (rc)
       {
        sprintf(intBuff1, "%d", SOCKET_ERRNO);
        AbtWscError(trans, ERR_STRING_RESOLVINGHOST, hostName, intBuff1, "");
        return -1;
       }
      else
        hostEnt = &hostEntStruct; 
     }
    #endif  
  #endif 
#endif

    hostAddr = *((unsigned long *)(hostEnt->h_addr));
   }

  /*---------------------------------------------------------------
   * create address
   *---------------------------------------------------------------*/

  sockAddr = AbtWscMalloc(trans, sizeof(struct sockaddr_in));
  if (!sockAddr)
    return -1;

  sockAddr->sin_family      = AF_INET;
  sockAddr->sin_port        = htons(port);
  sockAddr->sin_addr.s_addr = hostAddr;

  transRecord->data = (void *)sockAddr;

  AbtWscFree(trans, initCopy);

  return rc;
 }
/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

void _AbtWsi_Linkage_ AbtWstResetTransport(AbtWsiTransaction *trans)
 {
  if (Config) 
   {
    Config = NULL;
  /*---------------------------------------------------------------
   * winsock termination (Win32 only)
   *---------------------------------------------------------------*/
    #if defined(OPSYS_WIN32)
      WSACleanup( );
    #endif
   }
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Process the request via TCP/IP transport services
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWstSendRequest(AbtWsiTransaction *trans)
 {
  int                 rc;
  int                 ret = 0;
  SOCKPTR             sock;
  int                 tcpNoDelay;
  int                 socketLinger;
  AbtWsiElement      *current;
  unsigned long       length;
  unsigned long       ulongBuff;
  BuffSockPtr         buffSock;
  char                intBuff1[35];
  char                intBuff2[35];
  int                 onceOnly = FALSE;
  struct sockaddr_in *sockAddr = (struct sockaddr_in *)trans->transport->data;

 /*---------------------------------------------------------------
  * A do-once loop for handling write errors
  *-------------------------------------/--------------------------*/
  do
   {
    /*---------------------------------------------------------------
     * Allocate and open socket
     *---------------------------------------------------------------*/
    sock = AbtWscMalloc(trans, sizeof(SOCKTYPE));
    if (!sock)
     {
      ret = -1;
      break;
     }

    *sock = GET_SOCKET(AF_INET,SOCK_STREAM,0);
    if (INVALID_SOCKET == *sock)
     {
      sprintf(intBuff1, "%d", SOCKET_ERRNO);
      AbtWscError(trans, ERR_STRING_CREATINGSOCKET, intBuff1, "", "");
      ret = -1;
      break;
     }

    /*---------------------------------------------------------------
     * create the buffered socket structure
     *---------------------------------------------------------------*/
    buffSock = BuffSockCreate(trans, sock, MAX_BUFFER_SIZE,MAX_BUFFER_SIZE,ReadSocket,WriteSocket);
    if (!buffSock)
     {
      ret = -1;
      break;
     }
 
    /*---------------------------------------------------------------
     * connect to server
     *---------------------------------------------------------------*/
    AbtWscDebugMessage(trans, "Connecting to server", "");

    if (CONNECT(*sock, (struct sockaddr *)sockAddr, sizeof(struct sockaddr_in)))
     {
      AbtWscSetStatus(trans, 503, MSG_STRING_UNAVAILABLE);
      sprintf(intBuff1, "%d", ntohs(sockAddr->sin_port));
      sprintf(intBuff2, "%d", SOCKET_ERRNO);
      AbtWscError(trans, ERR_STRING_NOCONNECT_TCP, intBuff1, intBuff2, "");
      ret = -1;
      break;
     }

    /*---------------------------------------------------------------
     * set tcpNoDelay
     *---------------------------------------------------------------*/
    tcpNoDelay = 1;
    SETSOCKOPT(*sock,IPPROTO_TCP,1, (char *)&tcpNoDelay, sizeof(tcpNoDelay));

   /*  PMR 28076,111,000   */
	#if defined(OPSYS_WIN32)
    /*---------------------------------------------------------------
     * set socketLinger
     *---------------------------------------------------------------*/
		socketLinger = 0;
    	SETSOCKOPT(*sock,SOL_SOCKET,SO_DONTLINGER, (char *)&socketLinger, sizeof(socketLinger));
	#endif

    /*---------------------------------------------------------------
     * send message header info'
     *---------------------------------------------------------------*/
    AbtWscDebugMessage(trans, "Sending magic cookie", "");
    rc = BuffSockWrite(trans, buffSock, (char *)&(trans->req.hdr.cookie), 4);
    if (rc < 0) break;

    AbtWscDebugMessage(trans, "Sending version info", "");
    ulongBuff = htonl(trans->req.hdr.major);
    rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);
    if (rc < 0) break;

    ulongBuff = htonl(trans->req.hdr.minor);
    rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);
    if (rc < 0) break;

    AbtWscDebugMessage(trans, "Sending id", "");
    ulongBuff = htonl(trans->req.hdr.id);
    rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);
    if (rc < 0) break;

    AbtWscDebugMessage(trans, "Sending type", "");
    ulongBuff = htonl(trans->req.hdr.type);
    rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);
    if (rc < 0) break;

	/*--------------------------------------------------------
	* find number of key/value pairs
	*--------------------------------------------------------*/
	ulongBuff = 0L;
	current = trans->req.data.firstProperty;
	
	while (current)
	{
		ulongBuff++;
		current = current->next;
	}

    /*---------------------------------------------------------
     * send number of key/value pairs
     *---------------------------------------------------------*/

    sprintf(intBuff1, "%ld", ulongBuff);
    AbtWscDebugMessage(trans, "Sending number of keys: ", intBuff1);

/*    ulongBuff = htonl(trans->req.data.propertyCount); */
	ulongBuff = htonl(ulongBuff);
    rc = BuffSockWrite(trans, buffSock,(char *)&ulongBuff,4);
    if (rc < 0) break;

    /*---------------------------------------------------------
     * send codepage string
     *---------------------------------------------------------*/
    if (trans->req.hdr.codePage && *(trans->req.hdr.codePage))
     {
      length = strlen(trans->req.hdr.codePage);
      ulongBuff = htonl(length);
      rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);
      if (rc < 0) break;
      rc = BuffSockWrite(trans, buffSock, trans->req.hdr.codePage, length);
      if (rc < 0) break;
     }
    else
     {
      ulongBuff = 0L;
      rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);
      if (rc < 0) break;
     }

    /*-------------------------------------------------------
     * send key/value pairs
     *-------------------------------------------------------*/
    AbtWscDebugMessage(trans, "Sending keys", "");
    current = trans->req.data.firstProperty;

    while (current)
     {
      length = strlen(current->prop.name);
      ulongBuff = htonl(length);
      rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);
      if (rc < 0) break;
      rc = BuffSockWrite(trans, buffSock, current->prop.name, length);
      if (rc < 0) break;

      length = strlen(current->prop.value);
      ulongBuff = htonl(length);
      rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);
      if (rc < 0) break;
      rc = BuffSockWrite(trans, buffSock, current->prop.value, length);
      if (rc < 0) break;

      current = current->next;
     }
    if (rc < 0) break;

   /*----------------------------------------------------------
    * send contentData and length
    *----------------------------------------------------------*/
    sprintf(intBuff1, "%ld", trans->req.data.contentLength);
    AbtWscDebugMessage(trans, "Starting to send content; length = ", intBuff1);
    ulongBuff = htonl(trans->req.data.contentLength);
    rc = BuffSockWrite(trans, buffSock, (char *)&ulongBuff, 4);

    if (rc < 0) break;

    rc = BuffSockWrite(trans, buffSock, trans->req.data.contentData,
                       trans->req.data.contentLength);
    if (rc < 0) break;

    AbtWscDebugMessage(trans, "Finished sending content", "");
    /*------------------------------------------------------
     * flush to send remaining data
     *------------------------------------------------------*/
    rc = BuffSockWriteFlush(trans, buffSock);
    if (rc < 0) break;

    AbtWscDebugMessage(trans, "Flushed buffers", "");

   } while (onceOnly);

  if (!ret)
   {  
    if (SOCKET_ERROR == rc)
     {
      sprintf(intBuff1, "%p", sock);
      sprintf(intBuff2, "%d", SOCKET_ERRNO);
      AbtWscError(trans, ERR_STRING_ERRORONSENDSOCKET, intBuff1, intBuff2, "");
      ret = -1;
     }
    else if (0 == rc)
     {
      sprintf(intBuff1, "%p", sock);
      AbtWscError(trans, ERR_STRING_SERVERCLOSEDSOCKET, intBuff1, "", "");
      ret = -1;
     }
   }

  if (!ret)
   {  
    /*------------------------------------------------------
     * save the buffSock
     *------------------------------------------------------*/
    trans->transData = (void *)buffSock;
    AbtWscDebugMessage(trans, "Finished Send Request", "");
   }
  else
   {
    AbtWscDebugMessage(trans, "Closing socket", "");
    if (sock)
     {
      if (INVALID_SOCKET != *sock)
        SOCKETCLOSE(*sock);
      AbtWscFree(trans, sock);
     }

    if (buffSock) 
      BuffSockDestroy(trans, buffSock);
   } 

  return ret;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Get the response via TCP/IP transport services
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ AbtWstGetResponse(AbtWsiTransaction *trans)
 {
  int             rc;
  int             count;
  SOCKPTR         sockPtr;
  char           *name;
  char           *value;
  unsigned long   length;
  unsigned long   ulongBuff;
  unsigned long   propCount;
  BuffSockPtr     buffSock;
  char            intBuff1[35];
  char            intBuff2[35];
  int             onceOnly = FALSE;

  buffSock = (BuffSockPtr)trans->transData;

  /*---------------------------------------------------------------
   * receive response, write back to adapter
   *-----------------------------------------------------------------*/
  AbtWscDebugMessage(trans, "Waiting for response", "");

   /*---------------------------------------------------------------
    * A do-once loop for handling errors
    *-------------------------------------/--------------------------*/
   do
    {
    /*---------------------------------------------------------------
     * receive message header info'
     *---------------------------------------------------------------*/
     rc = BuffSockRead(trans, buffSock, (char *)&ulongBuff, 4);
     if (rc != 4) break;
     trans->resp.hdr.id = ntohl(ulongBuff);
     AbtWscDebugMessage(trans, "Received id", "");

     rc = BuffSockRead(trans, buffSock, (char *)&ulongBuff, 4);
     if (rc != 4) break;
     trans->resp.hdr.type = ntohl(ulongBuff);
     AbtWscDebugMessage(trans, "Received type", "");

     /*---------------------------------------------------------
      * receive number of key/value pairs
      *---------------------------------------------------------*/
     rc = BuffSockRead(trans, buffSock, (char *)&ulongBuff, 4);
     if (rc != 4) break;
     propCount = ntohl(ulongBuff);
     AbtWscDebugMessage(trans, "Received number of keys/values", "");

     /*-------------------------------------------------------
      * receive key/value pairs
      *-------------------------------------------------------*/
     AbtWscDebugMessage(trans, "Receiving keys/values", "");

     for (count = 0; (unsigned long)count < propCount; count++)
      {
       rc = BuffSockRead(trans, buffSock, (char *)&ulongBuff, 4);
       if (rc != 4) break;
       length = ntohl(ulongBuff);
       name = AbtWscMalloc(trans, length+1);
       if (!name)
        {
         rc = -1;
         break;
        }
       rc = BuffSockRead(trans, buffSock, name, length);
       if (rc != length) break;
       name[length] = '\0';

       rc = BuffSockRead(trans, buffSock, (char *)&ulongBuff, 4);
       if (rc != 4) break;
       length = ntohl(ulongBuff);
       value = AbtWscMalloc(trans, length+1);
       if (!value)
        {
         rc = -1;
         break;
        }
       rc = BuffSockRead(trans, buffSock, value, length);
       if (rc != length) break;
       value[length] = '\0';

       AbtWscAddProperty(trans, name, value, &(trans->resp.data));
      }
     if (rc <= 0) break;

    /*----------------------------------------------------------
     * receive the contentData
     *----------------------------------------------------------*/
     rc = BuffSockRead(trans, buffSock, (char *)&ulongBuff, 4);
     if (rc != 4) break;
     trans->resp.data.contentLength = ntohl(ulongBuff);
     trans->resp.data.contentData = AbtWscMalloc(trans, trans->resp.data.contentLength + 1);
     if (!trans->resp.data.contentData)
      {
       rc = -1;
       break;
      }
     rc = BuffSockRead(trans, buffSock, trans->resp.data.contentData, trans->resp.data.contentLength);
     if (rc != trans->resp.data.contentLength) break;
     trans->resp.data.contentData[trans->resp.data.contentLength] = '\0';
     AbtWscDebugMessage(trans, "Received content", "");

   } while (onceOnly);

  /*---------------------------------------------------------------
   * close socket and free memory
   *---------------------------------------------------------------*/
  sockPtr = (SOCKPTR)(BuffSockSocket(buffSock));

  if (rc == SOCKET_ERROR)
   {
    sprintf(intBuff1, "%d", *sockPtr);
    sprintf(intBuff2, "%d", SOCKET_ERRNO);
    AbtWscError(trans, ERR_STRING_ERRORONRECVSOCKET, intBuff1, intBuff2, "");
   }
  else if (rc <= 0)
   {
    sprintf(intBuff1, "%d", rc);
    sprintf(intBuff2, "%d", SOCKET_ERRNO);
    AbtWscError(trans, ERR_STRING_ERRORONRECV, intBuff1, intBuff2, "");
   }

  AbtWscDebugMessage(trans, "Closing socket", "");
  SOCKETCLOSE(*sockPtr);

  BuffSockDestroy(trans, buffSock);
  AbtWscFree(trans, sockPtr);

  if (rc == 0)
    rc = -1;
  else if (rc > 0)
    rc = 0;

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Read data from the socket
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ ReadSocket(void  *socket, char *buffer, int bufferLength)
 {
  return(READ(*((SOCKPTR)socket), buffer, bufferLength));
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Write data to the socket
 *------------------------------------------------------------------*/
int _AbtWsi_Linkage_ WriteSocket(void *socket, char *buffer, int bufferLength)
 {
  return(WRITE(*((SOCKPTR)socket), buffer, bufferLength));
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

AbtFunctionPointer AbtGetProc(char * procName)
 {
  if (strcmp(procName, "AbtWstGetResponse") == 0  )
    return (AbtFunctionPointer)AbtWstGetResponse;
  else if (strcmp(procName, "AbtWstInit") == 0  )
    return (AbtFunctionPointer)AbtWstInit;
  else if (strcmp(procName, "AbtWstResetTransport") == 0  )
    return (AbtFunctionPointer)AbtWstResetTransport;
  else if (strcmp(procName, "AbtWstSendRequest") == 0  )
    return (AbtFunctionPointer)AbtWstSendRequest;
  else  
    return (AbtFunctionPointer)NULL;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/


