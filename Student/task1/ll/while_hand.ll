@a = common dso_local global i32 0, align 4   ;全局声明a
@b = common dso_local global i32 0, align 4   ;全局声明b

define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4                    ;给返回值分配空间
  store i32 0, i32* %1, align 4               ;将返回值初始化为0
  store i32 0, i32* @b, align 4               ;b = 0
  store i32 3, i32* @a, align 4               ;a = 3
  br label %2                                 ;跳转至%2

2:
  %3 = load i32, i32* @a, align 4             ;读取a的值
  %4 = icmp sgt i32 %3, 0                     ;将a与0比较
  br i1 %4, label %5, label %11               ;%4大于0跳转至%5,小于等于0跳转至%11

5:
  %6 = load i32, i32* @a,align 4              ;读取a的值到%6
  %7 = load i32, i32* @b,align 4              ;读取b的值到%7
  %8 = add nsw i32 %6,%7                      ;%8=%6+%7
  store i32 %8,i32* %@b,align 4               ;将%8的值存放到b中
  %9 = load i32, i32* @a, align 4             ;重新读取a的值到%9
  %10 = sub nsw i32 %9, 1                     ;%10=%9-1
  store i32 %10, i32* @a, align 4             ;将%10的值存放到a中
  br label %2                                 ;跳转到%2,开始下一轮while判断

11:
  %12 = load i32, i32* @b, align 4            ;读取b的值到%12
  ret i32 %12                                 ;return %12
}