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

  typedef void (*AMinstall_callback)(CFDictionaryRef dict, int arg);
  typedef void (*AMtransfer_callback)(CFDictionaryRef dict, int arg);
  mach_error_t AMDeviceInstallApplication(service_conn_t sockInstallProxy, CFStringRef cfStrPath, CFDictionaryRef opts, AMinstall_callback callback, int* unknow);
  mach_error_t AMDeviceTransferApplication(service_conn_t afcFd, CFStringRef path, CFDictionaryRef opts, AMtransfer_callback, int* unknown);


  int AMDeviceSecureTransferPath(int zero, AMDeviceRef device, CFURLRef url, CFDictionaryRef options, void *callback, void* cbarg);
  int AMDeviceSecureInstallApplication(int zero, AMDeviceRef device, CFURLRef url, CFDictionaryRef options, void *callback, void* cbarg);
  int AMDeviceMountImage(AMDeviceRef device, CFStringRef image, CFDictionaryRef options, void *callback, void* cbarg);
  int AMDeviceLookupApplications(AMDeviceRef device, int zero, CFDictionaryRef* result);
  
  
#ifdef __cplusplus
}
#endif

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

bool found_device = false, debug = false, verbose = false;
char *app_path = NULL;
char *device_id = NULL;
char *args = NULL;
int timeout = 0;
CFStringRef last_path = NULL;
service_conn_t gdbfd;

Boolean path_exists(CFTypeRef path)
{
  if (CFGetTypeID(path) == CFStringGetTypeID())
  {
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, (CFStringRef)path, kCFURLPOSIXPathStyle, true);
    Boolean result = CFURLResourceIsReachable(url, NULL);
    CFRelease(url);
    return result;
  }
  else if (CFGetTypeID(path) == CFURLGetTypeID())
  {
    return CFURLResourceIsReachable((CFURLRef)path, NULL);
  }
  else
  {
    return false;
  }
}

CFStringRef copy_device_support_path(AMDeviceRef device)
{
  CFStringRef version = AMDeviceCopyValue(device, 0, CFSTR("ProductVersion"));
  CFStringRef build = AMDeviceCopyValue(device, 0, CFSTR("BuildVersion"));
  const char* home = getenv("HOME");
  CFStringRef path;
  bool found = false;

  path = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s/Library/Developer/Xcode/iOS DeviceSupport/%@ (%@)"), home, version, build);
  found = path_exists(path);

  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("/Developer/Platforms/iPhoneOS.platform/DeviceSupport/%@ (%@)"), version, build);
    found = path_exists(path);
  }
  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s/Library/Developer/Xcode/iOS DeviceSupport/%@"), home, version);
    found = path_exists(path);
  }
  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("/Developer/Platforms/iPhoneOS.platform/DeviceSupport/%@"), version);
    found = path_exists(path);
  }
  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/%@"), version);
    found = path_exists(path);
  }

  CFRelease(version);
  CFRelease(build);

  if (!found)
  {
    CFRelease(path);
    printf("[ !! ] Unable to locate DeviceSupport directory.\n");
    exit(1);
  }

  return path;
}

CFStringRef copy_developer_disk_image_path(AMDeviceRef device)
{
  CFStringRef version = AMDeviceCopyValue(device, 0, CFSTR("ProductVersion"));
  CFStringRef build = AMDeviceCopyValue(device, 0, CFSTR("BuildVersion"));
  const char *home = getenv("HOME");
  CFStringRef path;
  bool found = false;

  path = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s/Library/Developer/Xcode/iOS DeviceSupport/%@ (%@)/DeveloperDiskImage.dmg"), home, version, build);
  found = path_exists(path);

  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("/Developer/Platforms/iPhoneOS.platform/DeviceSupport/%@ (%@/DeveloperDiskImage.dmg)"), version, build);
    found = path_exists(path);
  }
  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s/Library/Developer/Xcode/iOS DeviceSupport/@%/DeveloperDiskImage.dmg"), home, version);
    found = path_exists(path);
  }
  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("/Developer/Platforms/iPhoneOS.platform/DeviceSupport/@%/DeveloperDiskImage.dmg"), version);
    found = path_exists(path);
  }
  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s/Library/Developer/Xcode/iOS DeviceSupport/Latest/DeveloperDiskImage.dmg"), home);
    found = path_exists(path);
  }
  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("/Developer/Platforms/iPhoneOS.platform/DeviceSupport/Latest/DeveloperDiskImage.dmg"));
    found = path_exists(path);
  }
  if (!found)
  {
    path = CFStringCreateWithFormat(NULL, NULL, CFSTR("/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/DeviceSupport/6.1 (10B141)/DeveloperDiskImage.dmg"));
    found = path_exists(path);
  }

  CFRelease(version);
  CFRelease(build);

  if (!found)
  {
    CFRelease(path);
    printf("[ !! ] Unable to locate DeviceSupport directory containing DeveloperDiskImage.dmg.\n");
    exit(1);
  }

  return path;
}

