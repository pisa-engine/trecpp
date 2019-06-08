add_executable(trec trec.cpp)
target_link_libraries(trec
  trecpp
  CLI11
)
