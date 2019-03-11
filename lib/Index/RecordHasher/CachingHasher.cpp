//===--- CachingHasher.cpp - Hashing of indexed entities --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RecordHasher/CachingHasher.h"
#include "RecordHasher/DeclHasher.h"

namespace clang {
namespace index {

using llvm::hash_code;

hash_code CachingHasher::hash(const Decl *D) {
  assert(D->isCanonicalDecl());

  if (isa<TagDecl>(D) || isa<ObjCContainerDecl>(D)) {
    return getCachedHash(D, D);
  } else  if (auto *NS = dyn_cast<NamespaceDecl>(D)) {
    if (NS->isAnonymousNamespace())
      return hash_value(StringRef("@aN"));
    return getCachedHash(D, D);
  } else {
    // There's a balance between caching results and not growing the cache too
    // much. Measurements showed that avoiding caching all decls is beneficial
    // particularly when including all of Cocoa.
    return hashImpl(D);
  }
}

hash_code CachingHasher::hash(QualType NonCanTy) {
  CanQualType CanTy = Ctx.getCanonicalType(NonCanTy);
  return hash(CanTy);
}

hash_code CachingHasher::hash(CanQualType CT) {
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

  return hash_combine(Hash, getCachedHash(CT.getAsOpaquePtr(), CT));
}

hash_code CachingHasher::hash(DeclarationName Name) {
  assert(!Name.isEmpty());
  // Measurements for using cache or not here, showed significant slowdown when
  // using the cache for all DeclarationNames when parsing Cocoa, and minor
  // improvement or no difference for a couple of C++ single translation unit
  // files. So we avoid caching DeclarationNames.
  return hashImpl(Name);
}

hash_code CachingHasher::hash(const NestedNameSpecifier *NNS) {
  assert(NNS);
  // Measurements for the C++ single translation unit files did not show much
  // difference here; choosing to cache them currently.
  return getCachedHash(NNS, NNS);
}

hash_code CachingHasher::hash(const TemplateArgument &Arg) {
  // No caching.
  return hashImpl(Arg);
}

template <typename T>
hash_code CachingHasher::getCachedHash(const void *Ptr, T Obj) {
  auto It = HashByPtr.find(Ptr);
  if (It != HashByPtr.end())
    return It->second;

  hash_code Hash = hashImpl(Obj);
  // hashImpl() may call into getCachedHash recursively and mutate
  // HashByPtr, so we use find() earlier and insert the hash with another
  // lookup here instead of calling insert() earlier and utilizing the iterator
  // that insert() returns.
  HashByPtr[Ptr] = Hash;
  return Hash;
}

hash_code CachingHasher::hashImpl(const Decl *D) {
  return DeclHasher(*this).Visit(D);
}

hash_code CachingHasher::hashImpl(const IdentifierInfo *II) {
  return hash_value(II->getName());
}

hash_code CachingHasher::hashImpl(Selector Sel) {
  unsigned N = Sel.getNumArgs();
  if (N == 0)
    ++N;
  hash_code Hash = InitialHash;
  for (unsigned I = 0; I != N; ++I)
    if (IdentifierInfo *II = Sel.getIdentifierInfoForSlot(I))
      Hash = hash_combine(Hash, hashImpl(II));
  return Hash;
}

hash_code CachingHasher::hashImpl(TemplateName Name) {
  hash_code Hash = InitialHash;
  if (TemplateDecl *Template = Name.getAsTemplateDecl()) {
    if (TemplateTemplateParmDecl *TTP
        = dyn_cast<TemplateTemplateParmDecl>(Template)) {
      return hash_combine(Hash, 't', TTP->getDepth(), TTP->getIndex());
    }

    return hash_combine(Hash, hash(Template->getCanonicalDecl()));
  }

  // FIXME: Hash dependent template names.
  return Hash;
}

hash_code CachingHasher::hashImpl(const TemplateArgument &Arg) {
  hash_code Hash = InitialHash;

  switch (Arg.getKind()) {
  case TemplateArgument::Null:
    break;

  case TemplateArgument::Declaration:
    Hash = hash_combine(Hash, hash(Arg.getAsDecl()));
    break;

  case TemplateArgument::NullPtr:
    break;

  case TemplateArgument::TemplateExpansion:
    Hash = hash_combine(Hash, 'P'); // pack expansion of...
    LLVM_FALLTHROUGH;
  case TemplateArgument::Template:
    Hash = hash_combine(Hash, hashImpl(Arg.getAsTemplateOrTemplatePattern()));
    break;
      
  case TemplateArgument::Expression:
    // FIXME: Hash expressions.
    break;
      
  case TemplateArgument::Pack:
    Hash = hash_combine(Hash, 'p');
    for (const auto &P : Arg.pack_elements())
      Hash = hash_combine(Hash, hashImpl(P));
    break;
      
  case TemplateArgument::Type:
    Hash = hash_combine(Hash, hash(Arg.getAsType()));
    break;
      
  case TemplateArgument::Integral:
    Hash = hash_combine(Hash, 'V', hash(Arg.getIntegralType()),
                        Arg.getAsIntegral());
    break;
  }

  return Hash;
}

hash_code CachingHasher::hashImpl(CanQualType CQT) {
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
    Hash = hash_combine(Hash, '>', hashImpl(Spec->getTemplateName()));
    for (unsigned I = 0, N = Spec->getNumArgs(); I != N; ++I)
      Hash = hash_combine(Hash, hashImpl(Spec->getArg(I)));
    return Hash;
  }
  if (const DependentNameType *DNT = dyn_cast<DependentNameType>(T)) {
    Hash = hash_combine(Hash, '^');
    if (const NestedNameSpecifier *NNS = DNT->getQualifier())
      Hash = hash_combine(Hash, hash(NNS));
    return hash_combine(Hash, hashImpl(DNT->getIdentifier()));
  }

