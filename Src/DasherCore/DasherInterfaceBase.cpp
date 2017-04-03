// DasherInterfaceBase.cpp
//
// Copyright (c) 2008 The Dasher Team
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

#include "../Common/Common.h"

#include "DasherInterfaceBase.h"

#include "DasherViewSquare.h"
#include "ControlManager.h"
#include "DasherScreen.h"
#include "DasherView.h"
#include "DasherInput.h"
#include "DasherModel.h"
#include "Observable.h"
#include "Event.h"
#include "NodeCreationManager.h"
#include "UserLog.h"
#include "BasicLog.h"
#include "GameModule.h"
#include "FileWordGenerator.h"

// Input filters
#include "AlternatingDirectMode.h"
#include "ButtonMode.h"
#include "ClickFilter.h"
#include "CompassMode.h"
#include "DefaultFilter.h"
#include "DemoFilter.h"
#include "OneButtonFilter.h"
#include "OneButtonDynamicFilter.h"
#include "OneDimensionalFilter.h"
#include "StylusFilter.h"
#include "TwoButtonDynamicFilter.h"
#include "TwoPushDynamicFilter.h"

// STL headers
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>

// Declare our global file logging object
#include "../DasherCore/FileLogger.h"
#ifdef _DEBUG
const eLogLevel g_iLogLevel   = logDEBUG;
const int       g_iLogOptions = logTimeStamp | logDateStamp | logDeleteOldFile;
#else
const eLogLevel g_iLogLevel   = logNORMAL;
const int       g_iLogOptions = logTimeStamp | logDateStamp;
#endif

CFileLogger* g_pLogger = NULL;

using namespace Dasher;
using namespace std;

// Track memory leaks on Windows to the line that new'd the memory
#ifdef _WIN32
#ifdef _DEBUG_MEMLEAKS
#define DEBUG_NEW new( _NORMAL_BLOCK, THIS_FILE, __LINE__ )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CDasherInterfaceBase::CDasherInterfaceBase(CSettingsStore *pSettingsStore, CFileUtils* fileUtils) 
  : CSettingsUser(pSettingsStore), 
  m_pDasherModel(new CDasherModel()), 
  m_pFramerate(new CFrameRate(this)), 
  m_pSettingsStore(pSettingsStore), 
  m_pLockLabel(NULL),
  m_preSetObserver(*pSettingsStore),
  m_bLastMoved(false) {
  
  pSettingsStore->Register(this);
  pSettingsStore->PreSetObservable().Register(&m_preSetObserver);

  m_fileUtils = fileUtils;
  
  // Ensure that pointers to 'owned' objects are set to NULL.
  m_DasherScreen = NULL;
  m_pDasherView = NULL;
  m_pInput = NULL;
  m_pInputFilter = NULL;
  m_AlphIO = NULL;
  m_ColourIO = NULL;
  m_ControlBoxIO = NULL;
  m_pUserLog = NULL;
  m_pNCManager = NULL;
  m_defaultPolicy = NULL;
  m_pWordSpeaker = NULL;
  m_pGameModule = NULL;

  // Various state variables
  m_bRedrawScheduled = false;

  //  m_bGlobalLock = false;

  // Global logging object we can use from anywhere
  g_pLogger = new CFileLogger("dasher.log",
                              g_iLogLevel,
                              g_iLogOptions);
}

