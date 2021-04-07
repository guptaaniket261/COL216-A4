#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
using namespace std;
// function to convert a decimal number to a hexadecimal number
long long int maxClockCycles = 10000;
bool infinite_loop = false;
int ROW_ACCESS_DELAY, COL_ACCESS_DELAY;
int row_buffer_updates = 0;
struct toPrint
{
    int startingCycle;
    int endingCycle;
    string instruction;
    string RegisterChanged;
    string DRAMoperation;
    string DRAMchanges;
};

vector<toPrint> prints;

struct Instruction
{
    string name;
    string field_1 = "";
    string field_2 = "";
    string field_3 = "";
};
struct DRAM_ins
{
    int ins_number;     //stores instruction_number
    int type;           //0 if lw, 1 if sw
    int memory_address; //stores load/store address
    int value = 0;      //stores the value of register in case of sw instruction, as we are already passing it
    string reg;         //stores the register of instruction
};

int load_store;
string curr_register;
DRAM_ins currentInstruction;
vector<string> words;              //stores the entire file as a vector of strings
map<int, string> register_numbers; //maps each number in 0-31 to a register
map<string, int> register_values;  //stores the value of data stored in each register
bool validFile = true;             //will be false if file is invalid at any point
vector<Instruction> instructs;     //stores instructions as structs
int memory[(1 << 20)] = {0};       //memory used to store the data
int PC = 0;                        // PC pointer, points to the next instruction
int clock_cycles = 0;
map<string, int> operation;
map<int, string> intTostr_operation;
map<string, int> ins_register; //map each register to its lw/sw row number
int curr_ins_num = -1;
int type_ins; /*instruction can be of three types 
type 0: only col_access
type 1: activation + col_acess
type 2: writeback, activation, col_access
*/
int row_buffer = -1;
int starting_cycle_num = -1; //stores the starting cycle number for each instruction
int writeback_row_num = -1;
int col_access_num = -1;
vector<deque<DRAM_ins>> DRAM_queues(1024);
vector<int> ROW_BUFFER(1024);
int total_queue_size;
//bool completed_just =false;
//stores that if an instruction was just completed in the current cycle
//for printing, we will store the starting cycle, ending cycle,
int findType(int row_number)
{
    if (row_buffer == -1)
    {
        //this is type 1, activation + COl_access
        return 1;
    }
    else
    {
        if (row_number == row_buffer)
        {
            return 0;
            //this is type 0, only col_access
        }
        else
        {
            return 2; //writeback also required
        }
    }
}

bool valid_register(string R)
{
    return register_values.find(R) != register_values.end();
}

string findInstruction(Instruction current)
{
    int action = operation[current.name];
    switch (action)
    {
    case 1:
        return "add " + current.field_1 + ", " + current.field_2 + ", " + current.field_3;
        break;
    case 2:
        return "sub " + current.field_1 + ", " + current.field_2 + ", " + current.field_3;
        break;
    case 3:
        return "mul " + current.field_1 + ", " + current.field_2 + ", " + current.field_3;
        break;
    case 4:
        return "beq " + current.field_1 + ", " + current.field_2 + ", " + current.field_3;
        break;
    case 5:
        return "bne " + current.field_1 + ", " + current.field_2 + ", " + current.field_3;
        break;
    case 6:
        return "slt " + current.field_1 + ", " + current.field_2 + ", " + current.field_3;
        break;
    case 7:
        return "j " + current.field_1;
        break;
    case 8:
        return "lw " + current.field_1 + ", " + current.field_2 + "(" + current.field_3 + ")";
        break;
    case 9:
        return "sw " + current.field_1 + ", " + current.field_2 + "(" + current.field_3 + ")";
        break;
    case 10:
        return "addi " + current.field_1 + ", " + current.field_2 + ", " + current.field_3;
        break;
    }

    return "N.A";
}

void map_operations()
{
    operation["add"] = 1;
    operation["sub"] = 2;
    operation["mul"] = 3;
    operation["beq"] = 4;
    operation["bne"] = 5;
    operation["slt"] = 6;
    operation["j"] = 7;
    operation["lw"] = 8;
    operation["sw"] = 9;
    operation["addi"] = 10;

    intTostr_operation[1] = "add";
    intTostr_operation[2] = "sub";
    intTostr_operation[3] = "mul";
    intTostr_operation[4] = "beq";
    intTostr_operation[5] = "bne";
    intTostr_operation[6] = "slt";
    intTostr_operation[7] = "j";
    intTostr_operation[8] = "lw";
    intTostr_operation[9] = "sw";
    intTostr_operation[10] = "addi";
}
void reset_instruction()
{

    curr_ins_num = -1;
    ins_register[currentInstruction.reg] = -1;
    type_ins = -1;
    //row_buffer = -1;
    starting_cycle_num = -1;
    //currentInstruction = NULL;
    writeback_row_num = -1;
    col_access_num = -1;
}

bool is_integer(string s)
{
    for (int j = 0; j < s.length(); j++)
    {
        if (isdigit(s[j]) == false && !(j == 0 and s[j] == '-'))
        {
            return false;
        }
    }
    return true;
}

void writeBack()
{
    for (int i = row_buffer * 1024; i < row_buffer * 1024 + 1024; i++)
    {
        memory[i] = ROW_BUFFER[i - row_buffer * 1024];
    }
}

void activateRow(int num_row)
{
    for (int i = num_row * 1024; i < num_row * 1024 + 1024; i++)
    {
        ROW_BUFFER[i - num_row * 1024] = memory[i];
    }
}

