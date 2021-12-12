#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRStmtBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG                                             // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl; // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
  ConstantInt::get(num, module)

#define CONST_FP(num) \
  ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main()
{
  auto module = new Module("SysY code"); // module name是什么无关紧要
  auto builder = new IRStmtBuilder(nullptr, module);
  Type *Int32Type = Type::get_int32_type(module);

  // 全局a,b
  auto zero_initializer = ConstantZero::get(Int32Type, module);
  auto a = GlobalVariable::create("a", module, Int32Type, false, zero_initializer);
  auto b = GlobalVariable::create("b", module, Int32Type, false, zero_initializer);
  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);

  auto bb = BasicBlock::create(module, "entry", mainFun);

  builder->set_insert_point(bb);                      // 一个BB的开始,将当前插入指令点的位置设在bb
  auto retAlloca = builder->create_alloca(Int32Type); // 在内存中分配返回值的位置
  builder->create_store(CONST_INT(0), retAlloca);
  builder->create_store(CONST_INT(0), b);
  builder->create_store(CONST_INT(3), a);
  auto condBB = BasicBlock::create(module, "condBB_while", mainFun);   // 条件BB
  auto trueBB = BasicBlock::create(module, "trueBB_while", mainFun);   // true分支
  auto falseBB = BasicBlock::create(module, "falseBB_while", mainFun); // false分支
  builder->create_br(condBB);

  builder->set_insert_point(condBB);
  auto aload = builder->create_load(a);
  auto icmp = builder->create_icmp_gt(aload, CONST_INT(0));
  builder->create_cond_br(icmp, trueBB, falseBB);

  builder->set_insert_point(trueBB);
  aload = builder->create_load(a);
  auto bload = builder->create_load(b);
  auto add = builder->create_iadd(aload, bload);
  builder->create_store(add, b);
  auto sub = builder->create_isub(aload, CONST_INT(1));
  builder->create_store(sub, a);
  builder->create_br(condBB);

  builder->set_insert_point(falseBB);
  bload = builder->create_load(b);
  builder->create_ret(bload);

  std::cout << module->print();
  delete module;
  return 0;
}
