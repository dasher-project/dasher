/*
 *  COSXDasherControl.cpp
 *  Dasher
 *
 *  Created by Doug Dickinson on 17/11/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>
#import "COSXDasherControl.h"
#import "COSXSettingsStore.h"
#import "ModuleManager.h"
#import "COSXMouseInput.h"
#import "DasherUtil.h"
#import "DasherApp.h"
#import "DasherEdit.h"
#import "Event.h"
#import "../Common/Common.h"
#import "../Common/Globber.h"
#import "GameModule.h"

#import <iostream>

#import <fcntl.h>

#import <sys/stat.h>

using namespace std;
using namespace Dasher::Settings;

class COSXGameModule : public CGameModule {
public:
  COSXGameModule(CSettingsUser *pCreator, CDasherInterfaceBase *pIntf, CDasherView *pView, CDasherModel *pModel, NSTextView *_textView)
  : CGameModule(pCreator, pIntf, pView, pModel), textView(_textView) {
    enteredAtts = [[NSDictionary dictionaryWithObject:[NSColor greenColor] forKey:NSForegroundColorAttributeName] retain];
    wrongAtts = [[NSDictionary dictionaryWithObjectsAndKeys:[NSColor redColor],NSForegroundColorAttributeName,[NSNumber numberWithInt:(NSUnderlineStyleThick | NSUnderlinePatternSolid)],NSStrikethroughStyleAttributeName,nil] retain];
    targetAtts = [[NSDictionary dictionary] retain]; //empty, black is default
  }
  void ChunkGenerated() {
    string sText;
    for (vector<symbol>::const_iterator it=targetSyms().begin(); it!=targetSyms().end(); it++)
      sText += m_pAlph->GetText(*it);
    [[textView textStorage] setAttributedString:[[[NSAttributedString alloc] initWithString:NSStringFromStdString(sText) attributes:targetAtts] autorelease]];
    textView.selectedRange=NSMakeRange(0, 0);
    numWrong=0;
  }
  void HandleEvent(const CEditEvent *pEvt) {
    const int iPrev(lastCorrectSym());
    CGameModule::HandleEvent(pEvt);
    NSTextStorage *storage([textView textStorage]);
    if (iPrev==lastCorrectSym()) {
      NSRange r = NSMakeRange(iPrev+1, numWrong);
      NSString *newWrong = NSStringFromStdString(m_strWrong);
      numWrong = [newWrong length];
      NSAttributedString *nwa = [[[NSAttributedString alloc] initWithString:newWrong attributes:wrongAtts] autorelease];
      [storage replaceCharactersInRange:r withAttributedString:nwa];
      [textView scrollRangeToVisible:NSMakeRange(r.location+r.length,0)];
    } else {
      DASHER_ASSERT(m_strWrong=="" && numWrong==0);
      if (iPrev<lastCorrectSym()) {
        //added more
        [storage setAttributes:enteredAtts range:NSMakeRange(iPrev+1,lastCorrectSym()-iPrev)];
      } else {
        [storage setAttributes:targetAtts range:NSMakeRange(lastCorrectSym()+1,iPrev-lastCorrectSym())];
      }
      [textView scrollRangeToVisible:NSMakeRange(lastCorrectSym()+1, 0)];
    }
    [textView setSelectedRange:NSMakeRange(lastCorrectSym()+1+numWrong, 0)];
  }
  void DrawText(CDasherView *pView) {
  }
private:
  NSDictionary *enteredAtts, *wrongAtts, *targetAtts;
  NSInteger numWrong;
  NSTextView *textView;
};

COSXDasherControl::COSXDasherControl(DasherApp *aDasherApp)
: CDashIntfScreenMsgs(new COSXSettingsStore(),this), dasherApp(aDasherApp), dasherEdit(nil),
  userDir([[NSString stringWithFormat:@"%@/Library/Application Support/Dasher/", NSHomeDirectory()] retain]) {
}

void COSXDasherControl::CreateModules() {
  CDasherInterfaceBase::CreateModules();
  // Create locally cached copies of the mouse input objects, as we
  // need to pass coordinates to them from the timer callback
  RegisterModule(m_pMouseInput = new COSXMouseInput());
  SetDefaultInputDevice(m_pMouseInput);
  RegisterModule(m_p1DMouseInput = new COSX1DMouseInput());
}
  
COSXDasherControl::~COSXDasherControl() {
  if(m_pMouseInput) {
    m_pMouseInput = NULL;
  }
  
  if(m_p1DMouseInput) {
    m_p1DMouseInput = NULL;
  }
  [userDir release];
}

void COSXDasherControl::Realize2() {
  CDasherInterfaceBase::Realize(get_time());
  [dasherApp startTimer];
}

void COSXDasherControl::ScanFiles(AbstractParser *parser, const string &strPattern) {

  if (![[NSFileManager defaultManager] fileExistsAtPath:userDir isDirectory:NULL]) {
    // userDir doesn't exist => create it, ready to receive stuff
    (void)[[NSFileManager defaultManager] createDirectoryAtPath:userDir withIntermediateDirectories:YES attributes:nil error:nil];
  }
  
  const string strPath(StdStringFromNSString([[NSBundle mainBundle] resourcePath])+"/"+strPattern);
  const char *sys[2];
  sys[0] = strPath.c_str();
  sys[1] = NULL;
  
  const char *user[2];
  const string userPath = StdStringFromNSString(userDir) + strPattern;
  user[0] = userPath.c_str();
  user[1] = NULL;
  
  globScan(parser, user, sys);
}

void COSXDasherControl::goddamn(unsigned long iTime, bool bForceRedraw) {
  NewFrame(iTime, bForceRedraw);
}

void COSXDasherControl::TimerFired(NSPoint p) {
  m_pMouseInput->SetCoordinates(p.x, p.y);
  m_p1DMouseInput->SetCoordinates(p.x, p.y);
  
  [[dasherApp dasherView] redisplay];
}  
 
void COSXDasherControl::EnterGameMode(CGameModule *pGameModule) {
  CDashIntfScreenMsgs::EnterGameMode(pGameModule);
  [dasherApp setGameModeOn:(GetGameModule()!=NULL)];
}

void COSXDasherControl::LeaveGameMode() {
  CDashIntfScreenMsgs::LeaveGameMode();
  [dasherApp setGameModeOn:(GetGameModule()!=NULL)];
}

void COSXDasherControl::SetEdit(id<DasherEdit> _dasherEdit) {
  //These both produce warnings, as release/retain are not defined in the DasherEdit protocol...(??)
  [dasherEdit release];
  [(dasherEdit = _dasherEdit) retain];
  SetBuffer([dasherEdit currentCursorPos]);
}

void COSXDasherControl::editOutput(const string &strText, CDasherNode *pNode) {
//NSLog(@"ExternalEventHandler edit insert");
  [dasherEdit outputCallback:NSStringFromStdString(strText)];
  CDasherInterfaceBase::editOutput(strText,pNode);
}

void COSXDasherControl::editDelete(const string &strText, CDasherNode *pNode) {
// NSLog(@"ExternalEventHandler edit delete");
  [dasherEdit deleteCallback:NSStringFromStdString(strText)];
  CDasherInterfaceBase::editDelete(strText, pNode);
}

void COSXDasherControl::editConvert(CDasherNode *pSource) {
  NSLog(@"ExternalEventHandler edit convert");
  CDasherInterfaceBase::editConvert(pSource);
}

void COSXDasherControl::editProtect(CDasherNode *pSource) {
  NSLog(@"ExternalEventHandler edit protect");
  CDasherInterfaceBase::editProtect(pSource);
}

unsigned int COSXDasherControl::ctrlMove(bool bForwards, CControlManager::EditDistance dist) {
  return [dasherEdit ctrlMove:dist forwards:bForwards];
}

unsigned int COSXDasherControl::ctrlDelete(bool bForwards, CControlManager::EditDistance dist) {
  return [dasherEdit ctrlDelete:dist forwards:bForwards];
}

int COSXDasherControl::GetFileSize(const std::string &strFileName) {
  struct stat sStatInfo;
  
  if(!stat(strFileName.c_str(), &sStatInfo))
    return sStatInfo.st_size;
  else
    return 0;
}

void COSXDasherControl::Train(NSString *fileName) {
  std::string f = StdStringFromNSString(fileName);
  NSLog(@"Read train file: %s", f.c_str());
  CDasherInterfaceBase::ImportTrainingText(f);
}

bool COSXDasherControl::WriteUserDataFile(const std::string &filename, const std::string &strNewText, bool append)
{
	if(strNewText.length() == 0)
	  return false;
	
	std::string strFilename(StdStringFromNSString(userDir) + filename);
	
	NSLog(@"Write user data file: %s", strFilename.c_str());
	int flg=O_CREAT|O_WRONLY;
	int mode=S_IRUSR|S_IWUSR;
	if(append)
		flg|=O_APPEND;
	int fd=open(strFilename.c_str(),flg, mode);
	write(fd,strNewText.c_str(),strNewText.length());
	close(fd);
	return true;
}

NSDictionary *COSXDasherControl::ParameterDictionary() {
  static NSMutableDictionary *parameterDictionary = nil;
  
  if (parameterDictionary == nil) {
    parameterDictionary = [[NSMutableDictionary alloc] initWithCapacity:NUM_OF_BPS + NUM_OF_LPS + NUM_OF_SPS];
    int ii;
    
    for(ii = 0; ii < NUM_OF_BPS; ii++) {
      [parameterDictionary setObject:[NSDictionary dictionaryWithObjectsAndKeys:
                                      NSStringFromStdString(boolparamtable[ii].regName), @"regName",
                                      NSStringFromStdString(boolparamtable[ii].humanReadable), @"humanReadable",
                                      [NSNumber numberWithInt:boolparamtable[ii].key], @"key",
                                      nil] forKey:NSStringFromStdString(boolparamtable[ii].regName)];
    }
    
    for(ii = 0; ii < NUM_OF_LPS; ii++) {
      [parameterDictionary setObject:[NSDictionary dictionaryWithObjectsAndKeys:
                                      NSStringFromStdString(longparamtable[ii].regName), @"regName",
                                      NSStringFromStdString(longparamtable[ii].humanReadable), @"humanReadable",
                                      [NSNumber numberWithInt:longparamtable[ii].key], @"key",
                                      nil] forKey:NSStringFromStdString(longparamtable[ii].regName)];
    }
    
    for(ii = 0; ii < NUM_OF_SPS; ii++) {
      [parameterDictionary setObject:[NSDictionary dictionaryWithObjectsAndKeys:
                                      NSStringFromStdString(stringparamtable[ii].regName), @"regName",
                                      NSStringFromStdString(stringparamtable[ii].humanReadable), @"humanReadable",
                                      [NSNumber numberWithInt:stringparamtable[ii].key], @"key",
                                      nil] forKey:NSStringFromStdString(stringparamtable[ii].regName)];
    }
  }
  
  return parameterDictionary;
}


int COSXDasherControl::GetParameterIndex(const std::string & aKey) {
  NSString *key = NSStringFromStdString(aKey);
  NSDictionary *parameterEntry = [ParameterDictionary() objectForKey:key];
  if (parameterEntry == nil) {
    NSLog(@"COSXDasherControl::GetParameterIndex - unknown key: %@", key);
    return NSNotFound;
  }
  
  return [[parameterEntry objectForKey:@"key"] intValue];
}

id COSXDasherControl::GetParameter(NSString *aKey) {
  
  int pIndex = GetParameterIndex(StdStringFromNSString(aKey));

  switch (GetParameterType(pIndex)) {
    case ParamBool:
      return [NSNumber numberWithBool:GetBoolParameter(pIndex)];
      break;
    case ParamLong:
      return [NSNumber numberWithLong:GetLongParameter(pIndex)];
      break;
    case ParamString:
      return NSStringFromStdString(GetStringParameter(pIndex));
      break;
    default:
      break;
  }
  
  return nil;
}

void COSXDasherControl::SetParameter(NSString *aKey, id aValue) {
  
  int pIndex = GetParameterIndex(StdStringFromNSString(aKey));
  
  switch (GetParameterType(pIndex)) {
    case ParamBool:
      SetBoolParameter(pIndex, [aValue boolValue]);
      break;
    case ParamLong:
      SetLongParameter(pIndex, [aValue longValue]);
      break;
    case ParamString:
      SetStringParameter(pIndex, StdStringFromNSString(aValue));
      break;
    default:
      break;
  }
}

bool COSXDasherControl::SupportsSpeech() {
  return [dasherApp supportsSpeech];
}

void COSXDasherControl::Speak(const std::string &strText, bool bInterrupt) {
  [dasherApp speak:NSStringFromStdString(strText) interrupt:bInterrupt];
}

void COSXDasherControl::CopyToClipboard(const std::string &strText) {
  [dasherApp copyToClipboard:NSStringFromStdString(strText)];
}

std::string COSXDasherControl::GetContext(unsigned int iOffset, unsigned int iLength) {
  return StdStringFromNSString([dasherEdit textAtOffset:iOffset Length:iLength]);
}

std::string COSXDasherControl::GetAllContext() {
  return StdStringFromNSString([dasherEdit allContext]);
}
int COSXDasherControl::GetAllContextLenght()
{
	return StdStringFromNSString([dasherEdit allContext]).length();
}
void COSXDasherControl::ClearAllContext() {
  [dasherEdit clearContext];
  SetBuffer(0);
}

CGameModule *COSXDasherControl::CreateGameModule() {
  return new COSXGameModule(this, this, GetView(), m_pDasherModel, [dasherApp textView]);
}
