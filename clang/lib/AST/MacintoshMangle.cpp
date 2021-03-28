//===--- MacintoshMangle.cpp - Macintosh C++ Name Mangling -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===-----------------------------------------------------------------------------------------------===//
//
// Implements C++ name mangling according to the Itanium C++ ABI,
// which is used in GCC 3.2 and newer (and many compilers that are
// ABI-compatible with GCC):
//
//   http://mirror.informatimago.com/next/developer.apple.com/tools/mpw-tools/compilers/docs/abi_spec.pdf
//
//===-----------------------------------------------------------------------------------------------===//

#include "clang/AST/Mangle.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/DeclOpenMP.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprObjC.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/ABI.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"



using namespace clang;

namespace {

/// Retrieve the declaration context that should be used when mangling the given
/// declaration.
static const DeclContext *getEffectiveDeclContext(const Decl *D) {
  // The ABI assumes that lambda closure types that occur within
  // default arguments live in the context of the function. However, due to
  // the way in which Clang parses and creates function declarations, this is
  // not the case: the lambda closure type ends up living in the context
  // where the function itself resides, because the function declaration itself
  // had not yet been created. Fix the context here.
  if (const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(D)) {
    if (RD->isLambda())
      if (ParmVarDecl *ContextParam
            = dyn_cast_or_null<ParmVarDecl>(RD->getLambdaContextDecl()))
        return ContextParam->getDeclContext();
  }

  // Perform the same check for block literals.
  if (const BlockDecl *BD = dyn_cast<BlockDecl>(D)) {
    if (ParmVarDecl *ContextParam
          = dyn_cast_or_null<ParmVarDecl>(BD->getBlockManglingContextDecl()))
      return ContextParam->getDeclContext();
  }

  const DeclContext *DC = D->getDeclContext();
  if (isa<CapturedDecl>(DC) || isa<OMPDeclareReductionDecl>(DC) ||
      isa<OMPDeclareMapperDecl>(DC)) {
    return getEffectiveDeclContext(cast<Decl>(DC));
  }

  if (const auto *VD = dyn_cast<VarDecl>(D))
    if (VD->isExternC())
      return VD->getASTContext().getTranslationUnitDecl();

  if (const auto *FD = dyn_cast<FunctionDecl>(D))
    if (FD->isExternC())
      return FD->getASTContext().getTranslationUnitDecl();

  return DC->getRedeclContext();
}

static const DeclContext *getEffectiveParentContext(const DeclContext *DC) {
  return getEffectiveDeclContext(cast<Decl>(DC));
}

static bool isLocalContainerContext(const DeclContext *DC) {
  return isa<FunctionDecl>(DC) || isa<ObjCMethodDecl>(DC) || isa<BlockDecl>(DC);
}

static const RecordDecl *GetLocalClassDecl(const Decl *D) {
  const DeclContext *DC = getEffectiveDeclContext(D);
  while (!DC->isNamespace() && !DC->isTranslationUnit()) {
    if (isLocalContainerContext(DC))
      return dyn_cast<RecordDecl>(D);
    D = cast<Decl>(DC);
    DC = getEffectiveDeclContext(D);
  }
  return nullptr;
}

static const FunctionDecl *getStructor(const FunctionDecl *fn) {
  if (const FunctionTemplateDecl *ftd = fn->getPrimaryTemplate())
    return ftd->getTemplatedDecl();

  return fn;
}

static const NamedDecl *getStructor(const NamedDecl *decl) {
  const FunctionDecl *fn = dyn_cast_or_null<FunctionDecl>(decl);
  return (fn ? getStructor(fn) : decl);
}

static bool isLambda(const NamedDecl *ND) {
  const CXXRecordDecl *Record = dyn_cast<CXXRecordDecl>(ND);
  if (!Record)
    return false;

  return Record->isLambda();
}

static const unsigned UnknownArity = ~0U;

class MacintoshMangleContextImpl : public MacintoshMangleContext {
public:
  explicit MacintoshMangleContextImpl(ASTContext &Context,
                                      DiagnosticsEngine &Diags)
      : MacintoshMangleContext(Context, Diags) {}

  /// @name Mangler Entry Points
  /// @{

