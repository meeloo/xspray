//
//  SymbolTree.h
//  Xspray
//
//  Created by Sébastien Métrot on 6/15/13.
//
//

#pragma once

class SymbolTree : public nuiTreeNode
{
public:
  enum Type
  {
    eTarget,
    eModule,
    eSymbol
  };

  SymbolTree(const lldb::SBTarget& rTarget);
  SymbolTree(const lldb::SBModule& rModule);
  SymbolTree(const lldb::SBSymbol& rSymbol);

  virtual ~SymbolTree();

  virtual void Open(bool Opened);
  bool IsEmpty() const;

  void Update();
  void UpdateTarget();
  void UpdateModule();
  void UpdateSymbol();

  Type GetType() const;

  nglPath GetSourcePath() const;

  const lldb::SBTarget& GetTarget() const;
  const lldb::SBModule& GetModule() const;
private:
  lldb::SBTarget mTarget;
  mutable lldb::SBModule mModule;
  lldb::SBSymbol mSymbol;
  Type mType;
};


