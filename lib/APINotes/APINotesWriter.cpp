//===--- APINotesWriter.cpp - API Notes Writer --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the \c APINotesWriter class that writes out
// source API notes data providing additional information about source
// code as a separate input, such as the non-nil/nilable annotations
// for method parameters.
//
//===----------------------------------------------------------------------===//
#include "clang/APINotes/APINotesWriter.h"
#include "APINotesFormat.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/OnDiskHashTable.h"
#include "llvm/Support/raw_ostream.h"
#include <tuple>
#include <vector>
using namespace clang;
using namespace api_notes;
using namespace llvm::support;

class APINotesWriter::Implementation {
  /// Mapping from strings to identifier IDs.
  llvm::StringMap<IdentifierID> IdentifierIDs;

  /// Mapping from selectors to selector ID.
  llvm::DenseMap<StoredObjCSelector, SelectorID> SelectorIDs;

  /// Scratch space for bitstream writing.
  SmallVector<uint64_t, 64> ScratchRecord;

public:
  /// The name of the module
  std::string ModuleName;

  bool SwiftInferImportAsMember = false;

  /// Information about Objective-C contexts (classes or protocols).
  ///
  /// Indexed by the identifier ID and a bit indication whether we're looking
  /// for a class (0) or protocol (1) and provides both the context ID and
  /// information describing the context within that module.
  llvm::DenseMap<std::pair<unsigned, char>,
                 std::pair<unsigned, ObjCContextInfo>> ObjCContexts;

  /// Mapping from context IDs to the identifier ID holding the name.
  llvm::DenseMap<unsigned, unsigned> ObjCContextNames;

  /// Information about Objective-C properties.
  ///
  /// Indexed by the context ID, property name, and whether this is an
  /// instance property.
  llvm::DenseMap<std::tuple<unsigned, unsigned, char>, ObjCPropertyInfo> 
    ObjCProperties;

  /// Information about Objective-C methods.
  ///
  /// Indexed by the context ID, selector ID, and Boolean (stored as a
  /// char) indicating whether this is a class or instance method.
  llvm::DenseMap<std::tuple<unsigned, unsigned, char>, ObjCMethodInfo> 
    ObjCMethods;

  /// Information about global variables.
  ///
  /// Indexed by the identifier ID.
  llvm::DenseMap<unsigned, GlobalVariableInfo> GlobalVariables;

  /// Information about global functions.
  ///
  /// Indexed by the identifier ID.
  llvm::DenseMap<unsigned, GlobalFunctionInfo> GlobalFunctions;

  /// Information about enumerators.
  ///
  /// Indexed by the identifier ID.
  llvm::DenseMap<unsigned, EnumConstantInfo> EnumConstants;

  /// Information about tags.
  ///
  /// Indexed by the identifier ID.
  llvm::DenseMap<unsigned, TagInfo> Tags;

  /// Information about typedefs.
  ///
  /// Indexed by the identifier ID.
  llvm::DenseMap<unsigned, TypedefInfo> Typedefs;

  /// Retrieve the ID for the given identifier.
  IdentifierID getIdentifier(StringRef identifier) {
    if (identifier.empty())
      return 0;

    auto known = IdentifierIDs.find(identifier);
    if (known != IdentifierIDs.end())
      return known->second;

    // Add to the identifier table.
    known = IdentifierIDs.insert({identifier, IdentifierIDs.size() + 1}).first;
    return known->second;
  }

  /// Retrieve the ID for the given selector.
  SelectorID getSelector(ObjCSelectorRef selectorRef) {
    // Translate the selector reference into a stored selector.
    StoredObjCSelector selector;
    selector.NumPieces = selectorRef.NumPieces;
    selector.Identifiers.reserve(selectorRef.Identifiers.size());
    for (auto piece : selectorRef.Identifiers) {
      selector.Identifiers.push_back(getIdentifier(piece));
    }

    // Look for the stored selector.
    auto known = SelectorIDs.find(selector);
    if (known != SelectorIDs.end())
      return known->second;

    // Add to the selector table.
    known = SelectorIDs.insert({selector, SelectorIDs.size()}).first;
    return known->second;
  }

