# Copyright (c) 2016-2025 The Zcash developers
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# To update the Rust stdlib, change the version below and then run the script
# ./contrib/devtools/update-rust-hashes.py

package:=rust_stdlib
$(package)_version:=1.88.0
$(package)_download_path:=https://static.rust-lang.org/dist
$(package)_dependencies:=native_rust

# FreeBSD (x86_64)
$(package)_targets += x86_64-unknown-freebsd
$(package)_target_x86_64-unknown-freebsd:=x86_64-unknown-freebsd
$(package)_sha256_hash_x86_64-unknown-freebsd:=ac55f0f90ef9ba0ed1cd740ba9aa294d31934cd26676b8157a803a37a82b6995

# Linux (ARMv7)
$(package)_targets += armv7-unknown-linux-musleabihf
$(package)_target_arm-unknown-linux-gnueabihf:=armv7-unknown-linux-musleabihf
$(package)_target_armv7-unknown-linux-gnueabihf:=armv7-unknown-linux-musleabihf
$(package)_sha256_hash_armv7-unknown-linux-musleabihf:=fe0b2d4c4abb6ec8fab1205e5700e0c59b64c1687488c984c9a0730370dee9bc

# Linux (ARMv8)
$(package)_targets += aarch64-unknown-linux-musl
$(package)_target_aarch64-unknown-linux-gnu:=aarch64-unknown-linux-musl
$(package)_sha256_hash_aarch64-unknown-linux-musl:=b49c15ec0e2a4d0315a39ef16be9018adbc2ac0ab246a2a930b0287fa3fe17b7

# Linux (PowerPC 64-bit little-endian)
$(package)_targets += powerpc64le-unknown-linux-musl
$(package)_target_powerpc64le-unknown-linux-gnu:=powerpc64le-unknown-linux-musl
$(package)_sha256_hash_powerpc64le-unknown-linux-musl:=f993e1feb81b38c7e1c671d58c8af047d67d3ef3f28608f5c167320c862c6a69

# Linux (RISCV64GC)
$(package)_targets += riscv64gc-unknown-linux-musl
$(package)_target_riscv64-unknown-linux-gnu:=riscv64gc-unknown-linux-musl
$(package)_target_riscv64gc-unknown-linux-gnu:=riscv64gc-unknown-linux-musl
$(package)_sha256_hash_riscv64gc-unknown-linux-musl:=5ed55315d49756544abb1aa185d3fa65a0a97ab2db851cd984cb0bfef3edfd9b

# Linux (x86_64)
$(package)_targets += x86_64-unknown-linux-musl
$(package)_target_x86_64-unknown-linux-gnu:=x86_64-unknown-linux-musl
$(package)_sha256_hash_x86_64-unknown-linux-musl:=6fe8b3106b14ea92eb33b3dc3cbe3cd367489e12b5c015f851d9713733438354

# macOS (ARMv8)
$(package)_targets += aarch64-apple-darwin
$(package)_target_aarch64-apple-darwin:=aarch64-apple-darwin
$(package)_target_arm64-apple-darwin:=aarch64-apple-darwin
$(package)_sha256_hash_aarch64-apple-darwin:=f852990a4bb1a84cef74fb92d0c9715b453379c32af43a8b3b357cadcc78f542

# macOS (x86_64)
$(package)_targets += x86_64-apple-darwin
$(package)_target_x86_64-apple-darwin:=x86_64-apple-darwin
$(package)_sha256_hash_x86_64-apple-darwin:=1e3494b90a88e8c01492f4d2e21bb60792f8b9c6e307e716db86822ccc55a77e

# Windows (x86_64)
$(package)_targets += x86_64-pc-windows-gnu
$(package)_target_x86_64-w64-mingw32:=x86_64-pc-windows-gnu
$(package)_sha256_hash_x86_64-pc-windows-gnu:=5dae719c6c2b11f9ce7858cc4b3f6302a1a83519adf7066bcf0c79e5c4cda568

$(package)_target:=$(or \
  $($(package)_target_$(canonical_host)),\
  $($(package)_target_$(host_arch)-$(host_vendor)-$(host_os)),\
  $($(package)_target_$(subst -pc-,-unknown-,$(canonical_host))),\
  $($(package)_target_$(subst -unknown-,-pc-,$(canonical_host))),\
  $($(package)_target_$(subst -linux-,-unknown-linux-,$(canonical_host))))

$(package)_file_name:=rust-std-$($(package)_version)-$($(package)_target).tar.gz
$(package)_sha256_hash:=$($(package)_sha256_hash_$($(package)_target))

define $(package)_fetch_cmds
  $(call fetch_file,$(package),$($(package)_download_path),$($(package)_file_name),$($(package)_file_name),$($(package)_sha256_hash))
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_dir)/$(host_prefix)/native/lib/rustlib && \
  cp -r rust-std-$($(package)_target)/lib/rustlib/$($(package)_target) $($(package)_staging_dir)/$(host_prefix)/native/lib/rustlib/
endef
