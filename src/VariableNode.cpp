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
