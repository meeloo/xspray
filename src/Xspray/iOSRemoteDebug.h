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

  bool GetDeviceSupportPath(nglString& rPath) const;
  bool GetDeveloperDiskImagePath(nglString& rPath) const;
  bool MountDeveloperImage() const;

  static nuiSignal1<iOSDevice&> DeviceConnected;
  static nuiSignal1<iOSDevice&> DeviceDisconnected;

  nuiSignal2<float, const nglString&> InstallationProgress; // arg 1 = percent complete, arg 2 = status message
  nuiSignal1<bool> InstallationDone;

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

  static std::map<am_device*, iOSDevice*> mDevices;
  static struct am_device_notification *mpNotifications;
};

}

