//
//  ProcessTree.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/3/13.
//
//

#include "Xspray.h"

using namespace Xspray;

ProcessTree::ProcessTree(const lldb::SBProcess& rProcess)
: nuiTreeNode(NULL, false, false, true, false), mProcess(rProcess), mType(eProcess)
{
  //SetTrace(true);
//  NGL_OUT("ProcessTree process\n");
}

ProcessTree::ProcessTree(const lldb::SBThread& rThread)
: nuiTreeNode(NULL, false, false, true, false), mThread(rThread), mType(eThread)
{
  //SetTrace(true);
//  NGL_OUT("ProcessTree thread\n");
}

ProcessTree::ProcessTree(const lldb::SBFrame& rFrame)
: nuiTreeNode(NULL, false, false, true, false), mFrame(rFrame), mType(eFrame)
{
  //SetTrace(true);
//  NGL_OUT("ProcessTree frame\n");
}

ProcessTree::~ProcessTree()
{

}

bool ProcessTree::IsEmpty() const
{
  if (mType == eFrame)
    return true;

  return false;
}

void ProcessTree::Open(bool Opened)
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

const lldb::SBProcess& ProcessTree::GetProcess() const
{
  return mProcess;
}

const lldb::SBThread& ProcessTree::GetThread() const
{
  return mThread;
}

const lldb::SBFrame& ProcessTree::GetFrame() const
{
  return mFrame;
}

void ProcessTree::Update()
{
  switch (mType)
  {
    case eProcess:
      UpdateProcess();
      break;
    case eThread:
      UpdateThread();
      break;
    case eFrame:
      UpdateFrame();
      break;
    default:
      NGL_ASSERT(0);
  }
}

void ProcessTree::UpdateProcess()
{
  lldb::SBTarget target = mProcess.GetTarget();
  lldb::SBFileSpec f = target.GetExecutable();
  nglString str;
  str.CFormat("Process %s (%d)", f.GetFilename(), mProcess.GetProcessID());
//  NGL_OUT("%s\n", str.GetChars());
  nglPath p(f.GetDirectory());
  p += nglString(f.GetFilename());
  nuiLabel* pLabel = new nuiLabel(str);
  pLabel->SetToolTip(p.GetChars());
  SetElement(pLabel);

  int threads = mProcess.GetNumThreads();
  for (int i = 0; i < threads; i++)
  {
    ProcessTree* pPT = new ProcessTree(mProcess.GetThreadAtIndex(i));
    AddChild(pPT);
    pPT->Open(true);
  }
}

void ProcessTree::UpdateThread()
{
  nglString str;
  str.CFormat("Thread 0x%x", mThread.GetThreadID ());
//  NGL_OUT("\t%s\n", str.GetChars());
  nuiLabel* pLabel = new nuiLabel(str);
  SetElement(pLabel);

  bool select = false;
  if (mThread.GetStopReason() != lldb::eStopReasonNone)
  {
    select = true;
  }

  int frames = mThread.GetNumFrames();
  for (int i = 0; i < frames; i++)
  {
    ProcessTree* pPT = new ProcessTree(mThread.GetFrameAtIndex(i));
    AddChild(pPT);
    pPT->Open(true);

    if (select)
    {
      pPT->Select(true);
      select = false;
    }
  }

}

void ProcessTree::UpdateFrame()
{
  nglString str;
  str.CFormat("%s", mFrame.GetFunctionName());
//  NGL_OUT("\t\t%s\n", str.GetChars());
  nuiLabel* pLabel = new nuiLabel(str);

  {
    lldb::SBSymbolContext context = mFrame.GetSymbolContext(1);
    lldb::SBLineEntry lineentry = mFrame.GetLineEntry();
    lldb::SBFileSpec file =  lineentry.GetFileSpec();

    const char * dir = file.GetDirectory();
    const char * fname = file.GetFilename();
    nglPath p(dir);
    p += fname;
    if (!p.Exists() || !p.IsLeaf())
      pLabel->SetEnabled(false);
  }

  SetElement(pLabel);
}

ProcessTree::Type ProcessTree::GetType() const
{
  return mType;
}


