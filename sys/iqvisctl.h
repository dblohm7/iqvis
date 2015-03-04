#ifndef __IQVISCTL_H
#define __IQVISCTL_H

#if defined(__IQVISCLIENT_H)
#include <windows.h>
#include <winioctl.h>
#else
#include <ntddk.h>
#include <ntintsafe.h>
#endif

#define IOCTL_TYPE (0x8000 + 0x42)

#define IOCTL_IQVIS_GET_INPUT_QUEUE_INFO \
  CTL_CODE(IOCTL_TYPE, (0x800 + 0x42), METHOD_BUFFERED, FILE_READ_DATA)

typedef struct tagIQINFOINPUT
{
  SIZE_T  cbSize;
  ULONG   offsetEnterCritAvoidingDitHitTestHazard;
  ULONG   offsetUserSessionSwitchLeaveCrit;
  ULONG   offsetGpai;
  ULONG   offsetGpresUser;
} IQINFOINPUT, *PIQINFOINPUT;

typedef struct tagIQINFOATTACHMENT
{
  DWORD   tidFrom;
  DWORD   tidTo;
  ULONG   count;
} IQINFOATTACHMENT;

typedef struct tagIQINFOOUTPUT
{
  ULONG             count;
  IQINFOATTACHMENT  attachments[1];
} IQINFO, *PIQINFO;

#endif /* __IQVISCTL_H */

