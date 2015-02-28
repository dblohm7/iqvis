#ifndef __NTUNDOC_H
#define __NTUNDOC_H

#include <ntddk.h>

/* Exported by ntoskrnl.lib */
__drv_maxIRQL(DISPATCH_LEVEL)
NTKERNELAPI
PVOID
PsGetThreadWin32Thread(
    __in PETHREAD Thread
    );

__drv_maxIRQL(APC_LEVEL)
NTKERNELAPI
PVOID
ExEnterPriorityRegionAndAcquireResourceExclusive(
    __in PERESOURCE Resource
    );

__drv_maxIRQL(DISPATCH_LEVEL)
NTKERNELAPI
HANDLE
PsGetThreadId(
    __in PETHREAD Thread
    );

typedef enum tagSYSTEM_INFORMATION_CLASS {
  SystemModuleInformation = 11
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY
{
  ULONG  Unknown1;
  ULONG  Unknown2;
#ifdef _WIN64
  ULONG Unknown3;
  ULONG Unknown4;
#endif
  PVOID  Base;
  ULONG  Size;
  ULONG  Flags;
  USHORT  Index;
  USHORT  NameLength;
  USHORT  LoadCount;
  USHORT  PathLength;
  CHAR  ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION
{
  ULONG Count;
  SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

NTKERNELAPI
NTSTATUS
ZwQuerySystemInformation(
  _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
  _Inout_ PVOID SystemInformation,
  _In_ ULONG SystemInformationLength,
  _Out_opt_ PULONG ReturnLength
  );

#endif /* __NTUNDOC_H */

