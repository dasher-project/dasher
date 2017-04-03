// SettingsStore.cpp
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008 Iain Murray
//
/////////////////////////////////////////////////////////////////////////////

#include "../Common/Common.h"

#include "SettingsStore.h"
#include "Event.h"
#include "Observable.h"

#include <cstring>
#include <cstdlib>
#include <iostream>

using namespace std;
using namespace Dasher;
using namespace Dasher::Settings;

// Track memory leaks on Windows to the line that new'd the memory
#ifdef _WIN32
#ifdef _DEBUG_MEMLEAKS
#define DEBUG_NEW new( _NORMAL_BLOCK, THIS_FILE, __LINE__ )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

static CSettingsStore *s_pSettingsStore = NULL;

CSettingsStore::CSettingsStore() {
}

void CSettingsStore::LoadPersistent() {
  // Load each of the persistent parameters.  If we fail loading for the store, then 
  // we'll save the settings with the default value that comes from Parameters.h
  AddParameters(boolparamtable, NUM_OF_BPS);
  AddParameters(longparamtable, NUM_OF_LPS);
  AddParameters(stringparamtable, NUM_OF_SPS);
}

void CSettingsStore::AddParameters(const Settings::bp_table* table, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    const auto& e = table[i];
    auto &parameter = parameters_[e.key];
    DASHER_ASSERT(parameter.type == Settings::ParamInvalid);
    parameter.type = ParamBool;
    parameter.name = e.regName;
    parameter.bool_default = e.defaultValue;
    parameter.persistence = e.persistent;
    if (!LoadSetting(e.regName, &parameter.bool_value)) {
      parameter.bool_value = e.defaultValue;
      SaveSetting(e.regName, e.defaultValue);
    }
  }
}

void CSettingsStore::AddParameters(const Settings::lp_table* table, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    const lp_table& e = table[i];
    auto &parameter = parameters_[e.key];
    DASHER_ASSERT(parameter.type == Settings::ParamInvalid);
    parameter.type = ParamLong;
    parameter.name = e.regName;
    parameter.long_default = e.defaultValue;
    parameter.persistence = e.persistent;
    if (!LoadSetting(e.regName, &parameter.long_value)) {
      parameter.long_value = e.defaultValue;
      SaveSetting(e.regName, e.defaultValue);
    }
  }
}

void CSettingsStore::AddParameters(const Settings::sp_table* table, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    const auto& e = table[i];
    auto &parameter = parameters_[e.key];
    DASHER_ASSERT(parameter.type == Settings::ParamInvalid);
    parameter.type = ParamString;
    parameter.name = e.regName;
    parameter.string_default = e.defaultValue;
    parameter.persistence = e.persistent;
    if (!LoadSetting(e.regName, &parameter.string_value)) {
      parameter.string_value = e.defaultValue;
      SaveSetting(e.regName, std::string(e.defaultValue));
    }
  }
}

// Return 0 on success, an error string on failure.
const char * CSettingsStore::ClSet(const std::string &strKey, const std::string &strValue) {
  for (auto& p : parameters_) {
    if(strKey == p.second.name) {
      switch (p.second.type) {
        case ParamBool: {
          if ((strValue == "0") || (strValue == _("true")) || (strValue == _("True")))
            SetBoolParameter(p.first, false);
          else if((strValue == "1") || (strValue == _("false")) || (strValue == _("False")))
            SetBoolParameter(p.first, true);
          else
            // Note to translators: This message will be output for a command line
            // with "--options foo=VAL" and foo is a boolean valued parameter, but
            // "VAL" is not true or false.
            return _("boolean value must be specified as 'true' or 'false'.");
          return nullptr;
        } break;

        case ParamLong: {
          // TODO: check the string to int conversion result.
          SetLongParameter(p.first, atoi(strValue.c_str()));
          return nullptr;
        }

        case ParamString: {
          SetStringParameter(p.first, strValue);
          return nullptr;
        }
        default:
          // Show unknown options.
          break;
      }
    }
  }
  // Note to translators: This is output when command line "--options" doesn't
  // specify a known option.
  return _("unknown option, use \"--help-options\" for more information.");
}


/* TODO: Consider using Template functions to make this neater. */

void CSettingsStore::SetBoolParameter(int iParameter, bool bValue) {
  auto p = parameters_.find(iParameter);
  // Check that the parameter is in fact in the right spot in the table
  DASHER_ASSERT(p != parameters_.end() && p->second.type == ParamBool);

  if(bValue == GetBoolParameter(iParameter))
    return;

  pre_set_observable_.DispatchEvent(CParameterChange(iParameter,bValue));

  // Set the value
  p->second.bool_value = bValue;

  // Initiate events for changed parameter
  DispatchEvent(iParameter);
  if (p->second.persistence == Persistence::PERSISTENT) {
    // Write out to permanent storage
    SaveSetting(p->second.name, bValue);
  }
}

