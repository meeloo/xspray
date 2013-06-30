//
//  DebuggerContext.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/21/13.
//
//

#include "Xspray.h"
#include "Application.h"
using namespace Xspray;

DebuggerContext::DebuggerContext()
: mDebugger(lldb::SBDebugger::Create())
{

}

DebuggerContext& Xspray::GetDebuggerContext()
{
  return ((Application*)App)->GetDebuggerContext();
}

