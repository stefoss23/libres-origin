set( source_files rms_file.c rms_util.c rms_tag.c rms_type.c rms_tagkey.c rms_stats.c rms_export.c)
set( header_files rms_file.h rms_util.h rms_tag.h rms_type.h rms_tagkey.h rms_stats.h rms_export.h)

add_library( rms ${LIBRARY_TYPE} ${source_files} )
set_target_properties( rms PROPERTIES VERSION ${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR} SOVERSION ${ERT_VERSION_MAJOR} )
target_link_libraries( rms ${ECL_LIBRARY} )
if (USE_RUNPATH)
   add_runpath( rms )
endif()   


#-----------------------------------------------------------------

install(TARGETS rms DESTINATION ${CMAKE_INSTALL_LIBDIR})
foreach(header ${header_files})
    install(FILES ../include/ert/rms/${header} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ert/rms)
endforeach()

