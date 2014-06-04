//
//  iOSRemoteDebug.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/22/13.
//
//


//TODO: don't copy/mount DeveloperDiskImage.dmg if it's already done - Xcode checks this somehow

#include "iOSRemoteDebug.h"

#ifdef __cplusplus
extern "C" {
#endif

  /*
   Remote debug connect url schemes:
   listen://HOST:PORT
   unix-accept://SOCKNAME
   connect:// (same as tcp-connect://)
   tcp-connect://
   udp://
   fd://     (Just passing a native file descriptor within this current process that is already opened (possibly from a service or other source).
   file:///PATH

   Find the code handling these schemes in lldb://source/Core/ConnectionFileDescriptor.cpp
   */

  typedef void (*AMinstall_callback)(CFDictionaryRef dict, void* arg);
  typedef void (*AMtransfer_callback)(CFDictionaryRef dict, void* arg);
  mach_error_t AMDeviceInstallApplication(service_conn_t sockInstallProxy, CFStringRef cfStrPath, CFDictionaryRef opts, AMinstall_callback callback, void* unknow);
  mach_error_t AMDeviceTransferApplication(service_conn_t afcFd, CFStringRef path, CFDictionaryRef opts, AMtransfer_callback, void* unknown);


  int AMDeviceSecureTransferPath(int zero, AMDeviceRef device, CFURLRef url, CFDictionaryRef options, void *callback, void* cbarg);
  int AMDeviceSecureInstallApplication(int zero, AMDeviceRef device, CFURLRef url, CFDictionaryRef options, void *callback, void* cbarg);
  int AMDeviceMountImage(AMDeviceRef device, CFStringRef image, CFDictionaryRef options, void *callback, void* cbarg);
  int AMDeviceLookupApplications(AMDeviceRef device, int zero, CFDictionaryRef* result);
  
  
#ifdef __cplusplus
}
#endif

using namespace Xspray;

void DisplayCFValue(const char* domain, CFTypeRef v)
{
  if (v)
  {
    CFTypeID t = CFGetTypeID(v);
    if (t == CFStringGetTypeID())
    {
      NGL_OUT("Domain [%s] -> %s\n", domain, CFStringGetCStringPtr((CFStringRef)v, kCFStringEncodingUTF8));
    }
    else if (t == CFArrayGetTypeID())
    {
      NGL_OUT("Domain [%s] -> Array\n", domain);
    }
    else if (t == CFDataGetTypeID())
    {
      NGL_OUT("Domain [%s] -> Data\n", domain);
    }
    else if (t == CFDateGetTypeID())
    {
      NGL_OUT("Domain [%s] -> Date\n", domain);
    }
    else if (t == CFDictionaryGetTypeID())
    {
      NGL_OUT("Domain [%s] -> Dictionary\n", domain);
    }
    else if (t == CFBooleanGetTypeID())
    {
      NGL_OUT("Domain [%s] -> Boolean\n", domain);
    }
    else if (t == CFNumberGetTypeID())
    {
      NGL_OUT("Domain [%s] -> Number\n", domain);
    }
    else
    {
      CFStringRef td = CFCopyTypeIDDescription(t);
      const char* ts = CFStringGetCStringPtr(td, kCFStringEncodingUTF8);
      NGL_OUT("Domain [%s] -> %s\n", domain, ts);
      CFShow(v);
    }

  }
}

