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

  Breakpoint(lldb::SBBreakpoint, const nglPath& rPath, int32 line, int32 column);
  Breakpoint(lldb::SBBreakpoint, const nglString& rSymbol);
  Breakpoint(lldb::SBBreakpoint, lldb::LanguageType language);

  lldb::SBBreakpoint GetBreakpoint() const;
  Type GetType() const;
  const nglPath& GetPath() const;
  int32 GetLine() const;
  int32 GetColumn() const;

private:
  Type mType;
  nglPath mPath;
  int32 mLine;
  int32 mColumn;
  nglString mSymbol;
  lldb::LanguageType mLanguage;
  lldb::SBBreakpoint mBreakpoint;
};

