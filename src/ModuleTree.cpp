//
//  ModuleTree.cpp
//  Noodlz
//
//  Created by Sébastien Métrot on 6/15/13.
//
//

#include "ModuleTree.h"

ModuleTree::ModuleTree(const lldb::SBTarget& rTarget)
: nuiTreeNode(NULL, false, false, true, false), mTarget(rTarget), mType(eTarget)
{
  //SetTrace(true);
  //  NGL_OUT("ModuleTree process\n");
}

ModuleTree::ModuleTree(const lldb::SBModule& rModule)
: nuiTreeNode(NULL, false, false, true, false), mModule(rModule), mType(eModule)
{
  //SetTrace(true);
  //  NGL_OUT("ModuleTree process\n");
}

ModuleTree::~ModuleTree()
{

}

bool ModuleTree::IsEmpty() const
{
  if (mType == eElement)
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
    ModuleTree* pPT = new ModuleTree(mTarget.GetModuleAtIndex(i));
    AddChild(pPT);
    pPT->Open(true);
  }
}

void ModuleTree::UpdateModule()
{
 lldb::SBFileSpec f = mModule.GetFileSpec();
  nglString str;
  str.CFormat("Module %s", f.GetFilename());
  //  NGL_OUT("%s\n", str.GetChars());
  nglPath p(f.GetDirectory());
  p += nglString(f.GetFilename());
  nuiLabel* pLabel = new nuiLabel(str);
  pLabel->SetToolTip(p.GetChars());
  SetElement(pLabel);

//  int modules = mProcess.GetNumModules();
//  for (int i = 0; i < modules; i++)
//  {
//    ModuleTree* pPT = new ModuleTree(mProcess.GetModuleAtIndex(i));
//    AddChild(pPT);
//    pPT->Open(true);
//  }
}

ModuleTree::Type ModuleTree::GetType() const
{
  return mType;
}


