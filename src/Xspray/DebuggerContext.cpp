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
         "commands", // - log command argument parsing\n"
         "default", // - enable the default set of logging categories for liblldb\n"
         "dyld", // - log shared library related activities\n"
         "events", // - log broadcaster, listener and event queue activities\n"
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
         "verbose", // - enable verbose logging\n"
    //     "watch", // - log watchpoint related activities\n");
    NULL
  };
  mDebugger.SetLoggingCallback(MyLogOutputCallback, NULL);
  mDebugger.EnableLog(channel, categories);
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
  const char* apppath = pApp->GetLocalPath().GetChars();
  const char* arch = pApp->GetArchitecture().GetChars();
  mTarget = mDebugger.CreateTargetWithFileAndArch (apppath, arch);

  return mTarget.IsValid();
}

Breakpoint* DebuggerContext::CreateBreakpointByLocation(const nglPath& rPath, int32 line, int32 column)
{
  lldb::SBBreakpoint bp = mTarget.BreakpointCreateByLocation(rPath.GetChars(), line);
  Breakpoint* pBP = new Breakpoint(bp, rPath, line, column);
  mBreakpoints.push_back(pBP);
  return pBP;
}

Breakpoint* DebuggerContext::CreateBreakpointByName(const nglString& rSymbol)
{
  lldb::SBBreakpoint bp = mTarget.BreakpointCreateByName(rSymbol.GetChars());
  Breakpoint* pBP = new Breakpoint(bp, rSymbol, false);
  mBreakpoints.push_back(pBP);
  return pBP;
}

Breakpoint* DebuggerContext::CreateBreakpointByRegex(const nglString& rRegEx)
{
  lldb::SBBreakpoint bp = mTarget.BreakpointCreateByRegex(rRegEx.GetChars());
  Breakpoint* pBP = new Breakpoint(bp, rRegEx, true);
  mBreakpoints.push_back(pBP);
  return pBP;
}

Breakpoint* DebuggerContext::CreateBreakpointForException(lldb::LanguageType language, bool BreakOnCatch, bool BreakOnThrow)
{
  lldb::SBBreakpoint bp = mTarget.BreakpointCreateForException(language, BreakOnCatch, BreakOnThrow);
  Breakpoint* pBP = new Breakpoint(bp, language, BreakOnCatch, BreakOnThrow);
  mBreakpoints.push_back(pBP);
  return pBP;
}

void DebuggerContext::GetBreakpointsForFile(const nglPath& rPath, std::vector<Breakpoint*>& rBreakpoints)
{
  auto it = mBreakpoints.begin();
  while (it != mBreakpoints.end())
  {
    Breakpoint* pBP = *it;
    if (pBP->GetType() == Breakpoint::Location && pBP->GetPath() == rPath)
      rBreakpoints.push_back(pBP);
    ++it;
  }
}

void DebuggerContext::GetBreakpointsLinesForFile(const nglPath& rPath, std::set<int32>& rLines)
{
  //NGL_OUT("DebuggerContext::GetBreakpointsLinesForFile(%s) (among %d breakpoints)\n", rPath.GetChars(), mBreakpoints.size());
  auto it = mBreakpoints.begin();
  while (it != mBreakpoints.end())
  {
    Breakpoint* pBP = *it;
    if (pBP->GetType() == Breakpoint::Location && pBP->GetPath() == rPath)
    {
      rLines.insert(pBP->GetLine());
//      NGL_OUT("   OK for %s %d\n", pBP->GetPath().GetChars(), pBP->GetLine());
    }
    else
    {
//      NGL_OUT("   Skip %s %d\n", pBP->GetPath().GetChars(), pBP->GetLine());
    }
    ++it;
  }
}


void DebuggerContext::GetBreakpointsForFiles(std::vector<Breakpoint*>& rBreakpoints)
{
  auto it = mBreakpoints.begin();
  while (it != mBreakpoints.end())
  {
    Breakpoint* pBP = *it;
    if (pBP->GetType() == Breakpoint::Location)
      rBreakpoints.push_back(pBP);
    ++it;
  }
}

void DebuggerContext::GetBreakpointsForExceptions(std::vector<Breakpoint*>& rBreakpoints)
{
  auto it = mBreakpoints.begin();
  while (it != mBreakpoints.end())
  {
    Breakpoint* pBP = *it;
    if (pBP->GetType() == Breakpoint::Exception)
      rBreakpoints.push_back(pBP);
    ++it;
  }
}

void DebuggerContext::GetBreakpointsForSymbols(std::vector<Breakpoint*>& rBreakpoints)
{
  auto it = mBreakpoints.begin();
  while (it != mBreakpoints.end())
  {
    Breakpoint* pBP = *it;
    if (pBP->GetType() == Breakpoint::Symbolic)
      rBreakpoints.push_back(pBP);
    ++it;
  }
}

void DebuggerContext::DeleteBreakpoint(Breakpoint* pBreakpoint)
{
  auto it = mBreakpoints.begin();
  while (it != mBreakpoints.end())
  {
    Breakpoint* pBP = *it;
    if (pBP == pBreakpoint)
    {
      mBreakpoints.erase(it);
      lldb::SBBreakpoint bp = pBreakpoint->GetBreakpoint();
      mTarget.BreakpointDelete(bp.GetID());
      delete pBreakpoint;
      return;
    }
    ++it;
  }
}

Breakpoint* DebuggerContext::GetBreakpointByLocation(const nglPath& rPath, int32 line, int32 col) const
{
  //NGL_OUT("DebuggerContext::GetBreakpointsLinesForFile(%s) (among %d breakpoints)\n", rPath.GetChars(), mBreakpoints.size());
  auto it = mBreakpoints.begin();
  while (it != mBreakpoints.end())
  {
    Breakpoint* pBP = *it;
    if (pBP->GetType() == Breakpoint::Location && pBP->GetPath() == rPath && pBP->GetLine() == line && pBP->GetColumn() == col)
    {
      return pBP;
    }
    ++it;
  }

  return NULL;
}