  bool shouldMangleCXXName(const NamedDecl *D);
  bool shouldMangleStringLiteral(const StringLiteral *) {
    return false;
  }
  void mangleCXXName(GlobalDecl GD, raw_ostream &) override;
  void mangleThunk(const CXXMethodDecl *MD, const ThunkInfo &Thunk,
                   raw_ostream &) override;
  void mangleCXXDtorThunk(const CXXDestructorDecl *DD, CXXDtorType Type,
                          const ThisAdjustment &ThisAdjustment,
                          raw_ostream &) override;
  void mangleReferenceTemporary(const VarDecl *D, unsigned ManglingNumber,
                                raw_ostream &) override;
  void mangleCXXRTTI(QualType T, raw_ostream &) override;
  void mangleCXXRTTIName(QualType T, raw_ostream &) override;
  void mangleTypeName(QualType T, raw_ostream &) override;
  void mangleCXXCtor(const CXXConstructorDecl *D, CXXCtorType Type,
                     raw_ostream &);
  void mangleCXXDtor(const CXXDestructorDecl *D, CXXDtorType Type,
                     raw_ostream &);

  void mangleStaticGuardVariable(const VarDecl *D, raw_ostream &);
  void mangleDynamicInitializer(const VarDecl *D, raw_ostream &Out);
  void mangleDynamicAtExitDestructor(const VarDecl *D,
                                     raw_ostream &Out);
  void mangleSEHFilterExpression(const NamedDecl *EnclosingDecl,
                                 raw_ostream &Out);
  void mangleSEHFinallyBlock(const NamedDecl *EnclosingDecl,
                             raw_ostream &Out);

  void mangleStringLiteral(const StringLiteral *, raw_ostream &);

  void mangleCXXVTable(const CXXRecordDecl *RD, raw_ostream &) override;
  void mangleCXXVTT(const CXXRecordDecl *RD, raw_ostream &) override;
  void mangleCXXCtorVTable(const CXXRecordDecl *RD, int64_t Offset,
                           const CXXRecordDecl *Type,
                           raw_ostream &) override;
  void mangleItaniumThreadLocalInit(const VarDecl *D,
                                    raw_ostream &) override;
  void mangleItaniumThreadLocalWrapper(const VarDecl *D,
                                       raw_ostream &) override;

  void mangleCXXCtorComdat(const CXXConstructorDecl *D,
                           raw_ostream &) override;
  void mangleCXXDtorComdat(const CXXDestructorDecl *D,
                           raw_ostream &) override;

  void mangleLambdaSig(const CXXRecordDecl *Lambda, raw_ostream &Out) override;
  void mangleDynamicStermFinalizer(const VarDecl *D, raw_ostream &Out) override;

  std::string getLambdaString(const CXXRecordDecl *Lambda) override {
    // This function matches the one in MicrosoftMangle, which returns
    // the string that is used in lambda mangled names.
    assert(Lambda->isLambda() && "RD must be a lambda!");
    std::string Name("<lambda");
    Decl *LambdaContextDecl = Lambda->getLambdaContextDecl();
    unsigned LambdaManglingNumber = Lambda->getLambdaManglingNumber();
    unsigned LambdaId;
    const ParmVarDecl *Parm = dyn_cast_or_null<ParmVarDecl>(LambdaContextDecl);
    const FunctionDecl *Func =
        Parm ? dyn_cast<FunctionDecl>(Parm->getDeclContext()) : nullptr;

    if (Func) {
      unsigned DefaultArgNo =
          Func->getNumParams() - Parm->getFunctionScopeIndex();
      Name += llvm::utostr(DefaultArgNo);
      Name += "_";
    }

    if (LambdaManglingNumber)
      LambdaId = LambdaManglingNumber;
    else
      LambdaId = getAnonymousStructIdForDebugInfo(Lambda);

    Name += llvm::utostr(LambdaId);
    Name += '>';
    return Name;
  }

  /// @}
};

}

