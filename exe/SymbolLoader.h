#ifndef __SYMBOLLOADER_H
#define __SYMBOLLOADER_H

#include <unordered_map>
#include <string>

#include "ScopedProcess.h"

struct _SYMBOL_INFOW;

namespace aspk {

class SymbolLoader
{
public:
  SymbolLoader();
  ~SymbolLoader();

  bool Init(const wchar_t* aAppDataName);
  bool LoadModule(const wchar_t* aImageName);
  void* GetSymbolAddress(const wchar_t* aSymbolName);
  DWORD GetSymbolRVA(const wchar_t* aSymbolName);
  bool LoadTypeInfo(const wchar_t* aTypeName);
  bool GetOffset(const wchar_t* aTypeName, const wchar_t* aFieldName, unsigned int& aOffset) const;

  class ISymbolEnumerationCallback
  {
  public:
    virtual bool OnSymbolFound(const wchar_t* aName, ULONG aSymbolSize) = 0;
  };
  bool EnumerateSymbols(const wchar_t* aSpec, ISymbolEnumerationCallback& aCb) const;
  bool EnumerateTypes(ISymbolEnumerationCallback& aCb) const;

private:
  static BOOL CALLBACK EnumerationCallback(_SYMBOL_INFOW* aSymInfo,
                                           ULONG aSymbolSize, PVOID aContext);


  typedef std::unordered_map<std::wstring,DWORD> FieldMap;
  typedef std::unordered_map<std::wstring,FieldMap> StructMap;

  ScopedProcess mProc;
  bool mInit;
  DWORD64 mLoadAddr;
  StructMap mStructMap;

  static const wchar_t kSymbolPath[];
};

} // namespace aspk

#endif // __SYMBOLLOADER_H

