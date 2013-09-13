//
//  BreakpointsView.h
//  Xspray
//
//  Created by Sébastien Métrot on 30/07/13.
//
//

#pragma once

class BreakpointsView : public nuiSimpleContainer
{
public:
  BreakpointsView();
  virtual ~BreakpointsView();

private:
  nuiList* mpBreakpointList;
  nuiToggleButton* mpCatchObjC;
  nuiToggleButton* mpThrowObjC;
  nuiToggleButton* mpCatchObjCpp;
  nuiToggleButton* mpThrowObjCpp;
};