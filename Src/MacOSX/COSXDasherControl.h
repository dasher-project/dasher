/*
 *  COSXDasherControl.h
 *  Dasher
 *
 *  Created by Doug Dickinson on 17/11/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

#import "../DasherCore/SocketInput.h"

// #import "../DasherCore/DasherSettingsInterface.h"
#import "../DasherCore/DashIntfScreenMsgs.h"
#import "COSXMouseInput.h"

@class DasherApp;
@protocol DasherEdit;

///
/// \brief C++ core of the Dasher GTK UI component.
///
/// Class representing the Dasher UI component (ie the canvas and speed slider)
///

class COSXDasherControl : public CDashIntfScreenMsgs,CFileUtils {
  
public:
  
  ///
  /// \param pVBox GTK VBox to populate with the DasherControl
  /// component widgets. This needs to be created externally by the
  /// GObject wrapper and passed to the C++ class rather than being
  /// created internally.  
  /// \param pDasherControl Pointer to the GObject wrapper. This is
  /// needed so that we can emit signals from the GObject.
  ///
  
  COSXDasherControl(DasherApp *aDasherApp);
  ~COSXDasherControl();
  virtual int GetAllContextLenght();
  void Realize2();
  void TimerFired(NSPoint p);
  void Train(NSString *fileName);
  
  ///Wrappers round settings API for use by Objective-C...
  id GetParameter(NSString *aKey);
  void SetParameter(NSString *aKey, id aValue);
  int GetParameterIndex(const std::string & aKey);
  NSDictionary *ParameterDictionary();
  
  void goddamn(unsigned long iTime, bool bForceRedraw);
  virtual void WriteTrainFile(const std::string &filename, const std::string &strNewText);
	virtual bool WriteUserDataFile(const std::string &filename, const std::string &strNewText, bool append);
  std::string GetAllContext();
  void ClearAllContext();
  std::string GetContext(unsigned int iOffset, unsigned int iLength);
  virtual int GetFileSize(const std::string &strFileName);
  void EnterGameMode(CGameModule *pModule);
  void LeaveGameMode();
  void SetEdit(id<DasherEdit> pEdit);
  CGameModule *CreateGameModule();
private:
  virtual void ScanFiles(AbstractParser *parser, const std::string &strPattern);
  virtual void CreateModules();
  virtual bool SupportsSpeech();
  virtual void Speak(const std::string &strText, bool bInterrupt);
  virtual bool SupportsClipboard() {return true;}
  virtual void CopyToClipboard(const std::string &strText);
  ///Control-mode editing commands delegate to DasherEdit.
  unsigned int ctrlMove(bool bForwards, CControlManager::EditDistance dist);

  ///Control-mode editing commands delegate to DasherEdit.
  unsigned int ctrlDelete(bool bForwards, CControlManager::EditDistance dist);
  // No need to HandleEvent: the PreferencesController is observing changes to the 
  // user defaults controller which is observing the user defaults and will be notified when
  // the parameter is actually written by COSXSettingsStore.
  //void HandleEvent(int iParameter);

  ///Override to perform output/deletion via DasherEdit
  void editOutput(const std::string &strText, CDasherNode *pSource);
  void editDelete(const std::string &strText, CDasherNode *pSource);
  ///Just log (and call superclass)
  void editConvert(CDasherNode *pSource);
  void editProtect(CDasherNode *pSource);
  
  DasherApp *dasherApp;   // objc counterpart
  id<DasherEdit> dasherEdit;  // current output - sends to other apps or textfield
  
  COSXMouseInput *m_pMouseInput;
  COSX1DMouseInput *m_p1DMouseInput;

  NSString * const userDir;
};
