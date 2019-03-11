//===--- IndexRecordHasher.cpp - Index record hashing -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "IndexRecordHasher.h"
#include "FileIndexRecord.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclVisitor.h"
#include "llvm/Support/Path.h"

constexpr size_t InitialHash = 5381;

using namespace clang;
using namespace clang::index;
using namespace llvm;

static hash_code computeHash(const TemplateArgument &Arg,
                             IndexRecordHasher &Hasher);

namespace {
class DeclHashVisitor : public ConstDeclVisitor<DeclHashVisitor, hash_code> {
  IndexRecordHasher &Hasher;

public:
  DeclHashVisitor(IndexRecordHasher &Hasher) : Hasher(Hasher) {}

  hash_code VisitDecl(const Decl *D) {
    return VisitDeclContext(D->getDeclContext());
  }

  hash_code VisitNamedDecl(const NamedDecl *D) {
    hash_code Hash = VisitDecl(D);
    if (auto *attr = D->getExternalSourceSymbolAttr()) {
      Hash = hash_combine(Hash, hash_value(attr->getDefinedIn()));
    }
    return hash_combine(Hash, Hasher.hash(D->getDeclName()));
  }

  hash_code VisitTagDecl(const TagDecl *D) {
    if (D->getDeclName().isEmpty()) {
      if (const TypedefNameDecl *TD = D->getTypedefNameForAnonDecl())
        return Visit(TD);

      hash_code Hash = VisitDeclContext(D->getDeclContext());
      if (D->isEmbeddedInDeclarator() && !D->isFreeStanding()) {
        Hash = hash_combine(Hash,
                            hashLoc(D->getLocation(), /*IncludeOffset=*/true));
      } else
        Hash = hash_combine(Hash, 'a');
      return Hash;
    }

    hash_code Hash = VisitTypeDecl(D);
    return hash_combine(Hash, 'T');
  }

  hash_code VisitClassTemplateSpecializationDecl(const ClassTemplateSpecializationDecl *D) {
    hash_code Hash = VisitCXXRecordDecl(D);
    const TemplateArgumentList &Args = D->getTemplateArgs();
    Hash = hash_combine(Hash, '>');
    for (unsigned I = 0, N = Args.size(); I != N; ++I) {
      Hash = hash_combine(Hash, computeHash(Args.get(I), Hasher));
    }
    return Hash;
  }

  hash_code VisitObjCContainerDecl(const ObjCContainerDecl *D) {
    hash_code Hash = VisitNamedDecl(D);
    return hash_combine(Hash, 'I');
  }

  hash_code VisitObjCImplDecl(const ObjCImplDecl *D) {
    if (auto *ID = D->getClassInterface())
      return VisitObjCInterfaceDecl(ID);
    else
      return 0;
  }

  hash_code VisitObjCCategoryDecl(const ObjCCategoryDecl *D) {
    // FIXME: Differentiate between category and the interface ?
    if (auto *ID = D->getClassInterface())
      return VisitObjCInterfaceDecl(ID);
    else
      return 0;
  }

  hash_code VisitFunctionDecl(const FunctionDecl *D) {
    hash_code Hash = VisitNamedDecl(D);
    ASTContext &Ctx = Hasher.getASTContext();
    if ((!Ctx.getLangOpts().CPlusPlus && !D->hasAttr<OverloadableAttr>())
        || D->isExternC())
      return Hash;

    for (auto param : D->parameters()) {
      Hash = hash_combine(Hash, Hasher.hash(param->getType()));
    }
    return Hash;
  }

  hash_code VisitUnresolvedUsingTypenameDecl(const UnresolvedUsingTypenameDecl *D) {
    hash_code Hash = VisitNamedDecl(D);
    Hash = hash_combine(Hash, Hasher.hash(D->getQualifier()));
    return Hash;
  }

  hash_code VisitUnresolvedUsingValueDecl(const UnresolvedUsingValueDecl *D) {
    hash_code Hash = VisitNamedDecl(D);
    Hash = hash_combine(Hash, Hasher.hash(D->getQualifier()));
    return Hash;
  }

  hash_code VisitDeclContext(const DeclContext *DC) {
    // FIXME: Add location if this is anonymous namespace ?
    DC = DC->getRedeclContext();
    const Decl *D = cast<Decl>(DC)->getCanonicalDecl();
    if (auto *ND = dyn_cast<NamedDecl>(D))
      return Hasher.hash(ND);
    else
      return 0;
  }

