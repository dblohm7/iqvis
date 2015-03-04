#ifndef __IQVISCLIENT_H
#define __IQVISCLIENT_H

#include "iqvisctl.h"

#include <vector>

namespace aspk {

class IqvisClient
{
public:
  IqvisClient();
  ~IqvisClient();

  bool operator!() const;

  bool GetAttachedInputQueues(std::vector<IQINFOATTACHMENT>& aOutput);

private:
  bool IsOpen() const;
  bool LoadSymbols();

  HANDLE      mIqvis;
  bool        mHasSymbols;
  IQINFOINPUT mInput;
};

} // namespace aspk

#endif // __IQVISCLIENT_H

