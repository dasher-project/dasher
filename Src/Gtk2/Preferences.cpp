#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <libintl.h>
#include <cstring>
#include <cmath>

#include "../Common/Common.h"
#include "DasherTypes.h"
//#include "FontDialogues.h"
#include "Preferences.h"
#include "Parameters.h"
#include "module_settings_window.h"
#include "dasher_main_private.h"

#define DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf) (DasherPreferencesDialoguePrivate *)(pSelf->private_data);

using namespace Dasher;

/* Static global member - use to deal with callbacks */
static DasherPreferencesDialogue *g_pPreferencesDialogue = NULL;

// TODO: Reintroduce advanced settings and start on mouse position offset

typedef struct _BoolTranslation BoolTranslation;

struct _BoolTranslation {
  gint iParameter;
  const gchar *szWidgetName;
  GtkWidget *pWidget;
};

// Checkbox widgets which map directly to boolean parameters

#ifndef WITH_MAEMO

BoolTranslation sBoolTranslationTable[] = {
  {BP_DRAW_MOUSE_LINE, "showmouselinebutton", NULL},
  {BP_DRAW_MOUSE, "showmousebutton", NULL},
  { APP_BP_SHOW_STATUSBAR, "speedsliderbutton", NULL },
  {APP_BP_SHOW_TOOLBAR, "toolbarbutton", NULL},
  {BP_CONTROL_MODE, "control_controlmode", NULL},
  {APP_BP_TIME_STAMP, "timestampbutton", NULL},
  {BP_START_MOUSE, "leftbutton", NULL},  
  {BP_START_SPACE, "spacebutton", NULL},
  {BP_STOP_OUTSIDE, "winpausebutton", NULL},
  {BP_AUTO_SPEEDCONTROL, "adaptivebutton", NULL},
  {BP_LM_ADAPTIVE, "cb_adaptive", NULL},
  {BP_COPY_ALL_ON_STOP,"copy_all_on_stop",NULL},
  {BP_SPEAK_ALL_ON_STOP,"speak_all_on_stop",NULL},
  {BP_SPEAK_WORDS,"speak_words",NULL},
  {APP_BP_CONFIRM_UNSAVED, "confirm_unsaved_files", NULL}
};

#endif

// List widgets which map directly to string parameters

typedef struct _StringTranslation StringTranslation;

struct _StringTranslation {
  gint iParameter;
  const gchar *szWidgetName;
  GtkWidget *pWidget;
  const gchar *szHelperName;
  GtkWidget *pHelper;
};

#ifdef WITH_MAEMO

StringTranslation sStringTranslationTable[] = {
  {SP_ALPHABET_ID, "AlphabetTree", NULL, NULL, NULL},
  {SP_COLOUR_ID, "ColorTree", NULL, NULL, NULL},
};

#else

StringTranslation sStringTranslationTable[] = {
  {SP_ALPHABET_ID, "AlphabetTree", NULL, NULL, NULL},
  {SP_COLOUR_ID, "ColorTree", NULL, NULL, NULL},
  {SP_INPUT_FILTER, "input_filter_tree_view", NULL, "button13", NULL},
  {SP_INPUT_DEVICE, "input_tree_view", NULL, "button25", NULL}
};

#endif

// TODO: Look at coding convention stuff for gobjets

/// Newer, object based stuff

struct _DasherPreferencesDialoguePrivate {
  GtkWindow *pPreferencesWindow;
  DasherEditor *pEditor;
  DasherAppSettings *pAppSettings;
  GtkWindow *pMainWindow;

  // TODO: it really would be nice not to have to keep this arround
  GtkBuilder *pXML;

  // TODO: check all of these are really needed
  GtkListStore *pListStore;
  GtkToggleButton *pAlphOrient;
  GtkToggleButton *pLRButton;
  GtkToggleButton *pRLButton;
  GtkToggleButton *pTBButton;
  GtkToggleButton *pBTButton;
  GtkRange *pSpeedSlider;
  GtkToggleButton *pMousePosButton;
  GtkComboBox *pMousePosStyle;
  GtkNotebook *pNotebook;

  // Set this to ignore signals (ie loops coming back from setting widgets in response to parameters having changed)
  bool bIgnoreSignals;
};

typedef struct _DasherPreferencesDialoguePrivate DasherPreferencesDialoguePrivate;

// Private member functions
static void dasher_preferences_dialogue_class_init(DasherPreferencesDialogueClass *pClass);
static void dasher_preferences_dialogue_init(DasherPreferencesDialogue *pMain);
static void dasher_preferences_dialogue_destroy(GObject *pObject);