iOSDevice::iOSDevice(am_device *device)
: mpDevice(device), mType(Unknown), mRetina(false), mDebuggerFD(0)
{
	CFStringEncoding encoding = kCFStringEncodingUTF8;
	const char *udid          = CFStringGetCStringPtr(AMDeviceCopyDeviceIdentifier(mpDevice), encoding);

  CFRetain(mpDevice);

  AMDeviceConnect(mpDevice);
  AMDeviceIsPaired(mpDevice);
  AMDeviceValidatePairing(mpDevice);
  AMDeviceStartSession(mpDevice);

  const char *device_name  = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("DeviceName")),     encoding);
  const char *product_type = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductType")),    encoding);
  const char *ios_version  = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductVersion")), encoding);

  mName = device_name;
  mUDID = udid;
  mTypeName = product_type;
  mVersionString = ios_version;

  {
    char _device_name[15] = { 0 };
    char _device_gen[2] = { 0 };
    char _ios_version[2] = { 0 };

    nglString resolution;

    _device_name[0] = 0;
    _device_gen[0] = 0;
    _ios_version[0] = 0;

    if (product_type)
    {
      memcpy(_device_name, product_type, strlen(product_type)-3);
      if (_device_name)
        strncpy(_device_gen, strtok((char *)product_type, _device_name), 1);
    }
    if (ios_version)
      strncpy(_ios_version, ios_version, 1);

    int rev = atoi(_device_gen);

    if (strcmp(_device_name, "iPhone") == 0)
    {
      mType = iPhone;
      if (rev > 2)
        mRetina = true;
    }
    else if (strcmp(_device_name, "iPod") == 0)
    {
      mType = iPod;
      if (atoi(_device_gen) > 3)
        mRetina = true;
    }
    else if (strcmp(_device_name, "iPad") == 0)
    {
      mType = iPad;
    }
  }

  mDebugSocketPath = "/tmp/xspray-remote-debugserver-";
  mDebugSocketPath.Add(mUDID);

  printf("New iOS Device: '%s' (%s)  - UDID: %s - iOS: %s - screen: %s\n", mName.GetChars(), mTypeName.GetChars(), mUDID.GetChars(), mVersionString.GetChars(), mRetina?"retina":"");



  const char* domains[] = {
    "com.apple.mobile.internal",
    "com.apple.xcode.developerdomain",
    "com.apple.DevToolsFoundation",
    "com.apple.dt.DVTFoundation",
    "com.apple.dt.instruments.DTInstrumentsCP",
    "com.apple.dt.instruments.DTMessageQueueing",
    "com.apple.dt.instruments.DTXConnectionServices",
    "com.apple.dt.instruments.InstrumentsKit",
    "com.apple.dt.instruments.InstrumentsPlugIn",
    "com.apple.dt.services.capabilities.posix_spawn",
    "com.apple.dt.services.capabilities.server.wireless",
    "com.apple.DTDeviceKitBase",
    "com.apple.DVTiPhoneSimulatorRemoteClient",
    "com.apple.icon.doublelabel",
"com.apple.instruments.remoteserver",
    "com.apple.instruments.server",
    "com.apple.instruments.server.services.deviceinfo",
    "com.apple.instruments.server.services.launchdaemon",
    "com.apple.instruments.server.services.mobilenotifications",
    "com.apple.instruments.server.services.wireless",
    "com.apple.itunesstored.application_installed",
    "com.apple.mobile.application_installed",
    "com.apple.mobile.application_uninstalled",
    "com.apple.mobile.internal",
    "com.apple.mobile.lockdown.developer_status_changed",
    "com.apple.mobile.lockdown.device_name_changed",
"com.apple.mobile.notification_proxy",
    "com.apple.mobiledevice",
    "com.apple.options.single",
    "com.apple.pushbutton",
    "com.apple.springboard.deviceWillShutDown",
    "com.apple.xcode.developerdomain",
    "com.apple.xcode.SDKPath",
    "com.apple.xcode.simulatedDeviceFamily",
    "com.apple.xray.discovery.mobiledevice",
    "com.apple.xray.instrument-type.activity.all",
    "com.apple.xray.instrument-type.activity.cpu",
    "com.apple.xray.instrument-type.activity.disk",
    "com.apple.xray.instrument-type.activity.memory",
    "com.apple.xray.instrument-type.activity.network",
    "com.apple.xray.instrument-type.activity.process.fs_usage",
    "com.apple.xray.instrument-type.coreanimation",
    "com.apple.xray.instrument-type.coresamplerpmi",
    "com.apple.xray.instrument-type.counters",
    "com.apple.xray.instrument-type.embedded.opengl",
    "com.apple.xray.instrument-type.homeleaks",
    "com.apple.xray.instrument-type.keventsched",
    "com.apple.xray.instrument-type.keventsyscall",
    "com.apple.xray.instrument-type.keventvm",
    "com.apple.xray.instrument-type.oa",
    "com.apple.xray.instrument-type.sampler",
    "com.apple.xray.instrument-type.signpost",
    "com.apple.xray.instrument-type.vmtrack",
    "com.apple.xray.power.mobile.bluetooth",
    "com.apple.xray.power.mobile.cpu",
    "com.apple.xray.power.mobile.display",
    "com.apple.xray.power.mobile.energy",
    "com.apple.xray.power.mobile.gps",
    "com.apple.xray.power.mobile.net",
    "com.apple.xray.power.mobile.sleep",
    "com.apple.xray.power.mobile.wifi",
    NULL
  };

  const char* values[] =
  {
    "ActivationPublicKey",
    "ActivationState",
    "ActivationStateAcknowledged",
    "ActivityURL",
    "BasebandBootloaderVersion",
    "BasebandSerialNumber",
    "BasebandStatus",
    "BasebandVersion",
    "BluetoothAddress",
    "BuildVersion",
    "CPUArchitecture",
    "DeviceCertificate",
    "DeviceClass",
    "DeviceColor",
    "DeviceName",
    "DevicePublicKey",
    "DieID",
    "FirmwareVersion",
    "HardwareModel",
    "HardwarePlatform",
    "HostAttached",
    "IMLockdownEverRegisteredKey",
    "IntegratedCircuitCardIdentity",
    "InternationalMobileEquipmentIdentity",
    "InternationalMobileSubscriberIdentity",
    "iTunesHasConnected",
    "MLBSerialNumber",
    "MobileSubscriberCountryCode",
    "MobileSubscriberNetworkCode",
    "ModelNumber",
    "PartitionType",
    "PasswordProtected",
    "PhoneNumber",
    "ProductionSOC",
    "ProductType",
    "ProductVersion",
    "ProtocolVersion",
    "ProximitySensorCalibration",
    "RegionInfo",
    "SBLockdownEverRegisteredKey",
    "SerialNumber",
    "SIMStatus",
    "SoftwareBehavior",
    "SoftwareBundleVersion",
    "SupportedDeviceFamilies",
    "TelephonyCapability",
    "TimeIntervalSince1970",
    "TimeZone",
    "TimeZoneOffsetFromUTC",
    "TrustedHostAttached",
    "UniqueChipID",
    "UniqueDeviceID",
    "UseActivityURL",
    "UseRaptorCerts",
    "Uses24HourClock",
    "WeDelivered",
    "WiFiAddress",
    NULL
  };

  printf("------------------------------------------------\n");
  for (int i = 0; domains[i]; i++)
  {
    CFStringRef domain = CFStringCreateWithCString(NULL, domains[i], kCFStringEncodingUTF8);
    service_conn_t handle = NULL;
    unsigned int unknown = 0;
    mach_error_t err = AMDeviceStartService(mpDevice, domain, &handle, &unknown);

    if (!err)
      printf("Service %s -> %d %d %d\n", domains[i], err, handle, unknown);
  }

  printf("------------------------------------------------\n");
  for (int i = 0; domains[i]; i++)
  {
    CFStringRef value = CFStringCreateWithCString(NULL, domains[i], kCFStringEncodingUTF8);
    CFStringRef v = AMDeviceCopyValue(mpDevice, value, NULL);
    DisplayCFValue(domains[i], v);
  }

  printf("------------------------------------------------\n");
  for (int i = 0; values[i]; i++)
  {
    CFStringRef value = CFStringCreateWithCString(NULL, values[i], kCFStringEncodingUTF8);
    CFStringRef v = AMDeviceCopyValue(mpDevice, NULL, value);
    DisplayCFValue(values[i], v);
  }
  NGL_OUT("Done\n");

  (AMDeviceStopSession(mpDevice) == 0);
  (AMDeviceDisconnect(mpDevice) == 0);


}

