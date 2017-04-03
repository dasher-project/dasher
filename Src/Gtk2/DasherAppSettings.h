#ifndef __dasher_app_settings_h__
#define __dasher_app_settings_h__

#include <functional>
#include <map>
#include <memory>

#include <glib.h>
#include <glib-object.h>

#include "../DasherCore/Parameters.h"
#include "../Common/AppSettingsHeader.h"
#include "../Common/ModuleSettings.h"
#include "../DasherCore/SettingsStore.h"
#include "../DasherCore/Observable.h"

// Forward declarations
typedef struct _GtkDasherControl GtkDasherControl;
struct _GtkDasherControl;

// Define the number of each type of setting
#define NUM_OF_APP_BPS (END_OF_APP_BPS - END_OF_SPS)
#define NUM_OF_APP_LPS (END_OF_APP_LPS - END_OF_APP_BPS)
#define NUM_OF_APP_SPS (END_OF_APP_SPS - END_OF_APP_LPS)

// Singleton holding the settings.
class DasherAppSettings : public Observer<int> {
public:
  // Returns the AppSettings singleton, 'Create' must have been called once beforehand.
  static DasherAppSettings* Get() {
    return instance_.get();
  }
  virtual ~DasherAppSettings();

  // Initializes the AppSettings singleton and takes ownership of 'store'.
  static void Create(Dasher::CSettingsStore* store) {
    if (instance_ == nullptr) {
      instance_.reset(new DasherAppSettings(store));
    }
  }

  // 'fn' will be called each time a parameter is changed and its argument will be
  // the setting id.
  // The return value is used to unregister the callback.
  int RegisterParameterChangeCallback(std::function<void(int)> fn);
  // Use the value returned by 'RegisterParameterChangeCallback' to unregister it.
  void UnregisterParameterChangeCallback(int);

  void Load();
  void SetWidget(GtkDasherControl *pWidget);
  bool GetBool(int iParameter);
  void SetBool(int iParameter, bool bValue);
  gint GetLong(int iParameter);
  void SetLong(int iParameter, gint iValue);
  const std::string& GetString(int iParameter);
  void SetString(int iParameter, const gchar *szValue);
  int GetCount();
  GArray *GetAllowedValues(int iParameter);
  bool HaveAdvanced();
  void LaunchAdvanced();
  gboolean GetModuleSettings(const gchar *szValue, SModuleSettings **pSettings, gint *iCount);
  const gchar * ClSet(const gchar *szKey, const gchar *szValue);
  Dasher::CSettingsStore* GetStore() { return settings_store_.get(); }

private:
  DasherAppSettings(Dasher::CSettingsStore* store);
  // The callback for the SettingsStore notifications.
  void HandleEvent(int iParameter) override;

  // The object is non copyable.
  DasherAppSettings(const DasherAppSettings&) = delete;
  void operator=(const DasherAppSettings&) = delete;

  // Holds the singleton instance.
  static std::unique_ptr<DasherAppSettings> instance_;

  GtkDasherControl *pDasherWidget = nullptr;
  std::unique_ptr<Dasher::CSettingsStore> settings_store_ = nullptr;
  std::map<int, std::function<void(int)>> change_callbacks_;
  int callback_id_ = 0;
};

void option_help();
void option_version();
#endif
