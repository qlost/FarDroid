// stdafx.cpp : source file that includes just the standard includes
// fardroid2.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

PluginStartupInfo fInfo;
FarStandardFunctions FSF;
HMODULE hInst;
CConfig conf;

struct KeyBarLabel Label[4] = {
	{ { VK_F7, SHIFT_PRESSED }, L"DevName", L"Device Name" },
  { { VK_F10, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED }, L"ScrShot", L"Screenshot" },
	{ { VK_F10, SHIFT_PRESSED | LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED }, L"Sys RW", L"Mount /system RW" },
	{ { VK_F11, SHIFT_PRESSED | LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED }, L"Sys RO", L"Mount /system RO" },
};
struct KeyBarTitles KeyBar = {4, Label};

HANDLE hRegexpDate1 = nullptr
     , hRegexpDate2 = nullptr
     , hRegexpDate3 = nullptr
     , hRegexpProp = nullptr
     , hRegexpSize = nullptr
     , hRegexpMem = nullptr
     , hRegexpPart1 = nullptr
     , hRegexpPart2 = nullptr
     , hRegexpFile[2][2] = {};