void CDasherInterfaceBase::Realize(unsigned long ulTime) {

  //if ChangeScreen has been called, we'll have created a view;
  // otherwise, we still can't create a view, until we have a screen!
  DASHER_ASSERT(m_DasherScreen ? m_pDasherView!=NULL : m_pDasherView==NULL);

  srand(ulTime);
 
  m_AlphIO = new CAlphIO(this);
  ScanFiles(m_AlphIO, "alphabet*.xml");

  m_ColourIO = new CColourIO(this);
  ScanFiles(m_ColourIO, "colour*.xml");

  m_ControlBoxIO = new CControlBoxIO(this);
  ScanFiles(m_ControlBoxIO, "control*.xml");

  ChangeColours();

  ChangeView();
  // Create the user logging object if we are suppose to.  We wait
  // until now so we have the real value of the parameter and not
  // just the default.

  // TODO: Sort out log type selection

  int iUserLogLevel = GetLongParameter(LP_USER_LOG_LEVEL_MASK);

  if(iUserLogLevel == 10)
    m_pUserLog = new CBasicLog(this, this);
  else if (iUserLogLevel > 0)
    m_pUserLog = new CUserLog(this, this, iUserLogLevel);

  CreateModules();

  ChangeAlphabet(); // This creates the NodeCreationManager, the Alphabet,
  //and the tree of nodes in the model.

  CreateInput();
  CreateInputFilter();
  //we may have created a control manager already; in which case, we need
  // it to realize there's now an inputfilter (which may provide more actions).
  // So tell it the setting has changed...
  if (CControlManager *pCon = m_pNCManager->GetControlManager())
    pCon->HandleEvent(SP_INPUT_FILTER);

  HandleEvent(LP_NODE_BUDGET);
  HandleEvent(BP_SPEAK_WORDS);

  // FIXME - need to rationalise this sort of thing.
  // InvalidateContext(true);
  ScheduleRedraw();

  // All the setup is done by now, so let the user log object know
  // that future parameter changes should be logged.
  if (m_pUserLog != NULL)
    m_pUserLog->InitIsDone();
}

CDasherInterfaceBase::~CDasherInterfaceBase() {
  //WriteTrainFileFull();???
  delete m_pDasherModel;        // The order of some of these deletions matters
  delete m_pDasherView;
  delete m_ControlBoxIO;
  delete m_ColourIO;
  delete m_AlphIO;
  delete m_pNCManager;
  // Do NOT delete Edit box or Screen. This class did not create them.

  // When we destruct on shutdown, we'll output any detailed log file
  if (m_pUserLog != NULL)
  {
    m_pUserLog->OutputFile();
    delete m_pUserLog;
    m_pUserLog = NULL;
  }

  if (g_pLogger != NULL) {
    delete g_pLogger;
    g_pLogger = NULL;
  }

  delete m_pFramerate;
}

void CDasherInterfaceBase::CPreSetObserver::HandleEvent(CParameterChange d) {
  switch(d.iParameter) {
  case SP_ALPHABET_ID:
    string value = d.string_value;
    // Cycle the alphabet history
    vector<string> newHistory;
    newHistory.push_back(m_settingsStore.GetStringParameter(SP_ALPHABET_ID));
    string v;
    if ((v = m_settingsStore.GetStringParameter(SP_ALPHABET_1)) != value)
      newHistory.push_back(v);
    if ((v = m_settingsStore.GetStringParameter(SP_ALPHABET_2)) != value)
      newHistory.push_back(v);
    if ((v = m_settingsStore.GetStringParameter(SP_ALPHABET_3)) != value)
      newHistory.push_back(v);
    if ((v = m_settingsStore.GetStringParameter(SP_ALPHABET_4)) != value)
      newHistory.push_back(v);

    // Fill empty slots. 
    while (newHistory.size() < 4)
      newHistory.push_back("");

    m_settingsStore.SetStringParameter(SP_ALPHABET_1, newHistory[0]);
    m_settingsStore.SetStringParameter(SP_ALPHABET_2, newHistory[1]);
    m_settingsStore.SetStringParameter(SP_ALPHABET_3, newHistory[2]);
    m_settingsStore.SetStringParameter(SP_ALPHABET_4, newHistory[3]);
    break;
  }
}

