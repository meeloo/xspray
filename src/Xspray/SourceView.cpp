//
//  SourceView.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/10/13.
//
//

#include "Xspray.h"

using namespace Xspray;

//////// SourceLine
SourceLine::SourceLine(const nglString& rText, int offset, const nuiTextStyle& rStyle)
: nuiTextLayout(rStyle), mText(rText), mOffset(offset)
{
}

SourceLine::~SourceLine()
{
}

int SourceLine::GetOffset() const
{
  return mOffset;
}

const nglString& SourceLine::GetText() const
{
  return mText;
}

void SourceLine::Layout()
{
  nuiTextLayout::Layout(mText);
}

//////// SourceView
SourceView::SourceView()
{
  if (SetObjectClass("SourceView"))
  {
    // Attributes
  }

  mStyle.SetFont(nuiFont::GetFont(10));
//  mStyle.SetColor(nuiColor(0, 0, 0));
  nuiTextStyle s(mStyle);

  mStyles[CXToken_Punctuation] = s;
  s.SetColor(nuiColor(96, 0, 96));
  mStyles[CXToken_Keyword] = s;
  s.SetColor(nuiColor(0, 0, 192));
  mStyles[CXToken_Identifier] = s;
  s.SetColor(nuiColor(192, 0, 0));
  mStyles[CXToken_Literal] = s;
  s.SetColor(nuiColor(0, 128, 0));
  mStyles[CXToken_Comment] = s;

  mLine = -1;
  mCol = -1;
  mGutterWidth = 0;
  mGutterMargin = 8;
}

SourceView::~SourceView()
{
}

const char* GetTokenKindName(CXTokenKind kind)
{
  switch (kind)
  {
  case CXToken_Punctuation: return "Punctuation";
  case CXToken_Keyword: return "Keyword";
  case CXToken_Identifier: return "Identifier";
  case CXToken_Literal: return "Literal";
  case CXToken_Comment: return "Comment";
  }

  return "WTF Unknown Token Kind";
}

bool SourceView::Load(const nglPath& rPath)
{
  nglIStream* pStream = rPath.OpenRead();
  if (!pStream)
    return false;

  int32 size = pStream->Available();

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
  CXCursor* cursors = new CXCursor[NumTokens];
  clang_annotateTokens(mTranslationUnit, Tokens, NumTokens, cursors);

  //printf("Found %d tokens\n", NumTokens);
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

    //printf("%d:%d -> %d:%d --> '%s'\n", sline, scolumn, eline, ecolumn, clang_getCString(str));

    clang_disposeString(str);
  }

  nglString line;
  int32 offset = 0;
  int currentline = 0;

  nuiTextStyle style = mStyle;
  nuiTextStyle lnstyle = mStyle;
  lnstyle.SetColor(nuiColor(128, 128, 128));
  lnstyle.SetFont(nuiFont::GetFont(8));

  while (pStream->ReadLine(line))
  {
    SourceLine* pLine = NULL;
    nglString linestr;
    linestr.SetCInt(currentline + 1);
    nuiTextLayout* pLineNumber = new nuiTextLayout(lnstyle);
    pLineNumber->Layout(linestr);
    pLine = new SourceLine(line, offset, mStyle);
    mLines.push_back(std::make_pair(pLineNumber, pLine));
    offset = pStream->GetPos();

    currentline++;
  }

  for (int tokenindex = 0; tokenindex < NumTokens; tokenindex++)
  {
    CXToken Token = Tokens[tokenindex];
    CXString str = clang_getTokenSpelling(mTranslationUnit, Token);
    CXSourceLocation location = clang_getTokenLocation(mTranslationUnit, Token);
    CXSourceRange range = clang_getTokenExtent(mTranslationUnit, Token);
    CXSourceLocation start = clang_getRangeStart(range);
    CXSourceLocation end = clang_getRangeEnd(range);
    unsigned sline = 0, scolumn = 0, soffset = 0;
    unsigned eline, ecolumn, eoffset;

    clang_getFileLocation(start, NULL, &sline, &scolumn, &soffset);
    clang_getFileLocation(end, NULL, &eline, &ecolumn, &eoffset);
    sline--;
    scolumn--;
    eline--;
    ecolumn--;

    CXTokenKind tokenkind = clang_getTokenKind(Token);
    style = mStyles[tokenkind];

    //printf("%d:%d -> %d:%d --> '%s' (%s)\n", sline, scolumn, eline, ecolumn, clang_getCString(str), GetTokenKindName(tokenkind));

    clang_disposeString(str);

    for (int l = sline; l <= eline; l++)
    {
      NGL_ASSERT(l < mLines.size());
      auto line = mLines[l];
      SourceLine* pLine = line.second;
      if (pLine)
      {
        int s = 0;
        int e = pLine->GetText().GetLength();

        if (sline == l)
          s = scolumn;

        pLine->AddStyleChange(s, style);

        if (eline == l)
        {
          e = ecolumn;
          pLine->AddStyleChange(e, mStyle);
        }
      }
    }
  }

  delete pStream;

  delete[] cursors;

  clang_disposeTranslationUnit(mTranslationUnit);
  clang_disposeIndex(mIndex);

  InvalidateLayout();

  mPath = rPath;
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

