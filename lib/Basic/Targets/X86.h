//===--- X86.h - Declare X86 target feature support -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares X86 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_X86_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_X86_H

#include "OSTargets.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

// X86 target abstract base class; x86-32 and x86-64 are very close, so
// most of the implementation can be shared.
class LLVM_LIBRARY_VISIBILITY X86TargetInfo : public TargetInfo {

  enum X86SSEEnum {
    NoSSE,
    SSE1,
    SSE2,
    SSE3,
    SSSE3,
    SSE41,
    SSE42,
    AVX,
    AVX2,
    AVX512F
  } SSELevel = NoSSE;
  enum MMX3DNowEnum {
    NoMMX3DNow,
    MMX,
    AMD3DNow,
    AMD3DNowAthlon
  } MMX3DNowLevel = NoMMX3DNow;
  enum XOPEnum { NoXOP, SSE4A, FMA4, XOP } XOPLevel = NoXOP;

  bool HasAES = false;
  bool HasPCLMUL = false;
  bool HasLZCNT = false;
  bool HasRDRND = false;
  bool HasFSGSBASE = false;
  bool HasBMI = false;
  bool HasBMI2 = false;
  bool HasPOPCNT = false;
  bool HasRTM = false;
  bool HasPRFCHW = false;
  bool HasRDSEED = false;
  bool HasADX = false;
  bool HasTBM = false;
  bool HasLWP = false;
  bool HasFMA = false;
  bool HasF16C = false;
  bool HasAVX512CD = false;
  bool HasAVX512VPOPCNTDQ = false;
  bool HasAVX512ER = false;
  bool HasAVX512PF = false;
  bool HasAVX512DQ = false;
  bool HasAVX512BW = false;
  bool HasAVX512VL = false;
  bool HasAVX512VBMI = false;
  bool HasAVX512IFMA = false;
  bool HasSHA = false;
  bool HasMPX = false;
  bool HasSGX = false;
  bool HasCX16 = false;
  bool HasFXSR = false;
  bool HasXSAVE = false;
  bool HasXSAVEOPT = false;
  bool HasXSAVEC = false;
  bool HasXSAVES = false;
  bool HasMWAITX = false;
  bool HasCLZERO = false;
  bool HasPKU = false;
  bool HasCLFLUSHOPT = false;
  bool HasCLWB = false;
  bool HasMOVBE = false;
  bool HasPREFETCHWT1 = false;

  /// \brief Enumeration of all of the X86 CPUs supported by Clang.
  ///
  /// Each enumeration represents a particular CPU supported by Clang. These
  /// loosely correspond to the options passed to '-march' or '-mtune' flags.
  enum CPUKind {
    CK_Generic,

    /// \name i386
    /// i386-generation processors.
    //@{
    CK_i386,
    //@}

    /// \name i486
    /// i486-generation processors.
    //@{
    CK_i486,
    CK_WinChipC6,
    CK_WinChip2,
    CK_C3,
    //@}

    /// \name i586
    /// i586-generation processors, P5 microarchitecture based.
    //@{
    CK_i586,
    CK_Pentium,
    CK_PentiumMMX,
    //@}

    /// \name i686
    /// i686-generation processors, P6 / Pentium M microarchitecture based.
    //@{
    CK_i686,
    CK_PentiumPro,
    CK_Pentium2,
    CK_Pentium3,
    CK_PentiumM,
    CK_C3_2,

    /// This enumerator is a bit odd, as GCC no longer accepts -march=yonah.
    /// Clang however has some logic to support this.
    // FIXME: Warn, deprecate, and potentially remove this.
    CK_Yonah,
    //@}

    /// \name Netburst
    /// Netburst microarchitecture based processors.
    //@{
    CK_Pentium4,
    CK_Prescott,
    CK_Nocona,
    //@}

    /// \name Core
    /// Core microarchitecture based processors.
    //@{
    CK_Core2,

    /// This enumerator, like \see CK_Yonah, is a bit odd. It is another
    /// codename which GCC no longer accepts as an option to -march, but Clang
    /// has some logic for recognizing it.
    // FIXME: Warn, deprecate, and potentially remove this.
    CK_Penryn,
    //@}

    /// \name Atom
    /// Atom processors
    //@{
    CK_Bonnell,
    CK_Silvermont,
    CK_Goldmont,
    //@}