  hash_code hashLoc(SourceLocation Loc, bool IncludeOffset) {
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
};
}

hash_code IndexRecordHasher::hashRecord(const FileIndexRecord &Record) {
  hash_code Hash = InitialHash;
  for (auto &Info : Record.getDeclOccurrencesSortedByOffset()) {
    Hash = hash_combine(Hash, Info.Roles, Info.Offset, hash(Info.Dcl));
    for (auto &Rel : Info.Relations) {
      Hash = hash_combine(Hash, hash(Rel.RelatedSymbol));
    }
  }
  return Hash;
}

hash_code IndexRecordHasher::hash(const Decl *D) {
  assert(D->isCanonicalDecl());

  if (isa<TagDecl>(D) || isa<ObjCContainerDecl>(D)) {
    return tryCache(D, D);
  } else  if (auto *NS = dyn_cast<NamespaceDecl>(D)) {
    if (NS->isAnonymousNamespace())
      return hash_value(StringRef("@aN"));
    return tryCache(D, D);
  } else {
    // There's a balance between caching results and not growing the cache too
    // much. Measurements showed that avoiding caching all decls is beneficial
    // particularly when including all of Cocoa.
    return hashImpl(D);
  }
}

hash_code IndexRecordHasher::hash(QualType NonCanTy) {
  CanQualType CanTy = Ctx.getCanonicalType(NonCanTy);
  return hash(CanTy);
}

hash_code IndexRecordHasher::hash(CanQualType CT) {
  // Do some hashing without going to the cache, for example we can avoid
  // storing the hash for both the type and its const-qualified version.
  hash_code Hash = InitialHash;

  auto asCanon = [](QualType Ty) -> CanQualType {
    return CanQualType::CreateUnsafe(Ty);
  };

  while (true) {
    Qualifiers Q = CT.getQualifiers();
    CT = CT.getUnqualifiedType();
    const Type *T = CT.getTypePtr();
    unsigned qVal = 0;
    if (Q.hasConst())
      qVal |= 0x1;
    if (Q.hasVolatile())
      qVal |= 0x2;
    if (Q.hasRestrict())
      qVal |= 0x4;
    if(qVal)
      Hash = hash_combine(Hash, qVal);

    // Hash in ObjC GC qualifiers?

    if (const BuiltinType *BT = dyn_cast<BuiltinType>(T)) {
      return hash_combine(Hash, BT->getKind());
    }
    if (const PointerType *PT = dyn_cast<PointerType>(T)) {
      Hash = hash_combine(Hash, '*');
      CT = asCanon(PT->getPointeeType());
      continue;
    }
    if (const ReferenceType *RT = dyn_cast<ReferenceType>(T)) {
      Hash = hash_combine(Hash, '&');
      CT = asCanon(RT->getPointeeType());
      continue;
    }
    if (const BlockPointerType *BT = dyn_cast<BlockPointerType>(T)) {
      Hash = hash_combine(Hash, 'B');
      CT = asCanon(BT->getPointeeType());
      continue;
    }
    if (const ObjCObjectPointerType *OPT = dyn_cast<ObjCObjectPointerType>(T)) {
      Hash = hash_combine(Hash, '*');
      CT = asCanon(OPT->getPointeeType());
      continue;
    }
    if (const TagType *TT = dyn_cast<TagType>(T)) {
      return hash_combine(Hash, '$', hash(TT->getDecl()->getCanonicalDecl()));
    }
    if (const ObjCInterfaceType *OIT = dyn_cast<ObjCInterfaceType>(T)) {
      return hash_combine(Hash, '$', hash(OIT->getDecl()->getCanonicalDecl()));
    }
    if (const ObjCObjectType *OIT = dyn_cast<ObjCObjectType>(T)) {
      for (auto *Prot : OIT->getProtocols())
        Hash = hash_combine(Hash, hash(Prot));
      CT = asCanon(OIT->getBaseType());
      continue;
    }
    if (const TemplateTypeParmType *TTP = dyn_cast<TemplateTypeParmType>(T)) {
      return hash_combine(Hash, 't', TTP->getDepth(), TTP->getIndex());
    }
    if (const InjectedClassNameType *InjT = dyn_cast<InjectedClassNameType>(T)) {
      CT = asCanon(InjT->getInjectedSpecializationType().getCanonicalType());
      continue;
    }

    break;
  }

  return hash_combine(Hash, tryCache(CT.getAsOpaquePtr(), CT));
}

hash_code IndexRecordHasher::hash(DeclarationName Name) {
  assert(!Name.isEmpty());
  // Measurements for using cache or not here, showed significant slowdown when
  // using the cache for all DeclarationNames when parsing Cocoa, and minor
  // improvement or no difference for a couple of C++ single translation unit
  // files. So we avoid caching DeclarationNames.
  return hashImpl(Name);
}

hash_code IndexRecordHasher::hash(const NestedNameSpecifier *NNS) {
  assert(NNS);
  // Measurements for the C++ single translation unit files did not show much
  // difference here; choosing to cache them currently.
  return tryCache(NNS, NNS);
}

template <typename T>
hash_code IndexRecordHasher::tryCache(const void *Ptr, T Obj) {
  auto It = HashByPtr.find(Ptr);
  if (It != HashByPtr.end())
    return It->second;

  hash_code Hash = hashImpl(Obj);
  // hashImpl() may call into tryCache recursively and mutate
  // HashByPtr, so we use find() earlier and insert the hash with another
  // lookup here instead of calling insert() earlier and utilizing the iterator
  // that insert() returns.
  HashByPtr[Ptr] = Hash;
  return Hash;
}

hash_code IndexRecordHasher::hashImpl(const Decl *D) {
  return DeclHashVisitor(*this).Visit(D);
}

static hash_code computeHash(const IdentifierInfo *II) {
  return hash_value(II->getName());
}

static hash_code computeHash(Selector Sel) {
  unsigned N = Sel.getNumArgs();
  if (N == 0)
    ++N;
  hash_code Hash = InitialHash;
  for (unsigned I = 0; I != N; ++I)
    if (IdentifierInfo *II = Sel.getIdentifierInfoForSlot(I))
      Hash = hash_combine(Hash, computeHash(II));
  return Hash;
}

static hash_code computeHash(TemplateName Name, IndexRecordHasher &Hasher) {
  hash_code Hash = InitialHash;
  if (TemplateDecl *Template = Name.getAsTemplateDecl()) {
    if (TemplateTemplateParmDecl *TTP
        = dyn_cast<TemplateTemplateParmDecl>(Template)) {
      return hash_combine(Hash, 't', TTP->getDepth(), TTP->getIndex());
    }

    return hash_combine(Hash, Hasher.hash(Template->getCanonicalDecl()));
  }

  // FIXME: Hash dependent template names.
  return Hash;
}

static hash_code computeHash(const TemplateArgument &Arg,
                             IndexRecordHasher &Hasher) {
  hash_code Hash = InitialHash;

  switch (Arg.getKind()) {
  case TemplateArgument::Null:
    break;

  case TemplateArgument::Declaration:
    Hash = hash_combine(Hash, Hasher.hash(Arg.getAsDecl()));
    break;

  case TemplateArgument::NullPtr:
    break;

  case TemplateArgument::TemplateExpansion:
    Hash = hash_combine(Hash, 'P'); // pack expansion of...
    LLVM_FALLTHROUGH;
  case TemplateArgument::Template:
    Hash = hash_combine(
        Hash, computeHash(Arg.getAsTemplateOrTemplatePattern(), Hasher));
    break;
      
  case TemplateArgument::Expression:
    // FIXME: Hash expressions.
    break;
      
  case TemplateArgument::Pack:
    Hash = hash_combine(Hash, 'p');
    for (const auto &P : Arg.pack_elements())
      Hash = hash_combine(Hash, computeHash(P, Hasher));
    break;
      
  case TemplateArgument::Type:
    Hash = hash_combine(Hash, Hasher.hash(Arg.getAsType()));
    break;
      
  case TemplateArgument::Integral:
    Hash = hash_combine(Hash, 'V', Hasher.hash(Arg.getIntegralType()),
                        Arg.getAsIntegral());
    break;
  }

  return Hash;
}

hash_code IndexRecordHasher::hashImpl(CanQualType CQT) {
  hash_code Hash = InitialHash;

  auto asCanon = [](QualType Ty) -> CanQualType {
    return CanQualType::CreateUnsafe(Ty);
  };

  const Type *T = CQT.getTypePtr();

  if (const PackExpansionType *Expansion = dyn_cast<PackExpansionType>(T)) {
    return hash_combine(Hash, 'P', hash(asCanon(Expansion->getPattern())));
  }
  if (const RValueReferenceType *RT = dyn_cast<RValueReferenceType>(T)) {
    return hash_combine(Hash, '%', hash(asCanon(RT->getPointeeType())));
  }
  if (const FunctionProtoType *FT = dyn_cast<FunctionProtoType>(T)) {
    Hash = hash_combine(Hash, 'F', hash(asCanon(FT->getReturnType())));
    for (const auto &I : FT->param_types())
      Hash = hash_combine(Hash, hash(asCanon(I)));
    return hash_combine(Hash, FT->isVariadic());
  }
  if (const ComplexType *CT = dyn_cast<ComplexType>(T)) {
    return hash_combine(Hash, '<', hash(asCanon(CT->getElementType())));
  }
  if (const TemplateSpecializationType *Spec
      = dyn_cast<TemplateSpecializationType>(T)) {
    Hash = hash_combine(Hash, '>', computeHash(Spec->getTemplateName(), *this));
    for (unsigned I = 0, N = Spec->getNumArgs(); I != N; ++I)
      Hash = hash_combine(Hash, computeHash(Spec->getArg(I), *this));
    return Hash;
  }
  if (const DependentNameType *DNT = dyn_cast<DependentNameType>(T)) {
    Hash = hash_combine(Hash, '^');
    if (const NestedNameSpecifier *NNS = DNT->getQualifier())
      Hash = hash_combine(Hash, hash(NNS));
    return hash_combine(Hash, computeHash(DNT->getIdentifier()));
  }

  // Unhandled type.
  return Hash;
}

hash_code IndexRecordHasher::hashImpl(DeclarationName Name) {
  hash_code Hash = InitialHash;
  Hash = hash_combine(Hash, Name.getNameKind());

  switch (Name.getNameKind()) {
    case DeclarationName::Identifier:
      Hash = hash_combine(Hash, computeHash(Name.getAsIdentifierInfo()));
      break;
    case DeclarationName::ObjCZeroArgSelector:
    case DeclarationName::ObjCOneArgSelector:
    case DeclarationName::ObjCMultiArgSelector:
      Hash = hash_combine(Hash, computeHash(Name.getObjCSelector()));
      break;
    case DeclarationName::CXXConstructorName:
    case DeclarationName::CXXDestructorName:
    case DeclarationName::CXXConversionFunctionName:
      break;
    case DeclarationName::CXXOperatorName:
      Hash = hash_combine(Hash, Name.getCXXOverloadedOperator());
      break;
    case DeclarationName::CXXLiteralOperatorName:
      Hash = hash_combine(Hash, computeHash(Name.getCXXLiteralIdentifier()));
      break;
    case DeclarationName::CXXUsingDirective:
      break;
    case DeclarationName::CXXDeductionGuideName:
      Hash = hash_combine(Hash, computeHash(Name.getCXXDeductionGuideTemplate()
                                                ->getDeclName()
                                                .getAsIdentifierInfo()));
      break;
  }

  return Hash;
}

hash_code IndexRecordHasher::hashImpl(const NestedNameSpecifier *NNS) {
  hash_code Hash = InitialHash;
  if (auto *Pre = NNS->getPrefix())
    Hash = hash_combine(Hash, hash(Pre));

  Hash = hash_combine(Hash, NNS->getKind());

  switch (NNS->getKind()) {
  case NestedNameSpecifier::Identifier:
    Hash = hash_combine(Hash, computeHash(NNS->getAsIdentifier()));
    break;

  case NestedNameSpecifier::Namespace:
    Hash = hash_combine(Hash, hash(NNS->getAsNamespace()->getCanonicalDecl()));
    break;

  case NestedNameSpecifier::NamespaceAlias:
    Hash = hash_combine(Hash,
                        hash(NNS->getAsNamespaceAlias()->getCanonicalDecl()));
    break;

  case NestedNameSpecifier::Global:
    break;

  case NestedNameSpecifier::Super:
    break;

  case NestedNameSpecifier::TypeSpecWithTemplate:
    // Fall through to hash the type.

  case NestedNameSpecifier::TypeSpec:
    Hash = hash_combine(Hash, hash(QualType(NNS->getAsType(), 0)));
    break;
  }

  return Hash;
}
