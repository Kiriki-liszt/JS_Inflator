cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

set(vst3sdk_SOURCE_DIR "C:/VST_make/VST_SDK/vst3sdk")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(InflatorPackage
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 1.0.0.0 
    DESCRIPTION "InflatorPackage VST 3 Plug-in"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(InflatorPackage
    source/version.h
    source/InflatorPackagecids.h
    source/InflatorPackageprocessor.h
    source/InflatorPackageprocessor.cpp
    source/InflatorPackagecontroller.h
    source/InflatorPackagecontroller.cpp
    source/InflatorPackageentry.cpp
    source/param.h
    resource/InflatorPackageeditor.rc
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vst2wrapper/vst2wrapper.sdk.cpp
    source/InflatorPackageentry_vst2.cpp
)

#- VSTGUI Wanted ----
if(SMTG_ADD_VSTGUI)
    target_sources(InflatorPackage
        PRIVATE
            resource/InflatorPackageeditor.uidesc
    )
    target_link_libraries(InflatorPackage
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(InflatorPackage
        RESOURCES
            "resource/InflatorPackageeditor.uidesc"
    )
endif(SMTG_ADD_VSTGUI)
# -------------------
smtg_target_add_plugin_resources(InflatorPackage
    RESOURCES
        resource/InflatorPackageeditor.uidesc
        resource/background.png
        resource/handle.png
        resource/knob1.png
        resource/knob2.png
)
smtg_target_add_plugin_snapshots (InflatorPackage
    RESOURCES
        resource/A6B5CA9F4C4F5B9388F837774504BD37_snapshot.png
        resource/A6B5CA9F4C4F5B9388F837774504BD37_snapshot_2.0x.png
)

target_link_libraries(InflatorPackage
    PRIVATE
        sdk
        vstgui_support
)

smtg_target_configure_version_file(InflatorPackage)

if(SMTG_MAC)
    smtg_target_set_bundle(InflatorPackage
        BUNDLE_IDENTIFIER io.github.vst3.Inflator
        COMPANY_NAME "yg331"
    )
    smtg_target_set_debug_executable(InflatorPackage
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(InflatorPackage PRIVATE 
        resource/win32resource.rc
        resource/InflatorPackageeditor.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT InflatorPackage)

        smtg_target_set_debug_executable(InflatorPackage
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)


if(SMTG_WIN)
    if(NOT MINGW)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif(NOT MINGW)
endif(SMTG_WIN)