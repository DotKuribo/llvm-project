// RUN: %clang_cc1 %s -triple powerpc-ibm-kuribo-eabi -std=gnu++17 -fno-rtti \
// RUN:   -fdeclspec -fno-use-cxa-atexit -fno-use-init-array \
// RUN:   -fno-sized-deallocation -emit-llvm -o - | FileCheck %s

// A custom actor in the style of the objects Better Sunshine Engine projects
// add to Super Mario Sunshine: it derives from the game's THitActor (which
// multiply inherits JStage::TActor), overrides game virtuals, and is created
// and destroyed by game code. Its vtables, thunks, constructor and destructor
// must follow the CodeWarrior ABI the game was built with. The expectations
// match the clang 13 based DotKuribo compiler these projects ship with; the
// game itself provides thunks such as @32@JSGSetTranslation__Q26JDrama6TActorFRC3Vec
// for the JStage::TActor subobject at offset 32.

typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef unsigned long size_t;
typedef float f32;

void *operator new(size_t);
void operator delete(void *);

struct Vec { f32 x, y, z; };
class JSUMemoryInputStream;

namespace JStage {
class TObject {
public:
  virtual ~TObject();
  virtual u32 JSGGetName() const;
};
class TActor : public TObject {
public:
  virtual ~TActor();
  virtual void JSGGetTranslation(Vec *) const;
  virtual void JSGSetTranslation(const Vec &);
};
} // namespace JStage

namespace JDrama {
class TGraphics;
class TNameRef {
public:
  TNameRef(const char *);
  virtual ~TNameRef();
  virtual u32 getType() const;
  virtual void load(JSUMemoryInputStream &);
  const char *mKeyName;
  u16 mKeyCode;
  u16 _0A;
};
class TViewObj : public TNameRef {
public:
  TViewObj(const char *);
  virtual ~TViewObj();
  virtual void perform(u32, TGraphics *) = 0;
  u16 mPerformFlags;
};
class TPlacement : public TViewObj {
public:
  TPlacement(const char *);
  virtual ~TPlacement();
  void load(JSUMemoryInputStream &) override;
  f32 mTranslation[3];
  u16 mPlacementFlags;
};
class TActor : public TPlacement, public JStage::TActor {
public:
  TActor(const char *);
  virtual ~TActor();
  u32 getType() const override;
  void perform(u32, TGraphics *) override;
  void JSGGetTranslation(Vec *) const override;
  void JSGSetTranslation(const Vec &) override;
  f32 mScale[3];
  f32 mRotation[3];
  u32 _3C;
  TNameRef *mRootRef;
};
} // namespace JDrama

class THitActor : public JDrama::TActor {
public:
  THitActor(const char *name);
  virtual ~THitActor();
  void perform(u32, JDrama::TGraphics *) override;
  virtual bool receiveMessage(THitActor *sender, u32 msg);
  THitActor **mCollidingObjs;
  s16 mNumObjs;
  s16 mMaxObjs;
  u32 mObjectID;
  f32 mAttackRadius, mAttackHeight, mReceiveRadius, mReceiveHeight;
  f32 mEntryRadius;
  u32 mObjectType;
};

class TCustomActor : public THitActor {
public:
  TCustomActor(const char *name);
  ~TCustomActor() override;
  u32 getType() const override;
  void perform(u32, JDrama::TGraphics *) override;
  bool receiveMessage(THitActor *sender, u32 msg) override;
  void JSGGetTranslation(Vec *) const override;
  int mState;
};

TCustomActor::TCustomActor(const char *name) : THitActor(name), mState(0) {}
TCustomActor::~TCustomActor() {}
u32 TCustomActor::getType() const { return 0x40000F00; }
void TCustomActor::perform(u32 flags, JDrama::TGraphics *gfx) {
  THitActor::perform(flags, gfx);
}
bool TCustomActor::receiveMessage(THitActor *sender, u32 msg) {
  return msg == 1;
}
void TCustomActor::JSGGetTranslation(Vec *out) const {}

THitActor *spawn() { return new TCustomActor("custom"); }
void despawn(THitActor *actor) { delete actor; }
void destroyInPlace(TCustomActor *actor) { actor->~TCustomActor(); }
void onStack() { TCustomActor local("local"); }
bool message(THitActor *actor) { return actor->receiveMessage(actor, 7); }

