// stdafx.cpp : source file that includes just the standard includes
// fardroid2.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

PluginStartupInfo fInfo;
FarStandardFunctions FSF;
HMODULE hInst;
CConfig conf;

struct KeyBarLabel Label[1] = { {{VK_F10, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED }, L"ScrShot", L"Screenshot" } };
struct KeyBarTitles KeyBar = {1, Label};
