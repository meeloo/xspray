//
//  SourceView.cpp
//  Noodlz
//
//  Created by Sébastien Métrot on 6/10/13.
//
//

#include "SourceView.h"

////////
SourceView::SourceView()
{
  if (SetObjectClass("SourceView"))
  {
    // Attributes
  }

  mLine = -1;
  mCol = -1;
}

SourceView::~SourceView()
{
}

bool SourceView::Load(const nglPath& rPath)
{
  nglIStream* pStream = rPath.OpenRead();
  if (!pStream)
    return false;

  int32 size = pStream->Available();
  nglString line;
  while (pStream->ReadLine(line))
  {
    AddLine(line);
    line.Nullify();
  }

  delete pStream;

  mIndex = clang_createIndex(0, 0);
  int argc = 0;
  char** argv = NULL;

  mTranslationUnit = clang_parseTranslationUnit(mIndex, rPath.GetChars(), argv, argc, 0, 0, CXTranslationUnit_None);

  for (unsigned I = 0, N = clang_getNumDiagnostics(mTranslationUnit); I != N; ++I)
  {
    CXDiagnostic Diag = clang_getDiagnostic(mTranslationUnit, I);
    CXString String = clang_formatDiagnostic(Diag, clang_defaultDiagnosticDisplayOptions());
    printf("%s\n", clang_getCString(String));
    clang_disposeString(String);
  }

//  typedef struct {
//    const void *ptr_data[2];
//    unsigned begin_int_data;
//    unsigned end_int_data;
//  } CXSourceRange;

  CXFile f = clang_getFile (mTranslationUnit, rPath.GetChars());
  CXSourceLocation ls = clang_getLocationForOffset(mTranslationUnit, f, 0);
  CXSourceLocation le = clang_getLocationForOffset(mTranslationUnit, f, size);
  CXSourceRange range = clang_getRange (ls, le);
  CXToken *Tokens = NULL;
  unsigned NumTokens = 0;
  clang_tokenize(mTranslationUnit, range, &Tokens, &NumTokens);

  printf("Found %d tokens\n", NumTokens);
  for (int i = 0; i < NumTokens; i++)
  {
    CXString str = clang_getTokenSpelling(mTranslationUnit, Tokens[i]);
    CXSourceLocation location = clang_getTokenLocation(mTranslationUnit, Tokens[i]);
    CXSourceRange range = clang_getTokenExtent(mTranslationUnit, Tokens[i]);
    CXSourceLocation start = clang_getRangeStart(range);
    CXSourceLocation end = clang_getRangeEnd(range);

    unsigned sline = 0, scolumn = 0, soffset = 0;
    clang_getFileLocation(start, NULL, &sline, &scolumn, &soffset);
    unsigned eline, ecolumn, eoffset;
    clang_getFileLocation(end, NULL, &eline, &ecolumn, &eoffset);

    printf("%d:%d -> %d:%d --> '%s'\n", sline, scolumn, eline, ecolumn, clang_getCString(str));

    clang_disposeString(str);
  }

  clang_disposeTranslationUnit(mTranslationUnit);
  clang_disposeIndex(mIndex);
  return true;
}

void SourceView::ShowText(int line, int col)
{
  if (line >= mLines.size())
  {
    mLine = -1;
    mCol = -1;
    return;
  }

  mLine = line - 1;
  mCol = col;
  SetHotRect(GetSelectionRect());
  Invalidate();
}

void SourceView::AddLine(const nglString& rString)
{
  if (!rString.IsEmpty())
  {
    nuiLabel* pLabel = new nuiLabel(rString);
    AddChild(pLabel);
    mLines.push_back(pLabel);
  }
  else
  {
    mLines.push_back(NULL);
  }

  InvalidateLayout();
}

nuiRect SourceView::CalcIdealSize()
{
  float y = 0;
  nuiRect global;
  for (int32 i = 0; i < mLines.size(); i++)
  {
    nuiLabel* pLabel = mLines[i];

    if (pLabel)
    {
      nuiRect ideal = pLabel->GetIdealRect();
      ideal.MoveTo(0, y);
      ideal.RoundToAbove();
      global.Union(global, ideal);
      y = ideal.Bottom();
    }
    else
    {
      y += 20;
      global.SetHeight(y);
    }
  }

  return global;
}

bool SourceView::SetRect(const nuiRect& rRect)
{
  nuiWidget::SetRect(rRect);
  float y = 0;
  for (int32 i = 0; i < mLines.size(); i++)
  {
    nuiLabel* pLabel = mLines[i];

    if (pLabel)
    {
      nuiRect ideal = pLabel->GetIdealRect();
      ideal.MoveTo(0, y);
      ideal.RoundToAbove();
      pLabel->SetLayout(ideal);
      y = ideal.Bottom();
    }
    else
    {
      y += 20;
    }
  }
  return true;
}

nuiRect SourceView::GetSelectionRect()
{
  if (mLine < 0)
    return nuiRect();

  nuiLabel* pLabel = mLines[mLine];
  nuiRect rect;

  if (pLabel)
  {
    rect = pLabel->GetRect();
  }
  else
  {
    if (mLine > 0)
    {
      nuiLabel* pLabel = mLines[mLine - 1];
      rect = pLabel->GetRect();
      rect.MoveTo(0, rect.Bottom());
    }
  }
  rect.SetWidth(mRect.GetWidth());

  return rect;
}

bool SourceView::Draw(nuiDrawContext* pContext)
{
  if (mLine >= 0)
  {
    nuiRect rect = GetSelectionRect();
    
    pContext->SetFillColor(nuiColor(200, 200, 250));
    pContext->DrawRect(rect, eFillShape);
  }

  DrawChildren(pContext);
  return true;
}


bool SourceView::Clear()
{
  mLines.clear();
  mLine = -1;
  mCol = -1;
  return nuiSimpleContainer::Clear();
}

