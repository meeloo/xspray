//
//  NativeFileDialog.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/25/13.
//
//

#include "NativeFileDialog.h"

nglPath ChooseFileDialog(nglWindow* pWindow, const nglPath& rDefaultDirectory, const nglString& rDefaultName, std::vector<nglString>& types, ChooseFileMode mode)
{
  NSOpenPanel* openDlg = [NSOpenPanel openPanel];

  [openDlg setPrompt:@"Select"];

//  NSArray* imageTypes = [NSImage imageTypes];
//
//  [openDlg setAllowedFileTypes:imageTypes];

  [openDlg beginSheetModalForWindow:(NSWindow*)(pWindow->GetOSInfo()->mpNSWindow) completionHandler:^(NSInteger result)
  {
    NSArray* files = [openDlg URLs];
    for(NSString* filePath in files)
    {
      NSLog(@"%@",filePath);
      //do something with the file at filePath
    }
  }];

  return nglPath();
}