  void writeToStream(llvm::raw_ostream &os);

private:
  void writeBlockInfoBlock(llvm::BitstreamWriter &writer);
  void writeControlBlock(llvm::BitstreamWriter &writer);
  void writeIdentifierBlock(llvm::BitstreamWriter &writer);
  void writeObjCContextBlock(llvm::BitstreamWriter &writer);
  void writeObjCPropertyBlock(llvm::BitstreamWriter &writer);
  void writeObjCMethodBlock(llvm::BitstreamWriter &writer);
  void writeObjCSelectorBlock(llvm::BitstreamWriter &writer);
  void writeGlobalVariableBlock(llvm::BitstreamWriter &writer);
  void writeGlobalFunctionBlock(llvm::BitstreamWriter &writer);
  void writeEnumConstantBlock(llvm::BitstreamWriter &writer);
  void writeTagBlock(llvm::BitstreamWriter &writer);
  void writeTypedefBlock(llvm::BitstreamWriter &writer);
};

/// Record the name of a block.
static void emitBlockID(llvm::BitstreamWriter &out, unsigned ID,
                        StringRef name,
                        SmallVectorImpl<unsigned char> &nameBuffer) {
  SmallVector<unsigned, 1> idBuffer;
  idBuffer.push_back(ID);
  out.EmitRecord(llvm::bitc::BLOCKINFO_CODE_SETBID, idBuffer);

  // Emit the block name if present.
  if (name.empty())
    return;
  nameBuffer.resize(name.size());
  memcpy(nameBuffer.data(), name.data(), name.size());
  out.EmitRecord(llvm::bitc::BLOCKINFO_CODE_BLOCKNAME, nameBuffer);
}

/// Record the name of a record within a block.
static void emitRecordID(llvm::BitstreamWriter &out, unsigned ID,
                         StringRef name,
                         SmallVectorImpl<unsigned char> &nameBuffer) {
  assert(ID < 256 && "can't fit record ID in next to name");
  nameBuffer.resize(name.size()+1);
  nameBuffer[0] = ID;
  memcpy(nameBuffer.data()+1, name.data(), name.size());
  out.EmitRecord(llvm::bitc::BLOCKINFO_CODE_SETRECORDNAME, nameBuffer);
}

void APINotesWriter::Implementation::writeBlockInfoBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, llvm::bitc::BLOCKINFO_BLOCK_ID, 2);  

  SmallVector<unsigned char, 64> nameBuffer;
#define BLOCK(X) emitBlockID(writer, X ## _ID, #X, nameBuffer)
#define BLOCK_RECORD(K, X) emitRecordID(writer, K::X, #X, nameBuffer)

  BLOCK(CONTROL_BLOCK);
  BLOCK_RECORD(control_block, METADATA);
  BLOCK_RECORD(control_block, MODULE_NAME);

  BLOCK(IDENTIFIER_BLOCK);
  BLOCK_RECORD(identifier_block, IDENTIFIER_DATA);

  BLOCK(OBJC_CONTEXT_BLOCK);
  BLOCK_RECORD(objc_context_block, OBJC_CONTEXT_DATA);

  BLOCK(OBJC_PROPERTY_BLOCK);
  BLOCK_RECORD(objc_property_block, OBJC_PROPERTY_DATA);

  BLOCK(OBJC_METHOD_BLOCK);
  BLOCK_RECORD(objc_method_block, OBJC_METHOD_DATA);

  BLOCK(OBJC_SELECTOR_BLOCK);
  BLOCK_RECORD(objc_selector_block, OBJC_SELECTOR_DATA);

  BLOCK(GLOBAL_VARIABLE_BLOCK);
  BLOCK_RECORD(global_variable_block, GLOBAL_VARIABLE_DATA);

  BLOCK(GLOBAL_FUNCTION_BLOCK);
  BLOCK_RECORD(global_function_block, GLOBAL_FUNCTION_DATA);
#undef BLOCK
#undef BLOCK_RECORD
}

