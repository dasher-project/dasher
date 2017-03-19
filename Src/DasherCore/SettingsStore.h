// SettingsStore.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008 Iain Murray
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SettingsStore_h__
#define __SettingsStore_h__

#include <string>
#include <unordered_map>

#include "Observable.h"
#include "Parameters.h"

namespace Dasher {
/// \ingroup Core
/// @{

/// \brief Abstract representation of persistant storage.
///
/// Stores current runtime _values_ of all BP_, LP_, and SP_ preferences;
/// subclasses may load these from and persist them to disk;
/// is also an Observable for things that want to be notified when prefs change.
///
/// At present we allow for only one global SettingsStore across the whole of Dasher,
/// but the framework should allow for multiple SettingsStores with only minor changes.
/// (The exact use case for multiple SettingsStore's is not clear, and one suggestion is
/// that they should all share the same runtime data - perhaps persisting to different
/// locations. This requires only (a) defining SettingsObservers which persist changes
/// to arbitrary locations, or (b) make the actual pref-value data static i.e. shared
/// between instances.)
///
/// The public interface uses UTF-8 strings. All Keys should be
/// in American English and encodable in ASCII. However,
/// string Values may contain special characters where appropriate.
class CSettingsStore : public Observable<int> {
public:

  CSettingsStore();

  virtual ~CSettingsStore() = default;

  // New functions for event driven interface

  void SetBoolParameter(int iParameter, bool bValue);
  void SetLongParameter(int iParameter, long lValue);
  void SetStringParameter(int iParameter, const std::string sValue);

  bool GetBoolParameter(int iParameter) const;
  long GetLongParameter(int iParameter) const;
  const std::string &GetStringParameter(int iParameter) const;

  void ResetParameter(int iParameter);

  const char *ClSet(const std::string &strKey, const std::string &strValue);

  // TODO: just load the application parameters by default?
  void AddParameters(const Settings::bp_table* table, size_t count);
  void AddParameters(const Settings::lp_table* table, size_t count);
  void AddParameters(const Settings::sp_table* table, size_t count);
  Observable<CParameterChange>& PreSetObservable() { return pre_set_observable_; }

protected:
    ///Loads all (persistent) prefs from disk, using+storing default values when no
    /// existing value stored; non-persistent prefs are reinitialized from defaults.
    void LoadPersistent();
    
private:
  // Platform Specific settings file management

  // LoadSetting changes Value only if it succeeds in loading the setting,
  // in which case it also returns true. Failure is indicated by returning false.
  //! Load a setting with a boolean value
  //
  //! Load a setting with a boolean value. Return true if successful
  //! \param Key Name of the setting
  //! \param Value Value of the setting
  virtual bool LoadSetting(const std::string & Key, bool * Value);

  //! Load a setting with a long value
  //
  //! Load a setting with a long value. Return true if successful
  //! \param Key Name of the setting
  //! \param Value Value of the setting
  virtual bool LoadSetting(const std::string & Key, long *Value);

  //! Load a setting with a string value
  //
  //! Load a setting with a string value. Return true if successful
  //! \param Key Name of the setting
  //! \param Value Value of the setting, UTF8 encoded
  virtual bool LoadSetting(const std::string & Key, std::string * Value);

  //! Save a setting with a boolean value
  //
  //! \param Key Name of the setting
  //! \param Value Value of the setting
  virtual void SaveSetting(const std::string & Key, bool Value);

  //! Save a setting with a long value
  //
  //! \param Key Name of the setting
  //! \param Value Value of the setting
  virtual void SaveSetting(const std::string & Key, long Value);

  //! Save a setting with a string value
  //
  //! \param Key Name of the setting
  //! \param Value Value of the setting, UTF8 encoded
  virtual void SaveSetting(const std::string & Key, const std::string & Value);

  struct Parameter {
    const char* name;  // Doesn't own the string.
    Settings::ParameterType type = Settings::ParamInvalid;
    Persistence persistence = Persistence::PERSISTENT;
    bool bool_value;
    bool bool_default;
    long long_value;
    long long_default;
    std::string string_value;
    const char* string_default;  // Doesn't own the string.
  };

  std::unordered_map<int, Parameter> parameters_;
  Observable<CParameterChange> pre_set_observable_;
};
  /// Superclass for anything that wants to use/access/store persistent settings.
  /// (The nearest thing remaining to the old CDasherComponent,
  /// but more of a mixin rather than a universal superclass.)
  /// At the moment, _all_ clients share a single SettingsStore (static),
  /// but for future-proofing in case we ever want more than one SettingsStore
  /// (this has been suggested), SettingsUsers can only be created from other
  /// SettingsUsers (i.e. in a tree), so _could_ be modified to copy a SettingsStore
  /// pointer from the creator to inherit settings.
  class CSettingsUser {
  private:
    friend class CDasherInterfaceBase;
    ///Create the root of the SettingsUser hierarchy from a SettingsStore.
    /// ATM we allow only one SettingsStore, so this c'tor is private and 
    /// used only by the DasherInterface; if/when multiple SettingsStores
    /// are used, could be made public.
    CSettingsUser(CSettingsStore *pSettingsStore);
  public:
    virtual ~CSettingsUser();
  protected:
    ///Create a new SettingsUser, inheriting+sharing settings from the creator.
    CSettingsUser(CSettingsUser *pCreateFrom);
    bool GetBoolParameter(int iParameter) const;
    long GetLongParameter(int iParameter) const;
    const std::string &GetStringParameter(int iParameter) const;
    void SetBoolParameter(int iParameter, bool bValue);
    void SetLongParameter(int iParameter, long lValue);
    void SetStringParameter(int iParameter, const std::string &strValue);
  };
  ///Superclass for anything that wants to be notified when settings change.
  /// (Note inherited pure virtual HandleEvent(int) method, called when any pref changes).
  ///Exists as a distinct class from CSettingsUserObserver (below) to get round C++'s
  /// multiple inheritance problems, i.e. for indirect subclasses of CSettingsUser
  /// wanting to introduce settings-listener capabilities.
  ///Note we don't inherit from TransientObserver as it saves storing the SettingsStore ptr
  /// in every instance; if we move to multiple settings stores, we could so inherit.
  class CSettingsObserver : public Observer<int> {
  public:
    ///Create a CSettingsObserver listening to changes to the settings values
    /// used by a particular CSettingsUser.
    CSettingsObserver(CSettingsUser *pCreateFrom);
    ~CSettingsObserver() override;
  };
  ///Utility class, for (majority of) cases where a class wants to be both
  /// a CSettingsUser and CSettingsObserver.
  class CSettingsUserObserver : public CSettingsUser, public CSettingsObserver {
  public:
    CSettingsUserObserver(CSettingsUser *pCreateFrom);
  };
/// @}
}
#endif /* #ifndef __SettingsStore_h__ */
