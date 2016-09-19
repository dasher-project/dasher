
#include "Common\WinCommon.h"          // must include pch first

#include "../DasherCore/ModuleManager.h"

#include "Dasher.h"
#include "../DasherCore/Event.h"
#include "Common\WinUTF8.h"
#include "Widgets/Canvas.h"
#include "DasherMouseInput.h"
#include "DasherWindow.h"
#include "Widgets/Edit.h"
#include "Widgets/Popup.h"

#include "Sockets/SocketInput.h"
#include "BTSocketInput.h"
#include <Sphelper.h>
#include <sys/stat.h>

using namespace std;
using namespace Dasher;
using namespace WinUTF8;

// Used to signal our message loop to do our periodic work, the value
// shouldn't collide with anything else in our code.
#define WM_DASHER_TIMER WM_USER + 128

CONST UINT WM_DASHER_FOCUS = RegisterWindowMessage(L"WM_DASHER_FOCUS");

CDasher::CDasher(HWND Parent, CDasherWindow *pWindow, CEdit *pEdit,CPopup *pPopup, Dasher::CSettingsStore* settings, CFileUtils* fileUtils)
  : CDashIntfScreenMsgs(settings, fileUtils), m_hParent(Parent), m_pWindow(pWindow), m_pEdit(pEdit), m_pPopup(pPopup) {
  // This class will be a wrapper for the Dasher 'control' - think ActiveX

  // Set up COM for the accessibility stuff
  CoInitialize(NULL);
#ifdef WIN32_SPEECH
  m_bAttemptedSpeech = false;
#endif

  DWORD dwTicks = GetTickCount();

  //The following was done in SetupUI, i.e. the first thing in Realize.
  // So doing here:
  m_pCanvas = new CCanvas(this);
  m_pCanvas->Create(m_hParent, GetStringParameter(SP_DASHER_FONT)); // TODO - check return 

  // Framerate settings: currently 40fps.
  SetTimer(m_pCanvas->getwindow(), 1, 25, NULL);

  Realize(dwTicks);
}

CDasher::~CDasher(void) {
  WriteTrainFileFull();
  delete m_pCanvas;
}

void CDasher::CreateModules() {
  //create default set first.
  CDasherInterfaceBase::CreateModules();
  RegisterModule(new CSocketInput(this,this));
  RegisterModule(new CBTSocketInput());
  RegisterModule(new CDasherMouseInput(m_pCanvas->getwindow()));
}

void CDasher::Main() {
  DWORD dwTicks = GetTickCount();
	NewFrame(dwTicks, false);
}

// Handles the work we need to do periodically on a timer event
void CDasher::Log() {

  CUserLogBase* pUserLog = GetUserLogPtr();

	// We'll use this timer event to periodically log the user's mouse position
	if ((pUserLog != NULL) && (m_pCanvas != NULL)) {
        // Get the mouse x and y coordinates
        POINT sMousePos;
        GetCursorPos(&sMousePos);
        
        // Since the everything is in screen relative coordinates, we'll
        // make sure we have the right coordinates for our canvas and
        // screen since the user may have move the window around.
        int     iTop     = 0;
        int     iLeft    = 0;
        int     iBottom  = 0;
        int     iRight   = 0;

        if (m_pCanvas->GetCanvasSize(iTop, iLeft, iBottom, iRight))
          pUserLog->AddCanvasSize(iTop, iLeft, iBottom, iRight);

        // Also update the size of the window in the UserLogTrial object
        if (GetWindowSize(&iTop, &iLeft, &iBottom, &iRight))
          pUserLog->AddWindowSize(iTop, iLeft, iBottom, iRight);

        // We'll store a normalized version so if the user changes the window
        // size during a trial, it won't effect our coordinates.  The 
        // normalization is with respect to the canvas and not the main 
        // window.        
        pUserLog->AddMouseLocationNormalized(sMousePos.x, 
                                             sMousePos.y, 
                                             true,
                                             (float) GetNats());
    }

}

void Dasher::CDasher::HandleEvent(int iParameter) {
  CDashIntfScreenMsgs::HandleEvent(iParameter);
  m_pWindow->HandleParameterChange(iParameter);
  m_pEdit->HandleParameterChange(iParameter);
  m_pPopup->HandleParameterChange(iParameter);
  if (iParameter == SP_DASHER_FONT)
    m_pCanvas->SetFont(GetStringParameter(SP_DASHER_FONT));
}

void Dasher::CDasher::editOutput(const string &strText, CDasherNode *pSource) {
  m_pEdit->output(strText);
  CDasherInterfaceBase::editOutput(strText, pSource);
}

