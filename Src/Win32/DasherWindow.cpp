// DasherWindow.cpp
//
// Copyright (c) 2007 The Dasher Team
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

#include "Dasher.h"
#include "DasherWindow.h"
#include "Widgets/AboutBox.h"
#include "Widgets/Prefs.h"

#include "Widgets/Toolbar.h"
#include "WinCommon.h"
#include "../DasherCore/XmlSettingsStore.h"
#include <windows.h>
#include "resource.h"

#include <Htmlhelp.h>

using namespace Dasher;
using namespace std;

#define IDT_TIMER1 200

namespace {
class XmlErrorDisplay : public CMessageDisplay {
public:
    void Message(const std::string &strText, bool bInterrupt) override {
        std::wstring text;
        WinUTF8::UTF8string_to_wstring(strText, text);
        // TODO: find a way to localize.
        MessageBox(nullptr, text.c_str(), L"Configuration Error", MB_ICONERROR|MB_ICONSTOP);
    }
};
} // namespace

// NOTE: There were previously various bits and pieces in this class from 
// text services framework stuff, which were never really finished. If
// required, look in version control history (prior to May 2007).

CDasherWindow::CDasherWindow(const wstring& configName) : m_configName(configName){
  m_bFullyCreated = false;
  m_pAppSettings = 0;
  m_pToolbar = 0;
  m_pEdit = 0;
  m_pPopup = 0;
  m_pSpeedAlphabetBar = 0;
  m_pSplitter = 0;
  m_pDasher = 0;
  m_bSizeRestored = false;

  m_hIconSm = (HICON)LoadImage(WinHelper::hInstApp, (LPCTSTR)IDI_DASHER, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

  ATL::CWndClassInfo& wc = CDasherWindow::GetWndClassInfo();
  wc.m_wc.hIcon = LoadIcon(WinHelper::hInstApp, (LPCTSTR)IDI_DASHER);
  wc.m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.m_wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
  wc.m_wc.hIconSm = m_hIconSm;

  m_hMenu = LoadMenu(WinHelper::hInstApp, (LPCTSTR)IDC_DASHER);
}

HWND CDasherWindow::Create() {
  hAccelTable = LoadAccelerators(WinHelper::hInstApp, (LPCTSTR)IDC_DASHER);

  // Get window title from resource script
  Tstring WindowTitle;
  WinLocalisation::GetResourceString(IDS_APP_TITLE, &WindowTitle);

  string configFileName = "settings.xml";
  if (!m_configName.empty())
    configFileName = "settings." + WinUTF8::wstring_to_UTF8string(m_configName.c_str()) + ".xml";
  static XmlErrorDisplay display;
  static CWinFileUtils fileUtils;
  auto settings = new Dasher::XmlSettingsStore(configFileName, &fileUtils, &display);
  settings->Load();
  // Save the defaults if needed.
  settings->Save();

  m_pAppSettings = new CAppSettings(0, 0, settings);  // Takes ownership of the settings store.
  int iStyle(m_pAppSettings->GetLongParameter(APP_LP_STYLE));

  if (iStyle == APP_STYLE_DIRECT) {
    hWnd = CWindowImpl<CDasherWindow >::Create(NULL, NULL, WindowTitle.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, WS_EX_NOACTIVATE | WS_EX_APPWINDOW | WS_EX_TOPMOST);
    ::SetMenu(hWnd, NULL);
  }
  else {
    hWnd = CWindowImpl<CDasherWindow >::Create(NULL, NULL, WindowTitle.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);
    ::SetMenu(hWnd, m_hMenu);
  }

  // Create Widgets
  m_pEdit = new CEdit(m_pAppSettings);
  m_pEdit->Create(hWnd, m_pAppSettings->GetBoolParameter(APP_BP_TIME_STAMP));
  m_pEdit->SetFont(m_pAppSettings->GetStringParameter(APP_SP_EDIT_FONT), m_pAppSettings->GetLongParameter(APP_LP_EDIT_FONT_SIZE));

  // Create Pop-Out Window - for multiple display support
  m_pPopup = new CPopup(m_pAppSettings);
  m_pPopup->Create(hWnd, m_pAppSettings->GetBoolParameter(APP_BP_TIME_STAMP));
  m_pPopup->SetFont(m_pAppSettings->GetStringParameter(APP_SP_POPUP_FONT), m_pAppSettings->GetLongParameter(APP_LP_POPUP_FONT_SIZE));

  m_pDasher = new CDasher(hWnd, this, m_pEdit, m_pPopup, settings, &fileUtils);

  // Create a CAppSettings
  m_pAppSettings->SetHwnd(hWnd);
  m_pAppSettings->SetDasher(m_pDasher);

  m_pToolbar = new CToolbar(hWnd, m_pAppSettings->GetBoolParameter(APP_BP_SHOW_TOOLBAR));

  // FIXME - the edit box really shouldn't need access to the interface, 
  // but at the moment it does, for training, blanking the display etc

  m_pEdit->SetInterface(m_pDasher);
  m_pPopup->SetInterface(m_pDasher);

  m_pSpeedAlphabetBar = new CStatusControl(m_pDasher->GetSettingsUser(), m_pAppSettings);
  m_pSpeedAlphabetBar->Create(hWnd);

  m_pSplitter = new CSplitter(this, 100);
  m_pSplitter->Create(hWnd);

  m_bFullyCreated = true;

  return hWnd;
}

CDasherWindow::~CDasherWindow() {
  delete m_pToolbar;
  delete m_pEdit;
  delete m_pPopup;
  delete m_pSplitter;
  delete m_pDasher;
  delete m_pSpeedAlphabetBar;
  delete m_pAppSettings;

  DestroyIcon(m_hIconSm);
}

void CDasherWindow::Show(int nCmdShow) {
    RECT r = {
      m_pAppSettings->GetLongParameter(APP_LP_X),
      m_pAppSettings->GetLongParameter(APP_LP_Y),
      m_pAppSettings->GetLongParameter(APP_LP_SCREEN_WIDTH),
      m_pAppSettings->GetLongParameter(APP_LP_SCREEN_HEIGHT),
    };
    r.bottom += r.top;
    r.right += r.left;
  MoveWindow(&r, false);
  m_bSizeRestored = true;
  if (m_pAppSettings->GetBoolParameter(APP_BP_FULL_SCREEN))
    nCmdShow = SW_MAXIMIZE;
  ShowWindow(nCmdShow);
}
//This starts a brute force timer to update the popup window display
void CDasherWindow::configurePopupTimer(bool enable){
  if (enable) {
    ::SetTimer(hWnd, 2, 1270, TIMERPROC(NULL));
  }
  else {
    ::KillTimer(hWnd, 2);
  }
}
void CDasherWindow::HandleParameterChange(int iParameter) {
  switch (iParameter) {
  case APP_BP_SHOW_TOOLBAR:
    m_pToolbar->ShowToolbar(m_pAppSettings->GetBoolParameter(APP_BP_SHOW_TOOLBAR));
    Layout();
    break;

  case APP_BP_SHOW_STATUSBAR:
  case APP_BP_MIRROR_LAYOUT:
  case APP_LP_STYLE:
    Layout();
    break;
  case APP_BP_TIME_STAMP:
    // TODO: reimplement
    // m_pEdit->TimeStampNewFiles(m_pAppSettings->GetBoolParameter(APP_BP_TIME_STAMP));
    break;
  case LP_MAX_BITRATE:
    // TODO: reimplement
    break;
    // XXXPW
#if 0
  case BP_GAME_MODE: {
    int iNewState(m_pDasher->GetBoolParameter(BP_GAME_MODE) ? MF_CHECKED : MF_UNCHECKED);
    DWORD iPrevState = CheckMenuItem(m_hMenu, ID_GAMEMODE, MF_BYCOMMAND | iNewState);
    DASHER_ASSERT(iPrevState != -1); //-1 = item does not exist (i.e. params to previous incorrect!)
  }
#endif
  default:
    break;
  }
}

LRESULT CDasherWindow::OnCommand(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (!m_bFullyCreated)
  {
    bHandled = FALSE;
    return 0;
  }

  const int wmId = LOWORD(wParam); //command id
  const int wmEvent = HIWORD(wParam); //notification code: 1=from accelerator, 0=from menu
  //lParam is the HWND (window handle) of control sending message, 0 if not from control

  // Tell edit box if it has changed. It should know itself really, but this is easier
  // This shouldn't be here - it should be in the edit box class
  if (((HWND)lParam == *m_pEdit) && (HIWORD(wParam) == EN_CHANGE)) {
    m_pEdit->SetDirty();
    return 0;
  }

  // Parse the menu selections:
  // TODO: Put these into separate functions
  switch (wmId) {
  case IDM_ABOUT: {
    CAboutBox Aboutbox;
    Aboutbox.DoModal(m_hWnd);
    return 0;
  }
  case ID_OPTIONS_PREFS: {
    CPrefs Prefs(m_hWnd, m_pDasher, m_pAppSettings);
    return 0;
  }
  case ID_QUICK_POPUP: {
    m_pPopup->processToolbarButtonPress();
    return 0;
  }
  case ID_HELP_CONTENTS:
    HtmlHelp(m_hWnd, L"Dasher.chm", HH_DISPLAY_INDEX, NULL);
    return 0;
    // XXXPW
#if 0
  case ID_GAMEMODE: {
    unsigned int checkState(GetMenuState(m_hMenu, ID_GAMEMODE, MF_BYCOMMAND));
    DASHER_ASSERT(checkState == -1); //"specified item does not exist" - presumably, params to above aren't right...
    m_pDasher->SetBoolParameter(BP_GAME_MODE, (checkState & MF_CHECKED) ? false : true);
    return 0;
  }
#endif
  case IDM_EXIT:
    DestroyWindow();
    return 0;
  case ID_EDIT_SELECTALL:
    m_pEdit->SelectAll();
    return 0;
  case ID_EDIT_CUT:
    m_pEdit->Cut();
    return 0;
  case ID_EDIT_COPY:
    m_pEdit->Copy();
    return 0;
  case ID_EDIT_COPY_ALL:
    m_pDasher->CopyToClipboard(m_pDasher->GetAllContext());
    return 0;
  case ID_EDIT_PASTE:
    m_pEdit->Paste();
    return 0;
  case ID_FILE_NEW:{
    m_pEdit->New();
    // Selecting file->new indicates a new trial to our user logging object
    CUserLogBase* pUserLog = m_pDasher->GetUserLogPtr();
    if (pUserLog != NULL)
      pUserLog->NewTrial();
    m_pDasher->SetBuffer(0);
    return 0;
  }
  case ID_FILE_OPEN:
    m_pEdit->Open();
    return 0;
  case ID_FILE_SAVE:
    if (!m_pEdit->Save())
      m_pEdit->SaveAs();
    return 0;
  case ID_FILE_SAVE_AS:
    m_pEdit->SaveAs();
    return 0;
  case ID_IMPORT_TRAINFILE:
    m_pDasher->ImportTrainingText(m_pEdit->Import());
    return 0;
  default:
    bHandled = FALSE;
    return 0;
  }
}

LRESULT CDasherWindow::OnDasherFocus(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (!m_bFullyCreated)
  {
    bHandled = FALSE;
    return 0;
  }

  m_pEdit->SetFocus();

  // TODO: Is this obsolete?
  HWND *pHwnd((HWND *)lParam);
  m_pEdit->SetKeyboardTarget(*pHwnd);
  return 0;
}

LRESULT CDasherWindow::OnDestroy(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  m_bFullyCreated = false;
  PostQuitMessage(0);
  return 0;
}

LRESULT CDasherWindow::OnGetMinMaxInfo(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (!m_bFullyCreated)
  {
    bHandled = FALSE;
    return 0;
  }

  RECT rect = { 0, 0, 0, 0 };
  int iStyle = m_pAppSettings->GetLongParameter(APP_LP_STYLE);

  rect.right = GetMinCanvasWidth();
  if (iStyle == APP_STYLE_COMPOSE)
    rect.right *= 2;

  rect.bottom = GetMinCanvasHeight() + m_pSpeedAlphabetBar->GetHeight();
  if (iStyle == APP_STYLE_TRAD)
    rect.bottom += GetMinEditHeight() + m_pSplitter->GetHeight();
  if (m_pAppSettings->GetBoolParameter(APP_BP_SHOW_TOOLBAR))
    rect.bottom += m_pToolbar->GetHeight();

  //min size including non-client area
  ::AdjustWindowRectEx(&rect, GetStyle(), GetMenu() != NULL, GetExStyle());
  LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
  mmi->ptMinTrackSize.x = rect.right - rect.left;
  mmi->ptMinTrackSize.y = rect.bottom - rect.top;
  return 0;
}

LRESULT CDasherWindow::OnInitMenuPopup(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  EnableMenuItem((HMENU)wParam, ID_EDIT_PASTE, IsClipboardFormatAvailable(CF_TEXT)?MF_ENABLED: MF_GRAYED);
  return 0;
}

LRESULT CDasherWindow::OnClose(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (m_pEdit->ConfirmAndSaveIfNeeded()) {
    DestroyWindow();
  }
  return 0;
}

LRESULT CDasherWindow::OnSize(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (wParam == SIZE_MINIMIZED)
    return 0;

  if (!m_bFullyCreated)
    return 0;

  m_pToolbar->Resize();
  Layout();
  return 0;
}


LRESULT CDasherWindow::OnSetFocus(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (!m_bFullyCreated)
    return 0;

  m_pDasher->TakeFocus();
  return 0;
}

LRESULT CDasherWindow::OnOther(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (!m_bFullyCreated)
    return 0;

  // A switch statement would be preferable, except the message ids are
  // not constant-expressions since they are provided by the system at
  // runtime.
  if (message == WM_DASHER_FOCUS)
    return OnDasherFocus(message, wParam, lParam, bHandled);

  return 0;
}
/* Handles Timer Callbacks*/
LRESULT CDasherWindow::OnTimer(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  //Brute force timer to update the external display with a copy of the current Dasher edit display
  //Timer should only be in use (created) by the Popup (when enabled)
  string currentOutput = m_pEdit->getOutput();
  m_pPopup->updateDisplay(currentOutput);
  return false;
}


void CDasherWindow::Layout() {
  if (!m_bFullyCreated)
    return;

  int iStyle(m_pAppSettings->GetLongParameter(APP_LP_STYLE));

  // Set up the window properties
  if (iStyle == APP_STYLE_DIRECT) {
    SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) | WS_EX_NOACTIVATE | WS_EX_APPWINDOW);
    SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetMenu(NULL);
  }
  else {
    SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) & !WS_EX_NOACTIVATE);
    SetWindowPos(HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetMenu(m_hMenu);
  }

  // Now do the actual layout
  RECT ClientRect;
  GetClientRect(&ClientRect);
  const int Width = ClientRect.right;
  const int Height = ClientRect.bottom;

  int ToolbarHeight = m_pAppSettings->GetBoolParameter(APP_BP_SHOW_TOOLBAR) ? m_pToolbar->GetHeight() : 0;

  int StatusbarHeight = m_pAppSettings->GetBoolParameter(APP_BP_SHOW_STATUSBAR) ? m_pSpeedAlphabetBar->GetHeight() : 0;
  m_pSpeedAlphabetBar->MoveWindow(0, Height - StatusbarHeight, Width, StatusbarHeight);
  m_pSpeedAlphabetBar->ShowWindow(StatusbarHeight ? SW_SHOW : SW_HIDE);

  bool mirrorLayout = m_pAppSettings->GetBoolParameter(APP_BP_MIRROR_LAYOUT);

  int CanvasHeight = Height - ToolbarHeight - StatusbarHeight;

  switch (iStyle)
  {
  case APP_STYLE_DIRECT:
    m_pDasher->Move(0, ToolbarHeight, Width, CanvasHeight);
    m_pEdit->ShowWindow(SW_HIDE);
    m_pSplitter->ShowWindow(SW_HIDE);
    break;

  case APP_STYLE_COMPOSE:
    if (mirrorLayout)
    {
      m_pDasher->Move(Width / 2, ToolbarHeight, Width - Width / 2 - 1, CanvasHeight - 1);
      m_pEdit->Move(0, ToolbarHeight, Width / 2, CanvasHeight);
    }
    else {
      m_pDasher->Move(1, ToolbarHeight, Width / 2 - 1, CanvasHeight - 1);
      m_pEdit->Move(Width / 2, ToolbarHeight, Width - Width / 2, CanvasHeight);
    }
    m_pEdit->ShowWindow(SW_SHOW);
    m_pPopup->setupPopup(); //Checks configuration and shows if enabled
    m_pSplitter->ShowWindow(SW_HIDE);
    break;

  default:
    int SplitterHeight = m_pSplitter->GetHeight();
    int EditHeight = m_pAppSettings->GetLongParameter(APP_LP_EDIT_SIZE);

    if (mirrorLayout)
    {
      if (m_pSplitter->IsSizing())
        EditHeight = ToolbarHeight + CanvasHeight - SplitterHeight - m_pSplitter->GetPos();
      int SplitterY = ToolbarHeight + CanvasHeight - SplitterHeight - EditHeight;
      SplitterY = min(SplitterY, ToolbarHeight + CanvasHeight - GetMinEditHeight() - SplitterHeight);
      SplitterY = max(ToolbarHeight + GetMinCanvasHeight(), SplitterY);
      EditHeight = ToolbarHeight + CanvasHeight - SplitterY - SplitterHeight;
      m_pDasher->Move(1, ToolbarHeight, Width - 2, SplitterY - ToolbarHeight);
      m_pSplitter->Move(SplitterY, Width);
      m_pEdit->Move(0, SplitterY + SplitterHeight, Width, EditHeight);
    }
    else {
      if (m_pSplitter->IsSizing())
        EditHeight = m_pSplitter->GetPos() - ToolbarHeight;
      int SplitterY = ToolbarHeight + EditHeight;
      SplitterY = min(SplitterY, ToolbarHeight + CanvasHeight - GetMinCanvasHeight() - SplitterHeight);
      SplitterY = max(ToolbarHeight + GetMinEditHeight(), SplitterY);
      EditHeight = SplitterY - ToolbarHeight;
      m_pDasher->Move(1, SplitterY + SplitterHeight, Width - 2, ToolbarHeight + CanvasHeight - SplitterY - SplitterHeight - 1);
      m_pEdit->Move(0, ToolbarHeight, Width, EditHeight);
      m_pSplitter->Move(SplitterY, Width);
    }
    m_pEdit->ShowWindow(SW_SHOW);
    m_pPopup->setupPopup(); //Checks configuration and shows if enabled
    m_pSplitter->ShowWindow(SW_SHOW);
    if (m_bSizeRestored)
      m_pAppSettings->SetLongParameter(APP_LP_EDIT_SIZE, EditHeight);
  }
}