void optimizeSw(int numRow)
{
    deque<DRAM_ins> temp = DRAM_queues[numRow];
    vector<DRAM_ins> temp1;
    int mem_last = -1;
    DRAM_ins lastsw;
    for (auto u : temp)
    {
        DRAM_ins curr = u;
        if (u.type == 1)
        { //sw instruction, see if consecutive present ahead
            if (mem_last == -1)
            {
                mem_last = curr.memory_address;
                lastsw = u;
            }
            else
            {
                if (u.memory_address == mem_last)
                {
                    lastsw = u;
                }
                else
                {
                    temp1.push_back(lastsw);
                    lastsw = u;
                    mem_last = u.memory_address;
                }
            }
        }
        else
        {
            temp1.push_back(u);
            mem_last = -1;
        }
    }
    if (mem_last != -1)
    {
        temp1.push_back(lastsw);
    }
    DRAM_queues[numRow] = {};
    for (int i = 0; i < temp1.size(); i++)
    {
        DRAM_queues[numRow].push_back(temp1[i]);
    }
}

void print(int endingCycle)
{
    toPrint curr;
    if (endingCycle - clock_cycles > 2 * ROW_ACCESS_DELAY + COL_ACCESS_DELAY)
    {
        curr.startingCycle = clock_cycles + 1;
        curr.endingCycle = clock_cycles + 1;
        curr.RegisterChanged = "N.A.";
        curr.DRAMoperation = "DRAM request issued";
        curr.DRAMchanges = "N.A.";
        curr.instruction = findInstruction(instructs[currentInstruction.ins_number]);
        prints.push_back(curr);
        clock_cycles += 1;
    }
    if (endingCycle - clock_cycles > ROW_ACCESS_DELAY + COL_ACCESS_DELAY)
    {
        curr.startingCycle = clock_cycles + 1;
        curr.endingCycle = starting_cycle_num + ROW_ACCESS_DELAY;
        curr.RegisterChanged = "N.A.";
        curr.DRAMoperation = "Writeback row " + to_string(row_buffer);
        curr.DRAMchanges = "N.A.";
        curr.instruction = findInstruction(instructs[currentInstruction.ins_number]);
        prints.push_back(curr);
        clock_cycles += ROW_ACCESS_DELAY;
    }
    if (endingCycle - clock_cycles > COL_ACCESS_DELAY)
    {
        curr.startingCycle = clock_cycles + 1;
        if (type_ins == 2)
            curr.endingCycle = starting_cycle_num + 2 * ROW_ACCESS_DELAY;
        else
            curr.endingCycle = starting_cycle_num + ROW_ACCESS_DELAY;
        curr.instruction = "N.A";
        curr.RegisterChanged = "N.A.";
        curr.DRAMoperation = "Activated row " + to_string(currentInstruction.memory_address / 1024);
        curr.DRAMchanges = "N.A.";
        curr.instruction = findInstruction(instructs[currentInstruction.ins_number]);
        prints.push_back(curr);
        clock_cycles += ROW_ACCESS_DELAY;
    }
    curr.startingCycle = clock_cycles + 1;
    if (type_ins == 2)
        curr.endingCycle = starting_cycle_num + 2 * ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
    else if (type_ins == 1)
        curr.endingCycle = starting_cycle_num + ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
    else
        curr.endingCycle = starting_cycle_num + COL_ACCESS_DELAY;
    curr.instruction = "N.A.";
    curr.RegisterChanged = "N.A.";
    curr.DRAMoperation = "Column access " + to_string(currentInstruction.memory_address % 1024);
    if (currentInstruction.type == 0)
        curr.DRAMchanges = currentInstruction.reg + " = " + to_string(register_values[currentInstruction.reg]);
    else
        curr.DRAMchanges = "memory address " + to_string(currentInstruction.memory_address) + "-" + to_string(currentInstruction.memory_address + 3) + "=" + to_string(currentInstruction.value);
    curr.instruction = findInstruction(instructs[currentInstruction.ins_number]);
    prints.push_back(curr);
    //return curr;
}