void Dasher::CDasher::editDelete(const string &strText, CDasherNode *pSource) {
  m_pEdit->deletetext(strText);
  CDasherInterfaceBase::editDelete(strText, pSource);
}

unsigned int Dasher::CDasher::ctrlOffsetAfterMove(unsigned int offsetBefore, bool bForwards, CControlManager::EditDistance iDist) {
  return m_pEdit->OffsetAfterMove(offsetBefore, bForwards, iDist);
}

unsigned int Dasher::CDasher::ctrlMove(bool bForwards, CControlManager::EditDistance iDist) {
  return m_pEdit->Move(bForwards, iDist);
}

unsigned int Dasher::CDasher::ctrlDelete(bool bForwards, CControlManager::EditDistance iDist) {
  return m_pEdit->Delete(bForwards, iDist);
}

// Gets the size of the window in screen coordinates.  
bool Dasher::CDasher::GetWindowSize(int* pTop, int* pLeft, int* pBottom, int* pRight) {
  if ((pTop == NULL)||(pLeft == NULL)||(pBottom == NULL)||(pRight == NULL))
    return false;

  RECT sWindowRect;
  if (GetWindowRect(m_hParent, &sWindowRect))
  {
    *pTop    = sWindowRect.top;
    *pLeft   = sWindowRect.left;
    *pBottom = sWindowRect.bottom;
    *pRight  = sWindowRect.right;
    return true;
  }
  else
    return false;
}

bool CWinFileUtils::WriteUserDataFile(const std::string &filename, const std::string &strNewText, bool append) {
  if (strNewText.size() == 0)
    return true;

  string fullpath = GetDataPath(true) + filename;
  auto CreationDisposition = append ? OPEN_ALWAYS : CREATE_ALWAYS;
  HANDLE hFile = CreateFile(UTF8string_to_wstring(fullpath).c_str(),
	  GENERIC_WRITE, 0, NULL,
	  CreationDisposition,
	  FILE_ATTRIBUTE_NORMAL, 0);

  if(hFile == INVALID_HANDLE_VALUE) {
    OutputDebugString(TEXT("Can not open file\n"));
    return false;
  }
    DWORD NumberOfBytesWritten;
    SetFilePointer(hFile, 0, NULL, FILE_END);
    WriteFile(hFile, &strNewText.c_str()[0], strNewText.size(), &NumberOfBytesWritten, NULL);
    CloseHandle(hFile);

    return NumberOfBytesWritten == strNewText.size();
}

void CWinFileUtils::ScanDirectory(const string &strMask, std::vector<std::string> &vFileList) {
  using namespace WinUTF8;
  WIN32_FIND_DATA find;
  wstring wideMask = UTF8string_to_wstring(strMask);
  HANDLE handle = FindFirstFile(wideMask.c_str(), &find);
  if(handle != INVALID_HANDLE_VALUE) {
    vFileList.push_back(wstring_to_UTF8string(find.cFileName));
    while(FindNextFile(handle, &find) != false) {
		vFileList.push_back(wstring_to_UTF8string(find.cFileName));
    }
    FindClose(handle);
  }
}

void CWinFileUtils::ScanFiles(AbstractParser *parser, const std::string &strPattern) {
  using namespace WinHelper;
  using namespace WinUTF8;
  
  std::vector<std::string> vFileList;

  string sysDir = GetDataPath(false);
  ScanDirectory(sysDir +strPattern, vFileList);
  for (vector<std::string>::iterator it=vFileList.begin(); it!=vFileList.end(); it++)
    parser->ParseFile(sysDir + (*it),false);

  vFileList.clear();
  
  string userDir = GetDataPath(true);
  ScanDirectory(userDir + strPattern, vFileList); 
  for (vector<std::string>::iterator it=vFileList.begin(); it!=vFileList.end(); it++)
    parser->ParseFile(userDir + (*it),true);
}

std::string CWinFileUtils::GetDataPath(bool user) {
	using namespace WinHelper;
	using namespace WinUTF8;
	wstring DataPath;
	if (user) {
		GetUserDirectory(&DataPath);
		DataPath += TEXT("dasher.rc\\");
	}
	else {
		GetAppDirectory(&DataPath);
		DataPath += TEXT("system.rc\\");

	}
	CreateDirectory(DataPath.c_str(), NULL);// TODO: Any harm if they already exist

	return wstring_to_UTF8string(DataPath.c_str());
}

int CWinFileUtils::GetFileSize(const std::string &strFileName) {
  struct _stat sStatInfo;
  _stat(strFileName.c_str(), &sStatInfo);
  return sStatInfo.st_size;
}

