#include "IRBuilder.h"

#define CONST_INT(num) ConstantInt::get(num, module.get())
#define CONST_FLOAT(num) ConstantFloat::get(num, module.get())

Value *tmp_val = nullptr;

bool pre_enter_scope = false;
std::vector<SyntaxTree::FuncParam> func_fparams;
Function *cur_fun = nullptr;
bool require_lvalue = false;
bool exp_type_is_float = false;
int required_type = 0;
struct true_false_BB
{
    BasicBlock *trueBB = nullptr;
    BasicBlock *falseBB = nullptr;
};
std::list<true_false_BB> IF_While_And_Cond_Stack;
std::list<true_false_BB> IF_While_Or_Cond_Stack;
std::list<true_false_BB> While_Stack;
std::vector<BasicBlock *> cur_basic_block_list;
std::vector<int> array_bounds;
std::vector<int> array_sizes;
int cur_pos;
int cur_depth;
std::map<int, Value *> initval;
std::vector<Constant *> init_val;

// ret BB
BasicBlock *ret_BB;
Value *ret_addr;

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *FLOAT_T;
Type *INT32PTR_T;
Type *FLOATPTR_T;

void IRBuilder::visit(SyntaxTree::Assembly &node)
{
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());
    for (const auto &def : node.global_defs)
    {
        def->accept(*this);
    }
}

// You need to fill them

void IRBuilder::visit(SyntaxTree::InitVal &node)
{
    if (node.isExp)
    {
        node.expr->accept(*this);
        initval[cur_pos] = tmp_val;
        init_val.push_back(dynamic_cast<Constant *>(tmp_val));
        cur_pos++;
    }
    else
    {
        if (cur_depth != 0)
        {
            while (cur_pos % array_sizes[cur_depth] != 0)
            {
                init_val.push_back(CONST_INT(0));
                cur_pos++;
            }
        }
        int cur_start_pos = cur_pos;
        for (const auto &elem : node.elementList)
        {
            cur_depth++;
            elem->accept(*this);
            cur_depth--;
        }
        if (cur_depth != 0)
        {
            while (cur_pos < cur_start_pos + array_sizes[cur_depth])
            {
                init_val.push_back(CONST_INT(0));
                cur_pos++;
            }
        }
        if (cur_depth == 0)
        {
            while (cur_pos < array_sizes[0])
            {
                init_val.push_back(CONST_INT(0));
                cur_pos++;
            }
        }
    }
}

