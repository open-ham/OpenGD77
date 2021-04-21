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
 * Translator: ON7LDS
 *
 *
 * Rev: 1
 */
#ifndef USER_INTERFACE_LANGUAGES_DUTCH_H_
#define USER_INTERFACE_LANGUAGES_DUTCH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t dutchLanguage =
{
.LANGUAGE_NAME 				= "Nederlands", // MaxLen: 16
.menu						= "Menu", // MaxLen: 16
.credits					= "Vermeldingen", // MaxLen: 16
.zone						= "Zone", // MaxLen: 16
.rssi						= "Veldsterkte", // MaxLen: 16
.battery					= "Batterij", // MaxLen: 16
.contacts					= "Contacten", // MaxLen: 16
.last_heard					= "Laatst Gehoord", // MaxLen: 16
.firmware_info				= "Firmware Info", // MaxLen: 16
.options					= "Instellingen", // MaxLen: 16
.display_options			= "Scherm Instellen", // MaxLen: 16
.sound_options				= "Geluid Instellen", // MaxLen: 16
.channel_details			= "Kanaal Details", // MaxLen: 16
.language					= "Taal", // MaxLen: 16
.new_contact				= "Nieuw Contact", // MaxLen: 16
.dmr_contacts				= "DMR contacten", // MaxLen: 16
.contact_details			= "Contact Details", // MaxLen: 16
.hotspot_mode				= "Hotspot   ", // MaxLen: 16
.built						= "Gemaakt", // MaxLen: 16
.zones						= "Zones", // MaxLen: 16
.keypad						= "Toetsenbord", // MaxLen: 12 (with .ptt)
.ptt						= "PTT", // MaxLen: 12 (with .keypad)
.locked						= "Vergrendeld", // MaxLen: 15
.press_blue_plus_star		= "Druk Blauw + *", // MaxLen: 19
.to_unlock					= "om te ontgrendelen", // MaxLen: 19
.unlocked					= "Ontgrendeld", // MaxLen: 15
.power_off					= "Uitschakelen ...", // MaxLen: 16
.error						= "FOUT", // MaxLen: 8
.rx_only					= "Alleen Rx", // MaxLen: 14
.out_of_band				= "BUITEN BAND", // MaxLen: 14
.timeout					= "TIME-OUT", // MaxLen: 8
.tg_entry					= "TG Invoeren", // MaxLen: 15
.pc_entry					= "PO Invoeren", // MaxLen: 15
.user_dmr_id				= "DMRID Gebruiker", // MaxLen: 15
.contact 					= "Contact", // MaxLen: 15
.accept_call				= "Oproep Aannemen?", // MaxLen: 16
.private_call				= "Private Oproep", // MaxLen: 16
.squelch					= "Squelch", // MaxLen: 8
.quick_menu 				= "Snelmenu", // MaxLen: 16
.filter						= "Filter", // MaxLen: 7 (with ':' + settings: .none, "CC", "CC,TS", "CC,TS,TG")
.all_channels				= "Alle Kanalen", // MaxLen: 16
.gotoChannel				= "Ga Naar",  // MaxLen: 11 (" 1024")
.scan						= "Scan", // MaxLen: 16
.channelToVfo				= "Kanaal --> VFO", // MaxLen: 16
.vfoToChannel				= "VFO --> Kanaal", // MaxLen: 16
.vfoToNewChannel			= "VFO-->Nieuw Kan.", // MaxLen: 16
.group						= "Groep", // MaxLen: 16 (with .type)
.private					= "Privaat", // MaxLen: 16 (with .type)
.all						= "Alle", // MaxLen: 16 (with .type)
.type						= "Type", // MaxLen: 16 (with .type)
.timeSlot					= "Tijdslot  ", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none						= "Geen", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:", .filter/.mode/.dmr_beep)
.contact_saved				= "Contact Bewaard", // MaxLen: 16
.duplicate					= "Dubbel", // MaxLen: 16
.tg							= "TG",  // MaxLen: 8
.pc							= "PO", // MaxLen: 8
.ts							= "TS", // MaxLen: 8
.mode						= "Modus   ",  // MaxLen: 12
.colour_code				= "Kleur Code", // MaxLen: 16 (with ':' * .n_a)
.n_a						= "nvt",// MaxLen: 16 (with ':' * .colour_code or .timeout_beep)
.bandwidth					= "Bandbreedte", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq					= "Stap  ", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot						= "TOT   ", // MaxLen: 16 (with ':' + .off or 15..3825)
.off						= "Uit", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip					= "Zone Oversl", // MaxLen: 16 (with ':' + .yes or .no)
.all_skip					= "Alles Overs", // MaxLen: 16 (with ':' + .yes or .no)
.yes						= "Ja", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.no							= "Neen", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.rx_group					= "Rx Grp", // MaxLen: 16 (with ':' and codeplug group name)
.on							= "Aan", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep				= "Timeout Piep", // MaxLen: 16 (with ':' + .off or 5..20)
.UNUSED_1				= "",
.calibration				= "Calibratie", // MaxLen: 16 (with ':' + .on or .off)
.band_limits				= "Band Lim. ", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume				= "Piepvolume", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain				= "DMR Mic   ", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.fm_mic_gain				= "FM Mic    ", // MaxLen: 16 (with ':' + 0..31)
.key_long					= "Toets Lang", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat					= "Toets Herh", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout			= "Filter Tijd", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness					= "Helderheid ", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off				= "Min Helderh", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast					= "Contrast   ", // MaxLen: 16 (with ':' + 12..30)
.colour_invert				= "Omkeren", // MaxLen: 16
.colour_normal				= "Normaal", // MaxLen: 16
.backlight_timeout			= "Timeout ", // MaxLen: 16 (with ':' + .no to 30s)
.scan_delay					= "Scan Vertr.", // MaxLen: 16 (with ':' + 1..30 + 's')
.yes___in_uppercase			= "JA", // MaxLen: 8 (choice above green/red buttons)
.no___in_uppercase			= "NEEN", // MaxLen: 8 (choice above green/red buttons)
.DISMISS					= "AFWIJZEN", // MaxLen: 8 (choice above green/red buttons)
.scan_mode					= "ScanModus", // MaxLen: 16 (with ':' + .hold, .pause or .stop)
.hold						= "Houden", // MaxLen: 16 (with ':' + .scan_mode)
.pause						= "Pauze", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list					= "Lijst Leegmaken", // MaxLen: 16
.delete_contact_qm			= "Wis Contact?", // MaxLen: 16
.contact_deleted			= "Contact Gewist", // MaxLen: 16
.contact_used				= "Contact Gebruikt", // MaxLen: 16
.in_rx_group				= "in de Rx groep", // MaxLen: 16
.select_tx					= "Zet als Tx", // MaxLen: 16
.edit_contact				= "Bewerk Contact", // MaxLen: 16
.delete_contact				= "Wis Contact", // MaxLen: 16
.group_call					= "Groepsoproep", // MaxLen: 16
.all_call					= "Iedereen Roepen", // MaxLen: 16
.tone_scan					= "Toon Scan", // MaxLen: 16
.low_battery				= "BATTERY LEEG!!!", // MaxLen: 16
.Auto						= "Auto", // MaxLen 16 (with .mode + ':')
.manual						= "Manueel",  // MaxLen 16 (with .mode + ':')
.ptt_toggle					= "PTT latch ", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling		= "Sta PO toe", // MaxLen 16 (with ':' + .on or .off)
.stop						= "Stop", // Maxlen 16 (with ':' + .scan_mode/.dmr_beep)
.one_line					= "1 lijn", // MaxLen 16 (with ':' + .contact)
.two_lines					= "2 lijnen", // MaxLen 16 (with ':' + .contact)
.new_channel				= "Nw Kanaal", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order				= "Volgord", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep					= "DMR Piep  ", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start						= "Start", // MaxLen 16 (with ':' + .dmr_beep)
.both						= "Beide", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold				= "VOX Drempel", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail 					= "VOX Duur   ", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt				= "Melding ",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent						= "Stil", // Maxlen 16 (with : + audio_prompt)
.normal						= "Normaal", // Maxlen 16 (with : + audio_prompt)
.beep						= "Piep", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1		= "Stem L1", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias		= "TA Zenden ", // Maxlen 16 (with : + .on or .off)
.squelch_VHF				= "VHF Squelch",// Maxlen 16 (with : + XX%)
.squelch_220				= "220 Squelch",// Maxlen 16 (with : + XX%)
.squelch_UHF				= "UHF Squelch", // Maxlen 16 (with : + XX%)
.display_background_colour	= "Scherm ", // Maxlen 16 (with : + .colour_normal or .colour_invert)
.openGD77 					= "OpenGD77",// Do not translate
.openGD77S 					= "OpenGD77S",// Do not translate
.openDM1801 				= "OpenDM1801",// Do not translate
.openRD5R 					= "OpenRD5R",// Do not translate
.gitCommit					= "Git commit",
.voice_prompt_level_2		= "Stem L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3		= "Stem L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter					= "DMR Filter",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter				= "CC Filter ", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter				= "TS Filter ", // MaxLen: 12 (with ':' + settings: .on or .off)
.dtmf_contact_list			= "FM DTMFcontacten", // Maxlen: 16
.channel_power				= "K.Vermogen", //Displayed as "Ch Power:" + .from_master or "Ch Power:"+ power text e.g. "Power:500mW" . Max total length 16
.from_master				= "Master",// Displayed if per-channel power is not enabled  the .channel_power
.set_quickkey				= "Zet Sneltoets", // MaxLen: 16
.dual_watch				= "Dual Watch", // MaxLen: 16
.info					= "Info", // MaxLen: 16 (with ':' + .off or.ts or .pwr or .both)
.pwr					= "Pwr",
.user_power				= "Gebr. Vermogen",
.temperature				= "Temperatuur", // MaxLen: 16 (with ':' + .celcius or .fahrenheit)
.celcius				= "°C",
.seconds				= "seconden",
.radio_info				= "Radio Info",
.temperature_calibration		= "Temp Kal",
.pin_code				= "Pincode",
.please_confirm				= "Bevestig aub", // MaxLen: 15
.vfo_freq_bind_mode			= "Freq. Bind",
.overwrite_qm				= "Overschrijven?", //Maxlen: 14 chars
.eco_level				= "Eco Niveau",
.buttons				= "Knoppen",
.leds					= "leds",
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
#endif /* USER_INTERFACE_LANGUAGES_DUTCH_H_ */
