//
//  DebugView.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/30/13.
//
//

#include "Xspray.h"

using namespace Xspray;
using namespace lldb;

//class DebugView : public nuiSimpleContainer
DebugView::DebugView()
: nuiLayout(),
  mEventSink(this)
{
  if (SetObjectClass("DebugView"))
  {
    // Add Attributes
  }
}

DebugView::~DebugView()
{

}

void DebugView::Built()
{
  mpThreads = (nuiTreeView*)SearchForChild("Threads", true);
  NGL_ASSERT(mpThreads);
  mpVariables = (nuiTreeView*)SearchForChild("Variables", true);
  NGL_ASSERT(mpVariables);
  mpVariables->EnableSubElements(2);
  mpVariables->SetSubElementWidth(0, 200);
  mpVariables->SetSubElementWidth(1, 200);

  mpModulesFiles = (nuiTreeView*)SearchForChild("ModulesFiles", true);
  NGL_ASSERT(mpModulesFiles);
  mpModulesSymbols = (nuiTreeView*)SearchForChild("ModulesSymbols", true);
  NGL_ASSERT(mpModulesSymbols);

  nuiScrollView* pScroller = (nuiScrollView*)SearchForChild("ThreadsScroller", true);
  pScroller->ActivateHotRect(false, true);
  pScroller = (nuiScrollView*)SearchForChild("VariablesScroller", true);
  pScroller->ActivateHotRect(false, true);


  mpTransport = (nuiWidget*)SearchForChild("Transport", true);
  mpChooseApplication = (nuiButton*)SearchForChild("ChooseApplication", true);
  mpArchitecturesCombo = (nuiComboBox*)SearchForChild("Architectures", true);
  mpDevicesCombo = (nuiComboBox*)SearchForChild("Devices", true);
  mpStart = (nuiButton*)SearchForChild("StartStop", true);
  mpPause = (nuiButton*)SearchForChild("Pause", true);
  mpContinue = (nuiButton*)SearchForChild("Continue", true);
  mpStepIn = (nuiButton*)SearchForChild("StepIn", true);
  mpStepOver = (nuiButton*)SearchForChild("StepOver", true);
  mpStepOut = (nuiButton*)SearchForChild("StepOut", true);
  mpFilesTabView = (nuiTabView*)SearchForChild("FilesTabView", true);

  mpGraphView = (GraphView*)SearchForChild("SharedPlotter", true);

  mpOutput = (nuiText*)SearchForChild("STDOUT", true);
  mpErrors = (nuiText*)SearchForChild("STDERR", true);

  mpDevices = new nuiTreeNode("Devices");
  mpDevicesCombo->SetTree(mpDevices);

  mpArchitectures = new nuiTreeNode("Archs");
  mpArchitecturesCombo->SetTree(mpArchitectures);


  mEventSink.Connect(mpChooseApplication->Activated, &DebugView::OnChooseApplication);
  mEventSink.Connect(mpStart->Activated, &DebugView::OnStart);
  mEventSink.Connect(mpPause->Activated, &DebugView::OnPause);
  mEventSink.Connect(mpContinue->Activated, &DebugView::OnContinue);
  mEventSink.Connect(mpStepIn->Activated, &DebugView::OnStepIn);
  mEventSink.Connect(mpStepOver->Activated, &DebugView::OnStepOver);
  mEventSink.Connect(mpStepOut->Activated, &DebugView::OnStepOut);

  mEventSink.Connect(mpThreads->SelectionChanged, &DebugView::OnThreadSelectionChanged);
  mEventSink.Connect(mpModulesFiles->SelectionChanged, &DebugView::OnModuleFileSelectionChanged);
  mEventSink.Connect(mpModulesSymbols->SelectionChanged, &DebugView::OnModuleSymbolSelectionChanged);

  mEventSink.Connect(mpVariables->SelectionChanged, &DebugView::OnVariableSelectionChanged);

  mEventSink.Connect(nuiAnimation::GetTimer()->Tick, &DebugView::OnHandleSTDIO);

  mSlotSink.Connect(iOSDevice::DeviceConnected, nuiMakeDelegate(this, &DebugView::OnDeviceConnected));
  mSlotSink.Connect(iOSDevice::DeviceDisconnected, nuiMakeDelegate(this, &DebugView::OnDeviceDisconnected));

  // Load modules:
  DebuggerContext& rContext(GetDebuggerContext());
  ModuleTree* pFilesTree = new ModuleTree(rContext.mTarget);
  pFilesTree->Acquire();
  pFilesTree->Open(true);
  mpModulesFiles->SetTree(pFilesTree);

  SymbolTree* pSymbolsTree = new SymbolTree(rContext.mTarget);
  pSymbolsTree->Acquire();
  pSymbolsTree->Open(true);
  mpModulesSymbols->SetTree(pSymbolsTree);

}

