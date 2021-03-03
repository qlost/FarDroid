#include "stdafx.h"
#include "fardroidPlugin.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    hInst = hModule;
    DisableThreadLibraryCalls(hInst);
  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
    break;
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

void WINAPI GetGlobalInfoW(struct GlobalInfo* Info)
{
  Info->StructSize = sizeof(struct GlobalInfo);
  Info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 3000, VS_RELEASE);
  Info->Version = MAKEFARVERSION(MAJORVERSION, MINORVERSION, REVISION, BUILDNUMBER, VS_RELEASE);
  Info->Guid = MainGuid;
  Info->Title = L"FARDroid";
  Info->Description = L"FARDroid FAR Plugin";
  Info->Author = L"Vladimir Kubyshev, dimfish";
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo* Info)
{
  if (Info->StructSize >= sizeof(struct PluginStartupInfo))
  {
    fInfo = *Info;
    FSF = *Info->FSF;
    fInfo.FSF = &FSF;
    conf.Load();
  }
}

void WINAPI GetPluginInfoW(struct PluginInfo* Info)
{
  Info->StructSize = sizeof(PluginInfo);
  Info->Flags = 0;

  static const wchar_t *PluginConfigMenuStrings[1], *PluginMenuStrings[1];

  PluginConfigMenuStrings[0] = LOC(MTitle);
  Info->PluginConfig.Strings = PluginConfigMenuStrings;
  Info->PluginConfig.Count = 1;
  Info->PluginConfig.Guids = &MenuGuid;

  PluginMenuStrings[0] = LOC(MTitle);
  Info->PluginMenu.Strings = PluginMenuStrings;
  Info->PluginMenu.Count = 1;
  Info->PluginMenu.Guids = &MenuGuid;

  Info->CommandPrefix = _C(conf.Prefix);

  static const wchar_t* DiskMenuString[1];

  DiskMenuString[0] = LOC(MTitle);
  if (conf.AddToDiskMenu)
  {
    Info->DiskMenu.Strings = DiskMenuString;
    Info->DiskMenu.Count = 1;
    Info->DiskMenu.Guids = &MenuGuid;
  }
  else
  {
    Info->DiskMenu.Strings = nullptr;
    Info->DiskMenu.Count = 0;
    Info->DiskMenu.Guids = nullptr;
  }
}

HANDLE WINAPI OpenW(const struct OpenInfo* Info)
{
  if (!hRegexpDate1)
    hRegexpDate1 = RegexpMake(_T("/(\\d{4}-\\d{2}-\\d{2})\\s+(\\d{2}:\\d{2})/"));
  if (!hRegexpDate2)
    hRegexpDate2 = RegexpMake(_T("/(\\w{3})\\s+(\\d+)\\s+(\\d{4})/"));
  if (!hRegexpDate3)
    hRegexpDate3 = RegexpMake(_T("/(\\w{3})\\s+(\\d+)\\s+(\\d{2}:\\d{2})/"));
  if (!hRegexpProp)
    hRegexpProp = RegexpMake(_T("/^\\[([\\w.]+)\\]:\\s*\\[(.*)\\]$/"));
  if (!hRegexpSize)
    hRegexpSize = RegexpMake(_T("/([\\d.]+)(.*)/"));
  if (!hRegexpMem)
    hRegexpMem = RegexpMake(_T("/(\\w+):\\s+(.+)$/"));
  if (!hRegexpPart1)
    hRegexpPart1 = RegexpMake(_T("/(.*(?=:)):\\W+(\\w+(?=\\stotal)).+,\\s(\\w+(?=\\savailable))/"));
  if (!hRegexpPart2)
    hRegexpPart2 = RegexpMake(_T("/^(\\S+)\\s+([\\d.]+\\S*)\\s+([\\d.]+\\S*)\\s+([\\d.]+\\S*)\\s+(?:[\\d.]+%\\s+)?(\\S+)/"));

  if (!hRegexpFile[0][0]) {
    static const char *rx[2][2] = {
      {"(\\d+)?", "(\\d+,\\s*\\d+)"},//regexpSize = isDevice ?
      {"(.+)", "(.+(?=\\s->))\\s->\\s(.+)"}//regexpFile = isLink ?
    };
    static const CString regexpBase = "/^([\\w-]{10})\\s+(?:\\d+\\s+)?(\\w+)\\s+(\\w+)\\s+%S\\s+(\\d{4}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}|\\w{3}\\s+\\d+\\s+\\d{4}|\\w{3}\\s+\\d+\\s+\\d{2}:\\d{2})\\s%S$/";
    CString regex;

    for (int rxSize = 0; rxSize < 2; rxSize++) {
      for (int rxFile = 0; rxFile < 2; rxFile++) {
        regex.Format(regexpBase, rx[0][rxSize], rx[1][rxFile]);
        hRegexpFile[rxSize][rxFile] = RegexpMake((LPTSTR)(LPCTSTR)regex);
      }
    }
  }

  switch (Info->OpenFrom)
  {
  case OPEN_LEFTDISKMENU:
  case OPEN_RIGHTDISKMENU:
  case OPEN_PLUGINSMENU:
  {
    auto android = new fardroid();
    auto res = android->OpenFromMainMenu();
    if (res == INVALID_HANDLE_VALUE)
      delete android;

    return res;
  }
  case OPEN_COMMANDLINE:
  {
    auto android = new fardroid();
    CString cmd = reinterpret_cast<OpenCommandLineInfo*>(Info->Data)->CommandLine;
    auto res = android->OpenFromCommandLine(cmd);
    if (res == INVALID_HANDLE_VALUE)
      delete android;

    return res;
  }
  default:
    break;
  }
  return INVALID_HANDLE_VALUE;
}

