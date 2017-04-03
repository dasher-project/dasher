#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include "DasherAppSettings.h"
#include "../Common/AppSettingsData.h"

// FIXME - not sure that we're loading parameters from the registry at startup right now

// FIXME - shouldn't need to include this, but for now we need access to the global pointer to the dasher control.

#include "dasher.h"
#include "GtkDasherControl.h"

// TODO: Rename this file to fit in with naming conventions

using namespace Dasher::Settings;

std::unique_ptr<DasherAppSettings> DasherAppSettings::instance_;

DasherAppSettings::DasherAppSettings(Dasher::CSettingsStore* settings_store) {
  settings_store_.reset(settings_store);
  Load();
}

DasherAppSettings::~DasherAppSettings() {
  settings_store_->Unregister(this);
}

int DasherAppSettings::RegisterParameterChangeCallback(std::function<void(int)> fn) {
  int id = ++callback_id_;
  change_callbacks_[id] = fn;
  return id;
}

void DasherAppSettings::UnregisterParameterChangeCallback(int callback_id) {
  change_callbacks_.erase(callback_id);
}

void DasherAppSettings::Load() {
  settings_store_->AddParameters(app_boolparamtable, NUM_OF_APP_BPS);
  settings_store_->AddParameters(app_longparamtable, NUM_OF_APP_LPS);
  settings_store_->AddParameters(app_stringparamtable, NUM_OF_APP_SPS);
  settings_store_->Register(this);
}

bool DasherAppSettings::GetBool(int iParameter) {
  return settings_store_->GetBoolParameter(iParameter);
}

void DasherAppSettings::SetBool(int iParameter, bool bValue) {
  settings_store_->SetBoolParameter(iParameter, bValue);
}

gint DasherAppSettings::GetLong(int iParameter) {
  return settings_store_->GetLongParameter(iParameter);
}

void DasherAppSettings::SetLong(int iParameter, gint iValue) {
  return settings_store_->SetLongParameter(iParameter, iValue);
}

const std::string& DasherAppSettings::GetString(int iParameter) {
  return settings_store_->GetStringParameter(iParameter);
}

void DasherAppSettings::SetString(int iParameter, const gchar *szValue) {
  return settings_store_->SetStringParameter(iParameter, szValue);
}

int DasherAppSettings::GetCount() {
  return END_OF_APP_SPS;
}

bool DasherAppSettings::HaveAdvanced() {
  return (g_find_program_in_path("gconf-editor") != NULL);
}

void DasherAppSettings::LaunchAdvanced() {
  gchar *szArgs[3];

  szArgs[0] = g_strdup("gconf-editor");
  szArgs[1] = g_strdup("/apps/dasher4");
  szArgs[2] = NULL;

  GError *pError;

  if(!g_spawn_async(NULL, szArgs, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &pError)) {
    g_warning("Could not launch gconf-editor: %s", pError->message);
  }
  g_strfreev(szArgs);
}

void DasherAppSettings::SetWidget(GtkDasherControl *pWidget) {
  pDasherWidget = pWidget;
}

GArray *DasherAppSettings::GetAllowedValues(int iParameter) {
  return gtk_dasher_control_get_allowed_values(pDasherWidget, iParameter);
}

gboolean DasherAppSettings::GetModuleSettings(const gchar *szValue,
                                              SModuleSettings **pSettings,
                                              gint *iCount) {
  return gtk_dasher_control_get_module_settings(pDasherWidget, szValue, pSettings, iCount);
}

// Set the option szKey to szValue.  Return NULL if everything worked, a
// (literal) error string for unrecognized or illegal values.
const gchar * DasherAppSettings::ClSet(const gchar *szKey, const gchar *szValue) {
  return settings_store_->ClSet(szKey, szValue);
}

void DasherAppSettings::HandleEvent(int iParameter) {
  for (const auto& f : change_callbacks_) {
    f.second(iParameter);
  }
}

void option_help()
{
  g_print("\n");
  g_print("%-30s %-12s  %s\n", _("Boolean parameters"), _("Default"), _("Description"));
  g_print("%-30s %-12s  %s\n", "------------------------------", "------------", "------------------------------");
  for(unsigned int i=0; i < sizeof(app_boolparamtable)/sizeof(app_boolparamtable[0]); ++i) {
    g_print("%-30s %-12s  %s\n", app_boolparamtable[i].regName, (app_boolparamtable[i].defaultValue ? _("true") : _("false")), app_boolparamtable[i].humanReadable);
  }

  for(unsigned int i = 0; i < sizeof(boolparamtable)/sizeof(boolparamtable[0]); i++) {
    g_print("%-30s %-12s  %s\n", boolparamtable[i].regName, (boolparamtable[i].defaultValue ? _("true") : _("false")), boolparamtable[i].humanReadable);
  }

  g_print("\n");
  g_print("%-30s %-12s  %s\n", _("Integer parameters"), _("Default"), _("Description"));
  g_print("%-30s %-12s  %s\n", "------------------------------", "------------", "------------------------------");
  for(unsigned int i=0; i < sizeof(app_longparamtable)/sizeof(app_longparamtable[0]); ++i) {
    g_print("%-30s %12li  %s\n", app_longparamtable[i].regName, app_longparamtable[i].defaultValue, app_longparamtable[i].humanReadable);
  }

  for(unsigned int i = 0; i < sizeof(longparamtable)/sizeof(longparamtable[0]); i++) {
    g_print("%-30s %12li  %s\n", longparamtable[i].regName, longparamtable[i].defaultValue, longparamtable[i].humanReadable);
  }

  g_print("\n");
  g_print("%-30s %-12s  %s\n", _("String parameters"), _("Default"), _("Description"));
  g_print("%-30s %-12s  %s\n", "------------------------------", "------------", "------------------------------");
  for(unsigned int i=0; i < sizeof(app_stringparamtable)/sizeof(app_stringparamtable[0]); ++i) {
    g_print("%-30s %-12s  %s\n", app_stringparamtable[i].regName, app_stringparamtable[i].defaultValue, app_stringparamtable[i].humanReadable);
  }

  for(unsigned int i = 0; i < sizeof(stringparamtable)/sizeof(stringparamtable[0]); i++) {
    g_print("%-30s %-12s  %s\n", stringparamtable[i].regName, stringparamtable[i].defaultValue, stringparamtable[i].humanReadable);
  }
}

void option_version()
{
  g_print("\n");
  // Note to translators: This is the version keyword showing for the command line option "--version"
  g_print("%-30s %-30s \n", _("Version"), PACKAGE_VERSION);
  // Note to translators: This is the license keyword showing for the command line option "--version"
  g_print("%-30s %-30s\n", _("License"), "GPL 2+");
}
