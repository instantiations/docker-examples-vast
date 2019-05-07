/*------------------------------------------------------------------
 *  abtwstbs.c: VisualAge for Smalltalk, Web Connection,
 *              Buffered socket routines
 *              (C) Copyright IBM Corp. 1996
 *------------------------------------------------------------------*/
#if defined(MEM_DEBUG)
#pragma strings(readonly)
#endif

/*------------------------------------------------------------------
 * Includes
 *------------------------------------------------------------------*/
#include "abtwsi.h"
#include "abtwstbs.h"

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

#ifndef min
   #define min(a,b) ((a) < (b) ? a : b)
#endif

#ifndef max
   #define max(a,b) ((a) > (b) ? a : b)
#endif

/*------------------------------------------------------------------
 *
 *------------------------------------------------------------------*/
BuffSockPtr _AbtWsi_Linkage_ BuffSockCreate(
   AbtWsiTransaction *trans,
   void              *socketData,
   int                readBufferSize,
   int                writeBufferSize,
   BuffSockAccess    *readFunction,
   BuffSockAccess    *writeFunction
   )
   {
   BuffSockPtr   buffSock;

   char         *readBuffer;
   char         *writeBuffer;

   if (!readBufferSize)  readBufferSize  = BUFFSOCK_DEFAULT_BUFFER_SIZE;
   if (!writeBufferSize) writeBufferSize = BUFFSOCK_DEFAULT_BUFFER_SIZE;

   buffSock    = AbtWscMalloc(trans, sizeof(BuffSockStruct));
   readBuffer  = AbtWscMalloc(trans, readBufferSize);
   writeBuffer = AbtWscMalloc(trans, writeBufferSize);

   if (!buffSock || !readBuffer || !writeBuffer)
      {
      if (buffSock)    AbtWscFree(trans, buffSock);
      if (readBuffer)  AbtWscFree(trans, readBuffer);
      if (writeBuffer) AbtWscFree(trans, writeBuffer);

      return NULL;
      }

   buffSock->socketData       = socketData;
   buffSock->readBuffer       = readBuffer;
   buffSock->readBufferSize   = readBufferSize;
   buffSock->readBufferIndex  = 0;
   buffSock->readFunction     = readFunction;
   buffSock->writeBuffer      = writeBuffer;
   buffSock->writeBufferSize  = writeBufferSize;
   buffSock->writeBufferIndex = 0;
   buffSock->writeFunction    = writeFunction;

   return buffSock;
   }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 *
 *------------------------------------------------------------------*/
void _AbtWsi_Linkage_ BuffSockDestroy(
   AbtWsiTransaction *trans,
   BuffSockPtr        buffSock
   )
   {

   if (buffSock)
      {
      AbtWscFree(trans, buffSock->readBuffer);
      AbtWscFree(trans, buffSock->writeBuffer);
      AbtWscFree(trans, buffSock);
      }
   }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 *
 *------------------------------------------------------------------*/
void * _AbtWsi_Linkage_ BuffSockSocket(
   BuffSockPtr    buffSock
   )
   {
   return buffSock->socketData;
   }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 *
 *------------------------------------------------------------------*/
long _AbtWsi_Linkage_ BuffSockRead(
   AbtWsiTransaction *trans,
   BuffSockPtr        buffSock,
   char              *data,
   long               toRead
   )
   {
   int             rc;
   long            thisRead;
   long            dataIndex;
   long            origSize;

   origSize = toRead;

   if (origSize)
    {
     dataIndex = 0;
 
     if (buffSock->readBufferIndex)
        {
        thisRead = min(toRead,buffSock->readBufferIndex);

        memcpy(data,buffSock->readBuffer,thisRead);
        buffSock->readBufferIndex -= thisRead;
        memmove(
           buffSock->readBuffer,
           buffSock->readBuffer + thisRead,
           buffSock->readBufferIndex
           );

        dataIndex += thisRead;
        toRead    -= thisRead;
        }

     /*---------------------------------------------------------------
      * upon entry, we are either finished, or the buffer is empty ...
      *---------------------------------------------------------------*/
     while (dataIndex < origSize)
        {
        rc = buffSock->readFunction(
           buffSock->socketData,
           buffSock->readBuffer,
           buffSock->readBufferSize);
         if (rc <= 0)
           return(rc);

        buffSock->readBufferIndex = rc;
        thisRead = min(buffSock->readBufferIndex,toRead);

        memcpy(
           data + dataIndex,
           buffSock->readBuffer,
           thisRead);

        buffSock->readBufferIndex -= thisRead;
        memmove(
           buffSock->readBuffer,
           buffSock->readBuffer + thisRead,
           buffSock->readBufferIndex
           );

        dataIndex += thisRead;
        toRead   -= thisRead;
        }

     return (origSize);
     }
    return (origSize);
   }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 *
 *------------------------------------------------------------------*/
long _AbtWsi_Linkage_ BuffSockWriteFlush(
   AbtWsiTransaction *trans,
   BuffSockPtr        buffSock
   )
   {
   int             rc;
   int             sent;

   sent = 0;
   while (buffSock->writeBufferIndex)
      {
      rc = buffSock->writeFunction(
         buffSock->socketData,
         buffSock->writeBuffer,
         buffSock->writeBufferIndex);

         if (rc < 0)
         return(rc);
      if (rc == 0)
         return(-1);

      buffSock->writeBufferIndex -= rc;
      sent += rc;

      memmove(
         buffSock->writeBuffer,
         buffSock->writeBuffer + rc,
         buffSock->writeBufferIndex
         );
      }

    return(sent);
   }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
/*-\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/-*/

/*------------------------------------------------------------------
 *
 *------------------------------------------------------------------*/
long _AbtWsi_Linkage_ BuffSockWrite(
   AbtWsiTransaction *trans,
   BuffSockPtr        buffSock,
   char              *data,
   long               toWrite
   )
   {
   int             rc;
   long            thisWrite;
   long            dataIndex;
   long            origSize;

   origSize = toWrite;

   if (origSize)
    {
     dataIndex = 0;

     while (dataIndex < origSize)
        {
        thisWrite = min(buffSock->writeBufferSize,toWrite);

        if (thisWrite + buffSock->writeBufferIndex > buffSock->writeBufferSize)
           {
           rc = BuffSockWriteFlush(trans, buffSock);
           if (rc < 0)
              return(rc);
           }

        memcpy(
           buffSock->writeBuffer + buffSock->writeBufferIndex,
           data + dataIndex,
           thisWrite);

        buffSock->writeBufferIndex += thisWrite; 
        dataIndex += thisWrite;
        toWrite   -= thisWrite;
        }
    }
    return(origSize);
   }

/*-/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\-*/