void perform_row(string reg)
{
    //optimise sw here.
    //in this function:
    //if the instruction corresponding to the register is current instruction, complete it
    //else, go to the row number of register.
    //in that, go to instruction containing reg. now, note its memory value mem
    //iterate in the queue, and keep executing the instructions with memory value mem
    //until you reach instruction having this register. Set appropriate row_buffer
    //executing an instruction is equivalent to updating the number of clock_cycles
    //and, pushing the required strings for printing.
    //will have to complete a partial instruction
    //first execute the current instruction
    if (curr_ins_num != -1)
    {

        if (currentInstruction.type == 0)
        {
            register_values[currentInstruction.reg] = ROW_BUFFER[col_access_num];
            ins_register[currentInstruction.reg] = -1;
            row_buffer_updates += type_ins;
        }
        else
        {
            ROW_BUFFER[col_access_num] = currentInstruction.value;
            row_buffer_updates += type_ins + 1;
        }
        int ending_cycle = starting_cycle_num + (type_ins)*ROW_ACCESS_DELAY + COL_ACCESS_DELAY + 1;
        clock_cycles = ending_cycle;

        reset_instruction();
    }

    int num_row = ins_register[reg];
    bool found_in_queue = false;
    int mem_value = -1;
    for (auto u : DRAM_queues[num_row])
    {
        if (u.reg == reg)
        {
            found_in_queue = true;
            mem_value = u.memory_address;
        }
    }

    if (found_in_queue)
    {

        bool found_reg_ins = false;
        vector<DRAM_ins> temp; //stores the new queue
        optimizeSw(num_row);
        for (auto u : DRAM_queues[num_row])
        {
            if (u.memory_address != mem_value)
            {
                temp.push_back(u);
            }

            else
            {
                int ending_cycle = starting_cycle_num + (type_ins)*ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
                if (!found_reg_ins)
                {
                    // generate string for printing function
                    // clock_cycles++; //as will begin in a new clock cycle

                    currentInstruction = u;
                    curr_ins_num = u.ins_number;
                    starting_cycle_num = clock_cycles;

                    int temp_type = findType(num_row);
                    type_ins = temp_type;
                    if (temp_type == 1)
                    {
                        activateRow(num_row);
                    }
                    else if (temp_type == 2)
                    {
                        writeback_row_num = row_buffer;
                        writeBack();
                        activateRow(num_row);
                    }
                    row_buffer = num_row;
                    col_access_num = u.memory_address - 1024 * row_buffer;

                    if (u.type == 0)
                    {
                        register_values[u.reg] = ROW_BUFFER[col_access_num];
                        ins_register[u.reg] = -1;
                        row_buffer_updates += type_ins;
                        //now print/store output string
                    }
                    else
                    {
                        ROW_BUFFER[col_access_num] = u.value;
                        row_buffer_updates += type_ins + 1;
                        //now print/store output string
                    }
                    print(ending_cycle);
                    clock_cycles = ending_cycle;
                    total_queue_size--;

                    reset_instruction();
                }
                else
                {
                    temp.push_back(u);
                }
            }
        }
        DRAM_queues[num_row] = {};
        for (int i = 0; i < temp.size(); i++)
        {
            //insert the remaining queue back
            DRAM_queues[num_row].push_back(temp[i]);
        }
    }
    else
    {
        return;
    }
}
void remove(string reg)
{
    int num_row = ins_register[reg];
    int mem_value = -1;
    vector<DRAM_ins> temp;
    for (auto u : DRAM_queues[num_row])
    {
        if (u.reg != reg)
        {
            temp.push_back(u);
        }
    }
    DRAM_queues[num_row] = {};
    for (int i = 0; i < temp.size(); i++)
    {
        DRAM_queues[num_row].push_back(temp[i]);
    }
}

void optimizeLw()
{
    struct Instruction current = instructs[PC];
    if (ins_register[current.field_1] != -1 && ins_register[current.field_1] == row_buffer)
    {
        //here, remove the found instruction from the queue
        //perform_row(current.field_1, true);
        remove(current.field_1);
    }
    if (ins_register[current.field_3] != -1 && ins_register[current.field_3] == row_buffer)
    {
        perform_row(current.field_3);
        //remove(current.field_3);
    }

    if (ins_register[current.field_1] != -1)
        remove(current.field_1);
    if (ins_register[current.field_3] != -1)
        perform_row(current.field_3);
}

void complete_remaining()
{
    if (curr_ins_num != -1)
    {
        if (currentInstruction.type == 0)
        {
            register_values[currentInstruction.reg] = ROW_BUFFER[col_access_num];
            ins_register[currentInstruction.reg] = -1;
            row_buffer_updates += type_ins;
        }
        else
        {
            ROW_BUFFER[col_access_num] = currentInstruction.value;
            row_buffer_updates += type_ins + 1;
        }
        int ending_cycle = starting_cycle_num + (type_ins)*ROW_ACCESS_DELAY + COL_ACCESS_DELAY + 1;
        print(ending_cycle);
        clock_cycles = ending_cycle;
        reset_instruction();
    }
    if (row_buffer != -1 && DRAM_queues[row_buffer].size() > 0)
    {
        //first execute these instructions, as these are of type 0 (only column activation required)
        optimizeSw(row_buffer);
        for (auto u : DRAM_queues[row_buffer])
        {
            starting_cycle_num = clock_cycles;
            type_ins = 0;
            int ending_cycle = starting_cycle_num + (type_ins)*ROW_ACCESS_DELAY + COL_ACCESS_DELAY + 1;
            currentInstruction = u;
            curr_ins_num = u.ins_number;
            //starting_cycle_num = clock_cycles;
            col_access_num = u.memory_address - 1024 * row_buffer;
            if (u.type == 0)
            {
                register_values[u.reg] = ROW_BUFFER[col_access_num];
                ins_register[u.reg] = -1;
                row_buffer_updates += type_ins;
                //now print/store output string
            }
            else
            {
                ROW_BUFFER[col_access_num] = u.value;
                row_buffer_updates += type_ins + 1;
                //now print/store output string
            }
            print(ending_cycle);
            clock_cycles = ending_cycle;
            total_queue_size--;
            reset_instruction();
        }
        DRAM_queues[row_buffer] = {};
    }
    for (int num_row = 0; num_row < 1024; num_row++)
    {
        optimizeSw(num_row);
        for (auto u : DRAM_queues[num_row])
        {
            starting_cycle_num = clock_cycles;
            int temp_type = findType(num_row);
            type_ins = temp_type;
            int ending_cycle = starting_cycle_num + (type_ins)*ROW_ACCESS_DELAY + COL_ACCESS_DELAY + 1;
            currentInstruction = u;
            curr_ins_num = u.ins_number;
            if (temp_type == 1)
            {
                activateRow(num_row);
            }
            else if (temp_type == 2)
            {
                writeback_row_num = row_buffer;
                writeBack();
                activateRow(num_row);
            }
            row_buffer = num_row;
            col_access_num = u.memory_address - 1024 * row_buffer;

            if (u.type == 0)
            {
                register_values[u.reg] = ROW_BUFFER[col_access_num];
                ins_register[u.reg] = -1;
                row_buffer_updates += type_ins;
                //now print/store output string
            }
            else
            {
                ROW_BUFFER[col_access_num] = u.value;
                row_buffer_updates += type_ins + 1;
                //now print/store output string
            }
            print(ending_cycle);
            clock_cycles = ending_cycle;
            total_queue_size--;
            reset_instruction();
        }
        DRAM_queues[num_row] = {};
    }
}

