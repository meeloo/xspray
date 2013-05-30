
/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#include "nui.h"
#include "MainWindow.h"
#include "Application.h"
#include "nuiCSS.h"
#include "nuiVBox.h"

using namespace lldb;

/*
 * MainWindow
 */

MainWindow::MainWindow(const nglContextInfo& rContextInfo, const nglWindowInfo& rInfo, bool ShowFPS, const nglContext* pShared )
  : mEventSink(this),
    nuiMainWindow(rContextInfo, rInfo, pShared, nglPath(ePathCurrent))
{
  //mClearBackground = false;
  SetDebugMode(true);
  EnableAutoRotation(true);


#ifdef _DEBUG_
  nglString t = "DEBUG";
#else
  nglString t = "RELEASE";
#endif
  nuiLabel* pLabel = new nuiLabel(t);
  pLabel->SetPosition(nuiBottom);
  AddChild(pLabel);

  LoadCSS(_T("rsrc:/css/main.css"));

  nuiWidget* pTransport = nuiBuilder::Get().CreateWidget("Debugger");
  NGL_ASSERT(pTransport);
  AddChild(pTransport);

  nuiButton* pStart = (nuiButton*)pTransport->SearchForChild("StartStop", true);
  nuiButton* pPause = (nuiButton*)pTransport->SearchForChild("PauseContinue", true);
  //nuiButton* pStop = (nuiButton*)pTransport->SearchForChild("Stop", true);

  mEventSink.Connect(pStart->Activated, &MainWindow::OnStart);
  mEventSink.Connect(pPause->Activated, &MainWindow::OnPause);
  //mEventSink.Connect(pStop->Activated, &MainWindow::OnStop);
}

MainWindow::~MainWindow()
{
}


#define FONT_SIZE 35

void MainWindow::OnCreation()
{
  //EnableAutoRotation(false);
  //SetRotation(90);
  //SetState(nglWindow::eMaximize);

}

void MainWindow::OnClose()
{
  if (GetNGLWindow()->IsInModalState())
    return;
  
  
  App->Quit();
}

bool MainWindow::LoadCSS(const nglPath& rPath)
{
  nglIStream* pF = rPath.OpenRead();
  if (!pF)
  {
    NGL_OUT(_T("Unable to open CSS source file '%ls'\n"), rPath.GetChars());
    return false;
  }

  nuiCSS* pCSS = new nuiCSS();
  bool res = pCSS->Load(*pF, rPath);
  delete pF;

  if (res)
  {
    nuiMainWindow::SetCSS(pCSS);
    return true;
  }

  NGL_OUT(_T("%ls\n"), pCSS->GetErrorString().GetChars());

  delete pCSS;
  return false;
}

void MyLogOutputCallback(const char * str, void *baton)
{
  printf("LLDB> %s", str);
}

const char* GetStateName(StateType s)
{
  switch (s)
  {
    case eStateInvalid:
      return "eStateInvalid";
      break;
    case eStateUnloaded:
      return "eStateUnloaded";
      break;
    case eStateConnected:
      return "eStateConnected";
      break;
    case eStateAttaching:
      return "eStateAttaching";
      break;
    case eStateLaunching:
      return "eStateLaunching";
      break;
    case eStateStopped:
      return "eStateStopped";
      break;
    case eStateRunning:
      return "eStateRunning";
      break;
    case eStateStepping:
      return "eStateStepping";
      break;
    case eStateCrashed:
      return "eStateCrashed";
      break;
    case eStateDetached:
      return "eStateDetached";
      break;
    case eStateExited:
      return "eStateExited";
      break;
    case eStateSuspended:
      return "eStateSuspended";
      break;
  }

  return "WTF??";
}

const char* GetStopReasonName(StopReason reason)
{
  switch (reason)
  {
    case eStopReasonInvalid:
      return "Invalid";
    case eStopReasonNone:
      return "None";
    case eStopReasonTrace:
      return "Trace";
    case eStopReasonBreakpoint:
      return "Breakpoint";
    case eStopReasonWatchpoint:
      return "Watchpoint";
    case eStopReasonSignal:
      return "Signal";
    case eStopReasonException:
      return "Exception";
    case eStopReasonExec:
      return "Exec";
    case eStopReasonPlanComplete:
      return "Plan complete";
    case eStopReasonThreadExiting:
      return "Thread exiting";
  }

  return "WTF";
}

