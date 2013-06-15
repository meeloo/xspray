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
}

ModuleTree::ModuleTree(const lldb::SBModule& rModule, Type type)
: nuiTreeNode(NULL, false, false, true, false), mModule(rModule), mType(type)
{
  //SetTrace(true);
  nuiLabel* pLabel = NULL;
  switch (mType)
  {
    case eCompileUnitFolder:
      pLabel = new nuiLabel("Files");
      break;
    case eSymbolFolder:
      pLabel = new nuiLabel("Symbols");
      break;
    default:
      break;
  }
  if (pLabel)
    SetElement(pLabel);
}

ModuleTree::ModuleTree(const lldb::SBCompileUnit& rCompileUnit)
: nuiTreeNode(NULL, false, false, false, false), mCompileUnit(rCompileUnit), mType(eCompileUnit)
{
  Update();
}

ModuleTree::ModuleTree(const lldb::SBSymbol& rSymbol)
: nuiTreeNode(NULL, false, false, false, false), mSymbol(rSymbol), mType(eSymbol)
{
  Update();
}

ModuleTree::~ModuleTree()
{

}

bool ModuleTree::IsEmpty() const
{
  if (mType == eSymbol || mType == eCompileUnit)
    return true;
  if (mType == eCompileUnitFolder)
    return mModule.GetNumCompileUnits() == 0;
  if (mType == eSymbolFolder)
    return mModule.GetNumSymbols() == 0;

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
    case eSymbol:
      UpdateSymbol();
      break;
    case eCompileUnitFolder:
      UpdateCompileUnitFolder();
      break;
    case eSymbolFolder:
      UpdateSymbolFolder();
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
    pPT->Open(true);
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


  if (mModule.GetNumCompileUnits() != 0)
  {
    ModuleTree* pFiles = new ModuleTree(mModule, eCompileUnitFolder);
    AddChild(pFiles);
  }

  if (mModule.GetNumSymbols() != 0)
  {
    ModuleTree* pSymbols = new ModuleTree(mModule, eSymbolFolder);
    AddChild(pSymbols);
  }
}

void ModuleTree::UpdateCompileUnitFolder()
{
  uint32_t compileunits = mModule.GetNumCompileUnits();
  for (uint32_t i = 0; i < compileunits; i++)
  {
    ModuleTree* pPT = new ModuleTree(mModule.GetCompileUnitAtIndex(i));
    AddChild(pPT);
  }
}

void ModuleTree::UpdateSymbolFolder()
{
  uint32_t symbols = mModule.GetNumSymbols();
  for (uint32_t i = 0; i < symbols; i++)
  {
    ModuleTree* pPT = new ModuleTree(mModule.GetSymbolAtIndex(i));
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
}

void ModuleTree::UpdateSymbol()
{
  nglString str(mSymbol.GetName());
  //NGL_OUT("  symbol: %s\n", str.GetChars());
  nuiLabel* pLabel = new nuiLabel(str);
  SetElement(pLabel);
}

ModuleTree::Type ModuleTree::GetType() const
{
  return mType;
}


