// RUN: rm -rf %t

// Build PCH using A, with adjacent private module APrivate, which winds up being implicitly referenced
// RUN: %clang_cc1 -verify -fmodules -fimplicit-module-maps -fmodules-cache-path=%t -F %S/Inputs/implicit-private-canonical -emit-pch -o %t-A.pch %s -Wprivate-module

// Use the PCH with no explicit way to resolve APrivate, still pick it up by automatic second-chance search for "A" with "Private" appended
// RUN: %clang_cc1 -verify -fmodules -fimplicit-module-maps -fmodules-cache-path=%t -F %S/Inputs/implicit-private-canonical -include-pch %t-A.pch %s -fsyntax-only -Wprivate-module

// expected-no-diagnostics

#ifndef HEADER
#define HEADER
#import "A/aprivate.h"
const int *y = &APRIVATE;
#endif
