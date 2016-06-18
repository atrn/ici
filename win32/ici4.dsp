# Microsoft Developer Studio Project File - Name="ici4" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ici4 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ici4.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ici4.mak" CFG="ici4 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ici4 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ici4 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ici4 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /Zi /Ox /Ot /Oa /Ow /Og /Oi /Op /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /version:4.0 /subsystem:windows /dll /profile /debug /machine:I386

!ELSEIF  "$(CFG)" == "ici4 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /Fd"Debug/ici4-d.pdb" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /version:4.0 /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "ici4 - Win32 Release"
# Name "ici4 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\alloc.c
# End Source File
# Begin Source File

SOURCE=..\aplfuncs.c
# End Source File
# Begin Source File

SOURCE=..\arith.c
# End Source File
# Begin Source File

SOURCE=..\array.c
# End Source File
# Begin Source File

SOURCE=..\buf.c
# End Source File
# Begin Source File

SOURCE=..\call.c
# End Source File
# Begin Source File

SOURCE=..\catch.c
# End Source File
# Begin Source File

SOURCE=..\cfunc.c
# End Source File
# Begin Source File

SOURCE=..\cfunco.c
# End Source File
# Begin Source File

SOURCE=..\clib.c
# End Source File
# Begin Source File

SOURCE=..\clib2.c
# End Source File
# Begin Source File

SOURCE=..\compile.c
# End Source File
# Begin Source File

SOURCE=..\conf.c
# End Source File
# Begin Source File

SOURCE=..\control.c
# End Source File
# Begin Source File

SOURCE=..\crc.c
# End Source File
# Begin Source File

SOURCE=..\events.c
# End Source File
# Begin Source File

SOURCE=..\exec.c
# End Source File
# Begin Source File

SOURCE=..\exerror.c
# End Source File
# Begin Source File

SOURCE=..\file.c
# End Source File
# Begin Source File

SOURCE=..\findpath.c
# End Source File
# Begin Source File

SOURCE=..\float.c
# End Source File
# Begin Source File

SOURCE=..\forall.c
# End Source File
# Begin Source File

SOURCE=..\func.c
# End Source File
# Begin Source File

SOURCE=..\handle.c
# End Source File
# Begin Source File

SOURCE=..\ICI.DEF
# End Source File
# Begin Source File

SOURCE=..\icimain.c
# End Source File
# Begin Source File

SOURCE=..\idb.c
# End Source File
# Begin Source File

SOURCE=..\idb2.c
# End Source File
# Begin Source File

SOURCE=..\init.c
# End Source File
# Begin Source File

SOURCE=..\int.c
# End Source File
# Begin Source File

SOURCE=..\lex.c
# End Source File
# Begin Source File

SOURCE=..\load.c
# End Source File
# Begin Source File

SOURCE=..\mark.c
# End Source File
# Begin Source File

SOURCE=..\mem.c
# End Source File
# Begin Source File

SOURCE=..\method.c
# End Source File
# Begin Source File

SOURCE=..\mkvar.c
# End Source File
# Begin Source File

SOURCE=..\null.c
# End Source File
# Begin Source File

SOURCE=..\object.c
# End Source File
# Begin Source File

SOURCE=..\oofuncs.c
# End Source File
# Begin Source File

SOURCE=..\op.c
# End Source File
# Begin Source File

SOURCE=..\parse.c
# End Source File
# Begin Source File

SOURCE=..\pc.c
# End Source File
# Begin Source File

SOURCE=..\PCRE\PCRE.C
# End Source File
# Begin Source File

SOURCE=..\profile.c
# End Source File
# Begin Source File

SOURCE=..\ptr.c
# End Source File
# Begin Source File

SOURCE=..\refuncs.c
# End Source File
# Begin Source File

SOURCE=..\regexp.c
# End Source File
# Begin Source File

SOURCE=..\set.c
# End Source File
# Begin Source File

SOURCE=..\sfile.c
# End Source File
# Begin Source File

SOURCE=..\signals.c
# End Source File
# Begin Source File

