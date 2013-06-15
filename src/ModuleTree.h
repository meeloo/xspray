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
    eCompileUnitFolder,
    eCompileUnit,
    eSymbolFolder,
    eSymbol
  };

  ModuleTree(const lldb::SBTarget& rTarget);
  ModuleTree(const lldb::SBModule& rModule, Type type);
  ModuleTree(const lldb::SBCompileUnit& rCompileUnit);
  ModuleTree(const lldb::SBSymbol& rSymbol);

  virtual ~ModuleTree();

  virtual void Open(bool Opened);
  bool IsEmpty() const;

  void Update();
  void UpdateTarget();
  void UpdateModule();
  void UpdateCompileUnit();
  void UpdateSymbol();
  void UpdateCompileUnitFolder();
  void UpdateSymbolFolder();

  Type GetType() const;

  const lldb::SBTarget& GetTarget() const;
  const lldb::SBModule& GetModule() const;
private:
  lldb::SBTarget mTarget;
  mutable lldb::SBModule mModule;
  lldb::SBCompileUnit mCompileUnit;
  lldb::SBSymbol mSymbol;
  Type mType;
};


