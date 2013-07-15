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
: mColor("black"), mWeight(5), mName("noname")
{
}


GraphView::GraphView()
: mZoom(1.0),
  mZoomY(1.0),
  mYOffset(0),
  mStart(0),
  mEnd(0),
  mAutoZoomY(true)
{
  std::vector<float> v;
  v.resize(1000);
  for (int i = 0; i < 1000; i++)
  {
    float x = i;
    x = x * M_PI / (100.0f);
    x = sinf(x) * 200.0f + (i / 3);
    v[i] = x;
  }

  MemoryArray* pArray = new MemoryArray(&v[0], v.size());
  AddSource(pArray);
}

GraphView::~GraphView()
{
}



void GraphView::AddSource(ArrayModel<float>* pModel, const GraphOptions& rOptions)
{
  NGL_ASSERT(mModels.find(pModel) == mModels.end());
  pModel->Acquire();
  mModels[pModel] = rOptions;

  if (mEnd == 0)
    mEnd = pModel->GetNumValues();
  Invalidate();
}


void GraphView::DelSource(ArrayModel<float>* pModel)
{
  auto it = mModels.find(pModel);
  NGL_ASSERT(it != mModels.end());
  pModel->Release();
  mModels.erase(it);
  Invalidate();
}

void GraphView::DelAllSources()
{
  for (auto it = mModels.begin(); it != mModels.end(); ++it)
    it->first->Release();

  mModels.clear();
}

void GraphView::SetSourceOptions(ArrayModel<float>* pModel, const GraphOptions& rOptions)
{
  auto it = mModels.find(pModel);
  NGL_ASSERT(it != mModels.end());
  it->second = rOptions;
}

const GraphOptions& GraphView::GetSourceOptions(ArrayModel<float>* pModel) const
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
  float height = mRect.GetHeight();
  auto it = mModels.begin();
  auto end = mModels.end();

  while (it != end)
  {
    ArrayModel<float>* pModel = it->first;
    auto options = it->second;

    int32 start = MIN(mStart, pModel->GetNumValues());
    int32 end = MIN(mEnd, mStart + pModel->GetNumValues());

    int32 len = end - start;

    if (len > 0)
    {
      nuiShape* pShape = new nuiShape();

      // Scan for minimum and maximum:
      if (mAutoZoomY)
      {
        float min = pModel->GetValue(start);
        float max = min;

        for (int32 i = 0; i < len; i++)
        {
          min = MIN(min, pModel->GetValue(start + i));
          max = MAX(max, pModel->GetValue(start + i));
        }

        if (min > 0)
          min *= 0.9;
        else
          min *= 1.1;

        if (max > 0)
          max *= 1.1;
        else
          max *= 0.9;

        float diff = max - min;
        mZoomY = height / diff;
        mYOffset = -min;
      }

      for (int32 i = 0; i < len; i++)
      {
        float y = pModel->GetValue(start + i) + mYOffset;
        pShape->LineTo(nuiPoint(i, height - y * mZoomY));
      }

      pContext->SetLineWidth(options.mWeight);
      pContext->SetStrokeColor(options.mColor);
      pContext->DrawShape(pShape, eStrokeShape);
      delete pShape;
    }

    ++it;
  }
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

void GraphView::SetAutoZoomY(bool set)
{
  mAutoZoomY;
  Invalidate();
}

bool GraphView::GetAutoZoomY() const
{

}

