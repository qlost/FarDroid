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
  MRetryRoot,
  MSkip,

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
	MConfCopySD,
  MConfCopySDWarning,
  MConfRemountSystem,
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
  MCopyFile,
  MScanDirectory,
  MScreenshot,
  MScreenshotComplete,

	MBreakWarn,

	MDirName,
	MCopyTitle,
	MCopyDest,
  MRenameFileDest,
  MCopyFileDest,
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

	MNeedSuperuserPerm,
};

#endif