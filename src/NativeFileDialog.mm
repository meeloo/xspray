//
//  NativeFileDialog.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/25/13.
//
//

#include "NativeFileDialog.h"

ChooseFileParams::ChooseFileParams()
{
  //nglPath mPath;
  //std::vector<nglString>& mTypes;
  mMode = eOpenFile;
  mCancelled = true;
}


void ChooseFileDialog(nglWindow* pWindow, const ChooseFileParams& rParams)
{
  NSOpenPanel* openDlg = [NSOpenPanel openPanel];

  [openDlg setPrompt:@"Select"];

//  NSArray* imageTypes = [NSImage imageTypes];
//
//  [openDlg setAllowedFileTypes:imageTypes];

  __block ChooseFileParams saved_params(rParams);
  [openDlg beginSheetModalForWindow:(NSWindow*)(pWindow->GetOSInfo()->mpNSWindow) completionHandler:^(NSInteger result)
  {
    NSArray* files = [openDlg URLs];
    for (NSURL* url in files)
    {
      NSString* filePath = [url path];
      NSLog(@"%@", filePath);
      //do something with the file at filePath
      saved_params.mFiles.push_back(nglString([filePath UTF8String]));
    }

    saved_params.mCancelled = (result == NSFileHandlingPanelCancelButton);

    if (saved_params.mCompletionDelegate)
      saved_params.mCompletionDelegate(saved_params);
  }];
}