SOURCE=..\smash.c
# End Source File
# Begin Source File

SOURCE=..\src.c
# End Source File
# Begin Source File

SOURCE=..\sstring.c
# End Source File
# Begin Source File

SOURCE=..\string.c
# End Source File
# Begin Source File

SOURCE=..\strtol.c
# End Source File
# Begin Source File

SOURCE=..\struct.c
# End Source File
# Begin Source File

SOURCE=..\PCRE\STUDY.C
# End Source File
# Begin Source File

SOURCE=..\syserr.c
# End Source File
# Begin Source File

SOURCE=..\thread.c
# End Source File
# Begin Source File

SOURCE=..\trace.c
# End Source File
# Begin Source File

SOURCE=..\unary.c
# End Source File
# Begin Source File

SOURCE=..\uninit.c
# End Source File
# Begin Source File

SOURCE=..\win32err.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\alloc.h
# End Source File
# Begin Source File

SOURCE=..\array.h
# End Source File
# Begin Source File

SOURCE="..\..\..\Program Files\Microsoft Visual Studio\VC98\Include\BASETSD.H"
# End Source File
# Begin Source File

SOURCE=..\binop.h
# End Source File
# Begin Source File

SOURCE=..\buf.h
# End Source File
# Begin Source File

SOURCE=..\catch.h
# End Source File
# Begin Source File

SOURCE=..\cfunc.h
# End Source File
# Begin Source File

SOURCE=..\PCRE\chartables.c
# End Source File
# Begin Source File

SOURCE="..\conf-w32.h"
# End Source File
# Begin Source File

SOURCE=..\exec.h
# End Source File
# Begin Source File

SOURCE=..\file.h
# End Source File
# Begin Source File

SOURCE=..\float.h
# End Source File
# Begin Source File

SOURCE=..\forall.h
# End Source File
# Begin Source File

SOURCE=..\func.h
# End Source File
# Begin Source File

SOURCE=..\fwd.h
# End Source File
# Begin Source File

SOURCE=..\handle.h
# End Source File
# Begin Source File

SOURCE=..\int.h
# End Source File
# Begin Source File

SOURCE=..\PCRE\INTERNAL.H
# End Source File
# Begin Source File

SOURCE="..\load-w32.h"
# End Source File
# Begin Source File

SOURCE=..\mark.h
# End Source File
# Begin Source File

SOURCE=..\mem.h
# End Source File
# Begin Source File

SOURCE=..\method.h
# End Source File
# Begin Source File

SOURCE=..\null.h
# End Source File
# Begin Source File

SOURCE=..\object.h
# End Source File
# Begin Source File

SOURCE=..\op.h
# End Source File
# Begin Source File

SOURCE=..\parse.h
# End Source File
# Begin Source File

SOURCE=..\pc.h
# End Source File
# Begin Source File

SOURCE=..\PCRE\PCRE.H
# End Source File
# Begin Source File

SOURCE=..\primes.h
# End Source File
# Begin Source File

SOURCE=..\profile.h
# End Source File
# Begin Source File

SOURCE=..\ptr.h
# End Source File
# Begin Source File

SOURCE=..\re.h
# End Source File
# Begin Source File

SOURCE=..\set.h
# End Source File
# Begin Source File

SOURCE=..\src.h
# End Source File
# Begin Source File

SOURCE=..\sstring.h
# End Source File
# Begin Source File

SOURCE=..\str.h
# End Source File
# Begin Source File

SOURCE=..\struct.h
# End Source File
# Begin Source File

SOURCE=..\wrap.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\CHANGES
# End Source File
# Begin Source File

SOURCE=..\ici4core.ici
# End Source File
# Begin Source File

SOURCE=..\ici4core1.ici
# End Source File
# Begin Source File

SOURCE=..\ici4core2.ici
# End Source File
# Begin Source File

SOURCE=..\ici4core3.ici
# End Source File
# Begin Source File

SOURCE=.\README.txt
# End Source File
# Begin Source File

SOURCE=..\TODO
# End Source File
# End Target
# End Project
