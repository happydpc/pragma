/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __INPUTHELPER_H__
#define __INPUTHELPER_H__
#include "pragma/definitions.h"
#include <algorithm>
#include <string>
#ifndef GLFW_DLL
	#define GLFW_KEY_MENU               348
	#define GLFW_KEY_LAST               GLFW_KEY_MENU
	/* Printable keys */
	#define GLFW_KEY_SPACE              32
	#define GLFW_KEY_APOSTROPHE         39  /* ' */
	#define GLFW_KEY_COMMA              44  /* , */
	#define GLFW_KEY_MINUS              45  /* - */
	#define GLFW_KEY_PERIOD             46  /* . */
	#define GLFW_KEY_SLASH              47  /* / */
	#define GLFW_KEY_0                  48
	#define GLFW_KEY_1                  49
	#define GLFW_KEY_2                  50
	#define GLFW_KEY_3                  51
	#define GLFW_KEY_4                  52
	#define GLFW_KEY_5                  53
	#define GLFW_KEY_6                  54
	#define GLFW_KEY_7                  55
	#define GLFW_KEY_8                  56
	#define GLFW_KEY_9                  57
	#define GLFW_KEY_SEMICOLON          59  /* ; */
	#define GLFW_KEY_EQUAL              61  /* = */
	#define GLFW_KEY_A                  65
	#define GLFW_KEY_B                  66
	#define GLFW_KEY_C                  67
	#define GLFW_KEY_D                  68
	#define GLFW_KEY_E                  69
	#define GLFW_KEY_F                  70
	#define GLFW_KEY_G                  71
	#define GLFW_KEY_H                  72
	#define GLFW_KEY_I                  73
	#define GLFW_KEY_J                  74
	#define GLFW_KEY_K                  75
	#define GLFW_KEY_L                  76
	#define GLFW_KEY_M                  77
	#define GLFW_KEY_N                  78
	#define GLFW_KEY_O                  79
	#define GLFW_KEY_P                  80
	#define GLFW_KEY_Q                  81
	#define GLFW_KEY_R                  82
	#define GLFW_KEY_S                  83
	#define GLFW_KEY_T                  84
	#define GLFW_KEY_U                  85
	#define GLFW_KEY_V                  86
	#define GLFW_KEY_W                  87
	#define GLFW_KEY_X                  88
	#define GLFW_KEY_Y                  89
	#define GLFW_KEY_Z                  90
	#define GLFW_KEY_LEFT_BRACKET       91  /* [ */
	#define GLFW_KEY_BACKSLASH          92  /* \ */
	#define GLFW_KEY_RIGHT_BRACKET      93  /* ] */
	#define GLFW_KEY_GRAVE_ACCENT       96  /* ` */
	#define GLFW_KEY_WORLD_1            161 /* non-US #1 */
	#define GLFW_KEY_WORLD_2            162 /* non-US #2 */

	/* Function keys */
	#define GLFW_KEY_ESCAPE             256
	#define GLFW_KEY_ENTER              257
	#define GLFW_KEY_TAB                258
	#define GLFW_KEY_BACKSPACE          259
	#define GLFW_KEY_INSERT             260
	#define GLFW_KEY_DELETE             261
	#define GLFW_KEY_RIGHT              262
	#define GLFW_KEY_LEFT               263
	#define GLFW_KEY_DOWN               264
	#define GLFW_KEY_UP                 265
	#define GLFW_KEY_PAGE_UP            266
	#define GLFW_KEY_PAGE_DOWN          267
	#define GLFW_KEY_HOME               268
	#define GLFW_KEY_END                269
	#define GLFW_KEY_CAPS_LOCK          280
	#define GLFW_KEY_SCROLL_LOCK        281
	#define GLFW_KEY_NUM_LOCK           282
	#define GLFW_KEY_PRINT_SCREEN       283
	#define GLFW_KEY_PAUSE              284
	#define GLFW_KEY_F1                 290
	#define GLFW_KEY_F2                 291
	#define GLFW_KEY_F3                 292
	#define GLFW_KEY_F4                 293
	#define GLFW_KEY_F5                 294
	#define GLFW_KEY_F6                 295
	#define GLFW_KEY_F7                 296
	#define GLFW_KEY_F8                 297
	#define GLFW_KEY_F9                 298
	#define GLFW_KEY_F10                299
	#define GLFW_KEY_F11                300
	#define GLFW_KEY_F12                301
	#define GLFW_KEY_F13                302
	#define GLFW_KEY_F14                303
	#define GLFW_KEY_F15                304
	#define GLFW_KEY_F16                305
	#define GLFW_KEY_F17                306
	#define GLFW_KEY_F18                307
	#define GLFW_KEY_F19                308
	#define GLFW_KEY_F20                309
	#define GLFW_KEY_F21                310
	#define GLFW_KEY_F22                311
	#define GLFW_KEY_F23                312
	#define GLFW_KEY_F24                313
	#define GLFW_KEY_F25                314
	#define GLFW_KEY_KP_0               320
	#define GLFW_KEY_KP_1               321
	#define GLFW_KEY_KP_2               322
	#define GLFW_KEY_KP_3               323
	#define GLFW_KEY_KP_4               324
	#define GLFW_KEY_KP_5               325
	#define GLFW_KEY_KP_6               326
	#define GLFW_KEY_KP_7               327
	#define GLFW_KEY_KP_8               328
	#define GLFW_KEY_KP_9               329
	#define GLFW_KEY_KP_DECIMAL         330
	#define GLFW_KEY_KP_DIVIDE          331
	#define GLFW_KEY_KP_MULTIPLY        332
	#define GLFW_KEY_KP_SUBTRACT        333
	#define GLFW_KEY_KP_ADD             334
	#define GLFW_KEY_KP_ENTER           335
	#define GLFW_KEY_KP_EQUAL           336
	#define GLFW_KEY_LEFT_SHIFT         340
	#define GLFW_KEY_LEFT_CONTROL       341
	#define GLFW_KEY_LEFT_ALT           342
	#define GLFW_KEY_LEFT_SUPER         343
	#define GLFW_KEY_RIGHT_SHIFT        344
	#define GLFW_KEY_RIGHT_CONTROL      345
	#define GLFW_KEY_RIGHT_ALT          346
	#define GLFW_KEY_RIGHT_SUPER        347
	#define GLFW_KEY_MENU               348
	#define GLFW_KEY_LAST               GLFW_KEY_MENU
