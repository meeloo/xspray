//
//  VariableNode.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/6/13.
//
//

#include "Xspray.h"

using namespace Xspray;
using namespace lldb;

VariableNode::VariableNode(SBValue value)
: nuiTreeNode(NULL),
  mValue(value)
{
  std::map<nglString, nglString> dico;

  dico["VariableName"] = value.GetName();
  dico["VariableType"] = value.GetTypeName();
  dico["VariableValue"] = value.GetValue();
  nuiWidget* pElement = nuiBuilder::Get().CreateWidget("VariableView", dico);
  SetElement(pElement);

  //NGL_OUT("%s (%s) = %s\n", value.GetName(), value.GetTypeName(), value.GetValue());
}

VariableNode::~VariableNode()
{
}

void VariableNode::Open(bool Opened)
{
  nuiTreeNode::Open(Opened);

  if (Opened)
  {
    uint32_t count = mValue.GetNumChildren();

    for (auto i = 0; i < count; i++)
    {
      SBValue child(mValue.GetChildAtIndex(i, eDynamicCanRunTarget, true));

      AddChild(new VariableNode(child));

    }
  }
  else
  {
    Clear();
  }
}

bool VariableNode::IsEmpty() const
{
  return !const_cast<SBValue&>(mValue).MightHaveChildren();
}

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
  NGL_OUT("%sNumberOfFields: %d\n", indent, type.GetNumberOfFields());
  NGL_OUT("%sBasicType: %s\n", indent, GetBasicTypeName(type.GetBasicType()));
  NGL_OUT("%sIsPointer %s\n", indent, YESNO(type.IsPointerType()));
  if (type.IsPointerType())
  {
    ShowTypeInfo(type.GetPointeeType(), level + 2);
  }
  else if (type.IsReferenceType())
  {
    ShowTypeInfo(type.GetDereferencedType(), level + 2);
  }

}

void VariableNode::Select(bool DoSelect, bool Temporary)
{
  nuiTreeNode::Select(DoSelect, Temporary);

  if (DoSelect && !Temporary)
  {
    ShowTypeInfo(mValue.GetType());
    NGL_OUT("\n");
  }
}



