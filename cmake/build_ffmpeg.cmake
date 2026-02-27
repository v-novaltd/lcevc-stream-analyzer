include(ExternalProject)

set(FFMPEG_URL "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz")

set(FFMPEG_PREFIX "${CMAKE_BINARY_DIR}/ffmpeg")
set(FFMPEG_SOURCE_DIR "${FFMPEG_PREFIX}/source")
set(FFMPEG_BINARY_DIR "${FFMPEG_PREFIX}/build")
set(FFMPEG_STAMP_DIR "${FFMPEG_PREFIX}/stamp")
set(FFMPEG_TMP_DIR "${FFMPEG_PREFIX}/tmp")
set(FFMPEG_DL_DIR "${FFMPEG_PREFIX}/download")

set(FFMPEG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")

set(FFMPEG_INSTALL_LIB_DIR "${FFMPEG_INSTALL_DIR}/lib")
file(MAKE_DIRECTORY ${FFMPEG_INSTALL_LIB_DIR})

set(FFMPEG_INSTALL_INCLUDE_DIR "${FFMPEG_INSTALL_DIR}/include")
file(MAKE_DIRECTORY ${FFMPEG_INSTALL_INCLUDE_DIR})

set(FFMPEG_INSTALL_BIN_DIR "${FFMPEG_INSTALL_DIR}/bin")
file(MAKE_DIRECTORY ${FFMPEG_INSTALL_BIN_DIR})

ExternalProject_Add(
  ffmpeg_external
  URL "${FFMPEG_URL}"
  PREFIX "${FFMPEG_PREFIX}"
  DOWNLOAD_DIR "${FFMPEG_DL_DIR}"
  SOURCE_DIR "${FFMPEG_SOURCE_DIR}"
  BINARY_DIR "${FFMPEG_BINARY_DIR}"
  STAMP_DIR "${FFMPEG_STAMP_DIR}"
  TMP_DIR "${FFMPEG_TMP_DIR}"
  DOWNLOAD_EXTRACT_TIMESTAMP NEW
  # ExternalProject runs CONFIGURE/BUILD/INSTALL in BINARY_DIR by default
  CONFIGURE_COMMAND
    "bash" "${FFMPEG_SOURCE_DIR}/configure" --prefix="${FFMPEG_INSTALL_DIR}" --enable-shared
    --disable-static --disable-doc --enable-pic --extra-cflags=-fPIC --extra-ldflags=-fPIC
    --disable-asm --disable-x86asm --disable-openssl --disable-securetransport --disable-gnutls
    --disable-libsrt --disable-librtmp
  BUILD_COMMAND "${CMAKE_MAKE_PROGRAM}" -j8
  INSTALL_COMMAND "${CMAKE_MAKE_PROGRAM}" install)

function(ADD_FFMPEG_LIB TARGET BASE_LIB)
  add_library(${TARGET} SHARED IMPORTED GLOBAL)
  add_dependencies(${TARGET} ffmpeg_external)
  set_target_properties(
    ${TARGET}
    PROPERTIES
      IMPORTED_LOCATION
      "${FFMPEG_INSTALL_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${BASE_LIB}${CMAKE_SHARED_LIBRARY_SUFFIX}"
      IMPORTED_IMPLIB "${FFMPEG_INSTALL_LIB_DIR}/${BASE_LIB}.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INSTALL_INCLUDE_DIR}")
endfunction()

add_ffmpeg_lib(FFmpeg::avutil avutil)
add_ffmpeg_lib(FFmpeg::swresample swresample)
add_ffmpeg_lib(FFmpeg::swscale swscale)
add_ffmpeg_lib(FFmpeg::avcodec avcodec)
add_ffmpeg_lib(FFmpeg::avformat avformat)
add_ffmpeg_lib(FFmpeg::avfilter avfilter)
add_ffmpeg_lib(FFmpeg::avdevice avdevice)
add_ffmpeg_lib(FFmpeg::postproc postproc)