void CDasherInterfaceBase::HandleEvent(int iParameter) {
  switch (iParameter) {

  case LP_OUTLINE_WIDTH:
    ScheduleRedraw();
    break;
  case BP_DRAW_MOUSE:
    ScheduleRedraw();
    break;
  case BP_DRAW_MOUSE_LINE:
    ScheduleRedraw();
    break;
  case LP_ORIENTATION:
    m_pDasherView->SetOrientation(ComputeOrientation());
    ScheduleRedraw();
    break;
  case SP_ALPHABET_ID:
    ChangeAlphabet();
    ScheduleRedraw();
    break;
  case SP_COLOUR_ID:
    ChangeColours();
    ScheduleRedraw();
    break;
  case BP_PALETTE_CHANGE:
    if(GetBoolParameter(BP_PALETTE_CHANGE))
 SetStringParameter(SP_COLOUR_ID, m_pNCManager->GetAlphabet()->GetPalette());
    break;
  case LP_LANGUAGE_MODEL_ID:
    CreateNCManager();
    break;
  case LP_LINE_WIDTH:
    ScheduleRedraw();
    break;
  case LP_DASHER_FONTSIZE:
    ScheduleRedraw();
    break;
  case SP_INPUT_DEVICE:
    CreateInput();
    break;
  case SP_INPUT_FILTER:
    CreateInputFilter();
    ScheduleRedraw();
    break;
  case LP_MARGIN_WIDTH:
  case BP_NONLINEAR_Y:
  case LP_NONLINEAR_X:
  case LP_GEOMETRY:
  case LP_SHAPE_TYPE: //for platforms which actually have this as a GUI pref!
      ScheduleRedraw();
      break;
  case LP_NODE_BUDGET:
    delete m_defaultPolicy;
    m_defaultPolicy = new AmortizedPolicy(m_pDasherModel,GetLongParameter(LP_NODE_BUDGET));
    break;
  case BP_SPEAK_WORDS:
    delete m_pWordSpeaker;
    m_pWordSpeaker = GetBoolParameter(BP_SPEAK_WORDS) ? new WordSpeaker(this) : NULL;
    break;
  case BP_CONTROL_MODE:
  case SP_CONTROL_BOX_ID:
    // force rebuilding every node. If not control box is accessed after delete.
    CreateNCManager();
    break;
  default:
    break;
  }
}

void CDasherInterfaceBase::EnterGameMode(CGameModule *pGameModule) {
  DASHER_ASSERT(m_pGameModule == NULL);
  if (CWordGeneratorBase *pWords = m_pNCManager->GetAlphabetManager()->GetGameWords()) {
    if (!pGameModule) pGameModule=CreateGameModule();
    m_pGameModule=pGameModule;
    //m_pNCManager->updateControl();
    m_pGameModule->SetWordGenerator(m_pNCManager->GetAlphabet(), pWords);
  } else {
    ///TRANSLATORS: %s is the name of the alphabet; the string "GameTextFile"
    /// refers to a setting name in gsettings or equivalent, and should not be translated.
    FormatMessageWithString(_("Could not find game sentences file for %s - check alphabet definition, or override with GameTextFile setting"),
                            m_pNCManager->GetAlphabet()->GetID().c_str());
    delete pGameModule; //does nothing if null.
  }
}

void CDasherInterfaceBase::LeaveGameMode() {
  DASHER_ASSERT(m_pGameModule);
  CGameModule *pMod = m_pGameModule;
  m_pGameModule=NULL; //point at which we officially exit game mode
  delete pMod;
  //m_pNCManager->updateControl();
  SetBuffer(0);
}

CDasherInterfaceBase::WordSpeaker::WordSpeaker(CDasherInterfaceBase *pIntf) : TransientObserver<const CEditEvent *>(pIntf) {
}

void CDasherInterfaceBase::WordSpeaker::HandleEvent(const CEditEvent *pEditEvent) {
  CDasherInterfaceBase *pIntf(static_cast<CDasherInterfaceBase *> (m_pEventHandler));
  if (pIntf->GetGameModule()) return;
  if(pEditEvent->m_iEditType == 1) {
    if (pIntf->SupportsSpeech()) {
      const CAlphInfo *pAlphabet = pIntf->m_pNCManager->GetAlphabet();
      if (pEditEvent->m_sText == pAlphabet->GetText(pAlphabet->GetSpaceSymbol())) {
        pIntf->Speak(m_strCurrentWord, false);
        m_strCurrentWord="";
      } else
        m_strCurrentWord+=pEditEvent->m_sText;
    }
  }
  else if(pEditEvent->m_iEditType == 2) {
    m_strCurrentWord = m_strCurrentWord.substr(0, max(static_cast<string::size_type>(0), m_strCurrentWord.size()-pEditEvent->m_sText.size()));
  }
}

