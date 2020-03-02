licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "well_known_names",
    hdrs = ["well_known_names.h"],
    deps = [
        "//source/common/singleton:const_singleton",
    ],
)

envoy_cc_library(
    name = "redis_command_stats_lib",
    srcs = ["redis_command_stats.cc"],
    hdrs = ["redis_command_stats.h"],
    deps = [
        "//source/extensions/filters/network/common/redis:codec_interface", # TODO: Move the related functions out of here!
        "//source/extensions/filters/network/common/redis:supported_commands_lib",
        "//include/envoy/stats:stats_interface",
        "//include/envoy/stats:timespan_interface",
        "//include/envoy/upstream:upstream_interface",
        "//source/common/common:utility_lib",
        "//source/common/stats:symbol_table_lib",
        "//source/common/stats:timespan_lib",
    ],
)
