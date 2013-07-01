//
//  NativeFileDialog.h
//  Xspray
//
//  Created by Sébastien Métrot on 6/25/13.
//
//

#pragma once

#include "nui.h"
#import <Cocoa/Cocoa.h>

enum ChooseFileMode
{
  eOpenFile,
  eSaveFile
};

class ChooseFileParams
{
public:
  ChooseFileParams();

  // Inputs:
  nglPath mPath;
  std::vector<nglString> mTypes;
  ChooseFileMode mMode;

  // Outputs:
  nuiFastDelegate1<const ChooseFileParams&> mCompletionDelegate;
  std::vector<nglString> mFiles;
  bool mCancelled;

};

void ChooseFileDialog(nglWindow* pWindow, const ChooseFileParams& rParams);

