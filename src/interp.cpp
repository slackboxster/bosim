#include "interp.h"

interp::interp()
{
    namect = 0;
}

void interp::init
    (
        QPlainTextEdit *pscreen     ,
        QPlainTextEdit *pstack      ,
        QPlainTextEdit *pcode       ,
        QPlainTextEdit *pinput      ,
        QTableWidget   *pcodeseg    ,
        QTableWidget   *pstacktable ,
        QCheckBox      *puibounds   ,
        QCheckBox      *puirocode   ,
        QComboBox      *puicanaries
    )
{
    screen     = pscreen     ;
    stack      = pstack      ;
    code       = pcode       ;
    input      = pinput      ;
    codeseg    = pcodeseg    ;
    uibounds   = puibounds   ;
    uirocode   = puirocode   ;
    uicanaries = puicanaries ;
    stacktable = pstacktable ;
    compiled   = false       ;
    running    = false       ;
    bounds     = false       ;
    rocode     = false       ;
    canary     = c_none      ;
}

void interp::execute()
{
    compile();
    run();
    return;
}

void interp::compile()
{
    //set bounds, rocode and canary from the ui elements.
    bounds = uibounds->checkState();
    rocode = uirocode->checkState();
    canary = (t_canary)uicanaries->currentIndex();

    string src = code->toPlainText().toStdString();

    namect = 0; //need to start with a fresh namemap on a new compile.
    string line = "";
    int memctr = 0; //memctr tells us which element of memory is the next to be written to.
    //compiler temporary addressing starts addresses at 100; all the code must be read and all the names put into the map
    //before actual names can be assigned. real addresses will be inserted by going through and finding the sentinel address and putting the real one in its place.
    int linectr = 1; // line of input code, so that error messages refer to lines of human code not memory cell indices.
    int varaddr = MAXMEM + 1; // variable names (integers...) start above the top memory address. this variable is used to keep track of the highest value handed out.

    getsrcline(src,line);
    while ((src.length() > 0) || (line.length() > 0))
    {
        if (line.find("dostuff")        == 0)
        {
                mem[memctr] = DOSTUFF;
                memctr++;
        }
        else if (line.find("call") == 0)
        {
            mem[memctr] = CALL;
            memctr++;
            //get the parameters of the call function -- one name
            strip_param(line); // strip the actual text
            //now strip the name
            string target = strip_param(line);
            //resolve the name of the target.
            mem[memctr] = whereis(target);
            if (mem[memctr] == -1)
                screen->setPlainText("Error: invalid function call target \"" +
                                    QString::fromStdString(target) + "\" on line "
                                    + QString::number(linectr));
            memctr++;
        }
        else if (line.find("var") == 0)
        {
                mem[memctr] = VAR;
                memctr++;
                //strip var
                strip_param(line);
                //strip the variable name
                namemap[namect].name = strip_param(line);
                //create an address for this variable
                namemap[namect].addr = varaddr;
                mem[memctr] = varaddr;
                memctr++;
                int tempsize = atoi(strip_param(line).c_str());
                namemap[namect].size = tempsize / 4 + ((tempsize % 4 == 0) ? 0 : 1 );
                    // we translate the size from characters into integers from the get go.
                    //consequently the only variable logic that must change to store 4 chars in an int is logic that gets and sets vars.
                    //The rest is dependent on the size parameter in the compiled text...
                mem[memctr] = namemap[namect].size;
                memctr++;
                varaddr++;
                //increment the name count
                namect++;
        }
        else if (line.find("function")      == 0)
        {
            mem[memctr] = FUNCTION;
            //add name and address to namemap
            strip_param(line);
            namemap[namect].name = strip_param(line);
            namemap[namect].addr = memctr;
            memctr++;
            namect++;
        }
        else if (line.find("return")   == 0)
        {
            mem[memctr] = RETURN;
            memctr++;
        }
        //put readn before read because read is also in readn...
        else if (line.find("readn")      == 0) // readn
        {
            mem[memctr] = READN;
            memctr++;
            //get name and find its address. if a -1, error message. runtime will ignore it with a mutter.
            strip_param(line);
            string target = strip_param(line);
            mem[memctr] = whereis(target);
            if (mem[memctr] == -1)
                screen->setPlainText("Error: invalid readn target \"" +
                                    QString::fromStdString(target) + "\" on line "
                                    + QString::number(linectr));
            memctr++;
            //also get number of characters to read.
            mem[memctr] = atoi(strip_param(line).c_str());
            memctr++;
        }
        else if (line.find("read")      == 0)
        {
            mem[memctr] = READ;
            memctr++;
            //get name and find its address. if a -1, error message. runtime will ignore it with a mutter.
            strip_param(line);
            string target = strip_param(line);
            mem[memctr] = whereis(target);
            if (mem[memctr] == -1)
                screen->setPlainText("Error: invalid read target \"" +
                                    QString::fromStdString(target) + "\" on line "
                                    + QString::number(linectr));
            memctr++;
        }
        else if (line.find("print")      == 0)
        {
            mem[memctr] = PRINT;
            memctr++;
            //get name and find its address. if a -1, error message. runtime will ignore it with a mutter.
            strip_param(line);
            string target = strip_param(line);
            mem[memctr] = whereis(target);
            if (mem[memctr] == -1)
                screen->setPlainText("Error: invalid print target \"" +
                                    QString::fromStdString(target) + "\" on line "
                                    + QString::number(linectr));
            memctr++;
        }
        else if (line.find("exit")      == 0)
        {
            mem[memctr] = EXIT;
            memctr++;
        }
        else
        {
            screen->appendPlainText("Error: invalid instruction \"" + QString::fromStdString(line) + "\" on line " + QString::number(linectr) + "\n");
        }
        getsrcline(src,line);
        linectr++;
    }//while the strings have length
    compiled = true;
    codetop = memctr;
    return;
}


