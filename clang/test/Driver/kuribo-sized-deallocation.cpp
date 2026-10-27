// The CodeWarrior runtime targeted by Kuribo (and the games built with it)
// only provides unsized operator delete (__dl__FPv), so sized deallocation is
// off by default there unless requested explicitly.

// RUN: %clang -### --target=powerpc-ibm-kuribo-eabi -std=gnu++17 -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NOSIZED
// RUN: %clang -### --target=powerpc-ibm-kuribo-eabi -std=gnu++17 \
// RUN:   -fno-sized-deallocation -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NOSIZED
// RUN: %clang -### --target=powerpc-ibm-kuribo-eabi -std=gnu++17 \
// RUN:   -fsized-deallocation -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SIZED
// RUN: %clang -### --target=powerpc-unknown-linux-gnu -std=gnu++17 -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LINUX

// NOSIZED: "-fno-sized-deallocation"
// NOSIZED-NOT: "-fsized-deallocation"

// SIZED: "-fsized-deallocation"
// SIZED-NOT: "-fno-sized-deallocation"

// LINUX-NOT: "-f{{(no-)?}}sized-deallocation"
