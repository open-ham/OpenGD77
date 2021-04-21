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
 * Translators: CT4TX, CT1HSN
 *
 *
 * Rev: 2
 */
#ifndef USER_INTERFACE_LANGUAGES_PORTUGUESE_H_
#define USER_INTERFACE_LANGUAGES_PORTUGUESE_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t portuguesLanguage =
{
.LANGUAGE_NAME 			= "Portugues",
.menu					= "Menu",
.credits				= "Creditos",
.zone					= "Zona",
.rssi					= "RSSI",
.battery				= "Bateria",
.contacts				= "Contactos",
.last_heard				= "Ultima escutada",
.firmware_info			= "Versao Firmware",
.options				= "Opcoes",
.display_options		= "Opcoes Visor",
.sound_options			= "Sound options", // MaxLen: 16
.channel_details		= "Detalhes Canal",
.language				= "Lingua",
.new_contact			= "Contacto Novo",
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.contact_details		= "Detalhes Contato",
.hotspot_mode			= "Hotspot",
.built					= "Built",
.zones					= "Zonas",
.keypad					= "Teclado",
.ptt 					= "PTT",
.locked 				= "Bloqueado",
.press_blue_plus_star	="Prima Azul + *",
.to_unlock				= "Para Desbloquear",
.unlocked				= "Desbloqueado",
.power_off				= "Desligar...",
.error					= "ERRO",
.rx_only				= "Apenas Rx",
.out_of_band			= "FORA DA BANDA",
.timeout				= "TEMPO ESGOTADO",
.tg_entry				= "Entrada TG",
.pc_entry				= "Entrada PC",
.user_dmr_id			= "DMRID Utilizador",
.contact 				= "Contacto",
.accept_call			= "Aceitar Chamada?",
.private_call			= "Chamada Privada",
.squelch				= "Squelch",
.quick_menu 			= "Menu Rápido",
.filter					= "Filtro",
.all_channels			= "Todos os Canais",
.gotoChannel			= "Ir para",
.scan					= "Busca",
.channelToVfo			= "Canal --> VFO",
.vfoToChannel			= "VFO --> Canal",
.vfoToNewChannel		= "VFO --> Novo Can",
.group					= "Grupo",
.private				= "Privado",
.all					= "Todos",
.type					= "Tipo",
.timeSlot				= "Intervalo Tempo", // Too long
.none					= "Nenhum",
.contact_saved			= "Contacto Gravado",
.duplicate				= "Duplicado",
.tg						= "TG",
.pc						= "PC",
.ts						= "TS",
.mode					= "Mode",
.colour_code			= "Codigo de cores",
.n_a					= "N/A",
.bandwidth				= "Largura de banda",
.stepFreq				= "Passo",
.tot					= "TOT",
.off					= "Off",
.zone_skip				= "Z Ignorar",
.all_skip				= "A Ignorar",
.yes					= "Sim",
.no						= "Nao",
.rx_group				= "Rx Grp",
.on						= "On",
.timeout_beep			= "Beep timeout",
.UNUSED_1				= "",
.calibration			= "Calibracao",
.band_limits			= "Limites banda",
.beep_volume			= "Volume beep",
.dmr_mic_gain			= "Micro DMR",
.fm_mic_gain				= "Micro FM", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "Key long",
.key_repeat				= "Key rpt",
.dmr_filter_timeout		= "Filtro DMR",
.brightness				= "Brilho",
.brightness_off				= "Min bright",
.contrast				= "Contraste",
.colour_invert			= "Invertido",
.colour_normal			= "Normal",
.backlight_timeout		= "Timeout",
.scan_delay				= "Scan delay",
.yes___in_uppercase					= "SIM",
.no___in_uppercase						= "NÃO",
.DISMISS				= "DISPENSAR",
.scan_mode				= "Scan mode",
.hold					= "Hold",
.pause					= "Pause",
.empty_list				= "Empty List",
.delete_contact_qm			= "Delete contact?",
.contact_deleted			= "Contact deleted",
.contact_used				= "Contact used",
.in_rx_group				= "in RX group",
.select_tx				= "Select TX",
.edit_contact				= "Edit Contact",
.delete_contact				= "Delete Contact",
.group_call				= "Group Call",
.all_call				= "All Call",
.tone_scan				= "Tone scan",//// MaxLen: 16
.low_battery			= "LOW BATTERY !!!",//// MaxLen: 16
.Auto					= "Auto", // MaxLen 16 (with .mode + ':') 
.manual					= "Manual",  // MaxLen 16 (with .mode + ':') 
.ptt_toggle				= "PTT toggle", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling			= "Handle PC", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "Stop", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 line", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 lines", // MaxLen 16 (with ':' + .contact)
.new_channel			= "Canal novo", // MaxLen: 16, leave room for a space and four channel digits after
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
.voice_prompt_level_1					= "Voice", // Maxlen 16 (with : + audio_prompt)
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
.voice_prompt_level_2	= "Voice L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3	= "Voice L3", // Maxlen 16 (with : + audio_prompt)
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
#endif /* USER_INTERFACE_LANGUAGES_PORTUGUESE_H_ */
