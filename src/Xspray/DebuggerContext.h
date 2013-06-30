//
//  DebuggerContext.h
//  Xspray
//
//  Created by Sébastien Métrot on 6/21/13.
//
//

#pragma once

class DebuggerContext
{
public:
  DebuggerContext();

  lldb::SBDebugger mDebugger;
  lldb::SBTarget mTarget;
  lldb::SBProcess mProcess;
  nglPath mTargetApplication;
  std::list<Breakpoint*> mBreakpoints;
};

DebuggerContext& GetDebuggerContext();
