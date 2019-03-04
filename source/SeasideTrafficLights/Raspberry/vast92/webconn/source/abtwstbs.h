/*-------------------------------------------------------------------
 *  abtwstbs.h: VisualAge for Smalltalk, Web Connection,
 *              Buffered socket routines, header file
 *              (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#ifndef _ABTWSTBS_H
#define _ABTWSTBS_H

/*------------------------------------------------------------------
 * default buffer size
 *------------------------------------------------------------------*/
#define BUFFSOCK_DEFAULT_BUFFER_SIZE 4096

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Defines functions to read and write to the 'socket'.  One function
 * will be used to model recv(), the other send().  Both functions
 * take the user-defined socketData pointer, pointer to a buffer,
 * and amount to read or write.  The function should return the
 * number of bytes written or read, 0 if the 'socket' is closed, or
 * -1 if an error occurred.
 *------------------------------------------------------------------*/
typedef int _AbtWsi_Linkage_ BuffSockAccess(
   void         *socketData,
   char         *buffer,
   int           bufferLength
   );

typedef struct
   {
   void           *socketData;
   char           *readBuffer;
   long            readBufferSize;
   long            readBufferIndex;
   BuffSockAccess *readFunction;
   char           *writeBuffer;
   long            writeBufferSize;
   long            writeBufferIndex;
   BuffSockAccess *writeFunction;
   } BuffSockStruct;

/*------------------------------------------------------------------
 * Pointer to a BuffSock object
 *------------------------------------------------------------------*/
typedef BuffSockStruct *BuffSockPtr;

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * prototypes
 *------------------------------------------------------------------*/
BuffSockPtr _AbtWsi_Linkage_ BuffSockCreate(
   AbtWsiTransaction *trans,
   void              *socketData,
   int                readBufferSize,
   int                writeBufferSize,
   BuffSockAccess    *readFunction,
   BuffSockAccess   *writeFunction
   );

void _AbtWsi_Linkage_ BuffSockDestroy(
   AbtWsiTransaction *trans,
   BuffSockPtr        buffSock
   );

void * _AbtWsi_Linkage_ BuffSockSocket(
   BuffSockPtr    buffSock
   );

long _AbtWsi_Linkage_ BuffSockRead(
   AbtWsiTransaction *trans,
   BuffSockPtr        buffSock,
   char              *data,
   long               toRead
   );

long _AbtWsi_Linkage_ BuffSockWriteFlush(
   AbtWsiTransaction *trans,
   BuffSockPtr        buffSock
   );

long _AbtWsi_Linkage_ BuffSockWrite(
   AbtWsiTransaction *trans,
   BuffSockPtr        buffSock,
   char              *data,
   long               toWrite
   );

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 * Global Config pointer; defined in parent transport file
 *------------------------------------------------------------------*/
extern AbtWsiConfig *Config;

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

#endif
