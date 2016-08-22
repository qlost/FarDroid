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

	MError,
	MDeviceNotFound,
  MSelectDevice,

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
	MSetPermFail,
	MPermTitle,
	MPermFileName,
	MPermFileAttr,

	MNeedFolderExePerm,
	MNeedSuperuserPerm, 
	MNeedNativeSuperuserPerm,
};

#endif