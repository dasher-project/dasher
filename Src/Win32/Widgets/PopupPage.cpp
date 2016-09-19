// AlphabetBox.cpp
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002 Iain Murray, Inference Group, Cavendish, Cambridge.
//
/////////////////////////////////////////////////////////////////////////////

#include "WinCommon.h"

#include "PopupPage.h"
#include "../resource.h"

#include <utility>              // for std::pair

using namespace Dasher;
using namespace std;

// Track memory leaks on Windows to the line that new'd the memory
#ifdef _WIN32
#ifdef _DEBUG
#define DEBUG_NEW new( _NORMAL_BLOCK, THIS_FILE, __LINE__ )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CPopupPage::CPopupPage(HWND Parent, CAppSettings *pAppSettings)
:CPrefsPageBase(Parent, pAppSettings) {

  m_CurrentColours = pAppSettings->GetStringParameter(SP_COLOUR_ID);
}

struct menuentry {
  int paramNum; // enum value in Parameters.h for setting store
  int idcNum;   // #define value in resource.h for dasher.rc
};

// List of menu items that will be displayed in the General Preferences
static menuentry menutable[] = {
  {BP_DRAW_MOUSE, IDC_DRAWMOUSE},
  {BP_DRAW_MOUSE_LINE, IDC_DRAWMOUSELINE},
};

void CPopupPage::PopulateList() {
  // Populate the controls in the dialogue box based on the relevent parameters
  // in m_pDasher
  //Popup Enabled
  if (m_pAppSettings->GetBoolParameter(APP_BP_POPUP_ENABLE) == true) {
	  SendMessage(GetDlgItem(m_hwnd, IDC_POPUP_ENABLE), BM_SETCHECK, BST_CHECKED, 0);
  }
  if (m_pAppSettings->GetBoolParameter(APP_BP_POPUP_EXTERNAL_SCREEN) == true) {
	  SendMessage(GetDlgItem(m_hwnd, IDC_POPUP_EXTERNAL), BM_SETCHECK, BST_CHECKED, 0);
  }
  // TODO: Annoying inversion makes this hard
  if(m_pAppSettings->GetBoolParameter(BP_PALETTE_CHANGE)) {
    SendMessage(GetDlgItem(m_hwnd, IDC_COLOURSCHEME), BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(m_hwnd, IDC_COLOURS), FALSE);
  }
  else {
    SendMessage(GetDlgItem(m_hwnd, IDC_COLOURSCHEME), BM_SETCHECK, BST_CHECKED, 0);
    EnableWindow(GetDlgItem(m_hwnd, IDC_COLOURS), TRUE);
  }

  for(int ii = 0; ii<sizeof(menutable)/sizeof(menuentry); ii++)
  {
    if(m_pAppSettings->GetBoolParameter(menutable[ii].paramNum)) {
      SendMessage(GetDlgItem(m_hwnd, menutable[ii].idcNum), BM_SETCHECK, BST_CHECKED, 0);
    }
    else  {
      SendMessage(GetDlgItem(m_hwnd, menutable[ii].idcNum), BM_SETCHECK, BST_UNCHECKED, 0);
    }
  }


  HWND ListBox = GetDlgItem(m_hwnd, IDC_COLOURS);
  m_pAppSettings->GetPermittedValues(SP_COLOUR_ID, ColourList);

  // Add each string to list box and index each one
  bool SelectionSet = false;
  for(unsigned int i = 0; i < ColourList.size(); i++) {
    Tstring Item;
    WinUTF8::UTF8string_to_wstring(ColourList[i], Item);
    LRESULT Index = SendMessage(ListBox, LB_ADDSTRING, 0, (LPARAM) Item.c_str());
    SendMessage(ListBox, LB_SETITEMDATA, Index, (LPARAM) i);
    if(ColourList[i] == m_CurrentColours) {
      SendMessage(ListBox, LB_SETCURSEL, Index, 0);
      SelectionSet = true;
    }
  }
  if(SelectionSet == false) {
    SendMessage(ListBox, LB_SETCURSEL, 0, 0);
    LRESULT CurrentIndex = SendMessage(ListBox, LB_GETITEMDATA, 0, 0);
    m_CurrentColours = ColourList[CurrentIndex];
  }
  // Tell list box that we have set an item for it (so that delete and edit can be grayed if required)
  SendMessage(m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_COLOURS, LBN_SELCHANGE), 0);

  if(m_pAppSettings->GetLongParameter(LP_LINE_WIDTH) > 1)
    SendMessage(GetDlgItem(m_hwnd, IDC_THICKLINE), BM_SETCHECK, BST_CHECKED, 0);
  else
    SendMessage(GetDlgItem(m_hwnd, IDC_THICKLINE), BM_SETCHECK, BST_UNCHECKED, 0);

  SendMessage(GetDlgItem(m_hwnd, IDC_OUTLINE), BM_SETCHECK,
	  m_pAppSettings->GetLongParameter(LP_OUTLINE_WIDTH) ? BST_CHECKED : BST_UNCHECKED, 0); 
  
  if(m_pAppSettings->GetLongParameter(LP_DASHER_FONTSIZE) == Dasher::Opts::Normal) {
    SendMessage(GetDlgItem(m_hwnd, IDC_FONT_SMALL), BM_SETCHECK, BST_CHECKED, 0);
  }
  else if(m_pAppSettings->GetLongParameter(LP_DASHER_FONTSIZE) == Dasher::Opts::Big) {
    SendMessage(GetDlgItem(m_hwnd, IDC_FONT_LARGE), BM_SETCHECK, BST_CHECKED, 0);
  }
  else if(m_pAppSettings->GetLongParameter(LP_DASHER_FONTSIZE) == Dasher::Opts::VBig) {
    SendMessage(GetDlgItem(m_hwnd, IDC_FONT_VLARGE), BM_SETCHECK, BST_CHECKED, 0);
  }

}


