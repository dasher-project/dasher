#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glib/gi18n.h>

#include "Common/Common.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <signal.h>
#include <Gtk2/DasherAppSettings.h>
/* Just to make sure the symbols for the editor are visible. */
// #include <Gtk2/dasher_editor_internal.h>

#ifdef WITH_MAEMO
#include <libosso.h>
#endif

// TODO: This shouldn't need to be here
#ifdef USE_CSPI
#include <libbonobo.h>
#endif

#ifdef WITH_GPE
#include <gpe/init.h>
#endif

#include "dasher.h"
#include "dasher_main.h"

#ifdef WITH_GPE
#include "gpesettings_store.h"
#endif

//extern int optind;
//extern const gchar *filename;

DasherMain *g_pDasherMain;


// Stuff imported from dasher.cc

#ifdef WITH_MAEMO
  osso_context_t *osso_context;
#endif



//static GtkWidget *vbox; // Main vbox (top level under main window)
// GdkPixbuf *p;                   // Hmm... descriptive names
// GtkWidget *pw;
// GtkStyle *style;

// GtkWidget *dasher_menu_bar;
// GtkWidget *open_filesel;
// GtkWidget *save_filesel;
// GtkWidget *save_and_quit_filesel;
// GtkWidget *import_filesel;
// GtkWidget *append_filesel;
// GtkWidget *g_pHiddenWindow;
// GtkWidget *file_selector;
//GtkWidget *g_pEditPane = 0;
//GtkWidget *g_pActionPane = 0;

#ifdef WITH_MAEMO
Window g_xOldIMWindow; 
#endif

//DasherAction *g_pAction = 0;

// const gchar *filename = NULL;   // Filename of file currently being edited


// Apparently not obsolete, but should be sorted out

// gboolean file_modified = FALSE; // Have unsaved changes been made to the current file
// gint outputcharacters;

// const char *g_szAccessibleContext = 0;
// int g_iExpectedPosition = -1;
// int g_iOldPosition = -1;

// 'Private' methods



// "member" variables for main window "class"

// int g_bOnTop = true; // Whether the window should always be on top
// int g_bDock = true; // Whether to dock the window
// int g_iDockType; // Ignored for now - will determine how the window is docked to the side of the screen
// double g_dXFraction = 0.25; // Fraction of the width of the screen to use;
// double g_dYFraction = 0.25; // Fraction of the height of the screen to use;

/// ---

/// Old stuff from edit.cc

// GtkWidget *the_text_view;
// GtkTextBuffer *the_text_buffer;

// KeySym *origkeymap;
// int modifiedkey = 0;
// int numcodes;

/// ---

void sigint_handler(int iSigNum);
void clean_up();

// TODO: reimplement command line parsing - should just be a way of
// temporarily overriding parameters

// GOption command line parsing variables
// gboolean timedata = FALSE;
// gboolean preferences = FALSE;
// gboolean textentry = FALSE;
// gboolean stdoutpipe = FALSE;


// TODO: Do we actually need this - gtk should in theory have
// functions to prevent windows from taking focus, but maybe they
// don't work which is why this is here.

// GdkFilterReturn dasher_discard_take_focus_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data) {
//   XEvent *xev = (XEvent *) xevent;
//   if(xev->xany.type == ClientMessage && (Atom) xev->xclient.data.l[0] == gdk_x11_atom_to_xatom(gdk_atom_intern("WM_TAKE_FOCUS", False))) {
//     return GDK_FILTER_REMOVE;
//   }
//   else {
//     return GDK_FILTER_CONTINUE;
//   }
// }

extern "C" gint main_key_snooper(GtkWidget *pWidget, GdkEventKey *pEvent, gpointer pUserData);