    /// \name Nehalem
    /// Nehalem microarchitecture based processors.
    CK_Nehalem,

    /// \name Westmere
    /// Westmere microarchitecture based processors.
    CK_Westmere,

    /// \name Sandy Bridge
    /// Sandy Bridge microarchitecture based processors.
    CK_SandyBridge,

    /// \name Ivy Bridge
    /// Ivy Bridge microarchitecture based processors.
    CK_IvyBridge,

    /// \name Haswell
    /// Haswell microarchitecture based processors.
    CK_Haswell,

    /// \name Broadwell
    /// Broadwell microarchitecture based processors.
    CK_Broadwell,

    /// \name Skylake Client
    /// Skylake client microarchitecture based processors.
    CK_SkylakeClient,

    /// \name Skylake Server
    /// Skylake server microarchitecture based processors.
    CK_SkylakeServer,

    /// \name Cannonlake Client
    /// Cannonlake client microarchitecture based processors.
    CK_Cannonlake,

    /// \name Knights Landing
    /// Knights Landing processor.
    CK_KNL,

    /// \name Lakemont
    /// Lakemont microarchitecture based processors.
    CK_Lakemont,

    /// \name K6
    /// K6 architecture processors.
    //@{
    CK_K6,
    CK_K6_2,
    CK_K6_3,
    //@}

    /// \name K7
    /// K7 architecture processors.
    //@{
    CK_Athlon,
    CK_AthlonXP,
    //@}

    /// \name K8
    /// K8 architecture processors.
    //@{
    CK_K8,
    CK_K8SSE3,
    CK_AMDFAM10,
    //@}

    /// \name Bobcat
    /// Bobcat architecture processors.
    //@{
    CK_BTVER1,
    CK_BTVER2,
    //@}

    /// \name Bulldozer
    /// Bulldozer architecture processors.
    //@{
    CK_BDVER1,
    CK_BDVER2,
    CK_BDVER3,
    CK_BDVER4,
    //@}

    /// \name zen
    /// Zen architecture processors.
    //@{
    CK_ZNVER1,
    //@}

    /// This specification is deprecated and will be removed in the future.
    /// Users should prefer \see CK_K8.
    // FIXME: Warn on this when the CPU is set to it.
    //@{
    CK_x86_64,
    //@}

    /// \name Geode
    /// Geode processors.
    //@{
    CK_Geode
    //@}
  } CPU = CK_Generic;

  bool checkCPUKind(CPUKind Kind) const {
    // Perform any per-CPU checks necessary to determine if this CPU is
    // acceptable.
    // FIXME: This results in terrible diagnostics. Clang just says the CPU is
    // invalid without explaining *why*.
    switch (Kind) {
    case CK_Generic:
      // No processor selected!
      return false;

    case CK_i386:
    case CK_i486:
    case CK_WinChipC6:
    case CK_WinChip2:
    case CK_C3:
    case CK_i586:
    case CK_Pentium:
    case CK_PentiumMMX:
    case CK_i686:
    case CK_PentiumPro:
    case CK_Pentium2:
    case CK_Pentium3:
    case CK_PentiumM:
    case CK_Yonah:
    case CK_C3_2:
    case CK_Pentium4:
    case CK_Lakemont:
    case CK_Prescott:
    case CK_K6:
    case CK_K6_2:
    case CK_K6_3:
    case CK_Athlon:
    case CK_AthlonXP:
    case CK_Geode:
      // Only accept certain architectures when compiling in 32-bit mode.
      if (getTriple().getArch() != llvm::Triple::x86)
        return false;

      LLVM_FALLTHROUGH;
    case CK_Nocona:
    case CK_Core2:
    case CK_Penryn:
    case CK_Bonnell:
    case CK_Silvermont:
    case CK_Goldmont:
    case CK_Nehalem:
    case CK_Westmere:
    case CK_SandyBridge:
    case CK_IvyBridge:
    case CK_Haswell:
    case CK_Broadwell:
    case CK_SkylakeClient:
    case CK_SkylakeServer:
    case CK_Cannonlake:
    case CK_KNL:
    case CK_K8:
    case CK_K8SSE3:
    case CK_AMDFAM10:
    case CK_BTVER1:
    case CK_BTVER2:
    case CK_BDVER1:
    case CK_BDVER2:
    case CK_BDVER3:
    case CK_BDVER4:
    case CK_ZNVER1:
    case CK_x86_64:
      return true;
    }
    llvm_unreachable("Unhandled CPU kind");
  }