#endif

#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_1 (GLFW_KEY_LAST+0)
#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_2 (GLFW_KEY_LAST+1)
#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_3 (GLFW_KEY_LAST+2)
#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_4 (GLFW_KEY_LAST+3)
#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_5 (GLFW_KEY_LAST+4)
#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_6 (GLFW_KEY_LAST+5)
#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_7 (GLFW_KEY_LAST+6)
#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_8 (GLFW_KEY_LAST+7)
#define GLFW_KEY_SPECIAL_MOUSE_BUTTON_9 (GLFW_KEY_LAST+8)

#define GLFW_CUSTOM_KEY_SCRL_UP (GLFW_KEY_SPECIAL_MOUSE_BUTTON_9 +1)
#define GLFW_CUSTOM_KEY_SCRL_DOWN (GLFW_CUSTOM_KEY_SCRL_UP +1)

// Maximum amount of controls (keys and axes) per controller, has to be dividable by 2!
#define GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT 250
#define GLFW_MAX_JOYSTICK_COUNT 10
#define GLFW_MAX_VR_CONTROLLER_COUNT 10
#define GLFW_CUSTOM_KEY_JOYSTICK_KEY_COUNT (GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT /2)
#define GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT (GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT /2)

#define GLFW_CUSTOM_KEY_JOYSTICK_0_KEY_START (GLFW_CUSTOM_KEY_SCRL_DOWN +1000)
#define GLFW_CUSTOM_KEY_JOYSTICK_0_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_0_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_1_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_0_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_1_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_1_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_2_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_1_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_2_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_2_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_3_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_2_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_3_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_3_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_4_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_3_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_4_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_4_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_5_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_4_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_5_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_5_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_6_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_5_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_6_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_6_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_7_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_6_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_7_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_7_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_8_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_7_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_8_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_8_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_JOYSTICK_9_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_8_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_JOYSTICK_9_AXIS_START (GLFW_CUSTOM_KEY_JOYSTICK_9_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_0_KEY_START (GLFW_CUSTOM_KEY_JOYSTICK_9_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_0_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_0_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_1_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_0_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_1_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_1_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_2_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_1_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_2_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_2_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_3_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_2_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_3_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_3_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_4_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_3_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_4_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_4_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_5_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_4_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_5_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_5_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_6_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_5_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_6_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_6_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_7_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_6_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_7_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_7_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_8_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_7_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_8_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_8_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

#define GLFW_CUSTOM_KEY_VR_CONTROLLER_9_KEY_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_8_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_CONTROL_COUNT)
#define GLFW_CUSTOM_KEY_VR_CONTROLLER_9_AXIS_START (GLFW_CUSTOM_KEY_VR_CONTROLLER_9_KEY_START +GLFW_CUSTOM_KEY_JOYSTICK_AXIS_COUNT)

DLLENGINE bool get_controller_button(short &btId,uint32_t &controllerId,bool &axis);

DLLENGINE bool KeyToString(short c,std::string *key);
DLLENGINE bool KeyToText(short c,std::string *key);
DLLENGINE bool StringToKey(std::string key,short *c);
static const std::string BIND_KEYS[] = {
	"space",
	"escape",
	"f1",
	"f2",
	"f3",
	"f4",
	"f5",
	"f6",
	"f7",
	"f8",
	"f9",
	"f10",
	"f11",
	"f12",
	"f13",
	"f14",
	"f15",
	"f16",
	"f17",
	"f18",
	"f19",
	"f20",
	"f21",
	"f22",
	"f23",
	"f24",
	"f25",
	"uparrow",
	"downarrow",
	"leftarrow",
	"rightarrow",
	"lshift",
	"rshift",
	"lctrl",
	"rctrl",
	"lalt",
	"ralt",
	"tab",
	"enter",
	"backspace",
	"ins",
	"del",
	"pgup",
	"pgdn",
	"home",
	"end",
	"kp_0",
	"kp_1",
	"kp_2",
	"kp_3",
	"kp_4",
	"kp_5",
	"kp_6",
	"kp_7",
	"kp_8",
	"kp_9",
	"kp_slash",
	"kp_multiply",
	"kp_minus",
	"kp_plus",
	"kp_del",
	"kp_equal",
	"kp_enter",
	"kp_numlock",
	"capslock",
	"scrolllock",
	"printscreen",
	"pause",
	"lsuper",
	"rsuper",
	"'",
	",",
	".",
	"/",
	"0","1","2","3","4","5","6","7","8","9",
	"[",
	"\\",
	"]",
	"`",
	";",
	"=",
	"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z",

	// Extended ASCII
	//"�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�",
	//"�"," ","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�",
	//"�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�",
	//"�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�",
	//"�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�",
	//

	"mouse1",
	"mouse2",
	"mouse3",
	"mouse4",
	"mouse5",
	"mouse6",
	"mouse7",
	"mouse8",
	"mouse9",
	"scrlup",
	"scrldn"
};
#endif