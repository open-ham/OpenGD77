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
#ifndef USER_INTERFACE_LANGUAGES_SLOVENIAN_H_
#define USER_INTERFACE_LANGUAGES_SLOVENIAN_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t slovenianLanguage =
{
.LANGUAGE_NAME 			= "Slovenian", // MaxLen: 16
.menu					= "Meni", // MaxLen: 16
.credits				= "Zasluge", // MaxLen: 16
.zone					= "Cona", // MaxLen: 16
.rssi					= "RSSI", // MaxLen: 16
.battery				= "Baterija", // MaxLen: 16
.contacts				= "Kontakti", // MaxLen: 16
.last_heard				= "Zadnji slisan", // MaxLen: 16
.firmware_info			= "Firmware info", // MaxLen: 16
.options				= "Nastavitve", // MaxLen: 16
.display_options		= "Zaslonske nast.", // MaxLen: 16
.sound_options			= "Zvocne nast.", // MaxLen: 16
.channel_details		= "Podrob. kanala", // MaxLen: 16
.language				= "Jezik", // MaxLen: 16
.new_contact			= "Nov kontakt", // MaxLen: 16
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.contact_details		= "Podro. kontakta", // MaxLen: 16
.hotspot_mode			= "Hotspot", // MaxLen: 16
.built					= "Zgrajeno", // MaxLen: 16
.zones					= "Cone", // MaxLen: 16
.keypad				    = "Stevil.", // MaxLen: 12 (with .ptt)
.ptt					= "PTT", // MaxLen: 12 (with .keypad)
.locked					= "Zaklenjeno", // MaxLen: 15
.press_blue_plus_star	= "Pritisni modro + *", // MaxLen: 19
.to_unlock				= "za odklepanje", // MaxLen: 19
.unlocked				= "Odklenjeno", // MaxLen: 15
.power_off				= "Izkljuceno...", // MaxLen: 16
.error					= "NAPAKA", // MaxLen: 8
.rx_only				= "Samo Sprejem", // MaxLen: 14
.out_of_band			= "IZVEN PASU", // MaxLen: 14
.timeout				= "TIMEOUT", // MaxLen: 8
.tg_entry				= "TG vnos", // MaxLen: 15
.pc_entry				= "PC vnos", // MaxLen: 15
.user_dmr_id			= "DMR ID uporab.", // MaxLen: 15
.contact 				= "Kontakt", // MaxLen: 15
.accept_call			= "Vrni klic na", // MaxLen: 16
.private_call			= "Privatni klic", // MaxLen: 16
.squelch				= "Squelch", // MaxLen: 8
.quick_menu 			= "Hitri meni", // MaxLen: 16
.filter				    = "Filter", // MaxLen: 7 (with ':' + settings: .none, "CC", "CC,TS", "CC,TS,TG")
.all_channels			= "Vsi kanali", // MaxLen: 16
.gotoChannel			= "Pojdi na",  // MaxLen: 11 (" 1024")
.scan					= "Isci", // MaxLen: 16
.channelToVfo			= "Kanal --> VFO", // MaxLen: 16
.vfoToChannel			= "VFO --> Kanal", // MaxLen: 16
.vfoToNewChannel		= "VFO --> Nov Kan.", // MaxLen: 16
.group					= "Skupina", // MaxLen: 16 (with .type)
.private				= "Privat", // MaxLen: 16 (with .type)
.all					= "Vsi", // MaxLen: 16 (with .type)
.type					= "Tip", // MaxLen: 16 (with .type)
.timeSlot				= "Timeslot", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none					= "Brez", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:", .filter/.mode/.dmr_beep)
.contact_saved			= "Kontakt shranjen", // MaxLen: 16
.duplicate				= "Podvojeno", // MaxLen: 16
.tg					    = "TG",  // MaxLen: 8
.pc					    = "PC", // MaxLen: 8
.ts					    = "TS", // MaxLen: 8
.mode					= "Nacin",  // MaxLen: 12
.colour_code			= "Barvna Koda", // MaxLen: 16 (with ':' * .n_a)
.n_a					= "N/A",// MaxLen: 16 (with ':' * .colour_code)
.bandwidth				= "Sirina", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq				= "Korak", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot					= "TOT", // MaxLen: 16 (with ':' + .off or 15..3825)
.off					= "Izk", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip				= "Preskoci cono", // MaxLen: 16 (with ':' + .yes or .no) 
.all_skip				= "Preskoci vse", // MaxLen: 16 (with ':' + .yes or .no)
.yes					= "Da", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.no					    = "Ne", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.rx_group				= "Rx Sku", // MaxLen: 16 (with ':' and codeplug group name)
.on					    = "Vkl", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep			= "Timeout pisk", // MaxLen: 16 (with ':' + .off or 5..20)
.UNUSED_1				= "",
.calibration			= "Kalibracija", // MaxLen: 16 (with ':' + .on or .off)
.band_limits			= "Omejitve pasu", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume			= "Glas Piska", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain			= "DMR mic", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.fm_mic_gain			= "FM mic", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "Gumb dolg", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat				= "Gumb pono", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout		= "Cas filtra", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness				= "Svetlost", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off			= "Min svetlos", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast				= "Kontrast", // MaxLen: 16 (with ':' + 12..30)
.colour_invert			= "Invertirano", // MaxLen: 16
.colour_normal			= "Normalno", // MaxLen: 16
.backlight_timeout		= "Timeout", // MaxLen: 16 (with ':' + .no to 30s)
.scan_delay				= "Premor iskan", // MaxLen: 16 (with ':' + 1..30 + 's')
.yes___in_uppercase		= "DA", // MaxLen: 8 (choice above green/red buttons)
.no___in_uppercase		= "NE", // MaxLen: 8 (choice above green/red buttons)
.DISMISS				= "PREKLICI", // MaxLen: 8 (choice above green/red buttons)
.scan_mode				= "Nacin iskan", // MaxLen: 16 (with ':' + .hold, .pause or .stop)
.hold					= "Drzi", // MaxLen: 16 (with ':' + .scan_mode)
.pause					= "Pavza", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list				= "Prazen seznam", // MaxLen: 16
.delete_contact_qm		= "Izbrisi kontakt?", // MaxLen: 16
.contact_deleted		= "Kontakt izbrisan", // MaxLen: 16
.contact_used			= "Kontakt uporabl", // MaxLen: 16
.in_rx_group			= "v RX skupini", // MaxLen: 16
.select_tx				= "Izberi TX", // MaxLen: 16
.edit_contact			= "Popravi kontakt", // MaxLen: 16
.delete_contact			= "Izbrisi kontakt", // MaxLen: 16
.group_call				= "Skupinski klic", // MaxLen: 16
.all_call				= "Vsi klic", // MaxLen: 16
.tone_scan				= "Isci ton", // MaxLen: 16
.low_battery			= "BATERIJA PRAZNA!", // MaxLen: 16
.Auto					= "Avto", // MaxLen 16 (with .mode + ':') 
.manual					= "Rocno",  // MaxLen 16 (with .mode + ':')
.ptt_toggle				= "PTT drzi", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling	= "Dovoli PC", // MaxLen 16 (with ':' + .on or .off)
.stop					= "Stop", // Maxlen 16 (with ':' + .scan_mode/.dmr_beep)
.one_line				= "1 vrsta", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 vrsti", // MaxLen 16 (with ':' + .contact)
.new_channel			= "Nov kanal", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order			= "Zaporedje", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "DMR pisk", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Start", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Oba", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold          = "VOX meja", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail               = "VOX rep", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt			= "Poziv",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent                 = "Tiho", // Maxlen 16 (with : + audio_prompt)
.normal                 = "Normalno", // Maxlen 16 (with : + audio_prompt)
.beep					= "Pisk", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1	= "Glas", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias	= "TA Tx", // Maxlen 16 (with : + .on or .off)
.squelch_VHF			= "VHF Skvelc",// Maxlen 16 (with : + XX%)
.squelch_220			= "220 Skvelc",// Maxlen 16 (with : + XX%)
.squelch_UHF			= "UHF Skvelc", // Maxlen 16 (with : + XX%)
.display_background_colour = "Barva", // Maxlen 16 (with : + .colour_normal or .colour_invert)
.openGD77 				= "OpenGD77",// Do not translate
.openGD77S 				= "OpenGD77S",// Do not translate
.openDM1801 			= "OpenDM1801",// Do not translate
.openRD5R 				= "OpenRD5R",// Do not translate
.gitCommit				= "Git commit",
.voice_prompt_level_2	= "Glas L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3	= "Glas L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter				= "DMR filter",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter			= "CC filter", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter			= "TS filter", // MaxLen: 12 (with ':' + settings: .on or .off)
.dtmf_contact_list			= "FM DTMF contacts", // Maxlen: 16
.channel_power				= "Ch Power", //Displayed as "Ch Power:" + .from_master or "Ch Power:"+ power text e.g. "Power:500mW" . Max total length 16
.from_master				= "Master",// Displayed if per-channel power is not enabled  the .channel_power
.set_quickkey				= "Set Quickkey", // MaxLen: 16
.dual_watch				= "Dual Watch", // MaxLen: 16
.info					= "Info", // MaxLen: 16 (with ':' + .off or.ts or .pwr or .both)
.pwr					= "Pwr",
.user_power				= "User Power",
.temperature				= "Temperature", // MaxLen: 16 (with ':' + .celcius or .fahrenheit)
.celcius				= "°C",
.seconds				= "seconds",
.radio_info				= "Radio infos",
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
#endif /* USER_INTERFACE_LANGUAGES_SLOVENIAN_H_ */
