#------------------------------------------------------------------
# abtwsi make file for AIX 4.1.x
#------------------------------------------------------------------
#
# Ansi-C, with threads support
CC = xlc_r
LD = xlc_r
#
#  Add -O for optimization; -g for debug
COPTS = -O -DOPSYS_AIX
SO_COPTS = $(COPTS) -c  
#
SHL_ENTRY = AbtGetProc
#
#Linker Options
# -K		aligns output file sections on page boundaries
# -bro		prevent load-time relocations of text section
# -e  		define module entry point (handle returned by load())
# -bnoentry	no defined module entry point (used for shared libs)
# -bE:		export file, declaring symbols exported by this lib
# -bM:SRE	mark output file as shared
#
LOPTS = -K -bro -bM:SRE 
#
LIBS =-L.
LIBPATH=.
#
NETSCAPE_INCLUDES = -I../../netscape/nsapi/include -I../../netscape/nsapi/include/base -I../../netscape/nsapi/include/frame
#
ALL: abtwsc.a abtwstt.so abtwsac abtwsai.so abtwsan.so
#
#------------------------------------------------------------------
# Core Functions Shared Object
#------------------------------------------------------------------

obj/abtwsc.o: ../source/abtwsc.c \
	../source/abtwscos.h \
	../source/abtwsi.h
	$(CC) $(SO_COPTS) ../source/abtwsc.c -o $@

obj/abtwscos.o: ../source/abtwscos.c \
	../source/abtwscos.h \
	../source/abtwsi.h
	$(CC) $(SO_COPTS) ../source/abtwscos.c -o $@
    
obj/abtwsc.so: obj/abtwsc.o obj/abtwscos.o
	$(LD) $(LOPTS) -bnoentry -bE:abtwsc.exp -bl:obj/abtwsc.bind -o $@ obj/abtwscos.o obj/abtwsc.o $(LIBS)

abtwsc.a: obj/abtwsc.so
	ar -rv $@ obj/abtwsc.so

#------------------------------------------------------------------
# Base TCP/IP Transport Functions Shared Object
#------------------------------------------------------------------

obj/abtwstt.o: ../source/abtwstt.c \
	../source/abtwstbs.h \
	../source/abtwsi.h
	$(CC) $(SO_COPTS) ../source/abtwstt.c -o $@
    
obj/abtwstbs.o: ../source/abtwstbs.c \
	../source/abtwstbs.h \
	../source/abtwsi.h
	$(CC) $(SO_COPTS) ../source/abtwstbs.c -o $@
    
abtwstt.so: obj/abtwstt.o obj/abtwstbs.o 
	$(LD) $(LOPTS) -e$(SHL_ENTRY) -bl:obj/abtwstt.bind -o $@ obj/abtwstt.o obj/abtwstbs.o $(LIBS) abtwsc.a

#------------------------------------------------------------------
# CGI Executable
#------------------------------------------------------------------

abtwsac: ../source/abtwsac.c ../source/abtwsi.h
	$(CC) $(COPTS) -bl:obj/abtwsac.bind ../source/abtwsac.c -o $@ $(LIBS) abtwsc.a

#------------------------------------------------------------------
# IBM ICS Adapter Shared Object
#------------------------------------------------------------------

obj/abtwsai.o: ../source/abtwsai.c \
	../source/abtwsi.h
	cc_r $(SO_COPTS) -I/usr/samples/internet_server/API -qdbxextra -qcpluscmt ../source/abtwsai.c -o $@ 
   
abtwsai.so: obj/abtwsai.o
	cc_r -bM:SRE -bnoentry -o $@ obj/abtwsai.o $(LIBS) abtwsc.a -bl:obj/abtwsai.bind -bI:/usr/samples/internet_server/API/libhttpdapi.exp -bE:abtwsai.exp
	
#------------------------------------------------------------------
# Netscape NSAPI Adapter Shared Object
#------------------------------------------------------------------

obj/abtwsan.o: ../source/abtwsan.c \
	../source/abtwsi.h
	$(CC) $(SO_COPTS) $(NETSCAPE_INCLUDES) -DNETSSL -DAIX -DMCC_HTTPD -DXP_UNIX ../source/abtwsan.c -o $@   

abtwsan.so: obj/abtwsan.o
	$(LD)  $(LOPTS)  -bnoentry -bl:obj/abtwsan.bind -o $@ obj/abtwsan.o abtwsc.a -bE:abtwsan.exp -berok
	
#------------------------------------------------------------------
# Netscape TCP/IP Transport Functions Shared Object - not used for AIX
#------------------------------------------------------------------
#	
# obj/abtwstn.o: ../source/abtwstt.c \
#	../source/abtwstbs.h \
#	../source/abtwsi.h
#	$(CC) $(SO_COPTS) -DNS_NET $(NETSCAPE_INCLUDES) -DNETSSL -DAIX -DMCC_HTTPD -DXP_UNIX ../source/abtwstt.c -o $@
#    
# abtwstn.so: obj/abtwstn.o obj/abtwstbs.o 
#	$(LD) $(LOPTS) -e$(SHL_ENTRY) -bl:obj/abtwstn.bind -o $@ obj/abtwstn.o obj/abtwstbs.o $(LIBS) abtwsc.a -berok
#	
#
#------------------------------------------------------------------