nuiRect SourceView::CalcIdealSize()
{
  float h = mStyle.GetFont()->GetHeight();
  float y = h;
  nuiRect global;
  mGutterWidth = 20;
  for (int32 i = 0; i < mLines.size(); i++)
  {
    auto line = mLines[i];
    nuiTextLayout* pLineNumber = line.first;
    mGutterWidth = MAX(mGutterWidth, pLineNumber->GetRect().GetWidth());

    SourceLine* pLine = line.second;
    if (pLine)
    {
      pLine->Layout();
      nuiRect ideal = pLine->GetRect();
      ideal.MoveTo(0, y);
      ideal.RoundToAbove();
      global.Union(global, ideal);
    }
    else
    {
      global.SetHeight(y);
    }

    y = ToAbove(y + h);
  }

  mGutterWidth += mGutterMargin * 2;
  global.SetWidth(global.GetWidth() + mGutterWidth);
  global.SetHeight(ToAbove(h) * mLines.size());
  return global;
}

bool SourceView::SetRect(const nuiRect& rRect)
{
  return nuiWidget::SetRect(rRect);
}

nuiRect SourceView::GetSelectionRect()
{
  if (mLine < 0)
    return nuiRect();

  float h = mStyle.GetFont()->GetHeight();
  nuiFontInfo Info;
  mStyle.GetFont()->GetInfo(Info);
  nuiRect rect(mGutterWidth, (float)mLine * h - Info.Descender, GetRect().GetWidth() - mGutterWidth, h);
  return rect;
}

bool SourceView::Draw(nuiDrawContext* pContext)
{
  pContext->SetClearColor(nuiColor(255, 255, 255, 255));
  pContext->Clear();
  std::set<int32> breakpoints;
  GetDebuggerContext().GetBreakpointsLinesForFile(mPath, breakpoints);

  pContext->SetStrokeColor("grey");
  pContext->SetLineWidth(0.025f);
  pContext->EnableBlending(true);
  pContext->SetBlendFunc(nuiBlendTransp);

  if (mLine >= 0)
  {
    nuiRect rect = GetSelectionRect();
    rect.Left() = 0;
    
    pContext->SetFillColor(nuiColor(230, 230, 250));
    pContext->DrawRect(rect, eFillShape);
  }
  float x = mGutterWidth - mGutterMargin * 0.5;
  pContext->DrawLine(x, 0, x, mRect.GetHeight());

  float y = 0;
  float h = mStyle.GetFont()->GetHeight();
  for (int i = 0; i < mLines.size(); i++)
  {
    auto line = mLines[i];

    y = ToAbove((float)(i+1) * h);

    if (breakpoints.find(i+1) != breakpoints.end())
    {
//      pContext->SetFillColor("blue");
//      pContext->SetStrokeColor("black");
//      nuiRect r(0.0, y - h, mGutterMargin, h);
//      r.Grow(-1, -1);
//      pContext->DrawRect(r, eStrokeAndFillShape);

      nuiFont* pFontAwesome = nuiFont::GetFont("FontAwesome10");
      pContext->SetFont(pFontAwesome);
      pContext->SetTextColor("blue");
      nglString str("fontawesome_chevron_sign_right");
      if (i == mLine)
        str = "fontawesome_circle_arrow_right";
      pContext->DrawText(0, y, nuiObject::GetGlobalProperty(str));
      pContext->SetTextColor("black");
      pFontAwesome->Release();
    }

    nuiTextLayout* pLineNumber = line.first;
    nuiRect rln = pLineNumber->GetRect();
    pContext->DrawText(mGutterWidth - (rln.GetWidth() + mGutterMargin), y, *pLineNumber);

    SourceLine* pLine = line.second;

    if (pLine)
      pContext->DrawText(mGutterWidth, y, *pLine);
  }

  return true;
}


bool SourceView::Clear()
{
  for (int i = 0; i < mLines.size(); i++)
  {
    auto line = mLines[i];
    delete line.first;
    delete line.second;
  }
  mLines.clear();
  mLine = -1;
  mCol = -1;
  mPath = nglPath();
  return nuiSimpleContainer::Clear();
}

const nglPath& SourceView::GetPath() const
{
  return mPath;
}

bool SourceView::MouseClicked  (nuiSize X, nuiSize Y, nglMouseInfo::Flags Button)
{
  if (!(Button & nglMouseInfo::ButtonLeft || Button & nglMouseInfo::ButtonRight))
    return false;

  mClicked++;
  Grab();

  return true;
}

bool SourceView::MouseUnclicked(nuiSize X, nuiSize Y, nglMouseInfo::Flags Button)
{
  if (!(Button & nglMouseInfo::ButtonLeft || Button & nglMouseInfo::ButtonRight))
    return false;
  if (!mClicked)
    return false;
  Ungrab();

  mClicked++;
  if (IsInsideFromSelf(X, Y))
  {
    float h = mStyle.GetFont()->GetHeight();
    LineSelected(mPath, X, Y, Y / h, X < mGutterWidth);
  }
  return true;
}

bool SourceView::MouseMoved(nuiSize X, nuiSize Y)
{
  if (mClicked)
    return true;
  return false;
}


