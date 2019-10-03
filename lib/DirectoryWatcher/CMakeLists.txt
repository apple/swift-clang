include(CheckIncludeFiles)

set(LLVM_LINK_COMPONENTS support)

CHECK_SYMBOL_EXISTS("FSEventStreamRef" "CoreServices/CoreServices.h" HAS_FS_EVENTS)

add_clang_library(clangDirectoryWatcher
  DirectoryWatcher.cpp
  )

if(APPLE)
  check_include_files("CoreServices/CoreServices.h" HAVE_CORESERVICES_H)
  if(HAVE_CORESERVICES_H)
    target_link_libraries(clangDirectoryWatcher PRIVATE "-framework CoreServices")
  endif()
endif()