  CPUKind getCPUKind(StringRef CPU) const;

  enum FPMathKind { FP_Default, FP_SSE, FP_387 } FPMath = FP_Default;

public:
  X86TargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    LongDoubleFormat = &llvm::APFloat::x87DoubleExtended();
  }
  
  unsigned getFloatEvalMethod() const override {
    // X87 evaluates with 80 bits "long double" precision.
    return SSELevel == NoSSE ? 2 : 0;
  }

  ArrayRef<const char *> getGCCRegNames() const override;

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    return None;
  }

  ArrayRef<TargetInfo::AddlRegName> getGCCAddlRegNames() const override;

  bool validateCpuSupports(StringRef Name) const override;

  bool validateCpuIs(StringRef Name) const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &info) const override;

  bool validateGlobalRegisterVariable(StringRef RegName, unsigned RegSize,
                                      bool &HasSizeMismatch) const override {
    // esp and ebp are the only 32-bit registers the x86 backend can currently
    // handle.
    if (RegName.equals("esp") || RegName.equals("ebp")) {
      // Check that the register size is 32-bit.
      HasSizeMismatch = RegSize != 32;
      return true;
    }

    return false;
  }

  bool validateOutputSize(StringRef Constraint, unsigned Size) const override;

  bool validateInputSize(StringRef Constraint, unsigned Size) const override;

  virtual bool validateOperandSize(StringRef Constraint, unsigned Size) const;

  std::string convertConstraint(const char *&Constraint) const override;
  const char *getClobbers() const override {
    return "~{dirflag},~{fpsr},~{flags}";
  }

  StringRef getConstraintRegister(const StringRef &Constraint,
                                  const StringRef &Expression) const override {
    StringRef::iterator I, E;
    for (I = Constraint.begin(), E = Constraint.end(); I != E; ++I) {
      if (isalpha(*I))
        break;
    }
    if (I == E)
      return "";
    switch (*I) {
    // For the register constraints, return the matching register name
    case 'a':
      return "ax";
    case 'b':
      return "bx";
    case 'c':
      return "cx";
    case 'd':
      return "dx";
    case 'S':
      return "si";
    case 'D':
      return "di";
    // In case the constraint is 'r' we need to return Expression
    case 'r':
      return Expression;
    // Double letters Y<x> constraints
    case 'Y':
      if ((++I != E) && ((*I == '0') || (*I == 'z')))
        return "xmm0";
    default:
      break;
    }
    return "";
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;
  
  static void setSSELevel(llvm::StringMap<bool> &Features, X86SSEEnum Level,
                          bool Enabled);

  static void setMMXLevel(llvm::StringMap<bool> &Features, MMX3DNowEnum Level,
                          bool Enabled);

  static void setXOPLevel(llvm::StringMap<bool> &Features, XOPEnum Level,
                          bool Enabled);

  void setFeatureEnabled(llvm::StringMap<bool> &Features, StringRef Name,
                         bool Enabled) const override {
    setFeatureEnabledImpl(Features, Name, Enabled);
  }

  // This exists purely to cut down on the number of virtual calls in
  // initFeatureMap which calls this repeatedly.
  static void setFeatureEnabledImpl(llvm::StringMap<bool> &Features,
                                    StringRef Name, bool Enabled);

  bool
  initFeatureMap(llvm::StringMap<bool> &Features, DiagnosticsEngine &Diags,
                 StringRef CPU,
                 const std::vector<std::string> &FeaturesVec) const override;

  bool isValidFeatureName(StringRef Name) const override;

  bool hasFeature(StringRef Feature) const override;

  bool handleTargetFeatures(std::vector<std::string> &Features,
                            DiagnosticsEngine &Diags) override;

  StringRef getABI() const override {
    if (getTriple().getArch() == llvm::Triple::x86_64 && SSELevel >= AVX512F)
      return "avx512";
    if (getTriple().getArch() == llvm::Triple::x86_64 && SSELevel >= AVX)
      return "avx";
    if (getTriple().getArch() == llvm::Triple::x86 &&
        MMX3DNowLevel == NoMMX3DNow)
      return "no-mmx";
    return "";
  }

  bool isValidCPUName(StringRef Name) const override {
    return checkCPUKind(getCPUKind(Name));
  }

  bool setCPU(const std::string &Name) override {
    return checkCPUKind(CPU = getCPUKind(Name));
  }

  bool setFPMath(StringRef Name) override;

  CallingConvCheckResult checkCallingConvention(CallingConv CC) const override {
    // Most of the non-ARM calling conventions are i386 conventions.
    switch (CC) {
    case CC_X86ThisCall:
    case CC_X86FastCall:
    case CC_X86StdCall:
    case CC_X86VectorCall:
    case CC_X86RegCall:
    case CC_C:
    case CC_Swift:
    case CC_X86Pascal:
    case CC_IntelOclBicc:
    case CC_OpenCLKernel:
      return CCCR_OK;
    default:
      return CCCR_Warning;
    }
  }

  CallingConv getDefaultCallingConv(CallingConvMethodType MT) const override {
    return MT == CCMT_Member ? CC_X86ThisCall : CC_C;
  }

  bool hasSjLjLowering() const override { return true; }

  void setSupportedOpenCLOpts() override {
    getSupportedOpenCLOpts().supportAll();
  }
};

