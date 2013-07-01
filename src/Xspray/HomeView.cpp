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
}

HomeView::~HomeView()
{

}

void HomeView::Built()
{
  mpLaunchApplication = (nuiButton*)SearchForChild("LaunchApplication");
  mpAddApplication = (nuiButton*)SearchForChild("AddApplication");
  mpRemoveApplication = (nuiButton*)SearchForChild("RemoveApplication");
  NGL_ASSERT(mpLaunchApplication);

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
  for (uint32 i = 0; i < params.mFiles.size(); i++)
  {
    NGL_OUT("File chosen: %s\n", params.mFiles[i].GetChars());
  }
}

void HomeView::RemoveApplication(const nuiEvent& rEvent)
{
  NGL_OUT("Remove Application\n");
}
