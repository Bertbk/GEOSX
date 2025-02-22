# Specify list of tests
set( gtest_geosx_tests
     testFunctions.cpp )


set( dependencyList ${parallelDeps} gtest functions )

if (TARGET pugixml::pugixml)
  list( APPEND dependencyList pugixml::pugixml )
endif()

if (TARGET pugixml)
  list( APPEND dependencyList pugixml )
endif()

if (TARGET fmt::fmt-header-only)
  list( APPEND dependencyList fmt::fmt-header-only )
endif()

if (TARGET fmt)
  list( APPEND dependencyList fmt )
endif()

# Add gtest C++ based tests
foreach(test ${gtest_geosx_tests})
    get_filename_component( test_name ${test} NAME_WE )
    blt_add_executable( NAME ${test_name}
                        SOURCES ${test}
                        OUTPUT_DIR ${TEST_OUTPUT_DIRECTORY}
                        DEPENDS_ON ${dependencyList} )

    geos_add_test( NAME ${test_name}
                   COMMAND ${test_name} )

endforeach()