int main(int argc, char *argv[]) {

  //  DasherAppSettings *g_pDasherAppSettings;
  //  GtkWidget *window; // Main window

  signal(2, sigint_handler);

  bindtextdomain(PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  textdomain(PACKAGE);

  //  gchar *szOptionAppstyle = NULL;

  SCommandLine sCommandLine;

  gboolean do_option_help = false;
  gboolean do_show_version = false;
  static const GOptionEntry options[] = {
    //   {"timedata", 'w', 0, G_OPTION_ARG_NONE, &timedata, "Write basic timing information to stdout", NULL},
    //   {"preferences", 'p', 0, G_OPTION_ARG_NONE, &preferences, "Show preferences window only", NULL},
    //   {"textentry", 'o', 0, G_OPTION_ARG_NONE, &textentry, "Onscreen text entry mode", NULL},
    //   {"pipe", 's', 0, G_OPTION_ARG_NONE, &stdoutpipe, "Pipe text to stdout", NULL},
    /* Note to translators: This is the help string for "--appstyle". The four options in brackets MUST either NOT be translated or at least it MUST be clear that they must be used in english. Otherwise a user running a non-english system will receive an error message when using the translated one instead of the english one and has no chance to find out the correct option. */
    {"appstyle", 'a', 0, G_OPTION_ARG_STRING, &(sCommandLine.szAppStyle), N_("Application style (traditional, direct, compose or fullscreen)"), "traditional"},
    // Note to translators: This is the help string for "--options"
    {"options", 'o', 0, G_OPTION_ARG_STRING, &(sCommandLine.szOptions), N_("Override stored options"), NULL},
    {"config", 'c', 0, G_OPTION_ARG_STRING, &(sCommandLine.szConfigFile), N_("XML configuration file name"), NULL},
    // Note to translators: This is the help string for "--help-options"
    {"help-options", 0, 0, G_OPTION_ARG_NONE, &do_option_help, N_("Describe \"--options\"."), NULL},
    // Note to translators: This is the help string for "--version"
    {"version", 'v', 0, G_OPTION_ARG_NONE, &do_show_version, N_("Show the version details."), NULL},
    {NULL}
  };

  //parse command line options
  // Note to translators: This is the "--help" description of dasher.
  GOptionContext *goptcontext = g_option_context_new(_("- A text input application honouring accessibility"));
  g_option_context_add_main_entries(goptcontext, options, PACKAGE);
  g_option_context_add_group(goptcontext, gtk_get_option_group(TRUE));
  GError *error = nullptr;
  if (!g_option_context_parse(goptcontext, &argc, &argv, &error)) {
    g_print("option parsing failed: %s\n", error->message);
    // Note to translators: This is the help string for unrecognized command line options. Do not translate 'dasher --help' part.
    g_print("%s\n", _("Try 'dasher --help' for more information.")); 
    exit (1);
  }

  // TODO: Check what happens here when goption has done its stuff

  if(argc > 1)
    sCommandLine.szFilename = g_strdup(argv[1]);    
  //later GnomeProgram will call g_option_context_free() when we unref it
  if (do_option_help)
  {
    option_help();
    return 0;
  }

  if (do_show_version)
  {
    option_version();
    return 0;
  }

#ifdef WITH_GPE
  gpe_application_init(&argc, &argv);
  init_xsettings();
#else
  gtk_init(&argc, &argv);
#endif

#ifdef WITH_MAEMO
  osso_context = osso_initialize("dasher", PACKAGE_VERSION, TRUE, NULL);
#endif

#ifdef USE_CSPI
  if(!bonobo_is_initialized()) {
    if(!bonobo_init(&argc, argv)) {
      g_error("Can't initialize Bonobo...\n");
    }
    bonobo_activate();
  }
#endif

  g_set_application_name("Dasher");
#ifndef WITH_MAEMO
  gtk_window_set_default_icon_name("dasher");
#endif


  g_pDasherMain = dasher_main_new(&argc, &argv, &sCommandLine);

  g_free(sCommandLine.szFilename);
  g_free(sCommandLine.szAppStyle);
  g_free(sCommandLine.szOptions);
  g_free(sCommandLine.szConfigFile);

  if (g_pDasherMain == 0)
    return 1;

  dasher_main_show(g_pDasherMain);

  // This call is just to force the linker to export the following symbols.
  // dasher_editor_internal_get_type();

  // 10.
  gtk_main();

  // 11.
  clean_up();

  return 0;
}

void clean_up() {
#ifdef WITH_MAEMO
  osso_deinitialize(osso_context);
#endif

  /* TODO: check that this really does the right thing with the references counting */
  if(g_pDasherMain)
    g_object_unref(G_OBJECT(g_pDasherMain));
}

void sigint_handler(int iSigNum) { 
  if(gtk_main_level() > 0) {
    gtk_main_quit();  
  }
  else {
    // This implies that we haven't got as far as setting up the main loop yet
    clean_up();
    exit(0);
  }
}
