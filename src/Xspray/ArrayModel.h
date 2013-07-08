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

protected:
  ArrayModel() {}
  virtual ~ArrayModel() {}
};