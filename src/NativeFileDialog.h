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

nglPath ChooseFileDialog(nglWindow* pWindow, const nglPath& rDefaultDirectory, const nglString& rDefaultName, std::vector<nglString>& types, ChooseFileMode mode);