void mount_callback(CFDictionaryRef dict, int arg)
{
  CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));

  if (CFEqual(status, CFSTR("LookingUpImage")))
  {
    printf("[  0%%] Looking up developer disk image\n");
  }
  else if (CFEqual(status, CFSTR("CopyingImage")))
  {
    printf("[ 30%%] Copying DeveloperDiskImage.dmg to device\n");
  }
  else if (CFEqual(status, CFSTR("MountingImage")))
  {
    printf("[ 90%%] Mounting developer disk image\n");
  }
}

void mount_developer_image(AMDeviceRef device)
{
  CFStringRef ds_path = copy_device_support_path(device);
  CFStringRef image_path = copy_developer_disk_image_path(device);
  CFStringRef sig_path = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@.signature"), image_path);
  CFRelease(ds_path);

  if (verbose)
  {
    printf("Device support path: ");
    fflush(stdout);
    CFShow(ds_path);
    printf("Developer disk image: ");
    fflush(stdout);
    CFShow(image_path);
  }

  FILE* sig = fopen(CFStringGetCStringPtr(sig_path, kCFStringEncodingMacRoman), "rb");
  void *sig_buf = malloc(128);
  assert(fread(sig_buf, 1, 128, sig) == 128);
  fclose(sig);
  CFDataRef sig_data = CFDataCreateWithBytesNoCopy(NULL, (const UInt8*)sig_buf, 128, NULL);
  CFRelease(sig_path);

  CFTypeRef keys[] = { CFSTR("ImageSignature"), CFSTR("ImageType") };
  CFTypeRef values[] = { sig_data, CFSTR("Developer") };
  CFDictionaryRef options = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFRelease(sig_data);

  int result = AMDeviceMountImage(device, image_path, options, (void*)&mount_callback, 0);
  if (result == 0)
  {
    printf("[ 95%%] Developer disk image mounted successfully\n");
  }
  else if (result == 0xe8000076 /* already mounted */)
  {
    printf("[ 95%%] Developer disk image already mounted\n");
  }
  else
  {
    printf("[ !! ] Unable to mount developer disk image. (%x)\n", result);
    exit(1);
  }

  CFRelease(image_path);
  CFRelease(options);
}

void transfer_callback(CFDictionaryRef dict, int arg)
{
  int percent;
  CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));
  CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);

  if (CFEqual(status, CFSTR("CopyingFile")))
  {
    CFStringRef path = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Path"));

    if ((last_path == NULL || !CFEqual(path, last_path)) && !CFStringHasSuffix(path, CFSTR(".ipa")))
    {
      printf("[%3d%%] Copying %s to device\n", percent / 2, CFStringGetCStringPtr(path, kCFStringEncodingMacRoman));
    }

    if (last_path != NULL)
    {
      CFRelease(last_path);
    }
    last_path = CFStringCreateCopy(NULL, path);
  }
}

void install_callback(CFDictionaryRef dict, int arg)
{
  int percent;
  CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));
  CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);

  printf("[%3d%%] %s\n", (percent / 2) + 50, CFStringGetCStringPtr(status, kCFStringEncodingMacRoman));
}

void fdvendor_callback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef address, const void *data, void *info)
{
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

  *((int *) CMSG_DATA(control_message)) = gdbfd;

  sendmsg(socket, &message, 0);
  CFSocketInvalidate(s);
  CFRelease(s);
}

