cmake_minimum_required(VERSION 3.24)
project(ArtilodyPlugweaver VERSION 0.3.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# No runtime files next to the app or the plugins: the MSVC runtime is linked
# statically on Windows, libstdc++/libgcc statically on Linux.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_link_options(-static-libstdc++ -static-libgcc)
endif()
include(FetchContent)
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    # JUCE 7.0.12's juceaide bootstrap needs an explicit target, even with an existing helper cache.
    list(APPEND extra_compiler_flag_arguments "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()
set(JUCE_PATH "" CACHE PATH "Optional JUCE 7.0.12 checkout")
if(JUCE_PATH)
    file(STRINGS "${JUCE_PATH}/CMakeLists.txt" juce_version_line REGEX "^project\\(JUCE VERSION 7\\.0\\.12 ")
    if(NOT juce_version_line)
        message(FATAL_ERROR "Use JUCE 7.0.12 for this GPLv3 project. JUCE 8 is not a drop-in licence replacement.")
    endif()
    add_subdirectory("${JUCE_PATH}" juce)
    set(juce_SOURCE_DIR "${JUCE_PATH}")
else()
    FetchContent_Declare(JUCE URL https://codeload.github.com/juce-framework/JUCE/tar.gz/refs/tags/7.0.12
        URL_HASH SHA256=94e3b35e1990cd67f59736bb5e1eff1cfd9590b16fea82696f309a8f59452434 DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()
file(GLOB_RECURSE engine_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/EngineTemplate/*")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${engine_files})
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${CMAKE_CURRENT_BINARY_DIR}/engine_template.zip" --format=zip
    CMakeLists.txt Source Build-Mac.command Build-Linux.sh Build-Windows.bat .github .gitignore LICENSE JUCE-LICENSE.md CLAP-EXTENSIONS-LICENSE.md CLAP-LICENSE.txt CLAP-HELPERS-LICENSE.txt README.md
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/EngineTemplate" COMMAND_ERROR_IS_FATAL ANY)

# ---- Prebuilt instrument engine ---------------------------------------------------------
# One finished plugin per format. Plugweaver copies it on export and attaches the
# instrument's identity and data (EngineTemplate/Source/PwPayload.h), so building an
# instrument needs no compiler, CMake or internet on the user's machine.
set(CLAP_PATH "" CACHE PATH "Optional clap-juce-extensions checkout")
if(CLAP_PATH)
    add_subdirectory("${CLAP_PATH}" clap_ext)
else()
    FetchContent_Declare(clap_ext GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_MakeAvailable(clap_ext)
endif()
set(engine_formats VST3)
set(engine_items VST3 CLAP)
set(engine_targets PWEngine_VST3 PWEngine_CLAP)
if(APPLE)
    list(APPEND engine_formats AU)
    list(APPEND engine_items AU)
    list(APPEND engine_targets PWEngine_AU)
endif()
juce_add_plugin(PWEngine VERSION 1.0.0 PRODUCT_NAME "Plugweaver Engine" COMPANY_NAME Artilody
    BUNDLE_ID org.artilody.engine PLUGIN_MANUFACTURER_CODE Arti PLUGIN_CODE Pwen
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Instrument Sampler FORMATS ${engine_formats})
target_sources(PWEngine PRIVATE EngineTemplate/Source/Plugin.cpp EngineTemplate/Source/PwIdentity.cpp)
target_compile_definitions(PWEngine PUBLIC PW_PREBUILT_ENGINE=1
    JUCE_APP_CONFIG_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/EngineTemplate/Source/PwIdentity.h"
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
target_link_libraries(PWEngine PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(PWEngine PRIVATE ${CMAKE_DL_LIBS})
endif()
clap_juce_extensions_plugin(TARGET PWEngine CLAP_ID "org.artilody.engine" CLAP_FEATURES instrument sampler stereo)
set(engine_zip "${CMAKE_CURRENT_BINARY_DIR}/engine.zip")
add_custom_command(OUTPUT "${engine_zip}"
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${engine_zip}"
    COMMAND "${CMAKE_COMMAND}" -E chdir "$<TARGET_FILE_DIR:PWEngine>" "${CMAKE_COMMAND}" -E tar cf "${engine_zip}" --format=zip ${engine_items}
    DEPENDS ${engine_targets}
    COMMENT "Packing the prebuilt instrument engine"
    VERBATIM)
add_custom_target(PWEngineArchive DEPENDS "${engine_zip}")

juce_add_binary_data(BuilderAssets NAMESPACE BuilderAssets HEADER_NAME BuilderAssets.h SOURCES "${CMAKE_CURRENT_BINARY_DIR}/engine_template.zip" Assets/logo.png)
add_library(PWRuntime STATIC Source/Runtime.cpp Source/Install.cpp)
target_link_libraries(PWRuntime PRIVATE juce::juce_audio_utils juce::juce_gui_extra juce::juce_dsp juce::juce_recommended_config_flags)
if(WIN32)
    target_link_libraries(PWRuntime PUBLIC shell32)
endif()
target_include_directories(PWRuntime PUBLIC "${juce_SOURCE_DIR}/modules" EngineTemplate/Source Source)
target_compile_definitions(PWRuntime PUBLIC JUCE_PLUGINHOST_VST3=1 PW_CMAKE_PATH="${CMAKE_COMMAND}" JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_STANDALONE_APPLICATION=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
juce_add_gui_app(Plugweaver PRODUCT_NAME "Artilody Plugweaver" COMPANY_NAME Artilody BUNDLE_ID org.artilody.plugweaver ICON_BIG Assets/icon.png)
target_sources(Plugweaver PRIVATE Source/Main.cpp Source/EngineBlob.cpp)
target_link_libraries(Plugweaver PRIVATE PWRuntime BuilderAssets)
add_dependencies(Plugweaver PWEngineArchive)
target_compile_definitions(Plugweaver PRIVATE PW_APP_VERSION="${PROJECT_VERSION}")
# Embed the engine archive in the app itself.
if(APPLE)
    target_sources(Plugweaver PRIVATE "${engine_zip}")
    set_source_files_properties("${engine_zip}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
elseif(WIN32)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/engine.rc" "PWENGINE RCDATA \"${engine_zip}\"\n")
    target_sources(Plugweaver PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/engine.rc")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/engine.rc" PROPERTIES OBJECT_DEPENDS "${engine_zip}")
else()
    set_source_files_properties(Source/EngineBlob.cpp PROPERTIES OBJECT_DEPENDS "${engine_zip}")
    target_compile_definitions(Plugweaver PRIVATE PW_ENGINE_ZIP="${engine_zip}")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set_source_files_properties(Source/Main.cpp PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic")
endif()
include(CTest)
if(BUILD_TESTING)
    juce_add_console_app(PWTests PRODUCT_NAME "Plugweaver tests")
    target_sources(PWTests PRIVATE Tests/Test.cpp)
    target_link_libraries(PWTests PRIVATE PWRuntime BuilderAssets)
    target_compile_definitions(PWTests PRIVATE PW_ENGINE_ZIP="${engine_zip}")
    add_dependencies(PWTests PWEngineArchive)
    add_test(NAME PlugweaverRoundtrip COMMAND PWTests "${CMAKE_CURRENT_BINARY_DIR}/test-output")
endif()
option(PW_BUILD_VISUAL_TEST "Build optional offscreen UI renderer" OFF)
if(PW_BUILD_VISUAL_TEST)
    juce_add_console_app(PWVisual PRODUCT_NAME "Plugweaver visual test")
    target_sources(PWVisual PRIVATE Tests/Visual.cpp Source/EngineBlob.cpp)
    target_link_libraries(PWVisual PRIVATE PWRuntime BuilderAssets)
    target_compile_definitions(PWVisual PRIVATE PW_TESTING_VIEW=1 PW_ENGINE_ZIP="${engine_zip}" PW_APP_VERSION="${PROJECT_VERSION}")
    add_dependencies(PWVisual PWEngineArchive)
endif()