void interp::run()
{
    start();
    while (mem[memctr] != EXIT)
        step();
    return;
}


void interp::start()
{
    if ((running == false) && (compiled == true))
    {

        stacktop = MAXMEM; // the index of the top of the stack
        memctr = 0; // program counter

        //set the canary if it the option is set to require a random value (done at runtime, not compile time).
        if ((canary == c_random) || (canary == c_xor))
            mem[codetop] = rand(); // we store the canary at the top of the code segment (which means it's protected by rocode)
                                    // and because this is more realistic.
        //create the main stack frame.
        //main has an address of -1, since it doesn't have a "function" command, therefore its variable section is the first line of the
            //program. thus -2 means that the "function" is the memory cell before real memory starts.
        stacktop = add_stack_frame(-1,stacktop,stacktop,-1);//these values shouldn't ever have to be used.

        running = true;
        step();
    }
}


bool interp::step()
{
    if (running == true)
    {
        switch (mem[memctr])
        {
        case DOSTUFF:
            screen->appendPlainText("Doing Stuff, Yay!!!");
            memctr++;
            break;
        case CALL:
            {
                memctr++;
                int name = mem[memctr];
                int currbase = mem[stacktop +2]; //stacktop has the return address, below which is the previous top and then previous base
                int ret = memctr+1; // the operand after the current call line is where we need to return after the function call
                stacktop = add_stack_frame(name,currbase,stacktop,ret);
                if (stacktop == -1) // sentinel value for failed stack frame adding (in other words, we've run out of memory)
                {
                    screen->appendPlainText("Program has run out of memory and has been terminated for your safety.");
                    running = false;
                    return false;
                }
                memctr = name+1; // since the function operand causes the runtime to jump past its return statement, we must go to the instruction after the function operand.
            }
            break;
        case VAR:
            //if var is found, it will be interpreted by the code that adds stack frames. thus this part just ignores it.
            memctr += 3; //increment by 3 to jump over the two operands and to the next instruction.
            //we need this so that VAR doesn't throw errors.
            break;
        case FUNCTION:
            //if function is found, must jump over the entire function (starting with the instruction, and then continuing until return)
            memctr++;

            while (mem[memctr] != RETURN)
            {
                memctr += operands(mem[memctr]);
            }
            memctr++; //one more jump for the return statement.
            break;
        case RETURN:
            //if canaries are set, check the canaries appropriately.
                //if a check fails, we print a warning, stop the program, and return from step.
            if (canary > 0)
            {
                //canary is positioned at stacktop - 3.

                switch (canary)
                {
                case c_term:
                    if (mem[stacktop - 3] != 0)
                    {
                        screen->appendPlainText("Stack Smashing Detected. Program has been terminated for your safety.");
                        running = false;
                        return false;
                    }
                    break;
                case c_random:
                    if (mem[stacktop - 3] != mem[codetop])
                    {
                        screen->appendPlainText("Stack Smashing Detected. Program has been terminated for your safety.");
                        running = false;
                        return false;
                    }
                    break;
                case c_xor:
                    //xor canaries are xored with return address.
                    if (mem[stacktop - 3] != (mem[codetop] ^ mem[stacktop]))
                    {
                        screen->appendPlainText("Stack Smashing Detected. Program has been terminated for your safety.");
                        running = false;
                        return false;
                    }

                    break;
                default:
                    //fail, since something wrong if canary is greater than 0 and not one of the above...
                    screen->appendPlainText("Something's wrong with the canary configuration. Program has been terminated for your safety.");
                    running = false;
                    return false;
                    break;
                }

            }

        //the actual return code is really simple.(the following three lines of code) adding canaries increases overhead dramatically...
            //set memctr to the return address
            memctr = mem[stacktop];
            if ((memctr < 0) || (memctr > MAXMEM)) //if the return address is outside of memory, we need to quit.
            {
                screen->appendPlainText("Return address outside of memory. Program has been terminated for your safety.");
                running = false;
                return false;
                break;
            }
            //set the stacktop to the previous top.
            stacktop++; //because the stack grows down and shrinks up.
            stacktop = mem[stacktop];
            break;
        case READ:
            {
                memctr++;
                int name = mem[memctr];
                memctr++;

                //getsrcline.
                string newin = input->toPlainText().toStdString();
                string ourline = "" ;
                getsrcline(newin,ourline);
                input->setPlainText(QString::fromStdString(newin));

                setvar(name, ourline, stacktop);
            }
            break;
        case READN:
            {
                memctr++;
                int name = mem[memctr];
                memctr++;
                int size = mem[memctr];
                string inchars = input->toPlainText().toStdString();
                if (inchars != "")
                {
                    input->setPlainText(QString::fromStdString(inchars.substr(size)));
                    inchars = inchars.substr(0,size);
                }
                else
                    input->setPlainText("");
                memctr++;
                setvar(name, inchars, stacktop);
            }
            break;
        case PRINT:
            //use getvar to get contents, then send them to the PlainTexter...
            memctr++;
            screen->appendPlainText(QString::fromStdString(getvar(mem[memctr],stacktop)));
            memctr++;
            break;
        case EXIT:
            screen->appendPlainText("Program has exited cleanly.");
            running = false;
            return false;
            break;
        case CRASH:
            screen->setPlainText("Fatal System Error: System is Crashing. Too much Red Bull. You Win!!!");
            running = false;
            return false;
            break;
        default:
            screen->appendPlainText("Unknown Instruction in cell " + QString::number(memctr) + " Mutter Mutter Grumble Grumble. Write Better Code Next Time.");
            memctr++;
            return true;
        }
        showstack(stacktop);
    }
    return true;
}

