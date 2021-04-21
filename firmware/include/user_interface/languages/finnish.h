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
 * Translators: OH1E
 *
 *
 * Rev: 13 
 */
#ifndef USER_INTERFACE_LANGUAGES_FINNISH_H_
#define USER_INTERFACE_LANGUAGES_FINNISH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t finnishLanguage =
{
.LANGUAGE_NAME 		= "Suomi",
.menu			= "Menu",
.credits		= "Kiitokset",
.zone			= "Zone",
.rssi			= "RSSI Signaali",
.battery		= "Akun Tila",
.contacts		= "Kontaktit",
.last_heard		= "Viimeksi kuultu",
.firmware_info		= "Laiteohjelmisto",
.options		= "Yleis  Asetukset",
.display_options	= "Näytön Asetukset",
.sound_options		= "Ääni   Asetukset", 	// MaxLen: 16
.channel_details	= "Kanava Asetukset",
.language		= "Kieli",
.new_contact		= "Uusi kontakti",
.new_channel		= "Uusi kanava", 	// MaxLen: 16, leave room for a space and four channel digits after
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.hotspot_mode		= "Hotspotti tila",
.contact_details	= "Kontakti Asetus",
.built			= "Koontikäännös",
.zones			= "Zonet",
.keypad			= "Näppäin", 		// MaxLen: 12 (with .ptt)
.ptt			= "PTT", 		// MaxLen: 12 (with .keypad)
.locked			= "Lukossa", 		// MaxLen: 15
.press_blue_plus_star	= "Paina sinistä ja *", // MaxLen: 19
.to_unlock		= "avataksesi näplukko",// MaxLen: 19
.unlocked		= "Näplukko avattu", 	// MaxLen: 15
.power_off		= "Virta pois...",
.error			= "VIRHE", 		// MaxLen: 8
.rx_only		= "Vain Rx",
.out_of_band		= "Bändial ulkopu", 	// MaxLen: 14
.timeout		= "AIKAKATK", 		// MaxLen: 8
.tg_entry		= "Aseta TG", 		// MaxLen: 15
.pc_entry		= "Aseta PC", 		// MaxLen: 15
.user_dmr_id		= "Käyttäjän DMR ID",
.contact 		= "Kontakti", 		// MaxLen: 15
.accept_call		= "Vastaa puheluun?",
.private_call		= "Priv. puhelu",
.squelch		= "K.Salpa", 		// MaxLen: 8
.quick_menu 		= "Pika Menu",
.filter			= "Suodata", 		// MaxLen: 7 (with ':' + settings: "None", "CC", "CC,TS", "CC,TS,TG")
.all_channels		= "Kaikki Kanavat",
.gotoChannel		= "Muistipaikk", 	// MaxLen: 11 (" 1024")
.scan			= "Skannaus",
.channelToVfo		= "Kanava --> VFO",
.vfoToChannel		= "VFO --> Kanava",
.vfoToNewChannel	= "VFO --> Uusi kan", 	// MaxLen: 16
.group			= "Ryhmä", 		// MaxLen: 16 (with .type)
.private		= "Privaatti", 		// MaxLen: 16 (with .type)
.all			= "Kaikki", 		// MaxLen: 16 (with .type)
.type			= "Tyyppi", 		// MaxLen: 16 (with .type)
.timeSlot		= "Aikaväli", 		// MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none			= "Tyhjä", 		// MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:")
.contact_saved		= "Kontakti tallen.",
.duplicate		= "kaksoiskappale",
.tg			= "TG", 		// MaxLen: 8
.pc			= "PC", 		// MaxLen: 8
.ts			= "TS", 		// MaxLen: 8
.mode			= "Mode", 		// MaxLen: 12
.colour_code		= "Väri Koodi", 	// MaxLen: 16 (with ':' * .n_a)
.n_a			= "Pois", 		// MaxLen: 16 (with ':' * .colour_code)
.bandwidth		= "Kaistanl", 		// MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq		= "Steppi", 		// MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot			= "TOT", 		// MaxLen: 16 (with ':' + .off or 15..3825)
.off			= "Ei", 		// MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip		= "Skippaa zone", 	// MaxLen: 16 (with ':' + .yes or .no) 
.all_skip		= "Skippaa kaik", 	// MaxLen: 16 (with ':' + .yes or .no)
.yes			= "Joo", 		// MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.no			= "Ei", 		// MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.rx_group		= "Rx Ryhmä", 		// MaxLen: 16 (with ':' and codeplug group name)
.on			= "On", 		// MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep		= "Aikakatk beep", 	// MaxLen: 16 (with ':' + .off or 5..20)
.UNUSED_1				= "",
.calibration		= "Kalibriointi", 	// MaxLen: 16 (with ':' + .on or .off)
.band_limits		= "Bändi Rajoitu", 	// MaxLen: 16 (with ':' + .on or .off)
.beep_volume		= "NäpÄäniVoim", 	// MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain		= "DMR MicGain", 	// MaxLen: 16 (with ':' + -33..12 + 'dB')
.fm_mic_gain		= "FM MicGain", 	// MaxLen: 16 (with ':' + 0..31)
.key_long		= "Näp pitkä",	 	// MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat		= "Näp toisto", 	// MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout	= "Suodin aika", 	// MaxLen: 16 (with ':' + 1..90 + 's')
.brightness		= "Kirkkaus", 		// MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off		= "Min kirkkaus", 	// MaxLen: 16 (with ':' + 0..100 + '%')
.contrast		= "Kontrasti", 		// MaxLen: 16 (with ':' + 12..30)
.colour_invert		= "Käänteinen",
.colour_normal		= "Normaali",
.backlight_timeout	= "TaustValoAika", 	// MaxLen: 16 (with ':' + .no to 30s)
.scan_delay		= "Skann. viive", 	// MaxLen: 16 (with ':' + 1..30 + 's')
.yes___in_uppercase			= "KYLLÄ", 		// MaxLen: 8 (choice above green/red buttons)
.no___in_uppercase			= "EI", 		// MaxLen: 8 (choice above green/red buttons)
.DISMISS		= "POISTU", 		// MaxLen: 8 (choice above green/red buttons)
.scan_mode		= "Skannaus", 		// MaxLen: 16 (with ':' + .hold or .pause)
.hold			= "Pysäyty", 		// MaxLen: 16 (with ':' + .scan_mode)
.pause			= "Pauseta", 		// MaxLen: 16 (with ':' + .scan_mode)
.empty_list		= "Tyhjä lista", 	// MaxLen: 16
.delete_contact_qm	= "Poista kontakti?", 	// MaxLen: 16
.contact_deleted	= "Kontakti Poistet", 	// MaxLen: 16
.contact_used		= "Kontakti käytöss", 	// MaxLen: 16
.in_rx_group		= "on RX ryhmässä", 	// MaxLen: 16
.select_tx		= "Valitse TX", 	// MaxLen: 16
.edit_contact		= "Muokkaa Kontakti", 	// MaxLen: 16
.delete_contact		= "Poista Kontakti", 	// MaxLen: 16
.group_call		= "Ryhmä Puhelu", 	// MaxLen: 16
.all_call		= "Puhelu kaikille", 	// MaxLen: 16
.tone_scan		= "Aliääni scan",	// MaxLen: 16
.low_battery		= "Akku Vähissä !",	// MaxLen: 16
.Auto			= "Automaatti",		// MaxLen 16 (with .mode + ':') 
.manual			= "Manuaali",		// MaxLen 16 (with .mode + ':') 
.ptt_toggle		= "PTT Lukko",		// MaxLen 16 (with ':' + .on or .off)
.private_call_handling	= "Käsittele PC",	// MaxLen 16 (with ':' + .on ot .off)
.stop			= "Stoppaa", 		// Maxlen 16 (with ':' + .scan_mode)
.one_line		= "1 rivi", 		// MaxLen 16 (with ':' + .contact)
.two_lines		= "2 riviä", 		// MaxLen 16 (with ':' + .contact)
.priority_order		= "Järjest", 		// MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep		= "DMR piippi", 	// MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start			= "Alku", 		// MaxLen 16 (with ':' + .dmr_beep)
.both			= "Molemm",		// MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold          = "VOX Herkk.",		// MaxLen 16 (with ':' + .off or 1..30)
.vox_tail               = "VOX Viive",		// MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt				= "Prompt",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent                                 = "Silent", // Maxlen 16 (with : + audio_prompt)
.normal                                 = "Normal", // Maxlen 16 (with : + audio_prompt)
.beep					= "Beep", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1					= "Voice", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias	= "TA Tx", // Maxlen 16 (with : + .on or .off)
.squelch_VHF			= "VHF K.Salpa",// Maxlen 16 (with : + XX%)
.squelch_220			= "220 K.Salpa",// Maxlen 16 (with : + XX%)
.squelch_UHF			= "VHF K.Salpa", // Maxlen 16 (with : + XX%)
.display_background_colour = "Väri" , // Maxlen 16 (with : + .colour_normal or .colour_invert)
.openGD77 				= "OpenGD77",// Do not translate
.openGD77S 				= "OpenGD77S",// Do not translate
.openDM1801 			= "OpenDM1801",// Do not translate
.openRD5R 				= "OpenRD5R",// Do not translate
.gitCommit				= "Git commit",
.voice_prompt_level_2	= "Voice L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3	= "Voice L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter				= "DMR Filter",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter			= "CC Filter", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter			= "TS Filter", // MaxLen: 12 (with ':' + settings: .on or .off)
.dtmf_contact_list	= "FM DTMF contacts", // Maxlen: 16
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
#endif /* USER_INTERFACE_LANGUAGES_FINNISH_H_ */
