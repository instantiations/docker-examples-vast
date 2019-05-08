/*------------------------------------------------------------------
 * abtwscos.h :  VisualAge for Smalltalk, Web Connection,
 *                Web Server Interface, OS-Specific Header
 *                (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#ifndef _ABTWSCOS_H
#define _ABTWSCOS_H

#if defined(OPSYS_AIX)
  #include <macros.h>
#endif
#include <stdlib.h>
#if ( defined(OPSYS_HPUX) || defined(OPSYS_SOLARIS) )
  #include <macros.h>
#endif

#if defined(OPSYS_WIN32)
  #include <windows.h>
  #include <fcntl.h>
  #include <io.h>

  typedef HMODULE AbtModuleHandle;
  typedef FARPROC AbtFunctionPointer;
  #ifdef _MSC_VER
	#define _AbtWsi_Linkage_  __stdcall
  #else
    #ifdef __GNUC__
        #define _AbtWsi_Linkage_
    #else
        #define _AbtWsi_Linkage_  _System
    #endif // __GNUC__
  #endif
#else
  #if defined(OPSYS_OS2)
    #if defined(ABT_INCLUDE_LOAD)
      #define INCL_DOSMODULEMGR
    #endif
    #include <os2.h>
    typedef HMODULE AbtModuleHandle;
    typedef PFN        AbtFunctionPointer;
    #define _AbtWsi_Linkage_  _System
  #else
    typedef void *AbtModuleHandle;
    typedef void *AbtFunctionPointer;
    #define _AbtWsi_Linkage_
  #endif
#endif

#if defined(ABT_INCLUDE_SOCKETS)
  #if defined(OPSYS_WIN32)
    #include <windows.h>
    #include <winsock.h>

   #ifndef NS_NET
      #define SOCKETCLOSE(x) closesocket(x)
   #endif

   #define SOCKET_ERRNO (WSAGetLastError())

   #define WS_VERSION_REQD  0x0101
   #define WS_VERSION_MAJOR HIBYTE(WS_VERSION_REQD)
   #define WS_VERSION_MINOR LOBYTE(WS_VERSION_REQD)
   #define MIN_SOCKETS_REQD 1

  #else
    #if defined(OPSYS_OS2)
      #define OS2
      #include <fcntl.h>
      #include <io.h>
      #include <types.h>
      #include <sys/socket.h>
      #include <netinet/in.h>
      #include <netdb.h>
      #include <arpa/nameser.h>

      #ifdef NS_NET
        #define NET_WINSOCK
      #else
         #define SOCKETCLOSE(x) soclose(x)
      #endif

      #define SOCKET_ERRNO (sock_errno())
    #else
      #include <sys/types.h>
      #include <errno.h>
      #include <sys/socket.h>
      #include <netinet/in.h>
      #include <netdb.h>
      #include <arpa/inet.h>

      #ifndef NS_NET
        #define SOCKETCLOSE(x) close(x)
      #endif

      #define SOCKET_ERRNO (errno)
    #endif
  #endif
#endif

#if defined(ABT_INCLUDE_GETCWD)
/*------------------------------------------------------------------
 * pick up getcwd(char *,int)
 *------------------------------------------------------------------*/
  #if defined(OPSYS_OS2) || defined(OPSYS_WIN32)
     #include <direct.h>
  #else
     #include <unistd.h>
  #endif
#endif

#if defined(ABT_INCLUDE_LOAD)
/*------------------------------------------------------------------
 * for AIX ...
 *------------------------------------------------------------------*/
 #if defined(OPSYS_AIX)
    #include <sys/types.h>
    #include <errno.h>
    #include <sys/ldr.h>

    typedef AbtFunctionPointer (* PF_ABTGETPROC)(char * procName);
  #endif

/*------------------------------------------------------------------
 * for OE ...
 *------------------------------------------------------------------*/
  #if defined(OPSYS_OE)
     #pragma csect(CODE,"IMWLLDSR")
     #pragma comment(date)
     #pragma variable(IMWID,NORENT)
     static char IMWID[] = "IMWLLDSR " __FILE__ ;

     #include <dll.h>
  #endif

/*------------------------------------------------------------------
 * for Solaris and Linux ...
 *------------------------------------------------------------------*/
  #if defined(OPSYS_SOLARIS) | defined(OPSYS_LINUX)
     #include <dlfcn.h>
  #endif

/*------------------------------------------------------------------
 * for HP-UX ...
 *------------------------------------------------------------------*/
  #if defined(OPSYS_HPUX)
     #include <dl.h>
  #endif

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * External function prototypes
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 * load a module, return non-zero on error
 *------------------------------------------------------------------*/
  unsigned long _AbtWsi_Linkage_ AbtWscLoadModule(char *moduleName,
                                       AbtModuleHandle *moduleHandle);

/*------------------------------------------------------------------
 * load a function, return non-zero on error
 *------------------------------------------------------------------*/
  unsigned long _AbtWsi_Linkage_ AbtWscLoadFunction(AbtModuleHandle moduleHandle,
                                         char *functionName,
                                         AbtFunctionPointer *functionPointer);
  #endif
#endif
/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/
