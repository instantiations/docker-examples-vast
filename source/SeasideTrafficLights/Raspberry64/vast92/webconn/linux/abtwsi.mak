#------------------------------------------------------------------
# abtwsi make file for Red Hat Linux 7.1
#------------------------------------------------------------------
#
CC = gcc
LD = $(CC)
#
# CFLAGS:		C compiler flags
#
#	-c		Compile only
#	-fPIC		Create position-independent code
#	-O2		Level 2 optimization
#	-g		Add debug info'
#	-ansi		ANSI C compatibility
#
CFLAGS    = -fPIC -O2  -DPTHREADS -DOPSYS_LINUX -g
SO_CFLAGS = $(CFLAGS) -c  
#
# LFLAGS:		Linker flags
#
#	-shared		Create shared library
#	-i		Ignore LD_LIBRARY_PATH during link
#	-z defs		Fail on undefined sym-bols
#	-g		Add debug info'
#	-static		create static library
LFLAGS = -g -shared -Wl,-Bsymbolic

#
LIBS = -L.:/usr/lib -lc
ALL: abtwsc.so abtwstt.so abtwsac
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
	$(LD) $(LFLAGS) -o $@ obj/abtwscos.o obj/abtwsc.o $(LIBS) -ldl 

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
	$(LD) $(LFLAGS) -o $@ obj/abtwstt.o obj/abtwstbs.o $(LIBS) -lnsl abtwsc.so
	  
#------------------------------------------------------------------
# CGI Executable
#------------------------------------------------------------------

abtwsac: ../source/abtwsac.c ../source/abtwsi.h
	$(CC) $(CFLAGS) ../source/abtwsac.c -o $@ $(LIBS) abtwsc.so

