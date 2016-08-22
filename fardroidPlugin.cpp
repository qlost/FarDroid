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
  Info->Version = MAKEFARVERSION(MAJORVERSION, MINORVERSION, BUILDNUMBER, 0, VS_RC);
  Info->Guid = MainGuid;
  Info->Title = L"FARdroid";
  Info->Description = L"fardroid FAR Plugin";
  Info->Author = L"Vladimir Kubyshev";
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

  PluginConfigMenuStrings[0] = LOC(MConfTitle);
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
  switch (Info->OpenFrom)
  {
  case OPEN_LEFTDISKMENU:
  case OPEN_RIGHTDISKMENU:
  case OPEN_PLUGINSMENU:
    {
      fardroid* android = new fardroid();
      if (android)
      {
        HANDLE res = android->OpenFromMainMenu();
        if (res == INVALID_HANDLE_VALUE)
          delete android;

        return res;
      }
      break;
    }
  case OPEN_COMMANDLINE:
    {
      fardroid* android = new fardroid();
      if (android)
      {
        CString cmd = reinterpret_cast<OpenCommandLineInfo*>(Info->Data)->CommandLine;
        HANDLE res = android->OpenFromCommandLine(cmd);
        if (res == INVALID_HANDLE_VALUE)
          delete android;

        return res;
      }
      break;
    }
  default:
    break;
  }
  return INVALID_HANDLE_VALUE;
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
  if (DN_BTNCLICK == Msg)
    switch (Param1)
    {
    case 6:
      fInfo.SendDlgMessage(hDlg, DM_ENABLE, 7, Param2);
      break;
    case 5:
    case 9:
      int Enable = fInfo.SendDlgMessage(hDlg, DM_GETCHECK, 5, nullptr) && fInfo.SendDlgMessage(hDlg, DM_GETCHECK, 9, nullptr);
      fInfo.SendDlgMessage(hDlg, DM_ENABLE, 10, reinterpret_cast<void *>(Enable));
      break;
    }
  return fInfo.DefDlgProc(hDlg, Msg, Param1, Param2);
}

intptr_t WINAPI ConfigureW(const struct ConfigureInfo* Info)
{
  const auto width = 55;
  struct InitDialogItem InitItems[] = {
    /*00*/FDI_DOUBLEBOX(width - 4,19,(farStr *)MConfTitle),
    /*01*/FDI_CHECK(5, 2,(farStr *)MConfAddToDisk),
    /*02*/FDI_LABEL(5, 3, (farStr *)MConfPrefix),
    /*03*/FDI_EDIT(13, 3,28, _F("fardroidPrefix")),
    /*04*/FDI_RADIO(5, 5, (farStr*)MConfSafeMode),
    /*05*/FDI_RADIO(5, 6, (farStr*)MConfNative),
    /*06*/FDI_RADIO(5, 7, (farStr*)MConfBusybox),
    /*07*/FDI_CHECK(9, 8, (farStr*)MConfShowLinksAsDirs),
    /*08*/FDI_CHECK(5,10, (farStr*)MConfShowAllFilesystems),
    /*09*/FDI_CHECK(5,11, (farStr*)MConfUseSU),
    /*10*/FDI_CHECK(9,12, (farStr*)MConfUseExtendedAccess),
    /*11*/FDI_CHECK(5,13, (farStr*)MConfRemountSystem),
    /*12*/FDI_LABEL(5,14, (farStr*)MConfTimeout),
    /*13*/FDI_EDIT(14,14,27, _F("fardroidTimeout")),
    /*14*/FDI_LABEL(5,16, (farStr*)MConfADBPath),
    /*15*/FDI_EDIT(18,16,43, _F("fardroidADBPath")),
    /*16*/FDI_DEFCBUTTON(18,(farStr *)MOk),
    /*17*/FDI_CBUTTON(18,(farStr *)MCancel),
    /*--*/FDI_SEPARATOR(4,_F("")),
    /*--*/FDI_SEPARATOR(9,_F("")),
    /*--*/FDI_SEPARATOR(15,_F("")),
    /*--*/FDI_SEPARATOR(17,_F("")),
  };
  const int size = sizeof InitItems / sizeof InitItems[0];

  FarDialogItem DialogItems[size];
  InitDialogItems(InitItems, DialogItems, size);

  DialogItems[1].Selected = conf.AddToDiskMenu;

  wchar_t* editbuf2 = static_cast<wchar_t *>(my_malloc(1024));
  lstrcpyW(editbuf2, conf.Prefix);
  DialogItems[3].Data = editbuf2;

  DialogItems[4].Selected = conf.WorkMode == WORKMODE_SAFE;
  DialogItems[5].Selected = conf.WorkMode == WORKMODE_NATIVE;
  DialogItems[6].Selected = conf.WorkMode == WORKMODE_BUSYBOX;
  DialogItems[7].Selected = conf.ShowLinksAsDir;
  DialogItems[8].Selected = conf.ShowAllPartitions;
  DialogItems[9].Selected = conf.UseSU;
  DialogItems[10].Selected = conf.UseExtendedAccess;
  DialogItems[11].Selected = conf.RemountSystem;

  wchar_t* editbuf3 = static_cast<wchar_t *>(my_malloc(1024));
  FSF.sprintf(editbuf3, L"%d", conf.TimeOut);
  DialogItems[13].Data = editbuf3;

  wchar_t* editbuf4 = static_cast<wchar_t *>(my_malloc(1024));
  lstrcpyW(editbuf4, conf.ADBPath);
  DialogItems[15].Data = editbuf4;

  HANDLE hdlg = fInfo.DialogInit(&MainGuid, &DialogGuid, -1, -1, width, 21, _F("Config"), DialogItems, size, 0, 0, ConfigDlgProc, nullptr);
  if (conf.WorkMode != WORKMODE_BUSYBOX)
    fInfo.SendDlgMessage(hdlg, DM_ENABLE, 7, reinterpret_cast<void *>(FALSE));
  if (!(conf.UseSU && conf.WorkMode == WORKMODE_NATIVE))
    fInfo.SendDlgMessage(hdlg, DM_ENABLE, 10, reinterpret_cast<void *>(FALSE));
  int res = fInfo.DialogRun(hdlg);
  if (res == 16)
  {
    conf.AddToDiskMenu = GetItemSelected(hdlg, 1);

    conf.Prefix = GetItemData(hdlg, 3);
    if (GetItemSelected(hdlg, 4))
      conf.WorkMode = WORKMODE_SAFE;
    else if (GetItemSelected(hdlg, 5))
      conf.WorkMode = WORKMODE_NATIVE;
    if (GetItemSelected(hdlg, 6))
      conf.WorkMode = WORKMODE_BUSYBOX;

    conf.ShowLinksAsDir = GetItemSelected(hdlg, 7);
    conf.ShowAllPartitions = GetItemSelected(hdlg, 8);
    conf.UseSU = GetItemSelected(hdlg, 9);
    conf.UseExtendedAccess = GetItemSelected(hdlg, 10);
    conf.RemountSystem = GetItemSelected(hdlg, 11);

    conf.TimeOut = FSF.atoi(GetItemData(hdlg, 13));

    conf.ADBPath = GetItemData(hdlg, 15);

    conf.Save();
  }
  fInfo.DialogFree(hdlg);
  my_free(editbuf2);
  my_free(editbuf3);
  my_free(editbuf4);

  return res == 16;
}

