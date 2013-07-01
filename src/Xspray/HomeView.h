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

  nuiSignal1<const nglPath&> Launch;
private:
  nuiEventSink<HomeView> mEventSink;
  nuiSlotsSink mSlotSink;

  nuiButton* mpLaunchApplication;
  nuiButton* mpAddApplication;
  nuiButton* mpRemoveApplication;

  void OnLaunch(const nuiEvent& rEvent);
  void AddApplication(const nuiEvent& rEvent);
  void RemoveApplication(const nuiEvent& rEvent);
  void OnApplicationChosen(const ChooseFileParams& params);

};