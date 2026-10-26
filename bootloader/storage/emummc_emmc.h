#ifndef EMUMMC_EMMC_H
#define EMUMMC_EMMC_H

#include <bdk.h>

#define EMUMMC_ENABLED_EMMC            4
#define EMUMMC_EXO_TYPE_PARTITION_EMMC 3

bool             emummc_emmc_is_active();
sdmmc_storage_t *emummc_raw_storage();
u32              emummc_mmc_partition(u32 partition);
int              emummc_emmc_init();
bool             emummc_emmc_probe_path(const char *path);
int              emummc_emmc_path_enabled_value();

#endif
