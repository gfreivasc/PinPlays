#include "pin.H"
#include <iostream>

UINT64 icount = 0;

void docount() { icount++; }

void Instruction(INS ins, VOID *v)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount,
        IARG_END);
}

void Fini(INT32 code, VOID *v)
{
    std::cerr << "Count: " << icount << endl;
}

int main(int argc, char* argv[])
{
    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();  // Never Returns
    return 0;
}