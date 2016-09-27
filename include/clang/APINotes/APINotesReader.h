//===--- APINotesReader.h - API Notes Reader ----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the \c APINotesReader class that reads source
// API notes data providing additional information about source code as
// a separate input, such as the non-nil/nilable annotations for
// method parameters.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_API_NOTES_READER_H
#define LLVM_CLANG_API_NOTES_READER_H

#include "clang/APINotes/Types.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/MemoryBuffer.h"
#include <memory>

namespace clang {
namespace api_notes {

/// A class that reads API notes data from a binary file that was written by
/// the \c APINotesWriter.
class APINotesReader {
  class Implementation;

  Implementation &Impl;

  APINotesReader(std::unique_ptr<llvm::MemoryBuffer> inputBuffer, bool &failed);

public:
  /// Create a new API notes reader from the given member buffer, which
  /// contains the contents of a binary API notes file.
  ///
  /// \returns the new API notes reader, or null if an error occurred.
  static std::unique_ptr<APINotesReader>
  get(std::unique_ptr<llvm::MemoryBuffer> inputBuffer);

  ~APINotesReader();

  APINotesReader(const APINotesReader &) = delete;
  APINotesReader &operator=(const APINotesReader &) = delete;

  /// Retrieve the name of the module for which this reader is providing API
  /// notes.
  StringRef getModuleName() const;

  /// Retrieve the module options
  ModuleOptions getModuleOptions() const;

  /// Look for information regarding the given Objective-C class.
  ///
  /// \param name The name of the class we're looking for.
  ///
  /// \returns The ID and information about the class, if known.
  Optional<std::pair<ContextID, ObjCContextInfo>>
  lookupObjCClass(StringRef name);

  /// Look for information regarding the given Objective-C protocol.
  ///
  /// \param name The name of the protocol we're looking for.
  ///
  /// \returns The ID and information about the protocol, if known.
  Optional<std::pair<ContextID, ObjCContextInfo>>
  lookupObjCProtocol(StringRef name);

  /// Look for information regarding the given Objective-C property in
  /// the given context.
  ///
  /// \param contextID The ID that references the context we are looking for.
  /// \param name The name of the property we're looking for.
  /// \param isInstance Whether we are looking for an instance property (vs.
  /// a class property).
  ///
  /// \returns Information about the property, if known.
  Optional<ObjCPropertyInfo> lookupObjCProperty(ContextID contextID,
                                                StringRef name,
                                                bool isInstance);

  /// Look for information regarding the given Objective-C method in
  /// the given context.
  ///
  /// \param contextID The ID that references the context we are looking for.
  /// \param selector The selector naming the method we're looking for.
  /// \param isInstanceMethod Whether we are looking for an instance method.
  ///
  /// \returns Information about the method, if known.
  Optional<ObjCMethodInfo> lookupObjCMethod(ContextID contextID,
                                            ObjCSelectorRef selector,
                                            bool isInstanceMethod);

  /// Look for information regarding the given global variable.
  ///
  /// \param name The name of the global variable.
  ///
  /// \returns information about the global variable, if known.
  Optional<GlobalVariableInfo> lookupGlobalVariable(StringRef name);

  /// Look for information regarding the given global function.
  ///
  /// \param name The name of the global function.
  ///
  /// \returns information about the global function, if known.
  Optional<GlobalFunctionInfo> lookupGlobalFunction(StringRef name);

  /// Look for information regarding the given enumerator.
  ///
  /// \param name The name of the enumerator.
  ///
  /// \returns information about the enumerator, if known.
  Optional<EnumConstantInfo> lookupEnumConstant(StringRef name);

  /// Look for information regarding the given tag
  /// (struct/union/enum/C++ class).
  ///
  /// \param name The name of the tag.
  ///
  /// \returns information about the tag, if known.
  Optional<TagInfo> lookupTag(StringRef name);

  /// Look for information regarding the given typedef.
  ///
  /// \param name The name of the typedef.
  ///
  /// \returns information about the typedef, if known.
  Optional<TypedefInfo> lookupTypedef(StringRef name);

  /// Visitor used when walking the contents of the API notes file.
  class Visitor {
  public:
    virtual ~Visitor();

    /// Visit an Objective-C class.
    virtual void visitObjCClass(ContextID contextID, StringRef name,
                                const ObjCContextInfo &info);

    /// Visit an Objective-C protocol.
    virtual void visitObjCProtocol(ContextID contextID, StringRef name,
                                   const ObjCContextInfo &info);

    /// Visit an Objective-C method.
    virtual void visitObjCMethod(ContextID contextID, StringRef selector,
                                 bool isInstanceMethod,
                                 const ObjCMethodInfo &info);

    /// Visit an Objective-C property.
    virtual void visitObjCProperty(ContextID contextID, StringRef name,
                                   bool isInstance,
                                   const ObjCPropertyInfo &info);

    /// Visit a global variable.
    virtual void visitGlobalVariable(StringRef name,
                                     const GlobalVariableInfo &info);

    /// Visit a global function.
    virtual void visitGlobalFunction(StringRef name,
                                     const GlobalFunctionInfo &info);

    /// Visit an enumerator.
    virtual void visitEnumConstant(StringRef name,
                                   const EnumConstantInfo &info);

    /// Visit a tag.
    virtual void visitTag(StringRef name, const TagInfo &info);

    /// Visit a typedef.
    virtual void visitTypedef(StringRef name, const TypedefInfo &info);
  };

  /// Visit the contents of the API notes file, passing each entity to the
  /// given visitor.
  void visit(Visitor &visitor);
};

} // end namespace api_notes
} // end namespace clang

#endif // LLVM_CLANG_API_NOTES_READER_H