static void dasher_preferences_dialogue_initialise_tables(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_refresh_widget(DasherPreferencesDialogue *pSelf, gint iParameter);
static void dasher_preferences_dialogue_populate_list(DasherPreferencesDialogue *pSelf, GtkTreeView *pView, int iParameter, GtkWidget *pHelper);
static void dasher_preferences_dialogue_update_special(DasherPreferencesDialogue *pSelf, int iID);
static void dasher_preferences_dialogue_refresh_parameter(DasherPreferencesDialogue *pSelf, GtkWidget *pWidget, gpointer pUserData);

/* Special private members called via table (see below) */
static void dasher_preferences_dialogue_populate_special_speed(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_mouse_start(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_orientation(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_appstyle(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_linewidth(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_lm(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_uniform(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_colour(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_dasher_font(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_edit_font(DasherPreferencesDialogue *pSelf);
static void dasher_preferences_dialogue_populate_special_fontsize(DasherPreferencesDialogue *pSelf);

typedef struct _SpecialControl SpecialControl;

struct _SpecialControl {
  int iID;
  void (*pPopulate)(DasherPreferencesDialogue *);
  gboolean bPrimary;
};

SpecialControl sSpecialControlTable[] = {
  {LP_MAX_BITRATE, dasher_preferences_dialogue_populate_special_speed, true},
  {BP_MOUSEPOS_MODE, dasher_preferences_dialogue_populate_special_mouse_start, true},
  {BP_CIRCLE_START, dasher_preferences_dialogue_populate_special_mouse_start, false},
  {LP_ORIENTATION, dasher_preferences_dialogue_populate_special_orientation, true},
  {APP_LP_STYLE, dasher_preferences_dialogue_populate_special_appstyle, true},
  {LP_LINE_WIDTH, dasher_preferences_dialogue_populate_special_linewidth, true},
  {LP_LANGUAGE_MODEL_ID, dasher_preferences_dialogue_populate_special_lm, true},
  {LP_UNIFORM, dasher_preferences_dialogue_populate_special_uniform, true},
  {BP_PALETTE_CHANGE, dasher_preferences_dialogue_populate_special_colour, true},
  {SP_DASHER_FONT, dasher_preferences_dialogue_populate_special_dasher_font, true},
  {APP_SP_EDIT_FONT, dasher_preferences_dialogue_populate_special_edit_font, true}, 
  {LP_DASHER_FONTSIZE, dasher_preferences_dialogue_populate_special_fontsize, true}
};

// Callback functions
extern "C" void on_list_selection(GtkTreeSelection *pSelection, gpointer pUserData);
extern "C" void on_widget_realize(GtkWidget *pWidget, gpointer pUserData);
extern "C" gboolean show_helper_window(GtkWidget *pWidget, gpointer *pUserData);
extern "C" gboolean dasher_preferences_refresh_foreach_function(GtkTreeModel *pModel, GtkTreePath *pPath, GtkTreeIter *pIter, gpointer pUserData);


/* Start definitions */


GType dasher_preferences_dialogue_get_type() {

  static GType dasher_preferences_dialogue_type = 0;

  if(!dasher_preferences_dialogue_type) {
    static const GTypeInfo dasher_preferences_dialogue_info = {
      sizeof(DasherPreferencesDialogueClass),
      NULL,
      NULL,
      (GClassInitFunc) dasher_preferences_dialogue_class_init,
      NULL,
      NULL,
      sizeof(DasherPreferencesDialogue),
      0,
      (GInstanceInitFunc) dasher_preferences_dialogue_init,
      NULL
    };

    dasher_preferences_dialogue_type = g_type_register_static(G_TYPE_OBJECT, "DasherPreferencesDialogue", &dasher_preferences_dialogue_info, static_cast < GTypeFlags > (0));
  }

  return dasher_preferences_dialogue_type;
}

static void dasher_preferences_dialogue_class_init(DasherPreferencesDialogueClass *pClass) {
  GObjectClass *pObjectClass = (GObjectClass *) pClass;
  pObjectClass->finalize = dasher_preferences_dialogue_destroy;
}

static void dasher_preferences_dialogue_init(DasherPreferencesDialogue *pDasherControl) {
  pDasherControl->private_data = new DasherPreferencesDialoguePrivate;
}

static void dasher_preferences_dialogue_destroy(GObject *pObject) {
  DasherPreferencesDialogue *pSelf = (DasherPreferencesDialogue *)pObject;
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  g_object_unref(pPrivate->pXML);

  // FIXME - I think we need to chain up through the finalize methods
  // of the parent classes here...
  delete pPrivate;
}

// Public methods

DasherPreferencesDialogue *dasher_preferences_dialogue_new(GtkBuilder *pXML, DasherEditor *pEditor, DasherAppSettings *pAppSettings, GtkWindow *pMainWindow) {
  DasherPreferencesDialogue *pDasherPref;
  pDasherPref = (DasherPreferencesDialogue *)(g_object_new(dasher_preferences_dialogue_get_type(), NULL));

  g_pPreferencesDialogue = pDasherPref;

  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pDasherPref);

  pPrivate->bIgnoreSignals = false;

  pPrivate->pEditor = pEditor;
  pPrivate->pAppSettings = pAppSettings;

  pPrivate->pXML = (GtkBuilder *)g_object_ref(pXML);

  pPrivate->pPreferencesWindow = GTK_WINDOW(gtk_builder_get_object(pXML, "preferences"));

  pPrivate->pNotebook = GTK_NOTEBOOK(gtk_builder_get_object(pXML, "notebook1"));

  gtk_window_set_transient_for(pPrivate->pPreferencesWindow, pMainWindow);

  pPrivate->pMainWindow = pMainWindow;

  pPrivate->pSpeedSlider = GTK_RANGE(gtk_builder_get_object(pXML, "hscale1"));
  dasher_preferences_dialogue_initialise_tables(pDasherPref);
  dasher_preferences_dialogue_refresh_widget(pDasherPref, -1);
  dasher_preferences_dialogue_update_special(pDasherPref, -1);

#ifdef WITH_MAEMO
#ifndef WITH_MAEMOFULLSCREEN
  gtk_widget_hide(gtk_builder_get_object(pXML, "displaysizebox"));
#endif
#endif

#ifndef JAPANESE
  gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(pXML, "radiobutton9")));
#endif

  //  InitialiseFontDialogues(pXML, pAppSettings);

  return pDasherPref;
}

void dasher_preferences_dialogue_show(DasherPreferencesDialogue *pSelf, gint iPage) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  // FIXME - REIMPLEMENT

  // Keep the preferences window in the correct position relative to the
  // main Dasher window
  
  //  gtk_window_set_transient_for(pPrivate->pPreferencesWindow,pPrivate->pMainWindow);
#ifdef WITH_MAEMO
#ifndef WITH_MAEMOFULLSCREEN
  // gtk_window_set_keep_above(pPrivate->pPreferencesWindow, true);
#endif
#endif
  // TODO: reimplement
  //  gtk_window_set_keep_above(pPrivate->pPreferencesWindow, dasher_main_topmost(g_pDasherMain));

  if(iPage > 0) {
    gtk_notebook_set_current_page(pPrivate->pNotebook, iPage - 1);
  }

  gtk_window_present(pPrivate->pPreferencesWindow);
}


void dasher_preferences_dialogue_handle_parameter_change(DasherPreferencesDialogue *pSelf, int iParameter) {
  dasher_preferences_dialogue_refresh_widget(pSelf, iParameter);
  dasher_preferences_dialogue_update_special(pSelf, iParameter);
}


// --- Generic Options ---

void dasher_preferences_dialogue_initialise_tables(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
 

#ifndef WITH_MAEMO
  int iNumBoolEntries = sizeof(sBoolTranslationTable) / sizeof(BoolTranslation);
  for(int i(0); i < iNumBoolEntries; ++i) {
    sBoolTranslationTable[i].pWidget = GTK_WIDGET(gtk_builder_get_object(pPrivate->pXML, sBoolTranslationTable[i].szWidgetName));
  }
#endif

  int iNumStringEntries = sizeof(sStringTranslationTable) / sizeof(StringTranslation);
  for(int i(0); i < iNumStringEntries; ++i) {
    sStringTranslationTable[i].pWidget = GTK_WIDGET(gtk_builder_get_object(pPrivate->pXML, sStringTranslationTable[i].szWidgetName));
    if(sStringTranslationTable[i].szHelperName)
      sStringTranslationTable[i].pHelper = GTK_WIDGET(gtk_builder_get_object(pPrivate->pXML, sStringTranslationTable[i].szHelperName));

    dasher_preferences_dialogue_populate_list(pSelf, GTK_TREE_VIEW(sStringTranslationTable[i].pWidget), sStringTranslationTable[i].iParameter, sStringTranslationTable[i].pHelper);
    g_signal_connect(sStringTranslationTable[i].pWidget, "realize", (GCallback)on_widget_realize, &sStringTranslationTable[i].iParameter);
  }
}

extern "C" gboolean dasher_preferences_refresh_foreach_function(GtkTreeModel *pModel, GtkTreePath *pPath, GtkTreeIter *pIter, gpointer pUserData) {
  gpointer *pPointers = (gpointer *)pUserData;

  gchar *szTarget = (gchar *)pPointers[0];
  gchar *szComparison;

  gtk_tree_model_get(pModel, pIter, 2, &szComparison, -1);

  if(!strcmp(szTarget, szComparison)) {
    g_free(szComparison);
    // Todo: set selection here?
    gtk_tree_view_set_cursor((GtkTreeView *)pPointers[1], pPath, NULL, false);

    gtk_tree_view_scroll_to_cell((GtkTreeView *)pPointers[1], pPath, NULL, true, 0.5, 0.0);

    return true;
  }

  g_free(szComparison);
  
  return false;
}


void dasher_preferences_dialogue_refresh_widget(DasherPreferencesDialogue *pSelf, gint iParameter) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

#ifndef WITH_MAEMO
  int iNumBoolEntries = sizeof(sBoolTranslationTable) / sizeof(BoolTranslation);
  for(int i(0); i < iNumBoolEntries; ++i) {
    if((iParameter == -1) || (sBoolTranslationTable[i].iParameter == iParameter)) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sBoolTranslationTable[i].pWidget), pPrivate->pAppSettings->GetBool(sBoolTranslationTable[i].iParameter));
    }
  }
#endif

  // TODO: I believe that this is being called initially before the
  // widgets are realised, so the selection isn't being correctly
  // brought into view
  int iNumStringEntries = sizeof(sStringTranslationTable) / sizeof(StringTranslation);
  for(int i(0); i < iNumStringEntries; ++i) {
    if((iParameter == -1) || (sStringTranslationTable[i].iParameter == iParameter)) {
      GtkTreeModel *pModel = gtk_tree_view_get_model(GTK_TREE_VIEW(sStringTranslationTable[i].pWidget));

      // TODO: tidy up in a struct
      const void *pUserData[3];
      pUserData[0] = pPrivate->pAppSettings->GetString(sStringTranslationTable[i].iParameter).c_str();
      pUserData[1] = GTK_TREE_VIEW(sStringTranslationTable[i].pWidget);
      pUserData[2] = pSelf;

#if GTK_CHECK_VERSION (2,20,0)
      if(sStringTranslationTable[i].pWidget && gtk_widget_get_realized(sStringTranslationTable[i].pWidget))
#else
      if(sStringTranslationTable[i].pWidget && GTK_WIDGET_REALIZED(sStringTranslationTable[i].pWidget))
#endif
        gtk_tree_model_foreach(pModel, dasher_preferences_refresh_foreach_function, pUserData);
    }
  }
}

static void dasher_preferences_dialogue_refresh_parameter(DasherPreferencesDialogue *pSelf, GtkWidget *pWidget, gpointer pUserData) {
#ifndef WITH_MAEMO
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  
  int iNumBoolEntries = sizeof(sBoolTranslationTable) / sizeof(BoolTranslation);
  
  for(int i(0); i < iNumBoolEntries; ++i) {
    if((pWidget == NULL) || (sBoolTranslationTable[i].pWidget == pWidget)) {

      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sBoolTranslationTable[i].pWidget)) != pPrivate->pAppSettings->GetBool(sBoolTranslationTable[i].iParameter)) {
        
        pPrivate->pAppSettings->SetBool(sBoolTranslationTable[i].iParameter, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sBoolTranslationTable[i].pWidget)));
      }
    }
  }
