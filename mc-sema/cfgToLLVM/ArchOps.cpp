#include "llvm/Module.h"
#include "llvm/BasicBlock.h"
#include "llvm/Type.h"
#include <string>
#include <iostream>
#include "../common/Defaults.h"
#include "TransExcn.h"
#include "win32cb.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/InlineAsm.h"
#include "raiseX86.h"
#include "ArchOps.h"
#include "../common/to_string.h"

using namespace std;
using namespace llvm;

extern llvm::PointerType *g_PRegStruct;

static bool added_callbacks = false;

static bool linuxAddTypesToModule(Module *M) {
    Module *mod = M;

    StructType *StructTy_struct_rlimit = mod->getTypeByName("struct.rlimit");
    if (!StructTy_struct_rlimit) {
        StructTy_struct_rlimit = StructType::create(mod->getContext(), "struct.rlimit");
    }
    std::vector<Type*>StructTy_struct_rlimit_fields;
    StructTy_struct_rlimit_fields.push_back(IntegerType::get(mod->getContext(), 32));
    StructTy_struct_rlimit_fields.push_back(IntegerType::get(mod->getContext(), 32));
    if (StructTy_struct_rlimit->isOpaque()) {
        StructTy_struct_rlimit->setBody(StructTy_struct_rlimit_fields, /*isPacked=*/false);
    }

    PointerType* PointerTy_3 = PointerType::get(StructTy_struct_rlimit, 0);

    std::vector<Type*>FuncTy_6_args;
    FuncTy_6_args.push_back(IntegerType::get(mod->getContext(), 32));
    FuncTy_6_args.push_back(PointerTy_3);
    FunctionType* FuncTy_6 = FunctionType::get(
            /*Result=*/IntegerType::get(mod->getContext(), 32),
            /*Params=*/FuncTy_6_args,
            /*isVarArg=*/false);

    Function* func_getrlimit = mod->getFunction("getrlimit");
    if (!func_getrlimit) {
        func_getrlimit = Function::Create(
                /*Type=*/FuncTy_6,
                /*Linkage=*/GlobalValue::ExternalLinkage,
                /*Name=*/"getrlimit", mod); // (external, no body)
        func_getrlimit->setCallingConv(CallingConv::C);
    }

    PointerType* PointerTy_8 = PointerType::get(IntegerType::get(mod->getContext(), 8), 0);
    std::vector<Type*>FuncTy_11_args;
    FuncTy_11_args.push_back(PointerTy_8);
    FuncTy_11_args.push_back(IntegerType::get(mod->getContext(), 32));
    FuncTy_11_args.push_back(IntegerType::get(mod->getContext(), 32));
    FuncTy_11_args.push_back(IntegerType::get(mod->getContext(), 32));
    FuncTy_11_args.push_back(IntegerType::get(mod->getContext(), 32));
    FuncTy_11_args.push_back(IntegerType::get(mod->getContext(), 32));
    FunctionType* FuncTy_11 = FunctionType::get(
            /*Result=*/PointerTy_8,
            /*Params=*/FuncTy_11_args,
            /*isVarArg=*/false);

    Function* func_mmap = mod->getFunction("mmap");
    if (!func_mmap) {
        func_mmap = Function::Create(
                /*Type=*/FuncTy_11,
                /*Linkage=*/GlobalValue::ExternalLinkage,
                /*Name=*/"mmap", mod); // (external, no body)
        func_mmap->setCallingConv(CallingConv::C);
    }

    std::vector<Type*>FuncTy_15_args;
    FuncTy_15_args.push_back(PointerTy_8);
    FuncTy_15_args.push_back(IntegerType::get(mod->getContext(), 32));
    FunctionType* FuncTy_15 = FunctionType::get(
            /*Result=*/IntegerType::get(mod->getContext(), 32),
            /*Params=*/FuncTy_15_args,
            /*isVarArg=*/false);
    Function* func_munmap = mod->getFunction("munmap");
    if (!func_munmap) {
        func_munmap = Function::Create(
                /*Type=*/FuncTy_15,
                /*Linkage=*/GlobalValue::ExternalLinkage,
                /*Name=*/"munmap", mod); // (external, no body)
        func_munmap->setCallingConv(CallingConv::C);
    }

    return true;
}

