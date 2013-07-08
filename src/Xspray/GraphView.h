//
//  GraphView.h
//  Xspray
//
//  Created by Sébastien Métrot on 7/8/13.
//
//

#pragma once

class GraphOptions
{
public:
  GraphOptions();

  nuiColor mColor;
  float mWeight;
  nglString mName;
};

class GraphView : public nuiSimpleContainer
{
public:
  GraphView();
  virtual ~GraphView();

  void AddSource(ArrayModel* pModel, const GraphOptions& rOptions = GraphOptions());
  void DelSource(ArrayModel* pModel);
  void SetSourceOptions(ArrayModel* pModel, const GraphOptions& rOptions);
  const GraphOptions&  GetSourceOptions(ArrayModel* pModel) const;

  nuiRect CalcIdeadSize();
  bool Draw(nuiDrawContext* pContext);

  void SetZoom(float zoom);
  float GetZoom() const;
  void SetZoomY(float zoom);
  float GetZoomY() const;
  void SetRange(int32 start, int32 length);
  void SetRangeStart(int32 start);
  void SetRangeEnd(int32 end);
  void SetRangeLength(int32 length);

  void SetYOffset(float offset);
  float GetYOffset() const;
protected:
  std::map<ArrayModel*, GraphOptions> mModels;
  float mZoom;
  float mZoomY;
  float mYOffset;
  int32 mStart;
  int32 mEnd;
  
};