bool MacintoshMangleContextImpl::shouldMangleCXXName(const NamedDecl *D) {
  const FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
  if (FD) {
    // Pragma patch function declarations as extern "C" aren't mangled
    if (FD->getDeclContext()->isExternCContext())
      return false;

    LanguageLinkage L = FD->getLanguageLinkage();
    // Overloadable functions need mangling.
    if (FD->hasAttr<OverloadableAttr>())
      return true;

    // "main" is not mangled.
    if (FD->isMain())
      return false;

    // C++ functions and those whose names are not a simple identifier need
    // mangling.
    if (!FD->getDeclName().isIdentifier() || L == CXXLanguageLinkage)
      return true;

    // C functions are not mangled.
    if (L == CLanguageLinkage)
      return false;
  }

  // Otherwise, no mangling is done outside C++ mode.
  if (!getASTContext().getLangOpts().CPlusPlus)
    return false;

  const VarDecl *VD = dyn_cast<VarDecl>(D);
  if (VD && !isa<DecompositionDecl>(D)) {
    // Pragma patch variable declarations as extern "C" aren't mangled
    if (VD->getDeclContext()->isExternCContext())
      return false;
    // C variables are not mangled.
    if (VD->isExternC())
      return false;

    // Variables at global scope with non-internal linkage are not mangled
    const DeclContext *DC = getEffectiveDeclContext(D);
    // Check for extern variable declared locally.
    if ((DC->isFunctionOrMethod() && D->hasLinkage()))
      while (!DC->isNamespace() && !DC->isTranslationUnit())
        DC = getEffectiveParentContext(DC);
    if (DC->isTranslationUnit() && D->getFormalLinkage() != InternalLinkage &&
        !isa<VarTemplateSpecializationDecl>(D))
      return false;
  }

  return true;
}

static bool PrintType(QualType T, const ASTContext &Ctx,
                      raw_ostream &Out);

static void MangleTemplateSpecializationArg(const TemplateArgument &Arg,
                                            bool &NeedsComma,
                                            const ASTContext &Ctx,
                                            raw_ostream &Out) {
  switch (Arg.getKind()) {
  case TemplateArgument::Type:
    if (NeedsComma)
      Out << ',';
    PrintType(Arg.getAsType(), Ctx, Out);
    NeedsComma = true;
    break;
  case TemplateArgument::Integral:
    if (NeedsComma)
      Out << ',';
    Arg.getAsIntegral().print(Out, true);
    NeedsComma = true;
    break;
  default: break;
  }
}

static void MangleTemplateSpecialization(const TemplateArgumentList &List,
                                         const ASTContext &Ctx,
                                         raw_ostream &Out) {
  Out << '<';
  bool NeedsComma = false;
  for (const TemplateArgument &Arg : List.asArray())
    MangleTemplateSpecializationArg(Arg, NeedsComma, Ctx, Out);
  Out << '>';
}

static void MangleTemplateSpecialization(
    const DependentFunctionTemplateSpecializationInfo &List,
    const ASTContext &Ctx, raw_ostream &Out) {
  Out << '<';
  bool NeedsComma = false;
  const TemplateArgumentLoc *Args = List.getTemplateArgs();
  for (unsigned i = 0; i < List.getNumTemplateArgs(); ++i) {
    const TemplateArgument &Arg = Args[i].getArgument();
    MangleTemplateSpecializationArg(Arg, NeedsComma, Ctx, Out);
  }
  Out << '>';
}

static void MangleClassTemplateSpecialization(const Decl *Decl,
                                              const ASTContext &Ctx,
                                              raw_ostream &Out) {
  if (const ClassTemplateSpecializationDecl *TemplateSpec =
      dyn_cast<ClassTemplateSpecializationDecl>(Decl)) {
    const TemplateArgumentList &List = TemplateSpec->getTemplateInstantiationArgs();
    MangleTemplateSpecialization(List, Ctx, Out);
  }
}

static void PrintNamedDecl(const NamedDecl *ND, const ASTContext &Ctx,
                           raw_ostream &Out) {
  std::string Str;
  llvm::raw_string_ostream Name(Str);
  Name << ND->getName();
  MangleClassTemplateSpecialization(ND, Ctx, Name);
  auto &NameStr = Name.str();
  Out << NameStr.length() << NameStr;
}

static void PrintNameSpace(const NamedDecl *ND, const ASTContext &Ctx,
                           raw_ostream &Out) {
  std::string Str;
  llvm::raw_string_ostream Name(Str);
  Name << ND->getName();
  MangleClassTemplateSpecialization(ND, Ctx, Name);
  auto &NameStr = Name.str();
  Out << NameStr.length() << NameStr;
}