LRESULT CDasherWindow::OnWindowPosChanged(UINT message, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (m_bSizeRestored) {
    // Save window state
    WINDOWPLACEMENT wp;
    wp.length = sizeof(wp);
    if (GetWindowPlacement(&wp)) {
      if (wp.showCmd != SW_SHOWMINIMIZED) {
        bool normal = wp.showCmd != SW_SHOWMAXIMIZED;
        if (normal)
        {
          LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;
          m_pAppSettings->SetLongParameter(APP_LP_X, pwp->x);
          m_pAppSettings->SetLongParameter(APP_LP_Y, pwp->y);
          m_pAppSettings->SetLongParameter(APP_LP_SCREEN_WIDTH, pwp->cx);
          m_pAppSettings->SetLongParameter(APP_LP_SCREEN_HEIGHT, pwp->cy);
        }
        m_pAppSettings->SetBoolParameter(APP_BP_FULL_SCREEN, !normal);
      }
    }
  }
  bHandled = false;
  return 1;
}

void CDasherWindow::HandleWinEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
  if (!m_bFullyCreated)
    return;

  // Ignore events if not in direct mode
  if (m_pAppSettings->GetLongParameter(APP_LP_STYLE) != APP_STYLE_DIRECT)
    return;

  // For now assume all events are focus changes, so reset the buffer
  m_pDasher->SetBuffer(0);
}