void Assign_new_row()
{
    //optimize sw here
    if (curr_ins_num == -1 && row_buffer != -1 && DRAM_queues[row_buffer].size() != 0)
    {
        optimizeSw(row_buffer);
        DRAM_ins temp = DRAM_queues[row_buffer].front();
        currentInstruction = temp;
        DRAM_queues[row_buffer].pop_front();
        curr_ins_num = temp.ins_number;
        type_ins = findType(row_buffer); //type only depends on value in row buffer and current instruction row number
        col_access_num = temp.memory_address - 1024 * row_buffer;
        //assigning col_access_number
        //assign starting_cycle_num at callee loaction
        curr_register = temp.reg;
        load_store = temp.type; //0 for load , 1 for save instruction
        if (temp.type == 0)
        {
            ins_register[curr_register] = temp.memory_address / 1024;
        }
        total_queue_size--; //as an element is popped
        return;
    }
    else
    {
        for (int i = 0; i < 1024; i++)
        {
            if (DRAM_queues[i].size() > 0)
            {
                optimizeSw(i);
                //assign various current instruction parameters, pop from the queue current instruction

                DRAM_ins temp = DRAM_queues[i].front();
                currentInstruction = temp;
                DRAM_queues[i].pop_front();
                curr_ins_num = temp.ins_number;
                type_ins = findType(i); //type only depends on value in row buffer and current instruction row number
                if (type_ins == 2)
                {
                    writeback_row_num = row_buffer;
                    writeBack();
                    activateRow(i);
                }
                col_access_num = temp.memory_address - 1024 * i;
                //assigning col_access_number
                row_buffer = i;
                //assign starting_cycle_num at callee loaction
                curr_register = temp.reg;
                load_store = temp.type; //0 for load , 1 for save instruction
                if (temp.type == 0)
                {
                    ins_register[curr_register] = temp.memory_address / 1024;
                }
                total_queue_size--; //as an element is popped
                break;
            }
        }
    }
}

void parallelAction(struct toPrint curr)
{
    if (curr_ins_num != -1)
    {
        if (starting_cycle_num + (type_ins)*ROW_ACCESS_DELAY + COL_ACCESS_DELAY == clock_cycles)
        {
            //this means an instruction has been completed just now
            //push the required string for printing
            //completed_just =true;
            // an instruction is completed in current cycle, so cannot initiate another
            //update memory/register.

            if (currentInstruction.type == 0)
            {
                //lw instruction, update the register from row buffer
                register_values[currentInstruction.reg] = ROW_BUFFER[col_access_num];
                ins_register[currentInstruction.reg] = -1;
                row_buffer_updates += type_ins;
                curr.DRAMoperation = "Column access " + to_string(col_access_num);
                curr.DRAMchanges = currentInstruction.reg + " = " + to_string(register_values[currentInstruction.reg]);
            }
            else
            {
                //update the row buffer with register value
                curr.DRAMoperation = "Column access " + to_string(col_access_num);
                curr.DRAMchanges = "memory address " + to_string(currentInstruction.memory_address) + "-" + to_string(currentInstruction.memory_address + 3) + "=" + to_string(currentInstruction.value);
                ROW_BUFFER[col_access_num] = register_values[currentInstruction.reg];
                row_buffer_updates += type_ins + 1;
            }
            reset_instruction();
        }
        else
        {
            //execute other steps of that operation in parallel
            int endingCycle = starting_cycle_num + (type_ins)*ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
            if (endingCycle - clock_cycles > ROW_ACCESS_DELAY + COL_ACCESS_DELAY)
            {
                curr.DRAMoperation = "Writeback row " + to_string(writeback_row_num);
                curr.DRAMchanges = "N.A.";
            }
            else if (endingCycle - clock_cycles > COL_ACCESS_DELAY)
            {
                curr.DRAMoperation = "Activated row " + to_string(row_buffer);
                curr.DRAMchanges = "N.A.";
            }
            else
            {
                curr.DRAMoperation = "Column access " + to_string(currentInstruction.memory_address % 1024);
                if (currentInstruction.type == 0)
                    curr.DRAMchanges = currentInstruction.reg + " = " + to_string(register_values[currentInstruction.reg]);
                else
                    curr.DRAMchanges = "memory address " + to_string(currentInstruction.memory_address) + "-" + to_string(currentInstruction.memory_address + 3) + "=" + to_string(currentInstruction.value);
            }
        }
    }
    else
    {
        if (total_queue_size == 0)
        {
            //do nothing
        }
        else
        {
            // assign a non empty row
            Assign_new_row();
            starting_cycle_num = clock_cycles;
            curr.DRAMoperation = "DRAM request issued";
            // now we need to initiate a DRAM request for that, for printing purposes
        }
    }
    prints.push_back(curr);
}

void freeRegister()
{
    struct Instruction current = instructs[PC];
    if (valid_register(current.field_1) && ins_register[current.field_1] != -1 && ins_register[current.field_1] == row_buffer)
    {
        perform_row(current.field_1);
    }
    if (valid_register(current.field_2) && ins_register[current.field_2] != -1 && ins_register[current.field_2] == row_buffer)
    {
        perform_row(current.field_2);
    }
    if (valid_register(current.field_3) && ins_register[current.field_3] != -1 && ins_register[current.field_3] == row_buffer)
    {
        perform_row(current.field_3);
    }
    vector<pair<int, string>> row_order;
    if (valid_register(current.field_1) && ins_register[current.field_1] != -1)
        row_order.push_back({ins_register[current.field_1], current.field_1});
    if (valid_register(current.field_2) && ins_register[current.field_2] != -1)
        row_order.push_back({ins_register[current.field_2], current.field_2});
    if (valid_register(current.field_3) && ins_register[current.field_3] != -1)
        row_order.push_back({ins_register[current.field_3], current.field_3});
    int i = 0;
    while (i < row_order.size())
    {
        perform_row(row_order[i].second);
        i++;
    }
}