static void RecursiveDenest(const DeclContext *DCtx,
                            unsigned Count,
                            const ASTContext &Ctx,
                            raw_ostream &Out) {
  const NamedDecl *Named = dyn_cast<NamedDecl>(DCtx);
  if (!Named)
    return;
  const DeclContext *Prefix = DCtx->getParent();
  if (Prefix && isa<NamedDecl>(Prefix))
    RecursiveDenest(Prefix, Count + 1, Ctx, Out);
  else if (Count > 1)
	if(!DCtx->isNamespace()){
		Out << 'Q' << Count;
	}
	if(DCtx->isNamespace()){
        Out << 'Q' << Count;
		PrintNameSpace(Named, Ctx, Out);
	}else{
		PrintNamedDecl(Named, Ctx, Out);
	}
}

static bool PrintType(QualType T, const ASTContext &Ctx,
                      raw_ostream &Out) {
  if (const ConstantArrayType *Array =
      dyn_cast_or_null<ConstantArrayType>(T.getTypePtr()->getAsArrayTypeUnsafe())) {
    Out << 'A';
    Array->getSize().print(Out, false);
    Out << '_';
    return PrintType(Array->getElementType(), Ctx, Out);
  }

  if (T.isConstQualified())
    Out << 'C';
  if (T.isVolatileQualified())
    Out << 'V';

  if (const ReferenceType *Ref = T.getTypePtr()->getAs<ReferenceType>()) {
    Out << 'R';
    return PrintType(Ref->getPointeeType(), Ctx, Out);

  } else if (const PointerType *Ptr = T.getTypePtr()->getAs<PointerType>()) {
    Out << 'P';
    return PrintType(Ptr->getPointeeType(), Ctx, Out);

  } else if (const TagType *Tag = T.getTypePtr()->getAs<TagType>()) {
    const TagDecl *TD = Tag->getDecl();
    RecursiveDenest(getEffectiveDeclContext(TD), 2, Ctx, Out);
    PrintNamedDecl(TD, Ctx, Out);
    return true;

  } else if (const MemberPointerType *MemberPtr = T.getTypePtr()->getAs<MemberPointerType>()) {
    Out << 'M';
    const RecordType *Rec = dyn_cast<RecordType>(MemberPtr->getClass());
    if (Rec)
      RecursiveDenest(Rec->getDecl(), 1, Ctx, Out);
    return PrintType(MemberPtr->getPointeeType(), Ctx, Out);

  } else if (const FunctionProtoType *Proto = T.getTypePtr()->getAs<FunctionProtoType>()) {
    Out << 'F';
    if (!Proto->getNumParams() && !Proto->isVariadic())
      Out << 'v';
    else
      for (QualType Type : Proto->param_types())
        PrintType(Type, Ctx, Out);

    if (Proto->isVariadic())
      Out << 'e';

    Out << '_';
    PrintType(Proto->getReturnType(), Ctx, Out);
    return true;

  } else if (const BuiltinType *Builtin = T.getTypePtr()->getAs<BuiltinType>()) {
    switch (Builtin->getKind()) {
    case BuiltinType::Void:
      Out << 'v';
      return true;
    case BuiltinType::Bool:
      Out << 'b';
      return true;
    case BuiltinType::UChar:
      Out << "Uc";
      return true;
    case BuiltinType::UShort:
      Out << "Us";
      return true;
    case BuiltinType::UInt:
      Out << "Ui";
      return true;
    case BuiltinType::ULong:
      Out << "Ul";
      return true;
    case BuiltinType::ULongLong:
      Out << "Uq";
      return true;
    case BuiltinType::Char_S:
    case BuiltinType::Char_U:
    case BuiltinType::SChar:
      Out << 'c';
      return true;
    case BuiltinType::WChar_S:
    case BuiltinType::WChar_U:
      Out << 'w';
      return true;
    case BuiltinType::Short:
      Out << 's';
      return true;
    case BuiltinType::Int:
      Out << 'i';
      return true;
    case BuiltinType::Long:
      Out << 'l';
      return true;
    case BuiltinType::LongLong:
      Out << 'x';
      return true;
    case BuiltinType::Float:
      Out << 'f';
      return true;
    case BuiltinType::Double:
      Out << 'd';
      return true;
    case BuiltinType::LongDouble:
      Out << 'r';
      return true;
    default: break;
    }
  }
  return false;
}

