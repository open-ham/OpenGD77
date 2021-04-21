/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _OPENGD77_MENUSYSTEM_H_
#define _OPENGD77_MENUSYSTEM_H_

#include "main.h"
#include "user_interface/uiGlobals.h"
#include "functions/sound.h"
#include "functions/settings.h"
#include "functions/voicePrompts.h"


#define MENU_MAX_DISPLAYED_ENTRIES 3

// Short press event
#define BUTTONCHECK_SHORTUP(e, sk) (((e)->keys.key == 0) && ((e)->buttons & sk ## _SHORT_UP))
// Long press event
#define BUTTONCHECK_LONGDOWN(e, sk) (((e)->keys.key == 0) && ((e)->buttons & sk ## _LONG_DOWN))
// Extra long press event
#define BUTTONCHECK_EXTRALONGDOWN(e, sk) (((e)->keys.key == 0) && ((e)->buttons & sk ## _EXTRA_LONG_DOWN))
// SK*/ORANGE button is down, regardless event status
#define BUTTONCHECK_DOWN(e, sk) (((e)->buttons & sk))

typedef enum
{
	NO_EVENT       = 0,
	KEY_EVENT      = (1 << 0),
	BUTTON_EVENT   = (1 << 1),
	FUNCTION_EVENT = (1 << 2),
	ROTARY_EVENT   = (1 << 3)
} uiEventInput_t;

typedef enum
{
	CPS2UI_COMMAND_CLEARBUF = 0,
	CPS2UI_COMMAND_PRINT,
	CPS2UI_COMMAND_RENDER_DISPLAY,
	CPS2UI_COMMAND_BACKLIGHT,
	CPS2UI_COMMAND_GREEN_LED,
	CPS2UI_COMMAND_RED_LED,
	CPS2UI_COMMAND_END
} uiCPSCommand_t;


typedef enum
{
	MENU_STATUS_SUCCESS     = 0,
	MENU_STATUS_ERROR       = (1 << 0),
	MENU_STATUS_LIST_TYPE   = (1 << 1),
	MENU_STATUS_INPUT_TYPE  = (1 << 2),
	MENU_STATUS_FORCE_FIRST = (1 << 3)
} menuStatus_t;

typedef struct
{
	uint32_t        		buttons;
	keyboardCode_t  		keys;
	uint32_t        		rotary;
	uint16_t        		function;
	uiEventInput_t  		events;
	bool            		hasEvent;
	uint32_t        		time;
} uiEvent_t;


typedef menuStatus_t (*menuFunctionPointer_t)(uiEvent_t *, bool); // Typedef for menu function pointers.  Functions are passed the key, the button and the event data. Event can be a Key or a button or both. Last arg is for when the function is only called to initialise and display its screen.
typedef struct
{
	const menuFunctionPointer_t	function;
	int							lastItemIndex;
} menuFunctionData_t;

typedef struct
{
	int 					currentMenuNumber;
	int 					stackPosition;
	int 					stack[16];
} menuControlDataStruct_t;

typedef struct
{
	const int 				stringOffset; // String offset in stringsTable_t
	const int 				menuNum;
} menuItemNewData_t;

typedef struct
{
	const int				numItems;
	const menuItemNewData_t *items;
} menuItemsList_t;


void menuDisplayTitle(const char *title);
void menuDisplayEntry(int loopOffset, int focusedItem,const char *entryText);

int menuGetMenuOffset(int maxMenuEntries, int loopOffset);

void uiChannelModeUpdateScreen(int txTimeSecs);
void uiChannelModeColdStart(void);
void uiVFOModeUpdateScreen(int txTimeSecs);
void uiVFOLoadContact(struct_codeplugContact_t *contact);
void uiVFOModeStopScanning(void);
bool uiVFOModeIsScanning(void);
bool uiVFOModeDualWatchIsScanning(void);
void uiChannelModeStopScanning(void);
bool uiChannelModeIsScanning(void);

void uiCPSUpdate(uiCPSCommand_t command, int x, int y, ucFont_t fontSize, ucTextAlign_t alignment, bool isInverted, char *szMsg);

void menuSystemInit(void);
void menuSystemLanguageHasChanged(void);
void displayLightTrigger(bool fromKeyEvent);
void displayLightOverrideTimeout(int timeout);
void menuSystemPushNewMenu(int menuNumber);

void menuSystemSetCurrentMenu(int menuNumber);
int menuSystemGetCurrentMenuNumber(void);
int menuSystemGetPreviousMenuNumber(void);
int menuSystemGetPreviouslyPushedMenuNumber(void);

int menuSystemGetRootMenuNumber(void);

void menuSystemPopPreviousMenu(void);
void menuSystemPopAllAndDisplayRootMenu(void);
void menuSystemPopAllAndDisplaySpecificRootMenu(int newRootMenu, bool resetKeyboard);

void menuSystemCallCurrentMenuTick(uiEvent_t *ev);
int menuGetKeypadKeyValue(uiEvent_t *ev, bool digitsOnly);
void menuUpdateCursor(int pos, bool moved, bool render);
void moveCursorLeftInString(char *str, int *pos, bool delete);
void moveCursorRightInString(char *str, int *pos, int max, bool insert);

void menuRadioInfosInit(void);
void menuRadioInfosPushBackVoltage(int32_t voltage);

void menuLockScreenPop(void);

void menuLastHeardUpdateScreen(bool showTitleOrHeader, bool displayDetails, bool isFirstRun);
void menuLastHeardInit(void); // Only used in uiTXScreen
void menuLastHeardHandleEvent(uiEvent_t *ev); // Only used in uiTXScreen

void menuPrivateCallClear(void);
void menuPrivateCallDismiss(void);

void menuHotspotRestoreSettings(void);

