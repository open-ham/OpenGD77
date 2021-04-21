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
/* WARNING: THIS FILE USES BINARY (OCTAL) ENCODING, IT USES REMAPPED GLYPHS
 *
 * Translators: SQ7PTE
 *
 * Rev: 4.8
 */
#ifndef USER_INTERFACE_LANGUAGES_POLISH_H_
#define USER_INTERFACE_LANGUAGES_POLISH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with binary encoding
 *
 ********************************************************************/
const stringsTable_t polishLanguage =
{
.LANGUAGE_NAME 			= "Polski", // MaxLen: 16
.language				= "JÜzyk", // MaxLen: 16
.menu					= "Spis", // MaxLen: 16
.credits				= "WspÛàtwÛrcy", // MaxLen: 16
.zone					= "Strefy", // MaxLen: 16
.rssi					= "Sygnaà", // MaxLen: 16
.battery				= "Bateria", // MaxLen: 16
.contacts				= "Kontakty", // MaxLen: 16
.firmware_info				= "Wersja programu", // MaxLen: 16
.last_heard				= "Ostatnio aktywne", // MaxLen: 16
.options				= "Opcje", // MaxLen: 16
.display_options			= "Opcje ekranu", // MaxLen: 16
.sound_options				= "Opcje dêwiÜku", // MaxLen: 16
.channel_details			= "Detale kanaàu", // MaxLen: 16
.new_contact				= "Nowy kontakt", // MaxLen: 16
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.hotspot_mode				= "HotSpot", // MaxLen: 16
.contact_details			= "Detale kontaktu", // MaxLen: 16
.built					= "Kompilacja", // MaxLen: 16
.zones					= "Strefy", // MaxLen: 16
.keypad					= "Klawiatura", // MaxLen: 12 (with .ptt)
.ptt					= "PTT", // MaxLen: 12 (with .keypad)
.locked					= "zablokowana", // MaxLen: 15
.press_blue_plus_star			= "Niebieski + *", // MaxLen: 19
.to_unlock				= "aby odblokowaÇ", // MaxLen: 19
.unlocked				= "Odblokowane", // MaxLen: 15
.power_off				= "WyàÑczanie...", // MaxLen: 16
.error					= "BáÉD", // MaxLen: 8
.rx_only				= "Tylko odbiÛr", // MaxLen: 14
.out_of_band				= "POZA PASMEM", // MaxLen: 14
.timeout				= "CZAS MINÉá", // MaxLen: 8
.tg_entry				= "Wpisz numer TG", // MaxLen: 15
.pc_entry				= "Wpisz numer PC", // MaxLen: 15
.user_dmr_id				= "ID uíytkownika", // MaxLen: 15
.contact 				= "Kontakt", // MaxLen: 15
.accept_call				= "ZaakceptowaÇ?", // MaxLen: 16
.private_call				= "Prywatne", // MaxLen: 16
.squelch				= "Squelch", // MaxLen: 8
.quick_menu 				= "Szybkie opcje", // MaxLen: 16
.filter					= "Filtr", // MaxLen: 7 (with ':' + settings: .none, "CC", "CC,TS", "CC,TS,TG")
.all_channels				= "Wszystkie", // MaxLen: 16
.gotoChannel				= "Idê do",  // MaxLen: 11 (" 1024")
.scan					= "Skanowanie", // MaxLen: 16
.channelToVfo				= "Kanaà > VFO", // MaxLen: 16
.vfoToChannel				= "VFO > Kanaà", // MaxLen: 16
.vfoToNewChannel		= "VFO > Nowy kanaà", // MaxLen: 16
.group					= "Grupa", // MaxLen: 16 (with .type)
.private				= "Prywatny", // MaxLen: 16 (with .type)
.all					= "Wszystko", // MaxLen: 16 (with .type)
.type					= "Typ", // MaxLen: 16 (with .type)
.timeSlot				= "Szczelina", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none					= "Brak", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:", and .filter)
.contact_saved				= "Zapisz kontakt", // MaxLen: 16
.duplicate				= "Duplikat", // MaxLen: 16
.tg					= "TG",  // MaxLen: 8
.pc					= "PC", // MaxLen: 8
.ts					= "TS", // MaxLen: 8
.mode					= "Tryb",  // MaxLen: 12
.colour_code				= "Kolor kodu", // MaxLen: 16 (with ':' * .n_a)
.n_a					= "N/A",// MaxLen: 16 (with ':' * .colour_code)
.bandwidth				= "Pasmo", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq				= "Krok", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot					= "TOT", // MaxLen: 16 (with ':' + .off or 15..3825)
.off					= "Wyà.", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip				= "Pomiã kanaà", // MaxLen: 16 (with ':' + .yes or .no)
.all_skip				= "Pomiã All", // MaxLen: 16 (with ':' + .yes or .no)
.yes					= "Tak", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.no					= "Nie", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.rx_group				= "Grupa", // MaxLen: 16 (with ':' and codeplug group name)
.on					= "Wà.", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep				= "Czas bipa", // MaxLen: 16 (with ':' + .off or 5..20)
.UNUSED_1				= "",
.calibration				= "Kalibracja", // MaxLen: 16 (with ':' + .on or .off)
.band_limits				= "Limit pasma", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume				= "Gàos bipa", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain				= "DMR mic", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.fm_mic_gain				= "FM mic", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "Key dàugi", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat				= "Key krÛtki", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout			= "Czas filtra", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness				= "JasnoçÇ", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off				= "Mini jasnoçÇ", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast				= "Kontrast", // MaxLen: 16 (with ':' + 12..30)
.colour_invert				= "Inwersja", // MaxLen: 16
.colour_normal				= "Normalny", // MaxLen: 16
.backlight_timeout			= "åwiecenie", // MaxLen: 16 (with ':' + .no to 30)
.scan_delay				= "Czas skan.", // MaxLen: 16 (with ':' + 1..30 + 's')
.yes___in_uppercase					= "TAK", // MaxLen: 8 (choice above green/red buttons)
.no___in_uppercase					= "NIE", // MaxLen: 8 (choice above green/red buttons)
.DISMISS				= "ODWOáAÅ", // MaxLen: 8 (choice above green/red buttons)
.scan_mode				= "Tryb skan.", // MaxLen: 16 (with ':' + .hold or .pause)
.hold					= "StÛj", // MaxLen: 16 (with ':' + .scan_mode)
.pause					= "Pauza", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list				= "Lista pusta", // MaxLen: 16
.delete_contact_qm			= "Usuã kontakt?", // MaxLen: 16
.contact_deleted			= "Kontakt usuniety", // MaxLen: 16
.contact_used				= "Kontakt istnieje", // MaxLen: 16
.in_rx_group				= "W grupie RX", // MaxLen: 16
.select_tx				= "Wybierz TX", // MaxLen: 16
.edit_contact				= "Edytuj kontakt", // MaxLen: 16
.delete_contact				= "Usuã kontakt", // MaxLen: 16
.group_call				= "Grupy", // MaxLen: 16
.all_call				= "áÑcze wszystkich", // MaxLen: 16
.tone_scan				= "Skanuj",//// MaxLen: 16
.low_battery				= "SáABA BATERIA!!!",//// MaxLen: 16
.Auto					= "Automat", // MaxLen 16 (with .mode + ':')
.manual					= "Manualny",  // MaxLen 16 (with .mode + ':') 
.ptt_toggle				= "Staàe PTT", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling  = "ZezwÛl PC", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "Stop", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 linia", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 linie", // MaxLen 16 (with ':' + .contact)
.new_channel				= "Nowy kanaà", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order				= "WybÛr", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "WybÛr bipa", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Start", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Oba", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold                          = "VOX Thres.", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail                               = "VOX Tail", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt				= "Bip audio",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent                                 = "Brak", // Maxlen 16 (with : + audio_prompt)
.normal                                 = "Normal", // Maxlen 16 (with : + audio_prompt)
.beep					= "Bip", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1					= "Voice", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias	= "TA Tx", // Maxlen 16 (with : + .on or .off)
.squelch_VHF			= "VHF Squelch",// Maxlen 16 (with : + XX%)
.squelch_220			= "220 Squelch",// Maxlen 16 (with : + XX%)
.squelch_UHF			= "UHF Squelch", // Maxlen 16 (with : + XX%)
.display_background_colour = "Kolor" , // Maxlen 16 (with : + .colour_normal or .colour_invert)
.openGD77 				= "OpenGD77",// Do not translate
.openGD77S 				= "OpenGD77S",// Do not translate
.openDM1801 			= "OpenDM1801",// Do not translate
.openRD5R 				= "OpenRD5R",// Do not translate
.gitCommit				= "Git commit",
.voice_prompt_level_2	= "Gàos L2", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_3	= "Gàos L3", // Maxlen 16 (with : + audio_prompt)
.dmr_filter				= "DMR Filtr",// MaxLen: 12 (with ':' + settings: "TG" or "Ct" or "RxG")
.dmr_cc_filter			= "CC Filtr", // MaxLen: 12 (with ':' + settings: .on or .off)
.dmr_ts_filter			= "TS Filtr", // MaxLen: 12 (with ':' + settings: .on or .off)
.dtmf_contact_list			= "FM DTMF contacts", // Maxlen: 16
.channel_power				= "Moc kanaàu", //Displayed as "Ch Power:" + .from_master or "Ch Power:"+ power text e.g. "Power:500mW" . Max total length 16
.from_master				= "Master",// Displayed if per-channel power is not enabled  the .channel_power
.set_quickkey				= "Set Quick Key", // MaxLen: 16
.dual_watch				= "Nasàuch 1i2 VFO", // MaxLen: 16
.info					= "Info", // MaxLen: 16 (with ':' + .off or.ts or .pwr or .both)
.pwr					= "Moc",
.user_power				= "Max moc +W-",
.temperature				= "Temperatura", // MaxLen: 16 (with ':' + .celcius or .fahrenheit)
.celcius				= "∞C",
.seconds				= "sekund",
.radio_info				= "Info o radiu",
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
 * or emacs on Linux with binary encoding
 *
 ********************************************************************/
#endif /* USER_INTERFACE_LANGUAGES_POLISH_H_ */
