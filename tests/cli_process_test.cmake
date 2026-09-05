if(NOT DEFINED RIVA_EXECUTABLE OR NOT EXISTS "${RIVA_EXECUTABLE}")
  message(FATAL_ERROR "RIVA_EXECUTABLE must identify the built CLI")
endif()
if(NOT DEFINED RIVA_SOURCE_DIR OR NOT IS_DIRECTORY "${RIVA_SOURCE_DIR}")
  message(FATAL_ERROR "RIVA_SOURCE_DIR must identify the repository root")
endif()

set(shader_trace "${RIVA_SOURCE_DIR}/samples/spike_shader_compile.json")
set(cpu_trace "${RIVA_SOURCE_DIR}/samples/spike_cpu_game_thread.json")
set(report_path "${CMAKE_CURRENT_BINARY_DIR}/cli-process-report.json")
set(empty_budget "${CMAKE_CURRENT_BINARY_DIR}/empty-budget.json")

execute_process(
  COMMAND "${RIVA_EXECUTABLE}" analyze "${shader_trace}" --format json --output "${report_path}"
  RESULT_VARIABLE analyze_result
  ERROR_VARIABLE analyze_error
)
if(NOT analyze_result EQUAL 0)
  message(FATAL_ERROR "analyze failed with ${analyze_result}: ${analyze_error}")
endif()
if(NOT EXISTS "${report_path}")
  message(FATAL_ERROR "analyze did not create its requested report")
endif()
file(SIZE "${report_path}" report_size)
if(report_size EQUAL 0)
  message(FATAL_ERROR "analyze created an empty report")
endif()

execute_process(
  COMMAND "${RIVA_EXECUTABLE}" compare "${shader_trace}" "${shader_trace}"
  RESULT_VARIABLE identical_result
  ERROR_VARIABLE identical_error
)
if(NOT identical_result EQUAL 0)
  message(FATAL_ERROR "identical trace comparison failed with ${identical_result}: ${identical_error}")
endif()

execute_process(
  COMMAND "${RIVA_EXECUTABLE}" compare "${cpu_trace}" "${shader_trace}"
  RESULT_VARIABLE regression_result
  OUTPUT_VARIABLE regression_output
  ERROR_VARIABLE regression_error
)
if(NOT regression_result EQUAL 3)
  message(FATAL_ERROR
    "regression comparison returned ${regression_result}, expected 3\n"
    "stdout: ${regression_output}\nstderr: ${regression_error}")
endif()

file(WRITE "${empty_budget}" "{}\n")
execute_process(
  COMMAND "${RIVA_EXECUTABLE}" check-budget --budget "${empty_budget}" --trace "${shader_trace}"
  RESULT_VARIABLE empty_budget_result
  OUTPUT_VARIABLE empty_budget_output
  ERROR_VARIABLE empty_budget_error
)
if(NOT empty_budget_result EQUAL 1)
  message(FATAL_ERROR
    "empty budget returned ${empty_budget_result}, expected 1\n"
    "stdout: ${empty_budget_output}\nstderr: ${empty_budget_error}")
endif()

file(REMOVE "${report_path}" "${empty_budget}")
