include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangIndexRecordHasher
  IndexRecordHasher.cpp
  DeclHasher.cpp
  CachingHasher.cpp

  LINK_LIBS
  clangAST
  clangBasic
  )
