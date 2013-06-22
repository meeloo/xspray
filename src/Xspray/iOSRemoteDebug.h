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


typedef void (*AMinstall_callback)(CFDictionaryRef dict, int arg);
typedef void (*AMtransfer_callback)(CFDictionaryRef dict, int arg);
mach_error_t AMDeviceInstallApplication(service_conn_t sockInstallProxy, CFStringRef cfStrPath, CFDictionaryRef opts, AMinstall_callback callback, int* unknow);
mach_error_t AMDeviceTransferApplication(service_conn_t afcFd, CFStringRef path, CFDictionaryRef opts, AMtransfer_callback, int* unknown);

typedef struct am_device * AMDeviceRef;


int AMDeviceSecureTransferPath(int zero, AMDeviceRef device, CFURLRef url, CFDictionaryRef options, void *callback, int cbarg);
int AMDeviceSecureInstallApplication(int zero, AMDeviceRef device, CFURLRef url, CFDictionaryRef options, void *callback, int cbarg);
int AMDeviceMountImage(AMDeviceRef device, CFStringRef image, CFDictionaryRef options, void *callback, int cbarg);
int AMDeviceLookupApplications(AMDeviceRef device, int zero, CFDictionaryRef* result);


#ifdef __cplusplus
}
#endif