void WINAPI ExitFARW(const struct ExitInfo*)
{
  for (int rxSize = 0; rxSize < 2; rxSize++)
    for (int rxFile = 0; rxFile < 2; rxFile++)
      RegexpFree(hRegexpFile[rxSize][rxFile]);

  RegexpFree(hRegexpPart2);
  RegexpFree(hRegexpPart1);
  RegexpFree(hRegexpMem);
  RegexpFree(hRegexpSize);
  RegexpFree(hRegexpProp);
  RegexpFree(hRegexpDate3);
  RegexpFree(hRegexpDate2);
  RegexpFree(hRegexpDate1);
}

void WINAPI ClosePanelW(const struct ClosePanelInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return;
  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  delete android;
}

intptr_t WINAPI ConfigDlgProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  if (Msg == DN_CTLCOLORDLGITEM)
  {
    if (Param1 == ID_KillServerWarning && GetChecked(hDlg, ID_KillServer) ||
      Param1 == ID_CopySDWarning && GetChecked(hDlg, ID_CopySD))
    {
      auto color = static_cast<FarDialogItemColors*>(Param2);
      if (color) color->Colors[0].ForegroundColor = 0x4;
    }
  }
  else if (Msg == DN_BTNCLICK)
    switch (Param1)
    {
    case ID_WorkModeBB:
      fInfo.SendDlgMessage(hDlg, DM_ENABLE, ID_ShowLinksAsDir, Param2);
      break;
    }

  return fInfo.DefDlgProc(hDlg, Msg, Param1, Param2);
}

