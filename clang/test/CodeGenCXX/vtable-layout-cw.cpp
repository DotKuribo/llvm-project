// RUN: %clang_cc1 %s -triple=powerpc-gekko-ibm-kuribo-eabi -emit-llvm-only -fdump-vtable-layouts > %t
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
// RUN: FileCheck --check-prefix=CHECK-11 %s < %t
// RUN: FileCheck --check-prefix=CHECK-12 %s < %t
// RUN: FileCheck --check-prefix=CHECK-13 %s < %t
// RUN: FileCheck --check-prefix=CHECK-14 %s < %t
// RUN: FileCheck --check-prefix=CHECK-15 %s < %t
// RUN: FileCheck --check-prefix=CHECK-16 %s < %t

namespace Test1 {
// CHECK-1:      Vtable for 'Test1::A' (3 entries).
// CHECK-1-NEXT:   0 | Test1::A RTTI
// CHECK-1-NEXT:   1 | offset_to_top (0)
// CHECK-1-NEXT:       -- (Test1::A, 0) vtable address --
// CHECK-1-NEXT:   2 | void Test1::A::f()
//
// CHECK-1:      VTable indices for 'Test1::A' (1 entries).
// CHECK-1-NEXT:   0 | void Test1::A::f()
struct A {
  virtual void f();
};
void A::f() { }

}


namespace Test2 {

// This is a smoke test of the vtable dumper.
// CHECK-2:      Vtable for 'Test2::A' (8 entries).
// CHECK-2-NEXT:   0 | Test2::A RTTI
// CHECK-2-NEXT:   1 | offset_to_top (0)
// CHECK-2-NEXT:       -- (Test2::A, 0) vtable address --
// CHECK-2-NEXT:   2 | void Test2::A::f()
// CHECK-2-NEXT:   3 | void Test2::A::f() const
// CHECK-2-NEXT:   4 | Test2::A *Test2::A::g(int)
// CHECK-2-NEXT:   5 | Test2::A::~A() [deleting]
// CHECK-2-NEXT:   6 | void Test2::A::h()
// CHECK-2-NEXT:   7 | Test2::A &Test2::A::operator=(const Test2::A &)
//
// CHECK-2:      VTable indices for 'Test2::A' (6 entries).
// CHECK-2-NEXT:   0 | void Test2::A::f()
// CHECK-2-NEXT:   1 | void Test2::A::f() const
// CHECK-2-NEXT:   2 | Test2::A *Test2::A::g(int)
// CHECK-2-NEXT:   3 | Test2::A::~A() [deleting]
// CHECK-2-NEXT:   4 | void Test2::A::h()
// CHECK-2-NEXT:   5 | Test2::A &Test2::A::operator=(const Test2::A &)
struct A {
  virtual void f();
  virtual void f() const;
  
  virtual A* g(int a);
  virtual ~A();
  virtual void h();
  virtual A& operator=(const A&);
};
void A::f() { }

// Another simple vtable dumper test.

// CHECK-3:     Vtable for 'Test2::B' (5 entries).
// CHECK-3-NEXT:  0 | Test2::B RTTI
// CHECK-3-NEXT:  1 | offset_to_top (0)
// CHECK-3-NEXT:    -- (Test2::B, 0) vtable address --
// CHECK-3-NEXT:  2 | void Test2::B::f()
// CHECK-3-NEXT:  3 | void Test2::B::g() [pure]
// CHECK-3-NEXT:  4 | Test2::B::~B() [deleting] [pure]
//
// CHECK-3:     VTable indices for 'Test2::B' (3 entries).
// CHECK-3-NEXT:  0 | void Test2::B::f()
// CHECK-3-NEXT:  1 | void Test2::B::g()
// CHECK-3-NEXT:  2 | Test2::B::~B() [deleting]
struct B {
  virtual void f();
  virtual void g() = 0;
  virtual ~B() = 0;
};
void B::f() { }

}