void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  Info->StructSize = sizeof(*Info);
  Info->Flags = OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE;

  Info->StartSortMode = static_cast<OPENPANELINFO_SORTMODES>(conf.SortMode);
  Info->StartSortOrder = conf.SortOrder;

  Info->CurDir = nullptr;
  Info->Format = LOC(MTitle);

  android->PreparePanel(Info);

  Info->DescrFiles = nullptr;
  Info->DescrFilesNumber = 0;

  Info->StartPanelMode = _F('0') + conf.PanelMode;
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
  android->FreeFindData(Info->PanelItem, Info->ItemsNumber);
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
      android->ChangePermissionsDialog();
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

  if (!dwCTRL && dwSHIFT && !dwALT)
  {
    switch (Info->Rec.Event.KeyEvent.wVirtualKeyCode)
    {
    case VK_F6:
    {
      CString file = ExtractName(GetCurrentFileName());
      if (android->Rename(file) == TRUE)
      {
        android->Reread();
        fInfo.PanelControl(Info->hPanel, FCTL_UPDATEPANEL, 1, nullptr);
        fInfo.PanelControl(Info->hPanel, FCTL_REDRAWPANEL, 0, nullptr);
      }
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
    fInfo.PanelControl(Info->hPanel, FCTL_GETPANELINFO, 0, static_cast<void *>(&PInfo));
    conf.SortMode = PInfo.SortMode;
    conf.PanelMode = PInfo.ViewMode;
    conf.SortOrder = IS_FLAG(PInfo.Flags, PFLAGS_REVERSESORTORDER);
    conf.Save();
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
  auto result = android->GetFiles(Info->PanelItem, Info->ItemsNumber, dest, Info->Move, Info->OpMode);
  if (result == FALSE)
    return FALSE;

  if (result == TRUE && Info->Move)
  {
    Info->OpMode |= OPM_SILENT;
    if (android->DeleteFiles(Info->PanelItem, Info->ItemsNumber, Info->OpMode) == False)
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
  auto result = android->PutFiles(Info->PanelItem, Info->ItemsNumber, Info->SrcPath, Info->Move, Info->OpMode);
  if (result == FALSE)
    return FALSE;

  if (result == TRUE && Info->Move)
  {
    CString sPath = GetPanelPath();
    return DeletePanelItems(sPath, Info->PanelItem, Info->ItemsNumber);
  }

  return TRUE;
}

intptr_t WINAPI DeleteFilesW(const struct DeleteFilesInfo* Info)
{
  if (!Info->hPanel || INVALID_HANDLE_VALUE == Info->hPanel)
    return FALSE;

  fardroid* android = static_cast<fardroid *>(Info->hPanel);
  auto result = android->DeleteFiles(Info->PanelItem, Info->ItemsNumber, Info->OpMode);
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