void DebugView::OnDeviceConnected(iOSDevice& device)
{
  std::map<nglString, nglString> dico;
  dico["DeviceUDID"] = device.GetUDID();
  dico["DeviceName"] = device.GetName();
  dico["DeviceType"] = device.GetTypeName();
  dico["DeviceVersion"] = device.GetVersionString();

  nuiWidget* pWidget = nuiBuilder::Get().CreateWidget("iOSDeviceLabel", dico);

  nuiTreeNodePtr pNode = new nuiTreeNode(pWidget, false, false, false, false);
  pNode->SetToken(new nuiToken<iOSDevice&>(device));
  mpDevices->AddChild(pNode);
}

void DebugView::OnDeviceDisconnected(iOSDevice& device)
{
  for (int32 i = 0; i < mpDevices->GetChildrenCount(); i++)
  {
    nuiTreeNode* pNode = (nuiTreeNode*)mpDevices->GetChild(i);
    NGL_ASSERT(pNode);
    nuiToken<iOSDevice&>* pToken = (nuiToken<iOSDevice&>*)pNode->GetToken();
    NGL_ASSERT(pToken);
    iOSDevice* pDevice = &pToken->Token;
    NGL_ASSERT(pDevice);

    if (&device == pDevice)
    {
      mpDevices->DelChild(i);
      return;
    }
  }
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
    //    printf("State:\n%s\n", st.GetData());
  }
}


bool BPCallback (void *baton,
                 SBProcess &process,
                 SBThread &thread,
                 SBBreakpointLocation &location)
{
  PrintDebugState(process);
  return true;
}


void DebugView::OnChooseApplication(const nuiEvent& rEvent)
{
  
  nuiAnimation::RunOnAnimationTick(nuiMakeTask(&GoHome, &nuiSignal0<>::operator()));
}

void DebugView::OnStart(const nuiEvent& rEvent)
{
  DebuggerContext& rContext(GetDebuggerContext());
  NGL_ASSERT(rContext.mpAppDescription != NULL);
  if (rContext.mpAppDescription->GetVendor() == "apple")
  {
    if (rContext.mpAppDescription->GetTargetOS() == "ios")
    {
      OnStartIOS();
    }
    if (rContext.mpAppDescription->GetTargetOS() == "darwin" || rContext.mpAppDescription->GetTargetOS() == "macosx")
    {
      OnStartLOCAL();
    }
  }
  else
  {
    NGL_OUT("Vendor '%s' not suported\n", rContext.mpAppDescription->GetVendor().GetChars());
  }
}

void DebugView::OnStartLOCAL()
{
  DebuggerContext& rContext(GetDebuggerContext());
  NGL_ASSERT(rContext.mpAppDescription != NULL);
  nglPath p = rContext.mpAppDescription->GetLocalPath();

  StateType state = rContext.mProcess.GetState();
  if (state == eStateRunning)
  {
    rContext.mProcess.Kill();
    rContext.mProcess.Clear();
    rContext.mDebugger.Clear();
    return;
  }


  mpDebuggerEventLoop = new nglThreadDelegate(nuiMakeDelegate(this, &DebugView::Loop));
  mpDebuggerEventLoop->Start();

  if (rContext.mDebugger.IsValid())
  {
    if (rContext.mTarget.IsValid())
    {
      //static SBBreakpoint breakpoint1 = rContext.mTarget.BreakpointCreateByName("TestXspray");
      //static SBBreakpoint breakpoint2 = rContext.mTarget.BreakpointCreateByLocation("Application.cpp", 67);
      Breakpoint* breakpoint = rContext.CreateBreakpointByLocation("/Users/meeloo/work/yalive/src/Application.cpp", 67, 0);
      NGL_OUT("Breakpoint is valid: %s", YESNO(breakpoint->IsValid()));
      //breakpoint.SetCallback(BPCallback, 0);

      SBError error;
#if 1
      rContext.mProcess = rContext.mTarget.LaunchSimple (NULL, NULL, "/");
#else
      const char **argv = NULL;
      const char **envp = NULL;
      const char *working_directory = "/";
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

    }
  }

  //UpdateProcess();
  NGL_OUT("Start\n");

}