void CDasherInterfaceBase::SetLockStatus(const string &strText, int iPercent) {
  string newMessage; //empty - what we want if iPercent==-1 (unlock)
  if (iPercent!=-1) {
    ostringstream os;
    os << (strText.empty() ? "Training Dasher" : strText);
    if (iPercent) os << " " << iPercent << "%";
    newMessage = os.str();
  }
  if (newMessage != m_strLockMessage) {
    ScheduleRedraw();
    if (m_pLockLabel) {
      delete m_pLockLabel;
      m_pLockLabel = NULL;
    }
    m_strLockMessage = newMessage;
  }
}

void CDasherInterfaceBase::editOutput(const std::string &strText, CDasherNode *pCause) {
  CEditEvent evt(CEditEvent::EDIT_OUTPUT, strText, pCause);
  DispatchEvent(&evt);
}

void CDasherInterfaceBase::editDelete(const std::string &strText, CDasherNode *pCause) {
  CEditEvent evt(CEditEvent::EDIT_DELETE, strText, pCause);
  DispatchEvent(&evt);
}

void CDasherInterfaceBase::editConvert(CDasherNode *pCause) {
  CEditEvent evt(CEditEvent::EDIT_CONVERT, "", pCause);
  DispatchEvent(&evt);
}

void CDasherInterfaceBase::editProtect(CDasherNode *pCause) {
  CEditEvent evt(CEditEvent::EDIT_PROTECT, "", pCause);
  DispatchEvent(&evt);
}

void CDasherInterfaceBase::WriteTrainFileFull() {
  m_pNCManager->GetAlphabetManager()->WriteTrainFileFull(this);
}

void CDasherInterfaceBase::CreateNCManager() {

  if(!m_AlphIO || GetLongParameter(LP_LANGUAGE_MODEL_ID)==-1)
    return;

  //can't delete the old manager yet until we've deleted all its nodes...
  CNodeCreationManager *pOldMgr = m_pNCManager;

  //now create the new manager...
  m_pNCManager = new CNodeCreationManager(this, this, m_AlphIO, m_ControlBoxIO);
  if (GetBoolParameter(BP_PALETTE_CHANGE))
    SetStringParameter(SP_COLOUR_ID, m_pNCManager->GetAlphabet()->GetPalette());

  if (m_DasherScreen) {
    m_pNCManager->ChangeScreen(m_DasherScreen);
    //and start a new tree of nodes from it (retaining old offset -
    // this will be a sensible default of 0 if no nodes previously existed).
    // This deletes the old tree of nodes...
    SetOffset(m_pDasherModel->GetOffset(), true);
  } //else, if there is no screen, the model should not contain any nodes from the old NCManager. (Assert, somehow?)

  //...so now we can delete the old manager
  delete pOldMgr;
}

CDasherInterfaceBase::TextAction::TextAction(CDasherInterfaceBase *pIntf) : m_pIntf(pIntf) {
  m_iStartOffset= pIntf->GetAllContextLenght();
  pIntf->m_vTextActions.insert(this);
}

CDasherInterfaceBase::TextAction::~TextAction() {
  m_pIntf->m_vTextActions.erase(this);
}

void CDasherInterfaceBase::TextAction::executeOnDistance(CControlManager::EditDistance dist) {
  (*this)(strLast = m_pIntf->GetTextAroundCursor(dist));
  m_iStartOffset = m_pIntf->GetAllContextLenght();
}

void CDasherInterfaceBase::TextAction::executeOnNew() {
  (*this)(strLast = m_pIntf->GetContext(m_iStartOffset, m_pIntf->GetAllContextLenght() - m_iStartOffset));
  m_iStartOffset = m_pIntf->GetAllContextLenght();
}

void CDasherInterfaceBase::TextAction::executeLast() {
  (*this)(strLast);
}

void CDasherInterfaceBase::TextAction::NotifyOffset(int iOffset) {
  m_iStartOffset = min(m_pIntf->GetAllContextLenght(), m_iStartOffset);
}