// X86-32 generic target
class LLVM_LIBRARY_VISIBILITY X86_32TargetInfo : public X86TargetInfo {
public:
  X86_32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : X86TargetInfo(Triple, Opts) {
    DoubleAlign = LongLongAlign = 32;
    LongDoubleWidth = 96;
    LongDoubleAlign = 32;
    SuitableAlign = 128;
    resetDataLayout("e-m:e-p:32:32-f64:32:64-f80:32-n8:16:32-S128");
    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    RegParmMax = 3;

    // Use fpret for all types.
    RealTypeUsesObjCFPRet =
        ((1 << TargetInfo::Float) | (1 << TargetInfo::Double) |
         (1 << TargetInfo::LongDouble));

    // x86-32 has atomics up to 8 bytes
    // FIXME: Check that we actually have cmpxchg8b before setting
    // MaxAtomicInlineWidth. (cmpxchg8b is an i586 instruction.)
    MaxAtomicPromoteWidth = MaxAtomicInlineWidth = 64;
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::CharPtrBuiltinVaList;
  }

  int getEHDataRegisterNumber(unsigned RegNo) const override {
    if (RegNo == 0)
      return 0;
    if (RegNo == 1)
      return 2;
    return -1;
  }

  bool validateOperandSize(StringRef Constraint, unsigned Size) const override {
    switch (Constraint[0]) {
    default:
      break;
    case 'R':
    case 'q':
    case 'Q':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'S':
    case 'D':
      return Size <= 32;
    case 'A':
      return Size <= 64;
    }

    return X86TargetInfo::validateOperandSize(Constraint, Size);
  }

  ArrayRef<Builtin::Info> getTargetBuiltins() const override;
};

class LLVM_LIBRARY_VISIBILITY NetBSDI386TargetInfo
    : public NetBSDTargetInfo<X86_32TargetInfo> {
public:
  NetBSDI386TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : NetBSDTargetInfo<X86_32TargetInfo>(Triple, Opts) {}

  unsigned getFloatEvalMethod() const override {
    unsigned Major, Minor, Micro;
    getTriple().getOSVersion(Major, Minor, Micro);
    // New NetBSD uses the default rounding mode.
    if (Major >= 7 || (Major == 6 && Minor == 99 && Micro >= 26) || Major == 0)
      return X86_32TargetInfo::getFloatEvalMethod();
    // NetBSD before 6.99.26 defaults to "double" rounding.
    return 1;
  }
};

class LLVM_LIBRARY_VISIBILITY OpenBSDI386TargetInfo
    : public OpenBSDTargetInfo<X86_32TargetInfo> {
public:
  OpenBSDI386TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : OpenBSDTargetInfo<X86_32TargetInfo>(Triple, Opts) {
    SizeType = UnsignedLong;
    IntPtrType = SignedLong;
    PtrDiffType = SignedLong;
  }
};

