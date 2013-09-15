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

  mpAppName = NULL;
  mpAppPath = NULL;
  mpAppCommandLine = NULL;
  mpAppEnvironment = NULL;
  mpAppArch = NULL;
  mpAppOS = NULL;
  mpAppVendor = NULL;

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

  nuiWidget* pLine = NULL;
  pLine = SearchForChild("AppName");
  mpAppName = (nuiLabel*)pLine->SearchForChild("Contents", true);

  pLine = SearchForChild("AppPath");
  mpAppPath = (nuiLabel*)pLine->SearchForChild("Contents", true);

  pLine = SearchForChild("AppCommandLine");
  mpAppCommandLine = (nuiEditLine*)pLine->SearchForChild("Contents", true);

  pLine = SearchForChild("AppEnvironment");
  mpAppEnvironment = (nuiEditLine*)pLine->SearchForChild("Contents", true);

  pLine = SearchForChild("AppArch");
  mpAppArch = (nuiLabel*)pLine->SearchForChild("Contents", true);

  pLine = SearchForChild("AppOS");
  mpAppOS = (nuiLabel*)pLine->SearchForChild("Contents", true);

  pLine = SearchForChild("AppVendor");
  mpAppVendor = (nuiLabel*)pLine->SearchForChild("Contents", true);

  mEventSink.Connect(mpLaunchApplication->Activated, &HomeView::OnLaunch);
  mEventSink.Connect(mpAddApplication->Activated, &HomeView::AddApplication);
  mEventSink.Connect(mpRemoveApplication->Activated, &HomeView::RemoveApplication);
  mEventSink.Connect(mpApplicationList->SelectionChanged, &HomeView::OnApplicationSelected);

  for (int i = 0; i < AppDescription::GetAppCount(); i++)
  {
    AddApplication(AppDescription::GetApp(i));
  }
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
  AddApplication(params.mFiles[0]);
}

void HomeView::AddApplication(const nglPath& rApplication)
{
  int index = AppDescription::AddApp(rApplication);
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
  pLine->SetToken(new nuiFreeToken<AppDescription*>(pApp));
  nuiLabel* pLabel = (nuiLabel*)pLine->SearchForChild("Title", true);
  nuiImage* pIcon = (nuiImage*)pLine->SearchForChild("Icon", true);
  pLabel->SetText(pApp->GetName());
  pIcon->SetTexture(pApp->GetIcon());
  mpApplicationList->AddChild(pLine);

  mpApplicationList->SelectItem(pLine);
  mpApplicationList->SelectionChanged();
}

void HomeView::OnApplicationSelected(const nuiEvent& rEVent)
{
  DebuggerContext& rContext(GetDebuggerContext());

  nuiWidget* pLine = mpApplicationList->GetSelected();
  if (!pLine)
  {
    mpIcon->SetTexture(NULL);
    mpAppName->SetText(nglString::Empty);
    mpAppPath->SetText(nglString::Empty);
    mpAppEnvironment->SetText(nglString::Empty);
    mpAppCommandLine->SetText(nglString::Empty);
    mpAppArch->SetText(nglString::Empty);
    mpAppOS->SetText(nglString::Empty);
    mpAppVendor->SetText(nglString::Empty);

    rContext.mpAppDescription = NULL;

    return;
  }

  AppDescription* pApp = NULL;
  nuiGetTokenValue<AppDescription*>(pLine->GetToken(), pApp);
  NGL_ASSERT(pApp != NULL);
  mpIcon->SetTexture(pApp->GetIcon());

  mpAppName->SetText(pApp->GetName());
  mpAppPath->SetText(pApp->GetLocalPath().GetPathName());
  mpAppCommandLine->SetText(nglString::Empty);
  mpAppEnvironment->SetText(nglString::Empty);
  mpAppArch->SetText(pApp->GetArchitecture());
  mpAppOS->SetText(pApp->GetTargetOS());
  mpAppVendor->SetText(pApp->GetVendor());

  rContext.mpAppDescription = pApp;
}

void HomeView::RemoveApplication(const nuiEvent& rEvent)
{
  NGL_OUT("Remove Application\n");
}