bool CDasherInterfaceBase::hasDone() {
  return (GetBoolParameter(BP_COPY_ALL_ON_STOP) && SupportsClipboard())
  || (GetBoolParameter(BP_SPEAK_ALL_ON_STOP) && SupportsSpeech());
}

void CDasherInterfaceBase::Done() {
  ScheduleRedraw();

  if (m_pUserLog != NULL)
    m_pUserLog->StopWriting((float) GetNats());

  if (GetBoolParameter(BP_COPY_ALL_ON_STOP) && SupportsClipboard()) {
    CopyToClipboard(GetAllContext());
  }
  if (GetBoolParameter(BP_SPEAK_ALL_ON_STOP) && SupportsSpeech()) {
    Speak(GetAllContext(), true);
  }
}

void CDasherInterfaceBase::CreateInput() {
  if(m_pInput) {
    m_pInput->Deactivate();
  }

  m_pInput = (CDasherInput *)GetModuleByName(GetStringParameter(SP_INPUT_DEVICE));

  if (m_pInput == NULL)
    m_pInput = m_oModuleManager.GetDefaultInputDevice();

  if(m_pInput) {
    m_pInput->Activate();
  }
}

void CDasherInterfaceBase::NewFrame(unsigned long iTime, bool bForceRedraw) {
  // Prevent NewFrame from being reentered. This can happen occasionally and
  // cause crashes.
  static bool bReentered=false;
  if (bReentered) {
#ifdef DEBUG
    std::cout << "CDasherInterfaceBase::NewFrame was re-entered" << std::endl;
#endif
    return;
  }
  bReentered=true;

  if(m_DasherScreen) {
    //ok, can draw _something_. Try and see what we can :).

    bool bBlit = false; //set to true if we actually render anything different i.e. that needs blitting to display

    if (isLocked() || !m_pDasherView) {
      //Hmmm. If we're locked, NewFrame is never actually called - the thread
      // that would be rendering frames, is the same one doing the training.
      // So the following is never actually executed atm, but may be a simple
      // template if/when we ever implement multithreading widely/properly...
      m_DasherScreen->SendMarker(0); //this replaces the nodes...
      const screenint iSW = m_DasherScreen->GetWidth(), iSH = m_DasherScreen->GetHeight();
      m_DasherScreen->DrawRectangle(0,0,iSW,iSH,0,0,0); //fill in colour 0 = white
      unsigned int iSize(GetLongParameter(LP_MESSAGE_FONTSIZE));
      if (!m_pLockLabel) m_pLockLabel = m_DasherScreen->MakeLabel(m_strLockMessage, iSize);
      pair<screenint,screenint> dims = m_DasherScreen->TextSize(m_pLockLabel, iSize);
      m_DasherScreen->DrawString(m_pLockLabel, (iSW-dims.first)/2, (iSH-dims.second)/2, iSize, 4);
      m_DasherScreen->SendMarker(1); //decorations - don't draw any
      bBlit = true;
    } else {
      CExpansionPolicy *pol=m_defaultPolicy;
  
      //1. Schedule any per-frame movement in the model...
      if(m_pInputFilter) {
        m_pInputFilter->Timer(iTime, m_pDasherView, m_pInput, m_pDasherModel, &pol);
      }
      //2. Render...

      //If we've been told to render another frame via ScheduleRedraw,
      // that's the same as passing in true to NewFrame.
      if (m_bRedrawScheduled) bForceRedraw=true;
      m_bRedrawScheduled=false;

      //Apply any movement that has been scheduled
      if (m_pDasherModel->NextScheduledStep()) {
        //yes, we moved...
        if (!m_bLastMoved) onUnpause(iTime);
        // ...so definitely need to render the nodes. We also make sure
        // to render at least one more frame - think that's a bit of policy
        // just to be on the safe side, and may not be strictly necessary...
        bForceRedraw=m_bRedrawScheduled=m_bLastMoved=true;
      } else {
        //no movement
        if (m_bLastMoved) bForceRedraw=true;//move into onPause() method if reqd
        m_bLastMoved=false;
      }
      //2. Render nodes decorations, messages
      bBlit = Redraw(iTime, bForceRedraw, *pol);

      if (m_pUserLog != NULL) {
        //(any) UserLogBase will have been watching output events to gather information
        // about symbols added/deleted; this tells it to apply that information at end-of-frame
        // (previously DashIntf gathered the info, and then passed it to the logger here).
        m_pUserLog->FrameEnded();
      }
    }
    if (FinishRender(iTime)) bBlit = true;
    if (bBlit) m_DasherScreen->Display();
  }

  bReentered=false;
}

