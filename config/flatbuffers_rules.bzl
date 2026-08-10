# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

def _flatbuffer_json_to_bin_impl(ctx):
    flatc = ctx.executable.flatc
    json = ctx.file.json
    schema = ctx.file.schema

    # flatc will name the file the same as the json (can't be changed)
    out_name = json.basename[:-len(".json")] + ".bin"
    out = ctx.actions.declare_file(out_name, sibling = json)

    # flatc args ---------------------------------
    flatc_args = [
        "-b",
        "-o",
        out.dirname,
    ]

    for inc in ctx.attr.includes:
        flatc_args.extend(["-I", inc.path])

    if ctx.attr.strict_json:
        flatc_args.append("--strict-json")

    flatc_args.extend([schema.path, json.path])
    # --------------------------------------------

    ctx.actions.run(
        inputs = [json, schema] + list(ctx.files.includes),
        outputs = [out],
        executable = flatc,
        arguments = flatc_args,
        progress_message = "flatc generation {}".format(json.short_path),
        mnemonic = "FlatcGeneration",
    )

    rf = ctx.runfiles(
        files = [out],
        root_symlinks = {
            ("_main/" + ctx.attr.out_dir + "/" + out_name): out,
        },
    )

    return DefaultInfo(files = depset([out]), runfiles = rf)

flatbuffer_json_to_bin = rule(
    implementation = _flatbuffer_json_to_bin_impl,
    attrs = {
        "json": attr.label(
            allow_single_file = [".json"],
            mandatory = True,
            doc = "Json file to convert. Note that the binary file will have the same name as the json (minus the suffix)",
        ),
        "schema": attr.label(
            allow_single_file = [".fbs"],
            mandatory = True,
            doc = "FBS file to use",
        ),
        "out_dir": attr.string(
            default = "etc",
            doc = "Directory to copy the generated file to, sibling to 'src' and 'tests' dirs. Do not include a trailing '/'",
        ),
        "flatc": attr.label(
            default = Label("@flatbuffers//:flatc"),
            executable = True,
            cfg = "exec",
            doc = "Reference to the flatc binary",
        ),
        # flatc arguments
        "includes": attr.label_list(
            allow_files = True,
            doc = "Flatc include paths",
        ),
        "strict_json": attr.bool(
            default = False,
            doc = "Require strict JSON (no trailing commas etc)",
        ),
    },
)