namespace Test3 {

// If a function in a derived class overrides a function in a primary base,
// then the function should not have an entry in the derived class (unless the return
// value requires adjusting).

// CHECK-4:      Vtable for 'Test3::A' (3 entries).
// CHECK-4-NEXT:   0 | Test3::A RTTI
// CHECK-4-NEXT:   1 | offset_to_top (0)
// CHECK-4-NEXT:       -- (Test3::A, 0) vtable address --
// CHECK-4-NEXT:   2 | void Test3::A::f()
//
// CHECK-4:      VTable indices for 'Test3::A' (1 entries).
// CHECK-4-NEXT:   0 | void Test3::A::f()
struct A {
  virtual void f();
};
void A::f() { } 

// CHECK-5:     Vtable for 'Test3::B' (4 entries).
// CHECK-5-NEXT:  0 | Test3::B RTTI
// CHECK-5-NEXT:  1 | offset_to_top (0)
// CHECK-5-NEXT:      -- (Test3::A, 0) vtable address --
// CHECK-5-NEXT:      -- (Test3::B, 0) vtable address --
// CHECK-5-NEXT:  2 | void Test3::B::f()
// CHECK-5-NEXT:  3 | void Test3::B::g()
//
// CHECK-5:     VTable indices for 'Test3::B' (2 entries).
// CHECK-5-NEXT:  0 | void Test3::B::f()
// CHECK-5-NEXT:  1 | void Test3::B::g()
struct B : A {
  virtual void f();
  virtual void g();
};
void B::f() { }

// CHECK-6:     Vtable for 'Test3::C' (5 entries).
// CHECK-6-NEXT:  0 | Test3::C RTTI
// CHECK-6-NEXT:  1 | offset_to_top (0)
// CHECK-6-NEXT:     -- (Test3::A, 0) vtable address --
// CHECK-6-NEXT:     -- (Test3::C, 0) vtable address --
// CHECK-6-NEXT:  2 | void Test3::A::f()
// CHECK-6-NEXT:  3 | void Test3::C::g()
// CHECK-6-NEXT:  4 | void Test3::C::h()
//
// CHECK-6:     VTable indices for 'Test3::C' (2 entries).
// CHECK-6-NEXT:  1 | void Test3::C::g()
// CHECK-6-NEXT:  2 | void Test3::C::h()
struct C : A {
  virtual void g();
  virtual void h();
};
void C::g() { }

// CHECK-7:     Vtable for 'Test3::D' (5 entries).
// CHECK-7-NEXT:  0 | Test3::D RTTI
// CHECK-7-NEXT:  1 | offset_to_top (0)
// CHECK-7-NEXT:     -- (Test3::A, 0) vtable address --
// CHECK-7-NEXT:     -- (Test3::B, 0) vtable address --
// CHECK-7-NEXT:     -- (Test3::D, 0) vtable address --
// CHECK-7-NEXT:  2 | void Test3::D::f()
// CHECK-7-NEXT:  3 | void Test3::D::g()
// CHECK-7-NEXT:  4 | void Test3::D::h()
//
// CHECK-7:     VTable indices for 'Test3::D' (3 entries).
// CHECK-7-NEXT:  0 | void Test3::D::f()
// CHECK-7-NEXT:  1 | void Test3::D::g()
// CHECK-7-NEXT:  2 | void Test3::D::h()
struct D : B {
  virtual void f();
  virtual void g();
  virtual void h();
};

void D::f() { } 
}