iOSDevice::~iOSDevice()
{
  CFRelease(mpDevice);
  printf("Removed iOS Device: '%s' (%s)  - UDID: %s - iOS: %s - screen: %s\n", mName.GetChars(), mTypeName.GetChars(), mUDID.GetChars(), mVersionString.GetChars(), mRetina?"retina":"");
}

const nglString& iOSDevice::GetName() const
{
  return mName;
}

const nglString& iOSDevice::GetUDID() const
{
  return mUDID;
}

const nglString& iOSDevice::GetTypeName() const
{
  return mTypeName;
}

iOSDevice::Type iOSDevice::GetType() const
{
  return mType;
}

const nglString& iOSDevice::GetVersionString() const
{
  return mVersionString;
}

nuiSignal1<iOSDevice&> iOSDevice::DeviceConnected;
nuiSignal1<iOSDevice&> iOSDevice::DeviceDisconnected;

struct am_device_notification *iOSDevice::mpNotifications = NULL;

void iOSDevice::Init()
{
	AMDeviceNotificationSubscribe(&iOSDevice::DeviceCallback, 0, 0, NULL, &mpNotifications);
}

void iOSDevice::Exit()
{
  
}

std::map<am_device*, iOSDevice*> iOSDevice::mDevices;

void iOSDevice::DeviceCallback(struct am_device_notification_callback_info *info, void *arg)
{
	switch (info->msg)
  {
    case ADNCI_MSG_CONNECTED:
      {
        am_device* device = info->dev;
        iOSDevice* pDevice = new iOSDevice(device);
        mDevices[device] = pDevice;
        DeviceConnected(*pDevice);
      }
      break;
    case ADNCI_MSG_DISCONNECTED:
      {
        am_device* device = info->dev;
        auto it = mDevices.find(device);
        if (it != mDevices.end())
        {
          iOSDevice* pDevice = it->second;
          mDevices.erase(it);
          DeviceDisconnected(*pDevice);
          delete pDevice;
        }
      }
      break;
    default:
      break;
	}
}	 