void CDasherInterfaceBase::onUnpause(unsigned long lTime) {
  //TODO When Game+UserLog modules are combined => reduce to just one call here
  if (m_pGameModule)
    m_pGameModule->StartWriting(lTime);
  if (m_pUserLog)
      m_pUserLog->StartWriting();
}

bool CDasherInterfaceBase::Redraw(unsigned long ulTime, bool bRedrawNodes, CExpansionPolicy &policy) {
  DASHER_ASSERT(m_pDasherView);

  // Draw the nodes
  if(bRedrawNodes) {
    m_pDasherView->Screen()->SendMarker(0);
    if (m_pDasherModel) {
      m_pDasherModel->RenderToView(m_pDasherView,policy);
      // if anything was expanded or collapsed render at least one more
      // frame after this
      if (policy.apply())
        ScheduleRedraw();
    }
    if(m_pGameModule) {
      m_pGameModule->DecorateView(ulTime, m_pDasherView, m_pDasherModel);
    }          
  }
  //From here on, we'll use bRedrawNodes just to denote whether we need to blit the display...

  // Draw the decorations
  m_pDasherView->Screen()->SendMarker(1);


  if(m_pInputFilter) {
    if (m_pInputFilter->DecorateView(m_pDasherView, m_pInput)) bRedrawNodes=true;
  }
  
  return bRedrawNodes;

}

void CDasherInterfaceBase::ChangeAlphabet() {
  if(GetStringParameter(SP_ALPHABET_ID) == "") {
    SetStringParameter(SP_ALPHABET_ID, m_AlphIO->GetDefault());
    // This will result in ChangeAlphabet() being called again, so
    // exit from the first recursion
    return;
  }

  if (m_pNCManager) WriteTrainFileFull(); //can't/don't before creating first NCManager

  // Send a lock event

  // Lock Dasher to prevent changes from happening while we're training.

  CreateNCManager();
  if (m_pDasherView) m_pDasherView->SetOrientation(ComputeOrientation());
  // Apply options from alphabet

  //}
}

Opts::ScreenOrientations CDasherInterfaceBase::ComputeOrientation() {
  Opts::ScreenOrientations pref(Opts::ScreenOrientations(GetLongParameter(LP_ORIENTATION)));
  if (pref!=Opts::AlphabetDefault) return pref;
  if (m_pNCManager) return m_pNCManager->GetAlphabet()->GetOrientation();
  //haven't created the NCManager yet, so not yet reached Realize, but must
  // have been given Screen (to make View). Use default LR for now, as when
  // we ChangeAlphabet, we'll update the view.
  return Opts::LeftToRight;
}

void CDasherInterfaceBase::ChangeColours() {
  if(!m_ColourIO || !m_DasherScreen)
    return;

  // TODO: Make fuction return a pointer directly
  m_DasherScreen->SetColourScheme(&(m_ColourIO->GetInfo(GetStringParameter(SP_COLOUR_ID))));
}

void CDasherInterfaceBase::ChangeScreen(CDasherScreen *NewScreen) {
  
  m_DasherScreen = NewScreen;
  ChangeColours();
  
  if(m_pDasherView != 0) {
    m_pDasherView->ChangeScreen(NewScreen);
    ScreenResized(NewScreen);
  } else {
    //We can create the view as soon as we have a screen...
    ChangeView();
  }
  
  if (m_pNCManager) {
    m_pNCManager->ChangeScreen(m_DasherScreen);
    if (m_pDasherModel)
      SetOffset(m_pDasherModel->GetOffset(), true);
  }
}

