//! LLVM IR code generation for the Hunnu compiler.
//!
//! Converts [`ASTNode`] trees into LLVM IR, object files, or assembly.
//! Only available with the `llvm-codegen` feature.

use llvm_sys::prelude::*;
use llvm_sys::*;
use llvm_sys::core::*;
use super::ast::*;
use super::lexer::TokenType;
use std::ffi::{CStr, CString};
use std::ptr;

struct LoopInfo {
    cond_block: LLVMBasicBlockRef,
    end_block: LLVMBasicBlockRef,
}

/// LLVM IR code generator for Hunnu programs.
pub struct CodeGen {
    context: LLVMContextRef,
    module: LLVMModuleRef,
    builder: LLVMBuilderRef,
    named_values: std::collections::HashMap<String, LLVMValueRef>,
    printf_fn: Option<LLVMValueRef>,
    concat_fn: Option<LLVMValueRef>,
    current_function: Option<LLVMValueRef>,
    loop_stack: Vec<LoopInfo>,
    struct_types: std::collections::HashMap<String, LLVMTypeRef>,
    struct_fields: std::collections::HashMap<String, Vec<String>>,
}

impl CodeGen {
    /// Create a new code generator with the given module name.
    pub fn new(module_name: &str) -> Self {
        unsafe {
            let context = LLVMContextCreate();
            let name = CString::new(module_name).unwrap();
            let module = LLVMModuleCreateWithNameInContext(name.as_ptr(), context);
            let builder = LLVMCreateBuilderInContext(context);

            CodeGen {
                context,
                module,
                builder,
                named_values: std::collections::HashMap::new(),
                printf_fn: None,
                concat_fn: None,
                current_function: None,
                loop_stack: Vec::new(),
                struct_types: std::collections::HashMap::new(),
                struct_fields: std::collections::HashMap::new(),
            }
        }
    }

    fn i64_type(&self) -> LLVMTypeRef {
        unsafe { LLVMInt64TypeInContext(self.context) }
    }

    fn i32_type(&self) -> LLVMTypeRef {
        unsafe { LLVMInt32TypeInContext(self.context) }
    }

    fn i1_type(&self) -> LLVMTypeRef {
        unsafe { LLVMInt1TypeInContext(self.context) }
    }

    fn i8_type(&self) -> LLVMTypeRef {
        unsafe { LLVMInt8TypeInContext(self.context) }
    }

    fn void_type(&self) -> LLVMTypeRef {
        unsafe { LLVMVoidTypeInContext(self.context) }
    }

    fn create_block(&self, name: &str) -> LLVMBasicBlockRef {
        unsafe {
            let c_name = CString::new(name).unwrap();
            LLVMAppendBasicBlockInContext(self.context, self.current_function.unwrap(), c_name.as_ptr())
        }
    }

    fn build_string_global(&self, s: &str, name: &str) -> LLVMValueRef {
        unsafe {
            let null_term = format!("{}\0", s);
            let global_name = CString::new(name).unwrap();
            let global = LLVMAddGlobal(self.module,
                LLVMArrayType(self.i8_type(), null_term.len() as u32),
                global_name.as_ptr());
            LLVMSetInitializer(global, LLVMConstStringInContext(self.context,
                null_term.as_ptr(), null_term.len() as u32, 1));
            LLVMSetLinkage(global, LLVMLinkage::LLVMPrivateLinkage);
            LLVMSetGlobalConstant(global, 1);

            let zero = LLVMConstInt(self.i32_type(), 0, 0);
            let gep_indices = [zero, zero];
            LLVMBuildInBoundsGEP2(self.builder,
                LLVMArrayType(self.i8_type(), null_term.len() as u32),
                global,
                gep_indices.as_mut_ptr(),
                2,
                c"str".as_ptr())
        }
    }

    fn get_or_create_printf(&mut self) -> LLVMValueRef {
        unsafe {
            if let Some(f) = self.printf_fn {
                return f;
            }
            let i8_ptr = LLVMPointerType(self.i8_type(), 0);
            let mut printf_param_types = vec![i8_ptr];
            let printf_type = LLVMFunctionType(
                self.i32_type(),
                printf_param_types.as_mut_ptr(),
                1,
                1,
            );
            let printf_name = CString::new("printf").unwrap();
            let printf = LLVMAddFunction(self.module, printf_name.as_ptr(), printf_type);
            self.printf_fn = Some(printf);
            printf
        }
    }

    fn get_or_create_concat(&mut self) -> LLVMValueRef {
        unsafe {
            if let Some(f) = self.concat_fn {
                return f;
            }
            let i8_ptr = LLVMPointerType(self.i8_type(), 0);
            let mut concat_param_types = vec![i8_ptr, i8_ptr];
            let concat_type = LLVMFunctionType(
                i8_ptr,
                concat_param_types.as_mut_ptr(),
                2,
                0,
            );
            let concat_name = CString::new("__hunnu_concat").unwrap();
            let concat = LLVMAddFunction(self.module, concat_name.as_ptr(), concat_type);
            self.concat_fn = Some(concat);
            concat
        }
    }

    fn emit_printf_int(&mut self, value: LLVMValueRef) {
        unsafe {
            let printf = self.get_or_create_printf();
            let fmt_ptr = self.build_string_global("%lld\n", "int_fmt");
            let mut args = [fmt_ptr, value];
            LLVMBuildCall2(self.builder,
                LLVMGlobalGetValueType(printf),
                printf,
                args.as_mut_ptr(),
                2,
                c"printf_call".as_ptr());
        }
    }

