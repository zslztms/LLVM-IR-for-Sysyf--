; ModuleID = 'if_test.c'
source_filename = "if_test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@a = common dso_local global i32 0, align 4   ;全局声明a

define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4                    ;给返回值分配空间
  store i32 0, i32* %1, align 4               ;将返回值初始化为0
  store i32 10, i32* @a, align 4              ;a = 10
  %2 = load i32, i32* @a, align 4             ;读取a的值至%2
  %3 = icmp sgt i32 %2, 0                     ;将%2与0比较
  br i1 %3, label %4, label %6                ;%3大于0跳转至%4,小于等于0跳转至%6

4:
  %5 = load i32, i32* @a, align 4             ;读取a的值至%5
  store i32 %5, i32* %1, align 4              ;将%5的值存放在%1中
  br label %7                                 ;跳转至%7

6:
  store i32 0, i32* %1, align 4               ;将0存放在%1中
  br label %7                                 ;跳转至%7

7:
  %8 = load i32, i32* %1, align 4             ;读取%1的值至%8
  ret i32 %8                                  ;return %8
}



attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 "}