static Value *linuxGetStackSize(Module *M, BasicBlock *&driverBB) {

    linuxAddTypesToModule(M);

    Module *mod = M;

    BasicBlock *label_16 = driverBB;
    StructType *StructTy_struct_rlimit = mod->getTypeByName("struct.rlimit");
    PointerType* PointerTy_2 = PointerType::get(IntegerType::get(mod->getContext(), 64), 0);

    if(!StructTy_struct_rlimit ) {
        return NULL;
    }

    Function* func_getrlimit = mod->getFunction("getrlimit");

    if(!func_getrlimit) {
        return  NULL;
    }

    AllocaInst* ptr_rl = new AllocaInst(StructTy_struct_rlimit, "rl", label_16);
    CastInst* ptr_17 = new BitCastInst(ptr_rl, PointerTy_2, "", label_16);
    StoreInst* void_18 = new StoreInst(CONST_V<64>(label_16, 0), ptr_17, false, label_16);
    std::vector<Value*> int32_19_params;
    int32_19_params.push_back(CONST_V<32>(label_16, 3));
    int32_19_params.push_back(ptr_rl);
    CallInst* int32_19 = CallInst::Create(func_getrlimit, int32_19_params, "", label_16);
    int32_19->setCallingConv(CallingConv::C);
    int32_19->setTailCall(false);

    std::vector<Value*> ptr_20_indices;
    ptr_20_indices.push_back(CONST_V<32>(label_16, 0));
    ptr_20_indices.push_back(CONST_V<32>(label_16, 0));

    Instruction* ptr_20 = GetElementPtrInst::Create(ptr_rl, ptr_20_indices, "", label_16);
    LoadInst* int32_21 = new LoadInst(ptr_20, "", false, label_16);
    return int32_21;
}

static Value* linuxAllocateStack(Module *M, Value *stackSize, BasicBlock *&driverBB) {

    // call mmap(null, stackSize, ...) to allocate stack
    linuxAddTypesToModule(M);
    Module *mod = M;

    Function* func_mmap = mod->getFunction("mmap");

    if(!func_mmap) {
        return  NULL;
    }

    PointerType* PointerTy_8 = PointerType::get(IntegerType::get(mod->getContext(), 8), 0);
    ConstantPointerNull* null_ptr = ConstantPointerNull::get(PointerTy_8);

    std::vector<Value*> ptr_39_params;
    ptr_39_params.push_back(null_ptr);
    ptr_39_params.push_back(stackSize);
    ptr_39_params.push_back(CONST_V<32>(driverBB, 3));
    ptr_39_params.push_back(CONST_V<32>(driverBB, 0x20022));
    ptr_39_params.push_back(CONST_V<32>(driverBB, -1));
    ptr_39_params.push_back(CONST_V<32>(driverBB, 0));
    CallInst* ptr_39 = CallInst::Create(func_mmap, ptr_39_params, "", driverBB);
    ptr_39->setCallingConv(CallingConv::C);

    CastInst* stackPtrInt = new PtrToIntInst(ptr_39, IntegerType::get(mod->getContext(), 32), "", driverBB);

    return stackPtrInt;
}

static Value* win32GetStackSize(Module *M, BasicBlock *&driverBB) {
    Value *pTEB = win32GetTib(driverBB);
    Value *stackSize = win32GetStackSize(pTEB, driverBB);
    return stackSize;
}

static Value* win32AllocateStack(Module *M, Value *stackSize, BasicBlock *&driverBB) {
    Value *aStack = win32CallVirtualAlloc(stackSize, driverBB);
    return aStack;
}

Value* archGetStackSize(Module *M, BasicBlock *&driverBB) {
    const std::string &triple = M->getTargetTriple();
    Value *stackSize = NULL;

    if(triple == LINUX_TRIPLE) {
        stackSize = linuxGetStackSize(M, driverBB);
    } else if(triple == WINDOWS_TRIPLE) {
        stackSize = win32GetStackSize(M, driverBB);
    } else { 
        cout << "WARNING: Unknown architecture triple: " << triple << "\n";
        cout << "Assuming Win32 semantics\n";
        stackSize = win32GetStackSize(M, driverBB);
    }

    TASSERT(stackSize != NULL, "Could not allocate stack!");
    return stackSize;
}

