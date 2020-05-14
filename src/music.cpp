/* 
 * FFNx - Complete OpenGL replacement of the Direct3D renderer used in 
 * the original ports of Final Fantasy VII and Final Fantasy VIII for the PC.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * music.c - replacements routines for music player
 */

#include <windows.h>

#include "music.h"
#include "patch.h"
#include "directmusic.h"
#include "winamp/music.h"

void music_init()
{
	// Add Global Focus flag to DirectSound Secondary Buffers
	patch_code_byte(common_externals.directsound_buffer_flags_1 + 0x4, 0x80); // DSBCAPS_GLOBALFOCUS & 0x0000FF00

	if (use_external_music)
	{
		if (ff8) {
			replace_function(common_externals.play_midi, ff8_play_midi);
			replace_function(common_externals.pause_midi, pause_midi);
			replace_function(common_externals.restart_midi, restart_midi);
			replace_function(common_externals.stop_midi, ff8_stop_midi);
			replace_function(common_externals.midi_status, midi_status);
			replace_function(common_externals.set_midi_volume, ff8_set_direct_volume);
			replace_function(common_externals.remember_midi_playing_time, remember_playing_time);
		}
		else {
			replace_function(common_externals.midi_init, midi_init);
			replace_function(common_externals.use_midi, ff7_use_midi);
			replace_function(common_externals.play_midi, play_midi);
			replace_function(common_externals.cross_fade_midi, cross_fade_midi);
			replace_function(common_externals.pause_midi, pause_midi);
			replace_function(common_externals.restart_midi, restart_midi);
			replace_function(common_externals.stop_midi, stop_midi);
			replace_function(common_externals.midi_status, midi_status);
			replace_function(common_externals.set_master_midi_volume, set_master_midi_volume);
			replace_function(common_externals.set_midi_volume, set_midi_volume);
			replace_function(common_externals.set_midi_volume_trans, set_midi_volume_trans);
			replace_function(common_externals.set_midi_tempo, set_midi_tempo);
			replace_function(common_externals.directsound_release, ff7_directsound_release);
		}
		winamp_music_init();
	}
}

uint midi_init(uint unknown)
{
	// without this there will be no volume control for music in the config menu
	*ff7_externals.midi_volume_control = true;

	// enable fade function
	*ff7_externals.midi_initialized = true;

	return true;
}

char ff8_midi[32];

char* ff8_midi_name(uint midi)
{
	// midi_name format: {num}{type}-{name}.sgt or {name}.sgt or _Missing.sgt
	char* midi_name = common_externals.get_midi_name(midi),
		* truncated_name;

	truncated_name = strchr(midi_name, '-');

	if (nullptr != truncated_name) {
		truncated_name += 1; // Remove "-"
	}
	else {
		truncated_name = midi_name;
	}

	char* max_midi_name = strchr(truncated_name, '.');

	if (nullptr != max_midi_name) {
		size_t len = max_midi_name - truncated_name;

		if (len < 32) {
			memcpy(ff8_midi, truncated_name, len);
			ff8_midi[len] = '\0';

			return ff8_midi;
		}
	}

	return nullptr;
}

uint ff8_play_midi(uint midi, uint volume, uint u1, uint u2)
{
	info("FF8 midi play %i %i %i %i\n", midi, volume, u1, u2);

	char* midi_name = ff8_midi_name(midi);

	if (nullptr == midi_name) {
		return 0; // Error
	}

	if (use_external_music)
	{
		winamp_set_music_volume(volume);
		winamp_play_music(midi_name, midi);
	}

	return 1; // Success
}

uint ff7_use_midi(uint midi)
{
	char* name = common_externals.get_midi_name(midi);

	if (winamp_can_play(name)) {
		return 1;
	}

	return strcmp(name, "HEART") != 0 && strcmp(name, "SATO") != 0 && strcmp(name, "SENSUI") != 0 && strcmp(name, "WIND") != 0;
}

void play_midi(uint midi)
{
	if (use_external_music)
		winamp_play_music(common_externals.get_midi_name(midi), midi);
}

void cross_fade_midi(uint midi, uint time)
{
	if (use_external_music)
		winamp_cross_fade_music(common_externals.get_midi_name(midi), midi, time);
}

void pause_midi()
{
	if (use_external_music)
		winamp_pause_music();
}

void restart_midi()
{
	if (use_external_music)
		winamp_resume_music();
}

void stop_midi()
{
	if (use_external_music)
		winamp_stop_music();
}

uint ff8_stop_midi()
{
	if (trace_all || trace_music) info("FF8 stop midi\n");

	// Stop game midi for horizon concert instruments
	if (nullptr != *ff8_externals.directmusic_performance) {
		(*ff8_externals.directmusic_performance)->Stop(nullptr, nullptr, 0, DMUS_SEGF_DEFAULT);
	}

	stop_midi();

	return 0;
}

uint midi_status()
{
	if (use_external_music)
		return winamp_music_status();
	return 0;
}

uint ff8_set_direct_volume(int volume)
{
	if (trace_all || trace_music) info("FF8 set direct volume %i\n", volume);

	// Set game volume for horizon concert instruments
	if (nullptr != *ff8_externals.directmusic_performance) {
		(*ff8_externals.directmusic_performance)->SetGlobalParam(
			*(ff8_externals.GUID_PerfMasterVolume), &volume, sizeof(volume)
		);
	}

	if (volume == DSBVOLUME_MIN) {
		volume = 0;
	}
	else {
		volume = (pow(10, (volume + 2000) / 2000.0f) / 10.0f) * 255.0f;
	}
	
	if (use_external_music)
		winamp_set_direct_volume(volume);
	return 1; // Success
}

void set_master_midi_volume(uint volume)
{
	if (use_external_music)
		winamp_set_master_music_volume(volume);
}

void set_midi_volume(uint volume)
{
	if (use_external_music)
		winamp_set_music_volume(volume);
}

void set_midi_volume_trans(uint volume, uint step)
{
	if (use_external_music)
		winamp_set_music_volume_trans(volume, step);
}

void set_midi_tempo(unsigned char tempo)
{
	if (use_external_music)
		winamp_set_music_tempo(tempo);
}

uint ff7_directsound_release()
{
	if (nullptr == *common_externals.directsound) {
		return 0;
	}

	music_cleanup();
	(*common_externals.directsound)->Release();
	*common_externals.directsound = nullptr;

	return 0;
}

void music_cleanup()
{
	if (use_external_music)
		winamp_music_cleanup();
}

uint remember_playing_time()
{
	if (use_external_music)
		winamp_remember_playing_time();
	return 0;
}

bool is_wm_theme(char* midi)
{
	if (nullptr == midi)
	{
		return false;
	}

	if (ff8)
	{
		return strcmp(midi, "field") == 0;
	}
	
	return strcmp(midi, "TA") == 0 || strcmp(midi, "TB") == 0 || strcmp(midi, "KITA") == 0;
}

bool needs_resume(uint old_mode, uint new_mode, char* old_midi, char* new_midi)
{
	/*
	 * BATTLE -> FIELD or WM
	 * FIELD  -> WM
	 */
	return ((new_mode == MODE_WORLDMAP || new_mode == MODE_AFTER_BATTLE) && !is_wm_theme(old_midi) && is_wm_theme(new_midi))
		|| ((old_mode == MODE_BATTLE || old_mode == MODE_SWIRL) && (new_mode == MODE_FIELD || new_mode == MODE_AFTER_BATTLE))
		|| new_mode == MODE_CARDGAME;
}
