#include <ntifs.h>
#include <wdm.h>

#include "iqvisctl.h"
#include "ntundoc.h"
#include "win32k.h"

#define NT_DEVICE_NAME    L"\\Device\\IQVIS"
#define WIN32_DEVICE_NAME L"\\DosDevices\\InputQueueVisualizer"

DRIVER_INITIALIZE DriverEntry;

__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)
DRIVER_DISPATCH OnCreateClose;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH OnDeviceControl;

DRIVER_UNLOAD OnUnload;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, OnCreateClose)
#pragma alloc_text(PAGE, OnDeviceControl)
#pragma alloc_text(PAGE, OnUnload)
#endif // ALLOC_PRAGMA

ENTERCRITAVOIDINGDITHITTESTHAZARD gEnterCritAvoidingDitHitTestHazard;
USERSESSIONSWITCHLEAVECRIT gUserSessionSwitchLeaveCrit;
PERESOURCE* gGpresUser;
PATTACH_INFO* gGpai;
RTL_OSVERSIONINFOW gOsv;

NTSTATUS
DriverEntry(PDRIVER_OBJECT aDriverObject, PUNICODE_STRING aRegistryPath)
{
  NTSTATUS        ntStatus;
  UNICODE_STRING  ntDeviceName;
  UNICODE_STRING  win32DeviceName;
  PDEVICE_OBJECT  devObject;

  UNREFERENCED_PARAMETER(aRegistryPath);

  RtlInitUnicodeString(&ntDeviceName, NT_DEVICE_NAME);

  ntStatus = IoCreateDevice(aDriverObject, 0, &ntDeviceName,
                            FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN,
                            FALSE, &devObject);
  if (!NT_SUCCESS(ntStatus)) {
    return ntStatus;
  }

  aDriverObject->MajorFunction[IRP_MJ_CREATE] = &OnCreateClose;
  aDriverObject->MajorFunction[IRP_MJ_CLOSE] = &OnCreateClose;
  aDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &OnDeviceControl;
  aDriverObject->DriverUnload = &OnUnload;

  RtlInitUnicodeString(&win32DeviceName, WIN32_DEVICE_NAME);
  ntStatus = IoCreateSymbolicLink(&ntDeviceName, &win32DeviceName);
  if (!NT_SUCCESS(ntStatus)) {
    IoDeleteDevice(devObject);
  }

  return ntStatus;
}

VOID OnUnload(PDRIVER_OBJECT aDriverObject)
{
  PDEVICE_OBJECT  deviceObject = aDriverObject->DeviceObject;
  UNICODE_STRING  win32DeviceName;

  PAGED_CODE();

  RtlInitUnicodeString(&win32DeviceName, WIN32_DEVICE_NAME);

  IoDeleteSymbolicLink(&win32DeviceName);

  if (deviceObject) {
    IoDeleteDevice(deviceObject);
  }
}

