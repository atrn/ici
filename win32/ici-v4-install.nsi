;
; This is a script to generate a Win32 ICI self-extracting install exe
; using NSIS - the free Nullsoft Installer System for Windows. See:
;
;     http://www.nullsoft.com/free/nsis/
;
; If you have installed NSIS, you should be able to right-click on this
; file in Explorer and select "Compile" to generate a new Win32 installer.
; We assume everything has been built before you run this. We also assume
; that ici-modules is a sibliing of ici.
;

;
; The define below is the name we use everywhere - titles, registry keys,
; etc.
;
!define NAME "ICI Programming Language Core"

Name "${NAME}"
OutFile "ici4-core-install.exe"

SetDateSave on
SetOverwrite ifnewer
CRCCheck on

;
; Set the text of the component selection dialog. This has the side
; effect of enabling the component selection dialog.
;
ComponentText "This will install core run-time support for the \
ICI Programming Language Version 4. There is a separate installer \
for the extension modules and SDK."

;
; Enable and set the text for the install location dialog.
;
DirShow show
DirText "Select the folder for documentation and \
related files. Core language support will be installed in the \
Windows system directory." " "
InstallDir "$PROGRAMFILES\ICI"
InstallDirRegKey HKLM "SOFTWARE\${NAME}" ""

;
; Default section. Always executed. Other sections are only executed if
; the user selects them at install time.
;
Section ""
SetOutPath "$INSTDIR"
WriteRegStr HKLM "SOFTWARE\${NAME}" "" "$INSTDIR"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayName" "${NAME} (remove only)"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "UninstallString" '"$INSTDIR\ici-uninst.exe"'
WriteUninstaller "ici-uninst.exe"
SectionEnd

;
; Core language section. 
;
Section "Core Language"
SetOutPath "$SYSDIR"
File "/oname=ici.exe" "Release\ici.exe"
File "/oname=ici4.dll" "Release\ici4.dll"
File "/oname=iciw.exe" "Release\iciw.exe"
CreateDirectory "$SYSDIR\ici"
SetOutPath "$SYSDIR\ici"
File "/oname=ici4core.ici" "..\ici4core.ici"
File "/oname=ici4core1.ici" "..\ici4core1.ici"
File "/oname=ici4core2.ici" "..\ici4core2.ici"
File "/oname=ici4core3.ici" "..\ici4core3.ici"
SetOutPath "$INSTDIR"
File "/oname=test-core.ici" "..\test-core.ici"
SectionEnd

;
; Manual section.
;
Section "Manual in PDF"
SetOutPath "$INSTDIR"
File "/oname=ici.pdf" "..\doc\ici.pdf"
CreateDirectory "$SMPROGRAMS\ICI Programming Language"
CreateShortCut "$SMPROGRAMS\ICI Programming Language\ICI Programming Language Manual.lnk"\
 "$INSTDIR\ici.pdf"
SectionEnd

;----------------------------------------------------------------------
; Uninstall stuff. Note that this stuff is logically seperate from the
; install stuff above (for obvious reasons). This is what runs when the
; user uninstalls.
;
UninstallText "This will uninstall ${NAME} from your system"

Section Uninstall
Delete "$INSTDIR\ici-uninst.exe"
;
; Core...
;
Delete "$SYSDIR\ici.exe"
Delete "$SYSDIR\iciw.exe"
Delete "$SYSDIR\ici4.dll"
Delete "$SYSDIR\ici\ici4core.ici"
Delete "$SYSDIR\ici\ici4core1.ici"
Delete "$SYSDIR\ici\ici4core2.ici"
Delete "$SYSDIR\ici\ici4core3.ici"
RMDir  "$SYSDIR\ici"
Delete "$INSTDIR\test-core.ici"
;
; Manual...
;
Delete "$SMPROGRAMS\ICI Programming Language\ICI Programming Language Manual.lnk"
RMDir  "$SMPROGRAMS\ICI Programming Language"
Delete "$INSTDIR\ici.pdf"

RMDir "$INSTDIR"
DeleteRegKey HKLM "SOFTWARE\${NAME}"
DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
SectionEnd
