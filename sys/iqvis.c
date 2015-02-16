#include <ntifs.h>
#include <wdm.h>

#include "ntundoc.h"
#include "iqvisctl.h"

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
  PIQINFO             tmpOutBuf;
  PCHAR               threadInfo;
  PETHREAD            ethread;
  ULONG               outIdx = 0;
  ULONG               i;

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
      // Make sure we're checking at least 2 tids (otherwise what's the point?)
      if (inBuf->numTids < 2) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto END;
      }
      // Check the size of the input buffer
      expectedLen = sizeof(IQINFOINPUT) + (inBuf->numTids - 1) * sizeof(ULONG);
      if (inBufLen < expectedLen) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto END;
      }
      // Check the size of the output buffer based on expected output size
      expectedLen = sizeof(IQINFO) + (inBuf->numTids - 1) * sizeof(THREADIQ);
      if (outBufLen < expectedLen) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto END;
      }
      // Allocate a temporary buffer for output
      tmpOutBuf = (PIQINFO)ExAllocatePoolWithTag(PagedPool, expectedLen, 'NIQI');
      if (!tmpOutBuf) {
        ntStatus = STATUS_NO_MEMORY;
        goto END;
      }
      RtlZeroMemory(tmpOutBuf, expectedLen);
      // Now let's gather some info
      for (i = 0; i < inBuf->numTids; ++i) {
        // Resolve ETHREAD from thread id
        ntStatus = PsLookupThreadByThreadId((HANDLE)inBuf->tids[i], &ethread);
        if (!NT_SUCCESS(ntStatus)) {
          continue;
        }
        // Get the THREADINFO structure
        threadInfo = PsGetThreadWin32Thread(ethread);
        if (!threadInfo) {
          // Not all threads have Win32 info, just the ones that call user32
          ObDereferenceObject(ethread);
          continue;
        }
        tmpOutBuf->threadIQs[outIdx].tid = inBuf->tids[i];
        tmpOutBuf->threadIQs[outIdx].currentIQ = (PVOID)(threadInfo + inBuf->currentIQOffset);
        tmpOutBuf->threadIQs[outIdx].prevIQ = (PVOID)(threadInfo + inBuf->prevIQOffset);
        ObDereferenceObject(ethread);
        ++outIdx;
      }
      RtlCopyBytes(outBuf, tmpOutBuf, expectedLen);
      ExFreePoolWithTag(tmpOutBuf, 'NIQI');
      ntStatus = STATUS_SUCCESS;
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

