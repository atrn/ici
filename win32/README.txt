This directory contains workspace and projects to build ICI with Visual C
under Windows. This is probably preferable to the "hand crafted" makefile
in ..\Makefile.w32.

All intermediate and output files are built in Release and Debug
directories.

Projects:

ici4        Builds ici4.dll, which is the core language interpreter.

ici         Builds ici.exe, the Windows command-line utility.

iciw        Builds iciw.exe, the Windows non-command line version.

ici4widb    Builds ici4widb.dll, which is the Windows ICI debugger
            extension module. Under Windows, this installs as part
            of the core.

ici4install Builds ici4-core-install.exe, which is the core run-time
            installer.

icisdk      Builds ici-sdk-install.exe, which is an installer for the
            ICI SDK.


Visual Studio.NET Files

For VC++.NET (aka VC++ 7) the following files are provided:

ici.sln      Overall "solution" file (workspace) that contains
             the project files.

ici4.vcproj  Project to build ici4.dll, the core language interpreter.

ici.vcproj   Project to build ici.exe, the Windows command-line utility.

iciw.vcproj  Project to build iciw.exe, the Windows non-command line version.


Installer

The installer builds uses the free Nullsoft Installer System for Windows.
This can be downloaded and installed from:

    http://www.nullsoft.com/free/nsis/