  // Unhandled type.
  return Hash;
}

hash_code CachingHasher::hashImpl(DeclarationName Name) {
  hash_code Hash = InitialHash;
  Hash = hash_combine(Hash, Name.getNameKind());

  switch (Name.getNameKind()) {
    case DeclarationName::Identifier:
      Hash = hash_combine(Hash, hashImpl(Name.getAsIdentifierInfo()));
      break;
    case DeclarationName::ObjCZeroArgSelector:
    case DeclarationName::ObjCOneArgSelector:
    case DeclarationName::ObjCMultiArgSelector:
      Hash = hash_combine(Hash, hashImpl(Name.getObjCSelector()));
      break;
    case DeclarationName::CXXConstructorName:
    case DeclarationName::CXXDestructorName:
    case DeclarationName::CXXConversionFunctionName:
      break;
    case DeclarationName::CXXOperatorName:
      Hash = hash_combine(Hash, Name.getCXXOverloadedOperator());
      break;
    case DeclarationName::CXXLiteralOperatorName:
      Hash = hash_combine(Hash, hashImpl(Name.getCXXLiteralIdentifier()));
      break;
    case DeclarationName::CXXUsingDirective:
      break;
    case DeclarationName::CXXDeductionGuideName:
      Hash = hash_combine(Hash, hashImpl(Name.getCXXDeductionGuideTemplate()
                                             ->getDeclName()
                                             .getAsIdentifierInfo()));
      break;
  }

  return Hash;
}

hash_code CachingHasher::hashImpl(const NestedNameSpecifier *NNS) {
  hash_code Hash = InitialHash;
  if (auto *Pre = NNS->getPrefix())
    Hash = hash_combine(Hash, hash(Pre));

  Hash = hash_combine(Hash, NNS->getKind());

  switch (NNS->getKind()) {
  case NestedNameSpecifier::Identifier:
    Hash = hash_combine(Hash, hashImpl(NNS->getAsIdentifier()));
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

} // end namespace index
} // end namespace clang