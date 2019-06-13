//=== InMemoryOutputFileSystem.cpp - Collects outputs in memory -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/InMemoryOutputFileSystem.h"

namespace clang {

std::unique_ptr<llvm::raw_pwrite_stream>
InMemoryOutputFileSystem::CreateTemporaryBuffer(llvm::StringRef OutputPath,
                                                std::string *TemporaryPath) {
  assert(TemporaryPath);
  llvm::MutexGuard locked(Mu);
  llvm::StringMap<llvm::SmallVector<char, 0>>::iterator it;
  bool inserted = false;
  unsigned suffix = 0;
  while (!inserted) {
    *TemporaryPath = "";
    llvm::raw_string_ostream TemporaryPathOS(*TemporaryPath);
    TemporaryPathOS << OutputPath << "-" << suffix;
    TemporaryPathOS.flush();
    auto result = TemporaryBuffers.try_emplace(*TemporaryPath);
    it = result.first;
    inserted = result.second;
    suffix += 1;
  }
  return llvm::make_unique<llvm::raw_svector_ostream>(it->getValue());
}

void InMemoryOutputFileSystem::DeleteTemporaryBuffer(llvm::StringRef TemporaryPath) {
  llvm::MutexGuard locked(Mu);
  auto it = TemporaryBuffers.find(TemporaryPath);
  assert(it != TemporaryBuffers.end());
  TemporaryBuffers.erase(it);
}

void InMemoryOutputFileSystem::FinalizeTemporaryBuffer(llvm::StringRef OutputPath,
                             llvm::StringRef TemporaryPath) {
  llvm::MutexGuard locked(Mu);
  auto it = TemporaryBuffers.find(TemporaryPath);
  assert(it != TemporaryBuffers.end());
  auto memoryBuffer = llvm::MemoryBuffer::getMemBufferCopy(
      llvm::StringRef{it->getValue().data(), it->getValue().size()},
      OutputPath);
  OutputFiles->addFile(OutputPath, /*ModificationTime=*/0,
                       std::move(memoryBuffer));
  TemporaryBuffers.erase(it);
}

}  // namespace clang
