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

load("@rules_pkg//pkg:mappings.bzl", "pkg_files")
load("@score_baselibs//score/flatbuffers/bazel:tools.bzl", "serialize_buffer")

def _lm_config_generator_impl(ctx):
    """Run lifecycle_config.py to produce the unified configuration file."""
    script = ctx.executable.script
    config = ctx.file.config
    schema = ctx.file.schema
    base = config.basename
    dot_index = base.rfind(".")
    stem = base if dot_index == -1 else base[:dot_index]
    output_name = "{}_gen.json".format(stem)
    out_file = ctx.actions.declare_file("{}/{}".format(ctx.label.name, output_name))

    ctx.actions.run(
        inputs = [config, schema],
        outputs = [out_file],
        tools = [script],
        mnemonic = "LifecycleJsonConfigGeneration",
        executable = script,
        progress_message = "generating Launch Manager config from {}".format(config.short_path),
        arguments = [
            config.path,
            "--schema",
            schema.path,
            "-o",
            out_file.dirname,
        ],
    )

    return [DefaultInfo(files = depset([out_file]))]

lm_config_generator = rule(
    implementation = _lm_config_generator_impl,
    doc = "Generates the unified configuration file in the flatbuffer format.",
    attrs = {
        "config": attr.label(
            allow_single_file = [".json"],
            mandatory = True,
            doc = "Json file to convert.",
        ),
        "schema": attr.label(
            default = Label("//score/launch_manager/src/daemon/src/configuration/config_schema:launch_manager.schema.json"),
            allow_single_file = [".json"],
            doc = "Json schema file to validate the input json against",
        ),
        "script": attr.label(
            default = Label("//scripts/config_mapping:lifecycle_config"),
            executable = True,
            cfg = "exec",
            doc = "Python script to execute",
        ),
    },
)

def _lm_config_combiner_impl(ctx):
    """Declare a single directory that all serialized buffers get copied into.
    Consumers receive this one directory instead of individual files.
    """
    etc_dir = ctx.actions.declare_directory(ctx.attr.dir_name)

    srcs = ctx.files.srcs

    args = ctx.actions.args()
    args.add(etc_dir.path)
    args.add_all([f.path for f in srcs])

    ctx.actions.run_shell(
        inputs = srcs,
        outputs = [etc_dir],
        arguments = [args],
        command = 'dest="$1"; shift; mkdir -p "$dest"; for f in "$@"; do cp "$f" "$dest/"; done',
        mnemonic = "LmConfigCombine",
        progress_message = "Combining launch manager configs into {}".format(etc_dir.short_path),
    )

    return [DefaultInfo(files = depset([etc_dir]))]

lm_config_combiner = rule(
    implementation = _lm_config_combiner_impl,
    doc = "Combines the generated .bin files into a single etc directory.",
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            mandatory = True,
            doc = "List of .bin files to copy into the output directory.",
        ),
        "dir_name": attr.string(
            default = "etc",
            doc = "Name of the directory to place the combined config files into.",
        ),
    },
)

def launch_manager_config(
        name,
        config,
        schema = Label("//score/launch_manager/src/daemon/src/configuration/config_schema:launch_manager.schema.json"),
        script = Label("//scripts/config_mapping:lifecycle_config"),
        flatbuffer_out_dir = "flatbuffer_out",
        lm_schema = Label("//score/launch_manager/src/daemon/src/configuration:new_lm_flatcfg_fbs"),
        **kwargs):
    gen_name = name + "_gen"
    lm_config_generator(
        name = gen_name,
        config = config,
        schema = schema,
        script = script,
    )

    config_basename = Label(config).name
    dot_index = config_basename.rfind(".")
    config_stem = config_basename[:dot_index] if dot_index != -1 else config_basename

    serialize_buffer(
        name = name + "_lm_bin",
        data = ":" + gen_name,
        schema = lm_schema,
        output = name + "_serialized/" + config_stem + ".bin",
        **kwargs
    )

    # note that the combining is a workaround as we have to return
    # the etc directory where the .bin file is for backwards compatibility.
    lm_config_combiner(
        name = name,
        srcs = [":" + name + "_lm_bin"],
        dir_name = flatbuffer_out_dir,
    )
