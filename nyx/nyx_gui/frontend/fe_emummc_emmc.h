#ifndef _FE_EMUMMC_EMMC_H_
#define _FE_EMUMMC_EMMC_H_

#include "fe_emummc_tools.h"

#define EMUMMC_ENABLED_EMMC 4

static inline bool emummc_cfg_is_emmc(const emummc_cfg_t *emu_info)
{
	return emu_info->enabled == EMUMMC_ENABLED_EMMC;
}

#endif