CFURLRef copy_device_app_url(AMDeviceRef device, CFStringRef identifier)
{
  CFDictionaryRef result;
  assert(AMDeviceLookupApplications(device, 0, &result) == 0);

  CFDictionaryRef app_dict = (CFDictionaryRef)CFDictionaryGetValue(result, identifier);
  assert(app_dict != NULL);

  CFStringRef app_path = (CFStringRef)CFDictionaryGetValue(app_dict, CFSTR("Path"));
  assert(app_path != NULL);

  CFURLRef url = CFURLCreateWithFileSystemPath(NULL, app_path, kCFURLPOSIXPathStyle, true);
  CFRelease(result);
  return url;
}

CFStringRef copy_disk_app_identifier(CFURLRef disk_app_url)
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

void start_remote_debug_server(AMDeviceRef device)
{
  assert(AMDeviceStartService(device, CFSTR("com.apple.debugserver"), &gdbfd, NULL) == 0);

  CFSocketRef fdvendor = CFSocketCreate(NULL, AF_UNIX, 0, 0, kCFSocketAcceptCallBack, &fdvendor_callback, NULL);

  int yes = 1;
  setsockopt(CFSocketGetNative(fdvendor), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, FDVENDOR_PATH);
  address.sun_len = SUN_LEN(&address);
  CFDataRef address_data = CFDataCreate(NULL, (const UInt8 *)&address, sizeof(address));

  unlink(FDVENDOR_PATH);

  CFSocketSetAddress(fdvendor, address_data);
  CFRelease(address_data);
  CFRunLoopAddSource(CFRunLoopGetMain(), CFSocketCreateRunLoopSource(NULL, fdvendor, 0), kCFRunLoopCommonModes);
}

void gdb_ready_handler(int signum)
{
	_exit(0);
}



void handle_device(AMDeviceRef device)
{
  if (found_device) return; // handle one device only

  CFStringRef found_device_id = AMDeviceCopyDeviceIdentifier(device);

  if (device_id != NULL)
  {
    if(strcmp(device_id, CFStringGetCStringPtr(found_device_id, CFStringGetSystemEncoding())) == 0)
    {
      found_device = true;
    }
    else
    {
      return;
    }
  }
  else
  {
    found_device = true;
  }

  CFRetain(device); // don't know if this is necessary?

  printf("[  0%%] Found device (%s), beginning install\n", CFStringGetCStringPtr(found_device_id, CFStringGetSystemEncoding()));

  AMDeviceConnect(device);
  assert(AMDeviceIsPaired(device));
  assert(AMDeviceValidatePairing(device) == 0);
  assert(AMDeviceStartSession(device) == 0);

  CFStringRef path = CFStringCreateWithCString(NULL, app_path, kCFStringEncodingASCII);
  CFURLRef relative_url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, false);
  CFURLRef url = CFURLCopyAbsoluteURL(relative_url);

  CFRelease(relative_url);

  service_conn_t afcFd;
  assert(AMDeviceStartService(device, CFSTR("com.apple.afc"), &afcFd, NULL) == 0);
  assert(AMDeviceStopSession(device) == 0);
  assert(AMDeviceDisconnect(device) == 0);
  assert(AMDeviceTransferApplication(afcFd, path, NULL, transfer_callback, NULL) == 0);

  close(afcFd);

  CFStringRef keys[] = { CFSTR("PackageType") };
  CFStringRef values[] = { CFSTR("Developer") };
  CFDictionaryRef options = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

  AMDeviceConnect(device);
  assert(AMDeviceIsPaired(device));
  assert(AMDeviceValidatePairing(device) == 0);
  assert(AMDeviceStartSession(device) == 0);

  service_conn_t installFd;
  assert(AMDeviceStartService(device, CFSTR("com.apple.mobile.installation_proxy"), &installFd, NULL) == 0);

  assert(AMDeviceStopSession(device) == 0);
  assert(AMDeviceDisconnect(device) == 0);

  mach_error_t result = AMDeviceInstallApplication(installFd, path, options, install_callback, NULL);
  if (result != 0)
  {
    printf("AMDeviceInstallApplication failed: %d\n", result);
    exit(1);
  }

  close(installFd);

  CFRelease(path);
  CFRelease(options);

  printf("[100%%] Installed package %s\n", app_path);

  if (!debug)
    exit(0); // no debug phase

  AMDeviceConnect(device);
  assert(AMDeviceIsPaired(device));
  assert(AMDeviceValidatePairing(device) == 0);
  assert(AMDeviceStartSession(device) == 0);

  printf("------ Debug phase ------\n");

  mount_developer_image(device);      // put debugserver on the device
  start_remote_debug_server(device);  // start debugserver
  write_gdb_prep_cmds(device, url);   // dump the necessary gdb commands into a file

  CFRelease(url);

  printf("[100%%] Connecting to remote debug server\n");
  printf("-------------------------\n");

  signal(SIGHUP, gdb_ready_handler);

  pid_t parent = getpid();
  int pid = fork();
  if (pid == 0)
  {
    printf("Launching debugger with command:\n%s\n\n", GDB_SHELL);
    system(GDB_SHELL);      // launch gdb
    kill(parent, SIGHUP);  // "No. I am your father."
    _exit(0);
  }
}

