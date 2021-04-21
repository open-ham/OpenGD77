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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Translators: EA3IGM, EA5SW, EB3AM, EA3BIL
 *
 *
 * Rev: 6
 */
#ifndef USER_INTERFACE_LANGUAGES_CATALAN_H_
#define USER_INTERFACE_LANGUAGES_CATALAN_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t catalanLanguage=
{
.LANGUAGE_NAME			= "Català",
.menu					= "Menú",
.credits				= "Crèdits",
.zone					= "Zona",
.rssi					= "Senyal",
.battery				= "Batería",
.contacts				= "Contactes",
.last_heard				= "Escoltats",
.firmware_info			= "Info firmware",
.options				= "Opcions",
.display_options		= "Opcions pantalla",
.sound_options			= "Opcions de só", // MaxLen: 16
.channel_details		= "Detalls Canal",
.language				= "Idioma",
.new_contact			= "Nou contacte",
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.hotspot_mode			= "Mode Hotspot",
.contact_details		= "Detall Ctte",
.built					= "Compilat",
.zones					= "Zones",
.keypad					= "Teclat",
.ptt					= "PTT",
.locked					= "Blocat",
.press_blue_plus_star	= "Prem Blau + (*)",
.to_unlock				= "per desblocar",
.unlocked				= "Desblocat",
.power_off				= "Apagant...",
.error					= "ERROR",
.rx_only				= "Només RX",
.out_of_band			= "FORA DE BANDA",
.timeout				= "TIMEOUT",
.tg_entry				= "Entreu TG",
.pc_entry				= "Entreu ID",
.user_dmr_id			= "ID DMR d'Usuari",
.contact				= "Contacte",
.accept_call			= "Acceptar trucada?",
.private_call			= "Trucada privada",
.squelch				= "Squelch",
.quick_menu				= "Menú Ràpid",
.filter					= "Filtre",
.all_channels			= "Llista total",
.gotoChannel			= "Anar al canal",
.scan					= "Scan",
.channelToVfo			= "Canal -> VFO",
.vfoToChannel			= "VFO -> Canal",
.vfoToNewChannel		= "VFO -> Nou Canal", // MaxLen: 16
.group					= "Grup",
.private				= "Privada",
.all					= "Tots",
.type					= "Tipus",
.timeSlot				= "TS",
.none					= "Cap",
.contact_saved			= "Contacte Desat",
.duplicate				= "Duplicat",
.tg						= "TG",
.pc						= "ID",
.ts						= "TS",
.mode					= "Mode",
.colour_code			= "Codi Color",
.n_a					= "N/D",
.bandwidth				= "Ample banda",
.stepFreq				= "Salt",
.tot					= "TOT",
.off					= "NO",
.zone_skip				= "Saltar zona",
.all_skip				= "Saltar tot",
.yes					= "Sí",
.no						= "No",
.rx_group				= "RX Grup",
.on						= "Sí",
.timeout_beep			= "Avís T.O.T.",
.UNUSED_1				= "",
.calibration			= "Calibració",
.band_limits			= "Límit bandes",
.beep_volume			= "Volum tons",
.dmr_mic_gain			= "DMR mic",
.fm_mic_gain			= "FM mic", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "Prem llarg",
.key_repeat				= "Prem rpt",
.dmr_filter_timeout		= "Filtre temps",
.brightness				= "Brillantor",
.brightness_off			= "Brillan. min",
.contrast				= "Contrast",
.colour_invert			= "Invers",
.colour_normal			= "Normal",
.backlight_timeout		= "Temps llum",
.scan_delay				= "Temps Scan",
.yes___in_uppercase		= "Sí",
.no___in_uppercase		= "No",
.DISMISS				= "PASSAR",
.scan_mode				= "Mode Scan",
.hold					= "Parar",
.pause					= "Pausa",
.empty_list				= "Llista buida",
.delete_contact_qm		= "Esborrar ctte?",
.contact_deleted		= "Ctte esborrat",
.contact_used			= "Ctte en ús a",
.in_rx_group			= "la Llista TG/ID",
.select_tx				= "Selec. TX",
.edit_contact			= "Editar Ctte",
.delete_contact			= "Esborrar Ctte",
.group_call				= "Cridar a Grup",
.all_call				= "Cridar a Tots",
.tone_scan				= "Scan CCTCS",//// MaxLen: 16
.low_battery			= "BATERIA BAIXA !!",//// MaxLen: 16
.Auto					= "Auto", // MaxLen 16 (with .mode + ':') 
.manual					= "Manual",  // MaxLen 16 (with .mode + ':') 
.ptt_toggle				= "Enclavar PTT", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling	= "Gestió PC", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "Stop", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 línia", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 línies", // MaxLen 16 (with ':' + .contact)
.new_channel			= "Nou canal", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order			= "Prio.", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "DMR Beep", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Inici", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Tots", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold			= "VOX Nivell", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail				= "VOX Cua", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt			= "Prompt",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent					= "Silenci", // Maxlen 16 (with : + audio_prompt)
.normal					= "Normal", // Maxlen 16 (with : + audio_prompt)
.beep					= "Beep", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1	= "Veu L1", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias	= "TA Tx", // Maxlen 16 (with : + .on or .off)
.squelch_VHF			= "VHF Squelch",// Maxlen 16 (with : + XX%)
.squelch_220			= "220 Squelch",// Maxlen 16 (with : + XX%)
.squelch_UHF			= "UHF Squelch", // Maxlen 16 (with : + XX%)
.display_background_colour = "Color" , // Maxlen 16 (with : + .colour_normal or .colour_invert)
.openGD77 				= "OpenGD77",// Do not translate
.openGD77S 				= "OpenGD77S",// Do not translate
.openDM1801 			= "OpenDM1801",// Do not translate
.openRD5R 				= "OpenRD5R",// Do not translate
.gitCommit				= "Git commit",
.voice_prompt_level_2	= "Veu L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3	= "Veu L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter				= "DMR Filter",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter			= "CC Filter", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter			= "TS Filter", // MaxLen: 12 (with ':' + settings: .on or .off)
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
#endif /* USER_INTERFACE_LANGUAGES_CATALAN_H_ */
