//
//  ModuleTree.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/15/13.
//
//

#include "Xspray.h"

using namespace Xspray;

ModuleTree::ModuleTree(const lldb::SBTarget& rTarget)
: nuiTreeNode(NULL, false, false, true, false), mTarget(rTarget), mType(eTarget)
{
  //SetTrace(true);
}

ModuleTree::ModuleTree(const lldb::SBModule& rModule, Type type)
: nuiTreeNode(NULL, false, false, true, false), mModule(rModule), mType(type)
{
  Update();
}

ModuleTree::ModuleTree(const lldb::SBCompileUnit& rCompileUnit)
: nuiTreeNode(NULL, false, false, false, false), mCompileUnit(rCompileUnit), mType(eCompileUnit)
{
  Update();
}

ModuleTree::~ModuleTree()
{

}

bool ModuleTree::IsEmpty() const
{
  if (mType == eCompileUnit)
    return true;
  return false;
}

void ModuleTree::Open(bool Opened)
{
  if (Opened && !mOpened)
  {
    Update();
  }
  else
  {
    Clear();
  }
  nuiTreeNode::Open(Opened);
}

const lldb::SBTarget& ModuleTree::GetTarget() const
{
  return mTarget;
}

const lldb::SBModule& ModuleTree::GetModule() const
{
  return mModule;
}

void ModuleTree::Update()
{
  switch (mType)
  {
    case eTarget:
      UpdateTarget();
      break;
    case eModule:
      UpdateModule();
      break;
    case eCompileUnit:
      UpdateCompileUnit();
      break;
    default:
      NGL_ASSERT(0);
  }
}

void ModuleTree::UpdateTarget()
{
  lldb::SBFileSpec f = mTarget.GetExecutable();
  nglString str;
  str.CFormat("Target %s", f.GetFilename());
  //  NGL_OUT("%s\n", str.GetChars());
  nglPath p(f.GetDirectory());
  p += nglString(f.GetFilename());
  nuiLabel* pLabel = new nuiLabel(str);
  pLabel->SetToolTip(p.GetChars());
  SetElement(pLabel);

  int modules = mTarget.GetNumModules();
  for (int i = 0; i < modules; i++)
  {
    ModuleTree* pPT = new ModuleTree(mTarget.GetModuleAtIndex(i), eModule);
    AddChild(pPT);
  }
}

void ModuleTree::UpdateModule()
{
 lldb::SBFileSpec f = mModule.GetFileSpec();
  nglString str(f.GetFilename());
  NGL_OUT("module: %s\n", str.GetChars());
  nglPath p(f.GetDirectory());
  p += nglString(f.GetFilename());
  nuiLabel* pLabel = new nuiLabel(str);
  pLabel->SetToolTip(p.GetChars());
  SetElement(pLabel);

  uint32_t compileunits = mModule.GetNumCompileUnits();
  for (uint32_t i = 0; i < compileunits; i++)
  {
    ModuleTree* pPT = new ModuleTree(mModule.GetCompileUnitAtIndex(i));
    AddChild(pPT);
  }
}

void ModuleTree::UpdateCompileUnit()
{
  lldb::SBFileSpec f = mCompileUnit.GetFileSpec();
  nglString str(f.GetFilename());
  NGL_OUT("  compile unit: %s\n", str.GetChars());
  nglPath p(f.GetDirectory());
  p += nglString(f.GetFilename());
  nuiLabel* pLabel = new nuiLabel(str);
  pLabel->SetToolTip(p.GetChars());
  SetElement(pLabel);

//  uint32_t count = mCompileUnit.GetNumLineEntries();

//  for (uint32 i = 0; i < count; i++)
//  {
//    lldb::SBLineEntry entry = mCompileUnit.GetLineEntryAtIndex(i);
//    int32 line = entry.GetLine();
//    int32 column = entry.GetColumn();
//    NGL_OUT("    entry %d:%d\n", line, column);
//  }

}

nglPath ModuleTree::GetSourcePath() const
{
  NGL_ASSERT(mType == eCompileUnit);
  lldb::SBFileSpec f = mCompileUnit.GetFileSpec();
  nglPath p(f.GetDirectory());
  p += f.GetFilename();

  return p;
}

ModuleTree::Type ModuleTree::GetType() const
{
  return mType;
}

