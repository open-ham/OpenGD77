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
 * Translators: F1CXG, F1RMB
 *
 *
 * Rev: 3
 */
#ifndef USER_INTERFACE_LANGUAGES_FRENCH_H_
#define USER_INTERFACE_LANGUAGES_FRENCH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t frenchLanguage =
{
.LANGUAGE_NAME			= "Français",
.menu					= "Menu",
.credits				= "Crédits",
.zone					= "Zone",
.rssi					= "RSSI",
.battery				= "Batterie",
.contacts				= "Contacts",
.last_heard				= "Derniers reçus",
.firmware_info				= "Info Firmware",
.options				= "Options",
.display_options			= "Options affich.",
.sound_options				= "Options audio", // MaxLen: 16
.channel_details			= "Détails canal",
.language				= "Langue",
.new_contact				= "Nouv. contact",
.dmr_contacts				= "Contacts DMR", // MaxLen: 16
.contact_details			= "Détails contact",
.hotspot_mode				= "Hotspot",
.built					= "Créé",
.zones					= "Zones",
.keypad					= "Clavier",
.ptt					= "PTT",
.locked					= "Verrouillé",
.press_blue_plus_star			= "Pressez Bleu + *",
.to_unlock				= "pour déverrouiller",
.unlocked				= "Déverrouillé",
.power_off				= "Extinction...",
.error					= "ERREUR",
.rx_only				= "Rx Uniqmnt.",
.out_of_band				= "HORS BANDE",
.timeout				= "TIMEOUT",
.tg_entry				= "Entrez TG",
.pc_entry				= "Entrez PC",
.user_dmr_id				= "DMR ID Perso.",
.contact 				= "Contact",
.accept_call				= "Répondre à",
.private_call				= "Appel Privé",
.squelch				= "Squelch",
.quick_menu 				= "Menu Rapide",
.filter					= "Filtre",
.all_channels				= "Tous Canaux",
.gotoChannel				= "Aller",
.scan					= "Scan",
.channelToVfo				= "Canal --> VFO",
.vfoToChannel				= "VFO --> Canal",
.vfoToNewChannel			= "VFO --> Nv. Can.", // MaxLen: 16
.group					= "Groupe",
.private				= "Privé",
.all					= "Tous",
.type					= "Type",
.timeSlot				= "Timeslot",
.none					= "Aucun",
.contact_saved				= "Contact sauvé",
.duplicate				= "Dupliqué",
.tg					= "TG",
.pc					= "PC",
.ts					= "TS",
.mode					= "Mode",
.colour_code				= "Code Couleur",
.n_a					= "ND",
.bandwidth				= "Larg. bde",
.stepFreq				= "Pas",
.tot					= "TOT",
.off					= "Off",
.zone_skip				= "Saut Zone",
.all_skip				= "Saut Compl.",
.yes					= "Oui",
.no					= "Non",
.rx_group				= "Grp Rx",
.on					= "On",
.timeout_beep				= "Son timeout",
.UNUSED_1				= "",
.calibration				= "Étalonnage",
.band_limits				= "Lim. Bandes",
.beep_volume				= "Vol. bip",
.dmr_mic_gain				= "DMR mic",
.fm_mic_gain				= "FM mic", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "Appui long",
.key_repeat				= "Répét°",
.dmr_filter_timeout			= "Tps filtre",
.brightness				= "Rétro écl.",
.brightness_off				= "Écl. min",
.contrast				= "Contraste",
.colour_invert				= "Inverse",
.colour_normal				= "Normale",
.backlight_timeout			= "Timeout",
.scan_delay				= "Délai scan",
.yes___in_uppercase			= "OUI",
.no___in_uppercase			= "NON",
.DISMISS				= "CACHER",
.scan_mode				= "Mode scan",
.hold					= "Bloque",
.pause					= "Pause",
.empty_list				= "Liste Vide",
.delete_contact_qm			= "Efface contact ?",
.contact_deleted			= "Contact effacé",
.contact_used				= "Contact utilisé",
.in_rx_group				= "ds le groupe RX",
.select_tx				= "Select° TX",
.edit_contact				= "Édite Contact",
.delete_contact				= "Efface Contact",
.group_call				= "Appel de Groupe",
.all_call				= "All Call",
.tone_scan				= "Scan tons",
.low_battery			        = "BATT. FAIBLE !!!",//// MaxLen: 16
.Auto					= "Auto", // MaxLen 16 (with .mode + ':') 
.manual					= "Manuel",  // MaxLen 16 (with .mode + ':') }
.ptt_toggle				= "Bascule PTT", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling			= "Filtre PC", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "Arrêt", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 ligne", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 lignes", // MaxLen 16 (with ':' + .contact)
.new_channel				= "Nouv. canal", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order				= "Ordre", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "Bip TX", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Début", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Les Deux", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold                          = "Seuil VOX", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail                               = "Queue VOX", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt				= "Prompt",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent                                 = "Silence", // Maxlen 16 (with : + audio_prompt)
.normal                                 = "Normal", // Maxlen 16 (with : + audio_prompt)
.beep					= "Beep", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1			= "Voix", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias			= "Tx TA", // Maxlen 16 (with : + .on or .off)
.squelch_VHF				= "Squelch VHF",// Maxlen 16 (with : + XX%)
.squelch_220				= "Squelch 220",// Maxlen 16 (with : + XX%)
.squelch_UHF				= "Squelch UHF", // Maxlen 16 (with : + XX%)
.display_background_colour 		= "Couleur" , // Maxlen 16 (with : + .colour_normal or .colour_invert)
.openGD77 				= "OpenGD77",// Do not translate
.openGD77S 				= "OpenGD77S",// Do not translate
.openDM1801 				= "OpenDM1801",// Do not translate
.openRD5R 				= "OpenRD5R",// Do not translate
.gitCommit				= "Commit Git",
.voice_prompt_level_2			= "Voix L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3			= "Voix L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter				= "Filtre DMR",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter				= "Filtre CC", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter				= "Filtre TS", // MaxLen: 12 (with ':' + settings: .on or .off)
.dtmf_contact_list			= "Contacts DTMF FM", // Maxlen: 16
.channel_power				= "Pce Canal", //Displayed as "Ch Power:" + .from_master or "Ch Power:"+ power text e.g. "Power:500mW" . Max total length 16
.from_master				= "Maître",// Displayed if per-channel power is not enabled  the .channel_power
.set_quickkey				= "Défini Quickkey", // MaxLen: 16
.dual_watch				= "Double Veille", // MaxLen: 16
.info					= "Info", // MaxLen: 16 (with ':' + .off or.ts or .pwr or .both)
.pwr					= "Pwr",
.user_power				= "Pce Perso.", // MaxLen: 16 (with ':' + 0..4100)
.temperature				= "Temperature", // MaxLen: 16 (with ':' + .celcius or .fahrenheit)
.celcius				= "°C",
.seconds				= "secondes",
.radio_info				= "Infos radio",
.temperature_calibration		= "Etal. t°",
.pin_code				= "Code Pin",
.please_confirm				= "Confirmez", // MaxLen: 15
.vfo_freq_bind_mode			= "Freq. Liées",
.overwrite_qm				= "Écraser ?", //Maxlen: 14 chars
.eco_level				= "Niveau Eco",
.buttons				= "Boutons",
.leds					= "DELs",
.scan_dwell_time			= "Durée Scan"
};
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
#endif /* USER_INTERFACE_LANGUAGES_FRENCH_H_ */