void IRBuilder::visit(SyntaxTree::FuncDef &node)
{
    FunctionType *fun_type;
    Type *ret_type;
    if (node.ret_type == SyntaxTree::Type::INT)
        ret_type = INT32_T;
    else if (node.ret_type == SyntaxTree::Type::FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    std::vector<Type *> param_types;
    std::vector<SyntaxTree::FuncParam>().swap(func_fparams);
    node.param_list->accept(*this);
    for (const auto &param : func_fparams)
    {
        if (param.param_type == SyntaxTree::Type::INT)
        {
            if (param.array_index.empty())
            {
                param_types.push_back(INT32_T);
            }
            else
            {
                param_types.push_back(INT32PTR_T);
            }
        }
        else if (param.param_type == SyntaxTree::Type::FLOAT)
        {
            if (param.array_index.empty())
            {
                param_types.push_back(FLOAT_T);
            }
            else
            {
                param_types.push_back(FLOATPTR_T);
            }
        }
    }
    fun_type = FunctionType::get(ret_type, param_types);
    auto fun = Function::create(fun_type, node.name, module.get());
    scope.push(node.name, fun);
    cur_fun = fun;
    auto funBB = BasicBlock::create(module.get(), "entry", fun);
    builder->set_insert_point(funBB);
    cur_basic_block_list.push_back(funBB);
    scope.enter();
    pre_enter_scope = true;
    std::vector<Value *> args;
    for (auto arg = fun->arg_begin(); arg != fun->arg_end(); arg++)
    {
        args.push_back(*arg);
    }
    int param_num = func_fparams.size();
    for (int i = 0; i < param_num; i++)
    {
        if (func_fparams[i].array_index.empty())
        {
            Value *alloc;
            if (func_fparams[i].param_type == SyntaxTree::Type::INT)
                alloc = builder->create_alloca(INT32_T);
            else if (func_fparams[i].param_type == SyntaxTree::Type::FLOAT)
                alloc = builder->create_alloca(FLOAT_T);
            builder->create_store(args[i], alloc);
            scope.push(func_fparams[i].name, alloc);
        }
        else
        {
            Value *alloc_array;
            if (func_fparams[i].param_type == SyntaxTree::Type::INT)
                alloc_array = builder->create_alloca(INT32PTR_T);
            else if (func_fparams[i].param_type == SyntaxTree::Type::FLOAT)
                alloc_array = builder->create_alloca(FLOATPTR_T);
            builder->create_store(args[i], alloc_array);
            scope.push(func_fparams[i].name, alloc_array);
            array_bounds.clear();
            array_sizes.clear();
            for (auto bound_expr : func_fparams[i].array_index)
            {
                int bound;
                if (bound_expr == nullptr)
                {
                    bound = 1;
                }
                else
                {
                    bound_expr->accept(*this);
                    auto bound_const = dynamic_cast<ConstantInt *>(tmp_val);
                    bound = bound_const->get_value();
                }
                array_bounds.push_back(bound);
            }
            int total_size = 1;
            for (auto iter = array_bounds.rbegin(); iter != array_bounds.rend(); iter++)
            {
                array_sizes.insert(array_sizes.begin(), total_size);
                total_size *= (*iter);
            }
            array_sizes.insert(array_sizes.begin(), total_size);
            // scope.push_size(func_fparams[i].name, array_sizes);
        }
    }
    // ret BB
    if (ret_type == INT32_T)
    {
        ret_addr = builder->create_alloca(INT32_T);
    }
    else if (ret_type = FLOAT_T)
    {
        ret_addr = builder->create_alloca(FLOAT_T);
    }
    ret_BB = BasicBlock::create(module.get(), "ret", fun);

    node.body->accept(*this);

    if (builder->get_insert_block()->get_terminator() == nullptr)
    {
        if (cur_fun->get_return_type()->is_void_type())
        {
            builder->create_br(ret_BB);
        }
        else if (cur_fun->get_return_type()->is_integer_type())
        {
            builder->create_store(CONST_INT(0), ret_addr);
            builder->create_br(ret_BB);
        }
        else if (cur_fun->get_return_type()->is_float_type())
        {
            builder->create_store(CONST_FLOAT(0), ret_addr);
            builder->create_br(ret_BB);
        }
    }
    scope.exit();
    cur_basic_block_list.pop_back();

    // ret BB
    builder->set_insert_point(ret_BB);
    if (fun->get_return_type() == VOID_T)
    {
        builder->create_void_ret();
    }
    else
    {
        auto ret_val = builder->create_load(ret_addr);
        builder->create_ret(ret_val);
    }
}

void IRBuilder::visit(SyntaxTree::FuncFParamList &node)
{
    for (auto param : node.params)
    {
        param->accept(*this);
    }
}

void IRBuilder::visit(SyntaxTree::FuncParam &node)
{
    func_fparams.push_back(node);
}

void IRBuilder::visit(SyntaxTree::VarDef &node)
{
    Type *var_type;
    BasicBlock *cur_fun_entry_block;
    BasicBlock *cur_fun_cur_block;
    if (scope.in_global() == false)
    {
        cur_fun_entry_block = cur_fun->get_entry_block(); // entry block
        cur_fun_cur_block = cur_basic_block_list.back();  // current block
    }
    // if (node.is_constant)
    // {
    //     // constant
    //     Value *var;
    //     if (node.array_length.empty())
    //     {
    //         if (node.btype == SyntaxTree::Type::INT)
    //         {
    //             node.initializers->accept(*this);
    //             if (tmp_val->get_type()->is_float_type())
    //             {
    //                 tmp_val = builder->create_fptosi(tmp_val, INT32_T);
    //             }
    //             auto initializer = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
    //             var = ConstantInt::get(initializer, module.get());
    //         }
    //         else if (node.btype == SyntaxTree::Type::FLOAT)
    //         {
    //             node.initializers->accept(*this);
    //             if (tmp_val->get_type()->is_integer_type())
    //                 tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
    //             auto initializer = dynamic_cast<ConstantFloat *>(tmp_val)->get_value();
    //             var = ConstantFloat::get(initializer, module.get());
    //         }
    //         scope.push(node.name, var);
    //     }
    //     else
    //     {
    //         // array
    //         array_bounds.clear();
    //         array_sizes.clear();
    //         for (const auto &bound_expr : node.array_length)
    //         {
    //             bound_expr->accept(*this);
    //             auto bound_const = dynamic_cast<ConstantInt *>(tmp_val);
    //             auto bound = bound_const->get_value();
    //             array_bounds.push_back(bound);
    //         }
    //         int total_size = 1;
    //         for (auto iter = array_bounds.rbegin(); iter != array_bounds.rend();
    //              iter++)
    //         {
    //             array_sizes.insert(array_sizes.begin(), total_size);
    //             total_size *= (*iter);
    //         }
    //         array_sizes.insert(array_sizes.begin(), total_size);
    //         if (node.btype == SyntaxTree::Type::FLOAT)
    //         {
    //             var_type = FLOAT_T;
    //         }
    //         else
    //         {
    //             var_type = INT32_T;
    //         }
    //         auto *array_type = ArrayType::get(var_type, total_size);

    //         initval.clear();
    //         init_val.clear();
    //         cur_depth = 0;
    //         cur_pos = 0;
    //         node.initializers->accept(*this);
    //         auto initializer = ConstantArray::get(array_type, init_val);

    //         if (scope.in_global())
    //         {
    //             var = GlobalVariable::create(node.name, module.get(), array_type, true, initializer);
    //             scope.push(node.name, var);
    //             // scope.push_size(node.name, array_sizes);
    //             // scope.push_const(node.name, initializer);
    //         }
    //         else
    //         {
    //             auto tmp_terminator = cur_fun_entry_block->get_terminator();
    //             if (tmp_terminator != nullptr)
    //             {
    //                 cur_fun_entry_block->get_instructions().pop_back();
    //             }
    //             var = builder->create_alloca(array_type);
    //             cur_fun_cur_block->get_instructions().pop_back();
    //             cur_fun_entry_block->add_instruction(dynamic_cast<Instruction *>(var));
    //             dynamic_cast<Instruction *>(var)->set_parent(cur_fun_entry_block);
    //             if (tmp_terminator != nullptr)
    //             {
    //                 cur_fun_entry_block->add_instruction(tmp_terminator);
    //             }
    //             for (int i = 0; i < array_sizes[0]; i++)
    //             {
    //                 if (initval[i])
    //                 {
    //                     builder->create_store(initval[i], builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
    //                 }
    //                 else
    //                 {
    //                     if (node.btype == SyntaxTree::Type::FLOAT)
    //                         builder->create_store(CONST_FLOAT(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
    //                     else
    //                         builder->create_store(CONST_INT(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
    //                 }
    //             }
    //             scope.push(node.name, var);
    //             // scope.push_size(node.name, array_sizes);
    //             // scope.push_const(node.name, initializer);
    //         }
    //     }
    // }
    // else
    // {
    if (node.btype == SyntaxTree::Type::FLOAT)
    {
        var_type = FLOAT_T;
    }
    else
    {
        var_type = INT32_T;
    }
    if (node.array_length.empty())
    {
        Value *var;
        if (scope.in_global())
        {
            if (node.is_inited)
            {
                if (node.btype == SyntaxTree::Type::INT)
                {
                    node.initializers->accept(*this);
                    if (tmp_val->get_type()->is_float_type())
                    {
                        tmp_val = builder->create_fptosi(tmp_val, INT32_T);
                    }
                    auto initializer = dynamic_cast<ConstantInt *>(tmp_val);
                    var = GlobalVariable::create(node.name, module.get(), var_type, false, initializer);
                    scope.push(node.name, var);
                }
                else
                {
                    node.initializers->accept(*this);
                    if (tmp_val->get_type()->is_integer_type())
                    {
                        tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
                    }
                    auto initializer = dynamic_cast<ConstantFloat *>(tmp_val);
                    var = GlobalVariable::create(node.name, module.get(), var_type, false, initializer);
                    scope.push(node.name, var);
                }
            }
            else
            {
                auto initializer = ConstantZero::get(var_type, module.get());
                var = GlobalVariable::create(node.name, module.get(), var_type, false, initializer);
                scope.push(node.name, var);
            }
        }
        else
        {
            auto tmp_terminator = cur_fun_entry_block->get_terminator();
            if (tmp_terminator != nullptr)
            {
                cur_fun_entry_block->get_instructions().pop_back();
            }
            var = builder->create_alloca(var_type);
            cur_fun_cur_block->get_instructions().pop_back();
            cur_fun_entry_block->add_instruction(dynamic_cast<Instruction *>(var));
            if (tmp_terminator != nullptr)
            {
                cur_fun_entry_block->add_instruction(tmp_terminator);
            }
            if (node.is_inited)
            {
                node.initializers->accept(*this);
                if (node.btype == SyntaxTree::Type::INT && tmp_val->get_type()->is_float_type())
                {
                    tmp_val = builder->create_fptosi(tmp_val, INT32_T);
                }
                if (node.btype == SyntaxTree::Type::FLOAT && tmp_val->get_type()->is_integer_type())
                {
                    tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
                }
                builder->create_store(tmp_val, var);
            }
            scope.push(node.name, var);
        }
    }
    else
    {
        // array
        array_bounds.clear();
        array_sizes.clear();
        for (const auto &bound_expr : node.array_length)
        {
            bound_expr->accept(*this);
            auto bound_const = dynamic_cast<ConstantInt *>(tmp_val);
            auto bound = bound_const->get_value();
            array_bounds.push_back(bound);
        }
        int total_size = 1;
        for (auto iter = array_bounds.rbegin(); iter != array_bounds.rend();
             iter++)
        {
            array_sizes.insert(array_sizes.begin(), total_size);
            total_size *= (*iter);
        }
        array_sizes.insert(array_sizes.begin(), total_size);
        auto *array_type = ArrayType::get(var_type, total_size);

        Value *var;
        if (scope.in_global())
        {
            if (node.is_inited)
            {
                cur_pos = 0;
                cur_depth = 0;
                initval.clear();
                init_val.clear();
                node.initializers->accept(*this);
                auto initializer = ConstantArray::get(array_type, init_val);
                var = GlobalVariable::create(node.name, module.get(), array_type, false, initializer);
                scope.push(node.name, var);
                // scope.push_size(node.name, array_sizes);
            }
            else
            {
                auto initializer = ConstantZero::get(array_type, module.get());
                var = GlobalVariable::create(node.name, module.get(), array_type, false, initializer);
                scope.push(node.name, var);
                // scope.push_size(node.name, array_sizes);
            }
        }
        else
        {
            auto tmp_terminator = cur_fun_entry_block->get_terminator();
            if (tmp_terminator != nullptr)
            {
                cur_fun_entry_block->get_instructions().pop_back();
            }
            var = builder->create_alloca(array_type);
            cur_fun_cur_block->get_instructions().pop_back();
            cur_fun_entry_block->add_instruction(dynamic_cast<Instruction *>(var));
            if (tmp_terminator != nullptr)
            {
                cur_fun_entry_block->add_instruction(tmp_terminator);
            }
            if (node.is_inited)
            {
                cur_pos = 0;
                cur_depth = 0;
                initval.clear();
                init_val.clear();
                node.initializers->accept(*this);
                for (int i = 0; i < array_sizes[0]; i++)
                {
                    if (initval[i])
                    {
                        builder->create_store(initval[i], builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                    }
                    else
                    {
                        if (node.btype == SyntaxTree::Type::FLOAT)
                            builder->create_store(CONST_FLOAT(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                        else
                            builder->create_store(CONST_INT(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                    }
                }
            }
            scope.push(node.name, var);
            // scope.push_size(node.name, array_sizes);
        } // if of global check
    }
    // }
}

void IRBuilder::visit(SyntaxTree::LVal &node)
{
    // FIXME:may have bug
    auto var = scope.find(node.name, false);
    bool should_return_lvalue = require_lvalue;
    require_lvalue = false;
    if (node.array_index.empty())
    {
        if (should_return_lvalue)
        {
            if (var->get_type()->get_pointer_element_type()->is_array_type())
            {
                tmp_val = builder->create_gep(var, {CONST_INT(0), CONST_INT(0)});
            }
            else if (var->get_type()->get_pointer_element_type()->is_pointer_type())
            {
                tmp_val = builder->create_load(var);
            }
            else
            {
                tmp_val = var;
            }
            require_lvalue = false;
        }
        else
        {
            if (var->get_type()->get_pointer_element_type()->is_float_type())
            {
                auto val_const = dynamic_cast<ConstantFloat *>(var);
                if (val_const != nullptr)
                {
                    tmp_val = val_const;
                }
                else
                {
                    tmp_val = builder->create_load(var);
                }
            }
            else
            {
                auto val_const = dynamic_cast<ConstantInt *>(var);
                if (val_const != nullptr)
                {
                    tmp_val = val_const;
                }
                else
                {
                    tmp_val = builder->create_load(var);
                }
            }
        }
    }
    // else
    // {
    //     auto var_sizes = scope.find_size(node.name);
    //     std::vector<Value *> all_index;
    //     Value *var_index = nullptr;
    //     int index_const = 0;
    //     bool const_check = true;

    //     auto const_array = scope.find_const(node.name);
    //     if (const_array == nullptr)
    //     {
    //         const_check = false;
    //     }

    //     for (int i = 0; i < node.array_index.size(); i++)
    //     {
    //         node.array_index[i]->accept(*this);
    //         all_index.push_back(tmp_val);
    //         if (const_check == true)
    //         {
    //             auto tmp_const = dynamic_cast<ConstantInt *>(tmp_val);
    //             if (tmp_const == nullptr)
    //             {
    //                 const_check = false;
    //             }
    //             else
    //             {
    //                 index_const = var_sizes[i + 1] * tmp_const->get_value() + index_const;
    //             }
    //         }
    //     }

    //     if (should_return_lvalue == false && const_check)
    //     {
    //         ConstantInt *tmp_const = dynamic_cast<ConstantInt *>(const_array->get_element_value(index_const));
    //         tmp_val = CONST_INT(tmp_const->get_value());
    //     }
    //     else
    //     {
    //         for (int i = 0; i < all_index.size(); i++)
    //         {
    //             auto index_val = all_index[i];
    //             Value *one_index;
    //             if (var_sizes[i + 1] > 1)
    //             {
    //                 one_index = builder->create_imul(CONST_INT(var_sizes[i + 1]), index_val);
    //             }
    //             else
    //             {
    //                 one_index = index_val;
    //             }
    //             if (var_index == nullptr)
    //             {
    //                 var_index = one_index;
    //             }
    //             else
    //             {
    //                 var_index = builder->create_iadd(var_index, one_index);
    //             }
    //         } // end for
    //         if (node.array_index.size() > 1 || 1)
    //         {
    //             Value *tmp_ptr;
    //             if (var->get_type()->get_pointer_element_type()->is_pointer_type())
    //             {
    //                 auto tmp_load = builder->create_load(var);
    //                 tmp_ptr = builder->create_gep(tmp_load, {var_index});
    //             }
    //             else
    //             {
    //                 tmp_ptr = builder->create_gep(var, {CONST_INT(0), var_index});
    //             }
    //             if (should_return_lvalue)
    //             {
    //                 tmp_val = tmp_ptr;
    //                 require_lvalue = false;
    //             }
    //             else
    //             {
    //                 tmp_val = builder->create_load(tmp_ptr);
    //             }
    //         }
    //     }
    // }
}
void IRBuilder::visit(SyntaxTree::AssignStmt &node)
{
    node.value->accept(*this);
    auto result = tmp_val;
    require_lvalue = true;
    node.target->accept(*this);
    auto addr = tmp_val;
    if (addr->get_type()->get_pointer_element_type()->is_float_type() && result->get_type()->is_integer_type())
        result = builder->create_sitofp(result, FLOAT_T);
    else if (addr->get_type()->get_pointer_element_type()->is_integer_type() && result->get_type()->is_float_type())
        result = builder->create_fptosi(result, INT32_T);
    builder->create_store(result, addr);
    tmp_val = result;
}

void IRBuilder::visit(SyntaxTree::Literal &node)
{
    if (node.literal_type == SyntaxTree::Type::INT)
    {
        tmp_val = CONST_INT(node.int_const);
    }
    else
    {
        tmp_val = CONST_FLOAT(node.float_const);
    }
}

void IRBuilder::visit(SyntaxTree::ReturnStmt &node)
{
    if (node.ret == nullptr)
    {
        builder->create_br(ret_BB);
    }
    else
    {
        node.ret->accept(*this);
        builder->create_store(tmp_val, ret_addr);
        builder->create_br(ret_BB);
    }
}
void IRBuilder::visit(SyntaxTree::BlockStmt &node)
{
    bool need_exit_scope = !pre_enter_scope;
    if (pre_enter_scope)
    {
        pre_enter_scope = false;
    }
    else
    {
        scope.enter();
    }
    for (auto stmt : node.body)
    {
        stmt->accept(*this);
    }
    if (need_exit_scope)
    {
        scope.exit();
    }
}

void IRBuilder::visit(SyntaxTree::EmptyStmt &node)
{
}

void IRBuilder::visit(SyntaxTree::ExprStmt &node)
{
    node.exp->accept(*this);
}

void IRBuilder::visit(SyntaxTree::UnaryCondExpr &node)
{
    if (node.op == SyntaxTree::UnaryCondOp::NOT)
    {
        node.rhs->accept(*this);
        if (tmp_val->get_type()->is_integer_type())
        {
            auto r_val = tmp_val;
            tmp_val = builder->create_icmp_eq(r_val, CONST_INT(0));
        }
        else
        {
            auto r_val = tmp_val;
            tmp_val = builder->create_fcmp_eq(r_val, CONST_FLOAT(0));
        }
    }
}

void IRBuilder::visit(SyntaxTree::BinaryCondExpr &node)
{
    CmpInst *cond_val;
    // TODO: && || !
    //  if (node.op == SyntaxTree::BinaryCondOp::LAND)
    //  {
    //      auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    //      IF_While_And_Cond_Stack.push_back({trueBB, IF_While_Or_Cond_Stack.back().falseBB});
    //      node.lhs->accept(*this);
    //      IF_While_And_Cond_Stack.pop_back();
    //      auto ret_val = tmp_val;
    //
    //          cond_val = dynamic_cast<CmpInst *>(ret_val);
    //      if (cond_val == nullptr)
    //      {
    //              cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    //          }
    //      }
    //      builder->create_cond_br(cond_val, trueBB, IF_While_Or_Cond_Stack.back().falseBB);
    //      builder->set_insert_point(trueBB);
    //      node.rhs->accept(*this);
    //  }
    //  else if (node.op == SyntaxTree::BinaryCondOp::LOR)
    //  {
    //      auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
    //      IF_While_Or_Cond_Stack.push_back({IF_While_Or_Cond_Stack.back().trueBB, falseBB});
    //      node.lhs->accept(*this);
    //      IF_While_Or_Cond_Stack.pop_back();
    //      auto ret_val = tmp_val;
    //      cond_val = dynamic_cast<CmpInst *>(ret_val);
    //      if (cond_val == nullptr)
    //      {
    //              cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    //      }
    //      builder->create_cond_br(cond_val, IF_While_Or_Cond_Stack.back().trueBB, falseBB);
    //      builder->set_insert_point(falseBB);
    //      node.rhs->accept(*this);
    //  }
    //  else
    //  {
    node.lhs->accept(*this);
    auto l_val = tmp_val;
    node.rhs->accept(*this);
    auto r_val = tmp_val;
    Value *cmp;
    switch (node.op)
    {
    case SyntaxTree::BinaryCondOp::LT:
        if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_integer_type())
            cmp = builder->create_icmp_lt(l_val, r_val);
        else if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_float_type())
        {
            l_val = builder->create_sitofp(l_val, FLOAT_T);
            cmp = builder->create_fcmp_lt(l_val, r_val);
        }
        else if (l_val->get_type()->is_float_type() && r_val->get_type()->is_integer_type())
        {
            r_val = builder->create_sitofp(r_val, FLOAT_T);
            cmp = builder->create_fcmp_lt(l_val, r_val);
        }
        else
        {
            cmp = builder->create_fcmp_lt(l_val, r_val);
        }
        break;
    case SyntaxTree::BinaryCondOp::LTE:
        if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_integer_type())
            cmp = builder->create_icmp_le(l_val, r_val);
        else if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_float_type())
        {
            l_val = builder->create_sitofp(l_val, FLOAT_T);
            cmp = builder->create_fcmp_le(l_val, r_val);
        }
        else if (l_val->get_type()->is_float_type() && r_val->get_type()->is_integer_type())
        {
            r_val = builder->create_sitofp(r_val, FLOAT_T);
            cmp = builder->create_fcmp_le(l_val, r_val);
        }
        else
        {
            cmp = builder->create_fcmp_le(l_val, r_val);
        }
        break;
    case SyntaxTree::BinaryCondOp::GTE:
        if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_integer_type())
            cmp = builder->create_icmp_ge(l_val, r_val);
        else if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_float_type())
        {
            l_val = builder->create_sitofp(l_val, FLOAT_T);
            cmp = builder->create_fcmp_ge(l_val, r_val);
        }
        else if (l_val->get_type()->is_float_type() && r_val->get_type()->is_integer_type())
        {
            r_val = builder->create_sitofp(r_val, FLOAT_T);
            cmp = builder->create_fcmp_ge(l_val, r_val);
        }
        else
        {
            cmp = builder->create_fcmp_ge(l_val, r_val);
        }
        break;
    case SyntaxTree::BinaryCondOp::GT:
        if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_integer_type())
            cmp = builder->create_icmp_gt(l_val, r_val);
        else if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_float_type())
        {
            l_val = builder->create_sitofp(l_val, FLOAT_T);
            cmp = builder->create_fcmp_gt(l_val, r_val);
        }
        else if (l_val->get_type()->is_float_type() && r_val->get_type()->is_integer_type())
        {
            r_val = builder->create_sitofp(r_val, FLOAT_T);
            cmp = builder->create_fcmp_gt(l_val, r_val);
        }
        else
        {
            cmp = builder->create_fcmp_gt(l_val, r_val);
        }
        break;
    case SyntaxTree::BinaryCondOp::EQ:
        if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_integer_type())
            cmp = builder->create_icmp_eq(l_val, r_val);
        else if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_float_type())
        {
            l_val = builder->create_sitofp(l_val, FLOAT_T);
            cmp = builder->create_fcmp_eq(l_val, r_val);
        }
        else if (l_val->get_type()->is_float_type() && r_val->get_type()->is_integer_type())
        {
            r_val = builder->create_sitofp(r_val, FLOAT_T);
            cmp = builder->create_fcmp_eq(l_val, r_val);
        }
        else
        {
            cmp = builder->create_fcmp_eq(l_val, r_val);
        }
        break;
    case SyntaxTree::BinaryCondOp::NEQ:
        if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_integer_type())
            cmp = builder->create_icmp_ne(l_val, r_val);
        else if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_float_type())
        {
            l_val = builder->create_sitofp(l_val, FLOAT_T);
            cmp = builder->create_fcmp_ne(l_val, r_val);
        }
        else if (l_val->get_type()->is_float_type() && r_val->get_type()->is_integer_type())
        {
            r_val = builder->create_sitofp(r_val, FLOAT_T);
            cmp = builder->create_fcmp_ne(l_val, r_val);
        }
        else
        {
            cmp = builder->create_fcmp_ne(l_val, r_val);
        }
        break;
    default:
        break;
    }
    tmp_val = cmp;
    // }
}

