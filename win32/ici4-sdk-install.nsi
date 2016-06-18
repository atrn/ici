;
; This is a script to generate a Win32 ICI SDK self-extracting install exe
; using NSIS - the free Nullsoft Installer System for Windows. See:
;
;     http://www.nullsoft.com/free/nsis/
;

;
; The define below is the name we use everywhere - titles, registry keys,
; etc.
;
!define NAME "ICI Programming Language SDK"
Name "${NAME}"
OutFile "ici4-sdk-install.exe"

SetDateSave on
SetOverwrite ifnewer
CRCCheck on

;
; Set the text of the component selection dialog. This has the side
; effect of enabling the component selection dialog.
;
ComponentText "This will install library and include files \
for interfacing with ICI version 4. Both for extension \
modules, and using ICI as a component in another program."

;
; Enable and set the text for the install location dialog.
;
DirShow show
DirText "If you are using MS Visual C++ 6 and install in the \
VC98 directory, the include files and libs will be found without \
further Visual C option adjustments."
InstallDir "C:\ICI-SDK"
InstallDirRegKey HKLM "SOFTWARE\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual C++" "ProductDir"

;
; Default section. Always executed. Other sections are only executed if
; the user selects them at install time.
;
Section ""
SetOutPath "$INSTDIR"
WriteRegStr HKLM "SOFTWARE\${NAME}" "" "$INSTDIR"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayName" "${NAME} (remove only)"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "UninstallString" '"$INSTDIR\icisdk-uninst.exe"'
WriteUninstaller "icisdk-uninst.exe"
SectionEnd

;
; Basic SDK. 
;
Section "ICI Software Development Kit"
SetOutPath "$INSTDIR\lib"
File "/oname=ici4.lib" "Release\ici4.lib"
SetOutPath "$INSTDIR\include"
File "/oname=ici.h" "..\ici.h"
File "/oname=icistr-setup.h" "..\icistr-setup.h"
SetOutPath "$INSTDIR\ici"
File "/oname=ici.exe" "Release\ici.exe"
File "/oname=ici4.dll" "Release\ici4.dll"
File "/oname=ici4.pdb" "Release\ici4.pdb"
File "/oname=iciw.exe" "Release\iciw.exe"
SetOutPath "$INSTDIR\ici\debug"
File "/oname=ici.exe" "Debug\ici.exe"
File "/oname=ici.pdb" "Debug\ici.pdb"
File "/oname=iciw.exe" "Debug\iciw.exe"
File "/oname=iciw.pdb" "Debug\iciw.pdb"
File "/oname=ici4.dll" "Debug\ici4.dll"
File "/oname=ici4.pdb" "Debug\ici4.pdb"
SectionEnd

;
; Source code
;
Section "ICI Version 4 source"
SetOutPath "$INSTDIR\ici\src"
File ..\*.c
File ..\*.h
File ..\ici.def
SetOutPath "$INSTDIR\ici\src\pcre"
File ..\pcre\*.c
File ..\pcre\*.h
SectionEnd

;----------------------------------------------------------------------
; Uninstall stuff. Note that this stuff is logically seperate from the
; install stuff above (for obvious reasons).
;
UninstallText "This will uninstall ${NAME} from your system"

Section Uninstall
Delete "$INSTDIR\icisdk-uninst.exe"
Delete "$INSTDIR\lib\ici4.lib"
Delete "$INSTDIR\include\ici.h"
Delete "$INSTDIR\include\icistr-setup.h"
Delete "$INSTDIR\ici\debug\ici.exe"
Delete "$INSTDIR\ici\debug\ici.pdb"
Delete "$INSTDIR\ici\debug\iciw.exe"
Delete "$INSTDIR\ici\debug\ici4.dll"
Delete "$INSTDIR\ici\debug\ici4.pdb"
RMDir "$INSTDIR\ici\debug"
RMDir "$INSTDIR\ici"
DeleteRegKey HKLM "SOFTWARE\${NAME}"
DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
SectionEnd