    fn emit_printf_str(&mut self, str_ptr: LLVMValueRef) {
        unsafe {
            let printf = self.get_or_create_printf();
            let fmt_ptr = self.build_string_global("%s\n", "str_fmt");
            let mut args = [fmt_ptr, str_ptr];
            LLVMBuildCall2(self.builder,
                LLVMGlobalGetValueType(printf),
                printf,
                args.as_mut_ptr(),
                2,
                c"printf_call".as_ptr());
        }
    }

    fn emit_printf_float(&mut self, value: LLVMValueRef) {
        unsafe {
            let printf = self.get_or_create_printf();
            let fmt_ptr = self.build_string_global("%g\n", "float_fmt");
            let mut args = [fmt_ptr, value];
            LLVMBuildCall2(self.builder,
                LLVMGlobalGetValueType(printf),
                printf,
                args.as_mut_ptr(),
                2,
                c"printf_call".as_ptr());
        }
    }

    /// Generate LLVM IR for a parsed program AST.
    pub fn generate(&mut self, program: &ASTNode) -> Result<(), String> {
        match &program.data {
            NodeData::Program { statements } => {
                // First pass: register type declarations and extern functions
                for stmt in statements {
                    self.register_decl(stmt);
                }
                // Second pass: generate code
                for stmt in statements {
                    self.generate_statement(stmt)?;
                }
                Ok(())
            }
            _ => Err("Expected program node".to_string()),
        }
    }

    fn register_decl(&mut self, node: &ASTNode) {
        match &node.data {
            NodeData::TypeDecl { name, fields } => {
                unsafe {
                    let field_types: Vec<LLVMTypeRef> = fields.iter().map(|_| self.i64_type()).collect();
                    let struct_name = CString::new(name.as_str()).unwrap();
                    let struct_type = LLVMStructCreateNamed(self.context, struct_name.as_ptr());
                    LLVMStructSetBody(struct_type, field_types.as_ptr() as *mut LLVMTypeRef, field_types.len() as u32, 0);
                    self.struct_types.insert(name.clone(), struct_type);
                    self.struct_fields.insert(name.clone(), fields.clone());
                }
            }
            NodeData::ExternFn { name, params, returns_int, .. } => {
                unsafe {
                    let c_name = CString::new(name.as_str()).unwrap();
                    let existing = LLVMGetNamedFunction(self.module, c_name.as_ptr());
                    if !existing.is_null() {
                        return;
                    }
                    let return_type = match returns_int {
                        2 => LLVMPointerType(self.i8_type(), 0),
                        3 => LLVMDoubleTypeInContext(self.context),
                        _ if *returns_int != 0 => self.i64_type(),
                        _ => self.void_type(),
                    };
                    let mut param_types: Vec<LLVMTypeRef> = Vec::new();
                    for _ in params {
                        param_types.push(self.i64_type());
                    }
                    let function_type = LLVMFunctionType(
                        return_type,
                        param_types.as_mut_ptr(),
                        param_types.len() as u32,
                        0,
                    );
                    LLVMAddFunction(self.module, c_name.as_ptr(), function_type);
                }
            }
            _ => {}
        }
    }

    fn generate_statement(&mut self, node: &ASTNode) -> Result<(), String> {
        match &node.data {
            NodeData::VarDecl { name, initializer } => {
                self.generate_var_decl(name, initializer)
            }
            NodeData::FnDecl { name, params, body } => {
                self.generate_fn_decl(name, params, body)
            }
            NodeData::PrintStmt { argument } => {
                self.generate_print(argument)
            }
            NodeData::Block { statements } => {
                for stmt in statements {
                    self.generate_statement(stmt)?;
                }
                Ok(())
            }
            NodeData::ExprStmt { expression } => {
                self.generate_expression(expression)?;
                Ok(())
            }
            NodeData::ReturnStmt { value } => {
                match value {
                    Some(v) => {
                        let val = self.generate_expression(v)?;
                        unsafe { LLVMBuildRet(self.builder, val); }
                    }
                    None => {
                        unsafe { LLVMBuildRetVoid(self.builder); }
                    }
                }
                Ok(())
            }
            NodeData::IfStmt { condition, then_branch, else_branch } => {
                self.generate_if(condition, then_branch, else_branch)
            }
            NodeData::WhileStmt { condition, body } => {
                self.generate_while(condition, body)
            }
            NodeData::ForStmt { initializer, condition, update, body } => {
                self.generate_for(initializer, condition, update, body)
            }
            NodeData::BreakStmt => {
                if let Some(loop_info) = self.loop_stack.last() {
                    unsafe { LLVMBuildBr(self.builder, loop_info.end_block); }
                }
                Ok(())
            }
            NodeData::ContinueStmt => {
                if let Some(loop_info) = self.loop_stack.last() {
                    unsafe { LLVMBuildBr(self.builder, loop_info.cond_block); }
                }
                Ok(())
            }
            NodeData::TryStmt { try_block, catch_var, catch_block, finally_block } => {
                self.generate_try(try_block, catch_var, catch_block, finally_block)
            }
            _ => {
                self.generate_expression(node)?;
                Ok(())
            }
        }
    }

    fn generate_var_decl(&mut self, name: &str, initializer: &ASTNode) -> Result<(), String> {
        let value = self.generate_expression(initializer)?;
        unsafe {
            let c_name = CString::new(name).unwrap();
            let alloca = LLVMBuildAlloca(self.builder, self.i64_type(), c_name.as_ptr());
            LLVMBuildStore(self.builder, value, alloca);
            self.named_values.insert(name.to_string(), alloca);
        }
        Ok(())
    }