void IRBuilder::visit(SyntaxTree::BinaryExpr &node)
{
    if (node.rhs == nullptr)
    {
        node.lhs->accept(*this);
    }
    else
    {
        node.lhs->accept(*this);
        auto l_val = tmp_val;
        node.rhs->accept(*this);
        auto r_val = tmp_val;
        if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_integer_type())
        {
            auto l_val_const = dynamic_cast<ConstantInt *>(l_val);
            auto r_val_const = dynamic_cast<ConstantInt *>(r_val);
            if (l_val_const != nullptr && r_val_const != nullptr)
            {
                switch (node.op)
                {
                case SyntaxTree::BinOp::PLUS:
                    tmp_val = CONST_INT(l_val_const->get_value() + r_val_const->get_value());
                    break;
                case SyntaxTree::BinOp::MINUS:
                    tmp_val = CONST_INT(l_val_const->get_value() - r_val_const->get_value());
                    break;
                case SyntaxTree::BinOp::MULTIPLY:
                    tmp_val = CONST_INT(l_val_const->get_value() * r_val_const->get_value());
                    break;
                case SyntaxTree::BinOp::DIVIDE:
                    tmp_val = CONST_INT(l_val_const->get_value() / r_val_const->get_value());
                    break;
                case SyntaxTree::BinOp::MODULO:
                    tmp_val = CONST_INT(l_val_const->get_value() % r_val_const->get_value());
                }
            }
            else
            {
                switch (node.op)
                {
                case SyntaxTree::BinOp::PLUS:
                    tmp_val = builder->create_iadd(l_val, r_val);
                    break;
                case SyntaxTree::BinOp::MINUS:
                    tmp_val = builder->create_isub(l_val, r_val);
                    break;
                case SyntaxTree::BinOp::MULTIPLY:
                    tmp_val = builder->create_imul(l_val, r_val);
                    break;
                case SyntaxTree::BinOp::DIVIDE:
                    tmp_val = builder->create_isdiv(l_val, r_val);
                    break;
                case SyntaxTree::BinOp::MODULO:
                    tmp_val = builder->create_isrem(l_val, r_val);
                }
            }
        }
        else if (l_val->get_type()->is_float_type() && r_val->get_type()->is_integer_type())
        {
            switch (node.op)
            {
            case SyntaxTree::BinOp::PLUS:
                r_val = builder->create_sitofp(r_val, FLOAT_T);
                tmp_val = builder->create_fadd(l_val, r_val);
                break;
            case SyntaxTree::BinOp::MINUS:
                r_val = builder->create_sitofp(r_val, FLOAT_T);
                tmp_val = builder->create_fsub(l_val, r_val);
                break;
            case SyntaxTree::BinOp::MULTIPLY:
                r_val = builder->create_sitofp(r_val, FLOAT_T);
                tmp_val = builder->create_fmul(l_val, r_val);
                break;
            case SyntaxTree::BinOp::DIVIDE:
                r_val = builder->create_sitofp(r_val, FLOAT_T);
                tmp_val = builder->create_fdiv(l_val, r_val);
                break;
            }
        }
        else if (l_val->get_type()->is_integer_type() && r_val->get_type()->is_float_type())
        {
            switch (node.op)
            {
            case SyntaxTree::BinOp::PLUS:
                l_val = builder->create_sitofp(l_val, FLOAT_T);
                tmp_val = builder->create_fadd(l_val, r_val);
                break;
            case SyntaxTree::BinOp::MINUS:
                l_val = builder->create_sitofp(l_val, FLOAT_T);
                tmp_val = builder->create_fsub(l_val, r_val);
                break;
            case SyntaxTree::BinOp::MULTIPLY:
                l_val = builder->create_sitofp(l_val, FLOAT_T);
                tmp_val = builder->create_fmul(l_val, r_val);
                break;
            case SyntaxTree::BinOp::DIVIDE:
                l_val = builder->create_sitofp(l_val, FLOAT_T);
                tmp_val = builder->create_fdiv(l_val, r_val);
                break;
            }
        }
        else if (l_val->get_type()->is_float_type() && r_val->get_type()->is_float_type())
        {
            auto l_val_const = dynamic_cast<ConstantFloat *>(l_val);
            auto r_val_const = dynamic_cast<ConstantFloat *>(r_val);
            if (l_val_const != nullptr && r_val_const != nullptr)
            {
                switch (node.op)
                {
                case SyntaxTree::BinOp::PLUS:
                    tmp_val = CONST_FLOAT(l_val_const->get_value() + r_val_const->get_value());
                    break;
                case SyntaxTree::BinOp::MINUS:
                    tmp_val = CONST_FLOAT(l_val_const->get_value() - r_val_const->get_value());
                    break;
                case SyntaxTree::BinOp::MULTIPLY:
                    tmp_val = CONST_FLOAT(l_val_const->get_value() * r_val_const->get_value());
                    break;
                case SyntaxTree::BinOp::DIVIDE:
                    tmp_val = CONST_FLOAT(l_val_const->get_value() / r_val_const->get_value());
                    break;
                }
            }
            else
            {
                switch (node.op)
                {
                case SyntaxTree::BinOp::PLUS:
                    tmp_val = builder->create_fadd(l_val, r_val);
                    break;
                case SyntaxTree::BinOp::MINUS:
                    tmp_val = builder->create_fsub(l_val, r_val);
                    break;
                case SyntaxTree::BinOp::MULTIPLY:
                    tmp_val = builder->create_fmul(l_val, r_val);
                    break;
                case SyntaxTree::BinOp::DIVIDE:
                    tmp_val = builder->create_fdiv(l_val, r_val);
                    break;
                }
            }
        }
    }
}

