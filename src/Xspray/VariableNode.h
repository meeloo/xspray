//
//  VariableNode.h
//  Noodlz
//
//  Created by Sébastien Métrot on 6/6/13.
//
//

#pragma once

class VariableNode : public nuiTreeNode
{
public:
  VariableNode(lldb::SBValue value);
  virtual ~VariableNode();

  void Open(bool Opened);
  bool IsEmpty() const;

  lldb::SBValue GetValue() const;

private:
  lldb::SBValue mValue;
};