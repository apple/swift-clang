//===--- IndexRecordHasher.cpp - Hashing of FileIndexRecord -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "IndexRecordHasher.h"
#include "FileIndexRecord.h"
#include "RecordHasher/CachingHasher.h"

namespace clang {
namespace index {

using llvm::hash_code;

hash_code hashRecord(ASTContext &Ctx, const FileIndexRecord &Record) {

  CachingHasher Hasher(Ctx);

  hash_code Hash = InitialHash;
  for (auto &Info : Record.getDeclOccurrencesSortedByOffset()) {
    Hash = hash_combine(Hash, Info.Roles, Info.Offset, Hasher.hash(Info.Dcl));
    for (auto &Rel : Info.Relations) {
      Hash = hash_combine(Hash, Hasher.hash(Rel.RelatedSymbol));
    }
  }
  return Hash;
}

} // end namespace index
} // end namespace clang