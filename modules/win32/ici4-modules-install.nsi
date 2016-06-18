;
; This is a script to generate a Win32 ICI self-extracting install exe
; using NSIS - the free Nullsoft Installer System for Windows. See:
;
;     http://www.nullsoft.com/free/nsis/
;

;
; The define below is the name we use everywhere - titles, registry keys,
; etc.
;
!define NAME "ICI Programming Language Modules"

Name "${NAME}"
OutFile "ici4-modules-install.exe"

SetDateSave on
;SetOverwrite ifnewer
CRCCheck on

;
; Set the text of the component selection dialog. This has the side
; effect of enabling the component selection dialog.
;
ComponentText "This will install a set of extension modules for ICI \
version 4. There is a separate installer for the core language and SDK. \
The core should be installed first."

;
; Enable and set the text for the install location dialog. Note that we
; use InstallDirRegKey to find the location the *core* ICI is installed
; in, rather than our own previous location.
;
DirShow show
DirText "Select the folder for documentation and related \
files. The core module support will be installed in the Windows \
system directory." " "
InstallDir "$PROGRAMFILES\ICI"
InstallDirRegKey HKLM "SOFTWARE\ICI Programming Language Core" ""



;
; Default section. Always executed. Other sections are only executed if
; the user selects them at install time.
;
Section ""
    SetOutPath "$INSTDIR"
    WriteRegStr HKLM "SOFTWARE\${NAME}" "" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayName" "${NAME} (remove only)"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "UninstallString" '"$INSTDIR\ici-modules-uninst.exe"'
    WriteUninstaller "ici-modules-uninst.exe"
    StrCpy $0 "No docs"
SectionEnd

Section "Module documentation"
StrCpy $0 "Yes docs"
CreateDirectory "$SMPROGRAMS\ICI Programming Language"
SectionEnd

!macro install_doc MOD
    StrCmp $0 "No docs" no_${MOD}_doc
        SetOutPath "$INSTDIR"
        pop $R1
        File /oname=ici${MOD}.html ..\${MOD}\ici${MOD}.html
        CreateShortCut "$SMPROGRAMS\ICI Programming Language\ICI ${MOD} module doc.lnk"\
            "$INSTDIR\ici${MOD}.html"
    no_${MOD}_doc:
!macroend


Section "xml - Expat based XML parser"
SetOutPath "$SYSDIR\ici"
File "/oname=ici4xml.dll" "..\xml\Release\ici4xml.dll"
File "/oname=ici4xml.ici" "..\xml\ici4xml.ici"
!insertmacro install_doc xml
SectionEnd

Section "net - Sockets based networking"
SetOutPath "$SYSDIR\ici"
File "/oname=ici4net.dll" "..\net\Release\ici4net.dll"
File "/oname=ici4net.ici" "..\net\ici4net.ici"
!insertmacro install_doc net
SectionEnd

Section "sys - System calls and related"
SetOutPath "$SYSDIR\ici"
File "/oname=ici4sys.dll" "..\sys\Release\ici4sys.dll"
File "/oname=ici4sys.ici" "..\sys\ici4sys.ici"
!insertmacro install_doc sys
SectionEnd

Section "dll - Call DLL functions directly"
SetOutPath "$SYSDIR\ici"
File "/oname=ici4dll.dll" "..\dll\Release\ici4dll.dll"
!insertmacro install_doc dll
SectionEnd

Section "html - HTML element classes"
SetOutPath "$SYSDIR\ici"
File "/oname=ici4html.ici" "..\html\ici4html.ici"
!insertmacro install_doc html
SectionEnd

;----------------------------------------------------------------------
; Uninstall stuff. Note that this stuff is logically seperate from the
; install stuff above (for obvious reasons). This runs when the user does
; an uninstall.
;
UninstallText "This will uninstall ${NAME} from your system"

Section Uninstall
Delete "$INSTDIR\ici-modules-uninst.exe"

;
; Remove extension modules...
;
Delete "$SYSDIR\ici\ici4xml.dll"
Delete "$SYSDIR\ici\ici4xml.ici"
Delete "$SYSDIR\ici\ici4net.dll"
Delete "$SYSDIR\ici\ici4net.ici"
Delete "$SYSDIR\ici\ici4sys.dll"
Delete "$SYSDIR\ici\ici4sys.ici"
Delete "$SYSDIR\ici\ici4dll.dll"
Delete "$SYSDIR\ici\ici4html.ici"
RMDir  "$SYSDIR\ici"

;
; Remove documentation...
;
Delete "$INSTDIR\icidll.html"
Delete "$SMPROGRAMS\ICI Programming Language\ICI dll module doc.lnk"
Delete "$INSTDIR\icixml.html"
Delete "$SMPROGRAMS\ICI Programming Language\ICI xml module doc.lnk"
Delete "$INSTDIR\icinet.html"
Delete "$SMPROGRAMS\ICI Programming Language\ICI net module doc.lnk"
Delete "$INSTDIR\icisys.html"
Delete "$SMPROGRAMS\ICI Programming Language\ICI sys module doc.lnk"
Delete "$INSTDIR\icihtml.html"
Delete "$SMPROGRAMS\ICI Programming Language\ICI html module doc.lnk"
RMDir  "$SMPROGRAMS\ICI Programming Language"
RMDir  "$PROGRAMFILES\ICI"

DeleteRegKey HKLM "SOFTWARE\${NAME}"
DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
SectionEnd