intptr_t WINAPI ConfigureW(const struct ConfigureInfo* Info)
{
  const auto width = 55;
  struct InitDialogItem InitItems[] = {
    /*00*/FDI_DOUBLEBOX(width - 4,20,(farStr *)LOC(MConfTitle)),
    /*01*/FDI_CHECK(5, 2,(farStr *)LOC(MConfAddToDisk)),
    /*02*/FDI_LABEL(5, 3, (farStr *)LOC(MConfPrefix)),
    /*03*/FDI_EDIT(13, 3, 28, _F("fardroidPrefix")),
    /*04*/FDI_RADIO(5, 5, (farStr *)LOC(MConfSafeMode)),
    /*05*/FDI_RADIO(5, 6, (farStr *)LOC(MConfNative)),
    /*06*/FDI_RADIO(5, 7, (farStr *)LOC(MConfBusybox)),
    /*07*/FDI_CHECK(9, 8, (farStr *)LOC(MConfShowLinksAsDirs)),
    /*08*/FDI_CHECK(5,10, (farStr *)LOC(MConfUseSU)),
    /*09*/FDI_CHECK(9,11, (farStr *)LOC(MConfCopySD)),
    /*10*/FDI_LABEL(13,12, (farStr *)LOC(MConfCopySDWarning)),
    /*11*/FDI_CHECK(5,13, (farStr *)LOC(MConfRemountSystem)),
    /*12*/FDI_LABEL(5,15, (farStr *)LOC(MConfADBPath)),
    /*13*/FDI_EDIT(18,15, 48, _F("fardroidADBPath")),
    /*14*/FDI_CHECK(5,16, (farStr *)LOC(MConfKillServer)),
    /*15*/FDI_LABEL(9,17,(farStr *)LOC(MConfKillServerWarning)),
    /*16*/FDI_DEFCBUTTON(19,(farStr *)LOC(MOk)),
    /*17*/FDI_CBUTTON(19,(farStr *)LOC(MCancel)),
    /*--*/FDI_SEPARATOR(4,_F("")),
    /*--*/FDI_SEPARATOR(9,_F("")),
    /*--*/FDI_SEPARATOR(14,_F("")),
    /*--*/FDI_SEPARATOR(18,_F("")),
  };
  const int size = sizeof InitItems / sizeof InitItems[0];

  FarDialogItem DialogItems[size];
  InitDialogItems(InitItems, DialogItems, size);

  DialogItems[ID_AddToDiskMenu].Selected = conf.AddToDiskMenu;

  auto editbuf1 = static_cast<wchar_t *>(my_malloc(1024));
  lstrcpyW(editbuf1, conf.Prefix);
  DialogItems[ID_Prefix].Data = editbuf1;

  DialogItems[ID_WorkModeSafe].Selected = conf.WorkMode == WORKMODE_SAFE;
  DialogItems[ID_WorkModeNative].Selected = conf.WorkMode == WORKMODE_NATIVE;
  DialogItems[ID_WorkModeBB].Selected = conf.WorkMode == WORKMODE_BUSYBOX;
  DialogItems[ID_ShowLinksAsDir].Selected = conf.ShowLinksAsDir;
  DialogItems[ID_UseSU].Selected = conf.UseSU;
  DialogItems[ID_CopySD].Selected = conf.CopySD;
  DialogItems[ID_RemountSystem].Selected = conf.RemountSystem;
  DialogItems[ID_KillServer].Selected = conf.KillServer;

  if (conf.WorkMode != WORKMODE_BUSYBOX)
    DialogItems[ID_ShowLinksAsDir].Flags |= DIF_DISABLE;

  auto editbuf2 = static_cast<wchar_t *>(my_malloc(1024));
  lstrcpyW(editbuf2, conf.ADBPath);
  DialogItems[ID_ADBPath].Data = editbuf2;

  HANDLE hdlg = fInfo.DialogInit(&MainGuid, &DialogGuid, -1, -1, width, 22, _F("Config"), DialogItems, size, 0, 0, ConfigDlgProc, nullptr);
  BOOL result = static_cast<int>(fInfo.DialogRun(hdlg)) == ID_Ok;
  if (result)
  {
    if (GetItemSelected(hdlg, ID_WorkModeSafe))
      conf.WorkMode = WORKMODE_SAFE;
    else if (GetItemSelected(hdlg, ID_WorkModeNative))
      conf.WorkMode = WORKMODE_NATIVE;
    else if (GetItemSelected(hdlg, ID_WorkModeBB))
      conf.WorkMode = WORKMODE_BUSYBOX;

    conf.AddToDiskMenu = GetItemSelected(hdlg, ID_AddToDiskMenu);
    conf.Prefix = GetItemData(hdlg, ID_Prefix);
    conf.ShowLinksAsDir = GetItemSelected(hdlg, ID_ShowLinksAsDir);
    conf.UseSU = GetItemSelected(hdlg, ID_UseSU);
    conf.CopySD = GetItemSelected(hdlg, ID_CopySD);
    conf.RemountSystem = GetItemSelected(hdlg, ID_RemountSystem);
    conf.ADBPath = GetItemData(hdlg, ID_ADBPath);
    conf.KillServer = GetItemSelected(hdlg, ID_KillServer);

    conf.Save();

    PanelInfo PInfo;
    PInfo.StructSize = sizeof(PanelInfo);
    fardroid* android = nullptr;
    fInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, static_cast<void *>(&PInfo));
    if (PInfo.PluginHandle && PInfo.PluginHandle != INVALID_HANDLE_VALUE && PInfo.OwnerGuid == MainGuid)
      android = static_cast<fardroid *>(PInfo.PluginHandle);

    if (!android)
    {
      fInfo.PanelControl(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, static_cast<void *>(&PInfo));
      if (PInfo.PluginHandle && PInfo.PluginHandle != INVALID_HANDLE_VALUE && PInfo.OwnerGuid == MainGuid)
        android = static_cast<fardroid *>(PInfo.PluginHandle);
    }

    if (android)
    {
      android->Reread();
      fInfo.PanelControl(PInfo.PluginHandle, FCTL_UPDATEPANEL, 1, nullptr);
      fInfo.PanelControl(PInfo.PluginHandle, FCTL_REDRAWPANEL, 0, nullptr);
    }
  }

  fInfo.DialogFree(hdlg);
  my_free(editbuf1);
  my_free(editbuf2);

  return result;
}

