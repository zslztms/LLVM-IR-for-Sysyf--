; ModuleID = 'assign_hand.c'
source_filename = "assign_hand.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@__const.main.a = private unnamed_addr constant [2 x i32] [i32 2, i32 0], align 4   ;声明一个长度为2，a[0]=2,a[1]=0的数组，用于初始化数组
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4                                    ;给返回值申请空间
  %2 = alloca float, align 4                                  ;float b
  %3 = alloca [2 x i32], align 4                              ;int a[2]
  store i32 0, i32* %1, align 4                               ;%1=0,给返回值初值0
  store float 0x3FFCCCCCC0000000, float* %2, align 4          ;b=1.8
  %4 = bitcast [2 x i32]* %3 to i8*                           ;以i8*为步长访问%3
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %4, i8* align 4 bitcast ([2 x i32]* @__const.main.a to i8*), i64 8, i1 false)  ;调用函数将数组初始化于之前声明的const全局数组一样,第一个参数是writeonly(是被赋值的数组,%4),第二个参数是readonly(是要用来赋值的数组,@__const.main.a)
  %5 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 0        ;%5为a[0]所在位置
  %6 = load i32, i32* %5, align 4                                           ;读取%5的值到%7
  %7 = sitofp i32 %6 to float                                               ;将%6的类型从int转化到float
  %8 = load float, float* %2, align 4                                       ;将%2所在值加载到%8
  %9 = fmul float %7, %8                                                    ;%9 = float %7 * float %8
  %10 = fptosi float %9 to i32                                              ;将%9的类型从float转化到int
  %11 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 1       ;%11为a[1]所在地址
  store i32 %10, i32* %11, align 4                                          ;*(%11)=%10
  %12 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 1       ;%12为a[1]所在地址
  %13 = load i32, i32* %12, align 4                                         ;%13=*(%12)
  ret i32 %13                                                               ;return %13
}

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #1

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind willreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 "}