namespace Test4 {


// Test non-virtual result adjustments.

struct R1 { int r1; };
struct R2 { int r2; };
struct R3 : R1, R2 { int r3; };

struct A {
  virtual R2 *f();
};

// CHECK-8:     Vtable for 'Test4::B' (4 entries).
// CHECK-8-NEXT:  0 | Test4::B RTTI
// CHECK-8-NEXT:  1 | offset_to_top (0)
// CHECK-8-NEXT:      -- (Test4::A, 0) vtable address --
// CHECK-8-NEXT:      -- (Test4::B, 0) vtable address --
// CHECK-8-NEXT:  2 | Test4::R3 *Test4::B::f()
// CHECK-8-NEXT:      [return adjustment: 4 non-virtual]
// CHECK-8-NEXT:  3 | Test4::R3 *Test4::B::f()
//
// CHECK-8:     VTable indices for 'Test4::B' (1 entries).
// CHECK-8-NEXT:  1 | Test4::R3 *Test4::B::f()
struct B : A {
  virtual R3 *f();
};
R3 *B::f() { return 0; }

// Test virtual result adjustments.
struct V1 { int v1; };
struct V2 : virtual V1 { int v1; };

struct C {
  virtual V1 *f(); 
};

// CHECK-9:     Vtable for 'Test4::D' (4 entries).
// CHECK-9-NEXT:   0 | Test4::D RTTI
// CHECK-9-NEXT:   1 | offset_to_top (0)
// CHECK-9-NEXT:       -- (Test4::C, 0) vtable address --
// CHECK-9-NEXT:       -- (Test4::D, 0) vtable address --
// CHECK-9-NEXT:   2 | Test4::V2 *Test4::D::f()
// CHECK-9-NEXT:       [return adjustment: 0 non-virtual, -12 vbase offset offset]
// CHECK-9-NEXT:   3 | Test4::V2 *Test4::D::f()
//
// CHECK-9:     VTable indices for 'Test4::D' (1 entries).
// CHECK-9-NEXT:   1 | Test4::V2 *Test4::D::f()
struct D : C {
  virtual V2 *f();
};
V2 *D::f() { return 0; };

// Virtual result adjustments with an additional non-virtual adjustment.
struct V3 : virtual R3 { int r3; };

// CHECK-10:     Vtable for 'Test4::E' (4 entries).
// CHECK-10-NEXT:   0 | Test4::E RTTI
// CHECK-10-NEXT:   1 | offset_to_top (0)
// CHECK-10-NEXT:       -- (Test4::A, 0) vtable address --
// CHECK-10-NEXT:       -- (Test4::E, 0) vtable address --
// CHECK-10-NEXT:   2 | Test4::V3 *Test4::E::f()
// CHECK-10-NEXT:       [return adjustment: 4 non-virtual, -12 vbase offset offset]
// CHECK-10-NEXT:   3 | Test4::V3 *Test4::E::f()
//
// CHECK-10:     VTable indices for 'Test4::E' (1 entries).
// CHECK-10-NEXT:   1 | Test4::V3 *Test4::E::f()
struct E : A {
  virtual V3 *f();
};
V3 *E::f() { return 0;}

// Test that a pure virtual member doesn't get a thunk.

// CHECK-11:     Vtable for 'Test4::F' (5 entries).
// CHECK-11-NEXT:   0 | Test4::F RTTI
// CHECK-11-NEXT:   1 | offset_to_top (0)
// CHECK-11-NEXT:       -- (Test4::A, 0) vtable address --
// CHECK-11-NEXT:       -- (Test4::F, 0) vtable address --
// CHECK-11-NEXT:   2 | Test4::R3 *Test4::F::f() [pure]
// CHECK-11-NEXT:   3 | void Test4::F::g()
// CHECK-11-NEXT:   4 | Test4::R3 *Test4::F::f() [pure]
//
// CHECK-11:     VTable indices for 'Test4::F' (2 entries).
// CHECK-11-NEXT:   1 | void Test4::F::g()
// CHECK-11-NEXT:   2 | Test4::R3 *Test4::F::f()
struct F : A {
  virtual void g();
  virtual R3 *f() = 0;
};
void F::g() { }

}



namespace Test5 {

// Simple secondary vtables without 'this' pointer adjustments.
struct A {
  virtual void f();
  virtual void g();
  int a;
};

struct B1 : A {
  virtual void f();
  int b1;
};

struct B2 : A {
  virtual void g();
  int b2;
};

// CHECK-12:     Vtable for 'Test5::C' (9 entries).
// CHECK-12-NEXT:   0 | Test5::C RTTI
// CHECK-12-NEXT:   1 | offset_to_top (0)
// CHECK-12-NEXT:       -- (Test5::A, 0) vtable address --
// CHECK-12-NEXT:       -- (Test5::B1, 0) vtable address --
// CHECK-12-NEXT:       -- (Test5::C, 0) vtable address --
// CHECK-12-NEXT:   2 | void Test5::B1::f()
// CHECK-12-NEXT:   3 | void Test5::A::g()
// CHECK-12-NEXT:   4 | void Test5::C::h()
// CHECK-12-NEXT:   5 | Test5::C RTTI
// CHECK-12-NEXT:   6 | offset_to_top (-12)
// CHECK-12-NEXT:       -- (Test5::A, 12) vtable address --
// CHECK-12-NEXT:       -- (Test5::B2, 12) vtable address --
// CHECK-12-NEXT:   7 | void Test5::A::f()
// CHECK-12-NEXT:   8 | void Test5::B2::g()
//
// CHECK-12:     VTable indices for 'Test5::C' (1 entries).
// CHECK-12-NEXT:   2 | void Test5::C::h()
struct C : B1, B2 {
  virtual void h();
};
void C::h() { }  
}