class LLVM_LIBRARY_VISIBILITY DarwinI386TargetInfo
    : public DarwinTargetInfo<X86_32TargetInfo> {
public:
  DarwinI386TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : DarwinTargetInfo<X86_32TargetInfo>(Triple, Opts) {
    LongDoubleWidth = 128;
    LongDoubleAlign = 128;
    SuitableAlign = 128;
    MaxVectorAlign = 256;
    // The watchOS simulator uses the builtin bool type for Objective-C.
    llvm::Triple T = llvm::Triple(Triple);
    if (T.isWatchOS())
      UseSignedCharForObjCBool = false;
    SizeType = UnsignedLong;
    IntPtrType = SignedLong;
    resetDataLayout("e-m:o-p:32:32-f64:32:64-f80:128-n8:16:32-S128");
    HasAlignMac68kSupport = true;
  }

  bool handleTargetFeatures(std::vector<std::string> &Features,
                            DiagnosticsEngine &Diags) override {
    if (!DarwinTargetInfo<X86_32TargetInfo>::handleTargetFeatures(Features,
                                                                  Diags))
      return false;
    // We now know the features we have: we can decide how to align vectors.
    MaxVectorAlign =
        hasFeature("avx512f") ? 512 : hasFeature("avx") ? 256 : 128;
    return true;
  }
};

// x86-32 Windows target
class LLVM_LIBRARY_VISIBILITY WindowsX86_32TargetInfo
    : public WindowsTargetInfo<X86_32TargetInfo> {
public:
  WindowsX86_32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : WindowsTargetInfo<X86_32TargetInfo>(Triple, Opts) {
    WCharType = UnsignedShort;
    DoubleAlign = LongLongAlign = 64;
    bool IsWinCOFF =
        getTriple().isOSWindows() && getTriple().isOSBinFormatCOFF();
    resetDataLayout(IsWinCOFF
                        ? "e-m:x-p:32:32-i64:64-f80:32-n8:16:32-a:0:32-S32"
                        : "e-m:e-p:32:32-i64:64-f80:32-n8:16:32-a:0:32-S32");
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    WindowsTargetInfo<X86_32TargetInfo>::getTargetDefines(Opts, Builder);
  }
};

// x86-32 Windows Visual Studio target
class LLVM_LIBRARY_VISIBILITY MicrosoftX86_32TargetInfo
    : public WindowsX86_32TargetInfo {
public:
  MicrosoftX86_32TargetInfo(const llvm::Triple &Triple,
                            const TargetOptions &Opts)
      : WindowsX86_32TargetInfo(Triple, Opts) {
    LongDoubleWidth = LongDoubleAlign = 64;
    LongDoubleFormat = &llvm::APFloat::IEEEdouble();
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    WindowsX86_32TargetInfo::getTargetDefines(Opts, Builder);
    WindowsX86_32TargetInfo::getVisualStudioDefines(Opts, Builder);
    // The value of the following reflects processor type.
    // 300=386, 400=486, 500=Pentium, 600=Blend (default)
    // We lost the original triple, so we use the default.
    Builder.defineMacro("_M_IX86", "600");
  }
};

// x86-32 MinGW target
class LLVM_LIBRARY_VISIBILITY MinGWX86_32TargetInfo
    : public WindowsX86_32TargetInfo {
public:
  MinGWX86_32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : WindowsX86_32TargetInfo(Triple, Opts) {
    HasFloat128 = true;
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    WindowsX86_32TargetInfo::getTargetDefines(Opts, Builder);
    DefineStd(Builder, "WIN32", Opts);
    DefineStd(Builder, "WINNT", Opts);
    Builder.defineMacro("_X86_");
    addMinGWDefines(Opts, Builder);
  }
};

// x86-32 Cygwin target
class LLVM_LIBRARY_VISIBILITY CygwinX86_32TargetInfo : public X86_32TargetInfo {
public:
  CygwinX86_32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : X86_32TargetInfo(Triple, Opts) {
    WCharType = UnsignedShort;
    DoubleAlign = LongLongAlign = 64;
    resetDataLayout("e-m:x-p:32:32-i64:64-f80:32-n8:16:32-a:0:32-S32");
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    X86_32TargetInfo::getTargetDefines(Opts, Builder);
    Builder.defineMacro("_X86_");
    Builder.defineMacro("__CYGWIN__");
    Builder.defineMacro("__CYGWIN32__");
    addCygMingDefines(Opts, Builder);
    DefineStd(Builder, "unix", Opts);
    if (Opts.CPlusPlus)
      Builder.defineMacro("_GNU_SOURCE");
  }
};

