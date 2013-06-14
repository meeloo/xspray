/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#pragma once

#include "nuiMainWindow.h"
#include "ProcessTree.h"
#include "VariableNode.h"
#include "SourceView.h"

class MainWindow : public nuiMainWindow
{
public:
  MainWindow(const nglContextInfo& rContext, const nglWindowInfo& rInfo, bool ShowFPS = false, const nglContext* pShared = NULL);
  ~MainWindow();

  void OnCreation();
  void OnClose();

  void OnProcessPaused();
  void OnProcessRunning();
  void UpdateProcess();
protected:
  bool LoadCSS(const nglPath& rPath);

  nuiEventSink<MainWindow> mEventSink;

  void OnStart(const nuiEvent& rEvent);
  void OnPause(const nuiEvent& rEvent);
  void OnContinue(const nuiEvent& rEvent);
  void OnStepIn(const nuiEvent& rEvent);
  void OnStepOver(const nuiEvent& rEvent);
  void OnStepOut(const nuiEvent& rEvent);
  void OnThreadSelectionChanged(const nuiEvent& rEvent);
  void Loop();
  void UpdateVariablesForCurrentFrame();

  void ShowSource(const nglPath& rPath, int32 line, int32 col);

  nglThreadDelegate* mpDebuggerEventLoop;

  nuiTreeView* mpThreads;
  nuiTreeView* mpVariables;
  nuiWidget* mpTransport;
  nuiButton* mpStart;
  nuiButton* mpPause;
  nuiButton* mpContinue;
  nuiButton* mpStepIn;
  nuiButton* mpStepOver;
  nuiButton* mpStepOut;
  SourceView* mpSourceView;
  
  void SelectProcess(lldb::SBProcess process);
  void SelectThread(lldb::SBThread thread);
  void SelectFrame(lldb::SBFrame frame);
  void UpdateVariables(lldb::SBFrame frame);
};

