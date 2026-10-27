// RUN: %clang_cc1 %s -triple=powerpc-gekko-ibm-kuribo-eabi -emit-llvm-only -fdump-record-layouts > %t
// RUN: FileCheck --check-prefix=CHECK-1 %s < %t
// RUN: FileCheck --check-prefix=CHECK-2 %s < %t
// RUN: FileCheck --check-prefix=CHECK-3 %s < %t
// RUN: FileCheck --check-prefix=CHECK-4 %s < %t
// RUN: FileCheck --check-prefix=CHECK-5 %s < %t
// RUN: FileCheck --check-prefix=CHECK-6 %s < %t
// RUN: FileCheck --check-prefix=CHECK-7 %s < %t
// RUN: FileCheck --check-prefix=CHECK-8 %s < %t
// RUN: FileCheck --check-prefix=CHECK-9 %s < %t
// RUN: FileCheck --check-prefix=CHECK-10 %s < %t

// First virtual function decl resides as the first entry
// so its offset should be at 0
// CHECK-1:        0 | struct VtableAt0
// CHECK-1-NEXT:   0 |   (VtableAt0 vtable pointer)
// CHECK-1-NEXT:   4 |   int _
// CHECK-1-NEXT:     | [sizeof=8, dsize=8, align=4,
// CHECK-1-NEXT:     |  nvsize=8, nvalign=4]
struct VtableAt0 {
  virtual ~VtableAt0() {}
  virtual void a() {}
  int _;
};

// First virtual function decl resides as the second entry after type int
// so its offset should be at 4
// CHECK-2:        0 | struct VtableAt4
// CHECK-2-NEXT:   0 |   int _
// CHECK-2-NEXT:   4 |   (VtableAt4 vtable pointer)
// CHECK-2-NEXT:     | [sizeof=8, dsize=8, align=4,
// CHECK-2-NEXT:     |  nvsize=8, nvalign=4]
struct VtableAt4 {
  int _;
  virtual ~VtableAt4() {}
  virtual void d() {}
};

// CHECK-3:        0 | struct VtableInheritedAt0
// CHECK-3-NEXT:   0 |   struct VtableAt0 (primary base)
// CHECK-3-NEXT:   0 |     (VtableAt0 vtable pointer)
// CHECK-3-NEXT:   4 |     int _
// CHECK-3-NEXT:     | [sizeof=8, dsize=8, align=4,
// CHECK-3-NEXT:     |  nvsize=8, nvalign=4]
struct VtableInheritedAt0 : VtableAt0 {
  virtual ~VtableInheritedAt0() {}
  virtual void a() override {}
  virtual void b() {}
};

// CHECK-4:        0 | struct VtableInheritedAt4
// CHECK-4-NEXT:   0 |   struct VtableAt4 (primary base)
// CHECK-4-NEXT:   0 |     int _
// CHECK-4-NEXT:   4 |     (VtableAt4 vtable pointer)
// CHECK-4-NEXT:     | [sizeof=8, dsize=8, align=4,
// CHECK-4-NEXT:     |  nvsize=8, nvalign=4]
struct VtableInheritedAt4 : VtableAt4 {
  virtual ~VtableInheritedAt4() {}
  virtual void d() override {}
  virtual void e() {}
};

// CHECK-5:        0 | struct VtableMultiInheritedAt0_4
// CHECK-5-NEXT:   0 |   struct VtableAt0 (primary base)
// CHECK-5-NEXT:   0 |     (VtableAt0 vtable pointer)
// CHECK-5-NEXT:   4 |     int _
// CHECK-5-NEXT:   8 |   struct VtableAt4 (base)
// CHECK-5-NEXT:   8 |     int _
// CHECK-5-NEXT:  12 |     (VtableAt4 vtable pointer)
// CHECK-5-NEXT:     | [sizeof=16, dsize=16, align=4,
// CHECK-5-NEXT:     |  nvsize=16, nvalign=4]
struct VtableMultiInheritedAt0_4 : VtableAt0, VtableAt4 {
  virtual void f() {}
};

// CHECK-6:        0 | struct VtableMultiInheritedAt4_0
// CHECK-6-NEXT:   0 |   struct VtableAt4 (primary base)
// CHECK-6-NEXT:   0 |     int _
// CHECK-6-NEXT:   4 |     (VtableAt4 vtable pointer)
// CHECK-6-NEXT:   8 |   struct VtableAt0 (base)
// CHECK-6-NEXT:   8 |     (VtableAt0 vtable pointer)
// CHECK-6-NEXT:  12 |     int _
// CHECK-6-NEXT:     | [sizeof=16, dsize=16, align=4,
// CHECK-6-NEXT:     |  nvsize=16, nvalign=4]
struct VtableMultiInheritedAt4_0 : VtableAt4, VtableAt0 {
  virtual void f() {}
};

