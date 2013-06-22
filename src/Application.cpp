/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/


#include "Xspray/Xspray.h"

#include "Application.h"
#include "MainWindow.h"

#define APPLICATION_TITLE _T("Xspray")

using namespace lldb;

NGL_APP_CREATE(Application);

Application::Application()
{
  mpMainWindow = NULL;
  lldb::SBDebugger::Initialize();
  mpDebuggerContext = new Xspray::DebuggerContext();
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
      Width = 1600;
      Height = 1200;
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

  Xspray::iOSDevice::Init();
}


MainWindow* Application::GetMainWindow()
{
  return mpMainWindow;
}

Xspray::DebuggerContext& Application::GetDebuggerContext()
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

Xspray::DebuggerContext& GetDebuggerContext()
{
  return ((Application*)App)->GetDebuggerContext();
}