Value* archAllocateStack(Module *M, Value *stackSize, BasicBlock *&driverBB) {
    // VirtualAlloc a stack buffer the same size as the current thread's
    // stack size
    
    const std::string &triple = M->getTargetTriple();
    Value *stackAlloc = NULL;

    if(triple == LINUX_TRIPLE) {
        stackAlloc = linuxAllocateStack(M, stackSize, driverBB);
    } else if(triple == WINDOWS_TRIPLE) {
        stackAlloc = win32AllocateStack(M, stackSize, driverBB);
    } else { 
        cout << "WARNING: Unknown architecture triple: " << triple << "\n";
        cout << "Assuming Win32 semantics\n";
        stackAlloc = win32AllocateStack(M, stackSize, driverBB);
    }

    TASSERT(stackAlloc != NULL, "Could not allocate stack!");
    return stackAlloc;
}

static Value *linuxFreeStack(Module *M, Value *stackAlloc, BasicBlock *&driverBB) {
    Module *mod = M;
    linuxAddTypesToModule(M);

    Value *stack_size = linuxGetStackSize(M, driverBB);

    PointerType* PointerTy_8 = PointerType::get(IntegerType::get(mod->getContext(), 8), 0);

    Function* func_munmap = mod->getFunction("munmap");

    if(!func_munmap) {
        return  NULL;
    }

    Value *stackPtr = new IntToPtrInst(stackAlloc, PointerTy_8, "", driverBB);

    std::vector<Value*> int32_43_params;
    int32_43_params.push_back(stackPtr);
    int32_43_params.push_back(stack_size);
    CallInst* int32_43 = CallInst::Create(func_munmap, int32_43_params, "", driverBB);
    int32_43->setCallingConv(CallingConv::C);
    int32_43->setTailCall(true);

    return int32_43;
}

static Value *win32FreeStack(Value *stackAlloc, BasicBlock *&driverBB) {
    Value *freeIt = win32CallVirtualFree(stackAlloc, driverBB);
    return freeIt;
}

Value *archFreeStack(Module *M, Value *stackAlloc, BasicBlock *&driverBB) {

    const std::string &triple = M->getTargetTriple();
    Value *stackFree = NULL;

    if(triple == LINUX_TRIPLE) {
        stackFree = linuxFreeStack(M, stackAlloc, driverBB);
    } else if(triple == WINDOWS_TRIPLE) {
        // free our allocated stack
        stackFree = win32FreeStack(stackAlloc, driverBB);
    } else { 
        stackFree = win32FreeStack(stackAlloc, driverBB);
    }

    TASSERT(stackFree != NULL, "Could not free stack!");
    return stackFree;
}

Module* archAddCallbacksToModule(Module *M) {
    const std::string &triple = M->getTargetTriple();
    if(triple == LINUX_TRIPLE) {
        return M;
    } else if(triple == WINDOWS_TRIPLE) {
        return addWin32CallbacksToModule(M);
    } else { 
        return addWin32CallbacksToModule(M);
    }
}

// GEP to get a reg from pro_int_call return
#define READ_REG_OPAQUE(varname, reg, opaque) do {  \
        int   reg_off = mapStrToGEPOff(reg);        \
        Value *reg_GEPV[] = {                       \
          CONST_V<32>(driver_block, 0),             \
          CONST_V<32>(driver_block, reg_off)        \
        };                                          \
        varname = GetElementPtrInst::Create(        \
                 opaque,                            \
                 reg_GEPV,                          \
                 "",                                \
                 driver_block);                     \
    } while(0);                                     


static Function *getCallbackPrologueInternal(Module *M) {

    // should be created by archAddCallbacksToModule()
    Function *F = M->getFunction("callback_adapter_prologue_internal");
    TASSERT(F != NULL, "Want to use callback_adapter_prologue_internal, but its not defined");

    return F;
}

static Function *getCallbackEpilogue(Module *M) {
    
    Function *F = M->getFunction("callback_adapter_epilogue");
    TASSERT(F != NULL, "Want to use callback_adapter_epilogue but its not defined");    
    return F;
}

// type of internal callback handler
static FunctionType    *GetCBInternalFnTy(Module *M) {

    Type *int32ty = Type::getInt32Ty(M->getContext());
    Type *int32PtrTy = PointerType::get(int32ty, 0);
    Type *voidstar = PointerType::get(
            Type::getInt8Ty(M->getContext()), 
            0);

    std::vector<Type*>	cb_internal_args;
    cb_internal_args.push_back(int32PtrTy);     // int *retv
    cb_internal_args.push_back(voidstar);       // void* esp
    cb_internal_args.push_back(int32ty);        // int ebp
    FunctionType    *FuncTy_CBInternal = 
        FunctionType::get(int32ty,
                cb_internal_args,
                false);

    return FuncTy_CBInternal;
}

