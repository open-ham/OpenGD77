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
 * Translators: TA5AYX
 *
 *
 * Rev: 1.1
 */
#ifndef USER_INTERFACE_LANGUAGES_TURKISH_H_
#define USER_INTERFACE_LANGUAGES_TURKISH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t turkishLanguage =
{
.LANGUAGE_NAME 			= "Turkish", // MaxLen: 16
.menu					= "Menü", // MaxLen: 16
.credits				= "Yap“mc“lar", // MaxLen: 16
.zone					= "Bölge", // MaxLen: 16
.rssi					= "RSSI", // MaxLen: 16
.battery				= "Batarya", // MaxLen: 16
.contacts				= "Ki–iler", // MaxLen: 16
.last_heard				= "Son Duyulanlar", // MaxLen: 16
.firmware_info				= "YAZILIM Bilgisi", // MaxLen: 16
.options				= "Opsiyonlar", // MaxLen: 16
.display_options			= "Ekran Opsiyon", // MaxLen: 16
.sound_options				= "Sound options", // MaxLen: 16
.channel_details			= "Kanal Detay", // MaxLen: 16
.language				= "Dil", // MaxLen: 16
.new_contact				= "Yeni Ki–i", // MaxLen: 16
.dmr_contacts				= "DMR contacts", // MaxLen: 16
.hotspot_mode				= "Eri–im Modu", // MaxLen: 16
.contact_details			= "Ki–i Detaylar“", // MaxLen: 16
.built					= "Olu–turma", // MaxLen: 16
.zones					= "Bölgeler", // MaxLen: 16
.keypad					= "Klavye", // MaxLen: 12 (with .ptt)
.ptt					= "PTT", // MaxLen: 12 (with .keypad)
.locked					= "Kiltli", // MaxLen: 15
.press_blue_plus_star			= "Bas Mavi + *", // MaxLen: 19
.to_unlock				= "Açmak için", // MaxLen: 19
.unlocked				= "Kilit Aç“k", // MaxLen: 15
.power_off				= "HO•ÇAKALIN", // MaxLen: 16
.error					= "HATA", // MaxLen: 8
.rx_only				= "Sadece Rx", // MaxLen: 14
.out_of_band				= "BAND D“–“", // MaxLen: 14
.timeout				= "Timeout", // MaxLen: 8
.tg_entry				= "TG Giri–", // MaxLen: 15
.pc_entry				= "PC Giri–", // MaxLen: 15
.user_dmr_id				= "Ki–isel DMR ID", // MaxLen: 15
.contact 				= "Kontakt", // MaxLen: 15
.accept_call				= "Arama Kabul?", // MaxLen: 16
.private_call				= "Özel Arama", // MaxLen: 16
.squelch				= "Squelch", // MaxLen: 8
.quick_menu 				= "H“zl“ Menü", // MaxLen: 16
.filter					= "Filtre", // MaxLen: 7 (with ':' + settings: .none, "CC", "CC,TS", "CC,TS,TG")
.all_channels				= "Tüm Kanallar", // MaxLen: 16
.gotoChannel				= "Kanala Git",  // MaxLen: 11 (" 1024")
.scan					= "Tara", // MaxLen: 16
.channelToVfo				= "Kanal --> VFO", // MaxLen: 16
.vfoToChannel				= "VFO --> Kanal", // MaxLen: 16
.vfoToNewChannel		= "VFO --> Yeni Kanal", // MaxLen: 16
.group					= "Grup", // MaxLen: 16 (with .type)
.private				= "Özel", // MaxLen: 16 (with .type)
.all					= "Tüm", // MaxLen: 16 (with .type)
.type					= "Tip", // MaxLen: 16 (with .type)
.timeSlot				= "Zaman Dilimi", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none					= "Yok", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:", .filter and .mode )
.contact_saved				= "Ki–i Kay“tedildi", // MaxLen: 16
.duplicate				= "Çift!", // MaxLen: 16
.tg					= "TG",  // MaxLen: 8
.pc					= "PC", // MaxLen: 8
.ts					= "TS", // MaxLen: 8
.mode					= "Mod",  // MaxLen: 12
.colour_code				= "Renk Kodu", // MaxLen: 16 (with ':' * .n_a)
.n_a					= "N/A",// MaxLen: 16 (with ':' * .colour_code)
.bandwidth				= "Band geni–", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq				= "Ad“m", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot					= "TX Süre", // MaxLen: 16 (with ':' + .off or 15..3825)
.off					= "Kapal“", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip				= "Bölge Atla", // MaxLen: 16 (with ':' + .yes or .no) 
.all_skip				= "Tüm Atla", // MaxLen: 16 (with ':' + .yes or .no)
.yes					= "Evet", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.no					= "Hay“r", // MaxLen: 16 (with ':' + .zone_skip, .all_skip)
.rx_group				= "Rx Grp", // MaxLen: 16 (with ':' and codeplug group name)
.on					= "Aç", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep				= "Zaman bip", // MaxLen: 16 (with ':' + .off or 5..20)
.UNUSED_1				= "",
.calibration				= "Kalibrasyon", // MaxLen: 16 (with ':' + .on or .off)
.band_limits				= "Band S“n“r“", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume				= "Bip sesi", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain				= "DMR mic", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.fm_mic_gain				= "FM mic", // MaxLen: 16 (with ':' + 0..31)
.key_long				= "Tu– Uzun", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat				= "Tu– rpt", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout			= "FiltreSüre", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness				= "Parlakl“k", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off				= "Az parlak", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast				= "Kontrast", // MaxLen: 16 (with ':' + 12..30)
.colour_invert				= "Ters", // MaxLen: 16
.colour_normal				= "Normal", // MaxLen: 16
.backlight_timeout			= "Zamana–“m“", // MaxLen: 16 (with ':' + .no to 30s)
.scan_delay				= "Tarama H“z“", // MaxLen: 16 (with ':' + 1..30 + 's')
.yes___in_uppercase					= "EVET", // MaxLen: 8 (choice above green/red buttons)
.no___in_uppercase					= "HAYIR", // MaxLen: 8 (choice above green/red buttons)
.DISMISS				= "REDDET", // MaxLen: 8 (choice above green/red buttons)
.scan_mode				= "Tarama modu", // MaxLen: 16 (with ':' + .hold, .pause or .stop)
.hold					= "Dur", // MaxLen: 16 (with ':' + .scan_mode)
.pause					= "Bekle", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list				= "Bo– Liste", // MaxLen: 16
.delete_contact_qm			= "Ki–iyi silmek?", // MaxLen: 16
.contact_deleted			= "Ki–i silindi", // MaxLen: 16
.contact_used				= "Ki–i kullan“mda", // MaxLen: 16
.in_rx_group				= "RX grubunda", // MaxLen: 16
.select_tx				= "TX seçin", // MaxLen: 16
.edit_contact				= "Ki–iyi Düzenle", // MaxLen: 16
.delete_contact				= "Ki–iyi silmek", // MaxLen: 16
.group_call				= "Grup Ça˜r“s“", // MaxLen: 16
.all_call				= "Tüm aramalar", // MaxLen: 16
.tone_scan				= "Ton tarama", // MaxLen: 16
.low_battery				= "DÜ•ÜK BATARYA !", // MaxLen: 16
.Auto					= "Otomatik", // MaxLen 16 (with .mode + ':') 
.manual					= "Manüel",  // MaxLen 16 (with .mode + ':')
.ptt_toggle				= "PTT Tu–u", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling			= "PC izin ver", // MaxLen 16 (with ':' + .on or .off)
.stop					= "Stop", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 sat“r", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 sat“r", // MaxLen 16 (with ':' + .contact)
.new_channel				= "Yeni Kanal", // MaxLen: 16, leave room for a space and four channel digits after
.priority_order				= "Öncelik", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "DMR beep", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Start", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Both", // MaxLen 16 (with ':' + .dmr_beep)
.vox_threshold                          = "VOX Thres.", // MaxLen 16 (with ':' + .off or 1..30)
.vox_tail                               = "VOX Tail", // MaxLen 16 (with ':' + .n_a or '0.0s')
.audio_prompt				= "Prompt",// Maxlen 16 (with ':' + .silent, .normal, .beep or .voice_prompt_level_1)
.silent                                 = "Silent", // Maxlen 16 (with : + audio_prompt)
.normal                                 = "Normal", // Maxlen 16 (with : + audio_prompt)
.beep					= "Beep", // Maxlen 16 (with : + audio_prompt)
.voice_prompt_level_1	= "Voice L1", // Maxlen 16 (with : + audio_prompt)
.transmitTalkerAlias	= "TA Tx", // Maxlen 16 (with : + .on or .off)
.squelch_VHF			= "VHF Squelch",// Maxlen 16 (with : + XX%)
.squelch_220			= "220 Squelch",// Maxlen 16 (with : + XX%)
.squelch_UHF			= "UHF Squelch", // Maxlen 16 (with : + XX%)
.display_background_colour = "Renk" , // Maxlen 16 (with : + .colour_normal or .colour_invert)
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
.dtmf_contact_list		= "FM DTMF contacts", // Maxlen: 16
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
#endif /* USER_INTERFACE_LANGUAGES_TURKISH_H_ */