// x86-32 Haiku target
class LLVM_LIBRARY_VISIBILITY HaikuX86_32TargetInfo
    : public HaikuTargetInfo<X86_32TargetInfo> {
public:
  HaikuX86_32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : HaikuTargetInfo<X86_32TargetInfo>(Triple, Opts) {}

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    HaikuTargetInfo<X86_32TargetInfo>::getTargetDefines(Opts, Builder);
    Builder.defineMacro("__INTEL__");
  }
};

// X86-32 MCU target
class LLVM_LIBRARY_VISIBILITY MCUX86_32TargetInfo : public X86_32TargetInfo {
public:
  MCUX86_32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : X86_32TargetInfo(Triple, Opts) {
    LongDoubleWidth = 64;
    LongDoubleFormat = &llvm::APFloat::IEEEdouble();
    resetDataLayout("e-m:e-p:32:32-i64:32-f64:32-f128:32-n8:16:32-a:0:32-S32");
    WIntType = UnsignedInt;
  }

  CallingConvCheckResult checkCallingConvention(CallingConv CC) const override {
    // On MCU we support only C calling convention.
    return CC == CC_C ? CCCR_OK : CCCR_Warning;
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    X86_32TargetInfo::getTargetDefines(Opts, Builder);
    Builder.defineMacro("__iamcu");
    Builder.defineMacro("__iamcu__");
  }

  bool allowsLargerPreferedTypeAlignment() const override { return false; }
};

// x86-32 RTEMS target
class LLVM_LIBRARY_VISIBILITY RTEMSX86_32TargetInfo : public X86_32TargetInfo {
public:
  RTEMSX86_32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : X86_32TargetInfo(Triple, Opts) {
    SizeType = UnsignedLong;
    IntPtrType = SignedLong;
    PtrDiffType = SignedLong;
  }
  
  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    X86_32TargetInfo::getTargetDefines(Opts, Builder);
    Builder.defineMacro("__INTEL__");
    Builder.defineMacro("__rtems__");
  }
};

// x86-64 generic target
class LLVM_LIBRARY_VISIBILITY X86_64TargetInfo : public X86TargetInfo {
public:
  X86_64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : X86TargetInfo(Triple, Opts) {
    const bool IsX32 = getTriple().getEnvironment() == llvm::Triple::GNUX32;
    bool IsWinCOFF =
        getTriple().isOSWindows() && getTriple().isOSBinFormatCOFF();
    LongWidth = LongAlign = PointerWidth = PointerAlign = IsX32 ? 32 : 64;
    LongDoubleWidth = 128;
    LongDoubleAlign = 128;
    LargeArrayMinWidth = 128;
    LargeArrayAlign = 128;
    SuitableAlign = 128;
    SizeType = IsX32 ? UnsignedInt : UnsignedLong;
    PtrDiffType = IsX32 ? SignedInt : SignedLong;
    IntPtrType = IsX32 ? SignedInt : SignedLong;
    IntMaxType = IsX32 ? SignedLongLong : SignedLong;
    Int64Type = IsX32 ? SignedLongLong : SignedLong;
    RegParmMax = 6;

    // Pointers are 32-bit in x32.
    resetDataLayout(IsX32
                        ? "e-m:e-p:32:32-i64:64-f80:128-n8:16:32:64-S128"
                        : IsWinCOFF ? "e-m:w-i64:64-f80:128-n8:16:32:64-S128"
                                    : "e-m:e-i64:64-f80:128-n8:16:32:64-S128");

    // Use fpret only for long double.
    RealTypeUsesObjCFPRet = (1 << TargetInfo::LongDouble);

    // Use fp2ret for _Complex long double.
    ComplexLongDoubleUsesFP2Ret = true;

    // Make __builtin_ms_va_list available.
    HasBuiltinMSVaList = true;

    // x86-64 has atomics up to 16 bytes.
    MaxAtomicPromoteWidth = 128;
    MaxAtomicInlineWidth = 64;
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::X86_64ABIBuiltinVaList;
  }

