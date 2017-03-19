// Parameters.h
//
// Copyright (c) 2007 The Dasher Team
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

#ifndef __parameters_h__
#define __parameters_h__

#include "../Common/Common.h"

#include <string>

// All parameters go into the enums here
// They are unique across the different types
enum { 
  BP_DRAW_MOUSE_LINE, BP_DRAW_MOUSE, BP_CURVE_MOUSE_LINE,
  BP_START_MOUSE,
  BP_START_SPACE, BP_CONTROL_MODE, 
  BP_MOUSEPOS_MODE,
  BP_PALETTE_CHANGE, BP_TURBO_MODE, BP_EXACT_DYNAMICS,
  BP_AUTOCALIBRATE, BP_REMAP_XTREME,
  BP_AUTO_SPEEDCONTROL,
  BP_LM_ADAPTIVE, BP_SOCKET_DEBUG, 
  BP_CIRCLE_START, BP_GLOBAL_KEYBOARD, BP_NONLINEAR_Y,
  BP_STOP_OUTSIDE, BP_BACKOFF_BUTTON,
  BP_TWOBUTTON_REVERSE, BP_2B_INVERT_DOUBLE, BP_SLOW_START,
  BP_COPY_ALL_ON_STOP, BP_SPEAK_ALL_ON_STOP, BP_SPEAK_WORDS,
  BP_GAME_HELP_DRAW_PATH, BP_TWO_PUSH_RELEASE_TIME,
  BP_SLOW_CONTROL_BOX,
  END_OF_BPS
};

enum { 
  LP_ORIENTATION = END_OF_BPS, LP_MAX_BITRATE, LP_FRAMERATE,
  LP_LANGUAGE_MODEL_ID, LP_DASHER_FONTSIZE, LP_MESSAGE_FONTSIZE, LP_SHAPE_TYPE,
  LP_UNIFORM, LP_YSCALE, LP_MOUSEPOSDIST, LP_PY_PROB_SORT_THRES, LP_MESSAGE_TIME,
  LP_LM_MAX_ORDER, LP_LM_EXCLUSION,
  LP_LM_UPDATE_EXCLUSION, LP_LM_ALPHA, LP_LM_BETA,
  LP_LM_MIXTURE, LP_LINE_WIDTH, LP_GEOMETRY,
  LP_LM_WORD_ALPHA, LP_USER_LOG_LEVEL_MASK, 
  LP_ZOOMSTEPS, LP_B, LP_S, LP_BUTTON_SCAN_TIME, LP_R, LP_RIGHTZOOM,
  LP_NODE_BUDGET, LP_OUTLINE_WIDTH, LP_MIN_NODE_SIZE, LP_NONLINEAR_X,
  LP_AUTOSPEED_SENSITIVITY, LP_SOCKET_PORT, LP_SOCKET_INPUT_X_MIN, LP_SOCKET_INPUT_X_MAX,
  LP_SOCKET_INPUT_Y_MIN, LP_SOCKET_INPUT_Y_MAX,
  LP_CIRCLE_PERCENT, LP_TWO_BUTTON_OFFSET, LP_HOLD_TIME, LP_MULTIPRESS_TIME,
  LP_SLOW_START_TIME,
  LP_TWO_PUSH_OUTER, LP_TWO_PUSH_LONG, LP_TWO_PUSH_SHORT, LP_TWO_PUSH_TOLERANCE,
  LP_DYNAMIC_BUTTON_LAG, LP_STATIC1B_TIME, LP_STATIC1B_ZOOM,
  LP_DEMO_SPRING, LP_DEMO_NOISE_MEM, LP_DEMO_NOISE_MAG, LP_MAXZOOM, 
  LP_DYNAMIC_SPEED_INC, LP_DYNAMIC_SPEED_FREQ, LP_DYNAMIC_SPEED_DEC,
  LP_TAP_TIME, LP_MARGIN_WIDTH, LP_TARGET_OFFSET, LP_X_LIMIT_SPEED,
  LP_GAME_HELP_DIST, LP_GAME_HELP_TIME,
  END_OF_LPS
};

