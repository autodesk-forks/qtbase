# List of files that need to be packaged as resources.
# This file exists solely because of unit tests that need access to this
# information as well. This was previosly handled by referrencing a qrc
# file with the same information

set(corelib_mimetypes_resource_file
    "${CMAKE_CURRENT_LIST_DIR}/mime/packages/freedesktop.org.xml"
)

function(corelib_add_mimetypes_resources target)
    set(source_file "${corelib_mimetypes_resource_file}")
    set_source_files_properties("${source_file}"
        PROPERTIES alias "freedesktop.org.xml"
    )
    add_qt_resource(${target} "mimetypes"
        PREFIX
            "/qt-project.org/qmime/packages"
        FILES
            "${source_file}"
    )
endfunction()
