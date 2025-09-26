# Helper functions for third-party dependencies

def homebrew_library(
    name,
    brew,
    homebrew_header_path = "include",
    exported_preprocessor_flags = [],
    exported_linker_flags = [],
    deps = [],
    visibility = ["PUBLIC"]):

    # For now, use hardcoded Homebrew paths for macOS
    # TODO: Make this more portable
    native.prebuilt_cxx_library(
        name = name,
        exported_preprocessor_flags = exported_preprocessor_flags + [
            "-I/opt/homebrew/include",
            "-I/opt/homebrew/Cellar/{}/*/include".format(brew),
        ],
        exported_linker_flags = exported_linker_flags + [
            "-L/opt/homebrew/lib",
        ],
        deps = deps,
        visibility = visibility,
    )