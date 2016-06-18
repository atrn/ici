
ici4-sdk-install.exe : \
				ici4-sdk-install.nsi \
				Release\ici4.dll \
				Release\ici.exe \
				Release\iciw.exe \
				..\mk-ici-h.ici
		cd ..
		ici mk-ici-h.ici conf-w32.h
		cd win32
		"c:\program files\nsis\makensis" /v2 ici4-sdk-install.nsi
