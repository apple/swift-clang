//===-- InMemoryOutputFileSystem.h - Collects outputs in memory -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_INMEMORYOUTPUTFILESYSTEM_H_
#define LLVM_CLANG_BASIC_INMEMORYOUTPUTFILESYSTEM_H_

#include <memory>
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Mutex.h"
#include "llvm/Support/MutexGuard.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/VirtualFileSystem.h"

namespace clang {

/// Collects output files in memory, and provides a `llvm::vfs::FileSystem`
/// interface for accessing those files.
///
/// This class is threadsafe. Unsynchronized calls from multiple threads will
/// not corrupt the internal state, and operations occur atomically and
/// sequentially consistently from the point of view of all threads.
class InMemoryOutputFileSystem : public llvm::vfs::FileSystem {
 public:
  InMemoryOutputFileSystem() : OutputFiles(new llvm::vfs::InMemoryFileSystem())
  {}

  /// Creates a temporary buffer that collects data for a file that may
  /// eventually appear on the `llvm::vfs::FileSystem` interface.
  /// `InMemoryOutputFileSystem` owns the buffer, which will not be released
  /// until `DeleteTemporaryFile` or `FinalizeTemporaryFile` is called.
  /// \param OutputPath the path of the file that may eventually be created.
  /// \param TemporaryPath must be non-null. Pointee will be set to a unique
  //         string identifying this particular temporary buffer.
  //  \returns A stream that can be used to write to the buffer.
  std::unique_ptr<llvm::raw_pwrite_stream> CreateTemporaryBuffer(
      llvm::StringRef OutputPath,
      std::string *TemporaryPath);

  /// Releases the buffer underlying the temporary file.
  /// \param TemporaryPath the unique string from `CreateTemporaryFile`.
  void DeleteTemporaryBuffer(llvm::StringRef TemporaryPath);

  /// Makes the contents of the specified temporary buffer visible on the
  /// `llvm::vfs::FileSystem` interface, and releases the temporary buffer. If
  /// the file already exists on the `llvm::vfs::FileSystem` interface, then
  /// the new contents is silently ignored.
  /// \param OutputPath the path of the file to create.
  /// \param TemporaryPath the unique string from `CreateTemporaryFile`.
  void FinalizeTemporaryBuffer(llvm::StringRef OutputPath,
                               llvm::StringRef TemporaryPath);

  // MARK: - `llvm::vfs::FileSystem` overrides

  llvm::ErrorOr<llvm::vfs::Status> status(const llvm::Twine& relpath) override {
    llvm::MutexGuard locked(Mu);
    return OutputFiles->status(relpath);
  }

  llvm::ErrorOr<std::unique_ptr<llvm::vfs::File>> openFileForRead(
      const llvm::Twine& relpath) override {
    llvm::MutexGuard locked(Mu);
    return OutputFiles->openFileForRead(relpath);
  }

  llvm::vfs::directory_iterator dir_begin(const llvm::Twine& reldir,
                                          std::error_code& err) override {
    llvm::MutexGuard locked(Mu);
    return OutputFiles->dir_begin(reldir, err);
  }

  std::error_code setCurrentWorkingDirectory(const llvm::Twine& path) override {
    llvm::MutexGuard locked(Mu);
    return OutputFiles->setCurrentWorkingDirectory(path);
  }

  llvm::ErrorOr<std::string> getCurrentWorkingDirectory() const override {
    llvm::MutexGuard locked(Mu);
    return OutputFiles->getCurrentWorkingDirectory();
  }

  std::error_code getRealPath(
      const llvm::Twine& path,
      llvm::SmallVectorImpl<char>& output) const override {
    llvm::MutexGuard locked(Mu);
    return OutputFiles->getRealPath(path, output);
  }

 private:
  mutable llvm::sys::Mutex Mu;
  llvm::StringMap<llvm::SmallVector<char, 0>> TemporaryBuffers;
  llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> OutputFiles;
};

}  // namespace clang

#endif  // LLVM_CLANG_BASIC_INMEMORYOUTPUTFILESYSTEM_H_
