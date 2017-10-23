FILES = ici4-modules-install.nsi \
	..\xml\Release\ici4xml.dll ..\xml\ici4xml.ici \
	..\net\Release\ici4net.dll


ici4-modules-install.exe : $(FILES)
        "c:\program files\nsis\makensis" /v2 ici4-modules-install.nsi