    fn generate_fn_decl(&mut self, name: &str, params: &[String], body: &ASTNode) -> Result<(), String> {
        unsafe {
            let c_name = CString::new(name).unwrap();
            let is_main = name == "main";
            let return_type = if is_main { self.i64_type() } else { self.void_type() };

            let param_count = params.len();
            let mut param_types: Vec<LLVMTypeRef> = Vec::new();
            for _ in params {
                param_types.push(self.i64_type());
            }

            let function_type = LLVMFunctionType(
                return_type,
                param_types.as_mut_ptr(),
                param_count as u32,
                0,
            );

            let function = LLVMAddFunction(self.module, c_name.as_ptr(), function_type);

            if is_main {
                let target_triple = LLVMGetDefaultTargetTriple();
                LLVMSetTarget(self.module, target_triple);
            }

            let entry = LLVMAppendBasicBlockInContext(self.context, function, c"entry".as_ptr());
            LLVMPositionBuilderAtEnd(self.builder, entry);

            self.current_function = Some(function);

            for (i, param_name) in params.iter().enumerate() {
                let c_param_name = CString::new(param_name.as_str()).unwrap();
                let param = LLVMGetParam(function, i as u32);
                LLVMSetValueName(param, c_param_name.as_ptr());

                let alloca = LLVMBuildAlloca(self.builder, self.i64_type(), c_param_name.as_ptr());
                LLVMBuildStore(self.builder, param, alloca);
                self.named_values.insert(param_name.to_string(), alloca);
            }

            self.generate_statement(body)?;

            let current_block = LLVMGetInsertBlock(self.builder);
            let last_instruction = LLVMGetLastInstruction(LLVMGetBasicBlockTerminator(current_block));
            if last_instruction.is_null() {
                if is_main {
                    let zero = LLVMConstInt(self.i64_type(), 0, 0);
                    LLVMBuildRet(self.builder, zero);
                } else {
                    LLVMBuildRetVoid(self.builder);
                }
            }

            self.named_values.clear();
            self.current_function = None;

            Ok(())
        }
    }

