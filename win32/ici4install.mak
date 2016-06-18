FILES = ici-v4-install.nsi Release\ici4.dll Release\ici.exe Release\iciw.exe \
		Debug\ici4.dll Debug\ici.exe Debug\iciw.exe ..\doc\ici.pdf


ici4-core-install.exe : $(FILES)
        "c:\program files\nsis\makensis" /v1 ici-v4-install.nsi