static void MangleNumber(int64_t Number, raw_ostream &Out) {
  //  <number> ::= [n] <non-negative decimal integer>
  if (Number < 0) {
    Out << 'n';
    Number = -Number;
  }

  Out << Number;
}

static void MangleCallOffset(int64_t NonVirtual, int64_t Virtual,
                             raw_ostream &Out) {
  //  <call-offset>  ::= h <nv-offset> _
  //                 ::= v <v-offset> _
  //  <nv-offset>    ::= <offset number>        # non-virtual base
  //  <v-offset>     ::= <offset number> _ <virtual offset number>
  //                      # virtual base, with vcall offset
  if (!Virtual) {
    Out << 'h';
    MangleNumber(NonVirtual, Out);
    Out << '_';
    return;
  }

  Out << 'v';
  MangleNumber(NonVirtual, Out);
  Out << '_';
  MangleNumber(Virtual, Out);
  Out << '_';
}

static void MangleOperatorName(OverloadedOperatorKind OO, raw_ostream &Out) {
  Out << "__";
  
  switch (OO) {
  // <operator-name> ::= nw     # new
  case OO_New: Out << "nw"; break;
  //              ::= nwa       # new[]
  case OO_Array_New: Out << "nwa"; break;
  //              ::= dl        # delete
  case OO_Delete: Out << "dl"; break;
  //              ::= dla       # delete[]
  case OO_Array_Delete: Out << "dla"; break;
  //              ::= pl        # +
  case OO_Plus:
    Out << "pl"; break;
  //              ::= mi        # -
  case OO_Minus:
    Out << "mi"; break;
  //              ::= ml        # *
  case OO_Star:
    Out << "ml"; break;
  //              ::= dv        # /
  case OO_Slash: Out << "dv"; break;
  //              ::= md        # %
  case OO_Percent: Out << "md"; break;
  //              ::= er        # ^
  case OO_Caret: Out << "er"; break;
  //              ::= adv       # /=
  case OO_SlashEqual: Out << "adv"; break;
  //              ::= ad        # &
  case OO_Amp:
    Out << "ad"; break;
  //              ::= or        # |
  case OO_Pipe: Out << "or"; break;
  //              ::= co        # ~
  case OO_Tilde: Out << "co"; break;
  //              ::= nt        # !
  case OO_Exclaim: Out << "nt"; break;
  //              ::= as        # =
  case OO_Equal: Out << "as"; break;
  //              ::= lt        # <
  case OO_Less: Out << "lt"; break;
  //              ::= gt        # >
  case OO_Greater: Out << "gt"; break;
  //              ::= apl       # +=
  case OO_PlusEqual: Out << "apl"; break;
  //              ::= ami       # -=
  case OO_MinusEqual: Out << "mI"; break;
  //              ::= aml       # *=
  case OO_StarEqual: Out << "aml"; break;
  //              ::= amd       # %=
  case OO_PercentEqual: Out << "amd"; break;
  //              ::= aer       # ^=
  case OO_CaretEqual: Out << "aer"; break;
  //              ::= aad       # &=
  case OO_AmpEqual: Out << "aad"; break;
  //              ::= aor       # |=
  case OO_PipeEqual: Out << "aor"; break;
  //              ::= ls        # <<
  case OO_LessLess: Out << "ls"; break;
  //              ::= rs        # >>
  case OO_GreaterGreater: Out << "rs"; break;
  //              ::= ars       # >>=
  case OO_GreaterGreaterEqual: Out << "ars"; break;
  //              ::= als       # <<=
  case OO_LessLessEqual: Out << "als"; break;

  //              ::= eq        # ==
  case OO_EqualEqual: Out << "eq"; break;
  //              ::= ne        # !=
  case OO_ExclaimEqual: Out << "ne"; break;
  
  //              ::= le        # <=
  case OO_LessEqual: Out << "le"; break;
  //              ::= ge        # >=
  case OO_GreaterEqual: Out << "ge"; break;

  //              ::= aa        # &&
  case OO_AmpAmp: Out << "aa"; break;
  //              ::= oo        # ||
  case OO_PipePipe: Out << "oo"; break;
  //              ::= pp        # ++
  case OO_PlusPlus: Out << "pp"; break;
  //              ::= mm        # --
  case OO_MinusMinus: Out << "mm"; break;

  //              ::= cl        # ()
  case OO_Call: Out << "cl"; break;
  //              ::= vc        # []
  case OO_Subscript: Out << "vc"; break;
  //              ::= rf        # ->
  case OO_Arrow: Out << "rf"; break;
  //              ::= cm        # ,
  case OO_Comma: Out << "cm"; break;
  //              ::= rm        # ->*
  case OO_ArrowStar: Out << "rm"; break;
  
  //              ::= qu        # ?
  // The conditional operator can't be overloaded, but we still handle it when
  // mangling expressions.
  case OO_Conditional: Out << "qu"; break;

  // The following cases aren't specified by the Macintosh ABI or PowerPC EABI specifications:

  // Proposal on cxx-abi-dev, 2015-10-21.
  //              ::= aw        # co_await
  case OO_Coawait: Out << "aw"; break;
  // Proposed in cxx-abi github issue 43.
  //              ::= ss        # <=>
  case OO_Spaceship: Out << "ss"; break;

  case OO_None:
  case NUM_OVERLOADED_OPERATORS:
    llvm_unreachable("Not an overloaded operator");
  }
}