void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  Info->StructSize = sizeof *Info;
  Info->Flags = OPIF_SHOWPRESERVECASE | OPIF_USEFREESIZE;

  Info->Format = LOC(MTitle);

  android->PreparePanel(Info);

  Info->DescrFiles = nullptr;
  Info->DescrFilesNumber = 0;

  Info->StartPanelMode = _F('0') + conf.PanelMode;
  Info->StartSortMode = static_cast<OPENPANELINFO_SORTMODES>(conf.SortMode);
  Info->StartSortOrder = conf.SortOrder;

  Info->PanelModesArray = nullptr;
  Info->PanelModesNumber = 0;

  Info->KeyBar = &KeyBar;
}

intptr_t WINAPI GetFindDataW(struct GetFindDataInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  return android->GetFindData(&Info->PanelItem, &Info->ItemsNumber, Info->OpMode);
}

void WINAPI FreeFindDataW(const struct FreeFindDataInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  android->FreeFindData(Info->PanelItem, static_cast<int>(Info->ItemsNumber));
}

intptr_t WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);

  if (Info->Rec.EventType != KEY_EVENT) return FALSE;

  DWORD dwControl = Info->Rec.Event.KeyEvent.dwControlKeyState;
  DWORD dwCTRL = dwControl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
  DWORD dwALT = dwControl & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
  DWORD dwSHIFT = dwControl & SHIFT_PRESSED;

	if (dwCTRL && !dwSHIFT && !dwALT)
	{
		switch (Info->Rec.Event.KeyEvent.wVirtualKeyCode)
		{
		case 0x41:
		{
			PanelInfo PInfo;
      PInfo.StructSize = sizeof(PanelInfo);
			fInfo.PanelControl(Info->hPanel, FCTL_GETPANELINFO, 0, static_cast<void *>(&PInfo));
			android->ChangePermissionsDialog(static_cast<int>(PInfo.SelectedItemsNumber));
			android->Reread();
			fInfo.PanelControl(Info->hPanel, FCTL_UPDATEPANEL, 1, nullptr);
			fInfo.PanelControl(Info->hPanel, FCTL_REDRAWPANEL, 0, nullptr);
			return TRUE;
		}
		case 0x52:
		{
			android->Reread();
			fInfo.PanelControl(Info->hPanel, FCTL_UPDATEPANEL, 1, nullptr);
			fInfo.PanelControl(Info->hPanel, FCTL_REDRAWPANEL, 0, nullptr);
			return TRUE;
		}
		}
	}
	else if (!dwCTRL && dwSHIFT && !dwALT)
  {
    switch (Info->Rec.Event.KeyEvent.wVirtualKeyCode)
    {
    case VK_F5:
    {
      CString file = GetCurrentFileName();
      if (android->Copy(file) == TRUE)
      {
        android->Reread();
        fInfo.PanelControl(Info->hPanel, FCTL_UPDATEPANEL, 1, nullptr);
        fInfo.PanelControl(Info->hPanel, FCTL_REDRAWPANEL, 0, nullptr);
      }
      return TRUE;
    }
    case VK_F6:
    {
      CString file = GetCurrentFileName();
      if (android->Rename(file) == TRUE)
      {
        android->Reread();
        fInfo.PanelControl(Info->hPanel, FCTL_UPDATEPANEL, 1, nullptr);
        fInfo.PanelControl(Info->hPanel, FCTL_REDRAWPANEL, 0, nullptr);
      }
      return TRUE;
    }
    case VK_F7:
    {
      android->DeviceNameDialog();
      return TRUE;
    }
    }
  }
	else if (!dwCTRL && !dwSHIFT && dwALT)
  {
    switch (Info->Rec.Event.KeyEvent.wVirtualKeyCode)
    {
    case VK_F10:
    {
      android->GetFramebuffer();
      return TRUE;
    }
    }
  }
	else if (!dwCTRL && dwSHIFT && dwALT)
	{
		switch (Info->Rec.Event.KeyEvent.wVirtualKeyCode)
		{
		case VK_F10:
		{
			android->Remount(_T("rw"));
			return TRUE;
		}
		case VK_F11:
		{
			android->Remount(_T("ro"));
			return TRUE;
		}
		}
	}

  return FALSE;
}