toPrint print_normal_operation()
{
    toPrint curr;
    curr.startingCycle = clock_cycles;
    curr.endingCycle = clock_cycles;
    curr.instruction = "N.A";
    curr.RegisterChanged = "N.A.";
    curr.DRAMoperation = "N.A.";
    curr.DRAMchanges = "N.A.";
    return curr;
}

void add()
{
    struct Instruction current = instructs[PC];
    freeRegister();
    clock_cycles++;
    toPrint temp = print_normal_operation();
    if (is_integer(current.field_3))
    {
        register_values[current.field_1] = register_values[current.field_2] + stoi(current.field_3);
    }
    else
    {
        register_values[current.field_1] = register_values[current.field_2] + register_values[current.field_3];
    }
    temp.RegisterChanged = current.field_1 + " = " + to_string(register_values[current.field_1]);
    temp.instruction = findInstruction(current);
    parallelAction(temp);

    PC++;
}

void sub()
{
    struct Instruction current = instructs[PC];
    freeRegister();
    clock_cycles++;
    toPrint temp = print_normal_operation();
    if (is_integer(current.field_3))
    {
        register_values[current.field_1] = register_values[current.field_2] - stoi(current.field_3);
    }
    else
    {
        register_values[current.field_1] = register_values[current.field_2] - register_values[current.field_3];
    }
    temp.RegisterChanged = current.field_1 + " = " + to_string(register_values[current.field_1]);
    temp.instruction = findInstruction(current);
    parallelAction(temp);
    PC++;
}

void mul()
{
    struct Instruction current = instructs[PC];
    freeRegister();
    clock_cycles++;
    toPrint temp = print_normal_operation();
    if (is_integer(current.field_3))
    {
        register_values[current.field_1] = register_values[current.field_2] * stoi(current.field_3);
    }
    else
    {
        register_values[current.field_1] = register_values[current.field_2] * register_values[current.field_3];
    }
    temp.RegisterChanged = current.field_1 + " = " + to_string(register_values[current.field_1]);
    temp.instruction = findInstruction(current);
    parallelAction(temp);
    PC++;
}

void addi()
{
    struct Instruction current = instructs[PC];
    freeRegister();
    clock_cycles++;
    toPrint temp = print_normal_operation();
    register_values[current.field_1] = register_values[current.field_2] + stoi(current.field_3);
    temp.RegisterChanged = current.field_1 + " = " + to_string(register_values[current.field_1]);
    temp.instruction = findInstruction(current);
    parallelAction(temp);
    PC++;
}

void beq()
{
    struct Instruction current = instructs[PC];
    freeRegister();
    clock_cycles++;
    toPrint temp = print_normal_operation();
    if (register_values[current.field_1] == register_values[current.field_2])
    {
        PC = stoi(current.field_3) - 1;
    }
    else
        PC++;

    temp.instruction = findInstruction(current);
    parallelAction(temp);
}

void bne()
{
    struct Instruction current = instructs[PC];
    freeRegister();
    clock_cycles++;
    toPrint temp = print_normal_operation();
    if (register_values[current.field_1] != register_values[current.field_2])
    {
        PC = stoi(current.field_3) - 1;
    }
    else
        PC++;

    temp.instruction = findInstruction(current);
    parallelAction(temp);
}

void slt()
{
    struct Instruction current = instructs[PC];
    freeRegister();
    clock_cycles++;
    toPrint temp = print_normal_operation();
    if (is_integer(current.field_3))
    {
        if (stoi(current.field_3) > register_values[current.field_2])
            register_values[current.field_1] = 1;
        else
            register_values[current.field_1] = 0;
    }
    else
    {
        if (register_values[current.field_3] > register_values[current.field_2])
            register_values[current.field_1] = 1;
        else
            register_values[current.field_1] = 0;
    }
    PC++;
    temp.RegisterChanged = current.field_1 + " = " + to_string(register_values[current.field_1]);
    temp.instruction = findInstruction(current);
    parallelAction(temp);
}

void j()
{
    struct Instruction current = instructs[PC];
    PC = stoi(current.field_1) - 1;
    clock_cycles++;
    toPrint temp = print_normal_operation();
    temp.instruction = findInstruction(current);
    parallelAction(temp);
}

void lw()
{

    struct Instruction current = instructs[PC];
    //freeRegister();
    optimizeLw();
    int address = register_values[current.field_3] + stoi(current.field_2);
    DRAM_ins temp;
    temp.ins_number = PC;
    temp.type = 0;
    temp.memory_address = address;
    temp.reg = current.field_1;
    if (curr_ins_num == -1)
    {
        clock_cycles++;
        toPrint curr;
        curr.startingCycle = clock_cycles;
        curr.endingCycle = clock_cycles;
        curr.instruction = "N.A";
        curr.RegisterChanged = "N.A";
        curr.DRAMoperation = "DRAM request issued";
        curr.DRAMchanges = "N.A";
        prints.push_back(curr);
        //we may have to initiate a new DRAM request from some instruction present inside the queue
        if (total_queue_size != 0)
        {
            // assign a non empty row
            Assign_new_row();
            starting_cycle_num = clock_cycles;
            ins_register[current.field_1] = address / 1024;
            DRAM_queues[address / 1024].push_back(temp);
            total_queue_size++;
            // now we need to initiate a DRAM request for that, for printing purposes
        }
        else
        {
            currentInstruction = temp;
            ins_register[current.field_1] = address / 1024;
            curr_ins_num = PC;
            starting_cycle_num = clock_cycles;
            type_ins = findType(address / 1024);
            //copy contents from memory to row buffer
            if (type_ins == 2)
            {
                writeback_row_num = row_buffer;
                writeBack();
            }
            row_buffer = address / 1024;
            col_access_num = address - 1024 * row_buffer;
            if (type_ins != 0)
                activateRow(row_buffer);
        }
    }
    else
    {
        ins_register[current.field_1] = address / 1024;
        DRAM_queues[address / 1024].push_back(temp);
        total_queue_size++;
    }
    PC++;
}