bool iOSDevice::GetDeviceSupportPath(nglString& rDeviceSupportPath, nglString& rDeveloperDiskImagePath) const
{
  bool found1 = false;
  bool found2 = false;
  rDeviceSupportPath = nglPath();
  rDeveloperDiskImagePath = nglPath();


  CFStringRef v = AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductVersion"));
  DisplayCFValue("ProductVersion", v);
  nglString version = v;
  nglString smallversion = version.GetLeft(3);

  v = AMDeviceCopyValue(mpDevice, 0, CFSTR("BuildVersion"));
  DisplayCFValue("BuildVersion", v);
  nglString build = v;

  const char* home = getenv("HOME");
  {
    const char* sources[] =
    {
      "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}",
      "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{sversion}",

      "{home}/Library/Developer/Xcode/iOS DeviceSupport/{version} ({build})",
      "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build})",
      "{home}/Library/Developer/Xcode/iOS DeviceSupport/{version}",
      "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}",
      "/Applications/Xcode5-DP.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}",
      "/Applications/Xcode5-DP2.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}",
      NULL
    };

    for (int i = 0; sources[i] && !found1; i++)
    {
      nglString t = sources[i];
      t.Replace("{version}", version);
      t.Replace("{sversion}", smallversion);
      t.Replace("{build}", build);
      t.Replace("{home}", home);

      nglPath p = t;
      if (p.Exists())
      {
        rDeviceSupportPath = p;
        found1 = true;
      }
    }
  }

  {
    const char* sources[] =
    {
      // Current Xcode (Xcode 5 as of now)
      "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build})/DeveloperDiskImage.dmg",
      "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{sversion} ({build})/DeveloperDiskImage.dmg",
      "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}/DeveloperDiskImage.dmg",
      "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{sversion}/DeveloperDiskImage.dmg",

      // Last Beta version of Xcode
      "/Applications/Xcode6-Beta.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build})/DeveloperDiskImage.dmg",
      "/Applications/Xcode6-Beta.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{sversion} ({build})/DeveloperDiskImage.dmg",
      "/Applications/Xcode6-Beta.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}/DeveloperDiskImage.dmg",
      "/Applications/Xcode6-Beta.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{sversion}/DeveloperDiskImage.dmg",

      // Other known paths
      "{home}/Library/Developer/Xcode/iOS DeviceSupport/{version} ({build})/DeveloperDiskImage.dmg",
      "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build}/DeveloperDiskImage.dmg)",
      "{home}/Library/Developer/Xcode/iOS DeviceSupport/{version}/DeveloperDiskImage.dmg",
      "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}/DeveloperDiskImage.dmg",
      "{home}/Library/Developer/Xcode/iOS DeviceSupport/Latest/DeveloperDiskImage.dmg",
      "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/Latest/DeveloperDiskImage.dmg",
      "/Applications/Xcode5-DP.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build})/DeveloperDiskImage.dmg",
      "/Applications/Xcode5-DP2.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build})/DeveloperDiskImage.dmg",
      "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/6.1 (10B141)/DeveloperDiskImage.dmg",
      NULL
    };

    for (int i = 0; sources[i] && !found2; i++)
    {
      nglString t = sources[i];
      t.Replace("{version}", version);
      t.Replace("{sversion}", smallversion);
      t.Replace("{build}", build);
      t.Replace("{home}", home);

      nglPath p = t;
      if (p.Exists())
      {
        rDeveloperDiskImagePath = p;
        found2 = true;
      }
    }
  }

  return found1 && found2;
}