  int getEHDataRegisterNumber(unsigned RegNo) const override {
    if (RegNo == 0)
      return 0;
    if (RegNo == 1)
      return 1;
    return -1;
  }

  CallingConvCheckResult checkCallingConvention(CallingConv CC) const override {
    switch (CC) {
    case CC_C:
    case CC_Swift:
    case CC_X86VectorCall:
    case CC_IntelOclBicc:
    case CC_Win64:
    case CC_PreserveMost:
    case CC_PreserveAll:
    case CC_X86RegCall:
    case CC_OpenCLKernel:
      return CCCR_OK;
    default:
      return CCCR_Warning;
    }
  }

  CallingConv getDefaultCallingConv(CallingConvMethodType MT) const override {
    return CC_C;
  }

  // for x32 we need it here explicitly
  bool hasInt128Type() const override { return true; }

  unsigned getUnwindWordWidth() const override { return 64; }
  
  unsigned getRegisterWidth() const override { return 64; }

  bool validateGlobalRegisterVariable(StringRef RegName, unsigned RegSize,
                                      bool &HasSizeMismatch) const override {
    // rsp and rbp are the only 64-bit registers the x86 backend can currently
    // handle.
    if (RegName.equals("rsp") || RegName.equals("rbp")) {
      // Check that the register size is 64-bit.
      HasSizeMismatch = RegSize != 64;
      return true;
    }

    // Check if the register is a 32-bit register the backend can handle.
    return X86TargetInfo::validateGlobalRegisterVariable(RegName, RegSize,
                                                         HasSizeMismatch);
  }

  void setMaxAtomicWidth() override {
    if (hasFeature("cx16"))
      MaxAtomicInlineWidth = 128;
    return;
  }

  ArrayRef<Builtin::Info> getTargetBuiltins() const override;
};

// x86-64 Windows target
class LLVM_LIBRARY_VISIBILITY WindowsX86_64TargetInfo
    : public WindowsTargetInfo<X86_64TargetInfo> {
public:
  WindowsX86_64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : WindowsTargetInfo<X86_64TargetInfo>(Triple, Opts) {
    WCharType = UnsignedShort;
    LongWidth = LongAlign = 32;
    DoubleAlign = LongLongAlign = 64;
    IntMaxType = SignedLongLong;
    Int64Type = SignedLongLong;
    SizeType = UnsignedLongLong;
    PtrDiffType = SignedLongLong;
    IntPtrType = SignedLongLong;
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    WindowsTargetInfo<X86_64TargetInfo>::getTargetDefines(Opts, Builder);
    Builder.defineMacro("_WIN64");
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::CharPtrBuiltinVaList;
  }

  CallingConvCheckResult checkCallingConvention(CallingConv CC) const override {
    switch (CC) {
    case CC_X86StdCall:
    case CC_X86ThisCall:
    case CC_X86FastCall:
      return CCCR_Ignore;
    case CC_C:
    case CC_X86VectorCall:
    case CC_IntelOclBicc:
    case CC_X86_64SysV:
    case CC_Swift:
    case CC_X86RegCall:
    case CC_OpenCLKernel:
      return CCCR_OK;
    default:
      return CCCR_Warning;
    }
  }
};

// x86-64 Windows Visual Studio target
class LLVM_LIBRARY_VISIBILITY MicrosoftX86_64TargetInfo
    : public WindowsX86_64TargetInfo {
public:
  MicrosoftX86_64TargetInfo(const llvm::Triple &Triple,
                            const TargetOptions &Opts)
      : WindowsX86_64TargetInfo(Triple, Opts) {
    LongDoubleWidth = LongDoubleAlign = 64;
    LongDoubleFormat = &llvm::APFloat::IEEEdouble();
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    WindowsX86_64TargetInfo::getTargetDefines(Opts, Builder);
    WindowsX86_64TargetInfo::getVisualStudioDefines(Opts, Builder);
    Builder.defineMacro("_M_X64", "100");
    Builder.defineMacro("_M_AMD64", "100");
  }
};

