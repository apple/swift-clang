// RUN: rm -rf %t && mkdir -p %t

// Build and check the unversioned module file.
// RUN: %clang_cc1 -fmodules -fblocks -fimplicit-module-maps -fmodules-cache-path=%t/ModulesCache/Unversioned -fdisable-module-hash -fapinotes-modules -fapinotes-cache-path=%t/APINotesCache -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s
// RUN: %clang_cc1 -ast-print %t/ModulesCache/Unversioned/VersionedKit.pcm | FileCheck -check-prefix=CHECK-UNVERSIONED %s
// RUN: %clang_cc1 -fmodules -fblocks -fimplicit-module-maps -fmodules-cache-path=%t/ModulesCache/Unversioned -fdisable-module-hash -fapinotes-modules -fapinotes-cache-path=%t/APINotesCache -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s -ast-dump -ast-dump-filter 'DUMP' | FileCheck -check-prefix=CHECK-DUMP -check-prefix=CHECK-UNVERSIONED-DUMP %s

// Build and check the versioned module file.
// RUN: %clang_cc1 -fmodules -fblocks -fimplicit-module-maps -fmodules-cache-path=%t/ModulesCache/Versioned -fdisable-module-hash -fapinotes-modules -fapinotes-cache-path=%t/APINotesCache -fapinotes-swift-version=3 -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s
// RUN: %clang_cc1 -ast-print %t/ModulesCache/Versioned/VersionedKit.pcm | FileCheck -check-prefix=CHECK-VERSIONED %s
// RUN: %clang_cc1 -fmodules -fblocks -fimplicit-module-maps -fmodules-cache-path=%t/ModulesCache/Versioned -fdisable-module-hash -fapinotes-modules -fapinotes-cache-path=%t/APINotesCache -fapinotes-swift-version=3 -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s -ast-dump -ast-dump-filter 'DUMP' | FileCheck -check-prefix=CHECK-DUMP -check-prefix=CHECK-VERSIONED-DUMP %s

#import <VersionedKit/VersionedKit.h>

// CHECK-UNVERSIONED: void moveToPointDUMP(double x, double y) __attribute__((swift_name("moveTo(x:y:)")));
// CHECK-VERSIONED: void moveToPointDUMP(double x, double y) __attribute__((swift_name("moveTo(a:b:)")));

// CHECK-DUMP-LABEL: Dumping moveToPointDUMP
// CHECK-VERSIONED-DUMP: SwiftVersionedAttr {{.+}} Implicit 0
// CHECK-VERSIONED-DUMP-NEXT: SwiftNameAttr {{.+}} "moveTo(x:y:)"
// CHECK-VERSIONED-DUMP-NEXT: SwiftNameAttr {{.+}} Implicit "moveTo(a:b:)"
// CHECK-UNVERSIONED-DUMP: SwiftNameAttr {{.+}} "moveTo(x:y:)"
// CHECK-UNVERSIONED-DUMP-NEXT: SwiftVersionedAttr {{.+}} Implicit 3.0
// CHECK-UNVERSIONED-DUMP-NEXT: SwiftNameAttr {{.+}} Implicit "moveTo(a:b:)"

// CHECK-DUMP-LABEL: Dumping TestGenericDUMP
// CHECK-VERSIONED-DUMP: SwiftImportAsNonGenericAttr {{.+}} Implicit
// CHECK-UNVERSIONED-DUMP: SwiftVersionedAttr {{.+}} Implicit 3.0
// CHECK-UNVERSIONED-DUMP-NEXT: SwiftImportAsNonGenericAttr {{.+}} Implicit

// CHECK-DUMP-NOT: Dumping

// CHECK-UNVERSIONED: void acceptClosure(void (^block)(void) __attribute__((noescape)));
// CHECK-VERSIONED: void acceptClosure(void (^block)(void));

// CHECK-UNVERSIONED:      enum MyErrorCode {
// CHECK-UNVERSIONED-NEXT:     MyErrorCodeFailed = 1
// CHECK-UNVERSIONED-NEXT: } __attribute__((ns_error_domain(MyErrorDomain)));

// CHECK-UNVERSIONED: __attribute__((swift_bridge("MyValueType")))
// CHECK-UNVERSIONED: @interface MyReferenceType

// CHECK-UNVERSIONED: void privateFunc() __attribute__((swift_private));

// CHECK-UNVERSIONED: typedef double MyDoubleWrapper __attribute__((swift_wrapper("struct")));

// CHECK-VERSIONED:      enum MyErrorCode {
// CHECK-VERSIONED-NEXT:     MyErrorCodeFailed = 1
// CHECK-VERSIONED-NEXT: };

// CHECK-VERSIONED-NOT: __attribute__((swift_bridge("MyValueType")))
// CHECK-VERSIONED: @interface MyReferenceType

// CHECK-VERSIONED: void privateFunc();

// CHECK-VERSIONED: typedef double MyDoubleWrapper;
