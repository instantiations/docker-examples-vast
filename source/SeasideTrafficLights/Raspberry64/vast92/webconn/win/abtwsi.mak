#------------------------------------------------------------------
# abtwsi make file for Win32 (Visual Studio NMake)
# To build:
# nmake abtwsi.mak
#
# To clean:
# nmake clean /F abtwsi.mak
#------------------------------------------------------------------
#
!include <win32.mak>

#
# Add the element $(MEMDEBUG) to the COPTS to enable memory debugging
MEMDEBUG = /DMEM_DEBUG
#
#  Add /Ti for debug mode; use /O for optimization
COPTS = /DOPSYS_WIN32 /D_CRT_SECURE_NO_DEPRECATE /nologo /O2 /W1 /MT /c /I..\source
#
#  Use /Debug for debugging
LOPTS = /NOLOGO
	
ALL: abtwsc.dll abtwstt.dll abtwsac.exe abtwsam.dll

#
#------------------------------------------------------------------
# Core Functions DLL
#------------------------------------------------------------------

abtwsc.obj: ..\source\abtwsc.c
   $(cc) $(COPTS) /Foabtwsc.obj ..\source\abtwsc.c

abtwscos.obj: ..\source\abtwscos.c
    $(cc) $(COPTS) /Foabtwscos.obj ..\source\abtwscos.c

abtwsc.exp: abtwsc.def
    LIB  /NOLOGO abtwsc.obj /DEF:abtwsc.def /OUT:abtwsc.lib

abtwsc.dll: abtwsc.obj abtwscos.obj abtwsc.exp
    $(link) @<<
     $(LOPTS) /DLL
     abtwsc.exp
     abtwscos.obj
     abtwsc.obj
<<

#------------------------------------------------------------------
# Base TCP/IP Transport Functions DLL
#------------------------------------------------------------------

abtwstt.obj: ..\source\abtwstt.c
    $(cc) $(COPTS) /Foabtwstt.obj ..\source\abtwstt.c

abtwstbs.obj: ..\source\abtwstbs.c
    $(cc) $(COPTS) /Foabtwstbs.obj ..\source\abtwstbs.c

abtwstt.exp: abtwstt.def
	LIB /NOLOGO abtwstt.obj /DEF:abtwstt.def /OUT:abtwstt.lib

abtwstt.dll: abtwstt.obj abtwstbs.obj abtwstt.exp abtwsc.lib
    $(link) @<<
     $(LOPTS) /DLL
     abtwstt.exp
     abtwstt.obj
     abtwstbs.obj
     abtwsc.lib
     wsock32.lib
<<

#------------------------------------------------------------------
# CGI Executable
#------------------------------------------------------------------

abtwsac.obj: ..\source\abtwsac.c
    $(cc) $(COPTS) /Foabtwsac.obj ..\source\abtwsac.c

abtwsac.exe: abtwsac.obj abtwsc.lib
    $(link) @<<
      $(LOPTS)
      abtwsac.obj
      abtwsc.lib
      wsock32.lib
<<

#------------------------------------------------------------------
# NO LONGER SUPPORTED: IBM ICS Adapter DLL
#------------------------------------------------------------------

#------------------------------------------------------------------
# NO LONGER SUPPORTED: Netscape NSAPI Adapter DLL
#------------------------------------------------------------------

#------------------------------------------------------------------
# Microsoft ISAPI Adapter DLL
#------------------------------------------------------------------

abtwsam.obj: ..\source\abtwsam.c
    $(cc) $(COPTS) /Foabtwsam.obj ..\source\abtwsam.c

abtwsam.exp: abtwsam.def
	LIB /NOLOGO abtwsam.obj /DEF:abtwsam.def /OUT:abtwsam.lib

abtwsam.dll: abtwsam.obj abtwsam.exp abtwsc.lib
    $(link) @<<
     $(LOPTS) /DLL
     abtwsam.exp
     abtwsam.obj
     abtwsc.lib
     $(baselibs)
<<

clean:
	del /Q *.dll *.exe *.lib *.exp *.obj
#------------------------------------------------------------------