#endif
}

// TODO: Is this function actually useful? (conversely, is the other call to RefreshFoo elsewhere any use?)
extern "C" void on_widget_realize(GtkWidget *pWidget, gpointer pUserData) {
  gint *pParameter = (gint *)pUserData;
  dasher_preferences_dialogue_refresh_widget(g_pPreferencesDialogue, *pParameter); // TODO: Fix NULL pointer
}

// --- Generic boolean options ---

extern "C" void generic_bool_changed(GtkWidget *widget, gpointer user_data) {
  dasher_preferences_dialogue_refresh_parameter(g_pPreferencesDialogue, widget, user_data);// TODO: fix NULL
}

extern "C" void outline_button_toggled(GtkWidget *widget, gpointer user_data) {
	DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue);
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
		pPrivate->pAppSettings->SetLong(LP_OUTLINE_WIDTH, 1);
	} else {
		pPrivate->pAppSettings->SetLong(LP_OUTLINE_WIDTH, 0);
	}
}

// --- Generic string options ---
//pHelper, if provided, is a button to open a module settings dialog
// for parameters specific to the item selected in the list (i.e. GtkTreeView), if any.
void dasher_preferences_dialogue_populate_list(DasherPreferencesDialogue *pSelf, GtkTreeView *pView, int iParameter, GtkWidget *pHelper) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  
  const gchar *szCurrentValue(pPrivate->pAppSettings->GetString(iParameter).c_str());

  GArray *pFilterArray = pPrivate->pAppSettings->GetAllowedValues(iParameter);

  //for each item in the list: the dasher Parameters.h number (i.e. same for all items); the "helper" button (as above, also the same for all items);
  // the text to display (and perhaps pass to SetStringParameter).
  GtkListStore *pStore = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_STRING);
  gtk_tree_view_set_model(pView, GTK_TREE_MODEL(pStore));

  GtkCellRenderer *pRenderer;
  GtkTreeViewColumn *pColumn;
  
  pRenderer = gtk_cell_renderer_text_new();
  pColumn = gtk_tree_view_column_new_with_attributes(_("Action"), pRenderer, "text", 2, NULL);
  gtk_tree_view_append_column(pView, pColumn);

  GtkTreeIter oIter;
  //doing this before we add the items, means the callback will be invoked when we set
  // the selection (below), which'll enable/disable the pHelper button appropriately
  GtkTreeSelection *pSelection = gtk_tree_view_get_selection(pView);
  g_signal_connect(pSelection, "changed", (GCallback)on_list_selection, 0);

  for(unsigned int i(0); i < pFilterArray->len; ++i) {
    const gchar *szCurrentFilter = g_array_index(pFilterArray, gchar *, i);
    gchar *szName = new gchar[strlen(szCurrentFilter) + 1];
    strcpy(szName, szCurrentFilter);

    gtk_list_store_append(pStore, &oIter);
    
    gtk_list_store_set(pStore, &oIter, 0, iParameter, 1, pHelper, 2, szName, -1);

    delete[] szName;

    if(!strcmp(szCurrentFilter, szCurrentValue)) {
      gtk_tree_selection_select_iter(pSelection, &oIter);
    }
  }

  if(pHelper) {
    g_signal_connect(G_OBJECT(pHelper), "clicked", G_CALLBACK(show_helper_window), pView);
  }  
}

