# Finde alle Testdateien
file(GLOB TEST_SOURCES "test_*.c")

file(GLOB BENCH_SOURCES "bench_*.c")


# Durch jede gefundene Testdatei loopen
foreach(TEST_SOURCE_FILE ${TEST_SOURCES})
    # Den Namen für das Executable aus dem Dateinamen ableiten
    get_filename_component(TEST_NAME ${TEST_SOURCE_FILE} NAME_WE)

    # Erstelle das Test-Executable und linke es gegen die Hauptbibliothek.
    add_executable(${TEST_NAME} ${TEST_SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE str8)

    # Den Test zu CTest hinzufügen, damit er mit `ctest` ausgeführt werden kann
    add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)
endforeach()

foreach(BENCH_SOURCE_FILE ${BENCH_SOURCES})
    get_filename_component(BENCH_NAME ${BENCH_SOURCE_FILE} NAME_WE)

    add_executable(${BENCH_NAME} ${BENCH_SOURCE_FILE})
    target_link_libraries(${BENCH_NAME} PRIVATE str8)

    # Configure benchmark executables for high optimization, no sanitizers, no debug info, and without -DDEBUG
    # This overrides default build type settings for these specific targets.
    target_compile_options(${BENCH_NAME} PRIVATE -O3)

    # Override configuration-specific compile flags to ensure -O3 are always used
    # and to effectively remove other flags like -g (debug info) or -fsanitize (sanitizers).
    set_property(TARGET ${BENCH_NAME} PROPERTY CMAKE_C_FLAGS_DEBUG "-O3")
    set_property(TARGET ${BENCH_NAME} PROPERTY CMAKE_C_FLAGS_RELEASE "-O3")
    set_property(TARGET ${BENCH_NAME} PROPERTY CMAKE_C_FLAGS_RELWITHDEBINFO "-O3")
    set_property(TARGET ${BENCH_NAME} PROPERTY CMAKE_C_FLAGS_MINSIZEREL "-O3")

    # Strip all symbols from the executable to ensure no debug information is present
    target_link_options(${BENCH_NAME} PRIVATE -s)

endforeach()