void sw()
{
    struct Instruction current = instructs[PC];
    freeRegister();

    int address = register_values[current.field_3] + stoi(current.field_2);
    DRAM_ins temp;
    temp.ins_number = PC;
    temp.type = 1;
    temp.memory_address = address;
    temp.reg = current.field_1;
    temp.value = register_values[current.field_1];
    if (curr_ins_num == -1)
    {
        clock_cycles++;
        toPrint curr;
        curr.startingCycle = clock_cycles;
        curr.endingCycle = clock_cycles;
        curr.instruction = "N.A";
        curr.RegisterChanged = "N.A";
        curr.DRAMoperation = "DRAM request issued";
        curr.DRAMchanges = "N.A";
        prints.push_back(curr);
        if (total_queue_size != 0)
        {
            // assign a non empty row
            Assign_new_row();
            starting_cycle_num = clock_cycles;
            ins_register[current.field_1] = address / 1024;
            DRAM_queues[address / 1024].push_back(temp);
            total_queue_size++;
            // now we need to initiate a DRAM request for that, for printing purposes
        }
        else
        {
            currentInstruction = temp;
            curr_ins_num = PC;
            starting_cycle_num = clock_cycles;
            type_ins = findType(address / 1024);
            //copy contents from memory to row buffer
            if (type_ins == 2)
            {
                writeback_row_num = row_buffer;
                writeBack();
            }
            row_buffer = address / 1024;
            col_access_num = address - 1024 * row_buffer;
            if (type_ins != 0)
                activateRow(row_buffer);
        }
    }
    else
    {
        DRAM_queues[address / 1024].push_back(temp);
        total_queue_size++;
    }
    PC++;
}

void map_register_numbers()
{
    //maps each register to a unique number between 0-31 inclusive
    register_numbers[0] = "$r0";
    register_numbers[1] = "$at";
    register_numbers[2] = "$v0";
    register_numbers[3] = "$v1";
    for (int i = 4; i <= 7; i++)
    {
        register_numbers[i] = "$a" + to_string(i - 4);
    }
    for (int i = 8; i <= 15; i++)
    {
        register_numbers[i] = "$t" + to_string(i - 8);
    }
    for (int i = 16; i <= 23; i++)
    {
        register_numbers[i] = "$s" + to_string(i - 16);
    }
    register_numbers[24] = "$t8";
    register_numbers[25] = "$t9";
    register_numbers[26] = "$k0";
    register_numbers[27] = "$k1";
    register_numbers[28] = "$gp";
    register_numbers[29] = "$sp";
    register_numbers[30] = "$s8";
    register_numbers[31] = "$ra";
}

void initialise_Registers()
{
    //initialises all the registers
    for (int i = 0; i < 32; i++)
    {
        register_values[register_numbers[i]] = 0;
        ins_register[register_numbers[i]] = -1;
    }
    register_values["$sp"] = 2147479548;
}

int SearchForRegister(int starting_index, int ending_index, string file_string)
{
    //this is a helper function which searches for a register from starting index and returns the starting point of it
    int start = -1;
    for (int j = starting_index; j <= ending_index; j++)
    {
        if (file_string[j] == ' ' || file_string[j] == '\t')
        {
            continue;
        }
        else
        {
            start = j;
            break;
        }
    }
    if (start == -1 || start + 2 > ending_index)
    {
        return -1;
    }
    if (!valid_register(file_string.substr(start, 3)))
    {
        return -1;
    }
    return start; //else found a valid register
}
int SearchForCharacter(int starting_index, int ending_index, string file_string, char Matching)
{
    //returns the position of Matching if it is the first non-whitespace character to be found, -1 otherwise
    int start = -1;
    for (int j = starting_index; j <= ending_index; j++)
    {
        if (file_string[j] == ' ' || file_string[j] == '\t')
        {
            continue;
        }
        else if (file_string[j] == Matching)
        {
            return j;
        }
        else
        {
            return -1;
        }
    }
    return -1; //if no character found except whitespace
}
pair<int, int> SearchForInteger(int starting_index, int ending_index, string file_string)
{
    //returns the starting and ending index of integer if found
    int start = -1;
    int end = -1;
    bool firstMinus = true;
    for (int j = starting_index; j <= ending_index; j++)
    {
        if ((file_string[j] == ' ' || file_string[j] == '\t') && start == -1)
        {
            continue;
        } //removing the starting spaces and tabs}
        if (isdigit(file_string[j]) || (file_string[j] == '-' && firstMinus))
        {
            firstMinus = false;
            if (start == -1)
            {
                start = j;
                end = j;
            }
            else
            {
                end = j;
            }
        }
        else
        {
            return {start, end};
        }
    }
    return {start, end};
}
string Match_Instruction(int start, int end, string file_string)
{
    //returns the matched instruction
    if (start + 3 <= end)
    {
        string ins = file_string.substr(start, 4);
        if (ins == "addi")
        {
            return ins;
        }
    }
    if (start + 2 <= end)
    {
        string ins = file_string.substr(start, 3);
        if (ins == "add" || ins == "sub" || ins == "mul" || ins == "slt" || ins == "beq" || ins == "bne")
        {
            return ins;
        }
    }
    if (start + 1 <= end)
    {
        string ins = file_string.substr(start, 2);
        if (ins == "lw" || ins == "sw")
        {
            return ins;
        }
    }
    if (start <= end)
    {
        string ins = file_string.substr(start, 1);
        if (ins == "j")
        {
            return ins;
        }
    }
    return ""; //when no valid instruction found
}

