//
//  VariableNode.cpp
//  Noodlz
//
//  Created by Sébastien Métrot on 6/6/13.
//
//

#include "VariableNode.h"

VariableNode::VariableNode(lldb::SBValue value)
: nuiTreeNode(new nuiLabel(value.GetName())),
  mValue(value)
{
  mpType = new nuiLabel(value.GetTypeName());
  mpType->Acquire();
  mpValue = new nuiLabel("bleh");
  mpValue->Acquire();
}

VariableNode::~VariableNode()
{
  mpType->Release();
  mpValue->Release();
}

nuiWidgetPtr VariableNode::GetSubElement(uint32 index)
{
  if (index == 0)
    return mpType;
  else if (index == 1)
    return mpValue;
  return NULL;
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

