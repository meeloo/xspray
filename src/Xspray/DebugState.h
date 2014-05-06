//
//  DebugState.h
//  Xspray
//
//  Created by Sébastien Métrot on 17/09/13.
//
//

#pragma once

class DebugType : public nuiObject
{
  DebugType();
  virtual ~DebugType();

  
};

class DebugVariable
{
public:
  DebugVariable();
  virtual ~DebugVariable();

  const nglString& GetName() const;

};


class DebugFunction : public nuiObject
{
public:
  DebugFunction();
  virtual ~DebugFunction();

  const std::vector<DebugVariable*>& GetVariables() const;
  const std::vector<DebugVariable*>& GetArguments() const;

private:
  std::vector<DebugFunction*> mArguments;
  std::vector<DebugFunction*> mVariables;
};


class DebugThread : public nuiObject
{
public:
  DebugThread();
  virtual ~DebugThread();

  const std::vector<DebugFunction*>& GetFunctions() const;

private:
  std::vector<DebugFunction*> mFunctions;
};


class DebugState : public nuiObject
{
public:
  DebugState();
  virtual ~DebugState();

  const std::vector<DebugThread*>& GetThreads() const;

private:
  std::vector<DebugThread*> mThreads;
};

