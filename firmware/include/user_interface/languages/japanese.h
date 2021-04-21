/* -*- coding: binary; -*- */
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
 * Translators: JE4SMQ
 *
 *
 * Rev:
 */
#ifndef USER_INTERFACE_LANGUAGES_JAPANESE_H_
#define USER_INTERFACE_LANGUAGES_JAPANESE_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t japaneseLanguage =
{
.LANGUAGE_NAME 				= "ÆÎÝºÞ", // MaxLen: 16
.menu					= "ÒÆ­-", // MaxLen: 16
.credits				= "¶²ÊÂ¼¬", // MaxLen: 16
.zone					= "¿Þ-Ý", // MaxLen: 16
.rssi					= "RSSI", // MaxLen: 16
.battery				= "ÃÞÝÁ", // MaxLen: 16
.contacts				= "ºÝÀ¸Ä", // MaxLen: 16
.last_heard				= "¼Þ­¼ÝÛ¸Þ", // MaxLen: 16
.firmware_info				= "Ì§-Ñ³ª±¼Þ®³Î³", // MaxLen: 16
.options				= "µÌß¼®Ý", // MaxLen: 16
.display_options			= "Ë®³¼Þ µÌß¼®Ý", // MaxLen: 16
.sound_options				= "µÝ¾²  µÌß¼®Ý", // MaxLen: 16
.channel_details			= "Á¬ÝÈÙ Å²Ö³", // MaxLen: 16
.language				= "Language", // MaxLen: 16
.new_contact				= "New ºÝÀ¸Ä", // MaxLen: 16
.dmr_contacts				= "DMR ºÝÀ¸Ä", // MaxLen: 16
.contact_details			= "ºÝÀ¸ÄÅ²Ö³", // MaxLen: 16
.hotspot_mode				= "Î¯Ä½Îß¯Ä", // MaxLen: 16
.built					= "Built", // MaxLen: 16
.zones					= "¿Þ-Ý", // MaxLen: 16
.keypad					= "·-Êß¯ÄÞ", // MaxLen: 12 (with .ptt)
.ptt					= "PTT", // MaxLen: 12 (with .keypad)
.locked					= "Û¯¸", // MaxLen: 15
.press_blue_plus_star			= "F(±µ) + *", // MaxLen: 19
.to_unlock				= "Û¯¸¶²¼Þ®", // MaxLen: 19
.unlocked				= "Û¯¸¶²¼Þ®½ÞÐ", // MaxLen: 15
.power_off				= "ÃÞÝ¹ÞÝ Off...", // MaxLen: 16
.error					= "´×-", // MaxLen: 8
.rx_only				= "¿³¼Ý·Ý¼", // MaxLen: 14
.out_of_band				= "µÌÊÞÝÄÞ", // MaxLen: 14
.timeout				= "À²Ñ±³Ä", // MaxLen: 8
.tg_entry				= "TG Æ­³Ø®¸", // MaxLen: 15
.pc_entry				= "PC Æ­³Ø®¸", // MaxLen: 15
.user_dmr_id				= "Õ-»Þ- DMR ID", // MaxLen: 15
.contact 				= "ºÝÀ¸Ä", // MaxLen: 15
.accept_call				= "Return call to", // MaxLen: 16
.private_call				= "Ìß×²ÍÞ-Äº-Ù", // MaxLen: 16
.squelch				= "½¹ÙÁ", // MaxLen: 8
.quick_menu 				= "¸²¯¸ÒÆ­-", // MaxLen: 16
.filter					= "Ì¨ÙÀ-", // MaxLen: 7 (with ':' + settings: .none, "CC", "CC,TS", "CC,TS,TG")
.all_channels				= "¾ÞÝÁ¬ÝÈÙ", // MaxLen: 16
.gotoChannel				= "Á¬ÝÈÙ²ÄÞ³",  // MaxLen: 11 (" 1024")
.scan					= "½·¬Ý", // MaxLen: 16
.channelToVfo				= "Á¬ÝÈÙ --> VFO", // MaxLen: 16
.vfoToChannel				= "VFO --> Á¬ÝÈÙ", // MaxLen: 16
.vfoToNewChannel			= "VFO --> NewÁ¬ÝÈÙ", // MaxLen: 16
.group					= "¸ÞÙ-Ìß", // MaxLen: 16 (with .type)
.private				= "Ìß×²ÍÞ-Ä", // MaxLen: 16 (with .type)
.all					= "µ-Ù", // MaxLen: 16 (with .type)
.type					= "À²Ìß", // MaxLen: 16 (with .type)
.timeSlot				= "À²Ñ½Û¯Ä", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none					= "Å¼", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:", .filter/.mode/.dmr_beep)
.contact_saved				= "ºÝÀ¸Ä Î¿ÞÝ½Ð", // MaxLen: 16
.duplicate				= "¼Þ­³Ì¸", // MaxLen: 16
.tg					= "TG",  // MaxLen: 8
.pc					= "PC", // MaxLen: 8
.ts					= "TS", // MaxLen: 8
.mode					= "Ó-ÄÞ",  // MaxLen: 12
.colour_code				= "¶×-º-ÄÞ", // MaxLen: 16 (with ':' * .n_a)
.n_a					= "N/A",// MaxLen: 16 (with ':' * .colour_code)
.bandwidth				= "ÊÞÝÄÞÊÊÞ", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq				= "½Ã¯Ìß", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot					= "TOT", // MaxLen: 16 (with ':' + .off or 15..3825)
.off					= "µÌ", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip				= "¿Þ-Ý ½·¯Ìß", // MaxLen: 16 (with ':' + .yes or .no) 
.all_skip				= "µ-Ù ½·¯Ìß", // MaxLen: 16 (with ':' + .yes or .no)
.yes					= "Ê²", // MaxLen: 16 (with ':' + .zone_skip, .all_skip or .factory_reset)
.no					= "²²´", // MaxLen: 16 (with ':' + .zone_skip, .all_skip or .factory_reset)
.rx_group				= "RX Grp", // MaxLen: 16 (with ':' and codeplug group name)
.on					= "µÝ", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep				= "À²Ñ±³ÄËÞ-Ìß", // MaxLen: 16 (with ':' + .off or 5..20)
.UNUSED_1				= "",
.calibration				= "·¬ØÌÞÚ-¼®Ý", // MaxLen: 16 (with ':' + .on or .off)
.band_limits				= "ÊÞÝÄÞ¾²¹ÞÝ", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume				= "ËÞ-ÌßµÝØ®³", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain				= "DMR Ï²¸¹Þ²Ý", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.fm_mic_gain				= "FM Ï²¸¹Þ²Ý", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "·-ÛÝ¸Þ", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat				= "·-ØËß-Ä", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout			= "Ì¨ÙÀ-TO", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness				= "±¶Ù»", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off				= "ÃÞ¨Ï-", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast				= "ºÝÄ×½Ä", // MaxLen: 16 (with ':' + 12..30)
.colour_invert				= "ÊÝÃÝ", // MaxLen: 16
.colour_normal				= "Â³¼Þ®³", // MaxLen: 16
.backlight_timeout			= "À²Ñ±³Ä", // MaxLen: 16 (with ':' + .no to 30s)
.scan_delay				= "½·¬ÝÃÞ¨Ú²", // MaxLen: 16 (with ':' + 1..30 + 's')
.yes___in_uppercase			= "Ê²", // MaxLen: 8 (choice above green/red buttons)
.no___in_uppercase			= "²²´", // MaxLen: 8 (choice above green/red buttons)
.DISMISS				= "¼¯Êß²", // MaxLen: 8 (choice above green/red buttons)
.scan_mode				= "½·¬ÝÓ-ÄÞ", // MaxLen: 16 (with ':' + .hold, .pause or .stop)
.hold					= "Î-ÙÄÞ", // MaxLen: 16 (with ':' + .scan_mode)
.pause					= "Îß-½Þ", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list				= "Ø½ÄÅ¼", // MaxLen: 16
.delete_contact_qm			= "ºÝÀ¸ÄÉ»¸¼Þ®?", // MaxLen: 16
.contact_deleted			= "ºÝÀ¸Ä»¸¼Þ®½Ð", // MaxLen: 16
.contact_used				= "ºÝÀ¸ÄÊ½ÃÞÆ±Ù", // MaxLen: 16
.in_rx_group				= "RX¸ÞÙ-ÌßÅ²", // MaxLen: 16
.select_tx				= "¿³¼Ý¾ÝÀ¸", // MaxLen: 16
.edit_contact				= "ºÝÀ¸Ä ¼­³¾²", // MaxLen: 16
.delete_contact				= "ºÝÀ¸Ä »¸¼Þ®", // MaxLen: 16
.group_call				= "¸ÞÙ-Ìßº-Ù", // MaxLen: 16
.all_call				= "µ-Ùº-Ù", // MaxLen: 16
.tone_scan				= "Ä-Ý½·¬Ý", // MaxLen: 16
.low_battery				= "ÃÞÝÁ-·ÞÚ !!!", // MaxLen: 16
.Auto					= "µ-Ä", // MaxLen 16 (with .mode + ':') 
.manual					= "ÏÆ­±Ù",  // MaxLen 16 (with .mode + ':')
.ptt_toggle				= "PTT Ä¸ÞÙ", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling			= "PC³¹Â¹", // MaxLen 16 (with ':' + .on or .off)
.stop					= "½Ä¯Ìß", // Maxlen 16 (with ':' + .scan_mode/.dmr_beep)
.one_line				= "1 ·Þ®³", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 ·Þ®³", // MaxLen 16 (with ':' + .contact)
.new_channel				= "New Á¬ÝÈÙ", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order				= "DB¼Þ­Ý", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "DMR ËÞ-Ìß", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "½À-Ä", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Ø®³Î³", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold                          = "VOX ½Ú¯¼®ÙÄÞ", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail                               = "VOX Ã-Ù", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt				= "µÝ¾²±ÝÅ²",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent                                 = "Å¼", // Maxlen 16 (with : + audio_prompt)
.normal                                 = "Â³¼Þ®³", // Maxlen 16 (with : + audio_prompt)
.beep					= "ËÞ-Ìß", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1			= "µÝ¾² L1", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias			= "TA¿³¼Ý", // Maxlen 16 (with : + .on or .off)
.squelch_VHF				= "VHF ½¹ÙÁ",// Maxlen 16 (with : + XX%)
.squelch_220				= "220 ½¹ÙÁ",// Maxlen 16 (with : + XX%)
.squelch_UHF				= "UHF ½¹ÙÁ", // Maxlen 16 (with : + XX%)
.display_background_colour 		= "ÊÞ¯¸¶×-", // Maxlen 16 (with : + .colour_normal or .colour_invert)
.openGD77 				= "OpenGD77",// Do not translate
.openGD77S 				= "OpenGD77S",// Do not translate
.openDM1801 				= "OpenDM1801",// Do not translate
.openRD5R 				= "OpenRD5R",// Do not translate
.gitCommit				= "Git commit",
.voice_prompt_level_2			= "µÝ¾² L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3			= "µÝ¾² L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter				= "DMR Ì¨ÙÀ-",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter				= "CC Ì¨ÙÀ-", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter				= "TS Ì¨ÙÀ-", // MaxLen: 12 (with ':' + settings: .on or .off)
.dtmf_contact_list			= "DTMF ºÝÀ¸ÄØ½Ä", // Maxlen: 16
.channel_power				= "Ch Pwr", //Displayed as "Ch Power:" + .from_master or "Ch Power:"+ power text e.g. "Power:500mW" . Max total length 16
.from_master				= "Ï½À-",// Displayed if per-channel power is not enabled  the .channel_power
.set_quickkey				= "¸²¯¸·- ¾¯Ä", // MaxLen: 16
.dual_watch				= "ÃÞ­±ÙÜ¯Á", // MaxLen: 16
.info					= "¼Þ®³Î³", // MaxLen: 16 (with ':' + .off or .ts or .pwr or .both)
.pwr					= "Pwr",
.user_power				= "Õ-»Þ-Pwr",
.temperature				= "µÝÄÞ", // MaxLen: 16 (with ':' + .celcius or .fahrenheit)
.celcius				= "°C",
.seconds				= "ËÞ®³",
.radio_info				= "Ñ¾Ý· ¼Þ®³Î³",
.temperature_calibration		= "µÝÄÞCal",
.pin_code				= "±Ý¼®³ÊÞÝºÞ³",
.please_confirm				= "¶¸ÆÝ¼Ã¸ÀÞ»²", // MaxLen: 15
.vfo_freq_bind_mode			= "TRFÚÝÄÞ³",
.overwrite_qm				= "¶·¶´OK?", //Maxlen: 14 chars
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
#endif /* USER_INTERFACE_LANGUAGES_JAPANESE_H_ */
