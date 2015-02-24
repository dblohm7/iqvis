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

/*
PsGetThreadSessionId
PsGetCurrentProcessSessionId
*/

#endif /* __NTUNDOC_H */

