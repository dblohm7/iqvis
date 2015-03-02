rem cl -MD -EHsc -Zi -DUNICODE -D_UNICODE -I. SymbolLoader.cpp sltest.cpp -Fesltest.exe dbghelp.lib kernel32.lib user32.lib shell32.lib shlwapi.lib
cl -MD -EHsc -Zi -DUNICODE -D_UNICODE -I. DriverLoader.cpp iqvis.cpp -Feiqvis.exe ntdll.lib advapi32.lib dbghelp.lib kernel32.lib user32.lib shell32.lib shlwapi.lib -link
mt -nologo -manifest iqvis.exe.manifest -outputresource:iqvis.exe
