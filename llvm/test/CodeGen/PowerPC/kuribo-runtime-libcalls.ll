; RUN: llc -mtriple=powerpc-ibm-kuribo-eabi < %s | FileCheck %s
; RUN: llc -mtriple=powerpc-unknown-linux-gnu < %s | FileCheck %s --check-prefix=LINUX

; Kuribo targets link against the CodeWarrior runtime of the host game, so
; 64-bit division and integer/floating-point conversions must call its helpers
; (Super Mario Sunshine provides __div2i and __div2u, for example).

define i64 @sdiv64(i64 %a, i64 %b) {
; CHECK-LABEL: sdiv64:
; CHECK: bl __div2i
; LINUX-LABEL: sdiv64:
; LINUX: bl __divdi3
  %r = sdiv i64 %a, %b
  ret i64 %r
}

define i64 @udiv64(i64 %a, i64 %b) {
; CHECK-LABEL: udiv64:
; CHECK: bl __div2u
; LINUX-LABEL: udiv64:
; LINUX: bl __udivdi3
  %r = udiv i64 %a, %b
  ret i64 %r
}

define i64 @fptosi_f32(float %a) {
; CHECK-LABEL: fptosi_f32:
; CHECK: bl __cvt_dbl_usll
  %r = fptosi float %a to i64
  ret i64 %r
}

define i64 @fptosi_f64(double %a) {
; CHECK-LABEL: fptosi_f64:
; CHECK: bl __cvt_dbl_usll
  %r = fptosi double %a to i64
  ret i64 %r
}

define i64 @fptoui_f32(float %a) {
; CHECK-LABEL: fptoui_f32:
; CHECK: bl __cvt_dbl_usll
  %r = fptoui float %a to i64
  ret i64 %r
}

define i64 @fptoui_f64(double %a) {
; CHECK-LABEL: fptoui_f64:
; CHECK: bl __cvt_dbl_usll
  %r = fptoui double %a to i64
  ret i64 %r
}

define float @sitofp_f32(i64 %a) {
; CHECK-LABEL: sitofp_f32:
; CHECK: bl __cvt_sll_flt
  %r = sitofp i64 %a to float
  ret float %r
}

define double @sitofp_f64(i64 %a) {
; CHECK-LABEL: sitofp_f64:
; CHECK: bl __cvt_sll_dbl
  %r = sitofp i64 %a to double
  ret double %r
}

define float @uitofp_f32(i64 %a) {
; CHECK-LABEL: uitofp_f32:
; CHECK: bl __cvt_ull_flt
  %r = uitofp i64 %a to float
  ret float %r
}

define double @uitofp_f64(i64 %a) {
; CHECK-LABEL: uitofp_f64:
; CHECK: bl __cvt_ull_dbl
  %r = uitofp i64 %a to double
  ret double %r
}