namespace Test6 {

// Simple non-virtual 'this' pointer adjustments.
struct A1 {
  virtual void f();
  int a;
};

struct A2 {
  virtual void f();
  int a;
};

// CHECK-13:     Vtable for 'Test6::C' (6 entries).
// CHECK-13-NEXT:   0 | Test6::C RTTI
// CHECK-13-NEXT:   1 | offset_to_top (0)
// CHECK-13-NEXT:       -- (Test6::A1, 0) vtable address --
// CHECK-13-NEXT:       -- (Test6::C, 0) vtable address --
// CHECK-13-NEXT:   2 | void Test6::C::f()
// CHECK-13-NEXT:   3 | Test6::C RTTI
// CHECK-13-NEXT:   4 | offset_to_top (-8)
// CHECK-13-NEXT:       -- (Test6::A2, 8) vtable address --
// CHECK-13-NEXT:   5 | void Test6::C::f()
// CHECK-13-NEXT:       [this adjustment: -8 non-virtual]
//
// CHECK-13:     VTable indices for 'Test6::C' (1 entries).
// CHECK-13-NEXT:   0 | void Test6::C::f()
struct C : A1, A2 {
  virtual void f();
};
void C::f() { }

}

namespace Test7 {

// Test that the D::f overrider for A::f have different 'this' pointer
// adjustments in the two A base subobjects.

struct A {
  virtual void f();
  int a;
};

struct B1 : A { };
struct B2 : A { };

struct C { virtual void c(); };

// CHECK-14:     Vtable for 'Test7::D' (10 entries).
// CHECK-14-NEXT:   0 | Test7::D RTTI
// CHECK-14-NEXT:   1 | offset_to_top (0)
// CHECK-14-NEXT:       -- (Test7::C, 0) vtable address --
// CHECK-14-NEXT:       -- (Test7::D, 0) vtable address --
// CHECK-14-NEXT:   2 | void Test7::C::c()
// CHECK-14-NEXT:   3 | void Test7::D::f()
// CHECK-14-NEXT:   4 | Test7::D RTTI
// CHECK-14-NEXT:   5 | offset_to_top (-4)
// CHECK-14-NEXT:       -- (Test7::A, 4) vtable address --
// CHECK-14-NEXT:       -- (Test7::B1, 4) vtable address --
// CHECK-14-NEXT:   6 | void Test7::D::f()
// CHECK-14-NEXT:       [this adjustment: -4 non-virtual]
// CHECK-14-NEXT:   7 | Test7::D RTTI
// CHECK-14-NEXT:   8 | offset_to_top (-12)
// CHECK-14-NEXT:       -- (Test7::A, 12) vtable address --
// CHECK-14-NEXT:       -- (Test7::B2, 12) vtable address --
// CHECK-14-NEXT:   9 | void Test7::D::f()
// CHECK-14-NEXT:       [this adjustment: -12 non-virtual]
//
// CHECK-14:     VTable indices for 'Test7::D' (1 entries).
// CHECK-14-NEXT:   1 | void Test7::D::f()
struct D : C, B1, B2 {
  virtual void f();
};
void D::f() { }

}

namespace Test8 {

// Test that we don't try to layout vtables for classes that don't have
// virtual bases or virtual member functions.

struct A { };

// CHECK-15:     Vtable for 'Test8::B' (3 entries).
// CHECK-15-NEXT:   0 | Test8::B RTTI
// CHECK-15-NEXT:   1 | offset_to_top (0)
// CHECK-15-NEXT:       -- (Test8::B, 0) vtable address --
// CHECK-15-NEXT:   2 | void Test8::B::f()
//
// CHECK-15:     VTable indices for 'Test8::B' (1 entries).
// CHECK-15-NEXT:   0 | void Test8::B::f()
struct B : A {
  virtual void f();
};
void B::f() { }

}


namespace Test9 {

// Very simple test of vtables for virtual bases.
struct A1 { int a; };
struct A2 { int b; };

struct B : A1, virtual A2 {
  int b;
};

// CHECK-16:     Vtable for 'Test11::C' (8 entries).
// CHECK-16-NEXT:   0 | vbase_offset (24)
// CHECK-16-NEXT:   1 | vbase_offset (8)
// CHECK-16-NEXT:   2 | Test11::C RTTI
// CHECK-16-NEXT:   3 | offset_to_top (0)
// CHECK-16-NEXT:       -- (Test11::C, 0) vtable address --
// CHECK-16-NEXT:   4 | void Test11::C::f()
// CHECK-16-NEXT:   5 | vbase_offset (16)
// CHECK-16-NEXT:   6 | offset_to_top (-8)
// CHECK-16-NEXT:   7 | Test11::C RTTI
//
// CHECK-16:     VTable indices for 'Test11::C' (1 entries).
// CHECK-16-NEXT:   0 | void Test11::C::f()
struct C : virtual B {
  virtual void f();
};
void C::f() { }

}

