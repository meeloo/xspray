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

#if 0
#define FDVENDOR_PATH  "/tmp/xspray-remote-debugserver"
#define PREP_CMDS_PATH "/tmp/xspray-gdb-prep-cmds"
//#define GDB_SHELL      "/Developer/Platforms/iPhoneOS.platform/Developer/usr/libexec/gdb/gdb-arm-apple-darwin --arch armv7 -q -x " PREP_CMDS_PATH
#define GDB_SHELL      "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/libexec/gdb/gdb-arm-apple-darwin --arch armv7 -q -x " PREP_CMDS_PATH

// approximation of what Xcode does:
#define GDB_PREP_CMDS CFSTR("set mi-show-protections off\n\
set auto-raise-load-levels 1\n\
set shlib-path-substitutions /usr \"{ds_path}/Symbols/usr\" /System \"{ds_path}/Symbols/System\" \"{device_container}\" \"{disk_container}\" \"/private{device_container}\" \"{disk_container}\" /Developer \"{ds_path}/Symbols/Developer\"\n\
set remote max-packet-size 1024\n\
set sharedlibrary check-uuids on\n\
set env NSUnbufferedIO YES\n\
set minimal-signal-handling 1\n\
set sharedlibrary load-rules \\\".*\\\" \\\".*\\\" container\n\
set inferior-auto-start-dyld 0\n\
file \"{disk_app}\"\n\
set remote executable-directory {device_app}\n\
set remote noack-mode 1\n\
set trust-readonly-sections 1\n\
target remote-mobile " FDVENDOR_PATH "\n\
mem 0x1000 0x3fffffff cache\n\
mem 0x40000000 0xffffffff none\n\
mem 0x00000000 0x0fff none\n\
run {args}\n\
set minimal-signal-handling 0\n\
set inferior-auto-start-cfm off\n\
set sharedLibrary load-rules dyld \".*libobjc.*\" all dyld \".*CoreFoundation.*\" all dyld \".*Foundation.*\" all dyld \".*libSystem.*\" all dyld \".*AppKit.*\" all dyld \".*PBGDBIntrospectionSupport.*\" all dyld \".*/usr/lib/dyld.*\" all dyld \".*CarbonDataFormatters.*\" all dyld \".*libauto.*\" all dyld \".*CFDataFormatters.*\" all dyld \"/System/Library/Frameworks\\\\\\\\|/System/Library/PrivateFrameworks\\\\\\\\|/usr/lib\" extern dyld \".*\" all exec \".*\" all\n\
sharedlibrary apply-load-rules all\n\
set inferior-auto-start-dyld 1")

