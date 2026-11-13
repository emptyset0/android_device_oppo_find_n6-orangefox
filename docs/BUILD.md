# Build and package

The release uses a pinned Android 16 OrangeFox tree. The versions and patch paths are in `manifest/fox16.xml` and `manifest/project-pins.json`. Use a clean Ubuntu build host with the AOSP build prerequisites (including JDK 17, Python 3, and the `libdeflate0` package). Check out the device-tree commit recorded for the release; do not use a moving `main` to reproduce a tagged release.

Follow the commands in the [README build section](../README.md#build). For a
tagged release, use its tag for both the manifest and the device-tree checkout.

`apply-source.py` applies the lock-screen CPU-temperature fix as part of the default Recovery patch and installs all supplemental headers. It extracts the pinned kernel from the matching OEM `boot.img`; the extracted kernel SHA-256 must be `e2efc9dcae2d180e22c85b5d760776b232178dfa758784cf94df3bc2df39147f`. The kernel runs from the device's existing boot partition, not from the packaged recovery. The raw AOSP `recoveryimage` is **not** the downloadable N6 image.

The release is a **seed-based repack**, not a source-only reconstruction. It reuses the already integrated OEM runtime and other ramdisk files from the previous verified release. The allowed seed version, exact asset and SHA-256 are pinned in `manifest/release-seed.json`. `m recovery` does not package or publish; run the separate release command below.

```sh
version=v0.1.2 # replace with the intended release version
build_date=YYYYMMDD # build date in Asia/Taipei
product_out=out/target/product/swangoose/recovery/root
# Build the HAL as described in docs/CRYPTO-HAL.md using the pinned NDK.
python3 device/oppo/swangoose/tools/package-release.py \
  --version "$version" --date "$build_date" \
  --seed /path/to/orangefox_r12.0_oppo_find_n6_swangoose_v0.1.1_20261112.img \
  --core "$product_out/system/bin/recovery" \
  --minuitwrp "$product_out/system/lib64/libminuitwrp.so" \
  --crypto-hal /path/to/rebuilt/crypto-hal \
  --oem-boot /path/to/oem/boot.img \
  --ndk-properties /path/to/android-ndk-r29/source.properties \
  --android-clang prebuilts/clang/host/linux-x86/clang-r547379/bin/clang++ \
  --output-dir "release-${version}"
```

Use the Recovery copy of `libminuitwrp.so` from the same build as the executable. The release command requires all three payloads, verifies their packaged bytes, the complete ramdisk checksum ledger, the size limit and the preserved envelope, then writes the `.img`, `sha256sums.txt`, and `build-provenance.json`. The provenance records the device-tree commit, seed, OEM input, toolchain, and payload digests. It describes the supplied inputs; it is not a signed build attestation.

Publish these three files together with versioned notes identifying the asset, its SHA-256 and source commit. Recheck source preparation from a clean tree and verify the actual image before release. A byte-identical repack with the same inputs does not show that the OEM-dependent seed can be reconstructed from public source.
