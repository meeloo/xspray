//
//  iOSRemoteDebug.h
//  Xspray
//
//  Created by Sébastien Métrot on 6/22/13.
//
//

#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <signal.h>
#include <getopt.h>
#include "MobileDevice.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct am_device * AMDeviceRef;

#ifdef __cplusplus
}
#endif

namespace Xspray
{

  // The sequence to start debugging on an iOS device:
  // 1 - Copy the bundle
  // 2 - Install the bundle
  // 3 - Start remote debugserver
  //

class iOSDevice
{
public:
  enum Type
  {
    Unknown,
    iPod,
    iPhone,
    iPad,
    AppleTV
  };

  const nglString& GetName() const;
  const nglString& GetUDID() const;
  const nglString& GetTypeName() const;
  Type GetType() const;
  const nglString& GetVersionString() const;

  bool InstallApplication(const nglPath& rBundlePath);
  bool StartDebugServer();

  static int32 GetDeviceCount();
  static iOSDevice* GetDevice(int32 index);

  static nuiSignal1<iOSDevice&> DeviceConnected;
  static nuiSignal1<iOSDevice&> DeviceDisconnected;

  nuiSignal2<float, const nglString&> InstallationProgress; // arg 1 = percent complete, arg 2 = status message
  nuiSignal1<bool> InstallationDone;

  nglString GetDebugSocketPath() const;
  service_conn_t GetDebugSocket() const;

  static void Init();
  static void Exit();
private:
  iOSDevice(am_device *device);
  virtual ~iOSDevice();

  am_device* mpDevice;

  nglString mName;
  nglString mUDID;
  nglString mTypeName;
  nglString mVersionString;

  Type mType;
  bool mRetina;

  static void DeviceCallback(struct am_device_notification_callback_info *info, void *arg);
  static void MountCallback(CFDictionaryRef dict, void* arg);
  static void TransferCallback(CFDictionaryRef dict, void* arg);
  static void InstallCallback(CFDictionaryRef dict, void* arg);
  static void FDVendorCallback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef address, const void *data, void *info);

  static std::map<am_device*, iOSDevice*> mDevices;
  static struct am_device_notification *mpNotifications;

  bool GetDeviceSupportPath(nglString& rPath) const;
  bool GetDeveloperDiskImagePath(nglString& rPath) const;
  bool MountDeveloperImage() const;
  void StartRemoteDebugServer();

  service_conn_t mDebuggerFD;
  nglString mDebugSocketPath;

  CFURLRef GetDeviceAppURL(CFStringRef identifier);
  CFStringRef GetDiskAppIdentifier(CFURLRef disk_app_url);

};

}

