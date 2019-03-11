//===--- IndexRecordHasher.h - Hashing of FileIndexRecord -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_INDEX_INDEXRECORDHASHER_H
#define LLVM_CLANG_LIB_INDEX_INDEXRECORDHASHER_H

#include "clang/AST/ASTContext.h"
#include "llvm/ADT/Hashing.h"

namespace clang {
namespace index {
  class FileIndexRecord;

  /// \returns hash of the \p Record
  llvm::hash_code hashRecord(ASTContext &Ctx, const FileIndexRecord &Record);

} // end namespace index
} // end namespace clang

#endif // LLVM_CLANG_LIB_INDEX_INDEXRECORDHASHER_H