void CDasherInterfaceBase::ScreenResized(CDasherScreen *pScreen) {
  DASHER_ASSERT(pScreen == m_DasherScreen);
  if (!m_pDasherView) return;
  m_pDasherView->ScreenResized(m_DasherScreen);

  //Really, would like to do a Redraw _immediately_, but this will have to do.
  ScheduleRedraw();
}

void CDasherInterfaceBase::ChangeView() {
  if(m_DasherScreen != 0 /*&& m_pDasherModel != 0*/) {
    CDasherView *pNewView = new CDasherViewSquare(this, m_DasherScreen, ComputeOrientation());
    //the previous sends an event to all listeners registered with it, but there aren't any atm!
    // so send an event to tell them of the new view object _and_ get them to recompute coords:  
    if (m_pDasherView) m_pDasherView->TransferObserversTo(pNewView);
    delete m_pDasherView;

    m_pDasherView = pNewView;
  }
  ScheduleRedraw();
}

double CDasherInterfaceBase::GetCurCPM() {
  //
  return 0;
}

double CDasherInterfaceBase::GetCurFPS() {
  //
  return 0;
}

const CAlphInfo *CDasherInterfaceBase::GetActiveAlphabet() {
  return m_AlphIO->GetInfo(GetStringParameter(SP_ALPHABET_ID));
}

// int CDasherInterfaceBase::GetAutoOffset() {
//   if(m_pDasherView != 0) {
//     return m_pDasherView->GetAutoOffset();
//   }
//   return -1;
// }

double CDasherInterfaceBase::GetNats() const {
  if(m_pDasherModel)
    return m_pDasherModel->GetNats();
  else
    return 0.0;
}

void CDasherInterfaceBase::ResetNats() {
  if(m_pDasherModel)
    m_pDasherModel->ResetNats();
}

void CDasherInterfaceBase::ClearAllContext() {
  ctrlDelete(true, CControlManager::EDIT_FILE);
  ctrlDelete(false, CControlManager::EDIT_FILE);
  SetBuffer(0);
}

void CDasherInterfaceBase::ResetParameter(int iParameter) {
  m_pSettingsStore->ResetParameter(iParameter);
}

// We need to be able to get at the UserLog object from outside the interface
CUserLogBase* CDasherInterfaceBase::GetUserLogPtr() {
  return m_pUserLog;
}

void CDasherInterfaceBase::KeyDown(unsigned long iTime, int iId) {
  if(isLocked())
    return;

  if(m_pInputFilter) {
    m_pInputFilter->KeyDown(iTime, iId, m_pDasherView, m_pInput, m_pDasherModel);
  }

  if(m_pInput) {
    m_pInput->KeyDown(iTime, iId);
  }
}

void CDasherInterfaceBase::KeyUp(unsigned long iTime, int iId) {
  if(isLocked())
    return;

  if(m_pInputFilter) {
    m_pInputFilter->KeyUp(iTime, iId, m_pDasherView, m_pInput, m_pDasherModel);
  }

  if(m_pInput) {
    m_pInput->KeyUp(iTime, iId);
  }
}

void CDasherInterfaceBase::CreateInputFilter() {
  if(m_pInputFilter) {
    m_pInputFilter->pause();
    m_pInputFilter->Deactivate();
    m_pInputFilter = NULL;
  }

  m_pInputFilter = (CInputFilter *)GetModuleByName(GetStringParameter(SP_INPUT_FILTER));

  if (m_pInputFilter == NULL)
    m_pInputFilter = m_oModuleManager.GetDefaultInputMethod();

  m_pInputFilter->Activate();
}

CDasherModule *CDasherInterfaceBase::RegisterModule(CDasherModule *pModule) {
    return m_oModuleManager.RegisterModule(pModule);
}

CDasherModule *CDasherInterfaceBase::GetModule(ModuleID_t iID) {
    return m_oModuleManager.GetModule(iID);
}

CDasherModule *CDasherInterfaceBase::GetModuleByName(const std::string &strName) {
    return m_oModuleManager.GetModuleByName(strName);
}

