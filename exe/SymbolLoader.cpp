#include "SymbolLoader.h"
#include "loki/ScopeGuard.h"

#include <shlwapi.h>
#include <shlobj.h>
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>

namespace aspk {

const wchar_t SymbolLoader::kSymbolPath[] = L"SRV*%s*http://msdl.microsoft.com"
                                            L"/download/symbols";

SymbolLoader::SymbolLoader()
  :mProc(::GetCurrentProcessId()),
   mInit(false),
   mLoadAddr(0)
{
}

SymbolLoader::~SymbolLoader()
{
  if (mInit) {
    ::SymCleanup(mProc);
  }
}

bool
SymbolLoader::Init()
{
  if (!mProc) {
    return false;
  }

  wchar_t path[MAX_PATH] = {0};
  if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path))) {
    return false;
  }
  if (FAILED(PathAppend(path, L"iqvis"))) {
    return false;
  }

  DWORD attrs = GetFileAttributes(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    CreateDirectory(path, NULL);
  }

  wchar_t symPath[MAX_PATH + sizeof(kSymbolPath)/sizeof(kSymbolPath[0])];
  if (swprintf(symPath, kSymbolPath, path) < 0) {
    return false;
  }

  if (!SymInitialize(mProc, symPath, FALSE)) {
    return false;
  }

  mInit = true;

  SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME);

  return true;
}

bool
SymbolLoader::LoadModule(const wchar_t* aImageName)
{
  mLoadAddr = SymLoadModuleEx(mProc, NULL, aImageName, NULL, NULL, 0, NULL, 0 );
  if (!mLoadAddr && ::GetLastError() != ERROR_SUCCESS) {
    return false;
  }

  IMAGEHLP_MODULE64 ihm = { sizeof(ihm) };
  if (!SymGetModuleInfo64(mProc, mLoadAddr, &ihm)) {
    return false;
  }

  return true;
}

void*
SymbolLoader::GetSymbolAddress(const wchar_t* aSymbolName)
{
  SYMBOL_INFO symbolInfo = {sizeof(SYMBOL_INFO)};
  if (!SymFromName(mProc, aSymbolName, &symbolInfo)) {
    return nullptr;
  }
  return reinterpret_cast<void*>(symbolInfo.Address);
}

bool
SymbolLoader::EnumerateSymbols(const wchar_t* aSpec,
                               SymbolLoader::ISymbolEnumerationCallback& aCb) const
{
  return SymEnumSymbols(mProc, mLoadAddr, aSpec,
                        &SymbolLoader::EnumerationCallback, &aCb);
}

bool
SymbolLoader::EnumerateTypes(SymbolLoader::ISymbolEnumerationCallback& aCb) const
{
  return SymEnumTypes(mProc, mLoadAddr,
                      &SymbolLoader::EnumerationCallback, &aCb);
}

/* static */ BOOL CALLBACK
SymbolLoader::EnumerationCallback(_SYMBOL_INFOW* aSymInfo, ULONG aSymbolSize,
                                  PVOID aContext)
{
  SymbolLoader::ISymbolEnumerationCallback* cbInterface =
    static_cast<SymbolLoader::ISymbolEnumerationCallback*>(aContext);
  return cbInterface->OnSymbolFound(aSymInfo->Name, aSymbolSize);
}

bool
SymbolLoader::LoadTypeInfo(const wchar_t* aTypeName)
{
  SYMBOL_INFO si = { sizeof(si) };
  if (!SymGetTypeFromName(mProc, mLoadAddr, aTypeName, &si)) {
    return false;
  }

  DWORD fieldCount;
  if (!SymGetTypeInfo(mProc, mLoadAddr, si.TypeIndex, TI_GET_CHILDRENCOUNT, &fieldCount)) {
    return false;
  }

  TI_FINDCHILDREN_PARAMS* typeData = (TI_FINDCHILDREN_PARAMS*) malloc(sizeof(TI_FINDCHILDREN_PARAMS) + (fieldCount - 1) * sizeof(ULONG));
  LOKI_ON_BLOCK_EXIT(free, typeData);
  typeData->Count = fieldCount;
  typeData->Start = 0;
  if (!SymGetTypeInfo(mProc, mLoadAddr, si.TypeIndex, TI_FINDCHILDREN, typeData)) {
    return false;
  }

  FieldMap& fieldMap = mStructMap[aTypeName];
  for (unsigned int i = 0; i < fieldCount; ++i) {
    WCHAR* symName;
    if (!SymGetTypeInfo(mProc, mLoadAddr, typeData->ChildId[i], TI_GET_SYMNAME, &symName)) {
      return false;
    }
    std::wstring wSymName(symName);
    LocalFree(symName);
    DWORD offset;
    if (!SymGetTypeInfo(mProc, mLoadAddr, typeData->ChildId[i], TI_GET_OFFSET, &offset)) {
      return false;
    }
    fieldMap[wSymName] = offset;
  }
  return true;
}

bool
SymbolLoader::GetOffset(const wchar_t* aTypeName, const wchar_t* aFieldName, unsigned int& aOffset) const
{
  StructMap::const_iterator sitr = mStructMap.find(aTypeName);
  if (sitr == mStructMap.end()) {
    return false;
  }
  FieldMap::const_iterator fitr = sitr->second.find(aFieldName);
  if (fitr == sitr->second.end()) {
    return false;
  }
  aOffset = fitr->second;
  return true;
}

}