bool menuTxScreenDisplaysLastHeard(void);

void menuSystemMenuIncrement(int32_t *O, int32_t M);
void menuSystemMenuDecrement(int32_t *O, int32_t M);

// For QuickKeys
void menuDisplaySettingOption(const char *entryText, const char *valueText);


#if defined(PLATFORM_GD77S)
bool uiChannelModeTransmitDTMFContactForGD77S(void);
void uiChannelModeHeartBeatActivityForGD77S(uiEvent_t *ev);
#endif


//
//  ---- Menu internals ----
//

/*
 * ---------------------- IMPORTANT ----------------------------
 *
 * These enums must match the menuFunctions array in menuSystem.c
 *
 * ---------------------- IMPORTANT ----------------------------
 */
enum MENU_SCREENS
{
	MENU_EMPTY = -1,
	UI_SPLASH_SCREEN = 0,
	UI_POWER_OFF,
	UI_VFO_MODE,
	UI_CHANNEL_MODE,
	MENU_MAIN_MENU,
	MENU_CONTACTS_MENU,
	MENU_ZONE_LIST,
	MENU_RADIO_INFOS,
	MENU_FIRMWARE_INFO,
	MENU_NUMERICAL_ENTRY,
	UI_TX_SCREEN,
	MENU_RSSI_SCREEN,
	MENU_LAST_HEARD,
	MENU_OPTIONS,
	MENU_DISPLAY,
	MENU_SOUND,
	MENU_CREDITS,
	MENU_CHANNEL_DETAILS,
	UI_HOTSPOT_MODE,
	UI_CPS,
	UI_CHANNEL_QUICK_MENU,
	UI_VFO_QUICK_MENU,
	UI_LOCK_SCREEN,
	MENU_CONTACT_LIST,
	MENU_DTMF_CONTACT_LIST,
	MENU_CONTACT_QUICKLIST,
	MENU_CONTACT_LIST_SUBMENU,
	MENU_CONTACT_DETAILS,
	MENU_CONTACT_NEW,
	MENU_LANGUAGE,
	UI_PRIVATE_CALL,
	UI_MESSAGE_BOX,
	NUM_MENU_ENTRIES
};


// QuickKeys
#define QUICKKEY_CONTACT  0x0
#define QUICKKEY_MENU     0x1
#define QUICKKEY_TYPE(f)              ((f>>15) & 0x1)    // 1 bits
#define QUICKKEY_MENUID(f)            ((f>>10) & 0x1f)   // 5 bits
#define QUICKKEY_ENTRYID(f)           ((f>>5) & 0x1f)    // 5 bits
#define QUICKKEY_FUNCTIONID(f)        (f & 0x1f)         // 5 bits
#define QUICKKEY_LONGENTRYID(f)       (f & 0x3ff)        // 10 bits   (Zones)
#define QUICKKEY_MENUVALUE(m, e, f)   ((QUICKKEY_MENU << 15) | ((m & 0x1f) << 10) | ((e & 0x1f) << 5) | (f & 0x1f))
#define QUICKKEY_MENULONGVALUE(m, e)  ((QUICKKEY_MENU << 15) | ((m & 0x1f) << 10) | (e & 0x1ff))
#define QUICKKEY_CONTACTVALUE(c)      (c)

enum QUICK_FUNCTIONS {  FUNC_START_SCANNING = QUICKKEY_MENUVALUE(0, 0, 1),
						FUNC_RIGHT = 0x11,
						FUNC_LEFT = 0x12,
						FUNC_TOGGLE_TORCH = QUICKKEY_MENUVALUE(0, 0, 2),
						FUNC_REDRAW = QUICKKEY_MENUVALUE(0, 0, 3)
};

// This is used to store current position in menus. The system keeps track of its value, e.g entering in
// a submenu, it will be restored exiting that submenu. It's up to the menuFunction() to override its
// value when isFirstRun == true.

typedef struct
{
	int 					currentItemIndex;
	int 					startIndex;
	int 					endIndex;
	int 					lightTimer;
	menuItemNewData_t		*currentMenuList;
	menuControlDataStruct_t	controlData;
	char					menuOptionsSetQuickkey;
	int 					menuOptionsTimeout;
	bool 					newOptionSelected;
	const menuItemsList_t	*data[];
} menuDataGlobal_t;

extern menuDataGlobal_t 		menuDataGlobal;
extern const menuItemsList_t 	menuDataMainMenu;
extern const menuItemsList_t 	menuDataContact;

menuStatus_t uiVFOMode(uiEvent_t *event, bool isFirstRun);
menuStatus_t uiVFOModeQuickMenu(uiEvent_t *event, bool isFirstRun);
menuStatus_t uiChannelMode(uiEvent_t *event, bool isFirstRun);
menuStatus_t uiChannelModeQuickMenu(uiEvent_t *event, bool isFirstRun);
menuStatus_t uiCPS(uiEvent_t *event, bool isFirstRun);
menuStatus_t uiSplashScreen(uiEvent_t *event, bool isFirstRun);
menuStatus_t uiPowerOff(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuZoneList(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuDisplayMenuList(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuRadioInfos(uiEvent_t *event, bool isFirstRun);

menuStatus_t menuFirmwareInfoScreen(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuNumericalEntry(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuTxScreen(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuRSSIScreen(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuLastHeard(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuOptions(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuDisplayOptions(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuSoundOptions(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuCredits(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuChannelDetails(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuHotspotMode(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuLockScreen(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuContactList(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuContactListSubMenu(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuContactDetails(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuLanguage(uiEvent_t *event, bool isFirstRun);
menuStatus_t menuPrivateCall(uiEvent_t *event, bool isFirstRun);

menuStatus_t uiMessageBox(uiEvent_t *event, bool isFirstRun);


#endif