void CDasherInterfaceBase::SetDefaultInputDevice(CDasherInput *pModule) {
    m_oModuleManager.SetDefaultInputDevice(pModule);
}

void CDasherInterfaceBase::SetDefaultInputMethod(CInputFilter *pModule) {
    m_oModuleManager.SetDefaultInputMethod(pModule);
}

void CDasherInterfaceBase::CreateModules() {
  CInputFilter *defFil = new CDefaultFilter(this, this, m_pFramerate, 3, _("Normal Control"));
  RegisterModule(defFil);
  SetDefaultInputMethod(defFil);
  RegisterModule(new COneDimensionalFilter(this, this, m_pFramerate));
  RegisterModule(new CClickFilter(this, this));
  RegisterModule(new COneButtonFilter(this, this));
  RegisterModule(new COneButtonDynamicFilter(this, this, m_pFramerate));
  RegisterModule(new CTwoButtonDynamicFilter(this, this, m_pFramerate));
  RegisterModule(new CTwoPushDynamicFilter(this, this, m_pFramerate));
  // TODO: specialist factory for button mode
  RegisterModule(new CButtonMode(this, this, true, 8, _("Menu Mode")));
  RegisterModule(new CButtonMode(this, this, false,10, _("Direct Mode")));
  //  RegisterModule(new CDasherButtons(this, this, 4, 0, false,11, "Buttons 3"));
  RegisterModule(new CAlternatingDirectMode(this, this));
  RegisterModule(new CCompassMode(this, this));
  RegisterModule(new CStylusFilter(this, this, m_pFramerate));
  //WIP Temporary as too many segfaults! //RegisterModule(new CDemoFilter(this, this, m_pFramerate));
}

void CDasherInterfaceBase::GetPermittedValues(int iParameter, std::vector<std::string> &vList) {
  // TODO: Deprecate direct calls to these functions
  switch (iParameter) {
  case SP_ALPHABET_ID:
    DASHER_ASSERT(m_AlphIO != NULL);
    m_AlphIO->GetAlphabets(&vList);
    break;
  case SP_COLOUR_ID:
    DASHER_ASSERT(m_ColourIO != NULL);
    m_ColourIO->GetColours(&vList);
    break;
  case SP_CONTROL_BOX_ID:
    DASHER_ASSERT(m_ControlBoxIO != NULL);
    m_ControlBoxIO->GetControlBoxes(&vList);
    break;
  case SP_INPUT_FILTER:
    m_oModuleManager.ListModules(1, vList);
    break;
  case SP_INPUT_DEVICE:
    m_oModuleManager.ListModules(0, vList);
    break;
  }
}

bool CDasherInterfaceBase::GetModuleSettings(const std::string &strName, SModuleSettings **pSettings, int *iCount) {
  return GetModuleByName(strName)->GetSettings(pSettings, iCount);
}

void CDasherInterfaceBase::SetOffset(int iOffset, bool bForce) {
  if (iOffset == m_pDasherModel->GetOffset() && !bForce) return;

  CDasherNode *pNode = m_pNCManager->GetAlphabetManager()->GetRoot(NULL, iOffset!=0, iOffset);
  if (GetGameModule()) pNode->SetFlag(NF_GAME, true);
  m_pDasherModel->SetNode(pNode);
  
  //ACL TODO note that CTL_MOVE, etc., do not come here (that would probably
  // rebuild the model / violently repaint the screen every time!). But we
  // still want to notifyOffset all text actions, so the "New" suboption sees
  // all the editing the user's done...

  for (set<TextAction *>::iterator it = m_vTextActions.begin(); it!=m_vTextActions.end(); it++) {
    (*it)->NotifyOffset(iOffset);
  }
  
  ScheduleRedraw();
}

// Returns 0 on success, an error string on failure.
const char* CDasherInterfaceBase::ClSet(const std::string &strKey, const std::string &strValue) {
  return m_pSettingsStore->ClSet(strKey, strValue);
}


void
CDasherInterfaceBase::ImportTrainingText(const std::string &strPath) {
  if(m_pNCManager)
    m_pNCManager->ImportTrainingText(strPath);
}

