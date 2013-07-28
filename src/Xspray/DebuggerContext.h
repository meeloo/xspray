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
  bool LoadApp();

  Breakpoint* CreateBreakpointByLocation(const nglPath& rPath, int32 line, int32 column);
  Breakpoint* CreateBreakpointByName(const nglString& rSymbol);
  Breakpoint* CreateBreakpointByRegex(const nglString& rRegEx);
  Breakpoint* CreateBreakpointForException(lldb::LanguageType language, bool BreakOnCatch, bool BreakOnThrow);

  void GetBreakpointsForFile(const nglPath& rPath, std::vector<Breakpoint*>& rBreakpoints);
  void GetBreakpointsLinesForFile(const nglPath& rPath, std::set<int32>& rLines);
  void GetBreakpointsForFiles(std::vector<Breakpoint*>& rBreakpoints);
  void GetBreakpointsForExceptions(std::vector<Breakpoint*>& rBreakpoints);
  void GetBreakpointsForSymbols(std::vector<Breakpoint*>& rBreakpoints);

  lldb::SBDebugger mDebugger;
  lldb::SBTarget mTarget;
  lldb::SBProcess mProcess;
  AppDescription* mpAppDescription;
  std::list<Breakpoint*> mBreakpoints;
};

DebuggerContext& GetDebuggerContext();
