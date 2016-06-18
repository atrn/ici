# Microsoft Developer Studio Project File - Name="icisdk" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=icisdk - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "icisdk.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "icisdk.mak" CFG="icisdk - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "icisdk - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "icisdk - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "icisdk - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "icisdk___Win32_Release"
# PROP BASE Intermediate_Dir "icisdk___Win32_Release"
# PROP BASE Cmd_Line "NMAKE /f icisdk.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "icisdk.exe"
# PROP BASE Bsc_Name "icisdk.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "icisdk___Win32_Release"
# PROP Intermediate_Dir "icisdk___Win32_Release"
# PROP Cmd_Line "nmake /f "icisdk.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "ici4-sdk-install.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "icisdk - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "icisdk___Win32_Debug"
# PROP BASE Intermediate_Dir "icisdk___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f icisdk.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "icisdk.exe"
# PROP BASE Bsc_Name "icisdk.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "icisdk___Win32_Debug"
# PROP Intermediate_Dir "icisdk___Win32_Debug"
# PROP Cmd_Line "nmake /f "icisdk.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "ici4-sdk-install.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "icisdk - Win32 Release"
# Name "icisdk - Win32 Debug"

!IF  "$(CFG)" == "icisdk - Win32 Release"

!ELSEIF  "$(CFG)" == "icisdk - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=".\ici4-sdk-install.nsi"
# End Source File
# Begin Source File

SOURCE=.\icisdk.mak
# End Source File
# End Target
# End Project