void iOSDevice::MountCallback(CFDictionaryRef dict, void * arg)
{
  iOSDevice* pDevice = (iOSDevice*)arg;
  CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));

  printf ("MountCallback arg = %p\n", arg);
  if (CFEqual(status, CFSTR("LookingUpImage")))
  {
    printf("[  0%%] Looking up developer disk image\n");
    pDevice->InstallationProgress(0.0f, "Looking up developer disk image");
  }
  else if (CFEqual(status, CFSTR("CopyingImage")))
  {
    printf("[ 30%%] Copying DeveloperDiskImage.dmg to device\n");
    pDevice->InstallationProgress(30.0f, "Copying developer disk image");
  }
  else if (CFEqual(status, CFSTR("MountingImage")))
  {
    printf("[ 90%%] Mounting developer disk image\n");
    pDevice->InstallationProgress(90.0f, "Mounting developer disk image");
  }
}

bool iOSDevice::MountDeveloperImage() const
{
  nglString ds_path;
  nglString image_path;
  bool res = GetDeviceSupportPath(ds_path, image_path);
  nglString sig_path = image_path + ".signature";

  NGL_OUT("Device support path: %s\n", ds_path.GetChars());
  NGL_OUT("Developer disk image: %s\n", image_path.GetChars());

  FILE* sig = fopen(sig_path.GetChars(), "rb");
  void *sig_buf = malloc(128);
  if (sig && fread(sig_buf, 1, 128, sig) != 128)
  {
    NGL_OUT("Fread error...");
  }
  fclose(sig);
  CFDataRef sig_data = CFDataCreateWithBytesNoCopy(NULL, (const UInt8*)sig_buf, 128, NULL);

  CFTypeRef keys[] = { CFSTR("ImageSignature"), CFSTR("ImageType") };
  CFTypeRef values[] = { sig_data, CFSTR("Developer") };
  CFDictionaryRef options = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFRelease(sig_data);

  CFStringRef cstr = image_path.ToCFString();
  int result = AMDeviceMountImage(mpDevice, cstr, options, (void*)&iOSDevice::MountCallback, (void*)this);
  CFRelease(cstr);
  CFRelease(options);

  if (result == 0)
  {
    NGL_OUT("[ 95%%] Developer disk image mounted successfully\n");
    InstallationProgress(95, "Developer disk image mounted successfully");
  }
  else if (result == 0xe8000076 /* already mounted */)
  {
    NGL_OUT("[ 95%%] Developer disk image already mounted\n");
    InstallationProgress(95, "Developer disk image already mounted");
  }
  else
  {
    NGL_OUT("[ !! ] Unable to mount developer disk image. (%x)\n", result);
    nglString str;
    str.CFormat("Unable to mount developer disk image. (%x)", result);
    InstallationProgress(0, str);
    return false;
  }

  return true;
}

void iOSDevice::TransferCallback(CFDictionaryRef dict, void* arg)
{
  iOSDevice* pDevice = (iOSDevice*)arg;
  int percent;
  CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));
  CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);

  if (CFEqual(status, CFSTR("CopyingFile")))
  {
    nglString path = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Path"));

    printf("[%3d%%] Copying %s to device\n", percent / 2, path.GetChars());
    nglString str;

    str.CFormat("Copying %s to device", path.GetChars());
    pDevice->InstallationProgress(percent, str);
  }
}

void iOSDevice::InstallCallback(CFDictionaryRef dict, void* arg)
{
  iOSDevice* pDevice = (iOSDevice*)arg;
  int percent;
  CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));
  CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);

  printf("[%3d%%] %s\n", (percent / 2) + 50, CFStringGetCStringPtr(status, kCFStringEncodingUTF8));
  pDevice->InstallationProgress(percent, CFStringGetCStringPtr(status, kCFStringEncodingUTF8));
}