void APINotesWriter::Implementation::writeControlBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, CONTROL_BLOCK_ID, 3);
  control_block::MetadataLayout metadata(writer);
  metadata.emit(ScratchRecord, VERSION_MAJOR, VERSION_MINOR);

  control_block::ModuleNameLayout moduleName(writer);
  moduleName.emit(ScratchRecord, ModuleName);

  if (SwiftInferImportAsMember) {
    control_block::ModuleOptionsLayout moduleOptions(writer);
    moduleOptions.emit(ScratchRecord, SwiftInferImportAsMember);
  }
}

namespace {
  /// Used to serialize the on-disk identifier table.
  class IdentifierTableInfo {
  public:
    using key_type = StringRef;
    using key_type_ref = key_type;
    using data_type = IdentifierID;
    using data_type_ref = const data_type &;
    using hash_value_type = uint32_t;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return llvm::HashString(key);
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = key.size();
      uint32_t dataLength = sizeof(uint32_t);
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      out << key;
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(data);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeIdentifierBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, IDENTIFIER_BLOCK_ID, 3);

  if (IdentifierIDs.empty())
    return;

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<IdentifierTableInfo> generator;
    for (auto &entry : IdentifierIDs)
      generator.insert(entry.first(), entry.second);

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  identifier_block::IdentifierDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Retrieve the serialized size of the given CommonEntityInfo, for use in
  /// on-disk hash tables.
  static unsigned getCommonEntityInfoSize(const CommonEntityInfo &info) {
    return 5 + info.UnavailableMsg.size() + info.SwiftName.size();
  }

  /// Emit a serialized representation of the common entity information.
  static void emitCommonEntityInfo(raw_ostream &out,
                                   const CommonEntityInfo &info) {
    endian::Writer<little> writer(out);
    writer.write<uint8_t>(info.SwiftPrivate << 2
                          | info.Unavailable << 1 
                          | info.UnavailableInSwift);
    writer.write<uint16_t>(info.UnavailableMsg.size());
    out.write(info.UnavailableMsg.c_str(), info.UnavailableMsg.size());
    writer.write<uint16_t>(info.SwiftName.size());
    out.write(info.SwiftName.c_str(), info.SwiftName.size());
  }

  // Retrieve the serialized size of the given CommonTypeInfo, for use
  // in on-disk hash tables.
  static unsigned getCommonTypeInfoSize(const CommonTypeInfo &info) {
    return 2 + info.getSwiftBridge().size() +
           2 + info.getNSErrorDomain().size() +
           getCommonEntityInfoSize(info);
  }

  /// Emit a serialized representation of the common type information.
  static void emitCommonTypeInfo(raw_ostream &out, const CommonTypeInfo &info) {
    emitCommonEntityInfo(out, info);
    endian::Writer<little> writer(out);
    writer.write<uint16_t>(info.getSwiftBridge().size());
    out.write(info.getSwiftBridge().c_str(), info.getSwiftBridge().size());
    writer.write<uint16_t>(info.getNSErrorDomain().size());
    out.write(info.getNSErrorDomain().c_str(), info.getNSErrorDomain().size());
  }

  /// Used to serialize the on-disk Objective-C context table.
  class ObjCContextTableInfo {
  public:
    using key_type = std::pair<unsigned, char>; // identifier ID, is-protocol
    using key_type_ref = key_type;
    using data_type = std::pair<unsigned, ObjCContextInfo>;
    using data_type_ref = const data_type &;
    using hash_value_type = size_t;
    using offset_type = unsigned;

    /// The number of bytes in a data entry.
    static const unsigned dataBytes = 3;

    hash_value_type ComputeHash(key_type_ref key) {
      return static_cast<size_t>(llvm::hash_value(key));
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(uint32_t) + 1;
      uint32_t dataLength = sizeof(uint32_t)
                          + getCommonTypeInfoSize(data.second)
                          + dataBytes;
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(key.first);
      writer.write<uint8_t>(key.second);
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(data.first);

      emitCommonTypeInfo(out, data.second);

      // FIXME: Inefficient representation.
      uint8_t bytes[dataBytes] = { 0, 0, 0 };
      if (auto nullable = data.second.getDefaultNullability()) {
        bytes[0] = 1;
        bytes[1] = static_cast<uint8_t>(*nullable);
      } else {
        // Nothing to do.
      }
      bytes[2] = data.second.hasDesignatedInits();

      out.write(reinterpret_cast<const char *>(bytes), dataBytes);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeObjCContextBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, OBJC_CONTEXT_BLOCK_ID, 3);