static void MangleOperatorName(DeclarationName Name, raw_ostream &Out) {
  switch (Name.getNameKind()) {
  case DeclarationName::CXXConstructorName:
  case DeclarationName::CXXDestructorName:
  case DeclarationName::CXXDeductionGuideName:
  case DeclarationName::CXXUsingDirective:
  case DeclarationName::Identifier:
  case DeclarationName::ObjCMultiArgSelector:
  case DeclarationName::ObjCOneArgSelector:
  case DeclarationName::ObjCZeroArgSelector:
    llvm_unreachable("Not an operator name");

  case DeclarationName::CXXConversionFunctionName:
  case DeclarationName::CXXLiteralOperatorName:
    // TODO: Implement conversion and literal operator names
    return;

  case DeclarationName::CXXOperatorName:
    MangleOperatorName(Name.getCXXOverloadedOperator(), Out);
    break;
  }
}



/// Mangles the name of the declaration D and emits that name to the given
/// output stream.
///
/// If the declaration D requires a mangled name, this routine will emit that
/// mangled name to \p os and return true. Otherwise, \p os will be unchanged
/// and this routine will return false. In this case, the caller should just
/// emit the identifier of the declaration (\c D->getIdentifier()) as its
/// name.
void MacintoshMangleContextImpl::mangleCXXName(GlobalDecl GD,
                                               raw_ostream &Out) {
  const NamedDecl *D = cast<NamedDecl>(GD.getDecl());
  assert((isa<FunctionDecl>(D) || isa<VarDecl>(D)) &&
          "Invalid mangleName() call, argument is not a variable or function!");

  PrettyStackTraceDecl CrashInfo(D, SourceLocation(),
                                 getASTContext().getSourceManager(),
                                 "Mangling declaration");

  if (const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(D)) {
    MD = MD->getCanonicalDecl();
    if (isa<CXXConstructorDecl>(D))
      Out << "__ct";
    else if (isa<CXXDestructorDecl>(D))
      Out << "__dt";
    else if (MD->getNameInfo().getName().getNameKind() == DeclarationName::CXXOperatorName)
      MangleOperatorName(MD->getNameInfo().getName(), Out);
    else
      MD->getNameInfo().printName(Out, PrintingPolicy(LangOptions()));
    if (const TemplateArgumentList *TArgs = MD->getTemplateSpecializationArgs())
      MangleTemplateSpecialization(*TArgs, getASTContext(), Out);
    else if (DependentFunctionTemplateSpecializationInfo *DepArgs =
             MD->getDependentSpecializationInfo())
      MangleTemplateSpecialization(*DepArgs, getASTContext(), Out);
    Out << "__";
    RecursiveDenest(getEffectiveDeclContext(MD), 1,
                    getASTContext(), Out);
    if (MD->isConst())
      Out << 'C';
    Out << 'F';

    if (MD->param_empty() && !MD->isVariadic())
      Out << 'v';
    else
      for (const ParmVarDecl *Param : MD->parameters())
        PrintType(Param->getType(), getASTContext(), Out);

    if (MD->isVariadic())
      Out << 'e';

  } else if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
    FD = FD->getCanonicalDecl();
    if (FD->getNameInfo().getName().getNameKind() == DeclarationName::CXXOperatorName)
      MangleOperatorName(FD->getNameInfo().getName(), Out);
    else
      FD->getNameInfo().printName(Out, PrintingPolicy(LangOptions()));
    if (const TemplateArgumentList *TArgs = FD->getTemplateSpecializationArgs())
      MangleTemplateSpecialization(*TArgs, getASTContext(), Out);
    else if (DependentFunctionTemplateSpecializationInfo *DepArgs =
             FD->getDependentSpecializationInfo())
      MangleTemplateSpecialization(*DepArgs, getASTContext(), Out);
    Out << "__";
    RecursiveDenest(getEffectiveDeclContext(FD), 1,
                    getASTContext(), Out);
    Out << 'F';

    if (FD->param_empty() && !FD->isVariadic())
      Out << 'v';
    else
      for (const ParmVarDecl *Param : FD->parameters())
        PrintType(Param->getType(), getASTContext(), Out);

    if (FD->isVariadic())
      Out << 'e';

  } else if (const VarDecl *VD = dyn_cast<VarDecl>(D)) {
    VD = VD->getCanonicalDecl();
    Out << VD->getName();
    Out << "__";
    RecursiveDenest(getEffectiveDeclContext(VD), 1,
                    getASTContext(), Out);

  }
}

