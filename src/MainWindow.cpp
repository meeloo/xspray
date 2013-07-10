
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
#include "NativeFileDialog.h"

using namespace lldb;
using namespace Xspray;

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

  NUI_ADD_WIDGET_CREATOR(SourceView, "Container");
  NUI_ADD_WIDGET_CREATOR(HomeView, "Container");
  NUI_ADD_WIDGET_CREATOR(DebugView, "Container");
  NUI_ADD_WIDGET_CREATOR(GraphView, "Container");

#ifdef _DEBUG_
  nglString t = "DEBUG";
#else
  nglString t = "RELEASE";
#endif
  nuiLabel* pLabel = new nuiLabel(t);
  pLabel->SetPosition(nuiBottom);
  AddChild(pLabel);

  LoadCSS(_T("rsrc:/css/main.css"));

  mpController = new nuiNavigationController();
  mpController->ShowNavigationBar(false);
  AddChild(mpController);

  nuiViewController* pView = new nuiViewController();
  HomeView* pHome = (HomeView*)nuiBuilder::Get().CreateWidget("Home");
  NGL_ASSERT(pHome);
  pView->AddChild(pHome);
  mpController->PushViewController(pView);

  mSlotSink.Connect(pHome->Launch, nuiMakeDelegate(this, &MainWindow::OnLaunch));
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

void MainWindow::OnLaunch(const nglPath& rPath)
{
  if (GetDebuggerContext().LoadApp())
  {
    nuiViewController* pView = new nuiViewController();
    DebugView* pDebugger = (DebugView*)nuiBuilder::Get().CreateWidget("Debugger");
    NGL_ASSERT(pDebugger);
    pView->AddChild(pDebugger);
    mpController->PushViewController(pView);
    mSlotSink.Connect(pDebugger->GoHome, nuiMakeDelegate(this, &MainWindow::OnGoHome));
  }
  else
  {
    NGL_OUT("ERROR LOADING APPLICATION\n");
  }
}

void MainWindow::OnGoHome()
{
  mpController->PopToRootViewControllerAnimated();
}
