// RUN: %clang_cc1 %s -triple powerpc-ibm-kuribo-eabi -std=gnu++17 -fno-rtti \
// RUN:   -fdeclspec -fno-use-cxa-atexit -fno-use-init-array -emit-llvm -o - \
// RUN:   | FileCheck %s

// Every name checked here is a real symbol from Super Mario Sunshine (NTSC-U)
// as listed in BetterSunshineEngine's maps/us.map. Projects built on the
// Better Sunshine Engine call into the game through these names, so the
// CodeWarrior mangling must reproduce them exactly. The declarations are
// trimmed copies of those in SunshineHeaderInterface.

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long size_t;

struct Vec { float x, y, z; };
class JSUMemoryInputStream;
class JSUMemoryOutputStream;
class JKRHeap;

namespace JDrama {
class TGraphics;

class TNameRef {
public:
  TNameRef(const char *);
  virtual ~TNameRef();
  virtual u32 getType() const;
  virtual void load(JSUMemoryInputStream &stream);
  virtual void save(JSUMemoryOutputStream &stream);
  virtual void loadAfter();
  virtual TNameRef *searchF(u16, const char *);
  static TNameRef *genObject(JSUMemoryInputStream &, JSUMemoryInputStream &);
  static u32 getType(JSUMemoryInputStream &, JSUMemoryInputStream &);
  TNameRef *search(const char *);
  static u16 calcKeyCode(const char *);

  const char *mKeyName;
  u16 mKeyCode;
  u16 _0A;
};

class TViewObj : public TNameRef {
public:
  TViewObj(const char *);
  virtual ~TViewObj();
  virtual void perform(u32, TGraphics *) = 0;
  void testPerform(u32, TGraphics *);
  u16 mPerformFlags;
};
} // namespace JDrama

namespace JGeometry {
template <typename T> struct TUtil {
  static T inv_sqrt(T);
};

template <typename T> struct TVec3 {
  T x, y, z;
  TVec3 &operator=(const TVec3 &);
  void scale(T);
  T dot(const TVec3 &) const;
};

template <typename T> struct SMatrix34C { T mMtx[3][4]; };
template <typename T> struct TMatrix34 : public T {
  TMatrix34();
};
} // namespace JGeometry

template <typename T> T MsWrap(T, T, T);

class TBoid;
class TGraphWeb;
class TBoidLeader {
public:
  TBoidLeader(int, const char *);
  void perform(u32, JDrama::TGraphics *);
  void calcForces(const TBoid *) const;
  void calcGoalForce(const JGeometry::TVec3<float> &) const;
};

class TAnimalBase { public: void execWalk(bool); };
class MSound { public: u8 getMarioVoiceID(u8); };
class TTalk2D2 { public: void openWindow(s8, float); };
class TApplication;
class JUTGamePad { public: static void *mPadList; };

void CLBCalcScaleTranslateMatrix(float (*)[4], const Vec &, const Vec &);
extern TApplication gpApplication;

void *operator new(size_t);
void *operator new(size_t, int);
void *operator new(size_t, JKRHeap *, int);
void *operator new[](size_t);
void operator delete(void *);
void operator delete[](void *);
inline void *operator new(size_t, void *p) { return p; }

JSUMemoryInputStream &inStream();
JSUMemoryOutputStream &outStream();
TApplication *useApp() { return &gpApplication; }
void *usePads() { return JUTGamePad::mPadList; }

