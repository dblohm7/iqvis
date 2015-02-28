#include "SymbolLoader.h"
#include <iostream>
#include <ios>

using std::hex;
using std::cout;
using std::wcout;
using std::endl;

class SymbolPrinter : public aspk::SymbolLoader::ISymbolEnumerationCallback
{
public:
  bool OnSymbolFound(const wchar_t* aName, ULONG aSymbolSize)
  {
    wcout << aName << endl;
    return true;
  }
};

#define COUT_SYMBOL(sym) \
  cout << #sym << ": 0x" << hex << sym << endl

int wmain(int argc, wchar_t* argv[])
{
  aspk::SymbolLoader sl;
  if (!sl.Init()) {
    return 1;
  }
  PVOID old;
  Wow64DisableWow64FsRedirection(&old);
  if (!sl.LoadModule(L"C:\\Windows\\System32\\win32k.sys")) {
    cout << "LoadModule failed" << endl;
    return 1;
  }
  Wow64RevertWow64FsRedirection(old);
  void* gpresUser = sl.GetSymbolAddress(L"gpresUser");
  COUT_SYMBOL(gpresUser);
  void* gpai = sl.GetSymbolAddress(L"gpai");
  COUT_SYMBOL(gpai);
  void*  leaveCrit = sl.GetSymbolAddress(L"UserSessionSwitchLeaveCrit");
  COUT_SYMBOL(leaveCrit);
  void*  enterCrit = sl.GetSymbolAddress(L"EnterCritAvoidingDitHitTestHazard");
  COUT_SYMBOL(enterCrit);
  /*
  if (!sl.LoadTypeInfo(L"tagTHREADINFO")) {
    cout << "LoadTypeInfo failed" << endl;
    return 1;
  }
  unsigned int offset;
  if (!sl.GetOffset(L"tagTHREADINFO", L"pq", offset)) {
    cout << "GetOffset(pq) failed" << endl;
    return 1;
  }
  wcout << L"pq" << L": " << offset << endl;
  if (!sl.GetOffset(L"tagTHREADINFO", L"pqAttach", offset)) {
    cout << "GetOffset(pqAttach) failed" << endl;
    return 1;
  }
  wcout << L"pqAttach" << L": " << offset << endl;
  SymbolPrinter printer;
  if (!sl.EnumerateTypes(printer)) {
    cout << "EnumerateTypes failed" << endl;
    return 1;
  }
  */
  return 0;
}

