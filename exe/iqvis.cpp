#include "DriverLoader.h"
#include "IqvisClient.h"

#include <iostream>
#include <ios>

using namespace aspk;
using namespace std;

int wmain(int argc, wchar_t* argv[])
{
  DriverLoader dl(L"iqvis", L"iqvis.sys");
  if (!dl) {
    return 1;
  }
  IqvisClient client;
  if (!client) {
    return 2;
  }
  vector<IQINFOATTACHMENT> output;
  if (!client.GetAttachedInputQueues(output)) {
    return 3;
  }
  cout << "FromTID ToTID Count" << endl;
  for (auto i : output) {
    cout << i.tidFrom << " " << i.tidTo << " " << i.count << endl;
  }
  return 0;
}

