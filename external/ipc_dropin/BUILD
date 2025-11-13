cc_library(
    name = "ipc_dropin_internal",
    hdrs = glob(["include/ipc_dropin/*.hpp"]),
    includes = ["include"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

alias(
    name = "ipc_dropin",
    actual = ":ipc_dropin_internal",
    visibility = ["//visibility:public"],
)
