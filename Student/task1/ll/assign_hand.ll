; ModuleID = 'assign_hand.c'
source_filename = "assign_hand.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4      //返回值
  %2 = alloca float, align 4    //float b
  %3 = alloca [2 x i32], align 4 //int a[2]
  store i32 0, i32* %1, align 4  //%1=0
  store float 0x3FFCCCCCC0000000, float* %2, align 4//b=1.8
  %4 = bitcast [2 x i32]* %3 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %4, i8* align 4 bitcast ([2 x i32]* @__const.main.a to i8*), i64 8, i1 false)
  %5 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 0
  %6 = load i32, i32* %5, align 4
  %7 = sitofp i32 %6 to float
  %8 = load float, float* %2, align 4
  %9 = fmul float %7, %8
  %10 = fptosi float %9 to i32
  %11 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 1
  store i32 %10, i32* %11, align 4
  %12 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 1
  %13 = load i32, i32* %12, align 4
  ret i32 %13
}

; Function Attrs: argmemonly nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #1

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind willreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 "}
