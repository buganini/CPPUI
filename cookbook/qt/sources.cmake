# Shared recipes selected by this backend's implemented capabilities.
target_sources(${target} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/backend.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../progressbar.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../text.cpp")
