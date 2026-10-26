# emuMMC on eMMC (raw partition)

This fork adds one feature to upstream hekate: **booting HOS from an emuMMC stored as a raw partition on the console's internal eMMC**, instead of on the SD card.

It is meant to be used together with the matching Atmosphère fork ([postkevone/Atmosphere](https://github.com/postkevone/Atmosphere), branch `emmc-emummc`). The logic is ported from [NaGaa95/hekate-pro](https://github.com/NaGaa95/hekate-pro), whose eMMC work is by lulle2007200 (Christoph Baumann). Only the boot path for a raw eMMC partition was taken, and it was re-implemented as a small, separate module so the fork stays easy to rebase on upstream.

## Requirements

- **Atmosphère fork's exosphere:** boot entries must load `package3` from the Atmosphère fork, for example `pkg3=atmosphere/package3` (`fss0=` is the deprecated alias of the same key). Its exosphere understands emuMMC type `3` (partition on eMMC). Stock exosphere does not.
- **Atmosphère fork's emuMMC module as `bootloader/sys/emummc.kipm`:** hekate does not use the emuMMC KIP inside `package3`. It always injects `bootloader/sys/emummc.kipm` into FS. The stock `emummc.kipm` does not know type `3`, so it must be replaced with one built from the Atmosphère fork's `emummc/emummc_unpacked.kip` (see "Release package" below).
- **Creating the partition:** this fork can boot from an eMMC emuMMC partition but cannot create one. Create it once with hekate-pro. Its partition manager shrinks the sysNAND `USER` partition and adds the `emummc` partition after it, and its emuMMC tool writes the files described below.

Launching Atmosphère's `fusee.bin` as a payload doesn't need this fork, because fusee reads `emummc.ini` itself.

## Configuration

`sd:/emuMMC/emummc.ini`:

```ini
[emummc]
enabled=4
sector=0x<start sector of the partition>
id=0x0000
nintendo_path=emuMMC/<folder>/Nintendo
```

| `enabled` | Meaning |
|---|---|
| `0` | emuMMC off |
| `1` | emuMMC on SD (raw partition if `sector` is set, otherwise files at `path`). Same as upstream. |
| `4` | emuMMC raw partition on the eMMC (this fork). `sector` is required. |

`enabled=4` is the hekate-pro convention, and the Atmosphère fork uses it too. Upstream hekate does not know this value. It treats `enabled=4` like `enabled=1`, so it would boot an SD emuMMC at that sector, which is the wrong storage.

**`emupath` boot entries:** an emuMMC folder containing a `raw_emmc_based` file (4-byte little-endian start sector, as hekate-pro writes it) is selected as `enabled=4`. Folders with `raw_based` (SD partition) or `file_based` (SD files) work as upstream.

File-based emuMMC on the eMMC (hekate-pro's `file_emmc_based`) is not supported. `enabled=4` without a `sector` fails with "eMMC emuMMC must be partition based."

## Safety checks

Before booting, hekate reads the sysNAND GPT. It refuses to boot with "Failed to init emuMMC" if:

- the start sector is at or before the end of the sysNAND `USER` partition ("eMMC emuMMC overlaps sysNAND!"), or
- the partition's two boot areas (8 MiB) don't fit on the eMMC ("eMMC emuMMC is outside of eMMC!").

The Atmosphère fork's bootloader performs the same check.

## How it works

The emuMMC partition uses the usual raw layout: BOOT0 (4 MiB), BOOT1 (4 MiB), then the user area (GPP).

- **Reads and writes** (`emummc_storage_read/write`) use `emmc_storage` instead of `sd_storage`. The sector math is the same as upstream's SD raw mode: `sector + emu sector + part_offset * 0x2000`.
- **eMMC partition:** `emummc_storage_set_mmc_partition` records the requested partition, but keeps the real eMMC on the user area (GPP), because the whole emuMMC lives there.
- **Shutdown:** `emummc_storage_end` ends the eMMC, not the SD card.
- **Exosphere config:** `config_exosphere` writes emuMMC type `3` with the start sector. The config layout is unchanged, so it matches the Atmosphère fork.

Package1, the GPT and package2 are all read through these functions, so the whole HOS boot path uses the eMMC emuMMC.

## Nyx (GUI)

Only safety and display changes. Nyx has no eMMC emuMMC tools in this fork.

- **emuMMC info:** shows "eMMC Raw Partition" instead of "SD Raw Partition".
- **USB mass storage:** the emuMMC BOOT0, BOOT1 and GPP buttons refuse with "Active emuMMC is on eMMC! Not supported here." Upstream would expose the *SD card* at the eMMC sector offset, and writes from a PC could corrupt the SD.
- **Unchanged:** "Change emuMMC" does not list eMMC emuMMC folders, so it cannot switch to one.

## Files

New files, which contain all of the feature logic:

| File | Purpose |
|---|---|
| `bootloader/storage/emummc_emmc.h/.c` | Active check, storage/partition selection, sysNAND overlap check, `raw_emmc_based` probing |
| `nyx/nyx_gui/frontend/fe_emummc_emmc.h` | `emummc_cfg_is_emmc()` helper for Nyx |
| `docs/emummc_on_emmc.md` | This document |

Upstream files with small hook lines:

| File | Change |
|---|---|
| `Makefile` | `emummc_emmc` added to the bootloader `OBJS` |
| `bootloader/storage/emummc.c` | `raw_emmc_based` probe in `emummc_set_path`; init check; storage/partition selection in read, write, set_partition and end |
| `bootloader/hos/secmon_exo.c` | `emuMMC_PartitionEmmc = 3` and its use in the exosphere config |
| `nyx/nyx_gui/frontend/gui_tools.c` | UMS refusal (error `4`) and its message |
| `nyx/nyx_gui/frontend/gui_emummc_tools.c` | "eMMC"/"SD" label in the emuMMC info panel |

## Release package

The release zip is the official hekate release layout, with the files built from this fork replacing the official ones:

| File | Source |
|---|---|
| `hekate_ctcaer_<ver>.bin`, `bootloader/update.bin` | `output/hekate.bin` |
| `bootloader/sys/nyx.bin`, `libsys_lp0.bso`, `libsys_minerva.bso` | `output/` |
| `bootloader/sys/emummc.kipm` | Atmosphère fork: `python3 emummc/tools/kip1converter.py emummc/emummc_unpacked.kip emummc.data` (writes `emummc.caps` to the current folder), then `cat emummc.caps emummc.data > emummc.kipm` |
| `bootloader/res/*`, `bootloader/sys/res.pak`, `thk.bin`, `l4t/*` | Unchanged from the official release zip |

The 8GB variant (`__ram8GB.bin`) is the same payload with byte `0x122` set to `0x01`. That byte is `rsvd_flags` (`RSVD_FLAG_DRAM_8GB`) in the version header at `0x118` (magic `ICTC`). It is the only difference between CTCaer's official normal and `__ram8GB` binaries.

## Updating from upstream

```sh
git fetch upstream
git rebase upstream/master   # or: git merge upstream/master
```

Conflicts can only happen at the hook lines above. The most likely spots are `emummc_storage_*` in `bootloader/storage/emummc.c` and the emuMMC block of `config_exosphere()`. If upstream ever gives `enabled=4` or exosphere type `3` another meaning, change `EMUMMC_ENABLED_EMMC` / `EMUMMC_EXO_TYPE_PARTITION_EMMC` together with the Atmosphère fork.

## What was left out of hekate-pro

| Feature | Why it was left out |
|---|---|
| emuSD (SD redirection) and its new exosphere config layout | Not requested. It also needs the Atmosphère-Pro config layout. |
| File-based emuMMC on an eMMC FAT partition | Only the raw partition was requested |
| Booting hekate/Atmosphère files from the eMMC ("boot storage") | Not requested |
| Nyx eMMC partition manager, emuMMC creation/migration on eMMC | Boot-only scope. Use hekate-pro once to create the partition. |
| `memory_mode_auto`, bdk return-value changes, other tweaks | Not related to this feature |

## Testing

1. Build: `make` (devkitARM). The output is `output/hekate.bin` and `output/nyx.bin`.
2. With an eMMC emuMMC partition made by hekate-pro and `enabled=4`, launch an entry with `pkg3=` pointing at the Atmosphère fork's `package3`, and confirm the console boots into emuMMC.
3. Change something in emuMMC and check that sysNAND does not show it.
4. Regression: SD raw and SD file emuMMC still boot.
5. Nyx: the emuMMC info panel says "eMMC Raw Partition", and the emuMMC UMS buttons refuse.

## Licensing

Like the rest of hekate, the new files are licensed under GPLv2 (see `LICENSE`). Per this fork's rules they contain no comment header, so their licensing and credits are recorded here: lulle2007200 (Christoph Baumann), NaGaa95, and CTCaer (hekate).
