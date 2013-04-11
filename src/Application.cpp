/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/


#include "nui.h"
#include "nuiInit.h"
#include "Application.h"
#include "MainWindow.h"

#include "nglConsole.h"

#define APPLICATION_TITLE _T("Noodlz")

using namespace lldb;

void MyLogOutputCallback(const char * str, void *baton)
{
  printf("LLDB Output> %s\n", str);
}


void TestMain1(const char* exe_file_path)
{
  // Create a debugger instance so we can create a target
  SBDebugger debugger (SBDebugger::Create());

  debugger.SetLoggingCallback(MyLogOutputCallback, NULL);

  if (debugger.IsValid())
  {
    // Create a target using the executable.
    SBTarget target (debugger.CreateTargetWithFileAndArch (exe_file_path, "i386"));
    if (target.IsValid())
    {
      SBProcess process = target.LaunchSimple (NULL, NULL, "~/");
      if (process.IsValid())
      {
        while (process.GetState() == lldb::eStateLaunching)
          nglThread::MsSleep(10);

        int nummods = target.GetNumModules();
        for (int i = 0; i < nummods; i++)
        {
          SBModule module = target.GetModuleAtIndex(i);
          if (module.IsValid())
          {
            int numsyms = module.GetNumSymbols();
            for (int j = 0; j < numsyms; j++)
            {
              SBSymbol symbol = module.GetSymbolAtIndex(j);
              if (symbol.IsValid())
              {
                printf("Symbol %d: %s\n", j, symbol.GetName());
              }
            }
          }
        }
      }

      // Find the executable module so we can do a lookup inside it
      SBFileSpec exe_file_spec (exe_file_path, true);
      SBModule module (target.FindModule (exe_file_spec));
    }
  }
}


void TestMain(const char* exe_file_path)
{
  DebuggerContext& rContext(GetDebuggerContext());
  // Create a debugger instance so we can create a target
  //rContext.mDebugger = (SBDebugger::Create());

  rContext.mDebugger.SetLoggingCallback(MyLogOutputCallback, NULL);

  if (rContext.mDebugger.IsValid())
  {
    // Create a target using the executable.
    rContext.mTarget = rContext.mDebugger.CreateTargetWithFileAndArch (exe_file_path, "i386");
    if (rContext.mTarget.IsValid())
    {
      rContext.mProcess = rContext.mTarget.LaunchSimple (NULL, NULL, "~/");
      if (rContext.mProcess.IsValid())
      {
        while (rContext.mProcess.GetState() == lldb::eStateLaunching)
          nglThread::MsSleep(10);

        /*
        int nummods = rContext.mTarget.GetNumModules();
        for (int i = 0; i < nummods; i++)
        {
          SBModule module = rContext.mTarget.GetModuleAtIndex(i);
          if (module.IsValid())
          {
            int numsyms = module.GetNumSymbols();
            for (int j = 0; j < numsyms; j++)
            {
              SBSymbol symbol = module.GetSymbolAtIndex(j);
              if (symbol.IsValid())
              {
                printf("Symbol %d: %s\n", j, symbol.GetName());
              }
            }
          }
        }
        */
      }

      // Find the executable module so we can do a lookup inside it
      SBFileSpec exe_file_spec (exe_file_path, true);
      SBModule module (rContext.mTarget.FindModule (exe_file_spec));
    }
  }
}


#if 0
#include <stdint.h>
#include <stdlib.h>


//----------------------------------------------------------------------
// This quick sample code shows how to create a debugger instance and
// create an "i386" executable target. Then we can lookup the executable
// module and resolve a file address into a section offset address,
// and find all symbol context objects (if any) for that address:
// compile unit, function, deepest block, line table entry and the
// symbol.
//
// To build the program, type (while in this directory):
//
//    $ make
//
// then (for example):
//
//    $ DYLD_FRAMEWORK_PATH=/Volumes/data/lldb/svn/ToT/build/Debug ./a.out executable_path file_address
//----------------------------------------------------------------------
class LLDBSentry
{
public:
  LLDBSentry() {
    // Initialize LLDB
    SBDebugger::Initialize();
  }
  ~LLDBSentry() {
    // Terminate LLDB
    SBDebugger::Terminate();
  }
};

int
main (int argc, char const *argv[])
{
  // Use a sentry object to properly initialize/terminate LLDB.
  LLDBSentry sentry;

  if (argc < 2)
    exit (1);

  // The first argument is the file path we want to look something up in
  const char *exe_file_path = argv[1];

  //TestMain(exe_file_path);
  return 0;
}

#else

NGL_APP_CREATE(Application);
#endif

Application::Application()
{
  mpMainWindow = NULL;
  lldb::SBDebugger::Initialize();
  mpDebuggerContext = new DebuggerContext();
}

Application::~Application()
{
  delete mpDebuggerContext;
  lldb::SBDebugger::Terminate();
}

