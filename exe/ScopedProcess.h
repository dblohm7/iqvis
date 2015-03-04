#ifndef __SCOPEDPROCESS_H
#define __SCOPEDPROCESS_H

#include <windows.h>

namespace aspk {

class ScopedProcess
{
  public:
    explicit ScopedProcess(const DWORD aPid, const DWORD aAccess = PROCESS_ALL_ACCESS)
      :process(NULL)
    {
      process = ::OpenProcess(aAccess, FALSE, aPid);
    }

    ~ScopedProcess()
    {
      if (process) {
        ::CloseHandle(process);
      }
    }

    HANDLE forget()
    {
      HANDLE tmp = process;
      process = NULL;
      return tmp;
    }

    operator HANDLE() const
    {
      return process;
    }

    bool operator!() const
    {
      return !process;
    }


  private:
    HANDLE process;
};

} // namespace aspk

#endif // __SCOPEDPROCESS_H

