//
//  SourceView.cpp
//  Noodlz
//
//  Created by Sébastien Métrot on 6/10/13.
//
//

#include "SourceView.h"

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

void SourceLine::SetPosition(float x, float y)
{
  mX = x;
  mY = y;
}

nuiRect SourceLine::GetDisplayRect() const
{
  nuiRect r(GetRect());
  r.MoveTo(mX, mY);
  return r;
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

  nuiTextStyle s(mStyle);

  mStyles[CXToken_Punctuation] = s;
  s.SetColor("marine");
  mStyles[CXToken_Keyword] = s;
  s.SetColor("blue");
  mStyles[CXToken_Identifier] = s;
  s.SetColor("red");
  mStyles[CXToken_Literal] = s;
  s.SetColor("green");
  mStyles[CXToken_Comment] = s;

  mLine = -1;
  mCol = -1;
}

SourceView::~SourceView()
{
}

bool SourceView::Load(const nglPath& rPath)
{
  mStyle.SetFont(nuiFont::GetFont(10));
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

    //printf("%d:%d -> %d:%d --> '%s'\n", sline, scolumn, eline, ecolumn, clang_getCString(str));

    clang_disposeString(str);
  }

  nglString line;
  int32 offset = 0;
  int currentline = 0;

  nuiTextStyle style = mStyle;

  while (pStream->ReadLine(line))
  {
    SourceLine* pLine = NULL;
    
    if (!line.IsEmpty())
    {
      pLine = new SourceLine(line, offset, mStyle);
      mLines.push_back(pLine);
    }
    else
    {
      mLines.push_back(NULL);
      pLine = NULL;
    }

    offset = pStream->GetPos();
  }

  // Look for the first token valid for the current line:
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

    style = mStyles[clang_getTokenKind(Token)];

    printf("%d:%d -> %d:%d --> '%s'\n", sline, scolumn, eline, ecolumn, clang_getCString(str));
    
    clang_disposeString(str);

    for (int l = sline - 1; l <= eline - 1; l++)
    {
      NGL_ASSERT(l < mLines.size());
      SourceLine* pLine = mLines[l];
      if (pLine)
      {
        int s = 0;
        int e = pLine->GetText().GetLength();

        if (sline == l)
          s = scolumn;

        pLine->AddStyleChange(s, style);

//        if (eline == l)
//        {
//          pLine->AddStyleChange(e, mStyle);
//        }
      }
    }
  }

  delete pStream;


  clang_disposeTranslationUnit(mTranslationUnit);
  clang_disposeIndex(mIndex);

  InvalidateLayout();
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
  float y = 0;
  nuiRect global;
  for (int32 i = 0; i < mLines.size(); i++)
  {
    SourceLine* pLine = mLines[i];

    if (pLine)
    {
      pLine->Layout();
      nuiRect ideal = pLine->GetRect();
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
    SourceLine* pLine = mLines[i];

    if (pLine)
    {
      nuiRect ideal = pLine->GetRect();
      pLine->SetPosition(0, y);
      y += ideal.Bottom();
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

  SourceLine* pLine = mLines[mLine];
  nuiRect rect;

  if (pLine)
  {
    rect = pLine->GetDisplayRect();
  }
  else
  {
    if (mLine > 0)
    {
      SourceLine* pLine = mLines[mLine - 1];
      rect = pLine->GetDisplayRect();
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

  for (int i = 0; i < mLines.size(); i++)
  {
    SourceLine* pLine = mLines[i];

    if (pLine)
    {
      nuiRect rect = pLine->GetDisplayRect();
      rect.Move(0, rect.GetHeight());
      pContext->DrawText(rect.Left(), rect.Top(), *pLine);
    }
  }

  return true;
}


bool SourceView::Clear()
{
  for (int i = 0; i < mLines.size(); i++)
    delete mLines[i];
  mLines.clear();
  mLine = -1;
  mCol = -1;
  return nuiSimpleContainer::Clear();
}

