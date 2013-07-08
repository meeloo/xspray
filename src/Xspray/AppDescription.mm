//
//  AppDescription.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 7/1/13.
//
//

#include "Xspray.h"

using namespace Xspray;
using namespace lldb;

std::vector<AppDescription*> AppDescription::mApplications;

int AppDescription::AddApp(const nglPath& rPath)
{
  AppDescription* pApp = new AppDescription(rPath);
  mApplications.push_back(pApp);
  return mApplications.size() - 1;
}

int AppDescription::GetAppCount()
{
  return mApplications.size();
}

AppDescription* AppDescription::GetApp(int index)
{
  NGL_ASSERT(index < mApplications.size());
  return mApplications[index];
}

void AppDescription::RemoveApp(int index)
{
  NGL_ASSERT(index < mApplications.size());
  mApplications.erase(mApplications.begin() + index);
}

bool AppDescription::IsValid() const
{
  return !mArchitectures.empty() && mLocalPath.Exists();
}

AppDescription::AppDescription(const nglPath& rPath)
: mLocalPath(rPath), mpIcon(NULL)
{
  if (SetObjectClass("AppDescription"))
  {
    AddAttribute(new nuiAttribute<const nglString&>
                 (nglString(_T("AppName")), nuiUnitNone,
                  nuiMakeDelegate(this, &AppDescription::GetName)));
    
    AddAttribute(new nuiAttribute<const nglPath&>
                 (nglString(_T("LocalPath")), nuiUnitNone,
                  nuiMakeDelegate(this, &AppDescription::GetLocalPath)));

    AddAttribute(new nuiAttribute<const nglPath&>
                 (nglString(_T("RemotePath")), nuiUnitNone,
                  nuiMakeDelegate(this, &AppDescription::GetRemotePath)));

    AddAttribute(new nuiAttribute<const nglString&>
                 (nglString(_T("Architecture")), nuiUnitNone,
                  nuiMakeDelegate(this, &AppDescription::GetArchitecture)));
  }

  DebuggerContext& rContext(GetDebuggerContext());

  nglPath p = rPath.GetNodeName();
  mName = p.GetRemovedExtension();
  SBStringList archlist = rContext.mDebugger.GetAvailableArchsFromFile(rPath.GetChars());
  // first try to detect the target arch:
  std::vector<nglString> valid_archs;
  {
    size_t count = archlist.GetSize();

    for (size_t i = 0; i < count; i++)
      mArchitectures.push_back(archlist.GetStringAtIndex(i));
  }

  LoadBundleIcon(rPath);
}

AppDescription::~AppDescription()
{
  if (mpIcon)
    mpIcon->Release();
}

const std::vector<nglString>& AppDescription::GetArchitectures() const
{
  return mArchitectures;
}

nuiTexture* AppDescription::GetIcon() const
{
  return mpIcon;
}

const nglString& AppDescription::GetName() const
{
  return mName;
}

const nglPath& AppDescription::GetLocalPath() const
{
  return mLocalPath;
}

const nglPath& AppDescription::GetRemotePath() const
{
  return mRemotePath;
}

const nglString& AppDescription::GetArchitecture() const
{
  return mArchitecture;
}

const nglString& AppDescription::GetDevice()
{
  return mDevice;
}

const std::vector<nglString>& AppDescription::GetArguments() const
{
  return mArguments;
}

const std::map<nglString, nglString>& AppDescription::GetEnvironement() const
{
  return mEnvironement;
}

void AppDescription::DelArgument(int32 index)
{
  NGL_ASSERT(index < mArguments.size());
  mArguments.erase(mArguments.begin() + index);
  Changed();
}

void AppDescription::DelEnvironement(const nglString& rVar)
{
  auto it = mEnvironement.find(rVar);
  NGL_ASSERT(it != mEnvironement.end());
  mEnvironement.erase(it);
  Changed();
}

void AppDescription::SetArgument(int32 index, const nglString& rString)
{
  NGL_ASSERT(index < mArguments.size());
  mArguments[index] = rString;
  Changed();
}

void AppDescription::SetEnvironement(const nglString& rVar, const nglString& rString)
{
  mEnvironement[rVar] = rString;
  Changed();
}

void AppDescription::AddArgument(const nglString& rString)
{
  mArguments.push_back(rString);
  Changed();
}

void AppDescription::InsertArgument(int32 index, const nglString& rString)
{
  NGL_ASSERT(index < mArguments.size());
  mArguments.insert(mArguments.begin() + index, rString);
  Changed();
}


NSBitmapImageRep * GetBitmapImageRepresentation(NSImage* image)
{
#define SCALE (4.0 * nuiGetScaleFactor())
  int width = [image size].width * SCALE;
  int height = [image size].height * SCALE;

  if(width < 1 || height < 1)
    return nil;

  NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
                           initWithBitmapDataPlanes: NULL
                           pixelsWide: width
                           pixelsHigh: height
                           bitsPerSample: 8
                           samplesPerPixel: 4
                           hasAlpha: YES
                           isPlanar: NO
                           colorSpaceName: NSDeviceRGBColorSpace
                           bytesPerRow: width * 4
                           bitsPerPixel: 32];

  NSGraphicsContext *ctx = [NSGraphicsContext graphicsContextWithBitmapImageRep: rep];
  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext: ctx];
  NSAffineTransform* scale = [NSAffineTransform transform];
  [scale scaleBy: SCALE];
  [scale concat];
  [image drawAtPoint: NSZeroPoint fromRect: NSZeroRect operation: NSCompositeCopy fraction: 1.0];
  [ctx flushGraphics];
  [NSGraphicsContext restoreGraphicsState];

  return [rep autorelease];
}

bool ConvertNSImage(NSImage* image, nglImageInfo& rInfo)
{
  NSBitmapImageRep *bmp = GetBitmapImageRepresentation(image);
  NSInteger h = [bmp pixelsHigh], w = [bmp pixelsWide];
  unsigned char *data = [bmp bitmapData];

  rInfo.mBitDepth = [bmp bitsPerPixel];
  rInfo.mBufferFormat = eImageFormatRaw;
  rInfo.mBytesPerPixel = (rInfo.mBitDepth + 1) / 8; // compensate for 15 bit / pixels
  rInfo.mBytesPerLine = [bmp bytesPerRow];
  rInfo.mHeight = h;
  rInfo.mWidth = w;
  rInfo.mpBuffer = (char*)data;
  NGL_ASSERT(rInfo.mBitDepth == 32);
  if ([bmp bitmapFormat] & NSAlphaFirstBitmapFormat)
  {
    for (int y = 0; y < h; y++)
    {
      char* data = rInfo.mpBuffer + rInfo.mBytesPerLine * y;
      for (int x = 0; x < w; x++)
      {
        int o = 4 * x;
        data[o + 0] = data[o + 3];
        data[o + 1] = data[o + 2];
        data[o + 2] = data[o + 1];
        data[o + 3] = data[o + 0];
      }
    }
  }
  rInfo.mPixelFormat = nglImagePixelFormat::eImagePixelRGBA;

  return true;
}

bool AppDescription::LoadBundleIcon(const nglPath& rBundlePath)
{
  CFStringRef string = rBundlePath.GetPathName().ToCFString();

  NSImage* image = [[NSWorkspace sharedWorkspace] iconForFile:(NSString*)string];
  if (image)
  {
    nglImageInfo info;
    ConvertNSImage(image, info);
    mpIcon = nuiTexture::GetTexture(info);
    mpIcon->SetScale(nuiGetScaleFactor());

//    [image release];
  }
  //CFRelease(string);
  return mpIcon != NULL;
}

