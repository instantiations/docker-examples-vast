/*------------------------------------------------------------------
 * abtwscos.c :  VisualAge for Smalltalk, Web Connection,
 *               Web Server Interface, OS-specific Functions
 *               (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#if defined(MEM_DEBUG)
#pragma strings(readonly)
#endif

/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#define ABT_INCLUDE_LOAD
#include "abtwsi.h"
    
#if defined(OPSYS_AIX)
  AbtFunctionPointer *getProcAddr;
#endif

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * load a module, return non-zero on error
 *------------------------------------------------------------------*/
unsigned long _AbtWsi_Linkage_ AbtWscLoadModule(char *moduleName,
                                     AbtModuleHandle *moduleHandle)
 {
  unsigned long rc = (unsigned long)NULL;
  int length = 256;
  char *buffer = (char *)malloc(256);

  if ( (NULL == moduleName) || ('\0' == *moduleName) || (NULL == moduleHandle) )
    return(1);

  *moduleHandle = NULL;

  /*---------------------------------------------------------------
   * OS/2
   *---------------------------------------------------------------*/
#if defined(OPSYS_OS2)
  {
   char *lastOne = strrchr(moduleName, '.');
   
   if (lastOne)
    {
     if (!(AbtStricmp(lastOne, ".DLL")))
       *lastOne = NULL;
    }

   rc = DosLoadModule(NULL,0,moduleName,moduleHandle);
  }
#endif

  /*---------------------------------------------------------------
   * Win32
   *---------------------------------------------------------------*/
#if defined(OPSYS_WIN32)
  *moduleHandle = LoadLibrary(moduleName);
#endif

  /*---------------------------------------------------------------
   * AIX
   *---------------------------------------------------------------*/
#if defined(OPSYS_AIX)
  *moduleHandle = (AbtModuleHandle)load(moduleName,NULL,NULL);
#endif

  /*---------------------------------------------------------------
   * OE
   *---------------------------------------------------------------*/
#if defined(OPSYS_OE)
  *moduleHandle = dllload(moduleName);
#endif

  /*---------------------------------------------------------------
   * Linux
   *---------------------------------------------------------------*/
#if defined(OPSYS_LINUX)
  *moduleHandle = (AbtModuleHandle)dlopen(moduleName, RTLD_LAZY | RTLD_GLOBAL);
#endif

  /*---------------------------------------------------------------
   * Solaris
   *---------------------------------------------------------------*/
#if defined(OPSYS_SOLARIS)
  *moduleHandle = (AbtModuleHandle)dlopen(moduleName, RTLD_LAZY | RTLD_GLOBAL);
#endif

  /*---------------------------------------------------------------
   * HP
   *---------------------------------------------------------------*/
#if defined(OPSYS_HPUX)
  *moduleHandle = (AbtModuleHandle)shl_load(moduleName, BIND_VERBOSE | BIND_IMMEDIATE | DYNAMIC_PATH, 0L);
#endif

  if (!rc)
    rc = (NULL == *moduleHandle);

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * load a function, return non-zero on error
 *------------------------------------------------------------------*/
unsigned long _AbtWsi_Linkage_ AbtWscLoadFunction(AbtModuleHandle moduleHandle,
                                 char *functionName,
                                 AbtFunctionPointer *functionPointer)
 {
  unsigned long rc = (unsigned long)NULL;

  if ( (NULL == moduleHandle)  || (NULL == functionName)   ||
       ('\0' == *functionName) || (NULL == functionPointer) )
    return(1);

  *functionPointer = NULL;

  /*---------------------------------------------------------------
   * OS/2
   *---------------------------------------------------------------*/
#if defined(OPSYS_OS2)
  rc = DosQueryProcAddr(moduleHandle,0, functionName,functionPointer);
#endif

  /*---------------------------------------------------------------
   * Win32
   *---------------------------------------------------------------*/
#if defined(OPSYS_WIN32)
  *functionPointer = GetProcAddress(moduleHandle, functionName);
#endif

  /*---------------------------------------------------------------
   * AIX
   *---------------------------------------------------------------*/
#if defined(OPSYS_AIX)
  *functionPointer = ((PF_ABTGETPROC)moduleHandle)(functionName);
#endif

  /*---------------------------------------------------------------
   * OE
   *---------------------------------------------------------------*/
#if defined(OPSYS_OE)
  *functionPointer = (int (*)())dllqueryfn(moduleHandle,functionName);
#endif

  /*---------------------------------------------------------------
   * Solaris
   *---------------------------------------------------------------*/
#if defined(OPSYS_SOLARIS) | defined(OPSYS_LINUX)
  *functionPointer = dlsym(moduleHandle,functionName);
#endif

  /*---------------------------------------------------------------
   * HP
   *---------------------------------------------------------------*/
#if defined(OPSYS_HPUX)
  rc = shl_findsym((shl_t *)&moduleHandle,functionName,TYPE_PROCEDURE,functionPointer);
  if (rc == -1)
    rc = errno;
#endif
  
  if (!rc)
    rc = (NULL == *functionPointer);

  return rc;
 }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
