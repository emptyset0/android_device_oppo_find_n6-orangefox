# v0.1.1

- Use `swangoose` as the Find N6 Recovery codename.
- Apply balanced CPU scaling before Recovery and FastbootD diverge.
- Fix CPU-temperature reporting on the Recovery lock screen.

| Published asset | SHA-256 |
| --- | --- |
| `orangefox_r12.0_oppo_find_n6_swangoose_v0.1.1_20261112.img` | `2fcb6b428e69041ce709aed6c66bd65459030885b884abc6513895b63110c445` |

Device-tree commit: `79a7734534780c4e9ae4e4ad46f89d52c26cd669`.
This image used the v0.1.0 seed, SHA-256 `5bff330598089bcea57e49fb25acfa80e5cbaea004ce75a624a3f712e75fceed`.

Validated on PLP110 with CPH2765 firmware: slot A Recovery write/readback, cold boot, native PIN unlock and Windows MTP enumeration. FastbootD was also tested. Slot B was unchanged.