// CodeWarrior vtables start with the RTTI and offset-to-top slots and hold a
// single (deleting) destructor entry. The JStage::TActor half points at
// this-adjusting thunks named @<offset>@<target>.
// CHECK: @__vt__12TCustomActor = unnamed_addr constant { [6 x ptr], [9 x ptr] } {
// CHECK-SAME: [6 x ptr] [ptr null, ptr null, ptr @__dt__12TCustomActorFv,
// CHECK-SAME: ptr @getType__12TCustomActorCFv,
// CHECK-SAME: ptr @load__Q26JDrama10TPlacementFR20JSUMemoryInputStream,
// CHECK-SAME: ptr @perform__12TCustomActorFUlPQ26JDrama9TGraphics],
// CHECK-SAME: [9 x ptr] [ptr null, ptr inttoptr (i32 -32 to ptr),
// CHECK-SAME: ptr @"@32@__dt__12TCustomActorFv",
// CHECK-SAME: ptr @JSGGetName__Q26JStage7TObjectCFv,
// CHECK-SAME: ptr @"@32@JSGGetTranslation__12TCustomActorCFP3Vec",
// CHECK-SAME: ptr @"@32@JSGSetTranslation__Q26JDrama6TActorFRC3Vec",
// CHECK-SAME: ptr @JSGGetTranslation__12TCustomActorCFP3Vec,
// CHECK-SAME: ptr @JSGSetTranslation__Q26JDrama6TActorFRC3Vec,
// CHECK-SAME: ptr @receiveMessage__12TCustomActorFP9THitActorUl] }

// The constructor runs the base constructor, then installs both vtable
// pointers, the secondary one at offset 32.
// CHECK-LABEL: define {{.*}} @__ct__12TCustomActorFPCc(ptr
// CHECK: call {{.*}} @__ct__9THitActorFPCc(
// CHECK: store ptr getelementptr inbounds {{.*}}@__vt__12TCustomActor, i32 0, i32 0, i32 0)
// CHECK: getelementptr inbounds i8, ptr %{{.*}}, i32 32
// CHECK: store ptr getelementptr inbounds {{.*}}@__vt__12TCustomActor, i32 0, i32 1, i32 0)

// Destructors take an implicit int: base destructors are called with 0, and
// the storage is freed only when the flag is positive.
// CHECK-LABEL: define {{.*}} @__dt__12TCustomActorFv(ptr {{.*}}, i32 {{.*}}%int)
// CHECK: call {{.*}} @__dt__9THitActorFv(ptr {{.*}}, i32 {{.*}}0)
// CHECK: icmp sle i32 %vtt, 0
// CHECK: call void @__dl__FPv(ptr

// CHECK-LABEL: define {{.*}} @"@32@__dt__12TCustomActorFv"(ptr {{.*}}, i32 {{.*}}%int)
// CHECK: getelementptr inbounds i8, ptr %{{.*}}, i32 -32
// CHECK: tail call {{.*}} @__dt__12TCustomActorFv(ptr {{.*}}, i32 {{.*}}%vtt)

// CHECK-LABEL: define {{.*}} @"@32@JSGGetTranslation__12TCustomActorCFP3Vec"(
// CHECK: getelementptr inbounds i8, ptr %{{.*}}, i32 -32
// CHECK: tail call void @JSGGetTranslation__12TCustomActorCFP3Vec(

// CHECK-LABEL: define {{.*}} @spawn__Fv()
// CHECK: call {{.*}} @__nw__FUl(i32 {{.*}}108)
// CHECK: call {{.*}} @__ct__12TCustomActorFPCc(

// A delete-expression calls the virtual destructor (slot 2) with 1.
// CHECK-LABEL: define {{.*}} @despawn__FP9THitActor(
// CHECK: getelementptr inbounds ptr, ptr %vtable, i64 2
// CHECK: call {{.*}}(ptr {{.*}}, i32 {{.*}}1)

// Explicit destructor calls and scope exit pass -1.
// CHECK-LABEL: define {{.*}} @destroyInPlace__FP12TCustomActor(
// CHECK: getelementptr inbounds ptr, ptr %vtable, i64 2
// CHECK: call {{.*}}(ptr {{.*}}, i32 {{.*}}-1)

// CHECK-LABEL: define {{.*}} @onStack__Fv()
// CHECK: call {{.*}} @__ct__12TCustomActorFPCc(
// CHECK: call {{.*}} @__dt__12TCustomActorFv(ptr {{.*}}, i32 {{.*}}-1)

// THitActor::receiveMessage is the 15th slot of the primary vtable.
// CHECK-LABEL: define {{.*}} @message__FP9THitActor(
// CHECK: getelementptr inbounds ptr, ptr %vtable, i64 14
