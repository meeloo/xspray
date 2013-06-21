//
//  VariableNode.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/6/13.
//
//

#include "Xspray.h"

namespace Xspray
{

VariableNode::VariableNode(lldb::SBValue value)
: nuiTreeNode(NULL),
  mValue(value)
{
  std::map<nglString, nglString> dico;

  dico["VariableName"] = value.GetName();
  dico["VariableType"] = value.GetTypeName();
  dico["VariableValue"] = value.GetValue();
  nuiWidget* pElement = nuiBuilder::Get().CreateWidget("VariableView", dico);
  SetElement(pElement);

  //NGL_OUT("%s (%s) = %s\n", value.GetName(), value.GetTypeName(), value.GetValue());
}

VariableNode::~VariableNode()
{
}

void VariableNode::Open(bool Opened)
{
  nuiTreeNode::Open(Opened);

  if (Opened)
  {
    uint32_t count = mValue.GetNumChildren();

    for (auto i = 0; i < count; i++)
    {
      lldb::SBValue child(mValue.GetChildAtIndex(i, lldb::eDynamicCanRunTarget, true));

      AddChild(new VariableNode(child));
    }
  }
  else
  {
    Clear();
  }
}

bool VariableNode::IsEmpty() const
{
  return !const_cast<lldb::SBValue&>(mValue).MightHaveChildren();
}

}
