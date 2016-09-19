#pragma once

#include "Common/wincommon.h"
#include "../DasherCore/DashIntfScreenMsgs.h"
#include "../DasherCore/UserLog.h"

#ifdef WIN32_SPEECH
#include <sapi.h>
#endif

#include <string>
#include <vector>

extern CONST UINT WM_DASHER_FOCUS;

class CCanvas;
class CEdit;
class CPopup;
class CDasherWindow;

namespace Dasher {
class CWinFileUtils :public CFileUtils {
public:
  virtual int GetFileSize(const std::string &strFileName) override;
  virtual void ScanFiles(AbstractParser *parser, const std::string &strPattern) override;
  bool WriteUserDataFile(const std::string &filename, const std::string &strNewText, bool append) override;
private:
  void ScanDirectory(const std::string &strMask, std::vector<std::string> &vFileList);
  // Returns location where program data is stored.
  // When user is false, result is system data location.
  // When user is true, result is user data data location.
  virtual std::string GetDataPath(bool user);

};

class CDasher : public CDashIntfScreenMsgs
{
public:
  CDasher(HWND Parent, CDasherWindow *pWindow, CEdit *pEdit, CPopup *pPopup, Dasher::CSettingsStore* settings, CFileUtils* fileUtils);
  ~CDasher(void);

  // The following functions will not be part of the final interface
  // In order to get rid of this, it'll be necessary to make this class
  // act more like a UI widget.

  //CCanvas *GetCanvas() {
	 // return m_pCanvas;
  //}

  // TODO: Can some of these be obtained by inheriting from something?
  void Move(int iX, int iY, int iWidth, int iHeight);
  void TakeFocus();

  void HandleEvent(int iParameter) override;
  void editOutput(const std::string &strText, CDasherNode *pSource) override;
  void editDelete(const std::string &strText, CDasherNode *pSource) override;
  unsigned int ctrlOffsetAfterMove(unsigned int offsetBefore, bool bForwards, CControlManager::EditDistance iDist) override;
  unsigned int ctrlMove(bool bForwards, CControlManager::EditDistance iDist) override;
  unsigned int ctrlDelete(bool bForwards, CControlManager::EditDistance iDist) override;
    
  void Main(); 

  virtual std::string GetAllContext() override;
  std::string GetContext(unsigned int iStart, unsigned int iLength) override;
  int GetAllContextLenght() override;
  std::string GetTextAroundCursor(CControlManager::EditDistance iDist) override;
 
#ifdef WIN32_SPEECH
  bool SupportsSpeech() override;
  void Speak(const std::string &text, bool bInterrupt) override;
#endif
  bool SupportsClipboard() override { return true; };
  void CopyToClipboard(const std::string &text) override;
  bool GetWindowSize(int* pTop, int* pLeft, int* pBottom, int* pRight);
  void configurePopupTimer(bool enable);
private:
  virtual void CreateModules() override;

  void ScanDirectory(const Tstring &strMask, std::vector<std::string> &vFileList);
  
  void                    Log();                        // Does the logging

  CCanvas *m_pCanvas;
  HWND m_hParent;
  CDasherWindow *m_pWindow;
  CEdit *m_pEdit;
  CPopup *m_pPopup;
#ifdef WIN32_SPEECH
  ISpVoice* getVoice(const string& lang);
  CComPtr<ISpVoice> m_pDefaultVoice;
  bool m_bAttemptedSpeech;
  map<string, CComPtr<ISpVoice> > m_voicesByLangCode;
#endif
};
}
