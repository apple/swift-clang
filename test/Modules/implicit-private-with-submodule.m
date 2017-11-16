// RUN: rm -rf %t
// Build PCH using A, with private submodule A.Private
// RUN: %clang_cc1 -verify -fmodules -fimplicit-module-maps -fmodules-cache-path=%t -F %S/Inputs/implicit-private-with-submodule -emit-pch -o %t-A.pch %s -Wprivate-module

// RUN: rm -rf %t
// Build PCH using A, with private submodule A.Private, check the fixit
// RUN: %clang_cc1 -fmodules -fimplicit-module-maps -fmodules-cache-path=%t -F %S/Inputs/implicit-private-with-submodule -emit-pch -o %t-A.pch %s -Wprivate-module -fdiagnostics-parseable-fixits 2>&1 | FileCheck %s

// expected-warning@Inputs/implicit-private-with-submodule/A.framework/Modules/module.private.modulemap:1{{private submodule 'A.Private' in private module map, expected top-level module}}
// expected-note@Inputs/implicit-private-with-submodule/A.framework/Modules/module.private.modulemap:1{{rename 'A.Private' to ensure it can be found by name}}
// CHECK: fix-it:"{{.*}}module.private.modulemap":{1:20-1:27}:"A_Private"

#ifndef HEADER
#define HEADER
#import "A/aprivate.h"
const int *y = &APRIVATE;
#endif
