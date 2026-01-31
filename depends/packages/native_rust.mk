# Copyright (c) 2016-2025 The Zcash developers
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# To update the Rust compiler, change the version below and then run the script
# ./contrib/devtools/update-rust-hashes.py

package:=native_rust
$(package)_version:=1.88.0
$(package)_download_path:=https://static.rust-lang.org/dist

# FreeBSD (x86_64)
$(package)_file_name_x86_64_freebsd:=rust-$($(package)_version)-x86_64-unknown-freebsd.tar.gz
$(package)_sha256_hash_x86_64_freebsd:=961de5d723b034c1308d2b4a4d710fe006fb87bdbf914d045c01a5df87a0b332

# Linux (ARMv8)
$(package)_file_name_aarch64_linux:=rust-$($(package)_version)-aarch64-unknown-linux-gnu.tar.gz
$(package)_sha256_hash_aarch64_linux:=dbc75abc31d142eacf15e60d0e51c4f291539974221d217b80786756b0ce1d6b

# Linux (x86_64)
$(package)_file_name_x86_64_linux:=rust-$($(package)_version)-x86_64-unknown-linux-gnu.tar.gz
$(package)_sha256_hash_x86_64_linux:=ad6f0cc845e7fcca17fd451bafd2c04a7bbcb543f8f3ef5bc412fd1fef99ef7b

# macOS (ARMv8)
$(package)_file_name_aarch64_darwin:=rust-$($(package)_version)-aarch64-apple-darwin.tar.gz
$(package)_sha256_hash_aarch64_darwin:=dee921b9a41b1c3fbb088ad31dcca3b232de2cb89c268db75f40912eeaa474db

# macOS (x86_64)
$(package)_file_name_x86_64_darwin:=rust-$($(package)_version)-x86_64-apple-darwin.tar.gz
$(package)_sha256_hash_x86_64_darwin:=b36b0bfac17e0a1f6cc06b9fdc4e2131ad578b4122a67792236b58650ae4c5c8

$(package)_file_name:=$($(package)_file_name_$(build_arch)_$(build_os))
$(package)_sha256_hash:=$($(package)_sha256_hash_$(build_arch)_$(build_os))

define $(package)_set_vars
$(package)_stage_opts=--disable-ldconfig
$(package)_stage_build_opts=--without=rust-docs-json-preview,rust-docs
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_file_name),$($(package)_file_name),$($(package)_sha256_hash))
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_dir)/$(host_prefix)/native/bin && \
  mkdir -p $($(package)_staging_dir)/$(host_prefix)/native/lib/rustlib && \
  cp cargo/bin/cargo $($(package)_staging_dir)/$(host_prefix)/native/bin/ && \
  cp rustc/bin/rustc $($(package)_staging_dir)/$(host_prefix)/native/bin/ && \
  cp rustc/bin/rustdoc $($(package)_staging_dir)/$(host_prefix)/native/bin/ && \
  cp -r rustc/lib/* $($(package)_staging_dir)/$(host_prefix)/native/lib/ && \
  cp -r rust-std-*/lib/rustlib/* $($(package)_staging_dir)/$(host_prefix)/native/lib/rustlib/ && \
  $(call run_if_exists,$(BASEDIR)/../contrib/guix/libexec/fix-elf-interpreter.sh,\
    $($(package)_staging_dir)/$(host_prefix)/native/lib \
    $($(package)_staging_dir)/$(host_prefix)/native/bin/cargo \
    $($(package)_staging_dir)/$(host_prefix)/native/bin/rustc \
    $($(package)_staging_dir)/$(host_prefix)/native/bin/rustdoc)
endef
