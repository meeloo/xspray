//
//  AppDescription.h
//  Xspray
//
//  Created by Sébastien Métrot on 7/1/13.
//
//

#pragma once

class AppDescription : public nuiObject
{
public:
  static int AddApp(const nglPath& rPath);
  static int GetAppCount();
  static AppDescription* GetApp(int index);
  static void RemoveApp(int index);

  bool IsValid() const;

  const nglString& GetName() const;
  const nglPath& GetLocalPath() const;
  const nglPath& GetRemotePath() const;

  const std::vector<nglString>& GetArchitectures() const;
  const nglString& GetArchitecture() const;
  nuiTexture* GetIcon() const;
  const nglString& GetDevice();

  const std::vector<nglString>& GetArguments() const;
  const std::map<nglString, nglString>& GetEnvironement() const;

  void DelArgument(uint32 index);
  void DelEnvironement(const nglString& rVar);
  void SetArgument(uint32 index, const nglString& rString);
  void SetEnvironement(const nglString& rVar, const nglString& rString);
  void AddArgument(const nglString& rString);
  void InsertArgument(uint32 index, const nglString& rString);

  nuiSimpleEventSource<0> Changed;

protected:
  AppDescription(const nglPath& rPath);
  virtual ~AppDescription();

  nglString mName;
  nglPath mLocalPath;
  nglPath mRemotePath;

  std::vector<nglString> mArchitectures;
  nglString mDevice;
  nglString mArchitecture;

  std::vector<nglString> mArguments;
  std::map<nglString, nglString> mEnvironement;

  nuiTexture* mpIcon;

  bool LoadBundleIcon(const nglPath& rBundlePath);

  static std::vector<AppDescription*> mApplications;

  uint32 GetArchitecturesRange(uint32 dimension) const;
  const nglString& GetArchitectureByIndex(uint32 index) const;

  uint32 GetArgumentRange(uint32 dimension) const;
  const nglString& GetArgumentByIndex(uint32 index) const;

};