// TODO: In class
extern "C" void on_list_selection(GtkTreeSelection *pSelection, gpointer pUserData) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix null

  GtkTreeIter oIter;
  GtkTreeModel *pModel;
  
  if(gtk_tree_selection_get_selected(pSelection, &pModel, &oIter)) {
    int iParameter;
    gpointer pHelper;
    gchar *szValue;
    gtk_tree_model_get(pModel, &oIter, 0, &iParameter, 1, &pHelper, 2, &szValue, -1);
    
    pPrivate->pAppSettings->SetString(iParameter, szValue);

    if(pHelper) {
      //check if input filter/device has any settings...
      SModuleSettings *pSettings;
      int iCount;
      bool bHasSettings = pPrivate->pAppSettings->GetModuleSettings(szValue, &pSettings, &iCount);
      gtk_widget_set_sensitive(GTK_WIDGET(pHelper), bHasSettings);
    }

    g_free(szValue);
  }
}

// --- Special Cases ---

static void dasher_preferences_dialogue_populate_special_speed(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  double dNewValue = pPrivate->pAppSettings->GetLong(LP_MAX_BITRATE) / 100.0;
  gtk_range_set_value(pPrivate->pSpeedSlider, dNewValue);
}

static void dasher_preferences_dialogue_populate_special_mouse_start(DasherPreferencesDialogue *pSelf) {
#ifndef WITH_MAEMO
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  pPrivate->pMousePosButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "mouseposbutton"));
  pPrivate->pMousePosStyle = GTK_COMBO_BOX(gtk_builder_get_object(pPrivate->pXML, "MousePosStyle"));

  if(pPrivate->pAppSettings->GetBool(BP_MOUSEPOS_MODE)) {
    gtk_combo_box_set_active(pPrivate->pMousePosStyle, 1);
    gtk_toggle_button_set_active(pPrivate->pMousePosButton, true);
  }
  else if(pPrivate->pAppSettings->GetBool(BP_CIRCLE_START)) {
    gtk_combo_box_set_active(pPrivate->pMousePosStyle, 0);
    gtk_toggle_button_set_active(pPrivate->pMousePosButton, true);
  }
  else {
    gtk_toggle_button_set_active(pPrivate->pMousePosButton, false);
  }
