//
//  HomeView.h
//  Xspray
//
//  Created by Sébastien Métrot on 6/30/13.
//
//

#pragma once

class HomeView : public nuiLayout
{
public:
  HomeView();
  virtual ~HomeView();

  virtual void Built();

private:
  nuiEventSink<HomeView> mEventSink;
  nuiSlotsSink mSlotSink;

  
};