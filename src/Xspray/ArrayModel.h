//
//  ArrayModel.h
//  Xspray
//
//  Created by Sébastien Métrot on 7/7/13.
//
//

#pragma once

template <class T>
class ArrayModel : public nuiRefCount
{
public:
  virtual int32 GetNumValues() const = 0;
  virtual void GetValues(std::vector<T>& rValues, int32 index, int32 length) const = 0;
  virtual T GetValue(int32 index) const = 0;

  virtual float GetMax(int32 start, int32 length) const
  {
    float v = std::numeric_limits<float>::min();
    for (int32 i = start; i < start + length; i++)
    {
      v = MAX(v, GetValue(i));
    }

    return v;
  }

  virtual float GetMax() const
  {
    return GetMax(0, GetNumValues());
  }

  virtual float GetMin(int32 start, int32 length) const
  {
    float v = std::numeric_limits<float>::max();
    for (int32 i = start; i < start + length; i++)
    {
      v = MIN(v, GetValue(i));
    }

    return v;
  }

  virtual float GetMin() const
  {
    return GetMin(0, GetNumValues());
  }

protected:
  ArrayModel() {}
  virtual ~ArrayModel() {}
};


class MemoryArray : public ArrayModel<float>
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

  MemoryArray(lldb::SBValue value);

protected:
  std::vector<float> mArray;
};


class ValueArray : public ArrayModel<float>
{
public:
  ValueArray(lldb::SBValue value);
  virtual ~ValueArray();

  virtual int32 GetNumValues() const;
  virtual void GetValues(std::vector<float>& rValues, int32 index, int32 length) const;
  virtual float GetValue(int32 index) const;


protected:
  mutable lldb::SBValue mValue;
  lldb::SBType mType;
  lldb::BasicType mBasicType;
  lldb::TypeClass mTypeClass;

};

