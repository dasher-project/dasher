/*
 *  LowLevelKeyboardHandling.c
 *  TestAcc
 *
 */

/*
 * This file derived from:
 * http://www.opensource.apple.com/darwinsource/10.4/tcl-20/tk/tk/macosx/tkMacOSXKeyEvent.c
 *
 * I've modified it to take out the tk related stuff, leaving only the Mac Keyboard stuff.  I've kept
 * the original names of the functions because there seemed to be equal arguments for keeping them
 * as for changing them.
 *
 * As requested, the original copyright notice is reproduced here:
 */

/*
 * tkMacOSXKeyEvent.c --
 *
 *      This file implements functions that decode & handle keyboard events
 *      on MacOS X.
 *
 *      Copyright 2001, Apple Computer, Inc.
 *
 *      The following terms apply to all files originating from Apple
 *      Computer, Inc. ("Apple") and associated with the software
 *      unless explicitly disclaimed in individual files.
 *
 *
 *      Apple hereby grants permission to use, copy, modify,
 *      distribute, and license this software and its documentation
 *      for any purpose, provided that existing copyright notices are
 *      retained in all copies and that this notice is included
 *      verbatim in any distributions. No written agreement, license,
 *      or royalty fee is required for any of the authorized
 *      uses. Modifications to this software may be copyrighted by
 *      their authors and need not follow the licensing terms
 *      described here, provided that the new terms are clearly
 *      indicated on the first page of each file where they apply.
 *
 *
 *      IN NO EVENT SHALL APPLE, THE AUTHORS OR DISTRIBUTORS OF THE
 *      SOFTWARE BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
 *      INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
 *      THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
 *      EVEN IF APPLE OR THE AUTHORS HAVE BEEN ADVISED OF THE
 *      POSSIBILITY OF SUCH DAMAGE.  APPLE, THE AUTHORS AND
 *      DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
 *      BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS
 *      SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND APPLE,THE
 *      AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *      MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *      GOVERNMENT USE: If you are acquiring this software on behalf
 *      of the U.S. government, the Government shall have only
 *      "Restricted Rights" in the software and related documentation
 *      as defined in the Federal Acquisition Regulations (FARs) in
 *      Clause 52.227.19 (c) (2).  If you are acquiring the software
 *      on behalf of the Department of Defense, the software shall be
 *      classified as "Commercial Computer Software" and the
 *      Government shall have only "Restricted Rights" as defined in
 *      Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 *      foregoing, the authors grant the U.S. Government and others
 *      acting in its behalf permission to use and distribute the
 *      software in accordance with the terms specified in this
 *      license.
 */


#import "LowLevelKeyboardHandling.h"
#import <mach-o/dyld.h>

static int KLSIsInitialized = false;

static UInt32 deadKeyStateUp = 0;        /* The deadkey state for the current
                                          * sequence of keyup events or 0 if
                                          * not in a deadkey sequence */
static UInt32 deadKeyStateDown = 0;      /* Ditto for keydown */


/*
 * If we have old headers, we need to define these types and constants
 * ourself.  We use preprocessor macros instead of enums and typedefs,
 * because macros work even in case of version misunderstandings, while
 * duplicate enums and typedefs would give errrors.
 */

#if !defined(MAC_OS_X_VERSION_10_2) || \
(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_2)
#define KeyboardLayoutRef Ptr
#define KeyboardLayoutPropertyTag UInt32
#define kKLKCHRData 0
#define kKLuchrData 1
#define kKLIdentifier 2
#endif

static OSStatus (*KLGetCurrentKeyboardLayoutPtr)(
                                                 KeyboardLayoutRef *         oKeyboardLayout)    = NULL;
static OSStatus (*KLGetKeyboardLayoutPropertyPtr)(
                                                  KeyboardLayoutRef           iKeyboardLayout,
                                                  KeyboardLayoutPropertyTag   iPropertyTag,
                                                  const void **               oValue)             = NULL;

static void
KLSInit(void)
{
  static const char MODULE[] = "HIToolbox";
  static const char GET_LAYOUT[] = "_KLGetCurrentKeyboardLayout";
  static const char GET_PROP[] = "_KLGetKeyboardLayoutProperty";
  
  NSSymbol symbol;
  
  if (NSIsSymbolNameDefinedWithHint(GET_LAYOUT, MODULE)) {
    symbol = NSLookupAndBindSymbolWithHint(GET_LAYOUT, MODULE);
    KLGetCurrentKeyboardLayoutPtr = NSAddressOfSymbol(symbol);
  }
  if (NSIsSymbolNameDefinedWithHint(GET_PROP, MODULE)) {
    symbol = NSLookupAndBindSymbolWithHint(GET_PROP, MODULE);
    KLGetKeyboardLayoutPropertyPtr = NSAddressOfSymbol(symbol);
  }
  
  KLSIsInitialized = true;
}