// TODO: Check that syntax here is sensible
void CDasher::Move(int iX, int iY, int iWidth, int iHeight) {
  if(m_pCanvas)
    m_pCanvas->Move(iX, iY, iWidth, iHeight);
}

void CDasher::TakeFocus() {
  // TODO: Implement me
}
#ifdef WIN32_SPEECH
bool CDasher::SupportsSpeech() {
  if (!m_bAttemptedSpeech) {
    //try to create speech synthesizer lazily, saving resources if no speech req'd.
    HRESULT hr = m_pDefaultVoice.CoCreateInstance(CLSID_SpVoice);
    if (SUCCEEDED(hr)) {
      // First speak takes long time. 
      m_pDefaultVoice->Speak(L"",SPF_ASYNC,NULL);
      m_voicesByLangCode[""] = m_pDefaultVoice;
    }
    m_bAttemptedSpeech = true;
    // TODO find language code and add entry to map
  }
  return m_pDefaultVoice != 0;
}

ISpVoice* CDasher::getVoice(const string& lang)
{
  auto it = m_voicesByLangCode.find(lang);
  if (it != m_voicesByLangCode.end())
    return it->second;

  wstring wideLang = UTF8string_to_wstring(lang);
  long  lcid = LocaleNameToLCID(wideLang.c_str(), 0);
  if (lcid) {
    CString langFilter;
    langFilter.Format(L"Language=%lx", lcid);
    CComPtr<ISpObjectToken> cpToken;
    HRESULT hr = SpFindBestToken(SPCAT_VOICES, L"", langFilter, &cpToken);
    if (SUCCEEDED(hr)) {
      CComPtr<ISpVoice> pVoice;
      HRESULT hr = pVoice.CoCreateInstance(CLSID_SpVoice);
      if (SUCCEEDED(hr)) {
        hr = pVoice->SetVoice(cpToken);
        if (SUCCEEDED(hr)) {
          m_voicesByLangCode[lang] = pVoice;
          return pVoice;
        }
      }
    }
  }
  m_voicesByLangCode[lang] = m_pDefaultVoice;
  return m_pDefaultVoice;
}

void CDasher::Speak(const string &strText, bool bInterrupt) {
  if (!m_pDefaultVoice)
    return;

  string lang = GetActiveAlphabet()->GetLanguageCode();
  Tstring wideText;
  UTF8string_to_wstring(strText, wideText);
  int flags = SPF_ASYNC;
  if (bInterrupt)
    flags |= SPF_PURGEBEFORESPEAK;
  getVoice(lang)->Speak(wideText.c_str(), flags, NULL);
}
#endif

void CDasher::CopyToClipboard(const string &strText) {
  if (OpenClipboard(m_hParent))
  {
    EmptyClipboard(); //also frees memory containing any previous data
	Tstring wideText;
	UTF8string_to_wstring(strText, wideText);

    //Allocate global memory for string - enough for characters + NULL.
    HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, sizeof(WCHAR)*(wideText.length()+1));
    
    //GlobalLock returns a pointer to the data associated with the handle returned from GlobalAlloc    
	LPWSTR pchData = (LPWSTR)GlobalLock(hClipboardData);

    //now fill it...
	wcscpy(pchData, wideText.c_str());
    
    // Unlock memory, i.e. release our access to it - 
    // but don't free it (with GlobalFree), as it will "belong"
    // to the clipboard.
    GlobalUnlock(hClipboardData);
    
    //Now, point the clipboard at that global memory...
    SetClipboardData(CF_UNICODETEXT,hClipboardData);
    
    //Finally, unlock the clipboard (i.e. a pointer to the data on it!)
    // so that other applications can see / modify it
    CloseClipboard();
  }
}

std::string CDasher::GetAllContext() {
  CString wideText;
  m_pEdit->GetWindowText(wideText);
  return WinUTF8::wstring_to_UTF8string(wideText);
}

std::string CDasher::GetContext(unsigned int iStart, unsigned int iLength) {
  CString wideText;
  m_pEdit->GetWindowText(wideText);
  return WinUTF8::wstring_to_UTF8string(wideText.Mid(iStart, iLength));
}

int CDasher::GetAllContextLenght(){
  return m_pEdit->GetWindowTextLength();
}

std::string CDasher::GetTextAroundCursor(CControlManager::EditDistance iDist) {
  return m_pEdit->GetTextAroundCursor(iDist);
}

void CDasher::configurePopupTimer(bool enable) {
	//Timer is managed by the window object
	m_pWindow->configurePopupTimer(enable);
}