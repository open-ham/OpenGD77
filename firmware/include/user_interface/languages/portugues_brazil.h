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
 * Translators: PU4RON
 *
 *
 * Rev: 2
 */
#ifndef USER_INTERFACE_LANGUAGES_PORTUGUESE_BRAZIL_H_
#define USER_INTERFACE_LANGUAGES_PORTUGUESE_BRAZIL_H_
/********************************************************************
 *
 * MUITO IMPORTANTE.
 * Este arquivo não deve ser salvo com a codificação UTF-8
 * Use o Notepad ++ no Windows com codificação ANSI
 * ou emacs no Linux com codificação windows-1252-unix
 *
 ********************************************************************/
const stringsTable_t portuguesBrazilLanguage =
{
.LANGUAGE_NAME 			= "Portugues BR",
.menu					= "Menu",
.credits				= "Créditos",
.zone					= "Zona",
.rssi					= "RSSI",
.battery				= "Bateria",
.contacts				= "Contatos",
.last_heard				= "Último escutado",
.firmware_info			= "Firmware",
.options				= "Opções",
.display_options		= "Opções display",
.sound_options			= "Opções de áudio", 
.channel_details		= "Detalhes Canal",
.language				= "Idioma",
.new_contact			= "Novo contato",
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.contact_details		= "Detalhe contato",
.hotspot_mode			= "Modo Hotspot",
.built					= "Compilado",
.zones					= "Zonas",
.keypad					= "Teclado",
.ptt 					= "PTT",
.locked 				= "Bloqueado",
.press_blue_plus_star	= "Aperte azul + *",
.to_unlock				= "Desbloquear",
.unlocked				= "Desbloqueado",
.power_off				= "Desligando...",
.error					= "ERRO",
.rx_only				= "Apenas Rx",
.out_of_band			= "FORA DA BANDA",
.timeout				= "TEMPO ESGOTADO",
.tg_entry				= "Inserir TG",
.pc_entry				= "Inserir CP",
.user_dmr_id			= "DMRID usuário",
.contact 				= "Contato",
.accept_call			= "Aceitar chamada?",
.private_call			= "Chamada privada",
.squelch				= "Squelch",
.quick_menu 			= "Menu rápido",
.filter					= "Filtro",
.all_channels			= "Todos canais",
.gotoChannel			= "Ir para",
.scan					= "Busca",
.channelToVfo			= "Canal -> VFO",
.vfoToChannel			= "VFO -> Canal",
.vfoToNewChannel		= "VFO -> NovoCanal",
.group					= "Grupo",
.private				= "Privado",
.all					= "Todos",
.type					= "Tipo",
.timeSlot				= "TimeSlot", 
.none					= "Nenhum",
.contact_saved			= "Contato gravado",
.duplicate				= "Duplicado",
.tg						= "TG",
.pc						= "CP",
.ts						= "TS",
.mode					= "Modo",
.colour_code			= "Códig cor",
.n_a					= "N/A",
.bandwidth				= "Larg banda",
.stepFreq				= "Step",
.tot					= "TOT",
.off					= "Off",
.zone_skip				= "Ignorar Z",
.all_skip				= "Ignorar T",
.yes					= "Sim",
.no						= "Não",
.rx_group				= "Rx Grp",
.on						= "On",
.timeout_beep			= "Bip timeout",
.UNUSED_1				= "",
.calibration			= "Ajustes",
.band_limits			= "Limite banda",
.beep_volume			= "Volume bip",
.dmr_mic_gain			= "Micro DMR",
.fm_mic_gain			= "Micro FM", 
.key_long				= "Key long",
.key_repeat				= "Key rpt",
.dmr_filter_timeout		= "Filtro DMR",
.brightness				= "Brilho",
.brightness_off			= "Min brilho",
.contrast				= "Contraste",
.colour_invert			= "Cor:Invertido",
.colour_normal			= "Cor:Normal",
.backlight_timeout		= "Timeout",
.scan_delay				= "Scan delay",
.yes___in_uppercase					= "SIM",
.no___in_uppercase						= "NÃO",
.DISMISS				= "DISPENSAR",
.scan_mode				= "Modo Scan",
.hold					= "Hold",
.pause					= "Pausa",
.empty_list				= "Lista vazia",
.delete_contact_qm		= "Apagar contato?",
.contact_deleted		= "Contato apagado",
.contact_used			= "Contato usado",
.in_rx_group			= "no grupo Rx",
.select_tx				= "Selecione TX",
.edit_contact			= "Editar contato",
.delete_contact			= "Apagar contato",
.group_call				= "Chamada grupo",
.all_call				= "Todas chamadas",
.tone_scan				= "Tone scan",
.low_battery			= "BATERIA FRACA!",
.Auto					= "Auto", 
.manual					= "Manual", 
.ptt_toggle				= "PTT toggle", 
.private_call_handling	= "Handle CP", 
.stop					= "Stop", 
.one_line				= "1 linha",
.two_lines				= "2 linhas",
.new_channel			= "Novo canal", 
.priority_order			= "Ordem",
.dmr_beep				= "DMR bip",
.start					= "Start",
.both					= "Both",
.vox_threshold          = "VOX Thres.",
.vox_tail               = "VOX Tail",
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
/*********************************************************************
 *
 * MUITO IMPORTANTE.
 * Este arquivo não deve ser salvo com a codificação UTF-8
 * Use o Notepad ++ no Windows com codificação ANSI
 * ou emacs no Linux com codificação windows-1252-unix
 *
 ********************************************************************/
#endif /* USER_INTERFACE_LANGUAGES_PORTUGUESE_H_ */
