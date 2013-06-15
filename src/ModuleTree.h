//
//  ModuleTree.h
//  Noodlz
//
//  Created by Sébastien Métrot on 6/15/13.
//
//

#pragma once
#include "nui.h"
#include <LLDB/LLDB.h>

class ModuleTree : public nuiTreeNode
{
public:
  enum Type
  {
    eTarget,
    eModule,
    eSourceFile,
    eElement
  };

  ModuleTree(const lldb::SBTarget& rTarget);
  ModuleTree(const lldb::SBModule& rModule);
  virtual ~ModuleTree();

  virtual void Open(bool Opened);
  bool IsEmpty() const;

  void Update();
  void UpdateTarget();
  void UpdateModule();

  Type GetType() const;

  const lldb::SBTarget& GetTarget() const;
  const lldb::SBModule& GetModule() const;
private:
  lldb::SBTarget mTarget;
  lldb::SBModule mModule;
  Type mType;
};


