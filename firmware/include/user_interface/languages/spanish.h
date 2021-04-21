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
 * Translators: EA3IGM, EA5SW	
 *	
 *	
 * Rev: 5
 */
#ifndef USER_INTERFACE_LANGUAGES_SPANISH_H_
#define USER_INTERFACE_LANGUAGES_SPANISH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t spanishLanguage =
{
.LANGUAGE_NAME 			= "Spanish", // MaxLen: 16 
.menu					= "Menú", // MaxLen: 16
.credits				= "Créditos", // MaxLen: 16
.zone					= "Zona", // MaxLen: 16
.rssi					= "RSSI", // MaxLen: 16
.battery				= "Batería", // MaxLen: 16
.contacts				= "Contactos", // MaxLen: 16
.last_heard				= "Ult. escuchados", // MaxLen: 16
.firmware_info			= "Info Firmware", // MaxLen: 16
.options				= "Opciones", // MaxLen: 16
.display_options		= "Opciones display", // MaxLen: 16
.sound_options			= "Opciones sonido", // MaxLen: 16
.channel_details		= "Detalles Canal", // MaxLen: 16
.language				= "Idioma", // MaxLen: 16
.new_contact			= "Nuevo contacto", // MaxLen: 16
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.hotspot_mode			= "Hotspot", // MaxLen: 16
.contact_details		= "Detalles contacto", // MaxLen: 16
.built					= "Compilado", // MaxLen: 16
.zones					= "Zonas", // MaxLen: 16
.keypad					= "Teclado", // MaxLen: 12 (with .ptt)
.ptt					= "PTT", // MaxLen: 12 (with .keypad)
.locked					= "Bloqueado", // MaxLen: 15
.press_blue_plus_star	= "Pulsa azul + *", // MaxLen: 19
.to_unlock				= "para desbloquear", // MaxLen: 19
.unlocked				= "Desbloqueado", // MaxLen: 15
.power_off				= "Apagando...", // MaxLen: 16
.error					= "ERROR", // MaxLen: 8
.rx_only				= "Solo Rx", // MaxLen: 14
.out_of_band			= "FUERA DE BANDA", // MaxLen: 14
.timeout				= "TIMEOUT", // MaxLen: 8
.tg_entry				= "Entrar TG", // MaxLen: 15
.pc_entry				= "Entrar PC", // MaxLen: 15
.user_dmr_id			= "ID DMR Usuario", // MaxLen: 15
.contact 				= "Contacto", // MaxLen: 15
.accept_call			= "Aceptar PC?", // MaxLen: 16
.private_call			= "Llamada Privada", // MaxLen: 16
.squelch				= "Squelch", // MaxLen: 8
.quick_menu 			= "Menú rápido", // MaxLen: 16
.filter					= "Filtro", // MaxLen: 7 (with ':' + settings: "None", "CC", "CC,TS", "CC,TS,TG")
.all_channels			= "Lista Canales", // MaxLen: 16
.gotoChannel			= "Ir canal",  // MaxLen: 11 (" 1024")
.scan					= "Escanear", // MaxLen: 16
.channelToVfo			= "Canal --> VFO", // MaxLen: 16
.vfoToChannel			= "VFO --> Canal", // MaxLen: 16
.vfoToNewChannel		= "VFO --> Nuevo C.", // MaxLen: 16
.group					= "Grupo", // MaxLen: 16 (with .type)
.private				= "Privado", // MaxLen: 16 (with .type)
.all					= "Todos", // MaxLen: 16 (with .type)
.type					= "Tipo", // MaxLen: 16 (with .type)
.timeSlot				= "Timeslot", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none					= "Ninguno", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:")
.contact_saved			= "Contacto grabado", // MaxLen: 16
.duplicate				= "Duplicado", // MaxLen: 16
.tg						= "TG",  // MaxLen: 8
.pc						= "PC", // MaxLen: 8
.ts						= "TS", // MaxLen: 8
.mode					= "Modo",  // MaxLen: 12
.colour_code			= "Codigo Color", // MaxLen: 16 (with ':' * .n_a)
.n_a					= "N/A",// MaxLen: 16 (with ':' * .colour_code)
.bandwidth				= "Ancho banda", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq				= "Paso", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot					= "TOT", // MaxLen: 16 (with ':' + .off or 15..3825)
.off					= "Off", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip				= "Saltar zona", // MaxLen: 16 (with ':' + .yes or .no) 
.all_skip				= "Saltar todo", // MaxLen: 16 (with ':' + .yes or .no)
.yes					= "Si", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.no						= "No", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.rx_group				= "Grp.RX", // MaxLen: 16 (with ':' and codeplug group name)
.on						= "On", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep			= "Sonido TOT", // MaxLen: 16 (with ':' + .off or 5..20)
.UNUSED_1				= "",
.calibration			= "Calibración", // MaxLen: 16 (with ':' + .on or .off)
.band_limits			= "Limite Banda", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume			= "Vol. BEEP", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain			= "Micro DMR", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.fm_mic_gain			= "Micro FM", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "Tec.larga", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat				= "Tec.repetir", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout		= "Fil. tiempo", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness				= "Brillo", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off			= "Brillo min", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast				= "Contraste", // MaxLen: 16 (with ':' + 12..30)
.colour_invert			= "Color:Invertido", // MaxLen: 16
.colour_normal			= "Color:Normal", // MaxLen: 16
.backlight_timeout		= "Tiempo luz", // MaxLen: 16 (with ':' + .no to 30s)
.scan_delay				= "Tiempo Scan", // MaxLen: 16 (with ':' + 1..30 + 's')
.yes___in_uppercase		= "SI", // MaxLen: 8 (choice above green/red buttons)
.no___in_uppercase		= "NO", // MaxLen: 8 (choice above green/red buttons)
.DISMISS				= "DESPEDIR", // MaxLen: 8 (choice above green/red buttons)
.scan_mode				= "Modo Scan", // MaxLen: 16 (with ':' + .hold or .pause)
.hold					= "Deten", // MaxLen: 16 (with ':' + .scan_mode)
.pause					= "Pausa", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list				= "Lista vacía", // MaxLen: 16
.delete_contact_qm		= "Borrar contacto?", // MaxLen: 16
.contact_deleted		= "Contacto borrado", // MaxLen: 16
.contact_used			= "Contacto usado", // MaxLen: 16
.in_rx_group			= "en grupo RX", // MaxLen: 16
.select_tx				= "Selecciona TX", // MaxLen: 16
.edit_contact			= "Editar contacto", // MaxLen: 16
.delete_contact			= "Borrar contacto", // MaxLen: 16
.group_call				= "Llamada Grupo", // MaxLen: 16
.all_call				= "Llamada Todos", // MaxLen: 16
.tone_scan				= "Tono scan",//// MaxLen: 16
.low_battery			= "BATERÍA BAJA!!!",//// MaxLen: 16
.Auto					= "Auto", // MaxLen 16 (with .mode + ':') 
.manual					= "Manual",  // MaxLen 16 (with .mode + ':') 
.ptt_toggle				= "PTT Fijo", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling	= "Filtro PC", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "Parar", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 linea", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 linea", // MaxLen 16 (with ':' + .contact)
.new_channel			= "Nuevo canal", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order			= "Orden", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "DMR beep", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Inicio", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Ambos", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold			= "VOX Gan.", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail				= "VOX Ret.", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt			= "Prompt",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent                 = "Silencio", // Maxlen 16 (with : + audio_prompt)
.normal                 = "Normal", // Maxlen 16 (with : + audio_prompt)
.beep					= "Beep", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1	= "Voz L1", // Maxlen 16 (with : + audio_prompt)
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
.voice_prompt_level_2	= "Voz L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3	= "Voz L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter				= "Filtro DMR",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter			= "Filtro CC", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter			= "Filtro TS", // MaxLen: 12 (with ':' + settings: .on or .off)
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
#endif /* USER_INTERFACE_LANGUAGES_SPANISH_H_ */
