project(ShewchukPredicates C)

# Vendored, unmodified predicates.c from libigl-predicates (see README.md).
# The code relies on exact IEEE-754 double arithmetic. Fused multiply-add contraction would change the
# results of the error-free transformations, so it is explicitly disabled. Never use fast-math here.

add_library(${PROJECT_NAME} STATIC predicates.c predicates.h)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The original code uses K&R style function definitions, which are not valid in C23
set_target_properties(
  ${PROJECT_NAME} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /fp:precise /W0)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -ffp-contract=off -w)
endif()

# Second build of predicates.c with FMA contraction enabled and renamed symbols. Only used by unit tests to
# verify that the exact signs are identical with and without contraction (see cvfRobustPredicates-Test.cpp).
# Not part of the default build, it is built when a test links to it.
add_library(ShewchukPredicatesFmaProbe STATIC EXCLUDE_FROM_ALL predicates.c)
set_target_properties(ShewchukPredicatesFmaProbe PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
target_compile_definitions(
  ShewchukPredicatesFmaProbe
  PRIVATE exactinit=exactinit_contracted
          orient2d=orient2d_contracted
          orient3d=orient3d_contracted
          incircle=incircle_contracted
          insphere=insphere_contracted
)

if(MSVC)
  target_compile_options(ShewchukPredicatesFmaProbe PRIVATE /fp:contract /W0)
  target_compile_definitions(ShewchukPredicatesFmaProbe PRIVATE _CRT_SECURE_NO_WARNINGS)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
    target_compile_options(ShewchukPredicatesFmaProbe PRIVATE /arch:AVX2)
  endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(ShewchukPredicatesFmaProbe PRIVATE -ffp-contract=fast -w)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|x86")
    target_compile_options(ShewchukPredicatesFmaProbe PRIVATE -mfma)
  endif()
endif()
