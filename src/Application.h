/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#pragma once

#include "nui.h"
#include "nuiApplication.h"

#include <LLDB/LLDB.h>
#include <LLDB/SBStream.h>


class MainWindow;

class DebuggerContext
{
public:
  DebuggerContext();
  lldb::SBDebugger mDebugger;
  lldb::SBTarget mTarget;
  lldb::SBProcess mProcess;
};

class Application : public nuiApplication
{
public:
  Application();
  ~Application();

  void OnInit();
  void OnExit (int Code);
  
  MainWindow* GetMainWindow();

  DebuggerContext& GetDebuggerContext();
private:
  
  MainWindow* mpMainWindow;
  DebuggerContext* mpDebuggerContext;
};


// a global call to retrieve the application object
Application* GetApp();
MainWindow* GetMainWindow();
DebuggerContext& GetDebuggerContext();

