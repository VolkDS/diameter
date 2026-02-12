set(APP_NAME diameter_manager)

set(APP_SRC
    main.cpp
)

add_executable(${APP_NAME} ${APP_SRC})

default_cxx_compile_settings_default(${APP_NAME})
default_cxx_compile_settings_warnings(${APP_NAME})

target_link_libraries(${APP_NAME} PRIVATE
    Boost::boost
    diameter::diameter
)