void interp::codeChanged()
{
    compiled = false;
}

void interp::stop()
{
    running = false;
}

void interp::getsrcline(string &src, string &line)
{
    int endline = 0;
    endline = src.find("\n");
    //if there is no endline, we should just copy the src to line and be done.
    if (endline == string::npos)
    {
        line = src;
        src = "";
        return;
    }
    else
    {
       line = src.substr(0, endline); // put first line into line
       src = src.substr(endline+1, src.length() - endline-1); // remove that line from the remaining source;
       return;
    }
}

//removes everything before the first space and returns it, while the src value is the remainder of the string.
string interp::strip_param(string &src)
{
    if (src.find(" ") != string::npos)
    {
        string result = src.substr(0,src.find(" "));
        src = src.substr(src.find(" ")+1);
        return result;
    }
    else
        return src;
}

int interp::whereis(string name)
{
    if (namect == 0) return -1; //indicates error
    else
    {
        for (int x = 0; x < namect; x++)
        {
            if (name == namemap[x].name)
                return namemap[x].addr; //here it is.
        }
    }
    return -1; // name not found.
}

//func_addr so that it can find where to start looking for variables.
    //add_stack_frame returns the address of the new top of the stack.
int interp::add_stack_frame(int func_addr, int prev_base, int prev_top, int ret)
{
    int varctr = func_addr + 1; // the first instruction after the function opcode is the instruction at which the variable declarations start.
    int stackptr = prev_top - 1; // -1 because stack goes down.
    while (mem[varctr] == VAR)
    {
        varctr++; // advance to the name of the variable
        mem[stackptr] = mem[varctr]; // this copies the name of the variable into the first element of the stack
        stackptr--;
        varctr++;
        mem[stackptr] = mem[varctr]; // copies the size of the variable.
        stackptr -= mem[varctr]; // move the stack up by the size of the variable.
        stackptr--; // move past the end of the variable, because the size of the variable actually puts us on the last cell of said variable.
        varctr++; //advances the varctr to the next variable or next instruction.
    }
    //if we escape the above loop, it means we are done with getting variable declarations, so we add a null to end var space for this frame,
        //and we put the prev base, prev top and return address values in, then return the new stack top.
    mem[stackptr] = 0; // this forms the basic terminator canary. The difference if terminator is set is that we actually check that it's there.
    stackptr--;

    //if canaries are set, we add a canary according to the appropriate setting.
        //we can't just use the previous cell for random or xor canaries, because that would cause the algorithms that check for the last 0
            //when retreiving variables to fail.
    if ((canary == c_random) || (canary == c_xor)) // if it's random or xor, we actually need to do something.
    {
        //the terminator canary is already stored.
        if (canary == c_random)
        {
            //implementing the canary is really quite simple: copy the value...
            mem[stackptr] = mem[codetop];
            stackptr--;
        }
        else if (canary == c_xor)
        {
            //and for xor, xor and copy the value
            mem[stackptr] = mem[codetop] ^ ret; // xor it with the return address.
            stackptr--;
        }
    }

    mem[stackptr] = prev_base;
    stackptr--;
    mem[stackptr] = prev_top;
    stackptr--;
    mem[stackptr] = ret;

    //if the stack ptr overruns the end of the code segment, we need to terminate the program, as the program has officially run out of memory.
    //this should be independent of whether or not we are running a read only code segment, because if we do this with normal programs its just asking for trouble no matter what.
    //in other words, it's not a matter of countermeasures, it's a matter of sanity.

    //terminate if we hit code.
    if (stackptr <= codetop) return -1;

    return stackptr;
}

