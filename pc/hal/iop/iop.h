/**
 * iop.h — PS2 IOP (I/O Processor) HAL
 *
 * The IOP handles:
 *   - File I/O (CD/DVD reads via CDVDMAN/FILEIO)
 *   - Audio (SPU2 via LIBSDR/SDRDRV)
 *   - Memory card (MCMAN/MCSERV)
 *
 * On PC, these are replaced by POSIX file I/O, SDL2 audio, and
 * platform save file storage respectively.
 */

#ifndef AG_RAC_HAL_IOP_H
#define AG_RAC_HAL_IOP_H

#include "../../include/types.h"

/* ── File I/O ──────────────────────────────────────────────────────────────── */

/**
 * iop_file_open() — Open a game data file by path.
 *   On PS2, the game uses paths like "cdrom0:\\WADS\\LEVEL01.WAD;1"
 *   On PC, we redirect these to the extracted assets directory.
 *
 *   @return  File handle (>= 0), or -1 on error
 */
int iop_file_open(const char *path, int flags);

/**
 * iop_file_read() — Read bytes from an open file.
 *   @return  Number of bytes read, or -1 on error
 */
int iop_file_read(int fd, void *buf, u32 size);

/**
 * iop_file_seek() — Seek within a file.
 */
int iop_file_seek(int fd, s32 offset, int whence);

/**
 * iop_file_close() — Close a file handle.
 */
int iop_file_close(int fd);

/**
 * iop_file_exists() — Check if a file exists.
 */
int iop_file_exists(const char *path);

/* ── Audio ─────────────────────────────────────────────────────────────────── */

/**
 * iop_audio_init() — Initialize the audio subsystem via SDL2.
 *   Equivalent to sceSdInit() / SsInit() on PS2.
 */
void iop_audio_init(void);

/**
 * iop_audio_shutdown() — Shutdown audio.
 */
void iop_audio_shutdown(void);

/**
 * iop_audio_load_vag() — Load a .VAG ADPCM audio clip.
 *   @return  Audio handle, or -1 on error
 */
int iop_audio_load_vag(const void *data, u32 size);

/**
 * iop_audio_play() — Play a loaded audio clip.
 *   @param handle  From iop_audio_load_vag()
 *   @param loop    Non-zero to loop
 *   @param volume  0–127
 */
void iop_audio_play(int handle, int loop, u8 volume);

/**
 * iop_audio_stop() — Stop playback of a clip.
 */
void iop_audio_stop(int handle);

/* ── Memory Card (Save Data) ─────────────────────────────────────────────────── */

/**
 * iop_memcard_save() — Write save data to persistent storage.
 *   On PC, saves to %APPDATA%/AG-RAC/save.dat (or platform equivalent).
 */
int iop_memcard_save(const void *data, u32 size);

/**
 * iop_memcard_load() — Load save data from persistent storage.
 *   @return  Bytes read, or -1 if no save exists
 */
int iop_memcard_load(void *data, u32 max_size);

/* ── IOP Init/Shutdown ───────────────────────────────────────────────────────── */

/**
 * iop_hal_init() — Initialize all IOP subsystems.
 *   @param assets_root  Path to extracted game assets directory
 */
void iop_hal_init(const char *assets_root);

/**
 * iop_hal_shutdown() — Shutdown all IOP subsystems.
 */
void iop_hal_shutdown(void);

#endif /* AG_RAC_HAL_IOP_H */