enum {
  SP_ALPHABET_ID = END_OF_LPS, SP_ALPHABET_1, SP_ALPHABET_2, SP_ALPHABET_3, SP_ALPHABET_4, 
  SP_COLOUR_ID, SP_CONTROL_BOX_ID, SP_DASHER_FONT, SP_GAME_TEXT_FILE,
  SP_SOCKET_INPUT_X_LABEL, SP_SOCKET_INPUT_Y_LABEL, SP_INPUT_FILTER, SP_INPUT_DEVICE,
  SP_BUTTON_0, SP_BUTTON_1, SP_BUTTON_2, SP_BUTTON_3, SP_BUTTON_4, SP_BUTTON_10, SP_JOYSTICK_DEVICE,
  END_OF_SPS
};

// Define first int value of the first element of each type.
// Useful for offsetting into specific arrays,
// since each setting is a unique int, but all 3 arrays start at 0
#define FIRST_BP 0
#define FIRST_LP END_OF_BPS
#define FIRST_SP END_OF_LPS

// Define the number of each type of setting
#define NUM_OF_BPS END_OF_BPS
#define NUM_OF_LPS (END_OF_LPS - END_OF_BPS)
#define NUM_OF_SPS (END_OF_SPS - END_OF_LPS)

enum class Persistence { PERSISTENT, EPHEMERAL };

struct CParameterChange {
    CParameterChange(int parameter, bool value)
        :iParameter(parameter), bool_value(value) {}
    CParameterChange(int parameter, long value)
        :iParameter(parameter), long_value(value) {}
    CParameterChange(int parameter, const char* value)
        :iParameter(parameter),string_value(value){}
    int iParameter;
    bool bool_value = false;
    long long_value = 0;
    const char* string_value = "";
};

namespace Dasher {
  ///Namespace containing all static (i.e. fixed/constant) data about
  /// settings, that is _not_ dependent on the storage mechanism,
  /// the SettingsStore in use, or platform-specific details.
  /// (Except, some defaults are #ifdef'd according to platform).
  /// This data does NOT change at runtime.
  namespace Settings {
    ///Structure storing fixed data about bool settings...
    struct bp_table {
      int key;
      const char *regName;
      Persistence persistent;
      bool defaultValue;
      const char *humanReadable;
    };
    ///One bp_table per bool param, in the same order as the enum declarations
    /// (i.e.: boolparamtable[x]->key == x-FIRST_BP)
    extern const bp_table boolparamtable[NUM_OF_BPS];

    ///Structure storing fixed data about long settings...
    struct lp_table {
      int key;
      const char *regName;
      Persistence persistent;
      long defaultValue;
      const char *humanReadable;
    };
    ///One lp_table per long param, in the same order as the enum declarations
    /// (i.e.: longparamtable[x]->key == x-FIRST_LP)
    extern const lp_table longparamtable[NUM_OF_LPS];
    
    ///Structure storing fixed data about string settings...
    struct sp_table {
      int key;
      const char *regName;
      Persistence persistent;
      const char *defaultValue;
      const char *humanReadable;
    };
    
    ///One sp_table per string param, in the same order as the enum declarations
    /// (i.e.: stringparamtable[x]->key == x-FIRST_SP)
    extern const sp_table stringparamtable[NUM_OF_SPS];
    
    // Types that are parameters can be
    enum ParameterType {
      ParamBool,
      ParamLong,
      ParamString,
      ParamInvalid
    };
    
    ///Get the type of a parameter by its key.
    /// \param iParameter one of the BP_*, LP_* or SP_* enum constants
    /// \return ParamBool, ParamLong or ParamString, respectively; or
    /// ParamInvalid if iParameter is not in the range of those enums.
    ParameterType GetParameterType(int iParameter);
    
    ///Gets the regName member of the struct for a parameter (of any of the 3 types).
    /// This is appropriate for use as a key for storing the setting value into e.g. a registry.
    /// Note - returns a string not a reference to one, because the table stores only a char*.
    /// \param iParameter one of the BP_*, LP_* or SP_* enum constants
    /// \return the regName member of the corresponding bp_table, lp_table,
    /// or sp_table struct.
    std::string GetParameterName(int iParameter);
  }
}
#endif