//sets the given var to the given value.
void interp::setvar(int varname, string value, int stacktop)
{
    int varptr = mem[stacktop + 1] - 1; // set the variable pointer (which we will use to find our variable) to the previous top of the stack plus 1 (so first variable of current frame).
    if (varptr > MAXMEM) varptr = MAXMEM-1; // to prevent insane values from crashing the code.
    while (varptr > stacktop)
    {
        //if we have found the variable
        if (mem[varptr] == varname)
        {
            varptr -= 2; // increment by 2 to get to the start of the data.

            unsigned char mychar;
            mychar = value[0];
            mem[varptr] = mychar;

            //if bounds checking is to occur, it must happen here, so I would restructure the values in the for loop
            //so that it's a bunch of variables that are computed and then passed into it...
            //bounds checking here should just fail quietly. maybe output a warning to the screen?? (probably not)
            //also the read only code segment should happen here.

            int chars_to_write;
            //start with the default condition:
            chars_to_write = (value.length() + ( (value.length() % 4 == 0) ? 0 : (4 - (value.length() % 4)) ));
                // the horrible looking condition on this loop ensures that the last memory cell we write gets the appropriate number of nulls appended.
            // if the number cells that will be written will drop below codetop rocode drops that to the top of the code segment.
            if (rocode && ((varptr - (chars_to_write/4)) < codetop))
               chars_to_write = 4 * (varptr - codetop);
            // if bounds checking is on, take the same approach as above, except with the size.
            //also, bounds checking is applied after rocode, because bounds checking may be tighter.
            int size = mem[varptr+1]; // varptr + 1 gets us to the cell containing the size value.
            if (bounds && ((chars_to_write/4) > size ))
                chars_to_write = size*4;

            //now perform the loop based on the values we are supposed to have.
            for (int i = 1; i < chars_to_write  ; i++)
            {
                if ((i%4)==0)
                {
                    varptr--;
                    mem[varptr] = 0; // set the int to 0 if this is a new one.
                }
                mem[varptr] = mem[varptr] << 8; //shift the value by 8 bits to make space...
                mem[varptr] = mem[varptr] | (i<value.length() ? (unsigned char)value[i] : 0); // add the new character, unless we are over the end, in which case, add 0.
            }
            return;
        }
        else
        {
            //if we have not found variable, we need to go to the next one
            varptr--; //move to the size value
            varptr -= mem[varptr]; // increment varptr by the size to move to the next variable.
            varptr--; //move to the next varname (because size gets us to the end of the current var.
            if (mem[varptr] == 0) return; // so if we hit the null character, we assume we have finished the variable section...
        }
    }
}

