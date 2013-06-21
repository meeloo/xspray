/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#pragma once

#include "nui.h"
#include "nuiApplication.h"

#include "Xspray.h"

class MainWindow;

class Application : public nuiApplication
{
public:
  Application();
  virtual ~Application();

  void OnInit();
  void OnExit (int Code);
  
  MainWindow* GetMainWindow();

  Xspray::DebuggerContext& GetDebuggerContext();
private:
  
  MainWindow* mpMainWindow;
  Xspray::DebuggerContext* mpDebuggerContext;
};


// a global call to retrieve the application object
Application* GetApp();
MainWindow* GetMainWindow();
Xspray::DebuggerContext& GetDebuggerContext();

