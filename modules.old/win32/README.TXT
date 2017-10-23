My checklist for making new Visual-C projects for extension modules
-------------------------------------------------------------------

New... Project, Win32 Dynamic Link Library
- Make the project called 'ici4XXX' in the particular extension directory.
- Add it to the 'icimodules' workspace.

Project/Settings...
General...
- Verify it is using Debug and Release directories.
Debug...
Set Debug executable to:

    C:\Program Files\Microsoft Visual Studio\VC98\ici\debug\ici.exe

and Release executable to:

    C:\Program Files\Microsoft Visual Studio\VC98\ici\ici.exe

Link... Input
To the input libraries in Release & Debug config, add:

    ici4.lib

To the Project Options in *both* configs add:

    /export:ici_XXX_library_init
	



