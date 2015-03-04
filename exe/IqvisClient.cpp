#include "IqvisClient.h"
#include "SymbolLoader.h"

#include "loki/ScopeGuard.h"

#include <shlobj.h>

#include <string>

using std::wstring;

namespace aspk {

IqvisClient::IqvisClient()
  : mHasSymbols(false)
{
  mIqvis = ::CreateFile(L"\\\\.\\IQVIS",
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        nullptr,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        nullptr);
  if (!mIqvis) {
    return;
  }

  mHasSymbols = LoadSymbols();
}

IqvisClient::~IqvisClient()
{
  if (IsOpen()) {
    ::CloseHandle(mIqvis);
  }
}

bool
IqvisClient::LoadSymbols()
{
  SymbolLoader syms;
  if (!syms.Init(L"iqvis")) {
    return false;
  }
  PWSTR system32Path = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_System, 0, nullptr, &system32Path))) {
    return false;
  }
  LOKI_ON_BLOCK_EXIT(::CoTaskMemFree, system32Path);
  wstring win32kPath(system32Path);
  win32kPath += L"\\win32k.sys";
  if (!syms.LoadModule(win32kPath.c_str())) {
    return false;
  }
  mInput.cbSize = sizeof(mInput);
  mInput.offsetEnterCritAvoidingDitHitTestHazard =
    syms.GetSymbolRVA(L"EnterCritAvoidingDitHitTestHazard");
  mInput.offsetUserSessionSwitchLeaveCrit =
    syms.GetSymbolRVA(L"UserSessionSwitchLeaveCrit");
  mInput.offsetGpai = syms.GetSymbolRVA(L"gpai");
  mInput.offsetGpresUser = syms.GetSymbolRVA(L"gpresUser");
  return true;
}

bool
IqvisClient::IsOpen() const
{
  return mIqvis != NULL && mIqvis != INVALID_HANDLE_VALUE;
}

bool
IqvisClient::operator!() const
{
  return !IsOpen() || !mHasSymbols;
}

bool
IqvisClient::GetAttachedInputQueues(std::vector<IQINFOATTACHMENT>& aOutput)
{
  aOutput.clear();
  IQINFO tryOutput;
  ZeroMemory(&tryOutput, sizeof(tryOutput));
  tryOutput.count = 1;
  DWORD bytesReturned = 0;
  BOOL succeeded = ::DeviceIoControl(mIqvis, IOCTL_IQVIS_GET_INPUT_QUEUE_INFO,
                                     &mInput, sizeof(mInput),
                                     &tryOutput, sizeof(tryOutput),
                                     &bytesReturned, nullptr);
  if (!succeeded) {
    return false;
  }
  PIQINFO heapOutput = nullptr;
  LOKI_ON_BLOCK_EXIT(::free, heapOutput);
  DWORD outputLen = 0;
  if (bytesReturned == sizeof(tryOutput.count)) {
    outputLen = sizeof(IQINFO) +
                (tryOutput.count - 1) * sizeof(IQINFOATTACHMENT);
    heapOutput = reinterpret_cast<IQINFO*>(::malloc(outputLen));
    if (!heapOutput) {
      return false;
    }
    succeeded = ::DeviceIoControl(mIqvis, IOCTL_IQVIS_GET_INPUT_QUEUE_INFO,
                                  &mInput, sizeof(mInput),
                                  heapOutput, outputLen,
                                  &bytesReturned, nullptr);
    if (!succeeded) {
      return false;
    }
  }
  PIQINFO pOutput = heapOutput ? heapOutput : &tryOutput;
  for (ULONG i = 0; i < pOutput->count; ++i) {
    aOutput.push_back(pOutput->attachments[i]);
  }
  return true;
}

} // namespace aspk

