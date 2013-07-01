//
//  AppDescription.h
//  Xspray
//
//  Created by Sébastien Métrot on 7/1/13.
//
//

#pragma once

class AppDescription
{
public:
  AppDescription();

protected:
  nglString mName;
  nglPath mLocalPath;
  nglPath mRemotePath;

  std::vector<nglString> mArchitectures;
  nglString mDevice;
  nglString mArchitecture;

  std::vector<nglString> mArguments;
  std::map<nglString, nglString> mEnvironement;

};