#endif 
}

static void dasher_preferences_dialogue_populate_special_orientation(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  pPrivate->pAlphOrient = GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton1"));
  pPrivate->pLRButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton2"));
  pPrivate->pRLButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton3"));
  pPrivate->pTBButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton4"));
  pPrivate->pBTButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton5"));
  GtkToggleButton *pButton;
  switch (pPrivate->pAppSettings->GetLong(LP_ORIENTATION)) {
  case Dasher::Opts::AlphabetDefault:
    pButton = pPrivate->pAlphOrient; break;

  case Dasher::Opts::LeftToRight:
    pButton = pPrivate->pLRButton; break;

  case Dasher::Opts::RightToLeft:
    pButton = pPrivate->pRLButton; break;

  case Dasher::Opts::TopToBottom:
    pButton = pPrivate->pTBButton; break;

  case Dasher::Opts::BottomToTop:
    pButton = pPrivate->pBTButton; break;

  default:
    return;
  }

  if(gtk_toggle_button_get_active(pButton) != TRUE)
    gtk_toggle_button_set_active(pButton, TRUE);
}

static void dasher_preferences_dialogue_populate_special_appstyle(DasherPreferencesDialogue *pSelf) {
#ifndef WITH_MAEMO  
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  switch(pPrivate->pAppSettings->GetLong(APP_LP_STYLE)) {
  case 0:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "appstyle_classic")), TRUE);
    break;
  case 1: 
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "appstyle_compose")), TRUE);
    break;
  case 2:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "appstyle_direct")), TRUE);
    break;  
  case 3:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "appstyle_fullscreen")), TRUE);
    break;
  }