void iOSDevice::FDVendorCallback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef address, const void *data, void *info)
{
  iOSDevice* pDevice = (iOSDevice*)info;

  CFSocketNativeHandle socket = (CFSocketNativeHandle)(*((CFSocketNativeHandle *)data));

  struct msghdr message;
  struct iovec iov[1];
  struct cmsghdr *control_message = NULL;
  char ctrl_buf[CMSG_SPACE(sizeof(int))];
  char dummy_data[1];

  memset(&message, 0, sizeof(struct msghdr));
  memset(ctrl_buf, 0, CMSG_SPACE(sizeof(int)));

  dummy_data[0] = ' ';
  iov[0].iov_base = dummy_data;
  iov[0].iov_len = sizeof(dummy_data);

  message.msg_name = NULL;
  message.msg_namelen = 0;
  message.msg_iov = iov;
  message.msg_iovlen = 1;
  message.msg_controllen = CMSG_SPACE(sizeof(int));
  message.msg_control = ctrl_buf;

  control_message = CMSG_FIRSTHDR(&message);
  control_message->cmsg_level = SOL_SOCKET;
  control_message->cmsg_type = SCM_RIGHTS;
  control_message->cmsg_len = CMSG_LEN(sizeof(int));

  *((int *) CMSG_DATA(control_message)) = pDevice->mDebuggerFD;

  sendmsg(socket, &message, 0);
  CFSocketInvalidate(s);
  CFRelease(s);
}

nglString iOSDevice::GetDeviceAppURL(const nglString& AppId)
{
  AMDeviceConnect(mpDevice);
  CFStringRef identifier = AppId.ToCFString();
  CFDictionaryRef result;
  int res = 0;
  if ((res = AMDeviceLookupApplications(mpDevice, 0, &result)) != 0)
    return nglString::Null;

  CFDictionaryRef app_dict = (CFDictionaryRef)CFDictionaryGetValue(result, identifier);
  if (app_dict == NULL)
    return nglString::Null;

  CFStringRef app_path = (CFStringRef)CFDictionaryGetValue(app_dict, CFSTR("Path"));
  if (app_path == NULL)
    return nglString::Null;

  CFURLRef url = CFURLCreateWithFileSystemPath(NULL, app_path, kCFURLPOSIXPathStyle, true);
  CFRelease(result);
  CFRelease(identifier);

  CFStringRef app_url = CFURLGetString(url);

  nglString resultUrl(app_url);
  CFRelease(url);
  return resultUrl;
}

nglString iOSDevice::GetDiskAppIdentifier(const nglString& AppURL)
{
  nglString url = AppURL;
  url.Prepend("file://");
  CFStringRef disk_app_url_str = url.ToCFString();
  CFURLRef disk_app_url = CFURLCreateWithString(NULL, disk_app_url_str, NULL);
  CFURLRef plist_url = CFURLCreateCopyAppendingPathComponent(NULL, disk_app_url, CFSTR("Info.plist"), false);
  CFReadStreamRef plist_stream = CFReadStreamCreateWithFile(NULL, plist_url);
  CFReadStreamOpen(plist_stream);
  CFPropertyListRef plist = CFPropertyListCreateWithStream(NULL, plist_stream, 0, kCFPropertyListImmutable, NULL, NULL);
  CFStringRef bundle_identifier = (CFStringRef)CFRetain((CFTypeRef)CFDictionaryGetValue((CFDictionaryRef)plist, CFSTR("CFBundleIdentifier")));
  CFReadStreamClose(plist_stream);

  CFRelease(plist_url);
  CFRelease(plist_stream);
  CFRelease(plist);
  CFRelease(disk_app_url);
  CFRelease(disk_app_url_str);

  nglString id(bundle_identifier);
  CFRelease(bundle_identifier);
  return id;
}


