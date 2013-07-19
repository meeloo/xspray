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

  float mDisplayMin;
  float mDisplayMax;
};

class GraphView : public nuiSimpleContainer
{
public:
  GraphView();
  virtual ~GraphView();

  void DelAllSources();
  void AddSource(ArrayModel<float>* pModel, const GraphOptions& rOptions = GraphOptions());
  void DelSource(ArrayModel<float>* pModel);
  void SetSourceOptions(ArrayModel<float>* pModel, const GraphOptions& rOptions);
  const GraphOptions&  GetSourceOptions(ArrayModel<float>* pModel) const;

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
  int32 GetRangeStart() const;
  int32 GetRangeEnd() const;
  int32 GetRangeLength() const;
  void SetAutoZoomY(bool set);
  bool GetAutoZoomY() const;

  void SetYOffset(float offset);
  float GetYOffset() const;

  bool MouseClicked(const nglMouseInfo& rInfo);
  bool MouseUnclicked(const nglMouseInfo& rInfo);
  bool MouseMoved(const nglMouseInfo& rInfo);

protected:
  std::map<ArrayModel<float>*, GraphOptions> mModels;
  float mZoom;
  float mZoomY;
  float mYOffset;
  int32 mStart;
  int32 mEnd;
  bool mAutoZoomY;
  
};