bool CPopupPage::Apply() {
	OutputDebugStringW(L"Popup Prefs:: Apply\n");

  if(m_CurrentColours != std::string("")) {
        m_pAppSettings->SetStringParameter(SP_COLOUR_ID, m_CurrentColours);
  }

  m_pAppSettings->SetBoolParameter(BP_PALETTE_CHANGE, 
    SendMessage(GetDlgItem(m_hwnd, IDC_COLOURSCHEME), BM_GETCHECK, 0, 0) == BST_UNCHECKED );

  if(SendMessage(GetDlgItem(m_hwnd, IDC_FONT_SMALL), BM_GETCHECK, 0, 0) == BST_CHECKED)
    m_pAppSettings->SetLongParameter(LP_DASHER_FONTSIZE, Dasher::Opts::Normal);
  else if(SendMessage(GetDlgItem(m_hwnd, IDC_FONT_LARGE), BM_GETCHECK, 0, 0) == BST_CHECKED)
    m_pAppSettings->SetLongParameter(LP_DASHER_FONTSIZE, Dasher::Opts::Big);
  else if(SendMessage(GetDlgItem(m_hwnd, IDC_FONT_VLARGE), BM_GETCHECK, 0, 0) == BST_CHECKED)
    m_pAppSettings->SetLongParameter(LP_DASHER_FONTSIZE, Dasher::Opts::VBig);


  // Return false (and notify the user) if something is wrong.
  return TRUE;
}

LRESULT CPopupPage::WndProc(HWND Window, UINT message, WPARAM wParam, LPARAM lParam) {
  // most things we pass on to CPrefsPageBase, but we need to handle slider motion
	switch (message) {

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == LBN_SELCHANGE) {
			if (LOWORD(wParam) != 0 && m_hPropertySheet != 0 && m_hwnd != 0) {
				PropSheet_Changed(m_hPropertySheet, m_hwnd); // enables the 'Apply' button
				// Behaviour isn't *perfect* since it activates the Apply button even if you, say,
				// click 'new' alphabet then click Cancel when asked for a name.
			}
		}
		switch (LOWORD(wParam)) {
		case (IDC_COLOURS):
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				HWND ListBox = GetDlgItem(m_hwnd, IDC_COLOURS);
				LRESULT CurrentItem = SendMessage(ListBox, LB_GETCURSEL, 0, 0);
				LRESULT CurrentIndex = SendMessage(ListBox, LB_GETITEMDATA, CurrentItem, 0);
				m_CurrentColours = ColourList[CurrentIndex];
			}
			return TRUE;
			break;
		case IDC_DFONT_BUTTON:  // TODO: Put this in a function
			{
			CHOOSEFONT Data;
			LOGFONT lf;
			HFONT Font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			GetObject(Font, sizeof(LOGFONT), &lf);
			Tstring tstrFaceName;
			WinUTF8::UTF8string_to_wstring(m_pAppSettings->GetStringParameter(SP_DASHER_FONT), tstrFaceName);
			_tcscpy(lf.lfFaceName, tstrFaceName.c_str());
			Data.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
			Data.lStructSize = sizeof(CHOOSEFONT);
			// TODO: Give this an owner
			Data.hwndOwner = NULL;
			Data.lpLogFont = &lf;
			if (ChooseFont(&Data)) {
				string FontName;
				WinUTF8::wstring_to_UTF8string(lf.lfFaceName, FontName);
				//m_pAppSettings->SetStringParameter(SP_DASHER_FONT, FontName);
				m_pAppSettings->SetLongParameter(APP_LP_POPUP_FONT_SIZE, lf.lfHeight);
			}
			}
			break;
		case IDC_COLOURSCHEME:
			EnableWindow(GetDlgItem(m_hwnd, IDC_COLOURS), SendMessage(GetDlgItem(m_hwnd, IDC_COLOURSCHEME), BM_GETCHECK, 0, 0) == BST_CHECKED);
			break;
		case IDC_POPUP_ENABLE:
			m_pAppSettings->SetBoolParameter(APP_BP_POPUP_ENABLE, !(m_pAppSettings->GetBoolParameter(APP_BP_POPUP_ENABLE)));
			break;
		case IDC_POPUP_EXTERNAL:
			m_pAppSettings->SetBoolParameter(APP_BP_POPUP_EXTERNAL_SCREEN, !(m_pAppSettings->GetBoolParameter(APP_BP_POPUP_EXTERNAL_SCREEN)));
			break;
		case IDC_POPUP_ALWAYSTOP:
			OutputDebugStringW(L"Always On Top\n");
			break;
		default:
			break;

	}
  }
  return CPrefsPageBase::WndProc(Window, message, wParam, lParam);
}
