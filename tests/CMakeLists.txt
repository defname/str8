# Finde alle Testdateien
file(GLOB TEST_SOURCES "test_*.c" "bench_*.c")

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