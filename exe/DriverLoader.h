#ifndef __DRIVERLOADER_H
#define __DRIVERLOADER_H

#include <string>

#include <windows.h>
#include <winternl.h>

namespace aspk {

class DriverLoader
{
public:
  DriverLoader(std::wstring const &aDrvShortName, std::wstring const &aDrvPath);
  ~DriverLoader();

  bool operator!() const { return !mIsLoaded; }

private:
  bool Load(std::wstring const &aDrvShortName, std::wstring const &aDrvPath);

  typedef NTSTATUS (NTAPI* NTLOADDRIVER)(PUNICODE_STRING);
  typedef NTSTATUS (NTAPI* NTUNLOADDRIVER)(PUNICODE_STRING);
  NTLOADDRIVER    mNtLoadDriver;
  NTUNLOADDRIVER  mNtUnloadDriver;
  SC_HANDLE       mScm;
  SC_HANDLE       mDrvSvc;
  std::wstring    mDrvShortName;
  bool            mIsLoaded;
};

} // namespace aspk

#endif // __DRIVERLOADER_H

