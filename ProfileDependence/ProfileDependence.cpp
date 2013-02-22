


#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Type.h"
#include "llvm/Instructions.h"
#include "llvm/Instruction.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IRBuilder.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Constants.h"
 #include "llvm/GlobalVariable.h"
 #include "llvm/Function.h"
#include "llvm/LLVMContext.h"
#include "llvm/DebugInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

using namespace llvm;
#include <iostream>
#include <sstream>
using namespace std;

Constant *hookFunc;
Constant *printFunc;
Function * hook;
Function * func_print;
StringRef * loopCond= new StringRef ("for.cond");	
StringRef * loopEnd= new StringRef ("for.end");
StringRef * loopBody = new StringRef ("for.body");
StringRef * main = new StringRef ("main");
StringRef * main1 = new StringRef ("MAIN__");  //needed for f2c converted c files
int loop_count = 0;
//AllocaInst* alloca_count; 
PointerType* PointerTy_1;
Type * type;
Type * integerType;
Type * voidType;
PointerType* array;
GetElementPtrInst * inst;
Value *FBloc;
Value *IterationCount;
AllocaInst* alloca_int_ptr;
AllocaInst* alloca_void_ptr;

namespace{
    struct ProfileDependence : public ModulePass{
        static char ID;   
        ProfileDependence() : ModulePass(ID) {}

