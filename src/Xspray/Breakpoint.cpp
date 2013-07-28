//
//  Breakpoint.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/21/13.
//
//

#include "Xspray.h"

using namespace Xspray;

Breakpoint::Breakpoint(lldb::SBBreakpoint breakpoint, const nglPath& rPath, int32 line, int32 column)
: mType(Location), mPath(rPath), mLine(line), mColumn(column), mBreakOnCatch(false), mBreakOnThrow(false), mIsRegex(false), mBreakpoint(breakpoint)
{
}

Breakpoint::Breakpoint(lldb::SBBreakpoint breakpoint, const nglString& rSymbol, bool IsRegex)
: mType(Symbolic), mLine(-1), mColumn(-1), mSymbol(rSymbol), mBreakOnCatch(false), mBreakOnThrow(false), mIsRegex(IsRegex), mBreakpoint(breakpoint)
{

}

Breakpoint::Breakpoint(lldb::SBBreakpoint breakpoint, lldb::LanguageType language, bool BreakOnCatch, bool BreakOnThrow)
: mType(Exception), mLine(-1), mColumn(-1), mLanguage(language), mBreakOnCatch(BreakOnCatch), mBreakOnThrow(BreakOnThrow), mIsRegex(false), mBreakpoint(breakpoint)
{
  
}

bool Breakpoint::IsValid() const
{
  return mBreakpoint.IsValid();
}

lldb::SBBreakpoint Breakpoint::GetBreakpoint() const
{
  return mBreakpoint;
}

Breakpoint::Type Breakpoint::GetType() const
{
  return mType;
}

const nglPath& Breakpoint::GetPath() const
{
  return mPath;
}

int32 Breakpoint::GetLine() const
{
  return mLine;
}

int32 Breakpoint::GetColumn() const
{
  return mColumn;
}

const nglString& Breakpoint::GetSymbol() const
{
  return mSymbol;
}

bool Breakpoint::IsRegex() const
{
  return mIsRegex;
}


bool Breakpoint::GetBreakOnThrow() const
{
  return mBreakOnThrow;
}

bool Breakpoint::GetBreakOnCatch() const
{
  return mBreakOnCatch;
}