void DebugView::OnStartIOS()
{
  DebuggerContext& rContext(GetDebuggerContext());
  NGL_ASSERT(rContext.mpAppDescription != NULL);
  nglPath p = rContext.mpAppDescription->GetLocalPath();

  NGL_ASSERT(iOSDevice::GetDeviceCount() > 0);
  iOSDevice* pDevice = iOSDevice::GetDevice(0);
  if (!pDevice->InstallApplication(p))
  {
    NGL_OUT("Unable to install application on device\n");
  }

  if (!pDevice->StartDebugServer())
  {
    NGL_OUT("Unable to start debug server on device\n");
  }

  StateType state = rContext.mProcess.GetState();
  if (state == eStateRunning)
  {
    rContext.mProcess.Kill();
    rContext.mProcess.Clear();
    rContext.mDebugger.Clear();
    return;
  }

  mpDebuggerEventLoop = new nglThreadDelegate(nuiMakeDelegate(this, &DebugView::Loop));
  mpDebuggerEventLoop->Start();


  if (rContext.mDebugger.IsValid())
  {
    // Create a target using the executable.

    rContext.mTarget = rContext.mDebugger.CreateTargetWithFileAndArch (p.GetChars(), rContext.mpAppDescription->GetArchitecture().GetChars());
    if (rContext.mTarget.IsValid())
    {

      {
        // Change the remote executable in the target before connecting:
        NGL_ASSERT(iOSDevice::GetDeviceCount() > 0);
        iOSDevice* pDevice = iOSDevice::GetDevice(0);

        nglString APPID = pDevice->GetDiskAppIdentifier(p.GetPathName());
        printf("AppID: %s\n", APPID.GetChars());
        nglString AppUrl = pDevice->GetDeviceAppURL(APPID);
        printf("AppUrl: %s\n", AppUrl.GetChars());

        if (AppUrl.IsEmpty())
        {
          NGL_OUT("ERROR: Unable to get application URL from device");
          return;
        }
        int todelete = strlen("file://localhost");
        AppUrl.DeleteLeft(todelete);
        AppUrl.DeleteRight(1);
        printf("Trimmed AppUrl: %s\n", AppUrl.GetChars());


        SBFileSpec executable = rContext.mTarget.GetExecutable();
        NGL_OUT("Local Executable: %s/%s\n",  executable.GetDirectory(), executable.GetFilename());
        SBModule exe_module = rContext.mTarget.FindModule(executable);
        if (!exe_module.IsValid())
        {
          NGL_OUT("Remote Launch impossible: unable to find the executable module\n");
          return;
        }

        SBFileSpec remote_path = exe_module.GetPlatformFileSpec();
        NGL_OUT("Current remote Executable: %s/%s\n",  remote_path.GetDirectory(), remote_path.GetFilename());
        remote_path = SBFileSpec(AppUrl.GetChars(), false);
        exe_module.SetPlatformFileSpec (remote_path);
        NGL_OUT("New remote Executable: %s/%s\n",  remote_path.GetDirectory(), remote_path.GetFilename());
      }

      if (0)
      {
        uint32_t c = rContext.mDebugger.GetNumCategories ();
        for (int i = 0; i < c; i++)
        {
          SBTypeCategory cat = rContext.mDebugger.GetCategoryAtIndex(i);
          printf("cat %d %s\t\t\t(%s - %d formats - %d summaries - %d filters)\n", i, cat.GetName(), cat.GetEnabled()?"Enabled":"Disabled", cat.GetNumFormats(), cat.GetNumSummaries(), cat.GetNumFilters());

        }
      }

      SBError error;
      nglString url;
      url.CFormat("fd://%d", pDevice->GetDebugSocket());
      NGL_OUT("Debug URL: %s\n", url.GetChars());
      SBListener listener = rContext.mDebugger.GetListener();
      rContext.mProcess = rContext.mTarget.ConnectRemote (listener,
                                                          url.GetChars(),
                                                          NULL,
                                                          error);
    }
  }

  //UpdateProcess();
  NGL_OUT("Start\n");

}

