# Microsoft Developer Studio Project File - Name="installer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=installer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "installer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "installer.mak" CFG="installer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "installer - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "installer - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "installer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f installer.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "installer.exe"
# PROP BASE Bsc_Name "installer.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake /f "installer.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "ici4-modules-install.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "installer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f installer.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "installer.exe"
# PROP BASE Bsc_Name "installer.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "nmake /f "installer.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "ici4-modules-install.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "installer - Win32 Release"
# Name "installer - Win32 Debug"

!IF  "$(CFG)" == "installer - Win32 Release"

!ELSEIF  "$(CFG)" == "installer - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=".\ici4-modules-install.nsi"
# End Source File
# Begin Source File

SOURCE=.\installer.mak
# End Source File
# Begin Source File

SOURCE=.\README.TXT
# End Source File
# End Target
# End Project
