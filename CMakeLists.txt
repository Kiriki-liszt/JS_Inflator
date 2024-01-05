cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")


if(SMTG_MAC)
    set(vst3sdk_SOURCE_DIR "/Users/yoon/VST_SDK/vst3sdk")
elseif(SMTG_WIN)
    set(vst3sdk_SOURCE_DIR "C:/VST_SDK/vst3sdk")
endif(SMTG_MAC)

if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()


project(InflatorPackage
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 1.7.0.0
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
    # ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vst2wrapper/vst2wrapper.sdk.cpp
)

#set(ORIGINAL_GUI 1)
set(TWARCH 1)


if(ORIGINAL_GUI)
    add_compile_definitions(_ORIG)
elseif(TWARCH)
    add_compile_definitions(_TWARCH)
endif()

#- VSTGUI Wanted ----
if(SMTG_ENABLE_VSTGUI_SUPPORT)
    if(ORIGINAL_GUI)
    target_sources(InflatorPackage
        PRIVATE
            resource/InflatorPackageeditor.uidesc
    )
    smtg_target_add_plugin_resources(InflatorPackage
        RESOURCES
            "resource/InflatorPackageeditor.uidesc"
            "resource/background.png"
            "resource/button_onoff.png"
            "resource/handle.png"
            "resource/knob_green_.png"
            "resource/knob_red_.png"
            "resource/LED_cover.png"
            "resource/LED_ON.png"
            "resource/LED_OFF.png"
            "resource/LED_FX_ON.png"
            "resource/LED_FX_OFF.png"
            "resource/OS_text.png"
            "resource/oversample_bg.png"
            "resource/oversample_handle.png"
    )
    elseif(TWARCH)
    target_sources(InflatorPackage
        PRIVATE
            resource/twarch/InflatorPackageeditor.uidesc
    )
    smtg_target_add_plugin_resources(InflatorPackage
        RESOURCES
            "resource/twarch/InflatorPackageeditor.uidesc"
            "resource/twarch/background.png"
            "resource/twarch/btn_CLIP.png"
            "resource/twarch/btn_IN.png"
            "resource/twarch/btn_SPLIT.png"
            "resource/twarch/handle.png"
            "resource/twarch/knob_red_.png"
            "resource/twarch/knob_yellow_.png"
            "resource/twarch/meter_off.png"
            "resource/twarch/meter_on.png"
            "resource/twarch/meter_fx_on.png"
    )
    endif()
    target_link_libraries(InflatorPackage
        PRIVATE
            vstgui_support
    )
endif(SMTG_ENABLE_VSTGUI_SUPPORT)
# -------------------

smtg_target_add_plugin_snapshots (InflatorPackage
    RESOURCES
        resource/A6B5CA9F4C4F5B9388F837774504BD37_snapshot.png
        resource/A6B5CA9F4C4F5B9388F837774504BD37_snapshot_2.0x.png
)

target_link_libraries(InflatorPackage
    PRIVATE
        sdk
)

set(SIMDE_PATH "${PROJECT_BINARY_DIR}/../libs/simde" CACHE STRING "Path to simde library source tree")
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE ${SIMDE_PATH})
target_link_libraries(InflatorPackage
    PRIVATE 
        simde
)

set(R8B_PATH "${PROJECT_BINARY_DIR}/../libs/r8brain-free-src" CACHE STRING "Path to r8brain-free-src library source tree")

file(WRITE "${R8B_PATH}/CMakeLists.txt"
           [=[
project(r8brain-free-src)
add_library(r8brain-free-src r8bbase.cpp)
]=]
)
    
add_subdirectory(${R8B_PATH} ${PROJECT_BINARY_DIR}/r8brain-free-src-build EXCLUDE_FROM_ALL)
include_directories(${R8B_PATH})
target_link_libraries(InflatorPackage PUBLIC r8brain-free-src)
smtg_target_setup_universal_binary(r8brain-free-src)

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

# Add an AUv2 target
if (SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH)
	smtg_target_add_auv2(InflatorPackage-au
		BUNDLE_NAME InflatorPackage
		BUNDLE_IDENTIFIER io.github.vst3.Inflator.audiounit
		INFO_PLIST_TEMPLATE resource/au-info.plist
		VST3_PLUGIN_TARGET InflatorPackage)
endif(SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH)