        virtual bool runOnModule(Module &M)
        {
		for(Module::iterator func = M.begin(), y = M.end(); func!= y; ++func)
        	{
			if (func->isDeclaration())
          			continue;

			//get the required pass results
			LoopInfo &LI = getAnalysis<LoopInfo>(* func);
			ScalarEvolution &SE = getAnalysis<ScalarEvolution>(* func);

			ConstantInt* const_int32_4 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("1"), 10));
			ConstantInt* const_int32_5 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("0"), 10));

			for(Function::iterator F = func->begin(), E = func->end(); F!= E; ++F)
	        	{

				//if it is the first basic block of the function, we add a void pointer
				//that hold the temporary value of all accesses
				if(F == func->begin())
				{
					
					voidType= PointerType::get(IntegerType::getInt8Ty(getGlobalContext())
							, 0);	
					alloca_void_ptr = new AllocaInst(voidType, "void_pointer");
					for (BasicBlock::iterator j = F->begin(), e = F->end(); j != e; ++j ) {
				
						if(j == F->begin())
						{
							F->getInstList().insert((Instruction*)j, alloca_void_ptr);
							break;
						}
					}
				}

				//finding a block with name "for.cond" means there is a loop in the function
				if (!((F->getName().find(*loopCond,0))))
				{
		
						++loop_count;
						Loop * L = LI.getLoopFor(F);
						
						BasicBlock * BB1;
						if(L)
							//check if the loop is a candidate loop
							BB1 = L->getUniqueExitBlock();
							
						std::vector<Loop*> LoopBlocks = L->getSubLoops();
							
						
						if(L && (BB1 && LoopBlocks.empty()) && 
							!(isa<SCEVCouldNotCompute>(SE.getBackedgeTakenCount(L))))
						{
							//we got a candidate loop
							std::string count1;
							ostringstream convert; 
							convert << loop_count; 
							count1 = convert.str();
							ConstantInt* lcount = ConstantInt::get(getGlobalContext(), APInt(32, StringRef(count1), 										10));
							FBloc = new GlobalVariable(M,
		                        	                IntegerType::get(getGlobalContext(), 32),
	               		        	                false,
	               		        	                GlobalValue::ExternalLinkage,
	               		        	                lcount,
	               		        	                "loopCount");
							ConstantInt* icount = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("0"), 									10));
							IterationCount = new GlobalVariable(M,
	                        	                IntegerType::get(getGlobalContext(), 32),
	               		        	                false,
	               		        	                GlobalValue::ExternalLinkage,
	              		        	                icount,
	               		        	                "iterationCount");
							
							BasicBlock * BB = L->getUniqueExitBlock();
							if(BB)
							{
								for (BasicBlock::iterator j = (*BB).begin(), e = (*BB).end(); j != e; ++j ) {
									if(j == (*BB).begin())
									{
										//make a call to analyse_and_write()
										Constant *func = M.getOrInsertFunction("analyse_and_write",
											IntegerType::getInt32Ty	
											(getGlobalContext()), IntegerType::getInt32Ty
											(getGlobalContext()),NULL);
										Function *func1;
										func1= cast<Function>(func);
										LoadInst* loop = new LoadInst(FBloc, "", false);
										CallInst* call = CallInst::Create(func1, loop, "");
										
										//make count = 0;
										StoreInst* store_count = new StoreInst(const_int32_5, 													IterationCount,	false);
		
										(*BB).getInstList().insert((Instruction*)j, loop);
										(*BB).getInstList().insert((Instruction*)j, call);
										(*BB).getInstList().insert((Instruction*)j, store_count);
										
									}
								}//end of for
							}//if(BB)
						}//end of if candidate loop						
				}//end of if (!((F->getName().find(*loopCond,0)))) 
				
				Loop * L = LI.getLoopFor(F);
				
				if(L)
				{
					//check if it's a candidate loop
					BasicBlock * BB1 = L->getUniqueExitBlock();
					std::vector<Loop*> LoopBlocks = L->getSubLoops();
						
					if((BB1 && LoopBlocks.empty()) && !(isa<SCEVCouldNotCompute>(SE.getBackedgeTakenCount(L))))
					{
						//it's a candidate loop
						if (!(F->getName().find(*loopBody,0)))
						{
							LoadInst* int32_16 = new LoadInst(IterationCount, "", false);
  							BinaryOperator* int32_17 = BinaryOperator::Create(Instruction::Add, int32_16, 									const_int32_4, "");
	  						StoreInst* void_18 = new StoreInst(int32_17, IterationCount, false);
				
							for (BasicBlock::iterator j = F->begin(), e = F->end(); j != e; ++j ) {

								//if first instruction of loop body, add the following instructions
								//load count;
								//inc count;
								//store count
								if(j == F->begin())
								{
									
									F->getInstList().insert(j, int32_16);//errs() << "Got problem \n";
									F->getInstList().insert(j, int32_17);
									F->getInstList().insert(j, void_18);
									//break;
								}//end of if
								if(isa<LoadInst>(&(*j)))
								{
									//errs() << "Load inst \n";
									LoadInst *CI = dyn_cast<LoadInst>(j);
									type = CI->getOperand(0)->getType();
									//errs() << CI->getType() << " Load\n";

									if(type->isPointerTy())
									{

										CastInst* cast = new BitCastInst(CI->getPointerOperand(), 												voidType, "");
										StoreInst* store_void = new StoreInst(cast, alloca_void_ptr, 													false);
										hookFunc = M.getOrInsertFunction("print_load_pointer", 												IntegerType::getInt32Ty
											(getGlobalContext()), voidType,IntegerType::getInt32Ty
											(getGlobalContext()),IntegerType::getInt32Ty
											(getGlobalContext()), NULL);
										if(hookFunc)
											hook= dyn_cast<Function>(hookFunc);
										//load value of count
										LoadInst* load_count = new LoadInst(IterationCount, "", false);
										LoadInst* load_ptr = new LoadInst(alloca_void_ptr, "", false);
										LoadInst* load_loop = new LoadInst(FBloc, "", false);
	  									std::vector<Value*> int32_15_params;
	  									int32_15_params.push_back(load_ptr);
	  									int32_15_params.push_back(load_count);
	  									int32_15_params.push_back(load_loop);
  										//int32_15_params.push_back(inst);
										CallInst* call_print = CallInst::Create(hook, int32_15_params, "");
										/*insert the instructions*/
										//F->getInstList().insert((Instruction*)CI, alloca_ptr);
										F->getInstList().insert((Instruction*)CI, cast);
										F->getInstList().insert((Instruction*)CI, store_void);
										F->getInstList().insert((Instruction*)CI, load_ptr);
										F->getInstList().insert((Instruction*)CI, load_count);
										F->getInstList().insert((Instruction*)CI, load_loop);
										F->getInstList().insert((Instruction*)CI, call_print);
									}//if pointertype
			     					}//end of "if load instruction"
								if(isa<StoreInst>(&(*j)))
								{
									//errs() << "Store inst \n";
									StoreInst *CI = dyn_cast<StoreInst>(j);
									type = CI->getOperand(1) ->getType();
									//errs()<<CI->getOperand(0)->getType()<<"Store \n";
									if(type->isPointerTy())
									{

										CastInst* cast = new BitCastInst(CI->getPointerOperand(), 												voidType, "");
										StoreInst* store_void = new StoreInst(cast, alloca_void_ptr, 												false);
										hookFunc = M.getOrInsertFunction("print_store_pointer", 											IntegerType::getInt32Ty
											(getGlobalContext()), voidType,IntegerType::getInt32Ty
											(getGlobalContext()),IntegerType::getInt32Ty
											(getGlobalContext()), NULL);
										hook = dyn_cast <Function> (hookFunc);
										//load value of count
										LoadInst* load_count = new LoadInst(IterationCount, "", false);
										LoadInst* load_ptr = new LoadInst(alloca_void_ptr, "", false);
										LoadInst* load_loop = new LoadInst(FBloc, "", false);
	  									std::vector<Value*> int32_15_params;
	  									int32_15_params.push_back(load_ptr);
	  									int32_15_params.push_back(load_count);
	  									int32_15_params.push_back(load_loop);
  										//int32_15_params.push_back(inst);
										CallInst* call_print = CallInst::Create(hook, int32_15_params, "");
										/*insert the instructions*/
										//F->getInstList().insert((Instruction*)CI, alloca_ptr);
										F->getInstList().insert((Instruction*)CI, cast);
										F->getInstList().insert((Instruction*)CI, store_void);
										F->getInstList().insert((Instruction*)CI, load_ptr);
										F->getInstList().insert((Instruction*)CI, load_count);
										F->getInstList().insert((Instruction*)CI, load_loop);
										F->getInstList().insert((Instruction*)CI, call_print);
									}//if pointer type
								}//end of "if store instruction"
							}//end of for
						}//if loopBody
					}//end of if candidate loop
				}//end of if(L)

				//we also have to add a call to write_to_file at the last basic block from main()
				Constant *func4 = M.getOrInsertFunction("write_to_file", IntegerType::getInt32Ty
						(getGlobalContext()), NULL);
				for (BasicBlock::iterator j = F->begin(), e = F->end(); j != e; ++j ) {
					if(isa<ReturnInst>(&(*j)))
					{

						//avoid inserting to our own library functions
						if (MDNode *N = j->getMetadata("dbg")) {  // Here I is an LLVM instruction
							DILocation Loc(N);                      // DILocation is in DebugInfo.h
							StringRef File = Loc.getFilename();
							if(!((File.find("prof_library.c",0))))
								{int i = 0; }
							else 
							{
								//errs() << func -> getName() << "\n";
								Function * func55 = (Function *)func;
								StringRef name = func55->getName();
								if (!(name.find(*main,0)) || !(name.find(*main1,0)))
								{
									errs()<<"Found main\n";
									if(func4)
									{
										Function *func1;
										func1= cast<Function>(func4);
										CallInst* call = CallInst::Create(func1, "");
										F->getInstList().insert((Instruction*)j, call);
									}
									else
										errs() << "write_to_file not found \n";
								}
							}
						}//end of if MDNode
					}//end of if a return instruction
					if(isa<CallInst>(&(*j)))
					{
						CallInst *CI = dyn_cast<CallInst>(j);
						Function * fn = CI->getCalledFunction();
						//if it's a call to exit function, also add a call to write_to_file
						if(fn)
						{
							if (!(fn->getName().find("exit",0)))
							{
								if(func)
								{
									Function *func1;
									func1= cast<Function>(func4);
									CallInst* call = CallInst::Create(func1, "");
									F->getInstList().insert((Instruction*)j, call);
								}
								else
									errs() << "write_to_file not found \n";
							}//end of if called function is exit()
						}

					}//if call instruction
				}//end of for
		}//end of for(Function::iterator F = func->begin(), E = func->end(); F!= E; ++F)
	}//end of for(Module::iterator func = M.begin(), y = M.end(); func!= y; ++func)
	return false;
	}//end of runOnModule()
	virtual void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.addRequired<LoopInfo>();
		AU.addPreserved<LoopInfo>();
		AU.addRequired<ScalarEvolution>();
		AU.addPreserved<ScalarEvolution>();
	}

	//bool candidateLoop(Loop *L, LoopInfo &LIF, ScalarEvolution &S);
	
    };
}
char ProfileDependence::ID = 0;
static RegisterPass<ProfileDependence> X("profileDependence", "profile memory dependences",false);


