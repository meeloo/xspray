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


class SourceView : public nuiSimpleContainer
{
public:
  SourceView();
  virtual ~SourceView();

  bool Load(const nglPath& rPath);
  void ShowText(int line, int col);

  void AddLine(const nglString& rString);
  virtual nuiRect CalcIdealSize();
  virtual bool SetRect(const nuiRect& rRect);

  virtual bool Draw(nuiDrawContext* pContext);

  virtual bool Clear();
private:

  nuiRect GetSelectionRect();
  std::vector<nuiLabel*> mLines;
  int32 mLine;
  int32 mCol;
  CXIndex mIndex;
  CXTranslationUnit mTranslationUnit;
};