void DebugView::OnPause(const nuiEvent& rEvent)
{
  DebuggerContext& rContext(GetDebuggerContext());
  SBError error;
  StateType state = rContext.mProcess.GetState();
  NGL_OUT("State on pause: %s\n", GetStateName(state));
  error = rContext.mProcess.Stop();
}

void DebugView::OnContinue(const nuiEvent& rEvent)
{
  DebuggerContext& rContext(GetDebuggerContext());
  SBError error;
  StateType state = rContext.mProcess.GetState();
  NGL_OUT("State on continue: %s\n", GetStateName(state));
  error = rContext.mProcess.Continue();
}

void DebugView::OnStepIn(const nuiEvent& rEvent)
{
  DebuggerContext& rContext(GetDebuggerContext());
  SBThread thread = rContext.mProcess.GetSelectedThread();
  thread.StepInto();
}

void DebugView::OnStepOver(const nuiEvent& rEvent)
{
  DebuggerContext& rContext(GetDebuggerContext());
  SBThread thread = rContext.mProcess.GetSelectedThread();
  thread.StepOver();
}

void DebugView::OnStepOut(const nuiEvent& rEvent)
{
  DebuggerContext& rContext(GetDebuggerContext());
  SBThread thread = rContext.mProcess.GetSelectedThread();
  thread.StepOut();
}

