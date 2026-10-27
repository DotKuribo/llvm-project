// RUN: %clang_cc1 %s -triple powerpc-ibm-kuribo-eabi -std=gnu++17 -fno-rtti \
// RUN:   -fno-use-cxa-atexit -fno-use-init-array -emit-llvm -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,DTORS
// RUN: %clang_cc1 %s -triple powerpc-ibm-kuribo-eabi -std=gnu++17 -fno-rtti \
// RUN:   -fno-use-cxa-atexit -fno-use-init-array -fc++-static-destructors=none \
// RUN:   -emit-llvm -o - | FileCheck %s --check-prefixes=CHECK,NODTORS

// Global objects and pointers to member functions as used by Better Sunshine
// Engine projects. Patches are registered from global constructors, and game
// classes hand out CodeWarrior pointers to member functions (12 bytes:
// this-delta, vtable index or -1, function or vtable offset). The second RUN
// line matches the -fno-c++-static-destructors the projects build with.

struct Patch {
  Patch(unsigned address, void (*target)());
  ~Patch();
  unsigned mAddress;
};
void hook();
Patch gPatch(0x80005624, hook);

struct TObj {
  void update();
  virtual void perform();
  int mState;
};
typedef void (TObj::*Callback)();

Callback gUpdate = &TObj::update;
Callback gPerform = &TObj::perform;
static_assert(sizeof(Callback) == 12, "CodeWarrior PTMFs are 12 bytes");

void invoke(TObj *obj, Callback cb) { (obj->*cb)(); }
bool isSet(Callback cb) { return cb != nullptr; }

// Non-virtual members store -1 as the vtable index; virtual ones store the
// byte offset of their slot past the RTTI and offset-to-top entries.
// CHECK: @gUpdate = global { i32, i32, i32 } { i32 0, i32 -1, i32 ptrtoint (ptr @update__4TObjFv to i32) }
// CHECK: @gPerform = global { i32, i32, i32 } { i32 0, i32 0, i32 8 }
// CHECK: @llvm.global_ctors = appending global

// CHECK-LABEL: define internal void @__cxx_global_var_init()
// CHECK: call {{.*}} @__ct__5PatchFUiPFv_v(ptr {{.*}}@gPatch, i32 {{.*}}-2147461596, ptr {{.*}}@hook__Fv)
// DTORS: call void @__register_global_object(ptr @gPatch, ptr @__dt__5PatchFv, ptr @{{[0-9]+}})
// NODTORS-NOT: __register_global_object

// CHECK-LABEL: define {{.*}} @invoke__FP4TObjM4TObjFv_v(ptr {{.*}}, ptr {{.*}}byval({ i32, i32, i32 })
// CHECK: %memptr.adj = extractvalue { i32, i32, i32 } %{{.*}}, 0
// CHECK: %memptr.vtoffs = extractvalue { i32, i32, i32 } %{{.*}}, 1
// CHECK: %memptr.ptr = extractvalue { i32, i32, i32 } %{{.*}}, 2
// CHECK: icmp sgt i32 %memptr.vtoffs, -1
// CHECK: memptr.virtual:
// CHECK: %vtable = load ptr
// CHECK: getelementptr i8, ptr %vtable, i32 %memptr.ptr
// CHECK: memptr.nonvirtual:
// CHECK: inttoptr i32 %memptr.ptr to ptr

// A PTMF is null only when all three fields are zero.
// CHECK-LABEL: define {{.*}} @isSet__FM4TObjFv_v(
// CHECK: icmp ne i32 %lhs.memptr.adj, 0
// CHECK: icmp ne i32 %lhs.memptr.vtoffs, 0
// CHECK: icmp ne i32 %lhs.memptr.ptr, 0
