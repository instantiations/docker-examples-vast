#------------------------------------------------------------------
# abtwsi make file for Sun Solaris 2.x
#------------------------------------------------------------------
#
CC = cc
LD = $(CC)
#
# CFLAGS:		C compiler flags
#
#	-c		Compile only
#	-KPIC		Create position-independent code
#	-mt		Compile for multi-threading
#	-O		Level 2 optimization
#	-g		Add debug info'
#	-Xa		ANSI C compatibility
#
CFLAGS    = -KPIC -mt -O -Xa -DOPSYS_SOLARIS
SO_CFLAGS = $(CFLAGS) -c  
#
# LFLAGS:		Linker flags
#
#	-G		Create shared library
#	-mt		Link for multi-threading
#	-i		Ignore LD_LIBRARY_PATH during link
#	-z defs		Fail on undefined symbols
#	-g		Add debug info'
#
LFLAGS = -G -mt 
#
LIBS = -L.:/usr/lib -lc
#
NETSCAPE_INCLUDES = -I/usr/ns-home/nsapi/include -I/usr/ns-home/nsapi/include/base -I/usr/ns-home/nsapi/include/frame
ICS_INCLUDES = -I/opt/IBMICS/usr/samples/internet_server/api
#
ALL: abtwsc.so abtwstt.so abtwsac abtwsai.so abtwsan.so
#
#------------------------------------------------------------------
# Core Functions Shared Object
#------------------------------------------------------------------

obj/abtwsc.o: ../source/abtwsc.c \
	../source/abtwscos.h \
	../source/abtwsi.h
	$(CC) $(SO_CFLAGS) ../source/abtwsc.c -o obj/abtwsc.o

obj/abtwscos.o: ../source/abtwscos.c \
	../source/abtwscos.h \
	../source/abtwsi.h
	$(CC) $(SO_CFLAGS) ../source/abtwscos.c -o obj/abtwscos.o
    
abtwsc.so: obj/abtwsc.o obj/abtwscos.o
	$(LD) $(LFLAGS) -B symbolic -o $@ obj/abtwscos.o obj/abtwsc.o $(LIBS) -ldl 

#------------------------------------------------------------------
# Base TCP/IP Transport Functions Shared Object
#------------------------------------------------------------------

obj/abtwstt.o: ../source/abtwstt.c \
	../source/abtwstbs.h \
	../source/abtwsi.h
	$(CC) $(SO_CFLAGS) ../source/abtwstt.c -o obj/abtwstt.o
    
obj/abtwstbs.o: ../source/abtwstbs.c \
	../source/abtwstbs.h \
	../source/abtwsi.h
	$(CC) $(SO_CFLAGS) ../source/abtwstbs.c -o obj/abtwstbs.o
    
abtwstt.so: obj/abtwstt.o obj/abtwstbs.o 
	$(LD) $(LFLAGS) -B symbolic -o $@ obj/abtwstt.o obj/abtwstbs.o $(LIBS) -lnsl -lmp -lsocket abtwsc.so
	  
#------------------------------------------------------------------
# CGI Executable
#------------------------------------------------------------------

abtwsac: ../source/abtwsac.c ../source/abtwsi.h
	$(CC) $(CFLAGS) ../source/abtwsac.c -o $@ $(LIBS) abtwsc.so

#------------------------------------------------------------------
# IBM ICS Adapter Shared Object
#------------------------------------------------------------------

obj/abtwsai.o: ../source/abtwsai.c \
	../source/abtwsi.h
	$(CC) $(SO_CFLAGS) $(ICS_INCLUDES) ../source/abtwsai.c -o obj/abtwsai.o
    
abtwsai.so: obj/abtwsai.o
	$(LD) $(LFLAGS) -o $@ obj/abtwsai.o $(LIBS) -lpthread abtwsc.so /usr/lib/libhttpdapi.so 
	
#------------------------------------------------------------------
# Netscape NSAPI Adapter Shared Object
#------------------------------------------------------------------

obj/abtwsan.o: ../source/abtwsan.c \
	../source/abtwsi.h
	$(CC) $(SO_CFLAGS) $(NETSCAPE_INCLUDES) -DNETSSL -DSOLARIS -D_REENTRANT DMCC_HTTPD -DXP_UNIX ../source/abtwsan.c -o obj/abtwsan.o

abtwsan.so: obj/abtwsan.o 
	$(LD) $(LFLAGS) -o $@ obj/abtwsan.o $(LIBS) -lnsl abtwsc.so

#------------------------------------------------------------------