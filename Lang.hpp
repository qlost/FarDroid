#ifndef lang_hpp
#define lang_hpp

enum
{
  MTitle,

	MOk,
  MCancel,
	MYes,
	MNo,
	MAlwaysYes,
	MAlwaysNo,
	MRetry,

  MConfTitle,
	MConfAddToDisk,
	MConfAddToDiskLabel,
  MConfPrefix,
	MConfSafeMode,
	MConfNative,
	MConfBusybox,
	MConfShowLinksAsDirs,
	MConfShowAllFilesystems,
	MConfUseSU,
	MConfUseExtendedAccess,
	MConfRemountSystem,
	MConfTimeout,
	MConfADBPath,
  MConfKillServer,
  MConfKillServerWarning,

	MError,
	MDeviceNotFound,
  MSelectDevice,
  MRenameDeviceName,

	MADBExecError,

	MFrom,
	MTo,
  MProgress,
  MTotal,
  MFiles,
  MBytes,

	MGetFile,
	MPutFile,
	MDelFile,
	MCreateDir,
  MMoveFile,
  MRenameFile,
  MScanDirectory,
  MScreenshot,
  MScreenshotComplete,

	MBreakWarn,

	MDirName,
	MCopyTitle,
	MCopyDest,
	MCopyDestName,
	MCopyWarnIfExists,
	MCopyError,
	MDeleteTitle,
	MDeleteWarn,
	MCopyDeleteError,

	MMemoryInfo,
	MPartitionsInfo,

	MWarningTitle,
	MOnlyNative,
	MPermChownSelected,
  MPermChmodSelected,
  MPermTitle,
  MPermChange,
  MPermPermissions,
  MPermOwner,
  MPermGroup,
  MPermType,
  MPermLink,
  MPermAll,
  MPermNone,

	MNeedFolderExePerm,
	MNeedSuperuserPerm, 
	MNeedNativeSuperuserPerm,
};

#endif