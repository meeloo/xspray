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

void MyLogOutputCallback(const char * str, void *baton)
{
  printf("LLDB> %s", str);
}


DebuggerContext::DebuggerContext()
: mDebugger(lldb::SBDebugger::Create())
{
  // Create a debugger instance so we can create a target
  const char *channel = "lldb";
  const char *categories[] =
  {
    //strm->Printf("Logging categories for 'lldb':\n"
    //     "all", // - turn on all available logging categories\n"
    //"api", // - enable logging of API calls and return values\n"
    "break", // - log breakpoints\n"
    //     "commands", // - log command argument parsing\n"
    //     "default", // - enable the default set of logging categories for liblldb\n"
    //     "dyld", // - log shared library related activities\n"
    //     "events", // - log broadcaster, listener and event queue activities\n"
    //     "expr", // - log expressions\n"
    //     "object", // - log object construction/destruction for important objects\n"
    //     "module", // - log module activities such as when modules are created, detroyed, replaced, and more\n"
    "process", // - log process events and activities\n"
    //     "script", // - log events about the script interpreter\n"
    "state",  //- log private and public process state changes\n"
    "step", // - log step related activities\n"
    "symbol", // - log symbol related issues and warnings\n"
    "target", // - log target events and activities\n"
    "thread", // - log thread events and activities\n"
    //     "types", // - log type system related activities\n"
    //     "unwind", // - log stack unwind activities\n"
    //     "verbose", // - enable verbose logging\n"
    //     "watch", // - log watchpoint related activities\n");
    NULL
  };
//  mDebugger.SetLoggingCallback(MyLogOutputCallback, NULL);
//  mDebugger.EnableLog(channel, categories);
}

DebuggerContext& Xspray::GetDebuggerContext()
{
  return ((Application*)App)->GetDebuggerContext();
}

bool DebuggerContext::LoadApp()
{
  AppDescription* pApp = mpAppDescription;
  NGL_ASSERT(pApp != NULL);
  if (pApp->GetArchitectures().empty())
  {
    NGL_OUT("Found no valid archs\n");
    return false;
  }

  // Create a target using the executable.
  //mTarget = mDebugger.CreateTarget(p.GetChars());
  mTarget = mDebugger.CreateTargetWithFileAndArch (pApp->GetLocalPath().GetChars(), pApp->GetArchitecture().GetChars());

  return mTarget.IsValid();
}