/*
 *----------------------------------------------------------------------
 *
 * g --
 *
 *        Given MacOS key event data this function generates the Unicode
 *        characters.  It does this using a 'uchr' and the UCKeyTranslate
 *        API.
 *
 *        The parameter deadKeyStatePtr can be NULL, if no deadkey handling
 *        is needed.
 *
 *        Tested and known to work with US, Hebrew, Greek and Russian layouts
 *        as well as "Unicode Hex Input".
 *
 * Results:
 *        The number of characters generated if any, 0 if we are waiting for
 *        another byte of a dead-key sequence. Fills in the uniChars array
 *        with a Unicode string.
 *
 * Side Effects:
 *        None
 *
 *----------------------------------------------------------------------
 */

static int
KeycodeToUnicodeViaUnicodeResource(
                                   UniChar * uniChars, int maxChars,
                                   Ptr uchr,
                                   EventKind eKind,
                                   UInt32 keycode, UInt32 modifiers,
                                   UInt32 * deadKeyStatePtr)
{
  int action;
  unsigned long keyboardType;
  OptionBits options = 0;
  UInt32 dummy_state;
  UniCharCount actuallength; 
  OSStatus status;
  
  keycode &= 0xFF;
  modifiers = (modifiers >> 8) & 0xFF;
  keyboardType = LMGetKbdType();
  
  if (NULL==deadKeyStatePtr) {
    options = kUCKeyTranslateNoDeadKeysMask;
    dummy_state = 0;
    deadKeyStatePtr = &dummy_state;
  }
  
  switch(eKind) {     
    case kEventRawKeyDown:
      action = kUCKeyActionDown;
      break;
    case kEventRawKeyUp:
      action = kUCKeyActionUp;
      break;
    case kEventRawKeyRepeat:
      action = kUCKeyActionAutoKey;
      break;
    default:
      fprintf (stderr,
               "KeycodeToUnicodeViaUnicodeResource(): "
               "Invalid parameter eKind %d\n",
               (int) eKind);
      return 0;
  }
  
  status = UCKeyTranslate(
                          (const UCKeyboardLayout *) uchr,
                          keycode, action, modifiers, keyboardType,
                          options, deadKeyStatePtr,
                          maxChars, &actuallength, uniChars);
  
  if ((0 == actuallength) && (0 != *deadKeyStatePtr)) {
    /*
     * More data later
     */
    
    return 0; 
  }
  
  /*
   * some IMEs leave residue :-(
                                */
  
  *deadKeyStatePtr = 0; 
  
  if (noErr != status) {
    fprintf(stderr,"UCKeyTranslate failed: %d", (int) status);
    actuallength = 0;
  }
  
  return actuallength;
}

/*
 *----------------------------------------------------------------------
 *
 * KeycodeToUnicodeViaKCHRResource --
 *
 *        Given MacOS key event data this function generates the Unicode
 *        characters.  It does this using a 'KCHR' and the KeyTranslate API.
 *
 *        The parameter deadKeyStatePtr can be NULL, if no deadkey handling
 *        is needed.
 *
 * Results:
 *        The number of characters generated if any, 0 if we are waiting for
 *        another byte of a dead-key sequence. Fills in the uniChars array
 *        with a Unicode string.
 *
 * Side Effects:
 *        None
 *
 *----------------------------------------------------------------------
 */