//returns the value of the given variable.
string interp::getvar(int varname, int stacktop)
{
    int varptr = mem[stacktop +1] - 1; // set the variable pointer (which we will use to find our variable) to the previous top of the stack plus 1 (so first variable of current frame).
    if (varptr > MAXMEM) varptr = MAXMEM-1; // to prevent insane values from crashing the code.
    string value = "";
    while (varptr > stacktop)
    {
        //if we have found the variable
        if (mem[varptr] == varname)
        {
            varptr -= 1; // increment by 1 since the start of the data is 2 down and 0 % 4 == 0, which will increment the pointer by one later, we only increment by 1 here.
            char last = 'a';
            int ind = 0; // index in string.
                //bounds checking on reads is redundant if it is done on writes.
            while (last != '\0')
            {
                if (ind % 4 == 0) varptr--; //advance to the next data cell here...
                int temp = mem[varptr]; // using a temp ensures that our actions don't destroy memory.
                temp=temp<<(ind%4)*8; // left shift by 0,8,16,or 24 bits to cut the unnecessary data off the front of the int.
                temp=temp>>24; // shifting back by 24 bits means only the first 8 bits
                last = temp; // do this more for the loop terminating condition than anything else.
                if (last != 0) value += last;
                ind++;
            }
            return value;
        }
        else
        {
            //if we have not found variable, we need to go to the next one
            varptr--; //move to the size value
            varptr -= mem[varptr]; // increment varptr by the size to move to the next variable.
            varptr--; // move to the next varname
            if (mem[varptr] == 0) return ""; // so if we hit the null character, we assume we have finished the variable section...
        }
    }
    return "";
}

