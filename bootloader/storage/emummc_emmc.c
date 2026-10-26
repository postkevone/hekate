#include <string.h>
#include <stdlib.h>

#include <bdk.h>

#include "emummc.h"
#include "emummc_emmc.h"
#include "../config.h"
#include "../gfx/gfx.h"
#include <libs/fatfs/ff.h>

#define EMUMMC_EMMC_BOOT_PARTS_SECTORS 0x4000

static bool emummc_emmc_path_probed = false;

static const u16 sysnand_user_name[] = { 'U', 'S', 'E', 'R', 0 };

bool emummc_emmc_is_active()
{
	return emu_cfg.enabled == EMUMMC_ENABLED_EMMC && !h_cfg.emummc_force_disable;
}

sdmmc_storage_t *emummc_raw_storage()
{
	return emummc_emmc_is_active() ? &emmc_storage : &sd_storage;
}

u32 emummc_mmc_partition(u32 partition)
{
	return emummc_emmc_is_active() ? EMMC_GPP : partition;
}

static int _emummc_emmc_get_sysnand_user_end(u64 *user_end)
{
	int res = 1;
	gpt_t *gpt = (gpt_t *)zalloc(sizeof(gpt_t));

	emmc_set_partition(EMMC_GPP);
	if (sdmmc_storage_read(&emmc_storage, 1, sizeof(gpt_t) / EMMC_BLOCKSIZE, gpt))
		goto out;

	if (memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
		goto out;

	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, sysnand_user_name, sizeof(sysnand_user_name)))
		{
			*user_end = gpt->entries[i].lba_end;
			res = 0;
			break;
		}
	}

out:
	free(gpt);
	return res;
}

int emummc_emmc_init()
{
	if (!emu_cfg.sector)
	{
		EPRINTF("eMMC emuMMC must be partition based.");
		return 1;
	}

	u64 user_end;
	if (_emummc_emmc_get_sysnand_user_end(&user_end))
	{
		EPRINTF("Failed to read sysNAND GPT.");
		return 1;
	}

	if (emu_cfg.sector <= user_end)
	{
		EPRINTF("eMMC emuMMC overlaps sysNAND!");
		return 1;
	}

	if (emu_cfg.sector + EMUMMC_EMMC_BOOT_PARTS_SECTORS >= emmc_storage.sec_cnt)
	{
		EPRINTF("eMMC emuMMC is outside of eMMC!");
		return 1;
	}

	return 0;
}

bool emummc_emmc_probe_path(const char *path)
{
	FIL fp;
	u32 sector = 0;

	emummc_emmc_path_probed = false;

	strcpy(emu_cfg.emummc_file_based_path, path);
	strcat(emu_cfg.emummc_file_based_path, "/raw_emmc_based");

	if (f_open(&fp, emu_cfg.emummc_file_based_path, FA_READ))
		return false;

	if (!f_read(&fp, &sector, 4, NULL) && sector)
	{
		emu_cfg.sector = sector;
		emummc_emmc_path_probed = true;
	}

	f_close(&fp);

	return emummc_emmc_path_probed;
}

int emummc_emmc_path_enabled_value()
{
	int enabled = emummc_emmc_path_probed ? EMUMMC_ENABLED_EMMC : 1;
	emummc_emmc_path_probed = false;

	return enabled;
}