static int
KeycodeToUnicodeViaKCHRResource(
                                UniChar * uniChars, int maxChars,
                                Ptr kchr, TextEncoding encoding,
                                EventKind eKind,
                                UInt32 keycode, UInt32 modifiers,
                                UInt32 * deadKeyStatePtr)
{
  UInt32 result;
  char macBuff[3];
  char * macStr;
  int macStrLen;
  UInt32 dummy_state = 0;
  
  
  if (NULL == deadKeyStatePtr) {
    deadKeyStatePtr = &dummy_state;
  }
  
  keycode |= modifiers;
  result = KeyTranslate(kchr, keycode, deadKeyStatePtr);
  
  if ((0 == result) && (0 != dummy_state)) {
    /*
     * 'dummy_state' gets only filled if the caller did not want deadkey
     * processing (deadKeyStatePtr was NULL originally), but we still
     * have a deadkey.  We just push the keycode for the space bar to get
     * the real key value.
     */
    
    result = KeyTranslate(kchr, 0x31, deadKeyStatePtr);
    *deadKeyStatePtr = 0;
  }
  
  if ((0 == result) && (0 != *deadKeyStatePtr)) {
    /*
     * More data later
     */
    
    return 0; 
  }
  
  macBuff[0] = (char) (result >> 16);
  macBuff[1] = (char)  result;
  macBuff[2] = 0;
  
  if (0 != macBuff[0]) {
    /*
     * If the first byte is valid, the second is too
     */
    
    macStr = macBuff;
    macStrLen = 2;
  } else if (0 != macBuff[1]) {
    /*
     * Only the second is valid
     */
    
    macStr = macBuff+1;
    macStrLen = 1;
  } else {
    /*
     * No valid bytes at all -- shouldn't happen
     */
    
    macStr = NULL;
    macStrLen = 0;
  }
  
  if (macStrLen <= 0) {
    return 0;
  } else {
    
    /*
     * Use the CFString conversion routines.  This is the easiest and
     * most compatible way to get from an 8-bit string and a MacOS script
     * code to a Unicode string.
     *
     * FIXME: The system ships with an Irish 'KCHR' but without the
     * corresponding macCeltic encoding, which triggers the error below.
     * Tcl doesn't have the macCeltic encoding either right now, so until
     * we get that, we can just as well stick to this code.  The right
     * fix would be to use the Tcl encodings and add macCeltic and
     * probably others there.  Suitable Unicode data files for the
     * missing encodings are available from www.evertype.com.
     */
    
    CFStringRef cfString;
    int uniStrLen;
    
    cfString = CFStringCreateWithCStringNoCopy(
                                               NULL, macStr, encoding, kCFAllocatorNull);
    if (cfString == NULL) {
      fprintf(stderr, "CFString: Can't convert with encoding %d\n",
              (int) encoding);
      return 0;
    }
    
    uniStrLen = CFStringGetLength(cfString);
    if (uniStrLen > maxChars) {
      uniStrLen = maxChars;
    }
    CFStringGetCharacters(cfString, CFRangeMake(0,uniStrLen), uniChars);
    CFRelease(cfString);
    
    return uniStrLen;
  }
}


/*
 *----------------------------------------------------------------------
 *
 * GetKCHREncoding --
 *
 *      Upgrade a WorldScript code to a TEC encoding based on the keyboard
 *      layout id.
 *
 * Results:
 *      The TEC code that corresponds best to the combination of WorldScript
 *      code and 'KCHR' id.
 *
 * Side effects:
 *      None.
 *
 * Rationale and Notes:
 *      WorldScript codes are sometimes not unique encodings.  E.g. Icelandic
 *      uses script smRoman (0), but the actual encoding is
 *      kTextEncodingMacIcelandic (37).  ftp://ftp.unicode.org/Public
 *      /MAPPINGS/VENDORS/APPLE/README.TXT has a good summary of these
 *      variants.  So we need to upgrade the script to an encoding with
 *      GetTextEncodingFromScriptInfo().
 *
 *      'KCHR' ids are usually region codes (see the comments in Script.h).
 *      Where they are not, we get a paramErr from the OS function and have
 *      appropriate fallbacks.
 *
 *----------------------------------------------------------------------
 */

static TextEncoding
GetKCHREncoding(ScriptCode script, SInt32 layoutid)
{
  RegionCode region = layoutid;
  TextEncoding encoding = script;
  
  if (GetTextEncodingFromScriptInfo(script, kTextLanguageDontCare, region,
                                    &encoding) == noErr) {
    return encoding;
  }
  
  /*
   * GetTextEncodingFromScriptInfo() doesn't know about more exotic
   * layouts.  This provides a fallback for good measure.  In an ideal
   * world, exotic layouts would always provide a 'uchr' resource anyway,
   * so we wouldn't need this.
   *
   * We can add more keyboard layouts, if we get actual complaints.  Farsi
   * or other Celtic/Gaelic layouts would be candidates.
   */
  
  switch (layoutid) {
    
    /*
     * Icelandic and Faroese (planned).  These layouts are sold by Apple
     * Iceland for legacy applications.
     */
    
    case 1800: case 1821:
      return kTextEncodingMacIcelandic;
      
      /*
       * Irish and Welsh.  These layouts are mentioned in <Script.h>.
       *
       * FIXME: This may have to be kTextEncodingMacGaelic instead, but I
       * can't locate layouts of this type for testing.
       */
      
    case 581: case 779:
      return kTextEncodingMacCeltic;
  }
  
  /*
   * The valid script codes are also the valid default encoding codes, so
   * if nothing else helps, fall back on those.
   */
  
  return script;
}