void interp::showstack(int stacktop)
{

    //stack->setPlainText("Next Instruction: " + QString::fromStdString(instruction(mem[memctr])) + " at location " + QString::number(memctr));
    //this to show what the next instruction to be executed is.
    stack->setPlainText(QString::fromStdString(instruction(mem[memctr])) + "(" + QString::number(mem[memctr]) + ") at location " + QString::number(memctr));
    //reset the stack table
    stacktable->setRowCount(0);

    //stack->appendPlainText("The Stack:");
    int curr = MAXMEM -1;
    int framectr = 0; // tracks which frame we are using. corresponds to rowcount...
    while (curr > stacktop)
    {
        //this is a new frame, so add a new row and give it a frame cell
        stacktable->insertRow(stacktable->rowCount());
        stacktable->setItem(stacktable->rowCount()-1,0,new QTableWidgetItem(QString::number(framectr)));
        if (mem[curr] == 0)
        {
            //stack->appendPlainText("No Variables in this frame");
        }
        else
        {
            int varctr = 0; //we use this to track the number of vars in a frame. the main purpose is to see if we need to add a row
                            //to the stack display for the current variable.
            while ((mem[curr] != 0) && (curr > stacktop)) // adding the curr condition to prevent segfault by going off bottom of array
            {
                varctr++;
                if (varctr > 1) stacktable->insertRow(stacktable->rowCount()); //add a row if this is not the first row of the table.
                int name = mem[curr];
                stacktable->setItem(stacktable->rowCount()-1,1,new QTableWidgetItem(QString::number(name)));
                curr--;
                int size = mem[curr];
                if (size > 1000) size = 994; // as a sanity check
                stacktable->setItem(stacktable->rowCount()-1,2,new QTableWidgetItem(QString::number(size)));
                curr--;
                //now treat it as if it is a c-style string, but we limit reading to the size of the variable to prevent overrunning the stack.
                int ind = 0; //index in memory
                string value = "";
                while ( (ind < size) && (curr > stacktop) ) // adding the curr condition to prevent segfault by going off bottom of array
                {
                    for (int k = 0; k < 4; k++)
                    {
                        int temp = mem[curr]; // using a temp ensures that our actions don't destroy memory.
                        temp=temp<<k*8; // left shift by 0,8,16,or 24 bits to cut the unnecessary data off the front of the int.
                        temp=temp>>24; // shifting back by 24 bits means only the first 8 bits
                        char last = temp;
                        value += last;
                    }
                    ind++;
                    curr--;
                }
                //stack->appendPlainText(QString::number(name) + "(" + QString::number(size) +"):" + QString::fromStdString(value) );
                stacktable->setItem(stacktable->rowCount()-1,3,new QTableWidgetItem(QString::fromStdString(value)));
            }
        }
        //if terminator canary is set, we must display its value in the table
        if (canary == c_term) stacktable->setItem(stacktable->rowCount()-1,4,new QTableWidgetItem(QString::number(mem[curr])));
        curr--;

        //if canary is set (even terminator -- 0 or "invalid (blaa)") we need to create a value for its cell.
        if ((canary == c_random) || (canary == c_xor))
        {
            stacktable->setItem(stacktable->rowCount()-1,4,new QTableWidgetItem(QString::number(mem[curr])));
            curr--;
        }
        //now the stack base
        stacktable->setItem(stacktable->rowCount()-1,5,new QTableWidgetItem(QString::number(mem[curr])));
        curr--;
        //stack top
        stacktable->setItem(stacktable->rowCount()-1,6,new QTableWidgetItem(QString::number(mem[curr])));
        curr--;
        //return address
        stacktable->setItem(stacktable->rowCount()-1,7,new QTableWidgetItem(QString::number(mem[curr])));
        curr--;
        framectr++;
    }

    //now we show the code segment in the codeseg object.
    //this needs to become a function call so that we can call it within the compile button press (or even from the compile function).
    curr = 0;
    //stack->appendPlainText("Code Segment:");
    //we need to clear the code segment before redrawing it:
    codeseg->setRowCount(0);
    while (curr < codetop)
    {
        //declare four pointers to QTableWidgetItems which are then used for the instruction and its operands.
        QTableWidgetItem *addr = new QTableWidgetItem("[" + QString::number(curr) + "]");
        QTableWidgetItem *instout = NULL;
        QTableWidgetItem *opr1 = NULL;
        QTableWidgetItem *opr2 = NULL;

        int instr = mem[curr];
//        string out = "[" + QString::number(curr).toStdString() + "]: " + instruction(instr);
        instout = new QTableWidgetItem(QString::fromStdString(instruction(instr)) + " (" + QString::number(instr) + ")");
        int ops = operands(instr);
        for (int x = 1; x < ops; x++) // the instruction is considered one of the operands.
        {
            curr++;
            //out += ", " + QString::number(mem[curr]).toStdString();
            if (x == 1) opr1 = new QTableWidgetItem(QString::number(mem[curr]));
            if (x == 2) opr2 = new QTableWidgetItem(QString::number(mem[curr]));
        }
        //stack->appendPlainText(QString::fromStdString(out));

        //now we perform the output for the code segment.
        codeseg->insertRow(codeseg->rowCount()); //add a row
        codeseg->setItem(codeseg->rowCount() - 1, 0, addr); // insert the address cell
        codeseg->setItem(codeseg->rowCount() - 1, 1, instout); // insert the instruction cell
        if (opr1 != NULL) codeseg->setItem(codeseg->rowCount() - 1, 2, opr1); //insert first operand if it exists
        if (opr2 != NULL) codeseg->setItem(codeseg->rowCount() - 1, 3, opr2); //insert the second operand if it exists

        curr++;
    }

    return;
}


string interp::instruction(int opcode)
{
    switch (opcode)
    {
    case DOSTUFF:
        return "dostuff";
        break;
    case CALL:
        return "call";
        break;
    case VAR:
        return "var";
        break;
    case FUNCTION:
        return "function";
        break;
    case RETURN:
        return "return";
        break;
    case READ:
        return "read";
        break;
    case READN:
        return "readn";
        break;
    case PRINT:
        return "print";
        break;
    case EXIT:
        return "exit";
        break;
    case CRASH:
        return "crash";
        break;
    default:
        return "unknown: " + QString::number(opcode).toStdString();
        break;
    }
    return "huh?: " + QString::number(opcode).toStdString();
}


int interp::operands(int opcode)
{
    //in order to jump, we need to jump according to the number of operands in each instructoin as defined in the nXXXX constants
    switch (opcode)
    {
    case DOSTUFF:
        return nDOSTUFF;
        break;
    case CALL:
        return nCALL;
        break;
    case VAR:
        return nVAR;
        break;
    case FUNCTION:
        return nFUNCTION;
        break;
    case RETURN:
        return 1;
        break;
    case READ:
        return nREAD;
        break;
    case READN:
        return nREADN;
        break;
    case PRINT:
        return nPRINT;
        break;
    case EXIT:
        return nEXIT;
        break;
    case CRASH:
        return nCRASH;
        break;
    default:
        return 1;
    }
}
