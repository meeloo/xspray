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

  void AddApplication(const nglPath& rApplication);

  nuiSignal1<const nglPath&> Launch;
private:
  nuiEventSink<HomeView> mEventSink;
  nuiSlotsSink mSlotSink;

  nuiButton* mpLaunchApplication;
  nuiButton* mpAddApplication;
  nuiButton* mpRemoveApplication;

  nuiList* mpApplicationList;
  nuiImage* mpIcon;

  nuiLabel* mpAppName;
  nuiLabel* mpAppPath;
  nuiEditLine* mpAppCommandLine;
  nuiEditLine* mpAppEnvironment;
  nuiLabel* mpAppArch;
  nuiLabel* mpAppOS;
  nuiLabel* mpAppVendor;

  void OnLaunch(const nuiEvent& rEvent);
  void AddApplication(const nuiEvent& rEvent);
  void RemoveApplication(const nuiEvent& rEvent);
  void OnApplicationChosen(const ChooseFileParams& params);
  void AddApplication(AppDescription* pApp);

  void OnApplicationSelected(const nuiEvent& rEVent);
};