void CSettingsStore::SetLongParameter(int iParameter, long lValue) {
  auto p = parameters_.find(iParameter);
  // Check that the parameter is in fact in the right spot in the table
  DASHER_ASSERT(p != parameters_.end() && p->second.type == ParamLong);

  if(lValue == GetLongParameter(iParameter))
    return;

  pre_set_observable_.DispatchEvent(CParameterChange(iParameter, lValue));

  // Set the value
  p->second.long_value = lValue;

  // Initiate events for changed parameter
  DispatchEvent(iParameter);
  if (p->second.persistence == Persistence::PERSISTENT) {
    // Write out to permanent storage
    SaveSetting(p->second.name, lValue);
  }
}

void CSettingsStore::SetStringParameter(int iParameter, const std::string sValue) {
  auto p = parameters_.find(iParameter);
  // Check that the parameter is in fact in the right spot in the table
  DASHER_ASSERT(p != parameters_.end() && p->second.type == ParamString);

  if(sValue == GetStringParameter(iParameter))
    return;

  pre_set_observable_.DispatchEvent(CParameterChange(iParameter, sValue.c_str()));

  // Set the value
  p->second.string_value = sValue;

  // Initiate events for changed parameter
  DispatchEvent(iParameter);
  if (p->second.persistence == Persistence::PERSISTENT) {
    // Write out to permanent storage
    SaveSetting(p->second.name, sValue);
  }
}

bool CSettingsStore::GetBoolParameter(int iParameter) const {
  auto p = parameters_.find(iParameter);
  // Check that the parameter is in fact in the right spot in the table
  DASHER_ASSERT(p != parameters_.end() && p->second.type == ParamBool);
  return p->second.bool_value;
}

long CSettingsStore::GetLongParameter(int iParameter) const {
  auto p = parameters_.find(iParameter);
  // Check that the parameter is in fact in the right spot in the table
  DASHER_ASSERT(p != parameters_.end() && p->second.type == ParamLong);
  return p->second.long_value;
}

const std::string &CSettingsStore::GetStringParameter(int iParameter) const {
  auto p = parameters_.find(iParameter);
  // Check that the parameter is in fact in the right spot in the table
  DASHER_ASSERT(p != parameters_.end() && p->second.type == ParamString);
  return p->second.string_value;
}

void CSettingsStore::ResetParameter(int iParameter) {
  auto p = parameters_.find(iParameter);
  switch(p->second.type) {
    case ParamBool:
      SetBoolParameter(iParameter, p->second.bool_default);
      break;
    case ParamLong:
      SetLongParameter(iParameter, p->second.long_default);
      break;
    case ParamString:
      SetStringParameter(iParameter, std::string(p->second.string_default));
      break;
    case ParamInvalid:
      // TODO: Error handling?
      break;
  }
}

/* Private functions -- Settings are not saved between sessions unless these
functions are over-ridden.
--------------------------------------------------------------------------*/

bool CSettingsStore::LoadSetting(const std::string &, bool *) {
  return false;
}

bool CSettingsStore::LoadSetting(const std::string &, long *) {
  return false;
}

bool CSettingsStore::LoadSetting(const std::string &, std::string *) {
  return false;
}

void CSettingsStore::SaveSetting(const std::string &, bool ) {
}

void CSettingsStore::SaveSetting(const std::string &, long ) {
}

void CSettingsStore::SaveSetting(const std::string &, const std::string &) {
}

/* SettingsUser and SettingsObserver definitions... */

CSettingsUser::CSettingsUser(CSettingsStore *pSettingsStore) {
  //ATM, we only allow one settings store, total...
  DASHER_ASSERT(s_pSettingsStore==NULL);
  //but in future, remove that if we're allowing multiple settings stores to exist
  // concurrently.
  s_pSettingsStore = pSettingsStore;
}

CSettingsUser::CSettingsUser(CSettingsUser *pCreateFrom) {
  //No need to do anything atm; but in future, copy CSettingsStore pointer
  // from argument.
  DASHER_ASSERT(pCreateFrom);
}

CSettingsUser::~CSettingsUser() {
}

bool CSettingsUser::GetBoolParameter(int iParameter) const {return s_pSettingsStore->GetBoolParameter(iParameter);}
long CSettingsUser::GetLongParameter(int iParameter) const {return s_pSettingsStore->GetLongParameter(iParameter);}
const std::string &CSettingsUser::GetStringParameter(int iParameter) const {return s_pSettingsStore->GetStringParameter(iParameter);}
void CSettingsUser::SetBoolParameter(int iParameter, bool bValue) {s_pSettingsStore->SetBoolParameter(iParameter, bValue);}
void CSettingsUser::SetLongParameter(int iParameter, long lValue) {s_pSettingsStore->SetLongParameter(iParameter, lValue);}
void CSettingsUser::SetStringParameter(int iParameter, const std::string &strValue) {s_pSettingsStore->SetStringParameter(iParameter, strValue);}

CSettingsObserver::CSettingsObserver(CSettingsUser *pCreateFrom) {
  DASHER_ASSERT(pCreateFrom);
  s_pSettingsStore->Register(this);
}

CSettingsObserver::~CSettingsObserver() {
  s_pSettingsStore->Unregister(this);
}

CSettingsUserObserver::CSettingsUserObserver(CSettingsUser *pCreateFrom)
: CSettingsUser(pCreateFrom), CSettingsObserver(pCreateFrom) {
}