  if (ObjCContexts.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<ObjCContextTableInfo> generator;
    for (auto &entry : ObjCContexts)
      generator.insert(entry.first, entry.second);

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  objc_context_block::ObjCContextDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Retrieve the serialized size of the given VariableInfo, for use in
  /// on-disk hash tables.
  static unsigned getVariableInfoSize(const VariableInfo &info) {
    return 2 + getCommonEntityInfoSize(info);
  }

  /// Emit a serialized representation of the variable information.
  static void emitVariableInfo(raw_ostream &out, const VariableInfo &info) {
    emitCommonEntityInfo(out, info);

    uint8_t bytes[2] = { 0, 0 };
    if (auto nullable = info.getNullability()) {
      bytes[0] = 1;
      bytes[1] = static_cast<uint8_t>(*nullable);
    } else {
      // Nothing to do.
    }

    out.write(reinterpret_cast<const char *>(bytes), 2);
  }

  /// Used to serialize the on-disk Objective-C property table.
  class ObjCPropertyTableInfo {
  public:
    // (class ID, name ID, isInstance)
    using key_type = std::tuple<unsigned, unsigned, char>;
    using key_type_ref = key_type;
    using data_type = ObjCPropertyInfo;
    using data_type_ref = const data_type &;
    using hash_value_type = size_t;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return static_cast<size_t>(llvm::hash_value(key));
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
      uint32_t dataLength = getVariableInfoSize(data);
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(std::get<0>(key));
      writer.write<uint32_t>(std::get<1>(key));
      writer.write<uint8_t>(std::get<2>(key));
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      emitVariableInfo(out, data);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeObjCPropertyBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, OBJC_PROPERTY_BLOCK_ID, 3);

  if (ObjCProperties.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<ObjCPropertyTableInfo> generator;
    for (auto &entry : ObjCProperties)
      generator.insert(entry.first, entry.second);

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  objc_property_block::ObjCPropertyDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Retrieve the serialized size of the given FunctionInfo, for use in
  /// on-disk hash tables.
  static unsigned getFunctionInfoSize(const FunctionInfo &info) {
    return 2 + sizeof(uint64_t) + getCommonEntityInfoSize(info) + 
           2 + info.Params.size() * 1;
  }

  /// Emit a serialized representation of the function information.
  static void emitFunctionInfo(raw_ostream &out, const FunctionInfo &info) {
    emitCommonEntityInfo(out, info);

    endian::Writer<little> writer(out);
    writer.write<uint8_t>(info.NullabilityAudited);
    writer.write<uint8_t>(info.NumAdjustedNullable);
    writer.write<uint64_t>(info.NullabilityPayload);

    // Parameters.
    writer.write<uint16_t>(info.Params.size());
    for (const auto &pi : info.Params) {
      uint8_t payload = pi.isNoEscape();

      auto nullability = pi.getNullability();
      payload = (payload << 1) | nullability.hasValue();

      payload = payload << 2;
      if (nullability)
        payload |= static_cast<uint8_t>(*nullability);
      writer.write<uint8_t>(payload);
    }
  }

  /// Used to serialize the on-disk Objective-C method table.
  class ObjCMethodTableInfo {
  public:
    // (class ID, selector ID, is-instance)
    using key_type = std::tuple<unsigned, unsigned, char>; 
    using key_type_ref = key_type;
    using data_type = ObjCMethodInfo;
    using data_type_ref = const data_type &;
    using hash_value_type = size_t;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return llvm::hash_combine(std::get<0>(key), 
                                std::get<1>(key), 
                                std::get<2>(key));
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(uint32_t) + sizeof(uint32_t) + 1;
      uint32_t dataLength = getFunctionInfoSize(data) + 3;
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(std::get<0>(key));
      writer.write<uint32_t>(std::get<1>(key));
      writer.write<uint8_t>(std::get<2>(key));
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      emitFunctionInfo(out, data);

      endian::Writer<little> writer(out);

      // FIXME: Inefficient representation
      writer.write<uint8_t>(data.DesignatedInit);
      writer.write<uint8_t>(data.FactoryAsInit);
      writer.write<uint8_t>(data.Required);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeObjCMethodBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, OBJC_METHOD_BLOCK_ID, 3);

  if (ObjCMethods.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<ObjCMethodTableInfo> generator;
    for (auto &entry : ObjCMethods) {
      generator.insert(entry.first, entry.second);
    }

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  objc_method_block::ObjCMethodDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Used to serialize the on-disk Objective-C selector table.
  class ObjCSelectorTableInfo {
  public:
    using key_type = StoredObjCSelector;
    using key_type_ref = const key_type &;
    using data_type = SelectorID;
    using data_type_ref = data_type;
    using hash_value_type = unsigned;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return llvm::DenseMapInfo<StoredObjCSelector>::getHashValue(key);
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(uint16_t) 
                         + sizeof(uint32_t) * key.Identifiers.size();
      uint32_t dataLength = sizeof(uint32_t);
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(key.NumPieces);
      for (auto piece : key.Identifiers) {
        writer.write<uint32_t>(piece);
      }
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(data);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeObjCSelectorBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, OBJC_SELECTOR_BLOCK_ID, 3);

  if (SelectorIDs.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<ObjCSelectorTableInfo> generator;
    for (auto &entry : SelectorIDs)
      generator.insert(entry.first, entry.second);

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  objc_selector_block::ObjCSelectorDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Used to serialize the on-disk global variable table.
  class GlobalVariableTableInfo {
  public:
    using key_type = unsigned; // name ID
    using key_type_ref = key_type;
    using data_type = GlobalVariableInfo;
    using data_type_ref = const data_type &;
    using hash_value_type = size_t;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return static_cast<size_t>(llvm::hash_value(key));
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(uint32_t);
      uint32_t dataLength = getVariableInfoSize(data);
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(key);
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      emitVariableInfo(out, data);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeGlobalVariableBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, GLOBAL_VARIABLE_BLOCK_ID, 3);

  if (GlobalVariables.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<GlobalVariableTableInfo> generator;
    for (auto &entry : GlobalVariables)
      generator.insert(entry.first, entry.second);

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  global_variable_block::GlobalVariableDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Used to serialize the on-disk global function table.
  class GlobalFunctionTableInfo {
  public:
    using key_type = unsigned; // name ID
    using key_type_ref = key_type;
    using data_type = GlobalFunctionInfo;
    using data_type_ref = const data_type &;
    using hash_value_type = size_t;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return llvm::hash_value(key);
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(uint32_t);
      uint32_t dataLength = getFunctionInfoSize(data);
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(key);
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      emitFunctionInfo(out, data);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeGlobalFunctionBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, GLOBAL_FUNCTION_BLOCK_ID, 3);

  if (GlobalFunctions.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<GlobalFunctionTableInfo> generator;
    for (auto &entry : GlobalFunctions) {
      generator.insert(entry.first, entry.second);
    }

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  global_function_block::GlobalFunctionDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Used to serialize the on-disk global enum constant.
  class EnumConstantTableInfo {
  public:
    using key_type = unsigned; // name ID
    using key_type_ref = key_type;
    using data_type = EnumConstantInfo;
    using data_type_ref = const data_type &;
    using hash_value_type = size_t;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return static_cast<size_t>(llvm::hash_value(key));
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(uint32_t);
      uint32_t dataLength = getCommonEntityInfoSize(data);
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<uint32_t>(key);
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      emitCommonEntityInfo(out, data);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeEnumConstantBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, ENUM_CONSTANT_BLOCK_ID, 3);

  if (EnumConstants.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<EnumConstantTableInfo> generator;
    for (auto &entry : EnumConstants)
      generator.insert(entry.first, entry.second);

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  enum_constant_block::EnumConstantDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Used to serialize the on-disk tag table.
  class TagTableInfo {
  public:
    using key_type = unsigned; // name ID
    using key_type_ref = key_type;
    using data_type = TagInfo;
    using data_type_ref = const data_type &;
    using hash_value_type = size_t;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return static_cast<size_t>(llvm::hash_value(key));
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(IdentifierID);
      uint32_t dataLength = getCommonTypeInfoSize(data);
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<IdentifierID>(key);
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      emitCommonTypeInfo(out, data);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeTagBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, TAG_BLOCK_ID, 3);

  if (Tags.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<TagTableInfo> generator;
    for (auto &entry : Tags)
      generator.insert(entry.first, entry.second);

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  tag_block::TagDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

namespace {
  /// Used to serialize the on-disk typedef table.
  class TypedefTableInfo {
  public:
    using key_type = unsigned; // name ID
    using key_type_ref = key_type;
    using data_type = TypedefInfo;
    using data_type_ref = const data_type &;
    using hash_value_type = size_t;
    using offset_type = unsigned;

    hash_value_type ComputeHash(key_type_ref key) {
      return static_cast<size_t>(llvm::hash_value(key));
    }

    std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                    key_type_ref key,
                                                    data_type_ref data) {
      uint32_t keyLength = sizeof(IdentifierID);
      uint32_t dataLength = getCommonTypeInfoSize(data);
      endian::Writer<little> writer(out);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
    }

    void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer<little> writer(out);
      writer.write<IdentifierID>(key);
    }

    void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                  unsigned len) {
      emitCommonTypeInfo(out, data);
    }
  };
} // end anonymous namespace

void APINotesWriter::Implementation::writeTypedefBlock(
       llvm::BitstreamWriter &writer) {
  BCBlockRAII restoreBlock(writer, TYPEDEF_BLOCK_ID, 3);

  if (Typedefs.empty())
    return;  

  llvm::SmallString<4096> hashTableBlob;
  uint32_t tableOffset;
  {
    llvm::OnDiskChainedHashTableGenerator<TypedefTableInfo> generator;
    for (auto &entry : Typedefs)
      generator.insert(entry.first, entry.second);

    llvm::raw_svector_ostream blobStream(hashTableBlob);
    // Make sure that no bucket is at offset 0
    endian::Writer<little>(blobStream).write<uint32_t>(0);
    tableOffset = generator.Emit(blobStream);
  }

  typedef_block::TypedefDataLayout layout(writer);
  layout.emit(ScratchRecord, tableOffset, hashTableBlob);
}

void APINotesWriter::Implementation::writeToStream(llvm::raw_ostream &os) {
  // Write the API notes file into a buffer.
  SmallVector<char, 0> buffer;
  {
    llvm::BitstreamWriter writer(buffer);

    // Emit the signature.
    for (unsigned char byte : API_NOTES_SIGNATURE)
      writer.Emit(byte, 8);

    // Emit the blocks.
    writeBlockInfoBlock(writer);
    writeControlBlock(writer);
    writeIdentifierBlock(writer);
    writeObjCContextBlock(writer);
    writeObjCPropertyBlock(writer);
    writeObjCMethodBlock(writer);
    writeObjCSelectorBlock(writer);
    writeGlobalVariableBlock(writer);
    writeGlobalFunctionBlock(writer);
    writeEnumConstantBlock(writer);
    writeTagBlock(writer);
    writeTypedefBlock(writer);
  }

  // Write the buffer to the stream.
  os.write(buffer.data(), buffer.size());
  os.flush();
}

APINotesWriter::APINotesWriter(StringRef moduleName)
  : Impl(*new Implementation)
{
  Impl.ModuleName = moduleName;
}

APINotesWriter::~APINotesWriter() {
  delete &Impl;
}


void APINotesWriter::writeToStream(raw_ostream &os) {
  Impl.writeToStream(os);
}

ContextID APINotesWriter::addObjCClass(StringRef name,
                                       const ObjCContextInfo &info) {
  IdentifierID classID = Impl.getIdentifier(name);

  std::pair<unsigned, char> key(classID, 0);
  auto known = Impl.ObjCContexts.find(key);
  if (known != Impl.ObjCContexts.end()) {
    known->second.second |= info;
  } else {
    unsigned nextID = Impl.ObjCContexts.size() + 1;

    known = Impl.ObjCContexts.insert(
              std::make_pair(key, std::make_pair(nextID, info)))
              .first;

    Impl.ObjCContextNames[nextID] = classID;
  }

  return ContextID(known->second.first);
}

ContextID APINotesWriter::addObjCProtocol(StringRef name,
                                          const ObjCContextInfo &info) {
  IdentifierID protocolID = Impl.getIdentifier(name);

  std::pair<unsigned, char> key(protocolID, 1);
  auto known = Impl.ObjCContexts.find(key);
  if (known != Impl.ObjCContexts.end()) {
    known->second.second |= info;
  } else {
    unsigned nextID = Impl.ObjCContexts.size() + 1;

    known = Impl.ObjCContexts.insert(
              std::make_pair(key, std::make_pair(nextID, info)))
              .first;

    Impl.ObjCContextNames[nextID] = protocolID;
  }

  return ContextID(known->second.first);
}
void APINotesWriter::addObjCProperty(ContextID contextID, StringRef name,
                                     bool isInstance,
                                     const ObjCPropertyInfo &info) {
  IdentifierID nameID = Impl.getIdentifier(name);
  assert(!Impl.ObjCProperties.count({contextID.Value, nameID, isInstance}));
  Impl.ObjCProperties[{contextID.Value, nameID, isInstance}] = info;
}

void APINotesWriter::addObjCMethod(ContextID contextID,
                                   ObjCSelectorRef selector,
                                   bool isInstanceMethod,
                                   const ObjCMethodInfo &info) {
  SelectorID selectorID = Impl.getSelector(selector);
  auto key = std::tuple<unsigned, unsigned, char>{
      contextID.Value, selectorID, isInstanceMethod};
  assert(!Impl.ObjCMethods.count(key));
  Impl.ObjCMethods[key] = info;

  // If this method is a designated initializer, update the class to note that
  // it has designated initializers.
  if (info.DesignatedInit) {
    assert(Impl.ObjCContexts.count({Impl.ObjCContextNames[contextID.Value],
                                    (char)0}));
    Impl.ObjCContexts[{Impl.ObjCContextNames[contextID.Value], (char)0}]
      .second.setHasDesignatedInits(true);
  }
}

void APINotesWriter::addGlobalVariable(llvm::StringRef name,
                                       const GlobalVariableInfo &info) {
  IdentifierID variableID = Impl.getIdentifier(name);
  assert(!Impl.GlobalVariables.count(variableID));
  Impl.GlobalVariables[variableID] = info;
}

void APINotesWriter::addGlobalFunction(llvm::StringRef name,
                                       const GlobalFunctionInfo &info) {
  IdentifierID nameID = Impl.getIdentifier(name);
  assert(!Impl.GlobalFunctions.count(nameID));
  Impl.GlobalFunctions[nameID] = info;
}

void APINotesWriter::addEnumConstant(llvm::StringRef name,
                                     const EnumConstantInfo &info) {
  IdentifierID enumConstantID = Impl.getIdentifier(name);
  assert(!Impl.EnumConstants.count(enumConstantID));
  Impl.EnumConstants[enumConstantID] = info;
}

void APINotesWriter::addTag(llvm::StringRef name, const TagInfo &info) {
  IdentifierID tagID = Impl.getIdentifier(name);
  assert(!Impl.Tags.count(tagID));
  Impl.Tags[tagID] = info;
}

void APINotesWriter::addTypedef(llvm::StringRef name, const TypedefInfo &info) {
  IdentifierID typedefID = Impl.getIdentifier(name);
  assert(!Impl.Typedefs.count(typedefID));
  Impl.Typedefs[typedefID] = info;
}

void APINotesWriter::addModuleOptions(ModuleOptions opts) {
  Impl.SwiftInferImportAsMember = opts.SwiftInferImportAsMember;
}

