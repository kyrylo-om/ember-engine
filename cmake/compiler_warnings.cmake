# Enables the project warning set on a compiled target.
# Every Ember library, test and example calls this; third-party targets do not.
function(ember_enable_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wold-style-cast
        -Wnon-virtual-dtor
        -Woverloaded-virtual
    )
    if(EMBER_WARNINGS_AS_ERRORS)
        target_compile_options(${target} PRIVATE -Werror)
    endif()
endfunction()