void MacintoshMangleContextImpl::mangleCXXCtor(const CXXConstructorDecl *D,
                                               CXXCtorType Type,
                                               raw_ostream &Out) {
  mangleCXXName(D, Out);
}

void MacintoshMangleContextImpl::mangleCXXDtor(const CXXDestructorDecl *D,
                                               CXXDtorType Type,
                                               raw_ostream &Out) {
  mangleCXXName(D, Out);
}

void MacintoshMangleContextImpl::mangleThunk(const CXXMethodDecl *MD,
                                             const ThunkInfo &Thunk,
                                             raw_ostream &Out) {
  //  <special-name> ::= T <call-offset> <base encoding>
  //                      # base is the nominal target function of thunk
  //  <special-name> ::= Tc <call-offset> <call-offset> <base encoding>
  //                      # base is the nominal target function of thunk
  //                      # first call-offset is 'this' adjustment
  //                      # second call-offset is result adjustment

  assert(!isa<CXXDestructorDecl>(MD) &&
         "Use mangleCXXDtor for destructor decls!");
  Out << "T";

  // Mangle the 'this' pointer adjustment.
  MangleCallOffset(Thunk.This.NonVirtual,
                   Thunk.This.Virtual.Itanium.VCallOffsetOffset,
                   Out);

  // Mangle the return pointer adjustment if there is one.
  if (!Thunk.Return.isEmpty())
    MangleCallOffset(Thunk.Return.NonVirtual,
                     Thunk.Return.Virtual.Itanium.VBaseOffsetOffset,
                     Out);

  mangleCXXName(MD, Out);
}

void MacintoshMangleContextImpl::mangleCXXDtorThunk(const CXXDestructorDecl *DD,
                                                    CXXDtorType Type,
                                                    const ThisAdjustment &ThisAdjustment,
                                                    raw_ostream &Out) {
  //  <special-name> ::= T <call-offset> <base encoding>
  //                      # base is the nominal target function of thunk
  Out << "T";

  // Mangle the 'this' pointer adjustment.
  MangleCallOffset(ThisAdjustment.NonVirtual,
                   ThisAdjustment.Virtual.Itanium.VCallOffsetOffset,
                   Out);

  mangleCXXName(DD, Out);
}

/// Returns the mangled name for a guard variable for the passed in VarDecl.
void MacintoshMangleContextImpl::mangleStaticGuardVariable(const VarDecl *D,
                                                           raw_ostream &Out) {
  llvm_unreachable("Can't mangle StaticGuardVariable");
}

void MacintoshMangleContextImpl::mangleDynamicInitializer(const VarDecl *MD,
                                                          raw_ostream &Out) {
  // These symbols are internal in the Itanium ABI, so the names don't matter.
  // Clang has traditionally used this symbol and allowed LLVM to adjust it to
  // avoid duplicate symbols.
  Out << "__cxx_global_var_init";
}