void DebugView::Loop()
{
  DebuggerContext& rContext(GetDebuggerContext());
  while (true)
	{
    if (rContext.mProcess.IsValid())
    {
      SBEvent evt;
      rContext.mDebugger.GetListener().WaitForEvent(UINT32_MAX, evt);
      StateType state = SBProcess::GetStateFromEvent(evt);

      if (0 && SBProcess::GetRestartedFromEvent (evt))
      {
        size_t num_reasons = SBProcess::GetNumRestartedReasonsFromEvent(evt);
        if (num_reasons > 0)
        {
          // FIXME: Do we want to report this, or would that just be annoyingly chatty?
          if (num_reasons == 1)
          {
            char message[1024];
            const char *reason = SBProcess::GetRestartedReasonAtIndexFromEvent (evt, 0);
            ::snprintf (message, sizeof(message), "Process %lld stopped and restarted: %s\n",
                        rContext.mProcess.GetProcessID(), reason ? reason : "<UNKNOWN REASON>");
            printf("%s", message);
          }
          else
          {
            char message[1024];
            ::snprintf (message, sizeof(message), "Process %lld stopped and restarted, reasons:\n",
                        rContext.mProcess.GetProcessID());
            printf("%s", message);
            for (size_t i = 0; i < num_reasons; i++)
            {
              const char *reason = SBProcess::GetRestartedReasonAtIndexFromEvent (evt, i);
              ::snprintf(message, sizeof(message), "\t%s\n", reason ? reason : "<UNKNOWN REASON>");
              printf("%s", message);
            }
          }
        }
      }

      if (SBProcess::GetRestartedFromEvent(evt))
        continue;
      switch (state)
      {
        case eStateInvalid:
          printf("StateInvalid\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessRunning));
          break;
        case eStateDetached:
          printf("StateDetached\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessRunning));
          break;
        case eStateCrashed:
          printf("StateCrashed\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessPaused));
          break;
        case eStateUnloaded:
          printf("StateUnloaded\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessRunning));
          break;
        case eStateExited:
          printf("StateExited\n");
          break;
        case eStateConnected:
          printf("StateConnected\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessConnected));
          break;
        case eStateAttaching:
          printf("StateAttaching\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessRunning));
          break;
        case eStateLaunching:
          printf("StateLaunching\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessRunning));
          break;
        case eStateRunning:
          //printf("StateRunning\n"); break;
        case eStateStepping:
          printf("StateStepping\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessRunning));
          break;
        case eStateStopped:
        case eStateSuspended:
          printf("StateStopped or StateSuspended\n");
          nuiAnimation::RunOnAnimationTick(nuiMakeTask(this, &DebugView::OnProcessPaused));
          break;
			}
		}
  }
}

void DebugView::OnProcessConnected()
{
  DebuggerContext& rContext(GetDebuggerContext());
  StateType state = rContext.mProcess.GetState();


  //AppUrl = "C385ADFD-3D09-4BC5-9B17-3F547E043156";
  const char **argv = NULL;
  const char **envp = NULL;
  const char *working_directory = NULL;
  char *stdin_path = NULL;
  char *stdout_path = NULL;
  char *stderr_path = NULL;
  uint32_t launch_flags = 0; //eLaunchFlagDebug;
  bool stop_at_entry = false;
  SBError error;
  bool res = rContext.mProcess.RemoteLaunch (argv,
                                             envp,
                                             stdin_path,
                                             stdout_path,
                                             stderr_path,
                                             working_directory,
                                             launch_flags,
                                             stop_at_entry,
                                             error);

  NGL_OUT("Remote Launch result: %s\n", error.GetCString());
}

void DebugView::OnProcessPaused()
{
  mpStart->SetEnabled(false);
  mpPause->SetEnabled(false);
  mpContinue->SetEnabled(true);
  mpStepIn->SetEnabled(true);
  mpStepOver->SetEnabled(true);
  mpStepOut->SetEnabled(true);
  mpThreads->SetEnabled(true);
  mpVariables->SetEnabled(true);
  UpdateProcess();
}

void DebugView::OnProcessRunning()
{
  mpStart->SetEnabled(false);
  mpPause->SetEnabled(true);
  mpContinue->SetEnabled(false);
  mpStepIn->SetEnabled(false);
  mpStepOver->SetEnabled(false);
  mpStepOut->SetEnabled(false);
  mpThreads->SetEnabled(false);
  mpVariables->SetEnabled(false);
}

void DebugView::UpdateProcess()
{
  DebuggerContext& rContext(GetDebuggerContext());
  //PrintDebugState(rContext.mProcess);

  ProcessTree* pTree = new ProcessTree(rContext.mProcess);
  pTree->Acquire();
  pTree->Open(true);
  mpThreads->SetTree(pTree);
  UpdateVariablesForCurrentFrame();
}

void DebugView::SelectProcess(SBProcess process)
{
  //  NGL_OUT("Selected Process\n");
}

void DebugView::SelectThread(SBThread thread)
{
  //  NGL_OUT("Selected Thread\n");
}

void DebugView::UpdateVariables(SBFrame frame)
{
  nuiTreeNode* pTree = new nuiTreeNode("Variables");

  DynamicValueType dynamic = eDynamicCanRunTarget; // eNoDynamicValues, eDynamicCanRunTarget, eDynamicDontRunTarget
  SBValueList args;
  args = frame.GetVariables(
                            true, //bool arguments,
                            false, //bool locals,
                            false, //bool statics,
                            false, //bool in_scope_only);
                            dynamic
                            );

  SBValueList locals;
  locals = frame.GetVariables(
                              false, //bool arguments,
                              true, //bool locals,
                              false, //bool statics,
                              false, //bool in_scope_only);
                              dynamic
                              );

  SBValueList globals;
  globals = frame.GetVariables(
                               false, //bool arguments,
                               false, //bool locals,
                               true, //bool statics,
                               false, //bool in_scope_only);
                               dynamic
                               );

  uint32_t count = 0;
  nuiTreeNode* pArgNode = new nuiTreeNode("Arguments");
  pTree->AddChild(pArgNode);
  count = args.GetSize();
  for (uint32 i = 0; i < count; i++)
  {
    SBValue val = args.GetValueAtIndex(i);
    if (val.IsInScope())
    {
      nuiTreeNode* pNode = new VariableNode(val);
      pArgNode->AddChild(pNode);
    }
    //NGL_OUT("%d %s %s \n", i, val.GetTypeName(), val.GetName());
  }
  pArgNode->Open(true);

  count = locals.GetSize();
  nuiTreeNode* pLocalNode = new nuiTreeNode("Locals");
  pTree->AddChild(pLocalNode);
  for (uint32 i = 0; i < count; i++)
  {
    SBValue val = locals.GetValueAtIndex(i);
    if (val.IsInScope())
    {
      nuiTreeNode* pNode = new VariableNode(val);
      pArgNode->AddChild(pNode);
    }
    //NGL_OUT("%d %s %s \n", i, val.GetTypeName(), val.GetName());
  }
  pLocalNode->Open(true);

  count = globals.GetSize();
  nuiTreeNode* pGlobalNode = new nuiTreeNode("Globals");
  pTree->AddChild(pGlobalNode);
  for (uint32 i = 0; i < count; i++)
  {
    SBValue val = globals.GetValueAtIndex(i);
    if (val.IsInScope())
    {
      nuiTreeNode* pNode = new VariableNode(val);
      pArgNode->AddChild(pNode);
    }
    //NGL_OUT("%d %s %s \n", i, val.GetTypeName(), val.GetName());
  }
  pGlobalNode->Open(true);

  pTree->Open(true);
  mpVariables->SetTree(pTree);
}

void DebugView::SelectFrame(SBFrame frame)
{
  //  NGL_OUT("Selected Frame\n");

  UpdateVariables(frame);
  SBSymbolContext context = frame.GetSymbolContext(1);
  SBLineEntry lineentry = frame.GetLineEntry();
  SBFileSpec file =  lineentry.GetFileSpec();
  uint32_t line = lineentry.GetLine();
  uint32_t col = lineentry.GetColumn();

  const char * dir = file.GetDirectory();
  const char * fname = file.GetFilename();
  nglPath p(dir);
  p += fname;

  printf("%s (%d : %d)\n", p.GetChars(), line, col);

  ShowSource(p, line, col);
}

void DebugView::ShowSource(const nglPath& rPath, int32 line, int32 col)
{
  auto it = mFiles.find(rPath.GetPathName());
  if (it == mFiles.end())
  {
    if (!rPath.Exists() || !rPath.IsLeaf())
      return;

    std::map<nglString, nglString> dico;
    dico["TABNAME"] = rPath.GetNodeName();
    dico["FILENAME"] = rPath.GetPathName();
    nuiWidget* pWidget = nuiBuilder::Get().CreateWidget("SourceScroller", dico);
    NGL_ASSERT(pWidget != NULL);

    nuiWidget* pHeader = nuiBuilder::Get().CreateWidget("FileTabHeader", dico);
    NGL_ASSERT(pHeader != NULL);
    int32 tab = mpFilesTabView->AddTab(pHeader, pWidget);

    SourceView* pView = (SourceView*)pWidget->SearchForChild("source", true);
    NGL_ASSERT(pView != NULL);

    mSlotSink.Connect(pView->LineSelected, nuiMakeDelegate(this, &DebugView::OnLineSelected));

    nuiButton* pCloser = dynamic_cast<nuiButton*>(pHeader->SearchForChild("CloseTab", true));
    NGL_ASSERT(pCloser != NULL);
    mEventSink.Connect(pCloser->Activated, &DebugView::OnCloseTab, pWidget);

    pView->Load(rPath);
    mpFilesTabView->SelectTab(tab);
    mpFilesTabView->UpdateLayout();
    pView->ShowText(line, col);

    mFiles[rPath.GetPathName()] = pWidget;
  }
  else
  {
    nuiWidget* pWidget = it->second;

    SourceView* pView = (SourceView*)pWidget->SearchForChild("source", true);
    NGL_ASSERT(pView != NULL);

    mpFilesTabView->SelectTabByContents(pWidget);
    mpFilesTabView->UpdateLayout();
    pView->ShowText(line, col);
  }

}

void DebugView::OnCloseTab(const nuiEvent& event)
{
  nuiWidget* pWidget = (nuiWidget*)event.mpUser;
  NGL_ASSERT(pWidget != NULL);
  int32 tab = mpFilesTabView->GetTabIndexByContents(pWidget);
  NGL_ASSERT(tab >= 0);
  mpFilesTabView->RemoveTab(tab);

  // Remove the tab from our own structure of open files:
  auto it = mFiles.begin();
  auto end = mFiles.end();
  while (it != end)
  {
    if (it->second == pWidget)
    {
      mFiles.erase(it);
      return;
    }
    ++it;
  }
}

void DebugView::UpdateVariablesForCurrentFrame()
{
  ProcessTree* pNode = (ProcessTree*)mpThreads->GetSelectedNode();
  if (!pNode)
  {
    return;
  }

  ProcessTree::Type type = pNode->GetType();
  switch (type)
  {
    case ProcessTree::eProcess:
      // Nothing to do for now
      SelectProcess(pNode->GetProcess());
      break;
    case ProcessTree::eThread:
      // Select the thread
      SelectThread(pNode->GetThread());
      break;
    case ProcessTree::eFrame:
      // Select the frame
      SelectFrame(pNode->GetFrame());
      break;
  }
}

void DebugView::OnThreadSelectionChanged(const nuiEvent& rEvent)
{
  UpdateVariablesForCurrentFrame();
}

void DebugView::OnModuleFileSelectionChanged(const nuiEvent& rEvent)
{
  ModuleTree* pNode = (ModuleTree*)mpModulesFiles->GetSelectedNode();
  if (!pNode)
    return;
  
  if (pNode->GetType() != ModuleTree::eCompileUnit)
    return;
  
  nglPath p = pNode->GetSourcePath();
  ShowSource(p, 0, 0);
}

void DebugView::OnModuleSymbolSelectionChanged(const nuiEvent& rEvent)
{
//  ModuleTree* pNode = (ModuleTree*)mpModulesSymbols->GetSelectedNode();
//  if (!pNode)
//    return;
//
//  if (pNode->GetType() != ModuleTree::eSymbol)
//    return;
//
//  nglPath p = pNode->GetSourcePath();
//  ShowSource(p, 0, 0);
}


void DebugView::OnLineSelected(const nglPath& rPath, float X, float Y, int32 line, bool ingutter)
{
  printf("OnLineSelected: %f,%f -- %d (in gutter: %s)\n", X, Y, line + 1, YESNO(ingutter));
  if (ingutter)
  {
    Breakpoint* pBP = GetDebuggerContext().GetBreakpointByLocation(rPath, line + 1, 0);
    if (pBP)
      GetDebuggerContext().DeleteBreakpoint(pBP);
    else
    {
      pBP = GetDebuggerContext().CreateBreakpointByLocation(rPath, line + 1, 0);
    }

    auto it = mFiles.find(rPath.GetPathName());
    if (it != mFiles.end())
    {
      nuiWidget* pWidget = it->second;
      SourceView* pView = (SourceView*)pWidget->SearchForChild("source", true);
      NGL_ASSERT(pView != NULL);
      pView->Invalidate();
    }
  }

}

void DebugView::OnVariableSelectionChanged(const nuiEvent& rEvent)
{
  VariableNode* pNode = dynamic_cast<VariableNode*>(mpVariables->GetSelectedNode());
  mpGraphView->DelAllSources();
  if (!pNode)
    return;

  SBValue val = pNode->GetValue();
  ValueArray* pVal = new ValueArray(val);
  mpGraphView->AddSource(pVal);
}

void DebugView::OnHandleSTDIO(const nuiEvent& event)
{
  DebuggerContext& rContext(GetDebuggerContext());
  lldb::SBProcess process(rContext.mProcess);
  char buffer[10000];
  memset(buffer, 0, sizeof(buffer));
  size_t outcount = 0;
  while ((outcount = process.GetSTDOUT(buffer, sizeof(buffer))))
  {
    mpOutput->AddText(nglString(buffer, outcount));
    printf("OUT> %s", buffer);
    memset(buffer, 0, sizeof(buffer));
  }

  while ((outcount = process.GetSTDERR(buffer, sizeof(buffer))))
  {
    mpErrors->AddText(nglString(buffer, outcount));
    printf("ERR> %s", buffer);
    memset(buffer, 0, sizeof(buffer));
  }
}


