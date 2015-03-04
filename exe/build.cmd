cl -MD -EHsc -Zi -DUNICODE -D_UNICODE -I. -I../sys DriverLoader.cpp SymbolLoader.cpp IqvisClient.cpp iqvis.cpp -Feiqvis.exe ntdll.lib advapi32.lib dbghelp.lib kernel32.lib user32.lib shell32.lib shlwapi.lib ole32.lib -link -libpath:"/c/Program Files (x86)/Windows Kits/8.1/Debuggers/lib/x64"
mt -nologo -manifest iqvis.exe.manifest -outputresource:iqvis.exe
