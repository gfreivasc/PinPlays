#include "pin.H"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>

/* ================================================= */
/* Global Variables */
/* ================================================= */

std::map<ADDRINT, std::string>  disAssemblyMap;
std::ofstream                   MemoryReadLog;


/* ================================================= */
/* Commandline Switches */
/* ================================================= */

KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
    "memrltool.out", "specify memory read log file");

/* ================================================= */

VOID MemoryRead(ADDRINT appIp, ADDRINT memAddrRead, UINT32 memReadSize)
{
    MemoryReadLog << appIp << '\t' << disAssemblyMap[appIp]
        << "\t\t\treads " << memReadSize << " bytes of memory at "
        << memAddrRead << endl;
}

/* ================================================= */
/* Intruction Callback */
/* ================================================= */

VOID Instruction(INS ins, VOID *v)
{
    if (INS_IsMemoryRead(ins))
    {
        disAssemblyMap[INS_Address(ins)] = INS_Disassemble(ins);

        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryRead,
            IARG_INST_PTR, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
            IARG_END);
    }
}

int main(int argc, char* argv[])
{
    PIN_Init(argc, argv);

    MemoryReadLog.open(KnobOutputFile.Value().c_str());

    MemoryReadLog << hex;
    MemoryReadLog.setf(ios::showbase);

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_StartProgram();  // never returns
    return 0;
}