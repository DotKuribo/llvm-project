// RUN: %clang_cc1 %s -triple powerpc-ibm-kuribo-eabi -std=gnu++17 -fno-rtti \
// RUN:   -Wno-invalid-offsetof -fsyntax-only -verify
// RUN: %clang_cc1 %s -triple powerpc-ibm-kuribo-eabi -std=gnu++17 -fno-rtti \
// RUN:   -Wno-invalid-offsetof -emit-llvm-only -fdump-record-layouts \
// RUN:   | FileCheck %s
// expected-no-diagnostics

// Class layouts from Super Mario Sunshine's actor hierarchy, as declared in
// SunshineHeaderInterface. The offsets asserted below are the ones recorded in
// those headers; objects created by Better Sunshine Engine projects are handed
// to game code that reads these fields at fixed offsets.

typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef float f32;

struct Vec { f32 x, y, z; };
class JSUMemoryInputStream;

namespace JGeometry {
template <typename T> struct TVec3 { T x, y, z; };
}
typedef JGeometry::TVec3<f32> TVec3f;

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
template <typename T> class TFlagT {
public:
  TFlagT(T val) : mVal(val) {}
  T mVal;
};

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
  u16 mPerformFlags; // 0x000C
};

class TPlacement : public TViewObj {
public:
  TPlacement(const char *);
  virtual ~TPlacement();
  void load(JSUMemoryInputStream &) override;
  TVec3f mTranslation;
  TFlagT<u16> mPlacementFlags;
};

class TActor : public TPlacement, public JStage::TActor {
public:
  TActor(const char *);
  virtual ~TActor();
  u32 getType() const override;
  void perform(u32, TGraphics *) override;
  void JSGGetTranslation(Vec *) const override;
  void JSGSetTranslation(const Vec &) override;
  TVec3f mScale;     // 24
  TVec3f mRotation;  // 30
  u32 _3C;           // 3C
  TNameRef *mRootRef;
};
} // namespace JDrama

class THitActor : public JDrama::TActor {
public:
  THitActor(const char *name);
  virtual ~THitActor();
  void perform(u32, JDrama::TGraphics *) override;
  virtual bool receiveMessage(THitActor *sender, u32 msg);
  THitActor **mCollidingObjs; // 0x0044
  s16 mNumObjs;               // 0x0048
  s16 mMaxObjs;               // 0x004A
  u32 mObjectID;              // 0x004C
  f32 mAttackRadius;          // 0x0050
  f32 mAttackHeight;          // 0x0054
  f32 mReceiveRadius;         // 0x0058
  f32 mReceiveHeight;         // 0x005C
  f32 mEntryRadius;           // 0x0060
  u32 mObjectType;            // 0x0064
};

// A project-side actor in the style of BetterSunshineEngine's custom objects.
class TCustomActor : public THitActor {
public:
  TCustomActor(const char *name);
  ~TCustomActor() override;
  bool receiveMessage(THitActor *sender, u32 msg) override;
  u8 mState;
  f32 mTimer;
};

#define AT(T, F, OFF) static_assert(__builtin_offsetof(T, F) == OFF, #T "::" #F)
AT(JDrama::TNameRef, mKeyName, 0x04);
AT(JDrama::TNameRef, mKeyCode, 0x08);
AT(JDrama::TViewObj, mPerformFlags, 0x0C);
AT(JDrama::TPlacement, mTranslation, 0x10);
AT(JDrama::TPlacement, mPlacementFlags, 0x1C);
AT(JDrama::TActor, mScale, 0x24);
AT(JDrama::TActor, mRotation, 0x30);
AT(JDrama::TActor, _3C, 0x3C);
AT(JDrama::TActor, mRootRef, 0x40);
AT(THitActor, mCollidingObjs, 0x44);
AT(THitActor, mNumObjs, 0x48);
AT(THitActor, mMaxObjs, 0x4A);
AT(THitActor, mObjectID, 0x4C);
AT(THitActor, mEntryRadius, 0x60);
AT(THitActor, mObjectType, 0x64);
AT(TCustomActor, mState, 0x68);
AT(TCustomActor, mTimer, 0x6C);
static_assert(sizeof(JDrama::TNameRef) == 0x0C, "");
static_assert(sizeof(JDrama::TPlacement) == 0x20, "");
static_assert(sizeof(JDrama::TActor) == 0x44, "");
static_assert(sizeof(THitActor) == 0x68, "");
static_assert(sizeof(TCustomActor) == 0x70, "");

void force(JDrama::TActor *, THitActor *, TCustomActor *);

// The secondary JStage::TActor base sits directly after TPlacement and keeps
// its own vtable pointer at 0x20.
// CHECK-LABEL:   0 | class JDrama::TActor
// CHECK-NEXT:    0 |   class JDrama::TPlacement (primary base)
// CHECK-NEXT:    0 |     class JDrama::TViewObj (primary base)
// CHECK-NEXT:    0 |       class JDrama::TNameRef (primary base)
// CHECK-NEXT:    0 |         (TNameRef vtable pointer)
// CHECK-NEXT:    4 |         const char * mKeyName
// CHECK-NEXT:    8 |         u16 mKeyCode
// CHECK-NEXT:   10 |         u16 _0A
// CHECK-NEXT:   12 |       u16 mPerformFlags
// CHECK-NEXT:   16 |     struct JGeometry::TVec3<float> mTranslation
// CHECK:        28 |     class JDrama::TFlagT<unsigned short> mPlacementFlags
// CHECK:        32 |   class JStage::TActor (base)
// CHECK-NEXT:   32 |     class JStage::TObject (primary base)
// CHECK-NEXT:   32 |       (TObject vtable pointer)
// CHECK-NEXT:   36 |   struct JGeometry::TVec3<float> mScale
// CHECK:        48 |   struct JGeometry::TVec3<float> mRotation
// CHECK:        60 |   u32 _3C
// CHECK-NEXT:   64 |   TNameRef * mRootRef
// CHECK-NEXT:      | [sizeof=68, dsize=68, align=4,
// CHECK-NEXT:      |  nvsize=68, nvalign=4]
