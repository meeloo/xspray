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
  
}

HomeView::~HomeView()
{

}

void HomeView::Built()
{
  mpLaunchApplication = (nuiButton*)SearchForChild("LaunchApplication");
  NGL_ASSERT(mpLaunchApplication);

  mEventSink.Connect(mpLaunchApplication->Activated, &HomeView::OnLaunch);
}

void HomeView::OnLaunch(const nuiEvent& rEvent)
{
  nglPath path;
  Launch(path);
}
