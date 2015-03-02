#include "DriverLoader.h"

using namespace aspk;

int wmain(int argc, wchar_t* argv[])
{
  DriverLoader dl(L"iqvis", L"..\\sys\\Win8.1Release\\x64\\iqvis.sys");
  if (!dl) {
    return 1;
  }
  return 0;
}