void PrintDebugState(SBProcess& rProcess)
{
  int threadcount = rProcess.GetNumThreads();
  rProcess.Stop();
  for (int i = 0; i < threadcount; i++)
  {
    SBThread thread(rProcess.GetThreadAtIndex(i));
    int framecount = thread.GetNumFrames();
    StopReason reason = thread.GetStopReason();
    SBStream st;
    bool res = thread.GetStatus (st);
    NGL_OUT("Thread %d (%s) %d frames (stop reason: %s)\n", i, thread.GetName(), framecount, GetStopReasonName(reason));
    for (int j = 0; j < framecount; j++)
    {
      SBFrame frame(thread.GetFrameAtIndex(j));
      NGL_OUT("\t%d - %s\n", j, frame.GetFunctionName());
    }
    printf("State:\n%s\n", st.GetData());
  }
  rProcess.Continue();

}


bool BPCallback (void *baton,
                 SBProcess &process,
                 SBThread &thread,
                 SBBreakpointLocation &location)
{
  PrintDebugState(process);
  return true;
}



void MainWindow::OnStart(const nuiEvent& rEvent)
{
  nglPath p("/Users/meeloo/work/build/Noodlz-cwmoeooodusxmealpbjorzgwidxy/Build/Products/Default/YaLiveD.app");
//  TestMain2(p.GetChars());
//  return;

  DebuggerContext& rContext(GetDebuggerContext());

  StateType state = rContext.mProcess.GetState();
  if (state == eStateRunning)
  {
    rContext.mProcess.Kill();
    rContext.mProcess.Clear();
    rContext.mDebugger.Clear();
    return;
  }

  // Create a debugger instance so we can create a target
  const char *channel = "lldb";
  const char *categories[] =
  {
    //strm->Printf("Logging categories for 'lldb':\n"
//     "all", // - turn on all available logging categories\n"
     "api", // - enable logging of API calls and return values\n"
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
  //rContext.mDebugger.SetLoggingCallback(MyLogOutputCallback, NULL);
//  rContext.mDebugger.EnableLog(channel, categories);
  //rContext.mDebugger.SetAsync(false);

  if (rContext.mDebugger.IsValid())
  {
    // Create a target using the executable.
    rContext.mTarget = rContext.mDebugger.CreateTargetWithFileAndArch (p.GetChars(), "i386");
    if (rContext.mTarget.IsValid())
    {
      static SBBreakpoint breakpoint = rContext.mTarget.BreakpointCreateByName("main");
      breakpoint.SetCallback(BPCallback, 0);

      SBError error;
#if 1
      rContext.mProcess = rContext.mTarget.LaunchSimple (NULL, NULL, "~/");
#else
      const char **argv = NULL;
      const char **envp = NULL;
      const char *working_directory = "~/";
      char *stdin_path = NULL;
      char *stdout_path = NULL;
      char *stderr_path = NULL;
      uint32_t launch_flags = 0; //eLaunchFlagDebug;
      bool stop_at_entry = false;
      SBListener listener = rContext.mDebugger.GetListener();
      rContext.mProcess = rContext.mTarget.Launch (listener,
                     argv,
                     envp,
                     stdin_path,
                     stdout_path,
                     stderr_path,
                     working_directory,
                     launch_flags,
                     stop_at_entry,
                     error);
#endif

      mpDebuggerEventLoop = new nglThreadDelegate(nuiMakeDelegate(this, &MainWindow::Loop));
      mpDebuggerEventLoop->Start();


//      if (rContext.mProcess.IsValid())
//      {
//        NGL_OUT("Launch (res = %s)\n", error.GetCString());
////        while (rContext.mProcess.GetState() == eStateLaunching)
////          nglThread::MsSleep(10);
//        state = rContext.mProcess.GetState();
//        NGL_OUT("State after load: %s\n", GetStateName(state));
////        error = rContext.mProcess.Stop();
////        NGL_OUT("Stop after launch (res = %s)\n", error.GetCString());
//        state = rContext.mProcess.GetState();
//
//        PrintDebugState(rContext.mProcess);
//
//        SBThread thread = rContext.mProcess.GetThreadAtIndex(0);
//        thread.Resume();
//
//        NGL_OUT("Launched!!!\n");
//        PrintDebugState(rContext.mProcess);
//      }
    }
  }

  NGL_OUT("Start\n");
  
}

void MainWindow::OnPause(const nuiEvent& rEvent)
{
  DebuggerContext& rContext(GetDebuggerContext());
  PrintDebugState(rContext.mProcess);

  SBError error;
  StateType state = rContext.mProcess.GetState();
  NGL_OUT("State on pause: %s\n", GetStateName(state));
  if (state == eStateRunning)
    error = rContext.mProcess.Stop();
  else
    error = rContext.mProcess.Continue();

  state = rContext.mProcess.GetState();
  NGL_OUT("State after pause: %s\n", GetStateName(state));

  NGL_OUT("Pause/Continuer (res = %s)\n", error.GetCString());

  PrintDebugState(rContext.mProcess);
}

void MainWindow::Loop()
{
  DebuggerContext& rContext(GetDebuggerContext());
  while (true)
	{
    if (rContext.mProcess.IsValid())
    {
      SBEvent evt;
      rContext.mDebugger.GetListener().WaitForEvent(UINT32_MAX, evt);
      StateType state = SBProcess::GetStateFromEvent(evt);

      if (SBProcess::GetRestartedFromEvent(evt))
        continue;
      switch (state)
      {
        case eStateInvalid:
        case eStateDetached:
        case eStateCrashed:
        case eStateUnloaded:
          break;
        case eStateExited:
          return;
        case eStateConnected:
        case eStateAttaching:
        case eStateLaunching:
        case eStateRunning:
        case eStateStepping:
          continue;
        case eStateStopped:
        case eStateSuspended:
        {
          bool fatal = false;
          bool selected_thread = false;
          for (auto thread_index = 0; thread_index < rContext.mProcess.GetNumThreads(); thread_index++)
          {
            SBThread thread(rContext.mProcess.GetThreadAtIndex(thread_index));
            SBFrame frame(thread.GetFrameAtIndex(0));
            bool select_thread = false;
            StopReason stop_reason = thread.GetStopReason();
            switch (stop_reason)
            {
              case eStopReasonNone:
                printf("none\n");
                break;

              case eStopReasonTrace:
                select_thread = true;
                printf("trace\n");
                break;

              case eStopReasonPlanComplete:
                select_thread = true;
                printf("plan complete\n");
                break;
              case eStopReasonThreadExiting:
                printf("thread exiting\n");
                break;
              case eStopReasonExec:
                printf("exec\n");
                break;
              case eStopReasonInvalid:
                printf("invalid\n");
                break;
              case eStopReasonException:
                select_thread = true;
                printf("exception\n");
                fatal = true;
                break;
              case eStopReasonBreakpoint:
                select_thread = true;
                printf("breakpoint id = %lld.%lld\n",thread.GetStopReasonDataAtIndex(0),thread.GetStopReasonDataAtIndex(1));
                break;
              case eStopReasonWatchpoint:
                select_thread = true;
                printf("watchpoint id = %lld\n",thread.GetStopReasonDataAtIndex(0));
                break;
              case eStopReasonSignal:
                select_thread = true;
                printf("signal %d\n",(int)thread.GetStopReasonDataAtIndex(0));
                break;
            }

            if (select_thread && !selected_thread)
            {
              selected_thread = rContext.mProcess.SetSelectedThread(thread);
            }
          }
          if (fatal)
          {
            NGL_OUT("FATAL ERROR");
            exit(1);
          }
        }
          break;
			}
		}
  }
}