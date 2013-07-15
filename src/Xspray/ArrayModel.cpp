//
//  ArrayModel.h
//  Xspray
//
//  Created by Sébastien Métrot on 7/7/13.
//
//

#include "Xspray.h"

using namespace Xspray;
using namespace lldb;


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


// class ValueArray : public ArrayModel


const char* GetBasicTypeName(BasicType tpe)
{
  switch (tpe)
  {
    case eBasicTypeInvalid: return "invalid";
    case eBasicTypeVoid: return "void";
    case eBasicTypeChar: return "char";
    case eBasicTypeSignedChar: return "signed char";
    case eBasicTypeUnsignedChar: return "unsigned char";
    case eBasicTypeWChar: return "wchar";
    case eBasicTypeSignedWChar: return "signed wchar";
    case eBasicTypeUnsignedWChar: return "unsigned wchar";
    case eBasicTypeChar16: return "char16";
    case eBasicTypeChar32: return "char32";
    case eBasicTypeShort: return "short";
    case eBasicTypeUnsignedShort: return "unsigned short";
    case eBasicTypeInt: return "int";
    case eBasicTypeUnsignedInt: return "unsigned int";
    case eBasicTypeLong: return "long";
    case eBasicTypeUnsignedLong: return "unsigned long";
    case eBasicTypeLongLong: return "long long";
    case eBasicTypeUnsignedLongLong: return "unsigned long long";
    case eBasicTypeInt128: return "int128";
    case eBasicTypeUnsignedInt128: return "unsigned int 128";
    case eBasicTypeBool: return "bool";
    case eBasicTypeHalf: return "half";
    case eBasicTypeFloat: return "float";
    case eBasicTypeDouble: return "double";
    case eBasicTypeLongDouble: return "long double";
    case eBasicTypeFloatComplex: return "float complex";
    case eBasicTypeDoubleComplex: return "double complex";
    case eBasicTypeLongDoubleComplex: return "long double complex";
    case eBasicTypeObjCID: return "ObjC id";
    case eBasicTypeObjCClass: return "ObjC class";
    case eBasicTypeObjCSel: return "ObjC selector";
    case eBasicTypeNullPtr: return "NullPtr";
    case eBasicTypeOther: return "other";
    default:
      return "WTF?";
  }
}

const char* GetTypeClassName(TypeClass cls);

SBType ResolveType(SBType type)
{
  TypeClass typeclass = type.GetTypeClass();
  if (typeclass == eTypeClassTypedef)
  {
    return ResolveType(type.GetCanonicalType());
  }

  return type;
}

void ShowTypeInfo(SBType _type, int level = 0)
{
  SBType type(ResolveType(_type));
  TypeClass typeclass = type.GetTypeClass();

  // Detection:
  nglString idnt;
  for (int i = 0; i < level; i++)
    idnt.Add("   ");
  const char* indent = idnt.GetChars();
  NGL_OUT("%s================= Type: %s\n", indent, type.GetName());
  //NGL_OUT("%sMightHaveChildren: %s\n", indent, YESNO(mValue.MightHaveChildren()));
  NGL_OUT("%sTypeClass: %s\n", indent, GetTypeClassName(typeclass));
  NGL_OUT("%s# Of Fields: %d\n", indent, type.GetNumberOfFields());
  NGL_OUT("%s# Of Template Args %d\n", indent, type.GetNumberOfTemplateArguments());
  NGL_OUT("%s# Direct base classes %d\n", indent, type.GetNumberOfDirectBaseClasses());
  NGL_OUT("%s# Direct virtual classes %d\n", indent, type.GetNumberOfVirtualBaseClasses());
  NGL_OUT("%sBasicType: %s\n", indent, GetBasicTypeName(type.GetBasicType()));
  NGL_OUT("%sIsPointer %s\n", indent, YESNO(type.IsPointerType()));
  NGL_OUT("%sIsReference %s\n", indent, YESNO(type.IsReferenceType()));
  NGL_OUT("%sIsFunction %s\n", indent, YESNO(type.IsFunctionType()));
  NGL_OUT("%sIsPolymorphic %s\n", indent, YESNO(type.IsPolymorphicClass()));

  if (type.IsPointerType())
  {
    ShowTypeInfo(type.GetPointeeType(), level + 2);
  }
  else if (type.IsReferenceType())
  {
    ShowTypeInfo(type.GetDereferencedType(), level + 2);
  }

}


ValueArray::ValueArray(SBValue value)
: mValue(value)
{
  ShowTypeInfo(mValue.GetType());
  NGL_OUT("# Of Children: %d\n", mValue.GetNumChildren());
  NGL_OUT("\n");
}

ValueArray::~ValueArray()
{

}

int32 ValueArray::GetNumValues() const
{
  return mValue.GetNumChildren();
}

void ValueArray::GetValues(std::vector<float>& rValues, int32 index, int32 length) const
{
  for (int i = 0; i < length; i++)
  {
    rValues.push_back(GetValue(i + index));
  }
}

float ValueArray::GetValue(int32 index) const
{
  SBValue val = mValue.GetChildAtIndex(index);
  SBData data = val.GetData();
  SBError error;
  return data.GetFloat(error, 0);
}