void use(JDrama::TNameRef *ref, JDrama::TViewObj *view,
         JDrama::TGraphics *gfx, TBoidLeader *leader, TBoid *boid,
         JGeometry::TVec3<float> *v, TAnimalBase *animal, MSound *sound,
         TTalk2D2 *talk, JKRHeap *heap, float (*mtx)[4], Vec &a) {
  new (ref) JDrama::TNameRef("ref");
  ref->search("child");
  JDrama::TNameRef::calcKeyCode("key");
  JDrama::TNameRef::genObject(inStream(), inStream());
  JDrama::TNameRef::getType(inStream(), inStream());
  view->testPerform(0, gfx);

  JGeometry::TUtil<float>::inv_sqrt(2.0f);
  v->scale(2.0f);
  v->dot(*v);
  *v = *v;
  JGeometry::TMatrix34<JGeometry::SMatrix34C<float>> m;
  MsWrap<float>(1.0f, 0.0f, 2.0f);

  new (leader) TBoidLeader(1, "leader");
  leader->perform(0, gfx);
  leader->calcForces(boid);
  leader->calcGoalForce(*v);

  animal->execWalk(true);
  sound->getMarioVoiceID(1);
  talk->openWindow(-1, 1.0f);
  CLBCalcScaleTranslateMatrix(mtx, a, a);

  // Reference the allocation functions directly; a delete-expression on a
  // trivially destructible type does not call operator delete here.
  operator delete(operator new(4));
  operator delete(operator new(4, 0));
  operator delete(operator new(4, heap, 4));
  operator delete[](operator new[](16));
}

// Data symbols are not mangled at namespace scope; static members are.
// CHECK-DAG: @gpApplication = external global
// CHECK-DAG: @mPadList__10JUTGamePad = external global

// CHECK-DAG: declare {{.*}} @__ct__Q26JDrama8TNameRefFPCc(
// CHECK-DAG: declare {{.*}} @search__Q26JDrama8TNameRefFPCc(
// CHECK-DAG: declare {{.*}} @calcKeyCode__Q26JDrama8TNameRefFPCc(
// CHECK-DAG: declare {{.*}} @genObject__Q26JDrama8TNameRefFR20JSUMemoryInputStreamR20JSUMemoryInputStream(
// CHECK-DAG: declare {{.*}} @getType__Q26JDrama8TNameRefFR20JSUMemoryInputStreamR20JSUMemoryInputStream(
// CHECK-DAG: declare {{.*}} @testPerform__Q26JDrama8TViewObjFUlPQ26JDrama9TGraphics(

// Template arguments are spelled out in angle brackets.
// CHECK-DAG: declare {{.*}} @"inv_sqrt__Q29JGeometry8TUtil<f>Ff"(
// CHECK-DAG: declare {{.*}} @"scale__Q29JGeometry8TVec3<f>Ff"(
// CHECK-DAG: declare {{.*}} @"dot__Q29JGeometry8TVec3<f>CFRCQ29JGeometry8TVec3<f>"(
// CHECK-DAG: declare {{.*}} @"__as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>"(
// CHECK-DAG: declare {{.*}} @"__ct__Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>Fv"(
// CHECK-DAG: declare {{.*}} @"MsWrap<f>__Ffff"(

// CHECK-DAG: declare {{.*}} @__ct__11TBoidLeaderFiPCc(
// CHECK-DAG: declare {{.*}} @perform__11TBoidLeaderFUlPQ26JDrama9TGraphics(
// CHECK-DAG: declare {{.*}} @calcForces__11TBoidLeaderCFPC5TBoid(
// CHECK-DAG: declare {{.*}} @"calcGoalForce__11TBoidLeaderCFRCQ29JGeometry8TVec3<f>"(

// CHECK-DAG: declare {{.*}} @execWalk__11TAnimalBaseFb(
// CHECK-DAG: declare {{.*}} @getMarioVoiceID__6MSoundFUc(
// CHECK-DAG: declare {{.*}} @openWindow__8TTalk2D2FScf(
// CHECK-DAG: declare {{.*}} @CLBCalcScaleTranslateMatrix__FPA4_fRC3VecRC3Vec(

// CHECK-DAG: declare {{.*}} @__nw__FUl(
// CHECK-DAG: declare {{.*}} @__nw__FUli(
// CHECK-DAG: declare {{.*}} @__nw__FUlP7JKRHeapi(
// CHECK-DAG: declare {{.*}} @__nwa__FUl(
// CHECK-DAG: declare {{.*}} @__dl__FPv(
// CHECK-DAG: declare {{.*}} @__dla__FPv(