// CHECK-7:        0 | struct VtableComplexPtrs
// CHECK-7-NEXT:   0 |   struct VtableInheritedAt0 * vt0
// CHECK-7-NEXT:   4 |   struct VtableInheritedAt4 * vt4
// CHECK-7-NEXT:   8 |   (VtableComplexPtrs vtable pointer)
// CHECK-7-NEXT:     | [sizeof=12, dsize=12, align=4,
// CHECK-7-NEXT:     |  nvsize=12, nvalign=4]
struct VtableComplexPtrs {
  VtableInheritedAt0 *vt0;
  VtableInheritedAt4 *vt4;

  virtual void g() {}
};

// CHECK-8:   0 | struct VtableComplexInherited
// CHECK-8-NEXT:   0 |   struct VtableInheritedAt4 (primary base)
// CHECK-8-NEXT:   0 |     struct VtableAt4 (primary base)
// CHECK-8-NEXT:   0 |       int _
// CHECK-8-NEXT:   4 |       (VtableAt4 vtable pointer)
// CHECK-8-NEXT:   8 |   struct VtableMultiInheritedAt4_0 (base)
// CHECK-8-NEXT:   8 |     struct VtableAt4 (primary base)
// CHECK-8-NEXT:   8 |       int _
// CHECK-8-NEXT:  12 |       (VtableAt4 vtable pointer)
// CHECK-8-NEXT:  16 |     struct VtableAt0 (base)
// CHECK-8-NEXT:  16 |       (VtableAt0 vtable pointer)
// CHECK-8-NEXT:  20 |       int _
// CHECK-8-NEXT:  24 |   struct VtableComplexPtrs (virtual base)
// CHECK-8-NEXT:  24 |     struct VtableInheritedAt0 * vt0
// CHECK-8-NEXT:  28 |     struct VtableInheritedAt4 * vt4
// CHECK-8-NEXT:  32 |     (VtableComplexPtrs vtable pointer)
// CHECK-8-NEXT:     | [sizeof=36, dsize=36, align=4,
// CHECK-8-NEXT:     |  nvsize=24, nvalign=4]
struct VtableComplexInherited : VtableInheritedAt4,
                                VtableMultiInheritedAt4_0,
                                virtual VtableComplexPtrs {
  virtual void a() override {}
  virtual void e() override {}
};

// CHECK-9:        0 | struct VtableComplex
// CHECK-9-NEXT:   0 |   struct VtableComplexPtrs ptrs
// CHECK-9-NEXT:   0 |     struct VtableInheritedAt0 * vt0
// CHECK-9-NEXT:   4 |     struct VtableInheritedAt4 * vt4
// CHECK-9-NEXT:   8 |     (VtableComplexPtrs vtable pointer)
// CHECK-9-NEXT:  12 |   (VtableComplex vtable pointer)
// CHECK-9-NEXT:     | [sizeof=16, dsize=16, align=4,
// CHECK-9-NEXT:     |  nvsize=16, nvalign=4]
struct VtableComplex {
  VtableComplexPtrs ptrs;
  virtual void i() {}
};

// CHECK-10:       0 | struct VtableExtremeInherited
// CHECK-10-NEXT:  0 |   struct VtableComplexInherited (primary base)
// CHECK-10-NEXT:  0 |     struct VtableInheritedAt4 (primary base)
// CHECK-10-NEXT:  0 |       struct VtableAt4 (primary base)
// CHECK-10-NEXT:  0 |         int _
// CHECK-10-NEXT:  4 |         (VtableAt4 vtable pointer)
// CHECK-10-NEXT:  8 |     struct VtableMultiInheritedAt4_0 (base)
// CHECK-10-NEXT:  8 |       struct VtableAt4 (primary base)
// CHECK-10-NEXT:  8 |         int _
// CHECK-10-NEXT: 12 |         (VtableAt4 vtable pointer)
// CHECK-10-NEXT: 16 |       struct VtableAt0 (base)
// CHECK-10-NEXT: 16 |         (VtableAt0 vtable pointer)
// CHECK-10-NEXT: 20 |         int _
// CHECK-10-NEXT: 24 |   struct VtableComplex (virtual base)
// CHECK-10-NEXT: 24 |     struct VtableComplexPtrs ptrs
// CHECK-10-NEXT: 24 |       struct VtableInheritedAt0 * vt0
// CHECK-10-NEXT: 28 |       struct VtableInheritedAt4 * vt4
// CHECK-10-NEXT: 32 |       (VtableComplexPtrs vtable pointer)
// CHECK-10-NEXT: 36 |     (VtableComplex vtable pointer)
// CHECK-10-NEXT: 40 |   struct VtableComplexPtrs (virtual base)
// CHECK-10-NEXT: 40 |     struct VtableInheritedAt0 * vt0
// CHECK-10-NEXT: 44 |     struct VtableInheritedAt4 * vt4
// CHECK-10-NEXT: 48 |     (VtableComplexPtrs vtable pointer)
// CHECK-10-NEXT:    | [sizeof=52, dsize=52, align=4,
// CHECK-10-NEXT:    |  nvsize=24, nvalign=4]
struct VtableExtremeInherited : virtual VtableComplex,
                                VtableComplexInherited {
  virtual void i() override {};
  virtual void g() override {};
};

void f() {
  VtableExtremeInherited vt;
}