    fn generate_if(&mut self, condition: &ASTNode, then_branch: &ASTNode, else_branch: &Option<Box<ASTNode>>) -> Result<(), String> {
        unsafe {
            let cond_val = self.generate_expression(condition)?;
            let zero = LLVMConstInt(self.i64_type(), 0, 0);
            let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntNE, cond_val, zero, c"if_cond".as_ptr());

            let then_block = self.create_block("then");
            let else_block = self.create_block("else");
            let merge_block = self.create_block("if_merge");

            LLVMBuildCondBr(self.builder, cmp, then_block, else_block);

            LLVMPositionBuilderAtEnd(self.builder, then_block);
            self.generate_statement(then_branch)?;
            let then_term = LLVMGetBasicBlockTerminator(then_block);
            if then_term.is_null() {
                LLVMBuildBr(self.builder, merge_block);
            }

            LLVMPositionBuilderAtEnd(self.builder, else_block);
            if let Some(else_b) = else_branch {
                self.generate_statement(else_b)?;
            }
            let else_term = LLVMGetBasicBlockTerminator(else_block);
            if else_term.is_null() {
                LLVMBuildBr(self.builder, merge_block);
            }

            LLVMPositionBuilderAtEnd(self.builder, merge_block);
            Ok(())
        }
    }

    fn generate_while(&mut self, condition: &ASTNode, body: &ASTNode) -> Result<(), String> {
        unsafe {
            let cond_block = self.create_block("while_cond");
            let body_block = self.create_block("while_body");
            let end_block = self.create_block("while_end");

            LLVMBuildBr(self.builder, cond_block);
            LLVMPositionBuilderAtEnd(self.builder, cond_block);

            let cond_val = self.generate_expression(condition)?;
            let zero = LLVMConstInt(self.i64_type(), 0, 0);
            let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntNE, cond_val, zero, c"while_cmp".as_ptr());
            LLVMBuildCondBr(self.builder, cmp, body_block, end_block);

            LLVMPositionBuilderAtEnd(self.builder, body_block);
            self.loop_stack.push(LoopInfo { cond_block, end_block });
            self.generate_statement(body)?;
            self.loop_stack.pop();

            let body_term = LLVMGetBasicBlockTerminator(body_block);
            if body_term.is_null() {
                LLVMBuildBr(self.builder, cond_block);
            }

            LLVMPositionBuilderAtEnd(self.builder, end_block);
            Ok(())
        }
    }

    fn generate_for(&mut self, initializer: &Option<Box<ASTNode>>, condition: &Option<Box<ASTNode>>, update: &Option<Box<ASTNode>>, body: &ASTNode) -> Result<(), String> {
        unsafe {
            if let Some(init) = initializer {
                self.generate_statement(init)?;
            }

            let cond_block = self.create_block("for_cond");
            let body_block = self.create_block("for_body");
            let update_block = self.create_block("for_update");
            let end_block = self.create_block("for_end");

            LLVMBuildBr(self.builder, cond_block);
            LLVMPositionBuilderAtEnd(self.builder, cond_block);

            let cond_val = if let Some(cond) = condition {
                self.generate_expression(cond)?
            } else {
                LLVMConstInt(self.i64_type(), 1, 0)
            };
            let zero = LLVMConstInt(self.i64_type(), 0, 0);
            let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntNE, cond_val, zero, c"for_cmp".as_ptr());
            LLVMBuildCondBr(self.builder, cmp, body_block, end_block);

            LLVMPositionBuilderAtEnd(self.builder, body_block);
            self.loop_stack.push(LoopInfo { cond_block: update_block, end_block });
            self.generate_statement(body)?;
            self.loop_stack.pop();

            let body_term = LLVMGetBasicBlockTerminator(body_block);
            if body_term.is_null() {
                LLVMBuildBr(self.builder, update_block);
            }

            LLVMPositionBuilderAtEnd(self.builder, update_block);
            if let Some(upd) = update {
                self.generate_expression(upd)?;
            }
            LLVMBuildBr(self.builder, cond_block);

            LLVMPositionBuilderAtEnd(self.builder, end_block);
            Ok(())
        }
    }

    fn generate_try(&mut self, try_block: &ASTNode, _catch_var: &Option<String>, catch_block: &Option<Box<ASTNode>>, finally_block: &Option<Box<ASTNode>>) -> Result<(), String> {
        // Without runtime exception support, we execute try, catch, finally sequentially.
        // The catch block is only reached if try body executes normally (no exception mechanism yet).
        self.generate_statement(try_block)?;
        if let Some(catch) = catch_block {
            self.generate_statement(catch)?;
        }
        if let Some(finally) = finally_block {
            self.generate_statement(finally)?;
        }
        Ok(())
    }

    fn generate_print(&mut self, argument: &ASTNode) -> Result<(), String> {
        match &argument.data {
            NodeData::Literal { literal_type, value } => {
                match literal_type {
                    LiteralType::Int => {
                        let val = self.generate_literal(literal_type, value)?;
                        self.emit_printf_int(val);
                    }
                    LiteralType::Float => {
                        let val = self.generate_literal(literal_type, value)?;
                        self.emit_printf_float(val);
                    }
                    LiteralType::String => {
                        let val = self.generate_literal(literal_type, value)?;
                        self.emit_printf_str(val);
                    }
                    LiteralType::Bool => {
                        let val = self.generate_literal(literal_type, value)?;
                        self.emit_printf_int(val);
                    }
                }
            }
            _ => {
                let value = self.generate_expression(argument)?;
                self.emit_printf_int(value);
            }
        }
        Ok(())
    }

    fn generate_expression(&mut self, node: &ASTNode) -> Result<LLVMValueRef, String> {
        match &node.data {
            NodeData::Literal { literal_type, value } => {
                self.generate_literal(literal_type, value)
            }
            NodeData::Identifier { name } => {
                self.generate_identifier(name)
            }
            NodeData::BinaryExpr { operator, left, right } => {
                self.generate_binary_expr(operator, left, right)
            }
            NodeData::CallExpr { name, args } => {
                self.generate_call_expr(name, args)
            }
            NodeData::Assign { name, value } => {
                self.generate_assignment(name, value)
            }
            NodeData::UnaryExpr { operator, operand } => {
                self.generate_unary_expr(operator, operand)
            }
            NodeData::ArrayExpr { elements } => {
                self.generate_array_expr(elements)
            }
            NodeData::IndexExpr { array, index } => {
                self.generate_index_expr(array, index)
            }
            NodeData::IndexAssign { array, index, value } => {
                self.generate_index_assign(array, index, value)
            }
            NodeData::StringConcat { left, right } => {
                self.generate_string_concat(left, right)
            }
            NodeData::MatchExpr { value, patterns, bodies } => {
                self.generate_match_expr(value, patterns, bodies)
            }
            NodeData::FieldAccess { object, field } => {
                self.generate_field_access(object, field)
            }
            NodeData::AddressOf { operand } => {
                self.generate_address_of(operand)
            }
            NodeData::Dereference { operand } => {
                self.generate_dereference(operand)
            }
            _ => {
                unsafe {
                    Ok(LLVMConstInt(self.i64_type(), 0, 0))
                }
            }
        }
    }

    fn generate_literal(&mut self, literal_type: &LiteralType, value: &LiteralValue) -> Result<LLVMValueRef, String> {
        unsafe {
            match literal_type {
                LiteralType::Int => {
                    if let LiteralValue::Int(v) = value {
                        Ok(LLVMConstInt(self.i64_type(), *v as u64, 0))
                    } else {
                        Err("Expected int literal".to_string())
                    }
                }
                LiteralType::Float => {
                    if let LiteralValue::Float(v) = value {
                        Ok(LLVMConstReal(LLVMDoubleTypeInContext(self.context), *v))
                    } else {
                        Err("Expected float literal".to_string())
                    }
                }
                LiteralType::Bool => {
                    if let LiteralValue::Bool(v) = value {
                        Ok(LLVMConstInt(self.i64_type(), if *v { 1 } else { 0 }, 0))
                    } else {
                        Err("Expected bool literal".to_string())
                    }
                }
                LiteralType::String => {
                    if let LiteralValue::String(s) = value {
                        let name = format!("str_{}", s.len());
                        let c_string = CString::new(s.as_str()).unwrap_or(CString::new("").unwrap());
                        let null_term = format!("{}\0", c_string.to_str().unwrap_or(""));
                        let global_name = CString::new(name).unwrap();
                        let global = LLVMAddGlobal(self.module,
                            LLVMArrayType(self.i8_type(), null_term.len() as u32),
                            global_name.as_ptr());
                        LLVMSetInitializer(global, LLVMConstStringInContext(self.context,
                            null_term.as_ptr(), null_term.len() as u32, 1));
                        LLVMSetLinkage(global, LLVMLinkage::LLVMPrivateLinkage);
                        LLVMSetGlobalConstant(global, 1);

                        let zero = LLVMConstInt(self.i32_type(), 0, 0);
                        let gep_indices = [zero, zero];
                        let ptr = LLVMBuildInBoundsGEP2(self.builder,
                            LLVMArrayType(self.i8_type(), null_term.len() as u32),
                            global,
                            gep_indices.as_mut_ptr(),
                            2,
                            c"str".as_ptr());
                        Ok(ptr)
                    } else {
                        Err("Expected string literal".to_string())
                    }
                }
            }
        }
    }

    fn generate_identifier(&mut self, name: &str) -> Result<LLVMValueRef, String> {
        unsafe {
            if let Some(alloca) = self.named_values.get(name) {
                let loaded = LLVMBuildLoad2(self.builder, self.i64_type(), *alloca, c"load".as_ptr());
                Ok(loaded)
            } else {
                let c_name = CString::new(name).unwrap();
                let global = LLVMGetNamedGlobal(self.module, c_name.as_ptr());
                if !global.is_null() {
                    Ok(LLVMBuildLoad2(self.builder, self.i64_type(), global, c"load".as_ptr()))
                } else {
                    Ok(LLVMConstInt(self.i64_type(), 0, 0))
                }
            }
        }
    }

    fn generate_assignment(&mut self, name: &str, value: &ASTNode) -> Result<LLVMValueRef, String> {
        let val = self.generate_expression(value)?;
        unsafe {
            if let Some(alloca) = self.named_values.get(name) {
                LLVMBuildStore(self.builder, val, *alloca);
            } else {
                let c_name = CString::new(name).unwrap();
                let global = LLVMGetNamedGlobal(self.module, c_name.as_ptr());
                if !global.is_null() {
                    LLVMBuildStore(self.builder, val, global);
                }
            }
            Ok(val)
        }
    }

    fn generate_unary_expr(&mut self, operator: &TokenType, operand: &ASTNode) -> Result<LLVMValueRef, String> {
        let op_val = self.generate_expression(operand)?;
        unsafe {
            match operator {
                TokenType::Minus => {
                    Ok(LLVMBuildNeg(self.builder, op_val, c"neg".as_ptr()))
                }
                TokenType::Not => {
                    let zero = LLVMConstInt(self.i64_type(), 0, 0);
                    let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntEQ, op_val, zero, c"not".as_ptr());
                    Ok(LLVMBuildIntCast2(self.builder, cmp, self.i64_type(), 0, c"not_int".as_ptr()))
                }
                _ => Ok(op_val),
            }
        }
    }

    fn generate_array_expr(&mut self, elements: &[ASTNode]) -> Result<LLVMValueRef, String> {
        unsafe {
            let count = elements.len() as u32;
            let array_type = LLVMArrayType(self.i64_type(), count);
            let alloca = LLVMBuildAlloca(self.builder, array_type, c"array".as_ptr());

            for (i, elem) in elements.iter().enumerate() {
                let val = self.generate_expression(elem)?;
                let idx = LLVMConstInt(self.i64_type(), i as u64, 0);
                let zero = LLVMConstInt(self.i64_type(), 0, 0);
                let indices = [zero, idx];
                let ptr = LLVMBuildInBoundsGEP2(self.builder, array_type, alloca,
                    indices.as_mut_ptr(), 2,
                    CString::new(format!("array_ptr_{}", i)).unwrap().as_ptr());
                LLVMBuildStore(self.builder, val, ptr);
            }

            let zero = LLVMConstInt(self.i64_type(), 0, 0);
            let indices = [zero, zero];
            let ptr = LLVMBuildInBoundsGEP2(self.builder, array_type, alloca,
                indices.as_mut_ptr(), 2, c"array_decay".as_ptr());
            Ok(ptr)
        }
    }

    fn generate_index_expr(&mut self, array: &ASTNode, index: &ASTNode) -> Result<LLVMValueRef, String> {
        let array_ptr = self.generate_expression(array)?;
        let idx_val = self.generate_expression(index)?;
        unsafe {
            let zero = LLVMConstInt(self.i64_type(), 0, 0);
            let indices = [idx_val];
            let elem_ptr = LLVMBuildGEP2(self.builder, self.i64_type(), array_ptr,
                indices.as_mut_ptr(), 1, c"index_ptr".as_ptr());
            Ok(LLVMBuildLoad2(self.builder, self.i64_type(), elem_ptr, c"index_load".as_ptr()))
        }
    }

    fn generate_index_assign(&mut self, array: &ASTNode, index: &ASTNode, value: &ASTNode) -> Result<LLVMValueRef, String> {
        let array_ptr = self.generate_expression(array)?;
        let idx_val = self.generate_expression(index)?;
        let val = self.generate_expression(value)?;
        unsafe {
            let zero = LLVMConstInt(self.i64_type(), 0, 0);
            let indices = [idx_val];
            let elem_ptr = LLVMBuildGEP2(self.builder, self.i64_type(), array_ptr,
                indices.as_mut_ptr(), 1, c"index_ptr".as_ptr());
            LLVMBuildStore(self.builder, val, elem_ptr);
            Ok(val)
        }
    }

    fn generate_string_concat(&mut self, left: &ASTNode, right: &ASTNode) -> Result<LLVMValueRef, String> {
        let left_val = self.generate_expression(left)?;
        let right_val = self.generate_expression(right)?;
        unsafe {
            let concat = self.get_or_create_concat();
            let mut args = [left_val, right_val];
            let result = LLVMBuildCall2(self.builder,
                LLVMGlobalGetValueType(concat),
                concat,
                args.as_mut_ptr(),
                2,
                c"concat".as_ptr());
            Ok(result)
        }
    }

    fn generate_match_expr(&mut self, value: &ASTNode, patterns: &[ASTNode], bodies: &[ASTNode]) -> Result<LLVMValueRef, String> {
        unsafe {
            let result_alloca = LLVMBuildAlloca(self.builder, self.i64_type(), c"match_result".as_ptr());
            let val = self.generate_expression(value)?;

            let end_block = self.create_block("match_end");
            let num_arms = patterns.len().min(bodies.len());

            for i in 0..num_arms {
                let next_block = if i < num_arms - 1 {
                    Some(self.create_block(&format!("match_arm_{}", i)))
                } else {
                    None
                };

                let pattern = &patterns[i];
                let body = &bodies[i];

                // Generate condition: compare value to pattern
                let pat_val = self.generate_expression(pattern)?;
                let zero = LLVMConstInt(self.i64_type(), 0, 0);
                let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntEQ, val, pat_val, c"match_cmp".as_ptr());

                let arm_block = self.create_block(&format!("match_arm_body_{}", i));

                if let Some(next) = next_block {
                    LLVMBuildCondBr(self.builder, cmp, arm_block, next);
                    LLVMPositionBuilderAtEnd(self.builder, next);
                } else {
                    // Last arm (default)
                    LLVMBuildCondBr(self.builder, cmp, arm_block, end_block);
                }

                LLVMPositionBuilderAtEnd(self.builder, arm_block);
                let body_val = self.generate_expression(body)?;
                LLVMBuildStore(self.builder, body_val, result_alloca);
                LLVMBuildBr(self.builder, end_block);
            }

            LLVMPositionBuilderAtEnd(self.builder, end_block);
            let result = LLVMBuildLoad2(self.builder, self.i64_type(), result_alloca, c"match_load".as_ptr());
            Ok(result)
        }
    }

    fn generate_field_access(&mut self, object: &ASTNode, field: &str) -> Result<LLVMValueRef, String> {
        unsafe {
            let mut obj_ptr = self.generate_expression(object)?;

            // Try to find the field index by looking up which struct type contains this field
            for (struct_name, fields) in &self.struct_fields {
                if let Some(field_index) = fields.iter().position(|f| f == field) {
                    if let Some(st) = self.struct_types.get(struct_name) {
                        let zero = LLVMConstInt(self.i32_type(), 0, 0);
                        let idx = LLVMConstInt(self.i32_type(), field_index as u64, 0);
                        let indices = [zero, idx];
                        let ptr = LLVMBuildInBoundsGEP2(self.builder, *st, obj_ptr,
                            indices.as_mut_ptr(), 2, c"field_ptr".as_ptr());
                        return Ok(LLVMBuildLoad2(self.builder, self.i64_type(), ptr, c"field_load".as_ptr()));
                    }
                }
            }

            // Fallback: return 0
            Ok(LLVMConstInt(self.i64_type(), 0, 0))
        }
    }

    fn generate_address_of(&mut self, operand: &ASTNode) -> Result<LLVMValueRef, String> {
        if let NodeData::Identifier { name } = &operand.data {
            if let Some(alloca) = self.named_values.get(name) {
                return Ok(*alloca);
            }
        }
        unsafe {
            // Fallback: alloca, store operand, return pointer
            let val = self.generate_expression(operand)?;
            let alloca = LLVMBuildAlloca(self.builder, self.i64_type(), c"addr".as_ptr());
            LLVMBuildStore(self.builder, val, alloca);
            Ok(alloca)
        }
    }

    fn generate_dereference(&mut self, operand: &ASTNode) -> Result<LLVMValueRef, String> {
        let ptr = self.generate_expression(operand)?;
        unsafe {
            Ok(LLVMBuildLoad2(self.builder, self.i64_type(), ptr, c"deref".as_ptr()))
        }
    }

    fn generate_binary_expr(&mut self, operator: &TokenType, left: &ASTNode, right: &ASTNode) -> Result<LLVMValueRef, String> {
        let lhs = self.generate_expression(left)?;
        let rhs = self.generate_expression(right)?;

        unsafe {
            match operator {
                TokenType::Plus => {
                    Ok(LLVMBuildAdd(self.builder, lhs, rhs, c"add".as_ptr()))
                }
                TokenType::Minus => {
                    Ok(LLVMBuildSub(self.builder, lhs, rhs, c"sub".as_ptr()))
                }
                TokenType::Star => {
                    Ok(LLVMBuildMul(self.builder, lhs, rhs, c"mul".as_ptr()))
                }
                TokenType::Slash => {
                    Ok(LLVMBuildSDiv(self.builder, lhs, rhs, c"div".as_ptr()))
                }
                TokenType::Percent => {
                    Ok(LLVMBuildSRem(self.builder, lhs, rhs, c"rem".as_ptr()))
                }
                TokenType::Eq => {
                    let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntEQ, lhs, rhs, c"eq".as_ptr());
                    Ok(LLVMBuildIntCast2(self.builder, cmp, self.i64_type(), 0, c"eq_int".as_ptr()))
                }
                TokenType::Neq => {
                    let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntNE, lhs, rhs, c"ne".as_ptr());
                    Ok(LLVMBuildIntCast2(self.builder, cmp, self.i64_type(), 0, c"ne_int".as_ptr()))
                }
                TokenType::Lt => {
                    let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntSLT, lhs, rhs, c"lt".as_ptr());
                    Ok(LLVMBuildIntCast2(self.builder, cmp, self.i64_type(), 0, c"lt_int".as_ptr()))
                }
                TokenType::Le => {
                    let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntSLE, lhs, rhs, c"le".as_ptr());
                    Ok(LLVMBuildIntCast2(self.builder, cmp, self.i64_type(), 0, c"le_int".as_ptr()))
                }
                TokenType::Gt => {
                    let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntSGT, lhs, rhs, c"gt".as_ptr());
                    Ok(LLVMBuildIntCast2(self.builder, cmp, self.i64_type(), 0, c"gt_int".as_ptr()))
                }
                TokenType::Ge => {
                    let cmp = LLVMBuildICmp(self.builder, LLVMIntPredicate::LLVMIntSGE, lhs, rhs, c"ge".as_ptr());
                    Ok(LLVMBuildIntCast2(self.builder, cmp, self.i64_type(), 0, c"ge_int".as_ptr()))
                }
                _ => Ok(lhs),
            }
        }
    }

    fn generate_call_expr(&mut self, name: &str, args: &[ASTNode]) -> Result<LLVMValueRef, String> {
        unsafe {
            let c_name = CString::new(name).unwrap();
            let callee = LLVMGetNamedFunction(self.module, c_name.as_ptr());
            if callee.is_null() {
                return Err(format!("Unknown function '{}'", name));
            }

            let mut arg_values = Vec::new();
            for arg in args {
                let val = self.generate_expression(arg)?;
                arg_values.push(val);
            }

            let result = LLVMBuildCall2(
                self.builder,
                LLVMGlobalGetValueType(callee),
                callee,
                arg_values.as_mut_ptr(),
                arg_values.len() as u32,
                c"call".as_ptr(),
            );
            Ok(result)
        }
    }

    /// Print the generated LLVM IR to stderr.
    pub fn print_ir(&self) {
        unsafe {
            LLVMDumpModule(self.module);
        }
    }

    /// Write LLVM bitcode to a file.
    pub fn write_bitcode(&self, filename: &str) -> Result<(), String> {
        unsafe {
            let c_filename = CString::new(filename).unwrap();
            let result = LLVMWriteBitcodeToFile(self.module, c_filename.as_ptr());
            if result != 0 {
                Err(format!("Failed to write bitcode to '{}'", filename))
            } else {
                Ok(())
            }
        }
    }

    /// Emit a native object file (.o).
    pub fn emit_object(&self, filename: &str) -> Result<(), String> {
        unsafe {
            let triple = LLVMGetDefaultTargetTriple();
            LLVMSetTarget(self.module, triple);

            let mut target: LLVMTargetRef = ptr::null_mut();
            let mut error: *mut c_char = ptr::null_mut();
            if LLVMGetTargetFromTriple(triple, &mut target, &mut error) != 0 {
                let err_str = CStr::from_ptr(error).to_string_lossy().into_owned();
                LLVMDisposeMessage(error);
                return Err(format!("Failed to get target: {}", err_str));
            }

            let machine = LLVMCreateTargetMachine(
                target,
                triple,
                c"generic".as_ptr(),
                c"".as_ptr(),
                LLVMCodeGenOptLevel::LLVMCodeGenLevelDefault,
                LLVMRelocMode::LLVMRelocDefault,
                LLVMCodeModel::LLVMCodeModelDefault,
            );

            let c_filename = CString::new(filename).unwrap();
            LLVMTargetMachineEmitToFile(
                machine,
                self.module,
                c_filename.as_ptr() as *mut c_char,
                LLVMCodeGenFileType::LLVMObjectFile,
                &mut error,
            );

            if !error.is_null() {
                let err_str = CStr::from_ptr(error).to_string_lossy().into_owned();
                LLVMDisposeMessage(error);
                LLVMDisposeTargetMachine(machine);
                return Err(format!("Failed to emit object: {}", err_str));
            }

            LLVMDisposeTargetMachine(machine);
            Ok(())
        }
    }

    /// Emit assembly text (.s).
    pub fn emit_assembly(&self, filename: &str) -> Result<(), String> {
        unsafe {
            let triple = LLVMGetDefaultTargetTriple();
            LLVMSetTarget(self.module, triple);

            let mut target: LLVMTargetRef = ptr::null_mut();
            let mut error: *mut c_char = ptr::null_mut();
            if LLVMGetTargetFromTriple(triple, &mut target, &mut error) != 0 {
                let err_str = CStr::from_ptr(error).to_string_lossy().into_owned();
                LLVMDisposeMessage(error);
                return Err(format!("Failed to get target: {}", err_str));
            }

            let machine = LLVMTargetMachineCreate(
                target,
                triple,
                c"generic".as_ptr(),
                c"".as_ptr(),
                LLVMCodeGenOptLevel::LLVMCodeGenLevelDefault,
                LLVMRelocMode::LLVMRelocPIC,
                LLVMCodeModel::LLVMCodeModelDefault,
            );

            let c_filename = CString::new(filename).unwrap();
            LLVMTargetMachineEmitToFile(
                machine,
                self.module,
                c_filename.as_ptr() as *mut c_char,
                LLVMCodeGenFileType::LLVMAssemblyFile,
                &mut error,
            );

            if !error.is_null() {
                let err_str = CStr::from_ptr(error).to_string_lossy().into_owned();
                LLVMDisposeMessage(error);
                LLVMDisposeTargetMachine(machine);
                return Err(format!("Failed to emit assembly: {}", err_str));
            }

            LLVMDisposeTargetMachine(machine);
            Ok(())
        }
    }

    /// Link the module with gcc to produce a final executable.
    pub fn link_object(obj_file: &str, output_file: &str) -> Result<(), String> {
        let status = std::process::Command::new("gcc")
            .arg(obj_file)
            .arg("-o")
            .arg(output_file)
            .status()
            .map_err(|e| format!("Failed to run gcc: {}", e))?;
        if !status.success() {
            return Err("gcc linking failed".to_string());
        }
        Ok(())
    }
}

