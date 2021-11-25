; ModuleID = 'while_test.c'
source_filename = "while_test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@b = common dso_local global i32 0, align 4
@a = common dso_local global i32 0, align 4

define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4      ;给返回值声明
  store i32 0, i32* %1, align 4
  store i32 0, i32* @b, align 4
  store i32 3, i32* @a, align 4
  br label %2

2:                                       
  %3 = load i32, i32* @a, align 4    ;读取a
  %4 = icmp sgt i32 %3, 0     ;将a与0比较
  br i1 %4, label %5, label %10

5:                            
  %6 = load i32, i32* @a,align 4
  %7 = load i32, i32* @b,align 4
  %8 = add nsw i32 %6,%7
  store i32 %8,i32* %@b,align 4 
  %9 = sub nsw %6,align 4
  store i32 %9,i32* @a,align 4
  br label %2

10:                                      
  %11 = load i32, i32* @b, align 4
  ret i32 %11
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 "}
