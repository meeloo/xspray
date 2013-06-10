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

  nglString line;
  while (pStream->ReadLine(line))
  {
    AddLine(line);
    line.Nullify();
  }

  delete pStream;
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
      pLabel += 20;
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

