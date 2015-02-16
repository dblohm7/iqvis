#ifndef __IQVISCTL_H
#define __IQVISCTL_H

#include <ntddk.h>

#define IOCTL_TYPE (0x8000 + 0x42)

#define IOCTL_IQVIS_GET_INPUT_QUEUE_INFO \
  CTL_CODE(IOCTL_TYPE, (0x800 + 0x42), METHOD_BUFFERED, FILE_READ_DATA)

typedef struct tagIQINFOINPUT
{
  USHORT  currentIQOffset;
  USHORT  prevIQOffset;
  ULONG   numTids;
  ULONG   tids[1];
}
IQINFOINPUT, *PIQINFOINPUT;

typedef struct tagTHREADIQ
{
  ULONG tid;
  PVOID currentIQ;
  PVOID prevIQ;
}
THREADIQ, *PTHREADIQ;

typedef struct tagIQINFO
{
  ULONG     count;
  THREADIQ  threadIQs[1];
}
IQINFO, *PIQINFO;

#endif /* __IQVISCTL_H */