NTSTATUS OnCreateClose(PDEVICE_OBJECT aDeviceObject, PIRP aIrp)
{
  UNREFERENCED_PARAMETER(aDeviceObject);

  PAGED_CODE();

  aIrp->IoStatus.Status = STATUS_SUCCESS;
  aIrp->IoStatus.Information = 0;

  IoCompleteRequest(aIrp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

void EnterUserCriticalSection81(void)
{
  gEnterCritAvoidingDitHitTestHazard();
}

void LeaveUserCriticalSection(void)
{
  gUserSessionSwitchLeaveCrit();
}

void EnterUserCriticalSection7(void)
{
  ExEnterPriorityRegionAndAcquireResourceExclusive(*gGpresUser);
}

int EnterUserCriticalSection(void)
{
  if (gOsv.dwMajorVersion != 6) {
    return 0;
  }
  switch(gOsv.dwMinorVersion) {
    case 1:
      EnterUserCriticalSection7();
      return 1;
    case 3:
      EnterUserCriticalSection81();
      return 1;
    default:
      return 0;
  }
}

NTSTATUS
GetModuleBase(PCHAR aModuleName, PVOID* aOutBaseAddress)
{
  size_t i;
  NTSTATUS status;
  ULONG bufferSize = 0;
  PSYSTEM_MODULE_INFORMATION systemModuleInfo = NULL;

  if (!aModuleName || !aOutBaseAddress) {
    return STATUS_INVALID_PARAMETER;
  }

  status = ZwQuerySystemInformation(SystemModuleInformation, NULL, 0,
                                    &bufferSize);
  if (!NT_SUCCESS(status)) {
    return status;
  }

  systemModuleInfo = (PSYSTEM_MODULE_INFORMATION)
    ExAllocatePoolWithTag( NonPagedPool, bufferSize, 'IVQI');
  if (!systemModuleInfo) {
    return STATUS_NO_MEMORY;
  }

  status = ZwQuerySystemInformation(SystemModuleInformation, systemModuleInfo,
                                    bufferSize, &bufferSize);
  if (!NT_SUCCESS(status)) {
    goto end;
  }

  for (i = 0; i < systemModuleInfo->Count; ++i) {
    if (!_stricmp(systemModuleInfo->Module[i].ImageName +
                  systemModuleInfo->Module[i].PathLength, aModuleName)) {
      *aOutBaseAddress = systemModuleInfo->Module[i].Base;
      status = STATUS_SUCCESS;
      break;
    }
  }

end:
  if (systemModuleInfo) {
    ExFreePoolWithTag(systemModuleInfo, 'IVQI');
  }
  return status;
}

#define RESOLVE_ADDRESS(castTo, base, offset) \
  (castTo)(((char*)base) + offset)

NTSTATUS
ResolveGlobalData(PIQINFOINPUT aInput)
{
  PVOID win32kBase = NULL;
  NTSTATUS status;

  if (gGpai) {
    // Already initialized
    return STATUS_SUCCESS;
  }
  status = RtlGetVersion(&gOsv);
  if (!NT_SUCCESS(status)) {
    return status;
  }
  status = GetModuleBase("win32k", &win32kBase);
  if (!NT_SUCCESS(status)) {
    return status;
  }
  if (!win32kBase) {
    return STATUS_TOO_MANY_SECRETS;
  }
#pragma warning(push)
#pragma warning(disable:4055)
  gEnterCritAvoidingDitHitTestHazard =
    RESOLVE_ADDRESS(ENTERCRITAVOIDINGDITHITTESTHAZARD, win32kBase,
                    aInput->offsetEnterCritAvoidingDitHitTestHazard);
  gUserSessionSwitchLeaveCrit =
    RESOLVE_ADDRESS(USERSESSIONSWITCHLEAVECRIT, win32kBase,
                    aInput->offsetUserSessionSwitchLeaveCrit);
#pragma warning(pop)
  gGpai = RESOLVE_ADDRESS(PATTACH_INFO*, win32kBase, aInput->offsetGpai);
  gGpresUser = RESOLVE_ADDRESS(PERESOURCE*, win32kBase, aInput->offsetGpresUser);
  return STATUS_SUCCESS;
}

DWORD
ThreadInfoToTid(PTHREADINFO aThreadInfo)
{
  return (DWORD)PsGetThreadId(*aThreadInfo);
}

NTSTATUS
GetInputQueueInfo(PIQINFOINPUT aInput, PIQINFO aOutput, ULONG aOutputLen)
{
  NTSTATUS status;
  ULONG outputIndex = 0;
  ULONG requiredOutputLen = 0;

  if (aInput->cbSize != sizeof(IQINFOINPUT) ||
      !aInput->offsetEnterCritAvoidingDitHitTestHazard ||
      !aInput->offsetUserSessionSwitchLeaveCrit ||
      !aInput->offsetGpai) {
    return STATUS_INVALID_PARAMETER;
  }
  if (!NT_SUCCESS(status = ResolveGlobalData(aInput))) {
    return status;
  }
  if (!EnterUserCriticalSection()) {
    return STATUS_ACCESS_DENIED;
  }
  PATTACH_INFO node = *gGpai;
  while (node) {
    ++outputIndex;
    node = node->next;
  }
  // outputIndex should have a lower bound of 1 when measuring to account for
  // the IQINFOATTACHMENT that is already included in the IQINFO structure.
  if (outputIndex == 0) {
    outputIndex = 1;
  }
  requiredOutputLen = sizeof(IQINFO) +
                      sizeof(IQINFOATTACHMENT) * (outputIndex - 1);
  __try {
    if (aOutputLen < requiredOutputLen) {
      if (aOutputLen >= sizeof(ULONG)) {
        aOutput->count = outputIndex;
      }
      status = STATUS_INVALID_BUFFER_SIZE;
      goto end;
    }
    node = *gGpai;
    outputIndex = 0;
    while (node) {
      aOutput->attachments[outputIndex].tidFrom = ThreadInfoToTid(node->from);
      aOutput->attachments[outputIndex].tidTo = ThreadInfoToTid(node->to);
      aOutput->attachments[outputIndex].count = node->count;
      ++outputIndex;
      node = node->next;
    }
    aOutput->count = outputIndex;
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    status = STATUS_INVALID_BUFFER_SIZE;
  }
end:
  LeaveUserCriticalSection();
  return status;
}

NTSTATUS OnDeviceControl(PDEVICE_OBJECT aDeviceObject, PIRP aIrp)
{
  /* Local vars */
  NTSTATUS            ntStatus;
  PIO_STACK_LOCATION  irpStackLoc;
  ULONG               inBufLen;
  ULONG               outBufLen;
  ULONG               expectedLen;
  PIQINFOINPUT        inBuf;
  PIQINFO             outBuf;

  UNREFERENCED_PARAMETER(aDeviceObject);

  PAGED_CODE();

  /* IRP cracking */
  irpStackLoc = IoGetCurrentIrpStackLocation(aIrp);
  inBufLen = irpStackLoc->Parameters.DeviceIoControl.InputBufferLength;
  outBufLen = irpStackLoc->Parameters.DeviceIoControl.OutputBufferLength;

  if (!inBufLen || !outBufLen) {
    ntStatus = STATUS_INVALID_PARAMETER;
    goto END;
  }

  /* Switch on IOCTL code */
  switch (irpStackLoc->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_IQVIS_GET_INPUT_QUEUE_INFO:
      inBuf = (PIQINFOINPUT) aIrp->AssociatedIrp.SystemBuffer;
      outBuf = (PIQINFO) aIrp->AssociatedIrp.SystemBuffer;
      // Check the size of the input buffer
      expectedLen = sizeof(IQINFOINPUT);
      if (inBufLen < expectedLen) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto END;
      }
      ntStatus = GetInputQueueInfo(inBuf, outBuf, outBufLen);
      break;
    default:
      ntStatus = STATUS_INVALID_DEVICE_REQUEST;
      break;
  }

END:

  aIrp->IoStatus.Status = ntStatus;
  IoCompleteRequest(aIrp, IO_NO_INCREMENT);

  return ntStatus;
}