void IRBuilder::visit(SyntaxTree::UnaryExpr &node)
{
    node.rhs->accept(*this);
    if (node.op == SyntaxTree::UnaryOp::MINUS)
    {
        // auto val_const = nullptr;
        if (tmp_val->get_type()->is_integer_type())
        {
            auto val_const = dynamic_cast<ConstantInt *>(tmp_val);
            auto r_val = tmp_val;
            if (val_const != nullptr)
                tmp_val = CONST_INT(0 - val_const->get_value());
            else
                tmp_val = builder->create_isub(CONST_INT(0), r_val);
        }
        else if (tmp_val->get_type()->is_float_type())
        {
            auto val_const = dynamic_cast<ConstantFloat *>(tmp_val);
            auto r_val = tmp_val;
            if (val_const != nullptr)
                tmp_val = CONST_FLOAT(0 - val_const->get_value());
            else
                tmp_val = builder->create_fsub(CONST_FLOAT(0), r_val);
        }
    }
}

void IRBuilder::visit(SyntaxTree::FuncCallStmt &node)
{
    auto fun = static_cast<Function *>(scope.find(node.name, true)); // FIXME:STATIC OR DYNAMIC?
    std::vector<Value *> params;
    int i = 0;
    if (node.name == "starttime" || node.name == "stoptime")
    {
        params.push_back(ConstantInt::get(node.loc.begin.line, module.get()));
    }
    else
    {
        for (auto &param : node.params)
        {
            if (fun->get_function_type()->get_param_type(i)->is_integer_type())
            {
                require_lvalue = false;
                required_type = 1;
            }
            else if (fun->get_function_type()->get_param_type(i)->is_float_type())
            {
                require_lvalue = false;
                required_type = 2;
            }
            else
            {
                require_lvalue = true;
            }
            param->accept(*this);
            require_lvalue = false;
            if (required_type == 1 && tmp_val->get_type()->is_float_type())
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            else if (required_type == 2 && tmp_val->get_type()->is_integer_type())
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            params.push_back(tmp_val);
            required_type = 0;
            i++;
        }
    }
    tmp_val = builder->create_call(static_cast<Function *>(fun), params);
}