//helper functions

/*static bool candidateLoop(Loop *L, LoopInfo &LIF, ScalarEvolution &S)
{
		//check whether the loop has a dedicated exit block
		BasicBlock * BB = L->getUniqueExitBlock();
		if(BB);
		else return false;

		//check if the loop is countable
		unsigned TripCount; 
		BasicBlock *LatchBlock = L->getExitingBlock();
		if(LatchBlock)
			TripCount = S.getSmallConstantTripCount(L, LatchBlock);
		if(TripCount != 0);
		else return false;

		//check if the loop has function calls
		for(Loop::block_iterator bb = L->block_begin(), E = L->block_end(); bb!= E; ++bb)
	        {	
			
			for (BasicBlock::iterator j = (*bb)->begin(), e = (*bb)->end(); j != e; ++j ) {
				if(isa<CallInst>(&(*j)))
				{
					CallInst *CI = dyn_cast<CallInst>(j);
					Function * fn = CI->getCalledFunction();
					//if it's a call to exit function, also add a call to write_to_file
					if(fn)
					{
						if (!(fn->getName().find("exit",0)))
						{
							return false;
						}//end of if called function is exit()
					}//if(fn)

				}//if call instruction
				
			}//basicblock iterator	

		}//loop iterator
		return true;
}*/