void write_gdb_prep_cmds(AMDeviceRef device, CFURLRef disk_app_url)
{
  CFMutableStringRef cmds = CFStringCreateMutableCopy(NULL, 0, GDB_PREP_CMDS);
  CFRange range = { 0, CFStringGetLength(cmds) };

  CFStringRef ds_path = copy_device_support_path(device);
  CFStringFindAndReplace(cmds, CFSTR("{ds_path}"), ds_path, range, 0);
  range.length = CFStringGetLength(cmds);

  if (args)
  {
    CFStringRef cf_args = CFStringCreateWithCString(NULL, args, kCFStringEncodingASCII);
    CFStringFindAndReplace(cmds, CFSTR("{args}"), cf_args, range, 0);
    CFRelease(cf_args);
  }
  else
  {
    CFStringFindAndReplace(cmds, CFSTR(" {args}"), CFSTR(""), range, 0);
  }
  range.length = CFStringGetLength(cmds);

  CFStringRef bundle_identifier = copy_disk_app_identifier(disk_app_url);
  CFURLRef device_app_url = copy_device_app_url(device, bundle_identifier);
  CFStringRef device_app_path = CFURLCopyFileSystemPath(device_app_url, kCFURLPOSIXPathStyle);
  CFStringFindAndReplace(cmds, CFSTR("{device_app}"), device_app_path, range, 0);
  range.length = CFStringGetLength(cmds);

  CFStringRef disk_app_path = CFURLCopyFileSystemPath(disk_app_url, kCFURLPOSIXPathStyle);
  CFStringFindAndReplace(cmds, CFSTR("{disk_app}"), disk_app_path, range, 0);
  range.length = CFStringGetLength(cmds);

  CFURLRef device_container_url = CFURLCreateCopyDeletingLastPathComponent(NULL, device_app_url);
  CFStringRef device_container_path = CFURLCopyFileSystemPath(device_container_url, kCFURLPOSIXPathStyle);
  CFMutableStringRef dcp_noprivate = CFStringCreateMutableCopy(NULL, 0, device_container_path);
  range.length = CFStringGetLength(dcp_noprivate);
  CFStringFindAndReplace(dcp_noprivate, CFSTR("/private/var/"), CFSTR("/var/"), range, 0);
  range.length = CFStringGetLength(cmds);
  CFStringFindAndReplace(cmds, CFSTR("{device_container}"), dcp_noprivate, range, 0);
  range.length = CFStringGetLength(cmds);

  CFURLRef disk_container_url = CFURLCreateCopyDeletingLastPathComponent(NULL, disk_app_url);
  CFStringRef disk_container_path = CFURLCopyFileSystemPath(disk_container_url, kCFURLPOSIXPathStyle);
  CFStringFindAndReplace(cmds, CFSTR("{disk_container}"), disk_container_path, range, 0);

  CFDataRef cmds_data = CFStringCreateExternalRepresentation(NULL, cmds, kCFStringEncodingASCII, 0);
  FILE *out = fopen(PREP_CMDS_PATH, "w");
  fwrite(CFDataGetBytePtr(cmds_data), CFDataGetLength(cmds_data), 1, out);
  fclose(out);

  CFRelease(cmds);
  if (ds_path != NULL)
    CFRelease(ds_path);
  CFRelease(bundle_identifier);
  CFRelease(device_app_url);
  CFRelease(device_app_path);
  CFRelease(disk_app_path);
  CFRelease(device_container_url);
  CFRelease(device_container_path);
  CFRelease(dcp_noprivate);
  CFRelease(disk_container_url);
  CFRelease(disk_container_path);
  CFRelease(cmds_data);
}

#endif

using namespace Xspray;