impl Drop for CodeGen {
    fn drop(&mut self) {
        unsafe {
            LLVMDisposeBuilder(self.builder);
            LLVMDisposeModule(self.module);
            LLVMContextDispose(self.context);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::Lexer;
    use crate::parser::Parser;

    fn parse(source: &str) -> Result<ASTNode, String> {
        let tokens = {
            let mut lexer = Lexer::new(source);
            lexer.tokenize()
        };
        let mut parser = Parser::new(tokens);
        parser.parse()
    }

    fn generate_ok(source: &str) -> bool {
        match parse(source) {
            Ok(program) => {
                let mut codegen = CodeGen::new("test_module");
                codegen.generate(&program).is_ok()
            }
            Err(_) => false,
        }
    }

    #[test]
    fn test_codegen_empty_program() {
        assert!(generate_ok(""));
    }

    #[test]
    fn test_codegen_var_decl() {
        assert!(generate_ok("let x = 42"));
    }

    #[test]
    fn test_codegen_arithmetic() {
        assert!(generate_ok("let x = 3 + 4 * 5"));
    }

    #[test]
    fn test_codegen_comparison() {
        assert!(generate_ok("let x = 10 > 5"));
    }

    #[test]
    fn test_codegen_equality() {
        assert!(generate_ok("let x = 1 == 2"));
    }

    #[test]
    fn test_codegen_fn_decl() {
        assert!(generate_ok("fn main() { print(42) }"));
    }

    #[test]
    fn test_codegen_fn_with_params() {
        assert!(generate_ok("fn add(a, b) { return a + b }"));
    }

    #[test]
    fn test_codegen_if_stmt() {
        assert!(generate_ok("fn main() { if 1 { print(1) } }"));
    }

    #[test]
    fn test_codegen_if_else() {
        assert!(generate_ok("fn main() { if 1 { print(1) } else { print(2) } }"));
    }

    #[test]
    fn test_codegen_while_loop() {
        assert!(generate_ok("fn main() { while (1) { print(1) } }"));
    }

    #[test]
    fn test_codegen_for_loop() {
        assert!(generate_ok("fn main() { for let i = 0; i < 10; i = i + 1 { print(i) } }"));
    }

    #[test]
    fn test_codegen_break_continue() {
        assert!(generate_ok("fn main() { while (1) { break } }"));
        assert!(generate_ok("fn main() { while (1) { continue } }"));
    }

    #[test]
    fn test_codegen_assignment() {
        assert!(generate_ok("fn main() { let x = 5; x = x + 1 }"));
    }

    #[test]
    fn test_codegen_unary_minus() {
        assert!(generate_ok("let x = -5"));
    }

    #[test]
    fn test_codegen_logical_not() {
        assert!(generate_ok("let x = !true"));
    }

    #[test]
    fn test_codegen_array() {
        assert!(generate_ok("fn main() { let arr = [1, 2, 3] }"));
    }

    #[test]
    fn test_codegen_index_expr() {
        assert!(generate_ok("fn main() { let arr = [1, 2, 3]; let v = arr[0] }"));
    }

    #[test]
    fn test_codegen_string_concat() {
        assert!(generate_ok("fn main() { let s = \"hello\" + \" world\" }"));
    }

    #[test]
    fn test_codegen_match_expr() {
        assert!(generate_ok("fn main() { let x = 1; let y = match x { 1 => { 10 } 2 => { 20 } } }"));
    }

    #[test]
    fn test_codegen_extern_fn() {
        assert!(generate_ok("extern fn puts(s) -> int from \"libc.so.6\""));
    }

    #[test]
    fn test_codegen_try_catch() {
        assert!(generate_ok("fn main() { try { print(1) } catch { print(2) } }"));
    }

    #[test]
    fn test_codegen_block() {
        assert!(generate_ok("fn main() { { let x = 1; print(x) } }"));
    }

    #[test]
    fn test_codegen_return() {
        assert!(generate_ok("fn f() { return 42 }"));
    }

    #[test]
    fn test_codegen_nested_blocks() {
        assert!(generate_ok("fn main() { if 1 { while (1) { print(1) } } }"));
    }

    #[test]
    fn test_codegen_full_program() {
        let source = r#"
            fn add(a, b) {
                return a + b
            }
            fn main() {
                let x = 10
                let y = 20
                let z = add(x, y)
                print(z)
            }
        "#;
        assert!(generate_ok(source));
    }
}

/// Convenience: compile an AST to LLVM IR and write to a file (or print if None).
pub fn compile_to_ir(program: &ASTNode, output_file: Option<&str>) -> Result<(), String> {
    let mut codegen = CodeGen::new("hunnu_module");
    codegen.generate(program)?;

    if let Some(filename) = output_file {
        codegen.write_bitcode(filename)?
    } else {
        codegen.print_ir();
    }

    Ok(())
}

/// Convenience: compile an AST to a native object file.
pub fn compile_to_object(program: &ASTNode, output_file: &str) -> Result<(), String> {
    let mut codegen = CodeGen::new("hunnu_module");
    codegen.generate(program)?;
    codegen.emit_object(output_file)
}

/// Convenience: compile an AST to assembly text.
pub fn compile_to_assembly(program: &ASTNode, output_file: &str) -> Result<(), String> {
    let mut codegen = CodeGen::new("hunnu_module");
    codegen.generate(program)?;
    codegen.emit_assembly(output_file)
}

/// Convenience: compile an AST to an executable via gcc.
pub fn compile_to_exe(program: &ASTNode, output_file: &str) -> Result<(), String> {
    let obj_file = format!("{}.o", output_file);
    let mut codegen = CodeGen::new("hunnu_module");
    codegen.generate(program)?;
    codegen.emit_object(&obj_file)?;
    CodeGen::link_object(&obj_file, output_file)?;
    let _ = std::fs::remove_file(&obj_file);
    Ok(())
}
