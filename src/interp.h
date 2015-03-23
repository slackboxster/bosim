#ifndef INTERP_H
#define INTERP_H

#include <QPlainTextEdit>
#include <QTableWidget>
#include <QCheckBox>
#include <QComboBox>
#include <string>
using namespace std;

struct nmap
{
    string name;
    int addr;
    int size;
};


//this is where we add the stuff for the runtime environment:
const int MAXMEM = 1000;
const int MAXNAMES = 100;

//opcode constants for instruction set: //note that these correspond to the ascii for normal printable characters to make an injection easier.
const int DOSTUFF    = 65; //A
const int CALL       = 66; //B
const int VAR        = 67; //C
const int FUNCTION   = 68; //D
const int RETURN     = 69; //E
const int READ       = 70; //F
const int READN      = 71; //G
const int PRINT      = 72; //H
const int EXIT       = 73; //I
const int CRASH      = 74; //J. remember compiler does not recognize crash.

//number of operands for each opcode (includes the actual opcode
const int nDOSTUFF    = 1;
const int nCALL       = 2;
const int nVAR        = 3;
const int nFUNCTION   = 1;//function has no operands.
const int nRETURN     = 1;
const int nREAD       = 2;
const int nREADN      = 3;
const int nPRINT      = 2;
const int nEXIT       = 1;
const int nCRASH      = 1; // remember compiler does not recognize crash.


//interp defines the runtime environment.
//interp is created and run within a single gui event.
//input is read as if delimited by lines.
class interp
{
public:
    interp();

    void init(QPlainTextEdit * pscreen, QPlainTextEdit * pstack, QPlainTextEdit * pcode, QPlainTextEdit * pinput, QTableWidget * pcodeseg, QTableWidget * pstacktable, QCheckBox * puibounds, QCheckBox * procode, QComboBox * pcanaries); // this constructor enables using the screens to get the data.


    void execute(); // main function whereby the GUI causes the runtime to execute.

    //functions for the runtime environment
    void compile(); //takes the code written in the code widget and compiles it into the contents of the mem array -- opcodes, parameters, and variables.
    void run(); // this starts with code loaded into memory and performs the appropriate functions on the data.
    void start(); // starts with compiled code and creates the main stack frame and runs the first instruction.
    bool step(); // if start has been run, this will step through the compiled code by running the next instruction. returns true if program is still running
    void stop(); // stops the code running (basically unsets the running flag).
    void codeChanged(); // called if the code has changed -- this implies it must be recompiled.

private:
    //variables for runtime. These are here to enable stepping through the program in runtime.
    int stacktop; // the index of the top of the stack
    int memctr; // program counter
    bool compiled;
    bool running;
    int codetop; // the top of the code segment -- used for outputting the code segment post compilation.

    //countermeasures variables
    bool bounds; //keeps the current state of bounds checking.
    bool rocode; //keeps the current state of the read-only code segment setting.
    enum t_canary { c_none, c_term, c_random, c_xor };
    t_canary canary; // keeps the canary setting value (this will probably become based on a struct).
    //the following ui element pointers are consulted by the compiler, which then sets the above flags.
    //Apart from the compiler, the above settings should not be modified.
    QCheckBox * uibounds; //the state of this checkbox is consulted at compile to determine if bounds checking should be set.
    QCheckBox * uirocode; //pointer to the checkbox to consult to determine if code segment needs to be read only.
    QComboBox * uicanaries; // pointer to the combobox to consult to determine what kind of canaries to use.

    string instruction(int opcode); // performs reverse opcode lookup -- gives the instruction that corresponds with a given opcode.
    int operands (int opcode); // returns  the number of operands on the given opcode, thus the number to increment the current address by to get to the next instruction.

    //parser helper functions
    void getsrcline(string &src, string & line); // gets a line of the top of src, puts it into line, and pulls that line out of src
    string strip_param(string &src); //removes everything before the first space and returns it, while the src value is the remainder of the string.
    int whereis(string name); // resolves a function name or variable name by returning the integer value associated with that name.

    //runtime helper functions:
    int add_stack_frame(int func_addr, int prev_base, int prev_top, int ret); //func_addr so that it can find where to start looking for variables.
        //add_stack_frame returns the address of the new top of the stack.
    void setvar(int varname, string value, int stacktop); //sets the given var to the given value.
    string getvar(int varname, int stacktop); //returns the value of the given variable.
    void showstack(int stacktop); // puts the contents of the stack in readable form into the stack UI element.

    //variables for the runtime environment
    //we need ints to be unsigned so that characters inject properly.
    unsigned int mem[MAXMEM]; // this array stores the memory of our program.
    nmap namemap[MAXNAMES]; // this array of structs maps names to memory indices -- do we need this? (only for compiling)...
    int namect; // counts the names in the names array.

    //variables for I/O:
    QPlainTextEdit * screen; // points to the output element for program output
    QPlainTextEdit * stack;  // points to the output element that we use to show the stack.
    QPlainTextEdit * code;   // points to the input element for code to interpret
    QPlainTextEdit * input;  // points to the input element for data to read.
    QTableWidget   * codeseg; // points to the output element for the code segment.
    QTableWidget   * stacktable; // points to the output element for the stack in tabular form

    //Instruction Set:
    //exit
    //function <name>
    //return (endfunction)
    //var <name> <size>  //string variables end in 0, like C
    //dostuff  -- this one takes no parameters
    //call <name>
    //read <target name>
    //readn <target name> <num characters>
    //print <target name>
    //crash -- exists purely for an attack overflow. Compiler does not produce this opcode, so an attack is necessary to get it to occur.

    //stack frame format:
    // variables -- int for size, followed by that many elements
    // return address -- where in the code the program must return
    // previous stack frame -- head
    // previous stack frame -- tail

    // a function call adds the variable stuff for the current function call, adds the variables to the frame, and writes the previous values into the end.
    // then puts the current values into the runtime environment variables. in other words, disassembling a stack frame starts with the bottom of said stack frame.
};

#endif // INTERP_H


/*
    //functions to execute runtime instructions:
    void exit();
    void crash(); //the crash function. The compiler should never compile a crash function, but its opcode is still available, and the runtime interprets it.
    void call_func(); // calls a function, moves the current instruction pointer, adds element to the stack.
    void ret_func(); // returns from a function call.
    void read(); // reads from the input buffer until it hits a newline character.
    void readn(); //reads num characters from the input buffer
    void dostuff(); //basically just invokes the output for something.

    void var(); // not sure if this is necessary for the runtime -- probably only a compiler thing
 */
