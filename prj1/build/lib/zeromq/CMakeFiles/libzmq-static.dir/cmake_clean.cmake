file(REMOVE_RECURSE
  "../../libzmq.a"
  "../../libzmq.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/libzmq-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