/*
 *----------------------------------------------------------------------
 *
 * GetKeyboardLayout --
 *
 *      Queries the OS for a pointer to a keyboard resource.
 *
 *      This function works with the keyboard layout switch menu.  It uses
 *      Keyboard Layout Services, where available.
 *
 * Results:
 *      1 if there is returned a Unicode 'uchr' resource in *resourcePtr, 0
 *      if it is a classic 'KCHR' resource.  A pointer to the actual resource
 *      data goes into *resourcePtr.  If the resource is a 'KCHR' resource,
 *      the corresponding Mac encoding goes into *encodingPtr.
 *
 * Side effects:
 *      Sets some internal static variables.
 *
 *----------------------------------------------------------------------
 */

static int
GetKeyboardLayout (Ptr * resourcePtr, TextEncoding * encodingPtr)
{
  static KeyboardLayoutRef lastLayout = NULL;
  static SInt32 lastLayoutId;
  static TextEncoding lastEncoding = kTextEncodingMacRoman;
  static Ptr uchr = NULL;
  static Ptr KCHR = NULL;
  static Handle handle = NULL;
  
  int hasLayoutChanged = false;
  KeyboardLayoutRef currentLayout = NULL;
  SInt32 currentLayoutId = 0;
  ScriptCode currentKeyScript;
  
  /*
   * Several code branches need this information.
   */
  
  currentKeyScript = GetScriptManagerVariable(smKeyScript);
  
  if (!KLSIsInitialized) {
    KLSInit();
  }
  
  if (KLGetCurrentKeyboardLayoutPtr != NULL) {
    
    /*
     * Use the Keyboard Layout Services (these functions only exist since
                                         * 10.2).
     */
    
    (*KLGetCurrentKeyboardLayoutPtr)(&currentLayout);
    
    if (currentLayout != NULL) {
      
      /*
       * The layout pointer could in theory be the same for different
       * layouts, only the id gives us the information that the
       * keyboard has actually changed.  OTOH the layout object can
       * also change and it could still be the same layoutid.
       */
      
      (*KLGetKeyboardLayoutPropertyPtr)(currentLayout, kKLIdentifier,
                                        (const void**)&currentLayoutId);
      
      if ((lastLayout != currentLayout)
          || (lastLayoutId != currentLayoutId)) {
        
#ifdef TK_MAC_DEBUG
        fprintf (stderr, "GetKeyboardLayout(): Use KLS\n");
#endif
        
        hasLayoutChanged = true;
        
        /*
         * Reinitialize all relevant variables.
         */
        
        lastLayout = currentLayout;
        lastLayoutId = currentLayoutId;
        uchr = NULL;
        KCHR = NULL;
        
        if (((*KLGetKeyboardLayoutPropertyPtr)(currentLayout,
                                               kKLuchrData, (const void**)&uchr)
             == noErr)
            && (uchr != NULL)) {
          /* done */
        } else if (((*KLGetKeyboardLayoutPropertyPtr)(currentLayout,
                                                      kKLKCHRData, (const void**)&KCHR)
                    == noErr)
                   && (KCHR != NULL)) {
          /* done */
        }
      }
    }
    
  } else {
    
    /*
     * Use the classic approach as shown in Apple code samples, loading
     * the keyboard resources directly.  This is broken for 10.3 and
     * possibly already in 10.2.
     */
    
    currentLayoutId = GetScriptVariable(currentKeyScript,smKeyScript);
    
    if ((lastLayout == NULL) || (lastLayoutId != currentLayoutId)) {
      
#ifdef TK_MAC_DEBUG
      fprintf (stderr, "GetKeyboardLayout(): Use GetResource()\n");
#endif
      
      hasLayoutChanged = true;
      
      /*
       * Reinitialize all relevant variables.
       */
      
      lastLayout = (KeyboardLayoutRef)-1; 
      lastLayoutId = currentLayoutId;
      uchr = NULL;
      KCHR = NULL;
      
      /*
       * Get the new layout resource in the classic way.
       */
      
      if (handle != NULL) {
        HUnlock(handle);
      }
      
      if ((handle = GetResource('uchr',currentLayoutId)) != NULL) {
        HLock(handle);
        uchr = *handle;
      } else if ((handle = GetResource('KCHR',currentLayoutId)) != NULL) {
        HLock(handle);
        KCHR = *handle;
      }
    }
  }
  
  if (hasLayoutChanged) {
    
#ifdef TK_MAC_DEBUG
    if (KCHR != NULL) {
      fprintf (stderr, "GetKeyboardLayout(): New 'KCHR' layout %d\n",
               (int) (short) currentLayoutId);
    } else if (uchr != NULL) {
      fprintf (stderr, "GetKeyboardLayout(): New 'uchr' layout %d\n",
               (int) (short) currentLayoutId);
    } else {
      fprintf (stderr, "GetKeyboardLayout(): Use cached layout "
               "(should have been %d)\n",
               (int) (short) currentLayoutId);
    }
#endif
    
    deadKeyStateUp = deadKeyStateDown = 0;
    
    /*
     * If we did get a new 'KCHR', compute its encoding and put it into
     * lastEncoding.
     *
     * If we didn't get a new 'KCHR' and if we have no 'uchr' either, get
     * some 'KCHR' from the OS cache and leave lastEncoding at its
     * current value.  This should better not happen, it doesn't really
     * work.
     */
    
    if (KCHR != NULL) {
      lastEncoding = GetKCHREncoding(currentKeyScript, currentLayoutId);
#ifdef TK_MAC_DEBUG
      fprintf (stderr, "GetKeyboardLayout(): New 'KCHR' encoding %lu "
               "(%lu + 0x%lX)\n",
               lastEncoding, lastEncoding & 0xFFFFL,
               lastEncoding & ~0xFFFFL);
#endif
    } else if (uchr == NULL) {
      KCHR = (Ptr) GetScriptManagerVariable(smKCHRCache);
    }
  }
  
  if (uchr != NULL) {
    *resourcePtr = uchr;
    return 1;
  } else {
    *resourcePtr = KCHR;
    *encodingPtr = lastEncoding;
    return 0;
  }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXKeycodeToUnicode --
 *
 *        Given MacOS key event data this function generates the Unicode
 *        characters.  It does this using OS resources and APIs.
 *
 *        The parameter deadKeyStatePtr can be NULL, if no deadkey handling
 *        is needed.
 *
 *        This function is called from XKeycodeToKeysym() in
 *        tkMacOSKeyboard.c.
 *
 * Results:
 *        The number of characters generated if any, 0 if we are waiting for
 *        another byte of a dead-key sequence. Fills in the uniChars array
 *        with a Unicode string.
 *
 * Side Effects:
 *        None
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXKeycodeToUnicode(
                         UniChar * uniChars, int maxChars,
                         EventKind eKind,
                         UInt32 keycode, UInt32 modifiers,
                         UInt32 * deadKeyStatePtr)
{
  Ptr resource = NULL;
  TextEncoding encoding;
  int len;
  
  
  if (GetKeyboardLayout(&resource,&encoding)) {
    len = KeycodeToUnicodeViaUnicodeResource(
                                             uniChars, maxChars, resource, eKind,
                                             keycode, modifiers, deadKeyStatePtr);
  } else {
    len = KeycodeToUnicodeViaKCHRResource(
                                          uniChars, maxChars, resource, encoding, eKind,
                                          keycode, modifiers, deadKeyStatePtr);
  }
  
  return len;
}

bool keyboardMappingHasChanged(void)
{
  static Ptr lastResource = NULL;
  static TextEncoding lastEncoding;
  
  Ptr resource;
  TextEncoding encoding;
  bool result = false;
  
  (void)GetKeyboardLayout(&resource, &encoding);

//  printf("lastR = %p, r = %p; lastE = %p, e = %p", lastResource, lastEncoding, resource, encoding);
  
  if (lastResource != resource || lastEncoding != encoding)
   {
    lastResource = resource;
    lastEncoding = encoding;
    result = true;
   }
  
//  printf(result ? " -- CHANGED\n" : " -- same\n");
  
  return result;
}




