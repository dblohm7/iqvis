#ifndef __WIN32K_H
#define __WIN32K_H

#include <ntddk.h>

typedef PVOID (*ENTERCRITAVOIDINGDITHITTESTHAZARD)(VOID);
typedef VOID (*USERSESSIONSWITCHLEAVECRIT)(VOID);
/* This is not the real definition of THREADINFO, but should hopefully give
   us enough to be able to resolve a thread. */
typedef PETHREAD THREADINFO, *PTHREADINFO;
typedef struct tagATTACH_INFO
{
  struct tagATTACH_INFO*  next;
  PTHREADINFO             from;
  PTHREADINFO             to;
  ULONG                   count;
} ATTACH_INFO, *PATTACH_INFO;

#endif // __WIN32K_H