// x86-64 MinGW target
class LLVM_LIBRARY_VISIBILITY MinGWX86_64TargetInfo
    : public WindowsX86_64TargetInfo {
public:
  MinGWX86_64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : WindowsX86_64TargetInfo(Triple, Opts) {
    // Mingw64 rounds long double size and alignment up to 16 bytes, but sticks
    // with x86 FP ops. Weird.
    LongDoubleWidth = LongDoubleAlign = 128;
    LongDoubleFormat = &llvm::APFloat::x87DoubleExtended();
    HasFloat128 = true;
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    WindowsX86_64TargetInfo::getTargetDefines(Opts, Builder);
    DefineStd(Builder, "WIN64", Opts);
    Builder.defineMacro("__MINGW64__");
    addMinGWDefines(Opts, Builder);

    // GCC defines this macro when it is using __gxx_personality_seh0.
    if (!Opts.SjLjExceptions)
      Builder.defineMacro("__SEH__");
  }
};

// x86-64 Cygwin target
class LLVM_LIBRARY_VISIBILITY CygwinX86_64TargetInfo : public X86_64TargetInfo {
public:
  CygwinX86_64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : X86_64TargetInfo(Triple, Opts) {
    TLSSupported = false;
    WCharType = UnsignedShort;
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    X86_64TargetInfo::getTargetDefines(Opts, Builder);
    Builder.defineMacro("__x86_64__");
    Builder.defineMacro("__CYGWIN__");
    Builder.defineMacro("__CYGWIN64__");
    addCygMingDefines(Opts, Builder);
    DefineStd(Builder, "unix", Opts);
    if (Opts.CPlusPlus)
      Builder.defineMacro("_GNU_SOURCE");

    // GCC defines this macro when it is using __gxx_personality_seh0.
    if (!Opts.SjLjExceptions)
      Builder.defineMacro("__SEH__");
  }
};

class LLVM_LIBRARY_VISIBILITY DarwinX86_64TargetInfo
    : public DarwinTargetInfo<X86_64TargetInfo> {
public:
  DarwinX86_64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : DarwinTargetInfo<X86_64TargetInfo>(Triple, Opts) {
    Int64Type = SignedLongLong;
    // The 64-bit iOS simulator uses the builtin bool type for Objective-C.
    llvm::Triple T = llvm::Triple(Triple);
    if (T.isiOS())
      UseSignedCharForObjCBool = false;
    resetDataLayout("e-m:o-i64:64-f80:128-n8:16:32:64-S128");
  }

  bool handleTargetFeatures(std::vector<std::string> &Features,
                            DiagnosticsEngine &Diags) override {
    if (!DarwinTargetInfo<X86_64TargetInfo>::handleTargetFeatures(Features,
                                                                  Diags))
      return false;
    // We now know the features we have: we can decide how to align vectors.
    MaxVectorAlign =
        hasFeature("avx512f") ? 512 : hasFeature("avx") ? 256 : 128;
    return true;
  }
};

class LLVM_LIBRARY_VISIBILITY OpenBSDX86_64TargetInfo
    : public OpenBSDTargetInfo<X86_64TargetInfo> {
public:
  OpenBSDX86_64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : OpenBSDTargetInfo<X86_64TargetInfo>(Triple, Opts) {
    IntMaxType = SignedLongLong;
    Int64Type = SignedLongLong;
  }
};

// x86_32 Android target
class LLVM_LIBRARY_VISIBILITY AndroidX86_32TargetInfo
    : public LinuxTargetInfo<X86_32TargetInfo> {
public:
  AndroidX86_32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : LinuxTargetInfo<X86_32TargetInfo>(Triple, Opts) {
    SuitableAlign = 32;
    LongDoubleWidth = 64;
    LongDoubleFormat = &llvm::APFloat::IEEEdouble();
  }
};

// x86_64 Android target
class LLVM_LIBRARY_VISIBILITY AndroidX86_64TargetInfo
    : public LinuxTargetInfo<X86_64TargetInfo> {
public:
  AndroidX86_64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : LinuxTargetInfo<X86_64TargetInfo>(Triple, Opts) {
    LongDoubleFormat = &llvm::APFloat::IEEEquad();
  }

  bool useFloat128ManglingForLongDouble() const override { return true; }
};
} // namespace targets
} // namespace clang
#endif // LLVM_CLANG_LIB_BASIC_TARGETS_X86_H
