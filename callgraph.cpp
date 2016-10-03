#include "pin.H"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <set>

std::ofstream   GraphOutput;
string          module_name;

KNOB<string>    KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "callgraph.out", "specify graph file name");
KNOB<BOOL>      KnobVerbose(KNOB_MODE_WRITEONCE, "pintool", "v", "0", "Choose full graph or process and symbols listing");

std::map< string, std::set<string> > symbolTable;

INT32 Usage()
{
    cerr << "This tool produces a dynamic callgraph" << endl << endl;
    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

int call_level = 0;
string invalid = "invalid_rtn";

const string *Target2String(ADDRINT target)
{
    string name = RTN_FindNameByAddress(target);
    if (name == "")
        return &invalid;
    else
        return new string(name);
}

string extractFilename(const string& filename)
{
    std::cout << "Flooding " << filename << endl;
    size_t lastBackslash = filename.rfind("/");

    if (lastBackslash == string::npos)
    {
        return filename;
    }
    else
    {
        return filename.substr(lastBackslash + 1);
    }
}

string getModule(ADDRINT address)
{
    for(IMG img=APP_ImgHead(); IMG_Valid(img); img = IMG_Next(img))
    {
        for(SEC sec=IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
        {
            if (address >= SEC_Address(sec) && address < SEC_Address(sec) + SEC_Size(sec))
            {
                return extractFilename(IMG_Name(img));
            }
        }
    }

    return "";
}

VOID WriteProcNear(ADDRINT target, const string *name)
{
    // if (module_name != getModule(target)) return;
    if (!KnobVerbose.Value())
    {
        string module = getModule(target);
        symbolTable[module].insert(*name);

        return;
    }

    call_level++;

    GraphOutput << target << " | ";
    for (int i = 0; i < call_level; ++i)
    {
        GraphOutput << '\t';
    }

    GraphOutput << "CALL " << *name << endl;
}

VOID WriteProcFar(ADDRINT origin)
{
    if (!KnobVerbose.Value())
        return;

    // if (module_name != getModule(origin)) return;
    GraphOutput << origin << " | ";
    for (int i = 0; i < call_level; ++i)
    {
        GraphOutput << '\t';
    }

    GraphOutput << "CALL FAR" << endl;
}

VOID WriteRet(ADDRINT target, ADDRINT origin)
{
    if (!KnobVerbose.Value())
        return;

    // if (module_name != getModule(target)) return;
    GraphOutput << target << " | ";
    for (int i = 0; i < call_level; ++i)
    {
        GraphOutput << '\t';
    }

    GraphOutput << "RET" << endl;
    call_level--;
}

VOID Trace(TRACE trace, VOID *v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        INS tail = BBL_InsTail(bbl);

        if (INS_IsCall(tail))
        {
            // NEAR
            if (INS_IsDirectCall(tail))
            {
                const ADDRINT target = INS_DirectBranchOrCallTargetAddress(tail);
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE, (AFUNPTR)WriteProcNear,
                    IARG_BRANCH_TARGET_ADDR, IARG_PTR, Target2String(target), IARG_END);
            }
            else if (KnobVerbose.Value())
            {
                INS_InsertCall(tail, IPOINT_BEFORE,
                    (AFUNPTR)WriteProcFar, IARG_INST_PTR, IARG_END);
            }
        }
        else if (INS_IsRet(tail) && KnobVerbose.Value())
        {
            INS_InsertCall(tail, IPOINT_BEFORE,
                (AFUNPTR)WriteRet, IARG_BRANCH_TARGET_ADDR, IARG_END);

        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    if (!KnobVerbose.Value())
    {
        std::map<string, std::set<string> >::iterator it;
        for (it = symbolTable.begin(); it != symbolTable.end(); it++)
        {
            GraphOutput << "MODULE " << it->first << ':' << endl;

            std::set<string>::iterator s_it;
            for (s_it = it->second.begin(); s_it != it->second.end(); s_it++)
            {
                GraphOutput << *s_it << endl;
            } 

            GraphOutput << endl;
        }
    }

    GraphOutput << "# eof" << endl;
    GraphOutput.close();
}

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    module_name = string(argv[argc - 2]);
    cout << module_name << endl;
    module_name = extractFilename(module_name);

    GraphOutput.open(KnobOutputFile.Value().c_str());
    GraphOutput.setf(ios::showbase);

    if (KnobVerbose.Value())
    {
        GraphOutput << "ADDRESS  |  MODULE: " << module_name << endl;
        GraphOutput << "---------|" << endl;
    }
    
    TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}