//
//  SourceView.h
//  Noodlz
//
//  Created by Sébastien Métrot on 6/10/13.
//
//

#pragma once

#include "nui.h"

#include <clang-c/Index.h>

class SourceLine : public nuiTextLayout
{
public:
  SourceLine(const nglString& rText, int offset, const nuiTextStyle& rStyle);
  virtual ~SourceLine();

  int GetOffset() const;

  const nglString& GetText() const;
  void Layout();

protected:
  nglString mText;
  int mOffset;
};

class SourceView : public nuiSimpleContainer
{
public:
  SourceView();
  virtual ~SourceView();

  bool Load(const nglPath& rPath);
  void ShowText(int line, int col);

  virtual nuiRect CalcIdealSize();
  virtual bool SetRect(const nuiRect& rRect);

  virtual bool Draw(nuiDrawContext* pContext);

  virtual bool Clear();

  virtual bool MouseClicked  (nuiSize X, nuiSize Y, nglMouseInfo::Flags Button);
  virtual bool MouseUnclicked(nuiSize X, nuiSize Y, nglMouseInfo::Flags Button);
  virtual bool MouseMoved    (nuiSize X, nuiSize Y);

  const nglPath& GetPath() const;

  nuiSignal5<const nglPath&, float, float, int32, bool> LineSelected;
private:
  nglPath mPath;
  nuiRect GetSelectionRect();
  std::vector<std::pair<nuiTextLayout*, SourceLine*> > mLines;
  int32 mLine;
  int32 mCol;
  CXIndex mIndex;
  CXTranslationUnit mTranslationUnit;
  nglString mText;
  nuiTextStyle mStyle;

  float mGutterWidth;
  float mGutterMargin;

  std::map<CXTokenKind, nuiTextStyle> mStyles;

  int32 mClicked = 0;

  static enum CXChildVisitResult CursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data);

};
