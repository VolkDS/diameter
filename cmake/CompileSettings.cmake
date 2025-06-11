function(default_cxx_compile_settings_default TARGET)
    set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    target_compile_options(${TARGET} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
    target_compile_options(${TARGET}
        PRIVATE $<$<CONFIG:Debug>:-g3 -O0 -fno-inline -D_DEBUG>
        PRIVATE $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endfunction()

function(default_cxx_compile_settings_warnings TARGET)
    target_compile_options(${TARGET} PRIVATE
        -Wold-style-cast
        -Wcast-align
        -Wcast-qual
        -Wconversion
        -Wduplicated-branches
        -Wduplicated-cond
        -Wlogical-op
        -Wredundant-decls
        -Wsign-conversion
        -Wsign-promo
        -Wctor-dtor-privacy
        -Wnon-virtual-dtor
        -Woverloaded-virtual
    )
endfunction()