#endif
}

static void dasher_preferences_dialogue_populate_special_linewidth(DasherPreferencesDialogue *pSelf) {
#ifndef WITH_MAEMO
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  if(pPrivate->pAppSettings->GetLong(LP_LINE_WIDTH) > 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "thicklinebutton")), true);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "thicklinebutton")), false);
#endif
}


static void dasher_preferences_dialogue_populate_special_lm(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  switch( pPrivate->pAppSettings->GetLong(LP_LANGUAGE_MODEL_ID )) {
  case 0:
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton6"))) != TRUE)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton6")), TRUE);
    break;
  case 2: 
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton7"))) != TRUE)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton7")), TRUE);
    break;
  case 3:
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton8"))) != TRUE)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton8")), TRUE);
    break;
  case 4:
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton9"))) != TRUE)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "radiobutton9")), TRUE);
    break;
  default:
    break;
  }
}

static void dasher_preferences_dialogue_populate_special_uniform(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  gtk_range_set_value( GTK_RANGE(gtk_builder_get_object(pPrivate->pXML, "uniformhscale")), pPrivate->pAppSettings->GetLong(LP_UNIFORM)/10.0);
}

static void dasher_preferences_dialogue_populate_special_colour(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "manual_colour")), !pPrivate->pAppSettings->GetBool(BP_PALETTE_CHANGE));
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(pPrivate->pXML, "ColorTree")), !pPrivate->pAppSettings->GetBool(BP_PALETTE_CHANGE));
}

static void dasher_preferences_dialogue_populate_special_dasher_font(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  GObject *pDasherFontButton = gtk_builder_get_object(pPrivate->pXML, "dasher_fontbutton");

  gtk_font_button_set_font_name(GTK_FONT_BUTTON(pDasherFontButton), 
                                pPrivate->pAppSettings->GetString(SP_DASHER_FONT).c_str());
}

static void dasher_preferences_dialogue_populate_special_edit_font(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  GObject *pEditFontButton = gtk_builder_get_object(pPrivate->pXML, "edit_fontbutton");

  gtk_font_button_set_font_name(GTK_FONT_BUTTON(pEditFontButton), 
                                pPrivate->pAppSettings->GetString(APP_SP_EDIT_FONT).c_str());
}
 
static void dasher_preferences_dialogue_populate_special_fontsize(DasherPreferencesDialogue *pSelf) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);

  int iValue = pPrivate->pAppSettings->GetLong(LP_DASHER_FONTSIZE);
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "fontsizenormal")), iValue == Opts::Normal);
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "fontsizelarge")), iValue == Opts::Big);
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "fontsizevlarge")), iValue == Opts::VBig);
}

static void dasher_preferences_dialogue_update_special(DasherPreferencesDialogue *pSelf, int iID) {
  for(unsigned int i(0); i < (sizeof(sSpecialControlTable) / sizeof(SpecialControl)); ++i) {
    if(((iID == -1) && sSpecialControlTable[i].bPrimary) || (sSpecialControlTable[i].iID == iID)) {
      (sSpecialControlTable[i].pPopulate)(pSelf);
    }
  }
}

// --- Callbacks for 'special case' controls ---

// TODO: Give these a systematic naming convention

extern "C" void OnMousePosChanged(GtkWidget *widget, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

  bool bActive = gtk_toggle_button_get_active(pPrivate->pMousePosButton);
  
  gtk_widget_set_sensitive(GTK_WIDGET(pPrivate->pMousePosStyle),bActive);
  
  int iIndex;
  if (bActive) {
	  iIndex=gtk_combo_box_get_active(pPrivate->pMousePosStyle);
	  if ((iIndex | 1) != 1) {
  	    //neither 0 or 1 => neither Circle nor Two-Box is actually selected
  	    // (i.e. combo box is empty) => forcibly select Circle
  	  gtk_combo_box_set_active(pPrivate->pMousePosStyle, iIndex=0);
	}
  } else iIndex=-1;
  
  pPrivate->pAppSettings->SetBool(BP_MOUSEPOS_MODE, iIndex==1);
  pPrivate->pAppSettings->SetBool(BP_CIRCLE_START, iIndex==0);
  
}

extern "C" void PrefsSpeedSliderChanged(GtkHScale *hscale, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL
  
  long iNewValue = long(round(gtk_range_get_value(GTK_RANGE(hscale)) * 100));
  pPrivate->pAppSettings->SetLong(LP_MAX_BITRATE, iNewValue);
}

