/* -*- coding: windows-1252-unix; -*- */
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
/*
 * Translators:
 *
 *
 * Rev:
 */
#ifndef USER_INTERFACE_LANGUAGES_ENGLISH_H_
#define USER_INTERFACE_LANGUAGES_ENGLISH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t englishLanguage =
{
.LANGUAGE_NAME 				= "English", // MaxLen: 16
.menu					= "Menu", // MaxLen: 16
.credits				= "Credits", // MaxLen: 16
.zone					= "Zone", // MaxLen: 16
.rssi					= "RSSI", // MaxLen: 16
.battery				= "Battery", // MaxLen: 16
.contacts				= "Contacts", // MaxLen: 16
.last_heard				= "Last heard", // MaxLen: 16
.firmware_info				= "Firmware info", // MaxLen: 16
.options				= "Options", // MaxLen: 16
.display_options			= "Display options", // MaxLen: 16
.sound_options				= "Sound options", // MaxLen: 16
.channel_details			= "Channel details", // MaxLen: 16
.language				= "Language", // MaxLen: 16
.new_contact				= "New contact", // MaxLen: 16
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.contact_details			= "Contact Details", // MaxLen: 16
.hotspot_mode				= "Hotspot", // MaxLen: 16
.built					= "Built", // MaxLen: 16
.zones					= "Zones", // MaxLen: 16
.keypad					= "Keypad", // MaxLen: 12 (with .ptt)
.ptt					= "PTT", // MaxLen: 12 (with .keypad)
.locked					= "Locked", // MaxLen: 15
.press_blue_plus_star			= "Press Blue + *", // MaxLen: 19
.to_unlock				= "to unlock", // MaxLen: 19
.unlocked				= "Unlocked", // MaxLen: 15
.power_off				= "Power Off...", // MaxLen: 16
.error					= "ERROR", // MaxLen: 8
.rx_only				= "Rx Only", // MaxLen: 14
.out_of_band				= "OUT OF BAND", // MaxLen: 14
.timeout				= "TIMEOUT", // MaxLen: 8
.tg_entry				= "TG entry", // MaxLen: 15
.pc_entry				= "PC entry", // MaxLen: 15
.user_dmr_id				= "User DMR ID", // MaxLen: 15
.contact 				= "Contact", // MaxLen: 15
.accept_call				= "Return call to", // MaxLen: 16
.private_call				= "Private Call", // MaxLen: 16
.squelch				= "Squelch", // MaxLen: 8
.quick_menu 				= "Quick Menu", // MaxLen: 16
.filter					= "Filter", // MaxLen: 7 (with ':' + settings: .none, "CC", "CC,TS", "CC,TS,TG")
.all_channels				= "All Channels", // MaxLen: 16
.gotoChannel				= "Goto",  // MaxLen: 11 (" 1024")
.scan					= "Scan", // MaxLen: 16
.channelToVfo				= "Channel --> VFO", // MaxLen: 16
.vfoToChannel				= "VFO --> Channel", // MaxLen: 16
.vfoToNewChannel			= "VFO --> New Chan", // MaxLen: 16
.group					= "Group", // MaxLen: 16 (with .type)
.private				= "Private", // MaxLen: 16 (with .type)
.all					= "All", // MaxLen: 16 (with .type)
.type					= "Type", // MaxLen: 16 (with .type)
.timeSlot				= "Timeslot", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none					= "None", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:", .filter/.mode/.dmr_beep)
.contact_saved				= "Contact saved", // MaxLen: 16
.duplicate				= "Duplicate", // MaxLen: 16
.tg					= "TG",  // MaxLen: 8
.pc					= "PC", // MaxLen: 8
.ts					= "TS", // MaxLen: 8
.mode					= "Mode",  // MaxLen: 12
.colour_code				= "Color Code", // MaxLen: 16 (with ':' * .n_a)
.n_a					= "N/A",// MaxLen: 16 (with ':' * .colour_code)
.bandwidth				= "Bandwidth", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq				= "Step", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot					= "TOT", // MaxLen: 16 (with ':' + .off or 15..3825)
.off					= "Off", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip				= "Zone Skip", // MaxLen: 16 (with ':' + .yes or .no) 
.all_skip				= "All Skip", // MaxLen: 16 (with ':' + .yes or .no)
.yes					= "Yes", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.no					= "No", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.rx_group				= "Rx Grp", // MaxLen: 16 (with ':' and codeplug group name)
.on					= "On", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep				= "Timeout beep", // MaxLen: 16 (with ':' + .off or 5..20)
.UNUSED_1				= "",
.calibration				= "Calibration", // MaxLen: 16 (with ':' + .on or .off)
.band_limits				= "Band Limits", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume				= "Beep vol", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain				= "DMR mic", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.fm_mic_gain				= "FM mic", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "Key long", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat				= "Key rpt", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout			= "Filter time", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness				= "Brightness", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off				= "Min bright", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast				= "Contrast", // MaxLen: 16 (with ':' + 12..30)
.colour_invert				= "Invert", // MaxLen: 16
.colour_normal				= "Normal", // MaxLen: 16
.backlight_timeout			= "Timeout", // MaxLen: 16 (with ':' + .no to 30s)
.scan_delay				= "Scan delay", // MaxLen: 16 (with ':' + 1..30 + 's')
.yes___in_uppercase			= "YES", // MaxLen: 8 (choice above green/red buttons)
.no___in_uppercase			= "NO", // MaxLen: 8 (choice above green/red buttons)
.DISMISS				= "DISMISS", // MaxLen: 8 (choice above green/red buttons)
.scan_mode				= "Scan mode", // MaxLen: 16 (with ':' + .hold, .pause or .stop)
.hold					= "Hold", // MaxLen: 16 (with ':' + .scan_mode)
.pause					= "Pause", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list				= "Empty List", // MaxLen: 16
.delete_contact_qm			= "Delete contact?", // MaxLen: 16
.contact_deleted			= "Contact deleted", // MaxLen: 16
.contact_used				= "Contact used", // MaxLen: 16
.in_rx_group				= "in RX group", // MaxLen: 16
.select_tx				= "Select TX", // MaxLen: 16
.edit_contact				= "Edit Contact", // MaxLen: 16
.delete_contact				= "Delete Contact", // MaxLen: 16
.group_call				= "Group Call", // MaxLen: 16
.all_call				= "All Call", // MaxLen: 16
.tone_scan				= "Tone scan", // MaxLen: 16
.low_battery				= "LOW BATTERY !!!", // MaxLen: 16
.Auto					= "Auto", // MaxLen 16 (with .mode + ':') 
.manual					= "Manual",  // MaxLen 16 (with .mode + ':')
.ptt_toggle				= "PTT latch", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling			= "Allow PC", // MaxLen 16 (with ':' + .on or .off)
.stop					= "Stop", // Maxlen 16 (with ':' + .scan_mode/.dmr_beep)
.one_line				= "1 line", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 lines", // MaxLen 16 (with ':' + .contact)
.new_channel				= "New channel", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order				= "Order", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "DMR beep", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Start", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Both", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold                          = "VOX Thres.", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail                               = "VOX Tail", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt				= "Prompt",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent                                 = "Silent", // Maxlen 16 (with : + audio_prompt)
.normal                                 = "Normal", // Maxlen 16 (with : + audio_prompt)
.beep					= "Beep", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1			= "Voice", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias			= "TA Tx", // Maxlen 16 (with : + .on or .off)
.squelch_VHF				= "VHF Squelch",// Maxlen 16 (with : + XX%)
.squelch_220				= "220 Squelch",// Maxlen 16 (with : + XX%)
.squelch_UHF				= "UHF Squelch", // Maxlen 16 (with : + XX%)
.display_background_colour 		= "Colour", // Maxlen 16 (with : + .colour_normal or .colour_invert)
.openGD77 				= "OpenGD77",// Do not translate
.openGD77S 				= "OpenGD77S",// Do not translate
.openDM1801 				= "OpenDM1801",// Do not translate
.openRD5R 				= "OpenRD5R",// Do not translate
.gitCommit				= "Git commit",
.voice_prompt_level_2			= "Voice L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3			= "Voice L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter				= "DMR Filter",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter				= "CC Filter", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter				= "TS Filter", // MaxLen: 12 (with ':' + settings: .on or .off)
.dtmf_contact_list			= "FM DTMF contacts", // Maxlen: 16
.channel_power				= "Ch Power", //Displayed as "Ch Power:" + .from_master or "Ch Power:"+ power text e.g. "Power:500mW" . Max total length 16
.from_master				= "Master",// Displayed if per-channel power is not enabled  the .channel_power
.set_quickkey				= "Set Quickkey", // MaxLen: 16
.dual_watch				= "Dual Watch", // MaxLen: 16
.info					= "Info", // MaxLen: 16 (with ':' + .off or .ts or .pwr or .both)
.pwr					= "Pwr",
.user_power				= "User Power",
.temperature				= "Temperature", // MaxLen: 16 (with ':' + .celcius or .fahrenheit)
.celcius				= "°C",
.seconds				= "seconds",
.radio_info				= "Radio info",
.temperature_calibration		= "Temp Cal",
.pin_code				= "Pin Code",
.please_confirm				= "Please confirm", // MaxLen: 15
.vfo_freq_bind_mode			= "Freq. Bind",
.overwrite_qm				= "Overwrite ?", //Maxlen: 14 chars
.eco_level				= "Eco Level",
.buttons				= "Buttons",
.leds					= "LEDs",
.scan_dwell_time		= "Scan dwell"
};
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
#endif /* USER_INTERFACE_LANGUAGES_ENGLISH_H_ */