void device_callback(struct am_device_notification_callback_info *info, void *arg)
{
  switch (info->msg) {
    case ADNCI_MSG_CONNECTED:
      handle_device(info->dev);
    default:
      break;
  }
}

void timeout_callback(CFRunLoopTimerRef timer, void *info)
{
  if (!found_device) {
    printf("Timed out waiting for device.\n");
    exit(1);
  }
}

#if 0
void usage(const char* app)
{
  printf("usage: %s [-d/--debug] [-i/--id device_id] -b/--bundle bundle.app [-a/--args arguments] [-t/--timeout timeout(seconds)]\n", app);
}

int main(int argc, char *argv[])
{
  static struct option longopts[] =
  {
    { "debug", no_argument, NULL, 'd' },
    { "id", required_argument, NULL, 'i' },
    { "bundle", required_argument, NULL, 'b' },
    { "args", required_argument, NULL, 'a' },
    { "verbose", no_argument, NULL, 'v' },
    { "timeout", required_argument, NULL, 't' },
    { NULL, 0, NULL, 0 },
  };
  char ch;

  while ((ch = getopt_long(argc, argv, "dvi:b:a:t:", longopts, NULL)) != -1)
  {
    switch (ch)
    {
      case 'd':
        debug = 1;
        break;
      case 'i':
        device_id = optarg;
        break;
      case 'b':
        app_path = optarg;
        break;
      case 'a':
        args = optarg;
        break;
      case 'v':
        verbose = 1;
        break;
      case 't':
        timeout = atoi(optarg);
        break;
      default:
        usage(argv[0]);
        return 1;
    }
  }

  if (!app_path)
  {
    usage(argv[0]);
    exit(0);
  }

  printf("------ Install phase ------\n");

  assert(access(app_path, F_OK) == 0);

  AMDSetLogLevel(5); // otherwise syslog gets flooded with crap
  if (timeout > 0)
  {
    CFRunLoopTimerRef timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent() + timeout, 0, 0, 0, timeout_callback, NULL);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
    printf("[....] Waiting up to %d seconds for iOS device to be connected\n", timeout);
  }
  else
  {
    printf("[....] Waiting for iOS device to be connected\n");
  }

  struct am_device_notification *notify;
  AMDeviceNotificationSubscribe(&device_callback, 0, 0, NULL, &notify);
  CFRunLoopRun();
}
#endif

using namespace Xspray;

iOSDevice::iOSDevice(am_device *device)
: mpDevice(device), mType(Unknown), mRetina(false)
{
	CFStringEncoding encoding = CFStringGetSystemEncoding();
	const char *udid          = CFStringGetCStringPtr(AMDeviceCopyDeviceIdentifier(mpDevice), encoding);

  CFRetain(mpDevice);

  AMDeviceConnect(mpDevice);
  assert(AMDeviceIsPaired(mpDevice));
  assert(AMDeviceValidatePairing(mpDevice) == 0);
  assert(AMDeviceStartSession(mpDevice) == 0);

  const char *device_name  = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("DeviceName")),     encoding);
  const char *product_type = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductType")),    encoding);
  const char *ios_version  = CFStringGetCStringPtr(AMDeviceCopyValue(mpDevice, 0, CFSTR("ProductVersion")), encoding);

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

  printf("New iOS Device: '%s' (%s)  - UDID: %s - iOS: %s - screen: %s\n", mName.GetChars(), mTypeName.GetChars(), mUDID.GetChars(), mVersionString.GetChars(), mRetina?"retina":"");

  MountDeveloperImage();
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