extern "C" void orientation(GtkRadioButton *widget, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

  if(pPrivate->bIgnoreSignals)
    return;

  GtkToggleButton *pButton = GTK_TOGGLE_BUTTON(widget);
  Dasher::Opts::ScreenOrientations orient = Dasher::Opts::AlphabetDefault;
  if (pButton == pPrivate->pAlphOrient) {
    orient = Dasher::Opts::AlphabetDefault;
  } else if (pButton == pPrivate->pLRButton) {
    orient = Dasher::Opts::LeftToRight;
  } else if (pButton == pPrivate->pRLButton) {
   orient = Dasher::Opts::RightToLeft;
  } else if (pButton == pPrivate->pTBButton) {
    orient = Dasher::Opts::TopToBottom;
  } else if (pButton == pPrivate->pBTButton) {
    orient = Dasher::Opts::BottomToTop;
  }
  pPrivate->pAppSettings->SetLong(LP_ORIENTATION, orient);

}

extern "C" void ThickLineClicked(GtkWidget *widget, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    pPrivate->pAppSettings->SetLong(LP_LINE_WIDTH, 3);
  else
    pPrivate->pAppSettings->SetLong(LP_LINE_WIDTH, 1);
}

extern "C" void autocolour_clicked(GtkWidget *widget, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

  pPrivate->pAppSettings->SetLong(BP_PALETTE_CHANGE, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

extern "C" void mouseposstart_y_changed(GtkRange *widget, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

  int mouseposstartdist=int(gtk_adjustment_get_value(gtk_range_get_adjustment(widget)));
  pPrivate->pAppSettings->SetLong(LP_MOUSEPOSDIST, mouseposstartdist);
}

extern "C" void languagemodel(GtkRadioButton *widget, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

#if GTK_CHECK_VERSION (2,20,0)
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))==TRUE) {
    if( !strcmp( gtk_buildable_get_name( GTK_BUILDABLE(widget) ), "radiobutton6" ) ) {
      pPrivate->pAppSettings->SetLong(LP_LANGUAGE_MODEL_ID, 0 );
    } else if (!strcmp( gtk_buildable_get_name( GTK_BUILDABLE(widget) ), "radiobutton7" )) {
      pPrivate->pAppSettings->SetLong(LP_LANGUAGE_MODEL_ID, 2 );
    } else if (!strcmp( gtk_buildable_get_name( GTK_BUILDABLE(widget) ), "radiobutton8" )) {
      pPrivate->pAppSettings->SetLong(LP_LANGUAGE_MODEL_ID, 3 );
    } else if (!strcmp( gtk_buildable_get_name( GTK_BUILDABLE(widget) ), "radiobutton9" )) {
      pPrivate->pAppSettings->SetLong(LP_LANGUAGE_MODEL_ID, 4 );
    }
  }
#else
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))==TRUE) {
    if( !strcmp( gtk_widget_get_name( GTK_WIDGET(widget) ), "radiobutton6" ) ) {
      pPrivate->pAppSettings->SetLong(LP_LANGUAGE_MODEL_ID, 0 );
    } else if (!strcmp( gtk_widget_get_name( GTK_WIDGET(widget) ), "radiobutton7" )) {
      pPrivate->pAppSettings->SetLong(LP_LANGUAGE_MODEL_ID, 2 );
    } else if (!strcmp( gtk_widget_get_name( GTK_WIDGET(widget) ), "radiobutton8" )) {
      pPrivate->pAppSettings->SetLong(LP_LANGUAGE_MODEL_ID, 3 );
    } else if (!strcmp( gtk_widget_get_name( GTK_WIDGET(widget) ), "radiobutton9" )) {
      pPrivate->pAppSettings->SetLong(LP_LANGUAGE_MODEL_ID, 4 );
    }
  }
#endif
}

extern "C" void uniform_changed(GtkHScale *hscale) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

  
  int iValue = (int)(gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(hscale))) * 10);

  if(iValue < 50) {
    iValue = 50;
    gtk_range_set_value(GTK_RANGE(gtk_builder_get_object(pPrivate->pXML, "uniformhscale")), 5.0);
  }
  
  pPrivate->pAppSettings->SetLong(LP_UNIFORM, iValue);
}

extern "C" gboolean show_helper_window(GtkWidget *pWidget, gpointer *pUserData) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue);
  GtkTreeView *pView = GTK_TREE_VIEW(pUserData);
  GtkTreeSelection *pSelection = gtk_tree_view_get_selection(pView);
  
  GtkTreeIter oIter;
  GtkTreeModel *pModel;
  
  if(!gtk_tree_selection_get_selected(pSelection, &pModel, &oIter))
    DASHER_ASSERT(false); //button should not be sensitive if nothing selected
    
  int iParameter;
  gchar *szValue;
  gtk_tree_model_get(pModel, &oIter, 0, &iParameter, 2, &szValue, -1);
  
  SModuleSettings *pSettings;
  int iCount;
  if (!pPrivate->pAppSettings->GetModuleSettings(szValue, &pSettings, &iCount))
    DASHER_ASSERT(false); //button should only be sensitive if item has settings
  
  GtkWidget *pWindow = module_settings_window_new(pPrivate->pAppSettings, szValue, pSettings, iCount);
  g_free(szValue);
  
  gtk_window_present(GTK_WINDOW(pWindow));
  
  return FALSE;
}

