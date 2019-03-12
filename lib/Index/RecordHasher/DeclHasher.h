//===--- DeclHasher.h - Hashing of Decl nodes in AST ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_INDEX_RECORDHASHER_DECLHASHER_H
#define LLVM_CLANG_LIB_INDEX_RECORDHASHER_DECLHASHER_H

#include "clang/AST/Decl.h"
#include "clang/AST/DeclVisitor.h"
#include "llvm/Support/Path.h"

namespace clang {
namespace index {

class CachingHasher;

/// Implements hashing for declaration nodes in AST.
/// This is just a convenient way how to avoid writing a huge switch for various
/// types derived from Decl. Uses CachingHasher for hashing of atomic entities.

class DeclHasher : public ConstDeclVisitor<DeclHasher, llvm::hash_code> {
  CachingHasher &Hasher;

public:
  DeclHasher(CachingHasher &Hasher) : Hasher(Hasher) {}

  llvm::hash_code VisitDecl(const Decl *D);
  llvm::hash_code VisitNamedDecl(const NamedDecl *D);
  llvm::hash_code VisitTagDecl(const TagDecl *D);
  llvm::hash_code VisitClassTemplateSpecializationDecl(
      const ClassTemplateSpecializationDecl *D);
  llvm::hash_code VisitObjCContainerDecl(const ObjCContainerDecl *D);
  llvm::hash_code VisitObjCImplDecl(const ObjCImplDecl *D);
  llvm::hash_code VisitObjCCategoryDecl(const ObjCCategoryDecl *D);
  llvm::hash_code VisitFunctionDecl(const FunctionDecl *D);
  llvm::hash_code
  VisitUnresolvedUsingTypenameDecl(const UnresolvedUsingTypenameDecl *D);
  llvm::hash_code
  VisitUnresolvedUsingValueDecl(const UnresolvedUsingValueDecl *D);
  llvm::hash_code VisitDeclContext(const DeclContext *DC);
  llvm::hash_code hashLoc(SourceLocation Loc, bool IncludeOffset);
};

} // end namespace index
} // end namespace clang

#endif // LLVM_CLANG_LIB_INDEX_RECORDHASHER_DECLHASHER_H
