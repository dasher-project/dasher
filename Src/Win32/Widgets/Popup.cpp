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

/**
* This class manages the external popup window
* to be used to be used on an 2nd monitor (extended display).
*
* Known Issues:
*	-> Setting the font- seems to capture size but not name
*
*
*
*/


using namespace Dasher;
using namespace std;
using namespace WinLocalisation;
using namespace WinUTF8;

BOOL contains(RECT rectA, RECT rectB);
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);


/**
* Constructors, Destructors, Initializers, Setup Functions
*/
CPopup::CPopup(CAppSettings *pAppSettings) {
  m_pAppSettings = pAppSettings;

  UINT CodePage = GetUserCodePage();
  m_Font = GetCodePageFont(CodePage, 14);

  m_setup = false;
  m_externalMonitorRect = RECT();
  m_dasherWindwowRect = RECT();
  m_popupRect = RECT();
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
	if (!m_setup) {
		m_setup = true;

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
  else {
	  m_Font = CreateFont(-Size, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, FontName.c_str());    // DEFAULT_CHARSET => font made just from Size and FontName
  }
  SendMessage(WM_SETFONT, (WPARAM) m_Font, true);
}

void CPopup::SetInterface(Dasher::CDasherInterfaceBase *DasherInterface) {
  m_pDasherInterface = DasherInterface;
}


/**
* Text Display Management- used to update the display with different text 
*/
//Timer callback function
void CPopup::updateDisplay(const std::string sText) {
  //If the display output has changed.. update the popup window
  if (sText.compare(m_Output) != 0) {
    m_Output = sText;
    output(m_Output);
  }
}
//Exposed Method so outside classes can force an update on the screen
void CPopup::output(const std::string &sText) {
  wstring String;
  WinUTF8::UTF8string_to_wstring(sText, String);
  InsertText(String);
}
//Actually updates the window with the provided text
void CPopup::InsertText(Tstring InsertText) {
  //Update entire screen
  SendMessage(WM_SETTEXT, TRUE, (LPARAM)InsertText.c_str());
  //Scroll to bottom
  SendMessage(EM_LINESCROLL, 0, INT_MAX); //Force to end of buffer with max integer
}


/**
* Popup Actionables- called when parameter changes in settings, or toolbar button is pressed
* BUG: Seems to be called twice on parameter changes
*/
void CPopup::HandleParameterChange(int iParameter) {
  switch(iParameter) {
  case APP_SP_POPUP_FONT:
  case APP_LP_POPUP_FONT_SIZE:
    SetFont(m_pAppSettings->GetStringParameter(APP_SP_POPUP_FONT), m_pAppSettings->GetLongParameter(APP_LP_POPUP_FONT_SIZE));
    break;
  case APP_BP_POPUP_ENABLE:
	if(m_pAppSettings->GetBoolParameter(APP_BP_POPUP_ENABLE) == true){
	  ShowWindow(SW_SHOW);
	  CDasher* dasher = (CDasher*)m_pDasherInterface; //Cast for concrete implementation
	  dasher->configurePopupTimer(true);
	}
	else {
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
//Toolbar quick action ignores 'use external monitor' setting
//Forces external monitor usages (fi exists)
bool CPopup::processToolbarButtonPress() {
	bool popupEnabled = false;
	popupEnabled = m_pAppSettings->GetBoolParameter(APP_BP_POPUP_ENABLE);
	//Action depends on current status
	if (popupEnabled == false) {
		//Quick button possitions fully on the external monitor, if monitor exists
		LPRECT popupDisplayRect;
		if (isExtMonitorDetected()) {
			popupDisplayRect = &m_externalMonitorRect;
		}
		else {
			popupDisplayRect = &m_popupRect;
		}
		MoveWindow(popupDisplayRect);
	}
	
	//Update the state flag
	popupEnabled = !popupEnabled;
	m_pAppSettings->SetBoolParameter(APP_BP_POPUP_ENABLE, popupEnabled); //Setting the parameter triggers action
	//Return the current state
	return popupEnabled;
}

/**
* Popup Management and Placement
*/
void CPopup::Move(int x, int y, int Width, int Height) {
	MoveWindow(x, y, Width, Height, TRUE);
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
	//Warning, assync call, enumerates all displays
	getMonitorInfo();
}

void CPopup::positionPopup() {
	LPRECT popupDisplayRect;
	if (m_pAppSettings->GetBoolParameter(APP_BP_POPUP_EXTERNAL_SCREEN) == true) {
		//May have setting to use external, but does external screen exists?
		if (isExtMonitorDetected()) {
			popupDisplayRect = &m_externalMonitorRect;
		}
		else {
			popupDisplayRect = &m_popupRect;
		}
	}
	else {
		popupDisplayRect = &m_popupRect;
	}

	//Redraw in the correct location
	MoveWindow(popupDisplayRect, true);
}

bool CPopup::isExtMonitorDetected() {
	bool retVal = false;
	if (!IsRectEmpty(&m_externalMonitorRect)){
		retVal = true;
	}
	return retVal;
}

void CPopup::getMonitorInfo(){
	RECT* userData[2] = { &m_dasherWindwowRect, &m_externalMonitorRect };
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&userData);
}

void CPopup::getDasherWidnowInfo() {
	//Get Dasher Window position and size
	CDasher* dasher = (CDasher*)m_pDasherInterface; //Cast for concrete implementation
	int     iTop = 0;
	int     iLeft = 0;
	int     iBottom = 0;
	int     iRight = 0;

	dasher->GetWindowSize(&iTop, &iLeft, &iBottom, &iRight);

	m_dasherWindwowRect.top = iTop;
	m_dasherWindwowRect.left = iLeft;
	m_dasherWindwowRect.right = iRight;
	m_dasherWindwowRect.bottom = iBottom;

	//Initialize our window based on the dasher window
	m_popupRect.top = m_dasherWindwowRect.top + 100;
	m_popupRect.left = m_dasherWindwowRect.left + 100;
	m_popupRect.right = m_dasherWindwowRect.right + 100;
	m_popupRect.bottom = m_dasherWindwowRect.top + (m_dasherWindwowRect.bottom - m_dasherWindwowRect.top)/2;
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
		//Never called if no extra monitor
		*externRect = monitorCoordinates;
	}
	return TRUE;
}

BOOL contains(RECT rectA, RECT rectB) {
	return (rectA.left < rectB.right && rectA.right > rectB.left &&
		rectA.top < rectB.bottom && rectA.bottom > rectB.top);
}