extern "C" void on_appstyle_changed(GtkWidget *widget, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

#if GTK_CHECK_VERSION (2,20,0)
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    if(!strcmp(gtk_buildable_get_name(GTK_BUILDABLE(widget)), "appstyle_classic"))
      pPrivate->pAppSettings->SetLong(APP_LP_STYLE, 0);
    else if(!strcmp(gtk_buildable_get_name(GTK_BUILDABLE(widget)), "appstyle_compose"))
      pPrivate->pAppSettings->SetLong(APP_LP_STYLE, 1);
    else if(!strcmp(gtk_buildable_get_name(GTK_BUILDABLE(widget)), "appstyle_direct"))
      pPrivate->pAppSettings->SetLong(APP_LP_STYLE, 2);
    else if(!strcmp(gtk_buildable_get_name(GTK_BUILDABLE(widget)), "appstyle_fullscreen"))
      pPrivate->pAppSettings->SetLong(APP_LP_STYLE, 3);
  }
#else
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    if(!strcmp(gtk_widget_get_name(GTK_WIDGET(widget)), "appstyle_classic"))
      pPrivate->pAppSettings->SetLong(APP_LP_STYLE, 0);
    else if(!strcmp(gtk_widget_get_name(GTK_WIDGET(widget)), "appstyle_compose"))
      pPrivate->pAppSettings->SetLong(APP_LP_STYLE, 1);
    else if(!strcmp(gtk_widget_get_name(GTK_WIDGET(widget)), "appstyle_direct"))
      pPrivate->pAppSettings->SetLong(APP_LP_STYLE, 2);
    else if(!strcmp(gtk_widget_get_name(GTK_WIDGET(widget)), "appstyle_fullscreen"))
      pPrivate->pAppSettings->SetLong(APP_LP_STYLE, 3);
  }
#endif
}

extern "C" void on_dasher_font_changed(GtkFontButton *pButton, gpointer pUserData) {
  DasherMainPrivate *pMainPrivate = DASHER_MAIN_GET_PRIVATE(pUserData);
  pMainPrivate->pAppSettings->SetString(
                                 SP_DASHER_FONT, 
                                 gtk_font_button_get_font_name(pButton));
}

extern "C" void on_edit_font_changed(GtkFontButton *pButton, gpointer pUserData) {
  DasherMainPrivate *pMainPrivate = DASHER_MAIN_GET_PRIVATE(pUserData);
  pMainPrivate->pAppSettings->SetString(
                                 APP_SP_EDIT_FONT, 
                                 gtk_font_button_get_font_name(pButton));
}

extern "C" void set_dasher_fontsize(GtkWidget *pWidget, gboolean pUserData) {
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue);

  int iValue = pPrivate->pAppSettings->GetLong(LP_DASHER_FONTSIZE);

  if((iValue != Opts::Normal) && gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "fontsizenormal"))))
    pPrivate->pAppSettings->SetLong(LP_DASHER_FONTSIZE, Opts::Normal);
  else if((iValue != Opts::Big) && gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "fontsizelarge"))))
    pPrivate->pAppSettings->SetLong(LP_DASHER_FONTSIZE, Opts::Big);
  else if((iValue != Opts::VBig) && gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(gtk_builder_get_object(pPrivate->pXML, "fontsizevlarge"))))
    pPrivate->pAppSettings->SetLong(LP_DASHER_FONTSIZE, Opts::VBig);
}


// --- General Callbacks ---

// For general purpose operation of the dialogue, rather than dealing
// with changing specific options

extern "C" gboolean preferences_hide(GtkWidget *widget, gpointer user_data) {
  //  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(pSelf);
  DasherPreferencesDialoguePrivate *pPrivate = DASHER_PREFERENCES_DIALOGUE_PRIVATE(g_pPreferencesDialogue); // TODO: Fix NULL

  gtk_widget_hide(GTK_WIDGET(pPrivate->pPreferencesWindow));
  return TRUE;
}


// --- TODOS:

#ifdef WITH_MAEMO
extern "C" void on_window_size_changed(GtkWidget *widget, gpointer user_data) {
  if(GTK_TOGGLE_BUTTON(widget)->active)
    dasher_app_settings_set_long(pPrivate->pAppSettings, APP_LP_MAEMO_SIZE, 1);
  else
    dasher_app_settings_set_long(pPrivate->pAppSettings, APP_LP_MAEMO_SIZE, 0);
}
#endif

