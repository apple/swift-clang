//===--- DeclHasher.cpp - Hashing of Decl nodes in AST ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RecordHasher/DeclHasher.h"
#include "RecordHasher/CachingHasher.h"

namespace clang {
namespace index {

using llvm::hash_code;

hash_code DeclHasher::VisitDecl(const Decl *D) {
  return VisitDeclContext(D->getDeclContext());
}

hash_code DeclHasher::VisitNamedDecl(const NamedDecl *D) {
  hash_code Hash = VisitDecl(D);
  if (auto *attr = D->getExternalSourceSymbolAttr()) {
    Hash = hash_combine(Hash, hash_value(attr->getDefinedIn()));
  }
  return hash_combine(Hash, Hasher.hash(D->getDeclName()));
}

hash_code DeclHasher::VisitTagDecl(const TagDecl *D) {
  if (D->getDeclName().isEmpty()) {
    if (const TypedefNameDecl *TD = D->getTypedefNameForAnonDecl())
      return Visit(TD);

    hash_code Hash = VisitDeclContext(D->getDeclContext());
    if (D->isEmbeddedInDeclarator() && !D->isFreeStanding()) {
      Hash =
          hash_combine(Hash, hashLoc(D->getLocation(), /*IncludeOffset=*/true));
    } else
      Hash = hash_combine(Hash, 'a');
    return Hash;
  }

  hash_code Hash = VisitTypeDecl(D);
  return hash_combine(Hash, 'T');
}

hash_code DeclHasher::VisitClassTemplateSpecializationDecl(
    const ClassTemplateSpecializationDecl *D) {
  hash_code Hash = VisitCXXRecordDecl(D);
  const TemplateArgumentList &Args = D->getTemplateArgs();
  Hash = hash_combine(Hash, '>');
  for (unsigned I = 0, N = Args.size(); I != N; ++I) {
    Hash = hash_combine(Hash, Hasher.hash(Args.get(I)));
  }
  return Hash;
}

hash_code DeclHasher::VisitObjCContainerDecl(const ObjCContainerDecl *D) {
  hash_code Hash = VisitNamedDecl(D);
  return hash_combine(Hash, 'I');
}

hash_code DeclHasher::VisitObjCImplDecl(const ObjCImplDecl *D) {
  if (auto *ID = D->getClassInterface())
    return VisitObjCInterfaceDecl(ID);
  else
    return 0;
}

hash_code DeclHasher::VisitObjCCategoryDecl(const ObjCCategoryDecl *D) {
  // FIXME: Differentiate between category and the interface ?
  if (auto *ID = D->getClassInterface())
    return VisitObjCInterfaceDecl(ID);
  else
    return 0;
}

hash_code DeclHasher::VisitFunctionDecl(const FunctionDecl *D) {
  hash_code Hash = VisitNamedDecl(D);
  ASTContext &Ctx = Hasher.getASTContext();
  if ((!Ctx.getLangOpts().CPlusPlus && !D->hasAttr<OverloadableAttr>()) ||
      D->isExternC())
    return Hash;

  for (auto param : D->parameters()) {
    Hash = hash_combine(Hash, Hasher.hash(param->getType()));
  }
  return Hash;
}

hash_code DeclHasher::VisitUnresolvedUsingTypenameDecl(
    const UnresolvedUsingTypenameDecl *D) {
  hash_code Hash = VisitNamedDecl(D);
  Hash = hash_combine(Hash, Hasher.hash(D->getQualifier()));
  return Hash;
}

hash_code
DeclHasher::VisitUnresolvedUsingValueDecl(const UnresolvedUsingValueDecl *D) {
  hash_code Hash = VisitNamedDecl(D);
  Hash = hash_combine(Hash, Hasher.hash(D->getQualifier()));
  return Hash;
}

hash_code DeclHasher::VisitDeclContext(const DeclContext *DC) {
  // FIXME: Add location if this is anonymous namespace ?
  DC = DC->getRedeclContext();
  const Decl *D = cast<Decl>(DC)->getCanonicalDecl();
  if (auto *ND = dyn_cast<NamedDecl>(D))
    return Hasher.hash(ND);
  else
    return 0;
}

hash_code DeclHasher::hashLoc(SourceLocation Loc, bool IncludeOffset) {
  if (Loc.isInvalid()) {
    return 0;
  }
  hash_code Hash = InitialHash;
  const SourceManager &SM = Hasher.getASTContext().getSourceManager();
  Loc = SM.getFileLoc(Loc);
  const std::pair<FileID, unsigned> &Decomposed = SM.getDecomposedLoc(Loc);
  const FileEntry *FE = SM.getFileEntryForID(Decomposed.first);
  if (FE) {
    Hash = hash_combine(Hash, llvm::sys::path::filename(FE->getName()));
  } else {
    // This case really isn't interesting.
    return 0;
  }
  if (IncludeOffset) {
    // Use the offest into the FileID to represent the location.  Using
    // a line/column can cause us to look back at the original source file,
    // which is expensive.
    Hash = hash_combine(Hash, Decomposed.second);
  }
  return Hash;
}

} // end namespace index
} // end namespace clang