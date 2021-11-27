; ModuleID = 'func_hand.c'
source_filename = "func_hand.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define dso_local i32 @add(i32 %0, i32 %1) #0 {
  %2 = alloca i32, align 4             ;分配空间
  %3 = alloca i32, align 4             ;分配空间
  store i32 %0, i32* %2, align 4       ;将形参a的值存入%2
  store i32 %1, i32* %3, align 4       ;将形参b的值存入%3
  %4 = load i32, i32* %2, align 4      ;将%2的值读取出来
  %5 = load i32, i32* %3, align 4      ;将%3的值读取出来
  %6 = add nsw i32 %4, %5              ;%6=a+b
  %7 = sub nsw i32 %6, 1               ;%7=%6-1
  ret i32 %7                           ;return %7
}


define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4             ;给返回值分配空间
  %2 = alloca i32, align 4             ;分配空间
  %3 = alloca i32, align 4             ;分配空间
  %4 = alloca i32, align 4             ;分配空间
  store i32 0, i32* %1,align 4         ;初始化返回值为0
  store i32 3, i32* %2, align 4        ;将3存入%2,a=3
  store i32 2, i32* %3, align 4        ;将2存入%3,b=2
  store i32 5, i32* %4, align 4        ;将5存入%4,c=5
  %5 = load i32, i32* %4, align 4      ;将%4的值读取到%5,c
  %6 = load i32, i32* %2, align 4      ;将%2的值读取到%6,a
  %7 = load i32, i32* %3, align 4      ;将%3的值读取到%7,b
  %8 = call i32 @add(i32 %6, i32 %7)   ;调用函数add,%6,%7为形参的值,%8为返回值
  %9 = add nsw i32 %5, %8              ;%8 = %5 + %8
  ret i32 %9                           ;return %9
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 "}