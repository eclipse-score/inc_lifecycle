// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
//
use std::path::PathBuf;

fn main() {
    let crate_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    let out_path = PathBuf::from(&crate_dir).join("include/alive_monitor_ffi.h");

    cbindgen::generate(&crate_dir)
        .expect("Unable to generate bindings")
        .write_to_file(out_path);

    println!("cargo:rerun-if-changed=src");
}
