function(add_structstore_binding binding_target binding_srcs)
    nanobind_add_module(${binding_target} ${binding_srcs} NB_STATIC)
    target_link_libraries(${binding_target} PUBLIC structstore_py_lib)
    set_target_properties(${binding_target} PROPERTIES
            LIBRARY_OUTPUT_NAME _${binding_target})
    target_compile_definitions(${binding_target} PRIVATE
            MODULE_NAME=_${binding_target}
    )

    # hide all symbols by default (including external libraries on Linux)
    set_target_properties(${binding_target} PROPERTIES
            CXX_VISIBILITY_PRESET "hidden"
            VISIBILITY_INLINES_HIDDEN true)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        target_link_options(${binding_target} PRIVATE "LINKER:--exclude-libs,ALL")
    endif()

    # mark nanobind headers as system includes to suppress warnings
    get_target_property(NB_INC_DIRS nanobind-static INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${binding_target} SYSTEM PRIVATE ${NB_INC_DIRS})

    if(NOT ${BUILD_WITH_SANITIZER})
        nanobind_stubgen(${binding_target})
    endif()
endfunction()