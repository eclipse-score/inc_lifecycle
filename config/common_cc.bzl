load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//config:common_opts.bzl", "SANITIZER_CXXOPTS", "SANITIZER_LINKOPTS")

_def_or_empty = lambda v: v if v else []

def cc_library_with_common_opts(
        name,
        srcs = None,
        hdrs = None,
        deps = None,
        cxxopts = None,
        linkopts = None,
        omit_common_cxxopts = False,
        omit_common_linkopts = False,
        **kwargs):
    base_cxx = [] if omit_common_cxxopts else SANITIZER_CXXOPTS
    base_link = [] if omit_common_linkopts else SANITIZER_LINKOPTS
    cc_library(
        name = name,
        srcs = _def_or_empty(srcs),
        hdrs = _def_or_empty(hdrs),
        deps = _def_or_empty(deps),
        cxxopts = base_cxx + _def_or_empty(cxxopts),
        linkopts = base_link + _def_or_empty(linkopts),
        **kwargs
    )

def cc_binary_with_common_opts(
        name,
        srcs = None,
        deps = None,
        cxxopts = None,
        linkopts = None,
        omit_common_cxxopts = False,
        omit_common_linkopts = False,
        **kwargs):
    base_cxx = [] if omit_common_cxxopts else SANITIZER_CXXOPTS
    base_link = [] if omit_common_linkopts else SANITIZER_LINKOPTS
    cc_binary(
        name = name,
        srcs = _def_or_empty(srcs),
        deps = _def_or_empty(deps),
        cxxopts = base_cxx + _def_or_empty(cxxopts),
        linkopts = base_link + _def_or_empty(linkopts),
        **kwargs
    )