void IRBuilder::visit(SyntaxTree::IfStmt &node)
{
    auto trueBB = BasicBlock::create(module.get(), "trueBB_if", cur_fun);
    auto falseBB = BasicBlock::create(module.get(), "falseBB_if", cur_fun);
    auto nextBB = BasicBlock::create(module.get(), "nextBB_if", cur_fun);
    IF_While_Or_Cond_Stack.push_back({trueBB, nextBB});
    if (node.else_statement)
    {
        IF_While_Or_Cond_Stack.back().falseBB = falseBB;
    }
    node.cond_exp->accept(*this);
    IF_While_Or_Cond_Stack.pop_back();
    auto ret_val = tmp_val;

    auto *cond_val = dynamic_cast<CmpInst *>(ret_val);
    if (cond_val == nullptr)
        cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));

    if (node.else_statement)
    {
        builder->create_cond_br(cond_val, trueBB, falseBB);
    }
    else
    {
        builder->create_cond_br(cond_val, trueBB, nextBB);
    }
    cur_basic_block_list.pop_back();
    builder->set_insert_point(trueBB);
    cur_basic_block_list.push_back(trueBB);
    if (dynamic_cast<SyntaxTree::BlockStmt *>(node.if_statement.get()))
    {
        node.if_statement->accept(*this);
    }
    else
    {
        scope.enter();
        node.if_statement->accept(*this);
        scope.exit();
    }

    if (builder->get_insert_block()->get_terminator() == nullptr)
    {
        builder->create_br(nextBB);
    }
    cur_basic_block_list.pop_back();

    if (node.else_statement == nullptr)
    {
        falseBB->erase_from_parent();
    }
    else
    {
        builder->set_insert_point(falseBB);
        cur_basic_block_list.push_back(falseBB);
        if (dynamic_cast<SyntaxTree::BlockStmt *>(node.else_statement.get()))
        {
            node.else_statement->accept(*this);
        }
        else
        {
            scope.enter();
            node.else_statement->accept(*this);
            scope.exit();
        }
        if (builder->get_insert_block()->get_terminator() == nullptr)
        {
            builder->create_br(nextBB);
        }
        cur_basic_block_list.pop_back();
    }

    builder->set_insert_point(nextBB);
    cur_basic_block_list.push_back(nextBB);
    if (nextBB->get_pre_basic_blocks().size() == 0)
    {
        builder->set_insert_point(trueBB);
        nextBB->erase_from_parent();
    }
}

