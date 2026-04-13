cmake_minimum_required(VERSION 3.16)

project(TransientTracker LANGUAGES CXX)

include(CTest)

list(APPEND CMAKE_PREFIX_PATH
    "/opt/homebrew/opt/opencv"
    "/opt/homebrew/opt/xtensor"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc CONFIG)
find_package(xtensor REQUIRED CONFIG)

add_library(transient_tracker_synthetic
    src/synthetic/dataset_layout.cpp
    src/synthetic/frame_renderer.cpp
    src/synthetic/frame_shift_generator.cpp
    src/synthetic/label_writer.cpp
    src/synthetic/moving_object_generator.cpp
    src/synthetic/noise_model.cpp
    src/synthetic/preview_writer.cpp
    src/synthetic/scenario_config.cpp
    src/synthetic/scenario_repository.cpp
    src/synthetic/star_field_generator.cpp
    src/synthetic/synthetic_data_app.cpp
)

target_compile_features(transient_tracker_synthetic PUBLIC cxx_std_20)
target_compile_options(transient_tracker_synthetic PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(transient_tracker_synthetic PUBLIC include)
target_include_directories(transient_tracker_synthetic SYSTEM PUBLIC ${xtensor_INCLUDE_DIRS})
target_link_libraries(transient_tracker_synthetic
    PUBLIC
        xtensor
        ${OpenCV_LIBS}
)
target_include_directories(transient_tracker_synthetic SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS})

add_library(transient_tracker_analysis
    src/analyze/analysis_config.cpp
    src/analyze/analyze_app.cpp
    src/analyze/analysis_writer.cpp
    src/analyze/candidate_extractor.cpp
    src/analyze/dataset_reader.cpp
    src/analyze/frame_aligner.cpp
    src/analyze/frame_loader.cpp
    src/analyze/frame_normalizer.cpp
    src/analyze/registration_estimator.cpp
    src/analyze/reference_builder.cpp
    src/analyze/track_linker.cpp
    src/analyze/truth_evaluator.cpp
)

target_compile_features(transient_tracker_analysis PUBLIC cxx_std_20)
target_compile_options(transient_tracker_analysis PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(transient_tracker_analysis PUBLIC include)
target_include_directories(transient_tracker_analysis SYSTEM PUBLIC ${xtensor_INCLUDE_DIRS})
target_link_libraries(transient_tracker_analysis
    PUBLIC
        xtensor
        ${OpenCV_LIBS}
)
target_include_directories(transient_tracker_analysis SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS})

add_executable(transient_tracker_generate apps/generate/main.cpp)
target_compile_features(transient_tracker_generate PRIVATE cxx_std_20)
target_compile_options(transient_tracker_generate PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(transient_tracker_generate PRIVATE transient_tracker_synthetic)

add_executable(transient_tracker_analyze apps/analyze/main.cpp)
target_compile_features(transient_tracker_analyze PRIVATE cxx_std_20)
target_compile_options(transient_tracker_analyze PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(transient_tracker_analyze PRIVATE transient_tracker_analysis)

if(BUILD_TESTING)
    add_executable(synthetic_config_test tests/synthetic_config_test.cpp)
    target_compile_features(synthetic_config_test PRIVATE cxx_std_20)
    target_compile_options(synthetic_config_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(synthetic_config_test PRIVATE transient_tracker_synthetic)

    add_executable(synthetic_truth_test tests/synthetic_truth_test.cpp)
    target_compile_features(synthetic_truth_test PRIVATE cxx_std_20)
    target_compile_options(synthetic_truth_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(synthetic_truth_test PRIVATE transient_tracker_synthetic)

    add_executable(synthetic_app_smoke_test tests/synthetic_app_smoke_test.cpp)
    target_compile_features(synthetic_app_smoke_test PRIVATE cxx_std_20)
    target_compile_options(synthetic_app_smoke_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(synthetic_app_smoke_test PRIVATE transient_tracker_synthetic)

    add_executable(synthetic_render_test tests/synthetic_render_test.cpp)
    target_compile_features(synthetic_render_test PRIVATE cxx_std_20)
    target_compile_options(synthetic_render_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(synthetic_render_test PRIVATE transient_tracker_synthetic)

    add_executable(analyze_dataset_reader_test tests/analyze_dataset_reader_test.cpp)
    target_compile_features(analyze_dataset_reader_test PRIVATE cxx_std_20)
    target_compile_options(analyze_dataset_reader_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(analyze_dataset_reader_test PRIVATE transient_tracker_analysis transient_tracker_synthetic)

    add_executable(analyze_candidate_test tests/analyze_candidate_test.cpp)
    target_compile_features(analyze_candidate_test PRIVATE cxx_std_20)
    target_compile_options(analyze_candidate_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(analyze_candidate_test PRIVATE transient_tracker_analysis transient_tracker_synthetic)

    add_executable(analyze_app_smoke_test tests/analyze_app_smoke_test.cpp)
    target_compile_features(analyze_app_smoke_test PRIVATE cxx_std_20)
    target_compile_options(analyze_app_smoke_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(analyze_app_smoke_test PRIVATE transient_tracker_analysis transient_tracker_synthetic)

    add_executable(analyze_registration_test tests/analyze_registration_test.cpp)
    target_compile_features(analyze_registration_test PRIVATE cxx_std_20)
    target_compile_options(analyze_registration_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(analyze_registration_test PRIVATE transient_tracker_analysis transient_tracker_synthetic)

    add_executable(analyze_track_linker_test tests/analyze_track_linker_test.cpp)
    target_compile_features(analyze_track_linker_test PRIVATE cxx_std_20)
    target_compile_options(analyze_track_linker_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(analyze_track_linker_test PRIVATE transient_tracker_analysis transient_tracker_synthetic)

    add_test(NAME synthetic_config_test COMMAND synthetic_config_test)
    add_test(NAME synthetic_truth_test COMMAND synthetic_truth_test)
    add_test(NAME synthetic_app_smoke_test COMMAND synthetic_app_smoke_test)
    add_test(NAME synthetic_render_test COMMAND synthetic_render_test)
    add_test(NAME analyze_dataset_reader_test COMMAND analyze_dataset_reader_test)
    add_test(NAME analyze_candidate_test COMMAND analyze_candidate_test)
    add_test(NAME analyze_app_smoke_test COMMAND analyze_app_smoke_test)
    add_test(NAME analyze_registration_test COMMAND analyze_registration_test)
    add_test(NAME analyze_track_linker_test COMMAND analyze_track_linker_test)
endif()