void process()
{
    int execution_no = 1;
    int op_count[11] = {0};
    int ins_count[instructs.size()];
    for (int j = 0; j < instructs.size(); j++)
    {
        ins_count[j] = 0;
    }
    while (PC < instructs.size())
    {
        //completed_just = false;
        ins_count[PC]++;
        struct Instruction current = instructs[PC];
        int action = operation[current.name];
        op_count[action]++;
        //all clock cycles are incremented inside the functions
        switch (action)
        {
        case 1:
            add();
            break;
        case 2:
            sub();
            break;
        case 3:
            mul();
            break;
        case 4:
            beq();
            break;
        case 5:
            bne();
            break;
        case 6:
            slt();
            break;
        case 7:
            j();
            break;
        case 8:
            lw();
            break;
        case 9:
            sw();
            break;
        case 10:
            addi();
            break;
        }
    }
    if (total_queue_size > 0 || curr_ins_num != -1)
    {
        complete_remaining(); //this function simply completes the remaining instructions starting from the current cycle
    }
}

//handle the case when integer is beyond instruction memory at execution time
void Create_structs(string file_string)
{
    int i = 0;
    bool instruction_found = false;
    // each line can contain atmost one instruction
    while (i < file_string.size())
    {
        if (file_string[i] == ' ' || file_string[i] == '\t')
        {
            i++;
            continue;
        }
        else
        {
            if (instruction_found)
            {
                validFile = false;
                return;
            } //if we have already found an instruction and a character appears, file is invalid
            string ins = Match_Instruction(i, file_string.size() - 1, file_string);
            if (ins == "")
            { //invalid matching
                validFile = false;
                return;
            }
            if (ins == "add" || ins == "sub" || ins == "mul" || ins == "slt" || ins == "beq" || ins == "bne" || ins == "addi")
            {
                //now, there must be three registers ahead, delimited by comma
                int reg1_start;
                if (ins == "addi")
                {
                    reg1_start = SearchForRegister(i + 4, file_string.size() - 1, file_string);
                }
                else
                {
                    reg1_start = SearchForRegister(i + 3, file_string.size() - 1, file_string);
                }
                if (reg1_start == -1)
                {
                    validFile = false;
                    return;
                }
                string R1 = file_string.substr(reg1_start, 3);
                //now first register has been found, it must be followed by a comma and there can be 0 or more whitespaces in between
                int comma1Pos = SearchForCharacter(reg1_start + 3, file_string.size() - 1, file_string, ',');
                if (comma1Pos == -1)
                {
                    validFile = false;
                    return;
                }
                int reg2_start = SearchForRegister(comma1Pos + 1, file_string.size() - 1, file_string);
                if (reg2_start == -1)
                {
                    validFile = false;
                    return;
                }
                string R2 = file_string.substr(reg2_start, 3);
                int comma2Pos = SearchForCharacter(reg2_start + 3, file_string.size() - 1, file_string, ',');
                if (comma2Pos == -1)
                {
                    validFile = false;
                    return;
                }
                int reg3_start = SearchForRegister(comma2Pos + 1, file_string.size() - 1, file_string);
                //instead of third register, we can also have an integer value
                pair<int, int> integer_indices = SearchForInteger(comma2Pos + 1, file_string.size() - 1, file_string);
                int index_looped;
                if (reg3_start == -1 && integer_indices.first == -1)
                {
                    validFile = false;
                    return;
                } //neither an integer nor a string
                string R3;
                if (reg3_start != -1)
                {
                    if (ins != "beq" && ins != "bne" && ins != "addi")
                    { //is a register and instruction is not bne,beq or addi
                        R3 = file_string.substr(reg3_start, 3);
                        index_looped = reg3_start + 3;
                    }
                    else
                    { //beq,bne and addi must have the third argument as an integer
                        validFile = false;
                        return;
                    }
                }
                else
                {
                    if (ins == "beq" || ins == "bne")
                    {
                        if (file_string[integer_indices.first] == '-')
                        {
                            validFile = false;
                            return;
                        }
                        else
                        {
                            R3 = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first + 1);
                            index_looped = integer_indices.second + 1;
                        }
                    }
                    else
                    {
                        R3 = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first + 1);
                        index_looped = integer_indices.second + 1;
                    }
                }
                struct Instruction new_instr;
                new_instr.name = ins;
                new_instr.field_1 = R1;
                new_instr.field_2 = R2;
                new_instr.field_3 = R3;
                i = index_looped; //increment i
                instruction_found = true;
                instructs.push_back(new_instr);
                continue;
            }
            else if (ins == "j")
            {
                pair<int, int> integer_indices = SearchForInteger(i + 1, file_string.size() - 1, file_string);
                if (integer_indices.first == -1 || file_string[integer_indices.first] == '-')
                {
                    validFile = false;
                    return;
                }
                struct Instruction new_instr;
                new_instr.name = ins;
                new_instr.field_1 = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first + 1);
                new_instr.field_2 = "";
                new_instr.field_3 = "";
                int index_looped = integer_indices.second + 1;
                i = index_looped;
                instruction_found = true;
                instructs.push_back(new_instr);
            }
            else if (ins == "lw" || ins == "sw")
            {
                //this has the format lw $t0, offset($register_name)
                // first of all search for the first register
                int reg1_start = SearchForRegister(i + 2, file_string.size() - 1, file_string);
                if (reg1_start == -1)
                {
                    validFile = false;
                    return;
                }
                string R1 = file_string.substr(reg1_start, 3);
                //now we will search for a comma and match it
                int commaPos = SearchForCharacter(reg1_start + 3, file_string.size() - 1, file_string, ',');
                if (commaPos == -1)
                {
                    validFile = false;
                    return;
                }
                // now we will search for an integer offset and match it
                pair<int, int> integer_indices = SearchForInteger(commaPos + 1, file_string.size() - 1, file_string);
                if (integer_indices.first == -1)
                {
                    validFile = false;
                    return;
                }
                string offset = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first + 1);
                // now we will match Left parenthesis
                int lparenPos = SearchForCharacter(integer_indices.second + 1, file_string.size() - 1, file_string, '(');
                if (lparenPos == -1)
                {
                    validFile = false;
                    return;
                }
                //now we will match a register
                int reg2_start = SearchForRegister(lparenPos + 1, file_string.size() - 1, file_string);
                if (reg2_start == -1)
                {
                    validFile = false;
                    return;
                }
                string R2 = file_string.substr(reg2_start, 3);
                // now we will match the right parenthesis
                int rparenPos = SearchForCharacter(reg2_start + 3, file_string.size() - 1, file_string, ')');
                if (rparenPos == -1)
                {
                    validFile = false;
                    return;
                }
                struct Instruction new_instr;
                new_instr.name = ins;
                new_instr.field_1 = R1;
                new_instr.field_2 = offset;
                new_instr.field_3 = R2;
                i = rparenPos + 1;
                instruction_found = true;
                instructs.push_back(new_instr);
            }
        }
    }
}

