file(REMOVE_RECURSE
  "../../.5"
  "../../libzmq.pdb"
  "../../libzmq.so"
  "../../libzmq.so.5"
  "../../libzmq.so.5.2.5"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/libzmq.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