static Function *win32MakeCallbackInternal(Module *M, VA local_target) {
    // no driver, make one
    std::string			fname = "callback_sub_"+
        to_string<VA>(local_target, std::hex)+
        "_internal";
    FunctionType    *callbackTy = GetCBInternalFnTy(M);
    Type *voidstar = PointerType::get(
            Type::getInt8Ty(M->getContext()), 
            0);
    Function *Fpro_internal = getCallbackPrologueInternal(M);
    Function *Fepi = getCallbackEpilogue(M);
    Type *int32ty = Type::getInt32Ty(M->getContext());


    Function *F = dynamic_cast<Function*>(M->getOrInsertFunction(fname, callbackTy));
    TASSERT( F != NULL, "" );
    F->setLinkage(GlobalValue::InternalLinkage);
    F->setCallingConv(CallingConv::C);
    //F->addFnAttr(Attributes::Naked);

    // get reference to function arguments
    Function::arg_iterator args = F->arg_begin();
    Value* arg_ESPDIFF = args++;
    arg_ESPDIFF->setName("arg_ESPDIFF");
    Value* arg_ESP = args++;
    arg_ESP->setName("arg_ESP");
    Value* arg_EBP = args++;
    arg_EBP->setName("arg_EBP");

    // add code to driver
    BasicBlock *driver_block = BasicBlock::Create(
        F->getContext(), "", F);

    // allocate pointer to hold new stack
    Instruction *new_stack = new AllocaInst(voidstar, "", driver_block);

    // get ready to call callback_adapter_internal
    std::vector<Value*>	pro_args;
    pro_args.push_back(arg_EBP);    // int32
    pro_args.push_back(arg_ESP);    // void*
    pro_args.push_back(new_stack);  // void**

    // call callback_adapter_internal
	CallInst *pro_int_call = CallInst::Create(Fpro_internal, pro_args, "", driver_block);
	pro_int_call->setCallingConv(CallingConv::X86_StdCall);


    // cast to g_pRegState?


    // old_esp = rs->ESP
    // GEP to get ESP from pro_int_call return
    Value *rs_esp;
    READ_REG_OPAQUE(rs_esp, "ESP", pro_int_call);

    LoadInst* orig_ESP = new LoadInst(rs_esp, "", false, driver_block);
    

    // call original functions
    std::string			realfn = "sub_"+to_string<VA>(local_target, std::hex);
    Function        *Freal = M->getFunction(realfn);
    TASSERT(Freal != NULL, "Cannot find original function: "+realfn);

    Value *gpreg_struct = CastInst::CreatePointerCast(pro_int_call, 
            g_PRegStruct, 
            "", 
            driver_block);

    std::vector<Value*>	realArgs;
    realArgs.push_back(gpreg_struct);

	CallInst *real_call = CallInst::Create(Freal, realArgs, "", driver_block);
    real_call->setCallingConv(CallingConv::X86_StdCall);
    real_call->setIsNoInline();

    // retv = rs->EAX;
    Value *rs_eax;
    READ_REG_OPAQUE(rs_eax, "EAX", gpreg_struct);
    LoadInst *eax_val = new LoadInst(rs_eax, "", false, driver_block);


    // *esp_diff = orig_esp - rs->ESP;
    Value *rs_esp_new;
    READ_REG_OPAQUE(rs_esp_new, "ESP", gpreg_struct);
    LoadInst* new_esp_val = new LoadInst(rs_esp_new, "", false, driver_block);
    Value *esp_diff = BinaryOperator::Create(
            Instruction::Sub, 
            orig_ESP, 
            new_esp_val, 
            "", 
            driver_block);

    StoreInst *save_retv = new StoreInst(esp_diff, arg_ESPDIFF, false, driver_block);
    // call epilogue
    Value *ns_deref = new LoadInst(new_stack, "", driver_block);
    std::vector<Value*>	epi_args;
    epi_args.push_back(gpreg_struct);
    epi_args.push_back(ns_deref);
    
	CallInst *epi_call = CallInst::Create(Fepi, epi_args, "", driver_block);
	epi_call->setCallingConv(CallingConv::X86_StdCall);

    // return value = eax
    llvm::ReturnInst::Create(F->getContext(), eax_val, driver_block);

}