bool iOSDevice::StartRemoteDebugServer()
{
  mach_error_t result = AMDeviceStartService(mpDevice, CFSTR("com.apple.debugserver"), &mDebuggerFD, NULL);
  return result == MDERR_OK;

  CFSocketContext CTX = { 0, (void*)this, NULL, NULL, NULL };
  CFSocketRef fdvendor = CFSocketCreate(NULL, AF_UNIX, 0, 0, kCFSocketAcceptCallBack, &iOSDevice::FDVendorCallback, &CTX);

  int yes = 1;
  setsockopt(CFSocketGetNative(fdvendor), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, mDebugSocketPath.GetChars());
  address.sun_len = SUN_LEN(&address);
  CFDataRef address_data = CFDataCreate(NULL, (const UInt8 *)&address, sizeof(address));

  unlink(mDebugSocketPath.GetChars());

  CFSocketSetAddress(fdvendor, address_data);
  CFRelease(address_data);
  CFRunLoopAddSource(CFRunLoopGetMain(), CFSocketCreateRunLoopSource(NULL, fdvendor, 0), kCFRunLoopCommonModes);
}


bool iOSDevice::InstallApplication(const nglPath& rPath)
{
  AMDeviceConnect(mpDevice);
  bool paired = AMDeviceIsPaired(mpDevice);
  bool validated = AMDeviceValidatePairing(mpDevice) == 0;
  bool sessionstarted = AMDeviceStartSession(mpDevice) == 0;

  //if (paired && validated && sessionstarted)
  {
    CFStringRef path = CFStringCreateWithCString(NULL, rPath.GetChars(), kCFStringEncodingUTF8);
    CFURLRef relative_url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, false);
    CFURLRef url = CFURLCopyAbsoluteURL(relative_url);

    CFRelease(relative_url);

    service_conn_t afcFd;
    bool servicestarted = (AMDeviceStartService(mpDevice, CFSTR("com.apple.afc"), &afcFd, NULL) == 0);
    (AMDeviceStopSession(mpDevice) == 0);
    (AMDeviceDisconnect(mpDevice) == 0);
    (AMDeviceTransferApplication(afcFd, path, NULL, &iOSDevice::TransferCallback, (void*)this) == 0);

    close(afcFd);

    CFStringRef keys[] = { CFSTR("PackageType") };
    CFStringRef values[] = { CFSTR("Developer") };
    CFDictionaryRef options = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    AMDeviceConnect(mpDevice);
    (AMDeviceIsPaired(mpDevice));
    (AMDeviceValidatePairing(mpDevice) == 0);
    (AMDeviceStartSession(mpDevice) == 0);

    service_conn_t installFd;
    (AMDeviceStartService(mpDevice, CFSTR("com.apple.mobile.installation_proxy"), &installFd, NULL) == 0);

    (AMDeviceStopSession(mpDevice) == 0);
    (AMDeviceDisconnect(mpDevice) == 0);

    mach_error_t result = AMDeviceInstallApplication(installFd, path, options, InstallCallback, (void*)this);
    if (result != 0)
    {
      printf("AMDeviceInstallApplication failed: %d\n", result);
      nglString str;
      str.CFormat("AMDeviceInstallApplication failed: %d", result);
      InstallationProgress(0, str);
      return false;
    }

    close(installFd);

    CFRelease(path);
    CFRelease(options);

    NGL_OUT("[100%%] Installed package %s\n", rPath.GetChars());
    nglString str;
    str.CFormat("Installed package %s", rPath.GetChars());
    InstallationProgress(100, str);
    return true;
  }

  return false;
}

bool iOSDevice::StartDebugServer()
{
  AMDeviceConnect(mpDevice);
  bool paired = AMDeviceIsPaired(mpDevice);
  bool validated = AMDeviceValidatePairing(mpDevice) == 0;
  bool sessionstarted = AMDeviceStartSession(mpDevice) == 0;

  //if (paired && validated && sessionstarted)
  {
    if (!MountDeveloperImage())
      return false;      // put debugserver on the device
    return StartRemoteDebugServer();  // start debugserver
  }

  return false;
}

nglString iOSDevice::GetDebugSocketPath() const
{
  return mDebugSocketPath;
}

service_conn_t iOSDevice::GetDebugSocket() const
{
  return mDebuggerFD;
}


int32 iOSDevice::GetDeviceCount()
{
  return mDevices.size();
}

iOSDevice* iOSDevice::GetDevice(int32 index)
{
  auto it = mDevices.begin();
  auto end = mDevices.end();
  int i = 0;
  while (it != end)
  {
    if (i == index)
      return it->second;

    ++it;
    ++i;
  }
  return NULL;
}
