//===--- CachingHasher.h - Hashing of indexed entities ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_INDEX_RECORDHASHER_CACHINGHASHER_H
#define LLVM_CLANG_LIB_INDEX_RECORDHASHER_CACHINGHASHER_H

#include "clang/AST/ASTContext.h"
#include "clang/Basic/LLVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Hashing.h"

namespace clang {
namespace index {

constexpr size_t InitialHash = 5381;

/// Utility class implementing hashing and caching of hashes.
class CachingHasher {
  ASTContext &Ctx;
  llvm::DenseMap<const void *, llvm::hash_code> HashByPtr;

public:
  explicit CachingHasher(ASTContext &Ctx) : Ctx(Ctx) {}
  ASTContext &getASTContext() { return Ctx; }

  /// Public interface that implements caching strategy.
  llvm::hash_code hash(const Decl *D);
  llvm::hash_code hash(QualType Ty);
  llvm::hash_code hash(CanQualType Ty);
  llvm::hash_code hash(DeclarationName Name);
  llvm::hash_code hash(const NestedNameSpecifier *NNS);
  llvm::hash_code hash(const TemplateArgument &Arg);

private:
  /// \returns hash of \p Obj.
  /// Uses cached value if it exists otherwise calculates the hash, adds it to
  /// the cache and returns.
  template <typename T> llvm::hash_code getCachedHash(const void *Ptr, T Obj);

  // Private methods implement hashing itself. Intentionally hidden from client
  // (DeclHasher) to prevent accidental caching bypass.
  llvm::hash_code hashImpl(const Decl *D);
  llvm::hash_code hashImpl(CanQualType Ty);
  llvm::hash_code hashImpl(DeclarationName Name);
  llvm::hash_code hashImpl(const NestedNameSpecifier *NNS);
  llvm::hash_code hashImpl(const TemplateArgument &Arg);
  llvm::hash_code hashImpl(const IdentifierInfo *II);
  llvm::hash_code hashImpl(Selector Sel);
  llvm::hash_code hashImpl(TemplateName Name);
};

} // end namespace index
} // end namespace clang

#endif // LLVM_CLANG_LIB_INDEX_RECORDHASHER_CACHINGHASHER_H