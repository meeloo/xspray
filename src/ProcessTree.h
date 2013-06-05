//
//  ProcessTree.h
//  Noodlz
//
//  Created by Sébastien Métrot on 6/3/13.
//
//

#pragma once
#include "nui.h"
#include <LLDB/LLDB.h>

class ProcessTree : public nuiTreeNode
{
public:
  enum Type
  {
    eProcess,
    eThread,
    eFrame
  };

  ProcessTree(const lldb::SBProcess& rProcess);
  ProcessTree(const lldb::SBThread& rThread);
  ProcessTree(const lldb::SBFrame& rFrame);
  virtual ~ProcessTree();

  virtual void Open(bool Opened);
  bool IsEmpty() const;

  const lldb::SBProcess& GetProcess() const;
  const lldb::SBThread& GetThread() const;
  const lldb::SBFrame& GetFrame() const;

  void Update();
  void UpdateProcess();
  void UpdateThread();
  void UpdateFrame();

  Type GetType() const;
private:
  Type mType;
  lldb::SBProcess mProcess;
  lldb::SBThread mThread;
  lldb::SBFrame mFrame;
};


