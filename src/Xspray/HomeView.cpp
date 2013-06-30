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

}
