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

/*
PsGetThreadSessionId
PsGetCurrentProcessSessionId
*/

#endif /* __NTUNDOC_H */

