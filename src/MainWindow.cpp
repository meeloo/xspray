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

void LogOutputCallback(const char * str, void *baton)
{
  printf("LLDB Output> %s\n", str);
}

const char* GetStateName(lldb::StateType s)
{
  switch (s)
  {
    case lldb::eStateInvalid:
      return "eStateInvalid";
      break;
    case lldb::eStateUnloaded:
      return "eStateUnloaded";
      break;
    case lldb::eStateConnected:
      return "eStateConnected";
      break;
    case lldb::eStateAttaching:
      return "eStateAttaching";
      break;
    case lldb::eStateLaunching:
      return "eStateLaunching";
      break;
    case lldb::eStateStopped:
      return "eStateStopped";
      break;
    case lldb::eStateRunning:
      return "eStateRunning";
      break;
    case lldb::eStateStepping:
      return "eStateStepping";
      break;
    case lldb::eStateCrashed:
      return "eStateCrashed";
      break;
    case lldb::eStateDetached:
      return "eStateDetached";
      break;
    case lldb::eStateExited:
      return "eStateExited";
      break;
    case lldb::eStateSuspended:
      return "eStateSuspended";
      break;
  }

  return "WTF??";
}

void MainWindow::OnStart(const nuiEvent& rEvent)
{
  nglPath p("/Users/meeloo/work/build/Noodlz-cwmoeooodusxmealpbjorzgwidxy/Build/Products/Default/YaLiveD.app");

  DebuggerContext& rContext(GetDebuggerContext());

  lldb::StateType state = rContext.mProcess.GetState();
  if (state == lldb::eStateRunning)
  {
    rContext.mProcess.Kill();
    rContext.mProcess.Clear();
    rContext.mDebugger.Clear();
    return;
  }

  // Create a debugger instance so we can create a target
  rContext.mDebugger.SetLoggingCallback(LogOutputCallback, NULL);

  if (rContext.mDebugger.IsValid())
  {
    // Create a target using the executable.
    rContext.mTarget = rContext.mDebugger.CreateTargetWithFileAndArch (p.GetChars(), "x86_64");
    if (rContext.mTarget.IsValid())
    {
      //rContext.mProcess = rContext.mTarget.LaunchSimple (NULL, NULL, "~/");
      const char **argv = NULL;
      const char **envp = NULL;
      const char *working_directory = "~/";
      char *stdin_path = NULL;
      char *stdout_path = NULL;
      char *stderr_path = NULL;
      uint32_t launch_flags = lldb::eLaunchFlagDebug;
      bool stop_at_entry = true;
      lldb::SBError error;
      lldb::SBListener listener = rContext.mTarget.GetDebugger().GetListener();
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

      if (rContext.mProcess.IsValid())
      {
        NGL_OUT("Launch (res = %s)\n", error.GetCString());
//        while (rContext.mProcess.GetState() == lldb::eStateLaunching)
//          nglThread::MsSleep(10);
        state = rContext.mProcess.GetState();
        error = rContext.mProcess.Stop();
        NGL_OUT("Stop after launch (res = %s)\n", error.GetCString());
        state = rContext.mProcess.GetState();

        NGL_OUT("Launched!!!\n");
      }
    }
  }

  NGL_OUT("Start\n");
  
}

void MainWindow::OnPause(const nuiEvent& rEvent)
{
  lldb::SBError error;
  DebuggerContext& rContext(GetDebuggerContext());
  lldb::StateType state = rContext.mProcess.GetState();
  if (state == lldb::eStateRunning)
    error = rContext.mProcess.Stop();
  else
    error = rContext.mProcess.Continue();

  NGL_OUT("Pause/Continuer (res = %s)\n", error.GetCString());
}

