//
//  HomeView.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/30/13.
//
//

#include "Xspray.h"

using namespace Xspray;

HomeView::HomeView()
: mEventSink(this), mSlotSink()
{
  if (SetObjectClass("HomeView"))
  {
    
  }
  
  mpLaunchApplication = NULL;
  mpAddApplication = NULL;
  mpRemoveApplication = NULL;
  mpApplicationList = NULL;
}

HomeView::~HomeView()
{

}

void HomeView::Built()
{
  mpLaunchApplication = (nuiButton*)SearchForChild("LaunchApplication");
  mpAddApplication = (nuiButton*)SearchForChild("AddApplication");
  mpRemoveApplication = (nuiButton*)SearchForChild("RemoveApplication");
  mpApplicationList = (nuiList*)SearchForChild("AppList");
  NGL_ASSERT(mpLaunchApplication);
  mpIcon = (nuiImage*)SearchForChild("Icon");

  mEventSink.Connect(mpLaunchApplication->Activated, &HomeView::OnLaunch);
  mEventSink.Connect(mpAddApplication->Activated, &HomeView::AddApplication);
  mEventSink.Connect(mpRemoveApplication->Activated, &HomeView::RemoveApplication);
}

void HomeView::OnLaunch(const nuiEvent& rEvent)
{
  nglPath path;
  Launch(path);
}

void HomeView::AddApplication(const nuiEvent& rEvent)
{
  NGL_OUT("Add Application\n");
  nglWindow* pWindow = ((nuiMainWindow*)GetTopLevel())->GetNGLWindow();
  NGL_ASSERT(pWindow);

  ChooseFileParams params;
  params.mPath = "/Users/meeloo/Documents";
  params.mCompletionDelegate = nuiMakeDelegate(this, &HomeView::OnApplicationChosen);
  ChooseFileDialog(pWindow, params);
}

void HomeView::OnApplicationChosen(const ChooseFileParams& params)
{
  if (params.mFiles.empty())
    return;

  int index = AppDescription::AddApp(params.mFiles[0]);
  if (index >= 0)
  {
    AppDescription* pApp = AppDescription::GetApp(index);
    if (pApp->IsValid())
    {
      AddApplication(pApp);
      return;
    }

    AppDescription::RemoveApp(index);
  }
}

void HomeView::AddApplication(AppDescription* pApp)
{
  nuiWidget* pLine = nuiBuilder::Get().CreateWidget("ListLine");
  nuiLabel* pLabel = (nuiLabel*)pLine->SearchForChild("Title", true);
  nuiImage* pIcon = (nuiImage*)pLine->SearchForChild("Icon", true);
  pLabel->SetText(pApp->GetName());
  pIcon->SetTexture(pApp->GetIcon());
  mpApplicationList->AddChild(pLine);

  mpIcon->SetTexture(pApp->GetIcon());
}

void HomeView::RemoveApplication(const nuiEvent& rEvent)
{
  NGL_OUT("Remove Application\n");
}
