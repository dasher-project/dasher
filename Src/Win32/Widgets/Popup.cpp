// Popup.cpp
//
// Copyright (c) 2016 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software 
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
// NOTES: Created by Jeremy Cope to facilitate extended (multiple) displays.
//

#include "WinCommon.h"

#include "Popup.h"
#include <Windows.h>
#include "../../DasherCore/Event.h"
#include "FilenameGUI.h"
#include "../resource.h"
#include "../../DasherCore/DasherInterfaceBase.h"
#include "../Dasher.h"


using namespace Dasher;
using namespace std;
using namespace WinLocalisation;
using namespace WinUTF8;

BOOL contains(RECT rectA, RECT rectB);
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

CPopup::CPopup(CAppSettings *pAppSettings) {
  
  // TODO: Check that this is all working okay (it quite probably
  // isn't). In the long term need specialised editor classes.
  targetwindow = 0;

  m_pAppSettings = pAppSettings;

  UINT CodePage = GetUserCodePage();
  m_Font = GetCodePageFont(CodePage, 14);
}

HWND CPopup::Create(HWND hParent, bool bNewWithDate) {
  RECT r = getInitialWindow();
  m_popup = CWindowImpl<CPopup>::Create(hParent, r, NULL, WS_OVERLAPPEDWINDOW| ES_MULTILINE );
  return *this;
}

CPopup::~CPopup() {
  DeleteObject(m_Font);
}
void CPopup::setupPopup() {
	//Calculate the size of the windows,displays
	calculateDisplayProperties();

	//Determine the placement of the popup
	positionPopup();

	//If enabled, show and start the auto update timer
	if (m_pAppSettings->GetBoolParameter(APP_BP_POPUP_ENABLE) == true) {
		//Show the Popup
		ShowWindow(SW_SHOW);
		//Fire the update timer
		CDasher* dasher = (CDasher*)m_pDasherInterface; //Cast for concrete implementation
		dasher->configurePopupTimer(true);
	}
}
void CPopup::Move(int x, int y, int Width, int Height) {
  MoveWindow( x, y, Width, Height, TRUE);
}

void CPopup::SetFont(string Name, long Size) {
  Tstring FontName;
  UTF8string_to_wstring(Name, FontName);

  if(Size == 0)
    Size = 14;

  DeleteObject(m_Font);
  if (Name == "") {
    UINT CodePage = GetUserCodePage();
    m_Font = GetCodePageFont(CodePage, -Size);
  }
  else
    m_Font = CreateFont(-Size, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, FontName.c_str());    // DEFAULT_CHARSET => font made just from Size and FontName

  SendMessage(WM_SETFONT, (WPARAM) m_Font, true);
}

void CPopup::SetInterface(Dasher::CDasherInterfaceBase *DasherInterface) {
  m_pDasherInterface = DasherInterface;
}

void CPopup::updateDisplay(const std::string sText) {
  //If the display output has changed.. update the popup window
  if (sText.compare(m_Output) != 0) {
    m_Output = sText;
    output(m_Output);
  }
}

void CPopup::output(const std::string &sText) {
  wstring String;
  WinUTF8::UTF8string_to_wstring(sText, String);
  InsertText(String);
}

void CPopup::InsertText(Tstring InsertText) {
  //Update entire screen
  SendMessage(WM_SETTEXT, TRUE, (LPARAM)InsertText.c_str());
  //Scroll to bottom
  SendMessage(EM_LINESCROLL, 0, 5);
}

void CPopup::HandleParameterChange(int iParameter) {
  switch(iParameter) {
  case APP_SP_POPUP_FONT:
  case APP_LP_POPUP_FONT_SIZE:
    SetFont(m_pAppSettings->GetStringParameter(APP_SP_POPUP_FONT), m_pAppSettings->GetLongParameter(APP_LP_POPUP_FONT_SIZE));
    break;
  case APP_BP_POPUP_ENABLE:
	 OutputDebugString(L"Request to show/hide popup"); //Log Updated Display to console
	if(m_pAppSettings->GetBoolParameter(APP_BP_POPUP_ENABLE) == true){
	  ShowWindow(SW_SHOW);
	  CDasher* dasher = (CDasher*)m_pDasherInterface; //Cast for concrete implementation
	  dasher->configurePopupTimer(true);
	}
	else {
	  OutputDebugString(L"..hide.\n"); //Log Updated Display to console
	  ShowWindow(SW_HIDE);
	}		
    break;
  case APP_BP_POPUP_EXTERNAL_SCREEN:
	positionPopup();
    break;
  case APP_BP_POPUP_INFRONT:
	break;
  default:
    break;
  }
}


RECT CPopup::getInitialWindow() {
	RECT rect;
	rect = {
		1000,100,1800,500
	};
	return rect;
}
void CPopup::calculateDisplayProperties() {
	getDasherWidnowInfo();
	getMonitorInfo(); //Warning, asysnchronis call
}
void CPopup::positionPopup() {
	LPRECT popupDisplayRect;
	if (m_pAppSettings->GetBoolParameter(APP_BP_POPUP_EXTERNAL_SCREEN) == true) {
		popupDisplayRect = &externalMonitorRect;
	}
	else {
		popupDisplayRect = &popupRect;
	}

	//Redraw in the correct location
	MoveWindow(popupDisplayRect, true);
}
bool CPopup::getMonitorInfo()
{
	RECT* userData[2] = { &dasherWindwowRect, &externalMonitorRect };
	if (EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&userData))
		return true;
	return false;//signals an error
}
void CPopup::getDasherWidnowInfo() {
	//Get Dasher Window position and size
	CDasher* dasher = (CDasher*)m_pDasherInterface; //Cast for concrete implementation
	int     iTop = 0;
	int     iLeft = 0;
	int     iBottom = 0;
	int     iRight = 0;

	dasher->GetWindowSize(&iTop, &iLeft, &iBottom, &iRight);

	dasherWindwowRect.top = iTop;
	dasherWindwowRect.left = iLeft;
	dasherWindwowRect.right = iRight;
	dasherWindwowRect.bottom = iBottom;

	//Calculate default size based on dasher size
	popupRect.top = dasherWindwowRect.top;
	popupRect.left = dasherWindwowRect.right + 20;
	popupRect.bottom = (dasherWindwowRect.bottom - dasherWindwowRect.top) / 2;
	popupRect.right = (dasherWindwowRect.right - dasherWindwowRect.left) + popupRect.left;
}
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	RECT monitorCoordinates = *lprcMonitor;
	RECT** userData = (RECT**)dwData;
	RECT* dasherRect = userData[0];
	RECT* externRect = userData[1];


	//If this monitor contains the dasher Window, it is primary monitor
	//Else it is external monitor, and should be used for popup.
	//Else window spans multiple monitors..
	if (contains(monitorCoordinates, *dasherRect)) {

	}
	else {
		*externRect = monitorCoordinates;

		OutputDebugString(L"Use this rect as external\n");
		char output[100];
		sprintf(output, "Monitor[%d],%d,%d,%d,%d\n", 0, monitorCoordinates.left, monitorCoordinates.top, monitorCoordinates.right, monitorCoordinates.bottom);
		OutputDebugStringA(output);
	}
	return TRUE;
}
BOOL contains(RECT rectA, RECT rectB) {
	return (rectA.left < rectB.right && rectA.right > rectB.left &&
		rectA.top < rectB.bottom && rectA.bottom > rectB.top);
}