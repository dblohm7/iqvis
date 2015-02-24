#ifndef __WIN32K_H
#define __WIN32K_H

#include <ntddk.h>

typedef PVOID (*ENTERCRITAVOIDINGDITHITTESTHAZARD)(VOID);
typedef VOID (*USERSESSIONSWITCHLEAVECRIT)(VOID);
typedef struct tagATTACH_INFO
{
  PATTACH_INFO  next;
  PTHREADINFO   from;
  PTHREADINFO   to;
  ULONG         count;
} ATTACH_INFO, *PATTACH_INFO;

#endif // __WIN32K_H

