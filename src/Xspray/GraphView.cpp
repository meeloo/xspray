//
//  GraphView.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 7/8/13.
//
//

#include "Xspray.h"

using namespace lldb;
using namespace Xspray;

GraphOptions::GraphOptions()
: mColor("black"), mWeight(1), mName("noname")
{
}


GraphView::GraphView()
: mZoom(1.0),
  mZoomY(1.0),
  mYOffset(0),
  mStart(0),
  mEnd(0)
{
}

GraphView::~GraphView()
{
}



void GraphView::AddSource(ArrayModel* pModel, const GraphOptions& rOptions)
{
  NGL_ASSERT(mModels.find(pModel) == mModels.end());
  pModel->Acquire();
  mModels[pModel] = rOptions;
  Invalidate();
}


void GraphView::DelSource(ArrayModel* pModel)
{
  auto it = mModels.find(pModel);
  NGL_ASSERT(it != mModels.end());
  pModel->Release();
  mModels.erase(it);
  Invalidate();
}

void GraphView::SetSourceOptions(ArrayModel* pModel, const GraphOptions& rOptions)
{
  auto it = mModels.find(pModel);
  NGL_ASSERT(it != mModels.end());
  it->second = rOptions;
}

const GraphOptions& GraphView::GetSourceOptions(ArrayModel* pModel) const
{
  auto it = mModels.find(pModel);
  NGL_ASSERT(it != mModels.end());
  return it->second;
}


nuiRect GraphView::CalcIdeadSize()
{
  return nuiRect(200, 100);
}

bool GraphView::Draw(nuiDrawContext* pContext)
{
  return nuiSimpleContainer::Draw(pContext);
}


void GraphView::SetZoom(float zoom)
{
  mZoom = zoom;
  Invalidate();
}

float GraphView::GetZoom() const
{
  return mZoom;
}

void GraphView::SetZoomY(float zoom)
{
  mZoomY = zoom;
  Invalidate();
}

float GraphView::GetZoomY() const
{
  return mZoomY;
}

void GraphView::SetRange(int32 start, int32 length)
{
  mStart = start;
  mEnd = start + length;
  Invalidate();
}

void GraphView::SetRangeStart(int32 start)
{
  int32 l = mEnd - mStart;
  SetRange(start, l);
}

void GraphView::SetRangeEnd(int32 end)
{
  int32 l = mEnd - mStart;
  SetRange(end - l, l);
}

void GraphView::SetRangeLength(int32 len)
{
  SetRange(mStart, len);
}

void GraphView::SetYOffset(float offset)
{
  mYOffset = offset;
}

float GraphView::GetYOffset() const
{
  return mYOffset;
}
