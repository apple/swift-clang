// RUN: rm -rf %t && mkdir -p %t

// RUN: %clang_cc1 -fmodules -fimplicit-module-maps -fmodules-cache-path=%t/ModulesCache -fapinotes-modules  -fblocks -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s -ast-dump -ast-dump-filter 'TestProperties::' | FileCheck -check-prefix=CHECK -check-prefix=CHECK-4 %s
// RUN: %clang_cc1 -fmodules -fimplicit-module-maps -fmodules-cache-path=%t/ModulesCache -fapinotes-modules  -fblocks -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s -ast-dump -ast-dump-filter 'TestProperties::' -fapinotes-swift-version=3 | FileCheck -check-prefix=CHECK -check-prefix=CHECK-3 %s

// I know, FileChecking an AST dump is brittle. However, the attributes being
// tested aren't used for anything by Clang, and don't even have a spelling.

@import VersionedKit;

// CHECK-LABEL: ObjCPropertyDecl {{.+}} accessorsOnly 'id'
// CHECK-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-NOT: Attr

// CHECK-LABEL: ObjCPropertyDecl {{.+}} accessorsOnlyForClass 'id'
// CHECK-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-NOT: Attr

// CHECK-LABEL: ObjCPropertyDecl {{.+}} accessorsOnlyInVersion3 'id'
// CHECK-3-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-4-NEXT: SwiftVersionedAttr {{.+}} 3.0{{$}}
// CHECK-4-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-NOT: Attr

// CHECK-LABEL: ObjCPropertyDecl {{.+}} accessorsOnlyForClassInVersion3 'id'
// CHECK-3-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-4-NEXT: SwiftVersionedAttr {{.+}} 3.0{{$}}
// CHECK-4-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-NOT: Attr

// CHECK-LABEL: ObjCPropertyDecl {{.+}} accessorsOnlyExceptInVersion3 'id'
// CHECK-3-NEXT: SwiftVersionedAttr {{.+}} Implicit 3.0 IsReplacedByActive{{$}}
// CHECK-3-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-4-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-4-NEXT: SwiftVersionedRemovalAttr {{.+}} Implicit 3.0 {{[0-9]+}}
// CHECK-NOT: Attr

// CHECK-LABEL: ObjCPropertyDecl {{.+}} accessorsOnlyForClassExceptInVersion3 'id'
// CHECK-3-NEXT: SwiftVersionedAttr {{.+}} Implicit 3.0 IsReplacedByActive{{$}}
// CHECK-3-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-4-NEXT: SwiftImportPropertyAsAccessorsAttr {{.+}} Implicit
// CHECK-4-NEXT: SwiftVersionedRemovalAttr {{.+}} Implicit 3.0 {{[0-9]+}}
// CHECK-NOT: Attr

// CHECK-LABEL: Decl
