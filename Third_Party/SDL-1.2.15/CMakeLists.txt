file(GLOB SDL_SRC_VIDEO_DUMMY src/video/dummy/*.c)
cmake_minimum_required(VERSION 3.10)
project(SDL12 C)

# Add include directory for SDL headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


# Gather all SDL source files, including platform-specific files for Windows


# Gather all SDL source files using file(GLOB ...) for each directory
file(GLOB SDL_SRC_MAIN src/*.c)
file(GLOB SDL_SRC_AUDIO src/audio/*.c)
file(GLOB SDL_SRC_CDROM src/cdrom/*.c)
file(GLOB SDL_SRC_CPUINFO src/cpuinfo/*.c)
file(GLOB SDL_SRC_EVENTS src/events/*.c)
file(GLOB SDL_SRC_FILE src/file/*.c)
file(GLOB SDL_SRC_HERMES src/hermes/*.c)
file(GLOB SDL_SRC_JOYSTICK src/joystick/*.c)
file(GLOB SDL_SRC_LOADSO src/loadso/*.c)
file(GLOB SDL_SRC_STDLIB src/stdlib/*.c)
file(GLOB SDL_SRC_THREAD src/thread/*.c)
file(GLOB SDL_SRC_THREAD_WIN32 src/thread/win32/*.c)
file(GLOB SDL_SRC_AUDIO_WINDX5 src/audio/windx5/*.c)
file(GLOB SDL_SRC_AUDIO_WINDIB src/audio/windib/*.c)
file(GLOB SDL_SRC_AUDIO_DISK src/audio/disk/*.c)
file(GLOB SDL_SRC_AUDIO_DUMMY src/audio/dummy/*.c)
file(GLOB SDL_SRC_TIMER src/timer/*.c)
file(GLOB SDL_SRC_VIDEO src/video/*.c)
file(GLOB SDL_SRC_VIDEO_WINDIB src/video/windib/*.c)
file(GLOB SDL_SRC_VIDEO_WINCOMMON src/video/wincommon/*.c)
file(GLOB SDL_SRC_VIDEO_WINDX5 src/video/windx5/*.c)
file(GLOB SDL_SRC_JOYSTICK_WIN32 src/joystick/win32/*.c)
file(GLOB SDL_SRC_CDROM_WIN32 src/cdrom/win32/*.c)
file(GLOB SDL_SRC_TIMER_WIN32 src/timer/win32/*.c)

# Combine all sources into SDL_SRC
set(SDL_SRC
    ${SDL_SRC_MAIN}
    ${SDL_SRC_AUDIO}
    ${SDL_SRC_CDROM}
    ${SDL_SRC_CPUINFO}
    ${SDL_SRC_EVENTS}
    ${SDL_SRC_FILE}
    ${SDL_SRC_HERMES}
    ${SDL_SRC_JOYSTICK}
    ${SDL_SRC_LOADSO}
    ${SDL_SRC_STDLIB}
    ${SDL_SRC_THREAD}
    ${SDL_SRC_THREAD_WIN32}
    ${SDL_SRC_AUDIO_WINDX5}
    ${SDL_SRC_AUDIO_WINDIB}
    ${SDL_SRC_AUDIO_DISK}
    ${SDL_SRC_AUDIO_DUMMY}
    ${SDL_SRC_TIMER}
    ${SDL_SRC_VIDEO}
    ${SDL_SRC_VIDEO_WINDIB}
    ${SDL_SRC_VIDEO_DUMMY}
    ${SDL_SRC_VIDEO_WINCOMMON}
    ${SDL_SRC_VIDEO_WINDX5}
    ${SDL_SRC_JOYSTICK_WIN32}
    ${SDL_SRC_CDROM_WIN32}
    ${SDL_SRC_TIMER_WIN32}
)

# Add SDLmain (Win32 main entry)
set(SDLMAIN_SRC src/main/win32/SDL_win32_main.c)

add_library(SDL STATIC ${SDL_SRC})
add_library(SDLmain STATIC ${SDLMAIN_SRC})

# Link SDL against required Windows system libraries
target_link_libraries(SDL
    winmm
    dxguid
    dinput8
    ddraw
    user32
    gdi32
    ole32
    oleaut32
    uuid
    advapi32
    imm32
    version
    setupapi
)

# Set output directory for built libraries
set_target_properties(SDL SDLmain PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)
