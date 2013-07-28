//
//  Breakpoint.h
//  Xspray
//
//  Created by Sébastien Métrot on 6/21/13.
//
//

#pragma once

class Breakpoint
{
public:
  enum Type
  {
    Location,
    Symbolic,
    Exception
  };

  lldb::SBBreakpoint GetBreakpoint() const;
  bool IsValid() const;
  Type GetType() const;
  const nglPath& GetPath() const;
  const nglString& GetSymbol() const;
  bool IsRegex() const;
  int32 GetLine() const;
  int32 GetColumn() const;
  bool GetBreakOnThrow() const;
  bool GetBreakOnCatch() const;

private:
  friend class DebuggerContext;

  Breakpoint(lldb::SBBreakpoint, const nglPath& rPath, int32 line, int32 column);
  Breakpoint(lldb::SBBreakpoint, const nglString& rSymbol, bool IsRegex);
  Breakpoint(lldb::SBBreakpoint, lldb::LanguageType language, bool BreakOnCatch, bool BreakOnThrow);

  Type mType;
  nglPath mPath;
  int32 mLine;
  int32 mColumn;
  nglString mSymbol;
  lldb::LanguageType mLanguage;
  bool mBreakOnCatch;
  bool mBreakOnThrow;
  bool mIsRegex;
  lldb::SBBreakpoint mBreakpoint;
};