static Function *win32MakeCallbackStub(Module *M, VA local_target) {

    //lookup local function in the module
    std::string			fname = "callback_sub_"+to_string<VA>(local_target, std::hex);

    std::string			call_tgt_name = "callback_sub_"+
        to_string<VA>(local_target, std::hex)+
        "_internal";

    Function        *call_tgt = M->getFunction(call_tgt_name);
    TASSERT(call_tgt != NULL, "Cannot find call target function in callback stub");

    // no driver, make one
    FunctionType    *callbackTy = 
        FunctionType::get( Type::getInt32Ty(M->getContext()), false );

    Function *F = dynamic_cast<Function*>(M->getOrInsertFunction(fname, callbackTy));
    TASSERT( F != NULL, "Cannot create callback stub" );
    F->setLinkage(GlobalValue::InternalLinkage);
    F->addFnAttr(Attributes::Naked);

    // add code to driver
    BasicBlock *driver_block = BasicBlock::Create(
            F->getContext(), "", F);
    
    std::vector<Type*>	cb_args;
    cb_args.push_back(PointerType::get(GetCBInternalFnTy(M), 0));
    FunctionType    *FuncTy_CBStub = FunctionType::get(
                // int32 return since the function needs this prototype
                Type::getInt32Ty(M->getContext()),
                cb_args,
                false);

    InlineAsm* func_body = InlineAsm::get(FuncTy_CBStub, 
            "movl	%esp, %eax\n"
            "subl	$$4, %esp\n"
            "pushl	%ebp\n"
            "pushl	%eax\n"
            "subl	$$4, %eax\n"
            "pushl	%eax\n"
            "calll	$1\n"
            "addl   $$12, %esp\n"
            "movl	(%esp), %ecx\n"
            "addl	$$4, %esp\n"
            "subl	%ecx, %esp\n"
            "jmpl	*(%esp,%ecx)\n",
            "={eax},*imr,~{eax},~{ecx},~{dirflag},~{fpsr},~{flags}",
            true);

    std::vector<Value*> asm_args;
    asm_args.push_back(call_tgt);

    CallInst* do_asm = CallInst::Create(func_body, asm_args, "", driver_block);
    do_asm->setTailCall(true);

    // will never get here
    llvm::ReturnInst::Create(F->getContext(), do_asm, driver_block);

    return F;
}

static llvm::Value *win32MakeCallbackForLocalFunction(Module *M, VA local_target) {
    if(!added_callbacks) {
        std::cout << __FUNCTION__ << ": Adding Callbacks to Module!" << std::endl;
        archAddCallbacksToModule(M);
        added_callbacks = true;
    }


    //lookup local function in the module
    std::string			fname = "callback_sub_"+to_string<VA>(local_target, std::hex);
    Function        *F = M->getFunction(fname);

    // we already created a callback for this, re-use it.
    if(F != NULL) {
        return F;
    }

    Function *F_int = win32MakeCallbackInternal(M, local_target);
    Function *F_stub = win32MakeCallbackStub(M, local_target);


    return F_stub;
}

static llvm::Value *linuxMakeCallbackForLocalFunction(Module *M , VA local_target) {
    std::string			call_tgt_name = "sub_" + to_string<VA>(local_target, std::hex);

    Function        *call_tgt = M->getFunction(call_tgt_name);
    TASSERT(call_tgt != NULL, "Cannot find call target function in callback stub");

    std::cout << "!!!WARNING WARNING WARNING!!!" << "\n";
    std::cout << "Assuming all callbacks are to translated code!!!\n";
    std::cout << "!!!WARNING WARNING WARNING!!!" << std::endl;

    return call_tgt;
}

llvm::Value *archMakeCallbackForLocalFunction(Module *M, VA local_target)
{

    const std::string &triple = M->getTargetTriple();
    if(triple == LINUX_TRIPLE) {
        return linuxMakeCallbackForLocalFunction(M, local_target);
    } else if(triple == WINDOWS_TRIPLE) {
        return win32MakeCallbackForLocalFunction(M, local_target);
    } else { 
        return win32MakeCallbackForLocalFunction(M, local_target);
    }

}