void Application::OnExit (int Code)
{
  if (mpMainWindow)
    mpMainWindow->Release();
}

void Application::OnInit()
{
  uint Width = 0, Height = 0;
  bool HasSize = false;
  bool IsFullScreen = false;
  bool DebugObject = false;
  bool DebugInfo = false;
  bool ShowFPS = false;


  nuiRenderer Renderer = eOpenGL2;
//  nuiRenderer Renderer = eSoftware;
//  nuiRenderer Renderer = eDirect3D;

  // Accept NGL default options
  ParseDefaultArgs();

  GetLog().UseConsole(true);
  //GetLog().SetLevel(_T("fps"), 100);
  //GetLog().SetLevel(_T("all"), 100);

  // Manual
  if ( (GetArgCount() == 1) &&
       ((!GetArg(0).Compare(_T("-h"))) || (!GetArg(0).Compare(_T("--help")))) )
  {
    NGL_OUT(_T("no params\n"));
    Quit (0);
    return;
  }

  // Parse args
  int i = 0;
  while (i < GetArgCount())
  {
    nglString arg = GetArg(i);
    if ((!arg.Compare(_T("--size")) || !arg.Compare(_T("-s"))) && ((i+1) < GetArgCount()))
    {
      int w, h;

      std::string str(GetArg(i+1).GetStdString());
      sscanf(str.c_str(), "%dx%d", &w, &h);
      if (w > 0) Width  = w;
      if (h > 0) Height = h;
      HasSize = true;
      i++;
    }
    else if (!arg.Compare(_T("--showfps")) || !arg.Compare(_T("-fps"))) ShowFPS = true;
    else if (!arg.Compare(_T("--fullscreen")) || !arg.Compare(_T("-f"))) IsFullScreen = true;
    else if (!arg.Compare(_T("--debugobject")) || !arg.Compare(_T("-d"))) DebugObject = true;
    else if (!arg.Compare(_T("--debuginfo")) || !arg.Compare(_T("-i"))) DebugInfo = true;
    else if (!arg.Compare(_T("--renderer")) || !arg.Compare(_T("-r")))
    {
      arg = GetArg(i+1);
      if (!arg.Compare(_T("opengl"))) Renderer = eOpenGL;
      else if (!arg.Compare(_T("opengl2"))) Renderer = eOpenGL2;
      else if (!arg.Compare(_T("direct3d"))) Renderer = eDirect3D;
      else if (!arg.Compare(_T("software"))) Renderer = eSoftware;
      i++;
    }
    i++;
  }


  //TestMain(GetArg(0).GetChars());

  nuiMainWindow::SetRenderer(Renderer);

  if (!HasSize)
  {
    if (IsFullScreen)
    {
      nglVideoMode current_mode;

      Width = current_mode.GetWidth();
      Height = current_mode.GetHeight();
    }
    else
    {
#ifdef NUI_IPHONE
      Width = 320;
      Height = 480;
#else
      Width = 800;
      Height = 600;
#endif
    }
  }


  /* Create the nglWindow (and thus a GL context, don't even try to
   *   instantiate the gui (or nuiFont) before the nuiWin !)
   */
  nuiContextInfo ContextInfo(nuiContextInfo::StandardContext3D);
  nglWindowInfo Info;

  Info.Flags = IsFullScreen ? nglWindow::FullScreen : 0;
  Info.Width = Width;
  Info.Height = Height;
  Info.Pos = nglWindowInfo::ePosCenter;
  Info.Title = APPLICATION_TITLE;
  Info.XPos = 0;
  Info.YPos = 0;

  mpMainWindow = new MainWindow(ContextInfo,Info, ShowFPS);
  if ((!mpMainWindow) || (mpMainWindow->GetError()))
  {
    if (mpMainWindow)
      NGL_OUT(_T("Error: cannot create window (%s)\n"), mpMainWindow->GetErrorStr());
    Quit (1);
    return;
  }
  mpMainWindow->Acquire();
  mpMainWindow->Acquire();
  mpMainWindow->DBG_SetMouseOverInfo(DebugInfo);  mpMainWindow->DBG_SetMouseOverObject(DebugObject);
#ifdef NUI_IPHONE
  mpMainWindow->SetState(nglWindow::eMaximize);
#else
  mpMainWindow->SetState(nglWindow::eShow);
#endif

}


MainWindow* Application::GetMainWindow()
{
  return mpMainWindow;
}

DebuggerContext& Application::GetDebuggerContext()
{
  return *mpDebuggerContext;
}



Application* GetApp()
{
  return ((Application*)App);
}

MainWindow* GetMainWindow()
{
  return ((Application*)App)->GetMainWindow();
}

DebuggerContext& GetDebuggerContext()
{
  return ((Application*)App)->GetDebuggerContext();
}

DebuggerContext::DebuggerContext()
: mDebugger(SBDebugger::Create())
{

}