iOSDevice::iOSDevice(am_device *device)
: mpDevice(device), mType(Unknown), mRetina(false), mDebuggerFD(0)
{
	CFStringEncoding encoding = CFStringGetSystemEncoding();
	const char *udid          = CFStringGetCStringPtr(AMDeviceCopyDeviceIdentifier(mpDevice), encoding);

  CFRetain(mpDevice);

  AMDeviceConnect(mpDevice);
  (AMDeviceIsPaired(mpDevice));
  (AMDeviceValidatePairing(mpDevice) == 0);
  (AMDeviceStartSession(mpDevice) == 0);

  const char *device_name  = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("DeviceName")),     encoding);
  const char *product_type = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductType")),    encoding);
  const char *ios_version  = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductVersion")), encoding);

  (AMDeviceStopSession(mpDevice) == 0);
  (AMDeviceDisconnect(mpDevice) == 0);

  mName = device_name;
  mUDID = udid;
  mTypeName = product_type;
  mVersionString = ios_version;

  {
    char _device_name[15];
    char _device_gen[2];
    char _ios_version[2];

    nglString resolution;

    memcpy(_device_name, product_type, strlen(product_type)-3);
    strncpy(_device_gen, strtok((char *)product_type, _device_name), 1);
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


bool iOSDevice::GetDeviceSupportPath(nglString& rPath) const
{
  nglString version = AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductVersion"));
  nglString build = AMDeviceCopyValue(mpDevice, 0, CFSTR("BuildVersion"));
  const char* home = getenv("HOME");
  nglString path;
  bool found = false;

  const char* sources[] =
  {
    "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}",
    "{home}/Library/Developer/Xcode/iOS DeviceSupport/{version} ({build})",
    "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build})",
    "{home}/Library/Developer/Xcode/iOS DeviceSupport/{version}",
    "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}",
    NULL
  };

  for (int i = 0; sources[i]; i++)
  {
    nglString t = sources[i];
    t.Replace("{version}", version);
    t.Replace("{build}", build);
    t.Replace("{home}", home);

    nglPath p = t;
    if (p.Exists())
    {
      rPath = p;
      return true;
    }
  }

  rPath = nglPath();
  return false;
}

bool iOSDevice::GetDeveloperDiskImagePath(nglString& rPath) const
{
  nglString version = AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductVersion"));
  nglString build = AMDeviceCopyValue(mpDevice, 0, CFSTR("BuildVersion"));
  nglString home = getenv("HOME");
  nglString path;
  bool found = false;

  const char* sources[] =
  {
    "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build})/DeveloperDiskImage.dmg",
    "{home}/Library/Developer/Xcode/iOS DeviceSupport/{version} ({build})/DeveloperDiskImage.dmg",
    "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version} ({build}/DeveloperDiskImage.dmg)",
    "{home}/Library/Developer/Xcode/iOS DeviceSupport/{version}/DeveloperDiskImage.dmg",
    "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/{version}/DeveloperDiskImage.dmg",
    "{home}/Library/Developer/Xcode/iOS DeviceSupport/Latest/DeveloperDiskImage.dmg",
    "/Developer/Platforms/iPhoneOS.platform/DeviceSupport/Latest/DeveloperDiskImage.dmg",
    "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/6.1 (10B141)/DeveloperDiskImage.dmg",
    NULL
  };

  for (int i = 0; sources[i]; i++)
  {
    nglString t = sources[i];
    t.Replace("{version}", version);
    t.Replace("{build}", build);
    t.Replace("{home}", home);

    nglPath p = t;
    if (p.Exists())
    {
      rPath = p;
      return true;
    }
  }

  rPath = nglPath();
  return false;
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
  bool res = GetDeviceSupportPath(ds_path);
  nglString image_path;
  res = GetDeveloperDiskImagePath(image_path);
  nglString sig_path = image_path + ".signature";

  NGL_OUT("Device support path: %s\n", ds_path.GetChars());
  NGL_OUT("Developer disk image: %s\n", image_path.GetChars());

  FILE* sig = fopen(sig_path.GetChars(), "rb");
  void *sig_buf = malloc(128);
  if (fread(sig_buf, 1, 128, sig) != 128)
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

  printf("[%3d%%] %s\n", (percent / 2) + 50, CFStringGetCStringPtr(status, kCFStringEncodingMacRoman));
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

CFURLRef iOSDevice::GetDeviceAppURL(CFStringRef identifier)
{
  CFDictionaryRef result;
  if (AMDeviceLookupApplications(mpDevice, 0, &result) != 0)
    return NULL;

  CFDictionaryRef app_dict = (CFDictionaryRef)CFDictionaryGetValue(result, identifier);
  if (app_dict == NULL)
    return NULL;

  CFStringRef app_path = (CFStringRef)CFDictionaryGetValue(app_dict, CFSTR("Path"));
  if (app_path == NULL)
    return NULL;

  CFURLRef url = CFURLCreateWithFileSystemPath(NULL, app_path, kCFURLPOSIXPathStyle, true);
  CFRelease(result);
  return url;
}

CFStringRef iOSDevice::GetDiskAppIdentifier(CFURLRef disk_app_url)
{
  CFURLRef plist_url = CFURLCreateCopyAppendingPathComponent(NULL, disk_app_url, CFSTR("Info.plist"), false);
  CFReadStreamRef plist_stream = CFReadStreamCreateWithFile(NULL, plist_url);
  CFReadStreamOpen(plist_stream);
  CFPropertyListRef plist = CFPropertyListCreateWithStream(NULL, plist_stream, 0, kCFPropertyListImmutable, NULL, NULL);
  CFStringRef bundle_identifier = (CFStringRef)CFRetain((CFTypeRef)CFDictionaryGetValue((CFDictionaryRef)plist, CFSTR("CFBundleIdentifier")));
  CFReadStreamClose(plist_stream);

  CFRelease(plist_url);
  CFRelease(plist_stream);
  CFRelease(plist);

  return bundle_identifier;
}


void iOSDevice::StartRemoteDebugServer()
{
  AMDeviceStartService(mpDevice, CFSTR("com.apple.debugserver"), &mDebuggerFD, NULL);
  return;

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
    CFStringRef path = CFStringCreateWithCString(NULL, rPath.GetChars(), kCFStringEncodingASCII);
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
    StartRemoteDebugServer();  // start debugserver
    return true;
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