intptr_t WINAPI ProcessPanelEventW(const struct ProcessPanelEventInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  switch (Info->Event)
  {
  case FE_CHANGEVIEWMODE:
    PanelInfo PInfo;
    PInfo.StructSize = sizeof(PanelInfo);
    fInfo.PanelControl(Info->hPanel, FCTL_GETPANELINFO, 0, static_cast<void *>(&PInfo));

    if (IS_FLAG(PInfo.Flags, PFLAGS_PLUGIN) && IS_FLAG(PInfo.Flags, PFLAGS_VISIBLE))
    {
      conf.SortMode = PInfo.SortMode;
      conf.PanelMode = static_cast<int>(PInfo.ViewMode);
      conf.SortOrder = IS_FLAG(PInfo.Flags, PFLAGS_REVERSESORTORDER);
      conf.SavePanel();
    }
    break;
  }

  return FALSE;
}

intptr_t WINAPI GetFilesW(struct GetFilesInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  static CString dest;
  dest = Info->DestPath;
  auto result = android->GetFiles(Info->PanelItem, static_cast<int>(Info->ItemsNumber), dest, Info->Move, Info->OpMode);
  if (result == FALSE)
    return FALSE;

  if (result == TRUE && Info->Move)
  {
    Info->OpMode |= OPM_SILENT;
    if (android->DeleteFiles(Info->PanelItem, static_cast<int>(Info->ItemsNumber), Info->OpMode) == False)
      return FALSE;

    Info->DestPath = _C(dest);
  }

  return TRUE;
}

intptr_t WINAPI SetDirectoryW(const struct SetDirectoryInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  return android->ChangeDir(Info->Dir, Info->OpMode);
}

intptr_t WINAPI PutFilesW(const struct PutFilesInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  auto result = android->PutFiles(Info->PanelItem, static_cast<int>(Info->ItemsNumber), Info->SrcPath, Info->Move, Info->OpMode);
  if (result == FALSE)
    return FALSE;

  if (result == TRUE && Info->Move)
  {
    CString sPath = GetPanelPath();
    return DeletePanelItems(sPath, Info->PanelItem, static_cast<int>(Info->ItemsNumber));
  }

  return TRUE;
}

intptr_t WINAPI DeleteFilesW(const struct DeleteFilesInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  auto result = android->DeleteFiles(Info->PanelItem, static_cast<int>(Info->ItemsNumber), Info->OpMode);
  if (result == FALSE)
    return FALSE;

  return TRUE;
}

intptr_t WINAPI MakeDirectoryW(struct MakeDirectoryInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);

  static CString dest;
  dest = Info->Name;
  auto result = android->CreateDir(dest, static_cast<int>(Info->OpMode));
  if (result == FALSE)
    return FALSE;

  if (result == TRUE)
  {
    Info->Name = _C(dest);
  }

  return TRUE;
}