void print_registers()
{
    for (int i = 0; i < 32; i++)
    {
        if (i == 0)
        {
            cout << register_numbers[i] << " " << std::hex << (register_values[register_numbers[i]]);
        }
        else
        {
            cout << ", " << register_numbers[i] << " " << std::hex << (register_values[register_numbers[i]]);
        }
    }
    cout << "\n";
}
void Number_of_times(int ins_count[], int op_count[])
{
    cout << "The number of times each instruction was executed is given below : \n"
         << endl;
    for (int i = 0; i < instructs.size(); i++)
    {
        cout << "Instruction no: " << std::dec << i + 1 << " was executed " << std::dec << ins_count[i] << " times." << endl;
    }

    cout << "\nThe number of times each type of instruction was executed is given below : \n"
         << endl;
    for (int i = 1; i < 11; i++)
    {
        cout << "Operation " << intTostr_operation[i] << " was executed " << std::dec << op_count[i] << " times." << endl;
    }
    cout << "\nThe total number of clock cycles elapsed is : " << std::dec << clock_cycles << "\n\n";
}
void PrintData()
{
    cout << "\nTotal number of cycles: " << clock_cycles << endl;
    cout << "Total number of row buffer updates(loading new row into buffer or modifying row buffer): " << row_buffer_updates << endl;
    cout << "\nMemory content at the end of the execution:\n\n";
    for (int i = 0; i < (1 << 20); i += 4)
    {
        if (memory[i] != 0)
        {
            cout << i << "-" << i + 3 << ": " << memory[i] << endl;
        }
    }
    cout << endl;
    cout << "Every cycle description\n\n";
    cout << left << setw(18) << "Cycle numbers";
    cout << left << setw(30) << "Instructions";
    cout << left << setw(20) << "Register changed";
    cout << left << setw(25) << "DRAM operations";
    cout << left << setw(30) << "DRAM changes";
    cout << endl;
    for (auto u : prints)
    {
        string cycle;
        if (u.startingCycle == u.endingCycle)
        {
            cycle = "cycle " + to_string(u.startingCycle) + ":";
        }
        else
        {
            cycle = "cycle " + to_string(u.startingCycle) + "-" + to_string(u.endingCycle) + ":";
        }
        cout << left << setw(18) << cycle;
        cout << left << setw(30) << u.instruction;
        cout << left << setw(20) << u.RegisterChanged;
        cout << left << setw(25) << u.DRAMoperation;
        cout << left << setw(30) << u.DRAMchanges;
        cout << endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        cout << "Invalid arguments\n";
        return -1;
    }
    //Taking file name, row and column access delays from the command line
    string file_name = argv[1];
    ROW_ACCESS_DELAY = stoi(argv[2]);
    COL_ACCESS_DELAY = stoi(argv[3]);
    if (ROW_ACCESS_DELAY < 1 || COL_ACCESS_DELAY < 1)
    {
        cout << "Invalid arguments\n";
        return -1;
    }
    ifstream file(file_name);
    string current_line;
    map_register_numbers();
    initialise_Registers();
    validFile = true;
    infinite_loop = false;
    while (getline(file, current_line))
    {
        Create_structs(current_line);
    }
    map_operations();
    if (!validFile)
    {
        cout << "Invalid MIPS program" << endl;
        return -1;
    }
    // perform_operations(false);

    // if (infinite_loop)
    // {
    //     cout << "Time limit exceeded !" << endl;
    //     return -1;
    // }
    if (!validFile)
    {
        cout << "Invalid MIPS program" << endl; //due to wrong lw and sw addresses
        return -1;
    }
    PC = 0;
    clock_cycles = 0;
    initialise_Registers();
    process();
    PrintData();
    /*each instruction occupies 4 bytes. So, we will first of all maintain an array of instructions
     to get the instruction starting at memory address i (in the form of a struct). Rest of the memory is used in RAM*/
    /*so memory stores instructions (as structs here) and data as integers in decimal format*/

    return 0;
}
