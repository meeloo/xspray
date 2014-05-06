/* -------------------------------------------------------- *
  *   MobileDevice.h - Interface to MobileDevice.framework   *
  *                  - Licensed under GNU Lesser GPL         *
  * -------------------------------------------------------- */
 
 #if !defined(MOBILEDEVICE_H)
 #define MOBILEDEVICE_H 1
     
 #if defined(_MSC_VER)
 #  pragma pack(push, 1)
 #  define __PACK
 #else
 #  define __PACK __attribute__((__packed__))
 #endif
 
 #if defined(WIN32)
 #  define __DLLIMPORT [DllImport("iTunesMobileDevice.dll")]
     using namespace System::Runtime::InteropServices;
 #  include <CoreFoundation.h>
     typedef unsigned int mach_error_t;
 #elif defined(__APPLE__)
 #  define __DLLIMPORT
 #  include <CoreFoundation/CoreFoundation.h>
 #  include <mach/error.h>
 #endif    
     
     /* Error codes */
 #define MDERR_APPLE_MOBILE  (err_system(0x3a))
 #define MDERR_IPHONE        (err_sub(0))
     
     /* Apple Mobile (AM*) errors */
 #define MDERR_OK                ERR_SUCCESS
 #define MDERR_SYSCALL           (ERR_MOBILE_DEVICE | 0x01)
 #define MDERR_OUT_OF_MEMORY     (ERR_MOBILE_DEVICE | 0x03)
 #define MDERR_QUERY_FAILED      (ERR_MOBILE_DEVICE | 0x04) 
 #define MDERR_INVALID_ARGUMENT  (ERR_MOBILE_DEVICE | 0x0b)
 #define MDERR_DICT_NOT_LOADED   (ERR_MOBILE_DEVICE | 0x25)
     
     /* Apple File Connection (AFC*) errors */
 #define MDERR_AFC_OUT_OF_MEMORY 0x03
     
     /* USBMux errors */
 #define MDERR_USBMUX_ARG_NULL   0x16
 #define MDERR_USBMUX_FAILED     0xffffffff
     
     /* Messages passed to device notification callbacks: passed as part of
      * AMDeviceNotificationCallbackInfo. */
 #define ADNCI_MSG_CONNECTED     1
 #define ADNCI_MSG_DISCONNECTED  2
 #define ADNCI_MSG_UNSUBSCRIBED  3
     
 #define AMD_IPHONE_PRODUCT_ID   0x1290
     //#define AMD_IPHONE_SERIAL       ""
     
     /* Services, found in /System/Library/Lockdown/Services.plist */
 #define AMSVC_AFC                   CFSTR("com.apple.afc")
 #define AMSVC_BACKUP                CFSTR("com.apple.mobilebackup")
 #define AMSVC_CRASH_REPORT_COPY     CFSTR("com.apple.crashreportcopy")
 #define AMSVC_DEBUG_IMAGE_MOUNT     CFSTR("com.apple.mobile.debug_image_mount")
 #define AMSVC_NOTIFICATION_PROXY    CFSTR("com.apple.mobile.notification_proxy")
 #define AMSVC_PURPLE_TEST           CFSTR("com.apple.purpletestr")
 #define AMSVC_SOFTWARE_UPDATE       CFSTR("com.apple.mobile.software_update")
 #define AMSVC_SYNC                  CFSTR("com.apple.mobilesync")
 #define AMSVC_SCREENSHOT            CFSTR("com.apple.screenshotr")
 #define AMSVC_SYSLOG_RELAY          CFSTR("com.apple.syslog_relay")
 #define AMSVC_SYSTEM_PROFILER       CFSTR("com.apple.mobile.system_profiler")
     
 CF_EXTERN_C_BEGIN
     
     typedef UInt32 AFCError;
     typedef UInt32 USBMuxError;
 
     /* Structure that contains internal data used by AMDevice... functions. Never try 
      * to access its members directly! Use AMDeviceCopyDeviceIdentifier, 
      * AMDeviceGetConnectionID, AMDeviceRetain, AMDeviceRelease instead. */
     struct AMDevice {
         CFRuntimeBase _uuid;          /* 0  */
         UInt32 _deviceID;           /* 16 */
         UInt32 _productID;          /* 20 - set to AMD_IPHONE_PRODUCT_ID */
         CFStringRef _serial;        /* 24 - serial string of device */
         UInt32 _unknown0;           /* 28 */
         UInt32 _unknown1;           /* 32 - reference counter, increased by AMDeviceRetain, decreased by AMDeviceRelease */
         UInt32 _lockdownConnection; /* 36 */
         UInt8 _unknown2[8];         /* 40 */
 #if (__ITUNES_VER > 740)
         UInt32 _unknown3;           /* 48 - used to store CriticalSection Handle*/
 #if (__ITUNES_VER >= 800)
         UInt8 _unknown4[24];        /* 52 */
 #endif
 #endif
     } __PACK;
     typedef struct __AMDevice AMDevice;
     typedef const AMDevice *AMDeviceRef;
     
     typedef void (*AMDeviceNotificationCallback)(AMDeviceCallbackInfoRef);
     
     struct __AMDeviceNotification {
         UInt32 _unknown0;                       /* 0  */
         UInt32 _unknown1;                       /* 4  */
         UInt32 _unknown2;                       /* 8  */
         AMDeviceNotificationCallback _callback; /* 12 */ 
         UInt32 _cookie;                         /* 16 */
     } __PACK;
     typedef struct __AMDeviceNotification AMDeviceNotification;
     typedef const AMDeviceNotification *AMDeviceNotificationRef;
     
     struct __AMDeviceCallbackInfo {
         AMDeviceRef _device; /* 0  */
         UInt32 _type;        /* 52 */
     } __PACK;
     typedef struct __AMDeviceCallbackInfo AMDeviceCallbackInfo;
     typedef AMDeviceCallbackInfo *AMDeviceCallbackInfoRef;
     
     struct __AMDeviceNotification {
         UInt32 _unknown0; /* 0  */
         UInt32 _unknown1; /* 4  */
         UInt32 _unknown2; /* 8  */
         void *callback;   /* 12 - TODO: fix type */
         UInt32 _unknown3; /* 16 */
     } __PACK;
     typedef struct __AMDeviceNotification AMDeviceNotification;
     typedef AMDeviceNotification *AMDeviceNotificationRef;
     
     struct __AMDeviceConnection {
         UInt8 _unknown[44]; /* 0 */
     } __PACK;
     typedef struct __AMDeviceConnection AMDeviceConnection;
     typedef AMDeviceConnection *AMDeviceConnectionRef;
     
     struct __AMDeviceNotificationCallbackInfo {
         AMDeviceRef _device;                   /* 0 - device */ 
         UInt32 _message;                       /* 4 - one of ADNCI_MSG_* */
         AMDeviceNotificationRef _subscription; /* 8 */
     } __PACK;
     typedef struct __AMDeviceNotificationCallbackInfo AMDeviceNotificationCallbackInfo;
     typedef AMDeviceNotificationCallbackInfo *AMDeviceNotificationCallbackInfoRef;
     
     /* The type of the device notification callback function. */
     typedef void (*AMDeviceNotificationCallback)(AMDeviceNotificationCallbackInfoRef, int _cookie);
     
     /* The type of the _AMDDeviceAttached function.
      * TODO: change to correct type. */
     typedef void *AMDDeviceAttatchedCallback;
     
     /* The type of the device restore notification callback functions.
      * TODO: change to correct type. */
     typedef void (*AMRestoreDeviceNotificationCallback)(AMRecoveryDeviceRef);
     
     /* This is a CoreFoundation object of class AMRecoveryModeDevice. */
     struct __AMRecoveryDevice {
         UInt8 _unknown0;                               /* 0  - sizeof(CFRuntimeBase) == 16 */
         AMRestoreDeviceNotificationCallback _callback; /* 8  */
         void *_userInfo;                               /* 12 */
         UInt8 unknown1[12];                            /* 16 */
         UInt32 _readWritePipe;                         /* 28 */
         UInt8 _readPipe;                               /* 32 */
         UInt8 _writeControlPipe;                       /* 33 */
         UInt8 _readUnknownPipe;                        /* 34 */
         UInt8 _writeFilePipe;                          /* 35 */
         UInt8 _writeInputPipe;                         /* 36 */
     } __PACK;
     typedef struct __AMRecoveryDevice AMRecoveryDevice;
     typedef const AMRecoveryDevice *AMRecoveryDeviceRef;
     
     /* A CoreFoundation object of class AMRestoreModeDevice. */
     struct __AMRestoreDevice {
         UInt8 _unknown0[8];  /* 0  - sizeof(CFRuntimeBase) == 16 */
         UInt8 _unknown1[24]; /* 8  */
         SInt32 _port;        /* 32 */
     } __PACK;
     typedef struct __AMRestoreDevice AMRestoreDevice;
     typedef const AMRestoreDevice *AMRestoreDeviceRef;
     
     /* The type of the device restore notification callback functions.
      * TODO: change to correct type. */
     typedef void (*AMRestoreDeviceNotificationCallback)(AMRecoveryDeviceRef);
     
     struct __AFCConnection {
         UInt32 _handle;          /* 0  */
         UInt32 _unknown0;        /* 4  */
         UInt8 _unknown1;         /* 8  */
         UInt8 _padding[3];       /* 9  */
         UInt32 _unknown2;        /* 12 */
         UInt32 _unknown3;        /* 16 */
         UInt32 _unknown4;        /* 20 */
         UInt32 _fsBlockSize;     /* 24 */
         UInt32 _socketBlockSize; /* 28 - always 0x3c */
         UInt32 _ioTimeout;       /* 32 - from AFCConnectionOpen, usu. 0 */
         void *_afcLock;          /* 36 */
         UInt32 _context;         /* 40 */
     } __PACK;
     typedef struct __AFCConnection AFCConnection;
     typedef const AFCConnection *AFCConnectionRef;
     
     struct __AFCDeviceInfo {
         UInt8 _unknown[12]; /* 0 */
     } __PACK;
     typedef struct __AFCDeviceInfo AFCDeviceInfo;
     typedef const AFCDeviceInfo *AFCDeviceInfoRef;
 
     struct __AFCDirectory {
         UInt8 _unknown[0]; /* 0 - size unknown */
     } __PACK;
     typedef struct __AFCDirectory AFCDirectory;
     typedef const AFCDirectory *AFCDirectoryRef;
 
     struct __AFCDictionary {
         UInt8 _unknown[0]; /* 0 - size unknown */
     } __PACK;
     typedef struct __AFCDictionary AFCDictionary;
     typedef const AFCDictionary *AFCDictionaryRef;
     
     typedef UInt64 AFCFileRef;
     
     struct __USBMuxListener1 { 
         UInt32 _unknown0;                     /* 0  - set to 1 */
         void *_unknown1;                      /* 4  */
         AMDDeviceAttatchedCallback _callback; /* 8  - set to _AMDDeviceAttached */
         UInt32 _unknown3;                     /* 12 */
         UInt32 _unknown4;                     /* 16 */
         UInt32 _unknown5;                     /* 20 */
     } __PACK;
     typedef struct __USBMuxListener1 USBMuxListener1;
     typedef const USBMuxListener1 *USBMuxListener1Ref;
     
     struct __USBMuxListener2 {
         UInt8 _unknown[48];  /* 0  */
         UInt8 _buffer[4096]; /* 48 */
     } __PACK;
     typedef struct __USBMuxListener2 USBMuxListener2;
     typedef const USBMuxListener2 *USBMuxListener2Ref;
     
     struct __AMBootloaderControlPacket {
         UInt8 _opcode;     /* 0 */
         UInt8 _length;     /* 1 */
         UInt8 _magic[2];   /* 2 - set to 0x34, 0x12 */
         UInt8 _payload[0]; /* 4 - size unknown */
     } __PACK;
     typedef struct __AMBootloaderControlPacket AMBootloaderControlPacket;
     typedef const AMBootloaderControlPacket *AMBootloaderControlPacketRef;
     
     /* ----------------------------------------------------------------------------
      *   Public routines
      * ------------------------------------------------------------------------- */
     
     /*  Registers a notification with the current run loop. The callback gets
      *  copied into the notification struct, as well as being registered with the
      *  current run loop. Cookie gets copied into cookie in the same.
      *  (Cookie is a user info parameter that gets passed as an arg to
      *  the callback) unused0 and unused1 are both 0 when iTunes calls this.
      *
      *  Never try to access directly or copy contents of dev and subscription fields 
      *  in am_device_notification_callback_info. Treat them as abstract handles. 
      *  When done with connection use AMDeviceRelease to free resources allocated for am_device.
      *  
      *  Returns:
      *      MDERR_OK            if successful
      *      MDERR_SYSCALL       if CFRunLoopAddSource() failed
      *      MDERR_OUT_OF_MEMORY if we ran out of memory
      */
     __DLLIMPORT mach_error_t AMDeviceNotificationSubscribe(AMDeviceNotificationCallback callback, 
                                                            UInt32 unused0,
                                                            UInt32 unused1, 
                                                            UInt32 cookie, 
                                                            AMDeviceNotificationRef *subscription);
     
 
     /*  Unregisters notifications. Buggy (iTunes 8.2): if you subscribe, unsubscribe and subscribe again, arriving 
      * notifications will contain cookie and subscription from 1st call to subscribe, not the 2nd one. iTunes 
      * calls this function only once on exit. */
     __DLLIMPORT mach_error_t AMDeviceNotificationUnsubscribe(AMDeviceNotificationRef subscription);
 
     /*  Returns _deviceID field of AMDevice structure
      */
     __DLLIMPORT UInt32 AMDeviceGetConnectionID(AMDeviceRef device);
 
     /*  Returns serial field of AMDevice structure
      */
     __DLLIMPORT CFStringRef AMDeviceCopyDeviceIdentifier(AMDeviceRef device);
 
     /*  Connects to the iPhone. Pass in the AMDevice structure that the
      *  notification callback will give to you.
      *
      *  Returns:
      *      MDERR_OK                if successfully connected
      *      MDERR_SYSCALL           if setsockopt() failed
      *      MDERR_QUERY_FAILED      if the daemon query failed
      *      MDERR_INVALID_ARGUMENT  if USBMuxConnectByPort returned 0xffffffff
      */
     __DLLIMPORT mach_error_t AMDeviceConnect(AMDeviceRef device);
     
     /*  Calls PairingRecordPath() on the given device, than tests whether the path
      *  which that function returns exists. During the initial connect, the path
      *  returned by that function is '/', and so this returns 1.
      *
      *  Returns:
      *      0   if the path did not exist
      *      1   if it did
      */
     __DLLIMPORT mach_error_t AMDeviceIsPaired(AMDeviceRef device);
     __DLLIMPORT mach_error_t AMDevicePair(AMDeviceRef device);
     
     /*  iTunes calls this function immediately after testing whether the device is
      *  paired. It creates a pairing file and establishes a Lockdown connection.
      *
      *  Returns:
      *      MDERR_OK                if successful
      *      MDERR_INVALID_ARGUMENT  if the supplied device is null
      *      MDERR_DICT_NOT_LOADED   if the load_dict() call failed
      */
     __DLLIMPORT mach_error_t AMDeviceValidatePairing(AMDeviceRef device);
     
     /*  Creates a Lockdown session and adjusts the device structure appropriately
      *  to indicate that the session has been started. iTunes calls this function
      *  after validating pairing.
      *
      *  Returns:
      *      MDERR_OK                if successful
      *      MDERR_INVALID_ARGUMENT  if the Lockdown conn has not been established
      *      MDERR_DICT_NOT_LOADED   if the load_dict() call failed
      */
     __DLLIMPORT mach_error_t AMDeviceStartSession(AMDeviceRef device);
 
 
     /* Reads various device settings. One of domain or cfstring arguments should be NULL.
      *
      * ActivationPublicKey 
      * ActivationState 
      * ActivationStateAcknowledged 
      * ActivityURL 
      * BasebandBootloaderVersion 
      * BasebandSerialNumber 
      * BasebandStatus 
      * BasebandVersion 
      * BluetoothAddress 
      * BuildVersion 
      * CPUArchitecture 
      * DeviceCertificate 
      * DeviceClass 
      * DeviceColor 
      * DeviceName 
      * DevicePublicKey 
      * DieID 
      * FirmwareVersion 
      * HardwareModel 
      * HardwarePlatform 
      * HostAttached 
      * IMLockdownEverRegisteredKey 
      * IntegratedCircuitCardIdentity 
      * InternationalMobileEquipmentIdentity 
      * InternationalMobileSubscriberIdentity 
      * iTunesHasConnected 
      * MLBSerialNumber 
      * MobileSubscriberCountryCode 
      * MobileSubscriberNetworkCode 
      * ModelNumber 
      * PartitionType 
      * PasswordProtected 
      * PhoneNumber 
      * ProductionSOC 
      * ProductType 
      * ProductVersion 
      * ProtocolVersion 
      * ProximitySensorCalibration 
      * RegionInfo 
      * SBLockdownEverRegisteredKey 
      * SerialNumber 
      * SIMStatus 
      * SoftwareBehavior 
      * SoftwareBundleVersion 
      * SupportedDeviceFamilies 
      * TelephonyCapability 
      * TimeIntervalSince1970 
      * TimeZone 
      * TimeZoneOffsetFromUTC 
      * TrustedHostAttached 
      * UniqueChipID 
      * UniqueDeviceID 
      * UseActivityURL 
      * UseRaptorCerts 
      * Uses24HourClock 
      * WeDelivered 
      * WiFiAddress 
      * // Updated by DiAifU 14 Oct 2012 for iOS5 and iTunes 5.0
      *
      * Possible values for domain:
      * com.apple.mobile.battery
      */
     __DLLIMPORT CFStringRef AMDeviceCopyValue(AMDeviceRef device,
                                               CFStringRef domain,
                                               CFStringRef str);
     
     /* Starts a service and returns a socket file descriptor that can be used in order to further
      * access the service. You should stop the session and disconnect before using
      * the service. iTunes calls this function after starting a session. It starts 
      * the service and the SSL connection. service_name should be one of the AMSVC_*
      * constants.
      *
      * Returns:
      *      MDERR_OK                if successful
      *      MDERR_SYSCALL           if the setsockopt() call failed
      *      MDERR_INVALID_ARGUMENT  if the Lockdown conn has not been established
      */
     __DLLIMPORT mach_error_t AMDeviceStartService(AMDeviceRef device,
                                                   CFStringRef sericeName,
                                                   SInt32 *sockedFD);
     
     /* Stops a session. You should do this before accessing services.
      *
      * Returns:
      *      MDERR_OK                if successful
      *      MDERR_INVALID_ARGUMENT  if the Lockdown conn has not been established
      */
     __DLLIMPORT mach_error_t AMDeviceStopSession(AMDeviceRef device);
     
     /* Decrements reference counter and, if nothing left, releases resources hold 
      * by connection, invalidates  pointer to device
      */
     __DLLIMPORT void AMDeviceRelease(AMDeviceRef device);
 
     /* Increments reference counter
      */
     __DLLIMPORT void AMDeviceRetain(AMDeviceRef device);
 
     /* Opens an Apple File Connection. You must start the appropriate service
      * first with AMDeviceStartService(). In iTunes, io_timeout is 0.
      *
      * Returns:
      *      MDERR_OK                if successful
      *      MDERR_AFC_OUT_OF_MEMORY if malloc() failed
      */
     __DLLIMPORT AFCError AFCConnectionOpen(SInt32 socketFD,
                                            UInt32 ioTimeout,
                                            AFCConnectionRef *connection);
 
 
     /* Copy an environment variable value from iBoot
      */
     __DLLIMPORT CFStringRef AMRecoveryModeCopyEnvironmentVariable(AMRecoveryDeviceRef rdev,
                                                                   CFStringRef str);
     
     /* Pass in a pointer to an afc_dictionary structure. It will be filled. You can
      * iterate it using AFCKeyValueRead. When done use AFCKeyValueClose. Possible keys:
      * FSFreeBytes - free bytes on system device for afc2, user device for afc
      * FSBlockSize - filesystem block size
      * FSTotalBytes - size of device
      * Model - iPhone1,1 etc.
 
      */
     __DLLIMPORT AFCError AFCDeviceInfoOpen(AFCConnectionRef connection,
                                            AFCDictionaryRef *info);
     
     /* Turns debug mode on if the environment variable AFCDEBUG is set to a numeric
      * value, or if the file '/AFCDEBUG' is present and contains a value. */
 #if defined(__APPLE__)
     void AFCPlatformInitialize();
 #endif
     
     /* Opens a directory on the iPhone. Pass in a pointer in dir to be filled in.
      * Note that this normally only accesses the iTunes [[sandbox]]/partition as the
      * root, which is /var/root/Media. Pathnames are specified with '/' delimiters
      * as in Unix style. Use UTF-8 to specify non-ASCII symbols in path.
      *
      * Returns:
      *      MDERR_OK                if successful
      */
     __DLLIMPORT AFCError AFCDirectoryOpen(AFCConnectionRef connection,
                                           StringPtr path,
                                           AFCDirectoryRef *directory);
     
     /* I'm not completely sure they are char pointers
      * as Apple always uses CFStringRef */
     
     /* Acquires the next entry in a directory previously opened with
      * AFCDirectoryOpen(). When dirent is filled with a NULL value, then the end
      * of the directory has been reached. '.' and '..' will be returned as the
      * first two entries in each directory except the root; you may want to skip
      * over them.
      *
      * Returns:
      *      MDERR_OK                if successful, even if no entries remain
      */
     __DLLIMPORT AFCError AFCDirectoryRead(AFCConnectionRef connection,
                                           AFCDirectoryRef directory,
                                           StringPtr *dirent);
     __DLLIMPORT AFCError AFCDirectoryClose(AFCConnectionRef connection,
                                            AFCDirectoryRef directory);
     __DLLIMPORT AFCError AFCDirectoryCreate(AFCConnectionRef connection,
                                             StringPtr directory);
     __DLLIMPORT AFCError AFCRemovePath(AFCConnectionRef connection,
                                        StringPtr directory);
     __DLLIMPORT AFCError AFCRenamePath(AFCConnectionRef connection,
                                        StringPtr oldPath,
                                        StringPtr newPath);
     
 #if (__ITUNES_VER >= 800)
     /* Creates symbolic or hard link
      * linktype - int64: 1 means hard link, 2 - soft (symbolic) link
      * target - absolute or relative path to link target
      * linkname - absolute path where to create new link
      */
     __DLLIMPORT AFCError AFCLinkPath(AFCConnectionRef connection,
                                      SInt64 linkType,
                                      StringPtr target, 
                                      StringPtr linkName);
 
 #endif
     /* Opens file for reading or writing without locking it in any way. AFCFileRef should not be shared between threads - 
      * opening file in one thread and closing it in another will lead to possible crash.
      * path - UTF-8 encoded absolute path to file
      * mode - read (2); write (3); unknown (0)
      * fileRef - receives file handle
      */
     __DLLIMPORT AFCError AFCFileRefOpen(AFCConnectionRef connection,
                                         StringPtr path,
                                         UInt64 mode,
                                         AFCFileRef *fileRef);
     /* Reads specified amount (len) of bytes from file into buf. Puts actual count of read bytes into len on return
      */
     __DLLIMPORT AFCError AFCFileRefRead(AFCConnectionRef connection,
                                         AFCFileRef fileRef,
                                         void *buffer,
                                         UInt32 *length);
     /* Writes specified amount (len) of bytes from buf into file.
      */
     __DLLIMPORT AFCError AFCFileRefWrite(AFCConnectionRef connection,
                                          AFCFileRef ref,
                                          void *buffer,
                                          UInt32 length);
     /* Moves the file pointer to a specified location.
      * offset - Number of bytes from origin (int64)
      * origin - 0 = from beginning, 1 = from current position, 2 = from end
      */
     __DLLIMPORT AFCError AFCFileRefSeek(AFCConnectionRef connection,
                                         AFCFileRef ref,
                                         CFIndex offset,
                                         SInt32 origin,
                                         SInt32 unused);
 
     /* Gets the current position of a file pointer into offset argument.
      */
     __DLLIMPORT AFCError AFCFileRefTell(AFCConnectionRef connection,
                                         AFCFileRef ref,
                                         CFIndex *offset);
 
     /*  Truncates a file at the specified offset.
      */
     __DLLIMPORT AFCError AFCFileRefSetFileSize(AFCConnectionRef connection,
                                                AFCFileRef ref,
                                                CFIndex offset);
 
 
     __DLLIMPORT AFCError AFCFileRefLock(AFCConnectionRef connection,
                                         AFCFileRef ref);
     __DLLIMPORT AFCError AFCFileRefUnlock(AFCConnectionRef connection,
                                           AFCFileRef ref);
     __DLLIMPORT AFCError AFCFileRefClose(AFCConnectionRef connection,
                                          AFCFileRef ref);
 
     /* Opens dictionary describing specified file or directory (iTunes below 8.2 allowed using AFCGetFileInfo
        to get the same information)
     */
     __DLLIMPORT AFCError AFCFileInfoOpen(AFCConnectionRef connection,
                                          StringPtr path,
                                          AFCDictionaryRef *info);
 
     /* Reads next entry from dictionary. When last entry is read, function returns NULL in key argument
        Possible keys:
          "st_size":     val - size in bytes
          "st_blocks":   val - size in blocks
          "st_nlink":    val - number of hardlinks
          "st_ifmt":     val - "S_IFDIR" for folders
                             "S_IFLNK" for symlinks
          "LinkTarget":  val - path to symlink target
     */
     __DLLIMPORT AFCError AFCKeyValueRead(AFCDictionaryRef dictionary,
                                          StringPtr *key,
                                          StringPtr *val);
     /* Closes dictionary
     */
     __DLLIMPORT AFCError AFCKeyValueClose(AFCDictionaryRef dictionary);
 
     
     /* Returns the context field of the given AFC connection. */
     __DLLIMPORT unsigned int AFCConnectionGetContext(AFCConnectionRef connection);
     
     /* Returns the fs_block_size field of the given AFC connection. */
     __DLLIMPORT unsigned int AFCConnectionGetFSBlockSize(AFCConnectionRef connection);
     
     /* Returns the io_timeout field of the given AFC connection. In iTunes this is
      * 0. */
     __DLLIMPORT unsigned int AFCConnectionGetIOTimeout(AFCConnectionRef connection);
     
     /* Returns the sock_block_size field of the given AFC connection. */
     __DLLIMPORT unsigned int AFCConnectionGetSocketBlockSize(AFCConnectionRef connection);
     
     /* Closes the given AFC connection. */
     __DLLIMPORT AFCError AFCConnectionClose(AFCConnectionRef connection);
     
     /* Registers for device notifications related to the restore process. unknown0
      * is zero when iTunes calls this. In iTunes,
      * the callbacks are located at:
      *      1: $3ac68e-$3ac6b1, calls $3ac542(unknown1, arg, 0)
      *      2: $3ac66a-$3ac68d, calls $3ac542(unknown1, 0, arg)
      *      3: $3ac762-$3ac785, calls $3ac6b2(unknown1, arg, 0)
      *      4: $3ac73e-$3ac761, calls $3ac6b2(unknown1, 0, arg)
      */
     __DLLIMPORT UInt32 AMRestoreRegisterForDeviceNotifications(AMRestoreDeviceNotificationCallback dfuConnectCallback,
                                                                AMRestoreDeviceNotificationCallback recoveryConnectCallback,
                                                                AMRestoreDeviceNotificationCallback dfuDisconectCallback,
                                                                AMRestoreDeviceNotificationCallback recoveryDisconnectCallback,
                                                                UInt32 unknown0,
                                                                void *userInfo);
     
     /* Causes the restore functions to spit out (unhelpful) progress messages to
      * the file specified by the given path. iTunes always calls this right before
      * restoring with a path of
      * "$HOME/Library/Logs/iPhone Updater Logs/iPhoneUpdater X.log", where X is an
      * unused number.
      */
     __DLLIMPORT UInt32 AMRestoreEnableFileLogging(StringPtr path);
     
     /* Initializes a new option dictionary to default values. Pass the constant
      * kCFAllocatorDefault as the allocator. The option dictionary looks as
      * follows:
      * {
      *      NORImageType => 'production',
      *      AutoBootDelay => 0,
      *      KernelCacheType => 'Release',
      *      UpdateBaseband => true,
      *      DFUFileType => 'RELEASE',
      *      SystemImageType => 'User',
      *      CreateFilesystemPartitions => true,
      *      FlashNOR => true,
      *      RestoreBootArgs => 'rd=md0 nand-enable-reformat=1 -progress'
      *      BootImageType => 'User'
      *  }
      *
      * Returns:
      *      the option dictionary   if successful
      *      NULL                    if out of memory
      */ 
     __DLLIMPORT CFMutableDictionaryRef AMRestoreCreateDefaultOptions(CFAllocatorRef allocator);
     
     /* ----------------------------------------------------------------------------
      *   Less-documented public routines
      * ------------------------------------------------------------------------- */
     
     __DLLIMPORT UInt32 AMRestorePerformRecoveryModeRestore(AMRecoveryDeviceRef rdev,
                                                            CFDictionaryRef options,
                                                            void *callback,
                                                            void *userInfo);
     __DLLIMPORT UInt32 AMRestorePerformRestoreModeRestore(AMRecoveryDeviceRef rdev,
                                                           CFDictionaryRef options,
                                                           void *callback,
                                                           void *userInfo);
     __DLLIMPORT AMRestoreDeviceRef AMRestoreModeDeviceCreate(UInt32 unknown0,
                                                              UInt32 connectionID,
                                                              UInt32 unknown1);
     __DLLIMPORT UInt32 AMRestoreCreatePathsForBundle(CFStringRef restoreBundleType,
                                                      CFStringRef kernelCacheType,
                                                      CFStringRef bootImageType,
                                                      UInt32 unknown0,
                                                      CFStringRef *firwareDirectory,
                                                      CFStringRef *kernelCacheRestorePath,
                                                      UInt32 unknown1,
                                                      CFStringRef *ramdiskPath);
     __DLLIMPORT UInt32 AMRestoreModeDeviceReboot(AMRestoreDeviceRef rdev);
     __DLLIMPORT mach_error_t AMDeviceEnterRecovery(AMDeviceRef device);
     __DLLIMPORT mach_error_t AMDeviceDisconnect(AMDeviceRef device);
 
 
     /* to use this, start the service "com.apple.mobile.notification_proxy", handle will be the socket to use */
     typedef void (*AMDeviceNotifyCallback)(CFStringRef notification, USERDATA data);
     __DLLIMPORT mach_error_t AMDPostNotification(SOCKET socket,
                                                  CFStringRef notification,
                                                  CFStringRef userinfo);
     __DLLIMPORT mach_error_t AMDObserveNotification(SOCKET socket,
                                                     CFStringRef notification);
     __DLLIMPORT mach_error_t AMDListenForNotifications(SOCKET socket,
                                                        AMDeviceNotifyCallback callback,
                                                        USERDATA userData);
     __DLLIMPORT mach_error_t AMDShutdownNotificationProxy(SOCKET socket);
     
     __DLLIMPORT mach_error_t AMDeviceDeactivate(AMDeviceRef device);
     __DLLIMPORT mach_error_t AMDeviceActivate(AMDeviceRef device,
                                               CFDictionaryRef dict);
     __DLLIMPORT mach_error_t AMDeviceRemoveValue(AMDeviceRef device,
                                                  UInt32 unknown,
                                                  CFStringRef str);
     
     /* ----------------------------------------------------------------------------
      *   Semi-private routines
      * ------------------------------------------------------------------------- */
     
     /*  Pass in a USBMuxListener1 structure and a USBMuxListener2 structure
      *  pointer, which will be filled with the resulting USBMuxListener2.
      *
      *  Returns:
      *      MDERR_OK                if completed successfully
      *      MDERR_USBMUX_ARG_NULL   if one of the arguments was NULL
      *      MDERR_USBMUX_FAILED     if the listener was not created successfully
      */
     __DLLIMPORT USBMuxError USBMuxListenerCreate(USBMuxListener1Ref source,
                                                  USBMuxListener2Ref *destination);
     
     /* ----------------------------------------------------------------------------
      *   Less-documented semi-private routines
      * ------------------------------------------------------------------------- */
     __DLLIMPORT USBMuxError USBMuxListenerHandleData(void *unknown);
     
     /* ----------------------------------------------------------------------------
      *   Private routines - no exports means no function declarations
      * ------------------------------------------------------------------------- */
     
     /* AMRestorePerformRestoreModeRestore() calls this function with a dictionary
      * in order to perform certain special restore operations
      * (RESTORED_OPERATION_*). It is thought that this function might enable
      * significant access to the phone. */
     
     /*
      typedef unsigned int (*t_performOperation)(AMRestoreDeviceRef device, CFDictionaryRef operation) __attribute__((regparm(2)));
      t_performOperation _performOperation = (t_performOperation)0x3c39fa4b;
      */ 
     
     /* ----------------------------------------------------------------------------
      *   Less-documented private routines
      * ------------------------------------------------------------------------- */
     
     
     /*
      typedef SInt32 (*t_socketForPort)(struct AMRestoreDevice *device, unsigned int port) __attribute__((regparm(2)));
      t_socketForPort _socketForPort = (t_socketForPort)0x3c39f36c;
      
      typedef void (*t_restored_send_message)(int port, CFDictionaryRef msg);
      t_restored_send_message _restored_send_message = (t_restored_send_message)0x3c3a4e40;
      
      typedef CFDictionaryRef (*t_restored_receive_message)(int port);
      t_restored_receive_message _restored_receive_message = (t_restored_receive_message)0x3c3a4d40;
      
      typedef UInt32 (*t_sendControlPacket)(struct AMRecoveryDevice *device, UInt32 message1, UInt32 message2, UInt32 unknown0, UInt32 *unknown1, UInt8 *unknown2) __attribute__((regparm(3)));
      t_sendControlPacket _sendControlPacket = (t_sendControlPacket)0x3c3a3da3;;
      
      typedef UInt32 (*t_sendCommandToDevice)(struct AMRecoveryDevice *device, CFStringRef command) __attribute__((regparm(2)));
      t_sendCommandToDevice _sendCommandToDevice = (t_sendCommandToDevice)0x3c3a3e3b;
      
      typedef UInt32 (*t_AMRUSBInterfaceReadPipe)(UInt32 readwrite_pipe, UInt32 read_pipe, UInt8 *buffer, UInt32 *length);
      t_AMRUSBInterfaceReadPipe _AMRUSBInterfaceReadPipe = (t_AMRUSBInterfaceReadPipe)0x3c3a27e8;
      
      typedef UInt32 (*t_AMRUSBInterfaceWritePipe)(UInt32 readWritePipe, UInt32 writePipe, void *data, UInt32 length);
      t_AMRUSBInterfaceWritePipe _AMRUSBInterfaceWritePipe = (t_AMRUSBInterfaceWritePipe)0x3c3a27cb;
      */
     
     SInt32 performOperation(AMRestoreDeviceRef device,
                             CFMutableDictionaryRef message);
     SInt32 socketForPort(AMRestoreDeviceRef device,
                          UInt32 portnum);
     SInt32 sendCommandToDevice(AMRestoreDeviceRef device,
                                CFStringRef command,
                                int block);
     SInt32 sendFileToDevice(AMRestoreDeviceRef device,
                             CFStringRef fileName);
 
 #if defined(_MSC_VER)
 #  pragma pack(pop)
 #endif
 #undef __PACK

 CF_EXTERN_C_END
 
 #endif