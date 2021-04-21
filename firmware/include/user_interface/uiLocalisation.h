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
#ifndef _OPENGD77_UILOCALISATION_H_
#define _OPENGD77_UILOCALISATION_H_

#if defined(LANGUAGE_BUILD_JAPANESE)
#define NUM_LANGUAGES 2
#else
#define NUM_LANGUAGES 15
#endif

#define LANGUAGE_TEXTS_LENGTH 17

typedef struct
{
/*
 * IMPORTANT
 *
 * The first set of strings are used for menu names
 *
 * DO NOT RE-ORGANISE THIS LIST as the items are accessed using pointer arithmetic
 * from menuSystem. c mainMenuItems[]
 *
 */
   const char *LANGUAGE_NAME;// Menu number 0
   const char *menu;// Menu number  1
   const char *credits;// Menu number  2
   const char *zone;// Menu number 3
   const char *rssi;// Menu number  4
   const char *battery;// Menu number  5
   const char *contacts;// Menu number  6
   const char *last_heard;// Menu number  7
   const char *firmware_info;// Menu number  8
   const char *options;// Menu number  9
   const char *display_options;// Menu number  10
   const char *sound_options;// Menu number  11
   const char *channel_details;// Menu number  12
   const char *language;// Menu number  13
   const char *new_contact;// Menu number  14
   const char *dmr_contacts;// Menu number  15
   const char *contact_details;// Menu number 16
   const char *hotspot_mode;// Menu number 17

  /*
   * DO NOT RE-ORGANISE THIS LIST as the items are accessed using pointer arithmetic
   * for the voice prompts
   */

   const char *built;//
   const char *zones;//
   const char *keypad;//
   const char *ptt;//
   const char *locked;//
   const char *press_blue_plus_star;//
   const char *to_unlock;//
   const char *unlocked;//
   const char *power_off; //
   const char *error;//
   const char *rx_only;//
   const char *out_of_band;//
   const char *timeout;//
   const char *tg_entry;//
   const char *pc_entry;//
   const char *user_dmr_id;//
   const char *contact;//
   const char *accept_call;//  "Accept call?"
   const char *private_call;// "Private call"
   const char *squelch;// "Squelch"
   const char *quick_menu;//"Quick Menu"
   const char *filter;//"Filter:%s"
   const char *all_channels;//"All Channels"
   const char *gotoChannel;//"Goto %d"
   const char *scan;// "Scan"
   const char *channelToVfo;// "Channel --> VFO",
   const char *vfoToChannel;// "VFO --> Channel",
   const char *vfoToNewChannel;// "VFO --> New Chan",
   const char *group;//"Group",
   const char *private;//"Private",
   const char *all;//"All",
   const char *type;//"Type:"
   const char *timeSlot;//"Timeslot"
   const char *none;//"none"
   const char *contact_saved;// "Contact saved",
   const char *duplicate;//"Duplicate"
   const char *tg;//"TG"
   const char *pc;//"PC"
   const char *ts;//"TS"
   const char *mode;//"Mode"
   const char *colour_code;//"Color Code"
   const char *n_a;//"N/A"
   const char *bandwidth;
   const char *stepFreq;
   const char *tot;
   const char *off;
   const char *zone_skip;
   const char *all_skip;
   const char *yes;
   const char *no;
   const char *rx_group;
   const char *on;
   const char *timeout_beep;
   const char *UNUSED_1;
   const char *calibration;
   const char *band_limits;
   const char *beep_volume;
   const char *dmr_mic_gain;
   const char *fm_mic_gain;
   const char *key_long;
   const char *key_repeat;
   const char *dmr_filter_timeout;
   const char *brightness;
   const char *brightness_off;
   const char *contrast;
   const char *colour_invert;// for display background
   const char *colour_normal;// for display background
   const char *backlight_timeout;
   const char *scan_delay;
   const char *yes___in_uppercase;
   const char *no___in_uppercase;
   const char *DISMISS;
   const char *scan_mode;
   const char *hold;
   const char *pause;
   const char *empty_list;
   const char *delete_contact_qm;
   const char *contact_deleted;
   const char *contact_used;
   const char *in_rx_group;
   const char *select_tx;
   const char *edit_contact;
   const char *delete_contact;
   const char *group_call;
   const char *all_call;
   const char *tone_scan;
   const char *low_battery;
   const char *Auto;
   const char *manual;
   const char *ptt_toggle;
   const char *private_call_handling;
   const char *stop;
   const char *one_line;
   const char *two_lines;
   const char *new_channel;
   const char *priority_order;
   const char *dmr_beep;
   const char *start;
   const char *both;
   const char *vox_threshold;
   const char *vox_tail;
   const char *audio_prompt;
   const char *silent;
   const char *normal;
   const char *beep;
   const char *voice_prompt_level_1;
   const char *transmitTalkerAlias;
   const char *squelch_VHF;
   const char *squelch_220;
   const char *squelch_UHF;
   const char *display_background_colour;
   const char *openGD77;
   const char *openGD77S;
   const char *openDM1801;
   const char *openRD5R;
   const char *gitCommit;
   const char *voice_prompt_level_2;
   const char *voice_prompt_level_3;
   const char *dmr_filter;
   const char *dmr_cc_filter;
   const char *dmr_ts_filter;
   const char *dtmf_contact_list;// Menu number 18
   const char *channel_power;// "Ch Power" for the Channel details screen
   const char *from_master;// "Master" for the power or squelch setting on the Channel details screen
   const char *set_quickkey;
   const char *dual_watch;
   const char *info;
   const char *pwr;
   const char *user_power;
   const char *temperature;
   const char *celcius;
   const char *seconds;
   const char *radio_info;
   const char *temperature_calibration;
   const char *pin_code;
   const char *please_confirm;
   const char *vfo_freq_bind_mode;
   const char *overwrite_qm;
   const char *eco_level;// Economy / Power saving level
   const char *buttons;
   const char *leds;
   const char *scan_dwell_time;
} stringsTable_t;

extern const stringsTable_t languages[];
extern const stringsTable_t *currentLanguage;

enum languageNamesOrder  { 	englishLanguageName = 0,
#if defined(LANGUAGE_BUILD_JAPANESE)
							japaneseLanguageName,
#else
							catalanLanguageName,
							danishLanguageName,
							frenchLanguageName,
							deutschGermanLanguageName,
							italianLanguageName,
							portuguesLanguageName,
							spanishLanguageName,
							suomiFinnishLanguageName,
							polishLanguageName,
							turkishLanguageName,
							czechLanguageName,
							nederlandsDutchLanguageName,
							slovenianLanguageName,
							portuguesBrazilLanguageName
#endif
};

extern const int LANGUAGE_DISPLAY_ORDER[];

#endif
