//
//  ArrayModel.h
//  Xspray
//
//  Created by Sébastien Métrot on 7/7/13.
//
//

#include "Xspray.h"

using namespace Xspray;

float ArrayModel::GetMax(int32 start, int32 length) const
{
  float v = std::numeric_limits<float>::min();
  for (int32 i = start; i < start + length; i++)
  {
    v = MAX(v, GetValue(i));
  }

  return v;
}

float ArrayModel::GetMax() const
{
  return GetMax(0, GetNumValues());
}

float ArrayModel::GetMin(int32 start, int32 length) const
{
  float v = std::numeric_limits<float>::max();
  for (int32 i = start; i < start + length; i++)
  {
    v = MIN(v, GetValue(i));
  }

  return v;
}

float ArrayModel::GetMin() const
{
  return GetMin(0, GetNumValues());
}

/////////////////////////
MemoryArray::MemoryArray()
{
}

MemoryArray::~MemoryArray()
{
}

int32 MemoryArray::GetNumValues() const
{
  return mArray.size();
}

void MemoryArray::GetValues(std::vector<float>& rValues, int32 index, int32 length) const
{
  rValues.resize(length);
  for (int32 i = index; i < index + length; ++i)
  {
    rValues[i] = mArray.at(i);
  }
}

float MemoryArray::GetValue(int32 index) const
{
  return mArray.at(index);
}


MemoryArray::MemoryArray(const float* pData, int32 length)
{
  mArray.resize(length);
  for (int32 i = 0; i < length; ++i)
    mArray[i] = pData[i];
}

MemoryArray::MemoryArray(const double* pData, int32 length)
{
  mArray.resize(length);
  for (int32 i = 0; i < length; ++i)
    mArray[i] = (float)pData[i];
}

MemoryArray::MemoryArray(const int8* pData, int32 length)
{
  mArray.resize(length);
  for (int32 i = 0; i < length; ++i)
    mArray[i] = (float)pData[i];
}

MemoryArray::MemoryArray(const int16* pData, int32 length)
{
  mArray.resize(length);
  for (int32 i = 0; i < length; ++i)
    mArray[i] = (float)pData[i];
}

MemoryArray::MemoryArray(const int32* pData, int32 length)
{
  mArray.resize(length);
  for (int32 i = 0; i < length; ++i)
    mArray[i] = (float)pData[i];
}

MemoryArray::MemoryArray(const int64* pData, int32 length)
{
  mArray.resize(length);
  for (int32 i = 0; i < length; ++i)
    mArray[i] = (float)pData[i];
}

MemoryArray::MemoryArray(float* pData, int32 length)
{
  mArray.resize(length);
  for (int32 i = 0; i < length; ++i)
    mArray[i] = (float)pData[i];
}

MemoryArray::MemoryArray(lldb::SBValue value)
{
}


