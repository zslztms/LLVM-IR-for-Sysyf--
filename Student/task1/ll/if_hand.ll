; ModuleID = 'if_test.c'
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