void IRBuilder::visit(SyntaxTree::WhileStmt &node)
{
    auto whileBB = BasicBlock::create(module.get(), "", cur_fun);
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto nextBB = BasicBlock::create(module.get(), "", cur_fun);
    While_Stack.push_back({whileBB, nextBB});
    if (builder->get_insert_block()->get_terminator() == nullptr)
    {
        builder->create_br(whileBB);
    }
    cur_basic_block_list.pop_back();
    builder->set_insert_point(whileBB);
    IF_While_Or_Cond_Stack.push_back({trueBB, nextBB});
    node.cond_exp->accept(*this);
    IF_While_Or_Cond_Stack.pop_back();
    auto ret_val = tmp_val;
    auto *cond_val = dynamic_cast<CmpInst *>(ret_val);
    if (cond_val == nullptr)
    {
        cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    }
    builder->create_cond_br(cond_val, trueBB, nextBB);
    builder->set_insert_point(trueBB);
    cur_basic_block_list.push_back(trueBB);
    if (dynamic_cast<SyntaxTree::BlockStmt *>(node.statement.get()))
    {
        node.statement->accept(*this);
    }
    else
    {
        scope.enter();
        node.statement->accept(*this);
        scope.exit();
    }
    if (builder->get_insert_block()->get_terminator() == nullptr)
    {
        builder->create_br(whileBB);
    }
    cur_basic_block_list.pop_back();
    builder->set_insert_point(nextBB);
    cur_basic_block_list.push_back(nextBB);
    While_Stack.pop_back();
}

void IRBuilder::visit(SyntaxTree::BreakStmt &node)
{
    builder->create_br(While_Stack.back().falseBB);
}

void IRBuilder::visit(SyntaxTree::ContinueStmt &node)
{
    builder->create_br(While_Stack.back().trueBB);
}
