#include "DriverLoader.h"

#include "loki/ScopeGuard.h"

using std::wstring;

namespace aspk {

DriverLoader::DriverLoader(wstring const &aDrvShortName, wstring const &aDrvPath)
  : mNtLoadDriver(nullptr)
  , mNtUnloadDriver(nullptr)
  , mScm(NULL)
  , mDrvSvc(NULL)
  , mDrvShortName(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\")
  , mIsLoaded(false)
{
  mIsLoaded = Load(aDrvShortName, aDrvPath);
}

DriverLoader::~DriverLoader()
{
  if (mIsLoaded) {
    UNICODE_STRING usDrvShortName;
    RtlInitUnicodeString(&usDrvShortName, mDrvShortName.c_str());
    mNtUnloadDriver(&usDrvShortName);
  }
  if (mDrvSvc) {
    ::DeleteService(mDrvSvc);
    ::CloseServiceHandle(mDrvSvc);
  }
  if (mScm) {
    ::CloseServiceHandle(mScm);
  }
}

bool
DriverLoader::Load(wstring const &aDrvShortName, wstring const &aDrvPath)
{
  // (1) Enable the SE_LOAD_DRIVER_NAME privilege
  HANDLE processToken = NULL;
  if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
                          &processToken)) {
    return false;
  }
  LOKI_ON_BLOCK_EXIT(::CloseHandle, processToken);
  LUID privLuid;
  if (!::LookupPrivilegeValue(nullptr, SE_LOAD_DRIVER_NAME, &privLuid)) {
    return false;
  }
  TOKEN_PRIVILEGES newPrivState;
  DWORD newPrivStateLen = sizeof(newPrivState);
  ZeroMemory(&newPrivState, newPrivStateLen);
  newPrivState.PrivilegeCount = 1;
  newPrivState.Privileges[0].Luid = privLuid;
  newPrivState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!::AdjustTokenPrivileges(processToken, FALSE, &newPrivState,
                               newPrivStateLen, nullptr, nullptr)) {
    return false;
  }
  if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
    return false;
  }
  // (2) Find the NtLoadDriver entry point
  HMODULE ntdll = ::GetModuleHandle(L"ntdll.dll");
  mNtLoadDriver = reinterpret_cast<NTLOADDRIVER>(
    ::GetProcAddress(ntdll, "NtLoadDriver"));
  if (!mNtLoadDriver) {
    return false;
  }
  mNtUnloadDriver = reinterpret_cast<NTUNLOADDRIVER>(
    ::GetProcAddress(ntdll, "NtUnloadDriver"));
  if (!mNtUnloadDriver) {
    return false;
  }
  // (3) Install the driver service
  wchar_t fullPath[MAX_PATH];
  DWORD fullPathResult = ::GetFullPathName(aDrvPath.c_str(), MAX_PATH,
                                           fullPath, nullptr);
  if (!fullPathResult || fullPathResult > MAX_PATH) {
    return false;
  }
  mScm = ::OpenSCManager(nullptr, SERVICES_ACTIVE_DATABASE,
                         SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
  if (!mScm) {
    return false;
  }
  mDrvSvc = ::CreateService(mScm, aDrvShortName.c_str(), nullptr,
                            DELETE | SERVICE_QUERY_STATUS,
                            SERVICE_KERNEL_DRIVER,
                            SERVICE_DEMAND_START,
                            SERVICE_ERROR_IGNORE,
                            fullPath,
                            nullptr, nullptr, nullptr, nullptr, nullptr);
  if (!mDrvSvc) {
    if (::GetLastError() == ERROR_SERVICE_EXISTS) {
      // We aborted after creating this service last time?
      mDrvSvc = ::OpenService(mScm, aDrvShortName.c_str(),
                              DELETE | SERVICE_QUERY_STATUS);
      if (!mDrvSvc) {
        return false;
      }
    } else {
      return false;
    }
  }
  mDrvShortName += aDrvShortName;
  UNICODE_STRING usDrvShortName;
  RtlInitUnicodeString(&usDrvShortName, mDrvShortName.c_str());
  if (!NT_SUCCESS(mNtLoadDriver(&usDrvShortName))) {
    return false;
  }
  return true;
}

} // namespace aspk

