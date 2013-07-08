//
//  ArrayModel.h
//  Xspray
//
//  Created by Sébastien Métrot on 7/7/13.
//
//

#pragma once

class ArrayModel : public nuiRefCount
{
public:
  virtual int32 GetNumValues() const = 0;
  virtual void GetValues(std::vector<float>& rValues, int32 index, int32 length) const = 0;
  virtual float GetValue(int32 index) const = 0;

  virtual float GetMax(int32 start, int32 length) const;
  virtual float GetMax() const;
  virtual float GetMin(int32 start, int32 length) const;
  virtual float GetMin() const;
protected:
  ArrayModel() {}
  virtual ~ArrayModel() {}
};


class MemoryArray : public ArrayModel
{
public:
  MemoryArray();
  virtual ~MemoryArray();

  virtual int32 GetNumValues() const;
  virtual void GetValues(std::vector<float>& rValues, int32 index, int32 length) const;
  virtual float GetValue(int32 index) const;

  MemoryArray(const float* pData, int32 length);
  MemoryArray(const double* pData, int32 length);
  MemoryArray(const int8* pData, int32 length);
  MemoryArray(const int16* pData, int32 length);
  MemoryArray(const int32* pData, int32 length);
  MemoryArray(const int64* pData, int32 length);
  MemoryArray(float* pData, int32 length);

protected:
  std::vector<float> mArray;
};