void MacintoshMangleContextImpl::mangleDynamicAtExitDestructor(const VarDecl *D,
                                                               raw_ostream &Out) {
  // Prefix the mangling of D with __dtor_.
  Out << "__dtor_";
  if (shouldMangleDeclName(D)) {
  } else
    Out << D->getName();
}

void MacintoshMangleContextImpl::mangleSEHFilterExpression(
    const NamedDecl *EnclosingDecl, raw_ostream &Out) {
  Out << "__filt_";
  if (shouldMangleDeclName(EnclosingDecl)) {
  } else
    Out << EnclosingDecl->getName();
}

void MacintoshMangleContextImpl::mangleSEHFinallyBlock(
    const NamedDecl *EnclosingDecl, raw_ostream &Out) {
  Out << "__fin_";
  if (shouldMangleDeclName(EnclosingDecl)) {
  } else
    Out << EnclosingDecl->getName();
}

void MacintoshMangleContextImpl::mangleReferenceTemporary(const VarDecl *D,
                                                          unsigned ManglingNumber,
                                                          raw_ostream &Out) {
  llvm_unreachable("Can't mangle ReferenceTemporary");
}

void MacintoshMangleContextImpl::mangleCXXRTTI(QualType Ty, raw_ostream &Out) {
  // <special-name> ::= TI <type>  # typeinfo structure
  assert(!Ty.hasQualifiers() && "RTTI info cannot have top-level qualifiers");
  Out << "__ti__";
  PrintType(Ty, getASTContext(), Out);
}

void MacintoshMangleContextImpl::mangleCXXRTTIName(QualType Ty,
                                                   raw_ostream &Out) {
  // <special-name> ::= TS <type>  # typeinfo name (null terminated byte string)
  Out << "__ts__";
  PrintType(Ty, getASTContext(), Out);
}

void MacintoshMangleContextImpl::mangleTypeName(QualType Ty, raw_ostream &Out) {
  mangleCXXRTTIName(Ty, Out);
}

void MacintoshMangleContextImpl::mangleStringLiteral(
    const StringLiteral *, raw_ostream &) {
  llvm_unreachable("Can't mangle string literals");
}

void MacintoshMangleContextImpl::mangleCXXVTable(
    const CXXRecordDecl *RD, raw_ostream &Out) {
  Out << "__vt__";
  RecursiveDenest(RD, 1, getASTContext(), Out);
}

void MacintoshMangleContextImpl::mangleCXXVTT(
    const CXXRecordDecl *RD, raw_ostream &Out) {
  Out << "__vb__";
  RecursiveDenest(RD, 1, getASTContext(), Out);
}

void MacintoshMangleContextImpl::mangleCXXCtorVTable(
    const CXXRecordDecl *RD, int64_t Offset,
    const CXXRecordDecl *Type, raw_ostream &) {
  llvm_unreachable("Can't mangle CtorVTable");
}

void MacintoshMangleContextImpl::mangleItaniumThreadLocalInit(
    const VarDecl *D, raw_ostream &) {
  llvm_unreachable("Can't mangle ItaniumThreadLocalInit");
}

void MacintoshMangleContextImpl::mangleItaniumThreadLocalWrapper(
    const VarDecl *D, raw_ostream &) {
  llvm_unreachable("Can't mangle ItaniumThreadLocalWrapper");
}

void MacintoshMangleContextImpl::mangleCXXCtorComdat(
    const CXXConstructorDecl *D, raw_ostream &) {
  llvm_unreachable("Can't mangle CtorComdat");
}

void MacintoshMangleContextImpl::mangleCXXDtorComdat(
    const CXXDestructorDecl *D, raw_ostream &) {
  llvm_unreachable("Can't mangle DtorComdat");
}

void MacintoshMangleContextImpl::mangleLambdaSig(const CXXRecordDecl *Lambda,
                                                 raw_ostream &Out) {
  llvm_unreachable("Can't mangle LambdaSig");
}

void MacintoshMangleContextImpl::mangleDynamicStermFinalizer(const VarDecl *D,
                                                             raw_ostream &Out) {
  llvm_unreachable("Can't mangle DynamicStermFinalizer");
}

MacintoshMangleContext *
MacintoshMangleContext::create(ASTContext &Context, DiagnosticsEngine &Diags) {
  return new MacintoshMangleContextImpl(Context, Diags);
}
