#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
using namespace std;
struct Instruction
{
    string name;
    string field_1 = "";
    string field_2 = "";
    string field_3 = "";
};
bool valid_register(string R, map<string, int> register_values)
{
    return register_values.find(R) != register_values.end();
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
int SearchForRegister(int starting_index, int ending_index, string file_string, map<string, int> register_values)
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
    if (!valid_register(file_string.substr(start, 3), register_values))
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
//handle the case when integer is beyond instruction memory at execution time, and case of r0
pair<bool, Instruction> Create_structs(string file_string, map<string, int> register_values)
{
    int i = 0;
    bool instruction_found = false;
    struct Instruction new_instr;
    // each line can contain atmost one instruction
    new_instr.name = ""; //default name
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
                //validFile = false;
                return {false, new_instr};
            } //if we have already found an instruction and a character appears, file is invalid
            string ins = Match_Instruction(i, file_string.size() - 1, file_string);
            if (ins == "")
            { //invalid matching
                //validFile = false;
                return {false, new_instr};
            }
            if (ins == "add" || ins == "sub" || ins == "mul" || ins == "slt" || ins == "beq" || ins == "bne" || ins == "addi")
            {
                //now, there must be three registers ahead, delimited by comma
                int reg1_start;
                if (ins == "addi")
                {
                    reg1_start = SearchForRegister(i + 4, file_string.size() - 1, file_string, register_values);
                }
                else
                {
                    reg1_start = SearchForRegister(i + 3, file_string.size() - 1, file_string, register_values);
                }
                if (reg1_start == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                string R1 = file_string.substr(reg1_start, 3);
                //now first register has been found, it must be followed by a comma and there can be 0 or more whitespaces in between
                int comma1Pos = SearchForCharacter(reg1_start + 3, file_string.size() - 1, file_string, ',');
                if (comma1Pos == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                int reg2_start = SearchForRegister(comma1Pos + 1, file_string.size() - 1, file_string, register_values);
                if (reg2_start == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                string R2 = file_string.substr(reg2_start, 3);
                int comma2Pos = SearchForCharacter(reg2_start + 3, file_string.size() - 1, file_string, ',');
                if (comma2Pos == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                int reg3_start = SearchForRegister(comma2Pos + 1, file_string.size() - 1, file_string, register_values);
                //instead of third register, we can also have an integer value
                pair<int, int> integer_indices = SearchForInteger(comma2Pos + 1, file_string.size() - 1, file_string);
                int index_looped;
                if (reg3_start == -1 && integer_indices.first == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
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
                        //validFile = false;
                        return {false, new_instr};
                    }
                }
                else
                {
                    if (ins == "beq" || ins == "bne")
                    {
                        if (file_string[integer_indices.first] == '-')
                        {
                            //validFile = false;
                            return {false, new_instr};
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
                new_instr.name = ins;
                new_instr.field_1 = R1;
                new_instr.field_2 = R2;
                new_instr.field_3 = R3;
                i = index_looped; //increment i
                instruction_found = true;
                //instructs.push_back(new_instr);
                continue;
            }
            else if (ins == "j")
            {
                pair<int, int> integer_indices = SearchForInteger(i + 1, file_string.size() - 1, file_string);
                if (integer_indices.first == -1 || file_string[integer_indices.first] == '-')
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                new_instr.name = ins;
                new_instr.field_1 = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first + 1);
                new_instr.field_2 = "";
                new_instr.field_3 = "";
                int index_looped = integer_indices.second + 1;
                i = index_looped;
                instruction_found = true;
                //instructs.push_back(new_instr);
            }
            else if (ins == "lw" || ins == "sw")
            {
                //this has the format lw $t0, offset($register_name)
                // first of all search for the first register
                int reg1_start = SearchForRegister(i + 2, file_string.size() - 1, file_string, register_values);
                if (reg1_start == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                string R1 = file_string.substr(reg1_start, 3);
                //now we will search for a comma and match it
                int commaPos = SearchForCharacter(reg1_start + 3, file_string.size() - 1, file_string, ',');
                if (commaPos == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                // now we will search for an integer offset and match it
                pair<int, int> integer_indices = SearchForInteger(commaPos + 1, file_string.size() - 1, file_string);
                if (integer_indices.first == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                string offset = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first + 1);
                // now we will match Left parenthesis
                int lparenPos = SearchForCharacter(integer_indices.second + 1, file_string.size() - 1, file_string, '(');
                if (lparenPos == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                //now we will match a register
                int reg2_start = SearchForRegister(lparenPos + 1, file_string.size() - 1, file_string, register_values);
                if (reg2_start == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                string R2 = file_string.substr(reg2_start, 3);
                // now we will match the right parenthesis
                int rparenPos = SearchForCharacter(reg2_start + 3, file_string.size() - 1, file_string, ')');
                if (rparenPos == -1)
                {
                    //validFile = false;
                    return {false, new_instr};
                }
                new_instr.name = ins;
                new_instr.field_1 = R1;
                new_instr.field_2 = offset;
                new_instr.field_3 = R2;
                i = rparenPos + 1;
                instruction_found = true;
                //instructs.push_back(new_instr);
            }
        }
    }
    if (new_instr.name == "")
    {
        return {false, new_instr};
    }
    return {true, new_instr};
}
int sim_cycles = 0;
int maxClockCycles = 100000;
bool validFile = true;
int memory_sim[(1 << 20)] = {0};
int PC;
map<string, int> register_values_sim; //stores the value of data stored in each register
void initialise_Registers_simulator(map<int, string> register_numbers)
{
    //initialises all the registers
    for (int i = 0; i < 32; i++)
    {
        register_values_sim[register_numbers[i]] = 0;
    }
}
void add_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    if (current.field_1 == "$r0")
    {
        //do nothing
    }
    else if (is_integer(current.field_3))
    {
        register_values_sim[current.field_1] = register_values_sim[current.field_2] + stoi(current.field_3);
    }
    else
    {
        register_values_sim[current.field_1] = register_values_sim[current.field_2] + register_values_sim[current.field_3];
    }
    PC++;
}
void sub_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    if (current.field_1 == "$r0")
    {
        //do nothing
    }
    else if (is_integer(current.field_3))
    {
        register_values_sim[current.field_1] = register_values_sim[current.field_2] - stoi(current.field_3);
    }
    else
    {
        register_values_sim[current.field_1] = register_values_sim[current.field_2] - register_values_sim[current.field_3];
    }
    PC++;
}
void mul_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    if (current.field_1 == "$r0")
    {
        //do nothing
    }
    else if (is_integer(current.field_3))
    {
        register_values_sim[current.field_1] = register_values_sim[current.field_2] * stoi(current.field_3);
    }
    else
    {
        register_values_sim[current.field_1] = register_values_sim[current.field_2] * register_values_sim[current.field_3];
    }
    PC++;
}
void addi_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    if (current.field_1 == "$r0")
    {
        //do nothing
    }
    else
    {
        register_values_sim[current.field_1] = register_values_sim[current.field_2] + stoi(current.field_3);
    }
    PC++;
}
void beq_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    if (register_values_sim[current.field_1] == register_values_sim[current.field_2])
    {
        PC = stoi(current.field_3) - 1;
    }
    else
        PC++;
}
void bne_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    if (register_values_sim[current.field_1] != register_values_sim[current.field_2])
    {
        PC = stoi(current.field_3) - 1;
    }
    else
        PC++;
}
void slt_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    if (current.field_1 == "$r0")
    {
        // dp nothing
    }
    else if (is_integer(current.field_3))
    {
        if (stoi(current.field_3) > register_values_sim[current.field_2])
            register_values_sim[current.field_1] = 1;
        else
            register_values_sim[current.field_1] = 0;
    }
    else
    {
        if (register_values_sim[current.field_3] > register_values_sim[current.field_2])
            register_values_sim[current.field_1] = 1;
        else
            register_values_sim[current.field_1] = 0;
    }
    PC++;
}
void j_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    PC = stoi(current.field_1) - 1;
}
void lw_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    int address = register_values_sim[current.field_3] + stoi(current.field_2);
    if ((address >= (1 << (20)) || address < 4 * instructs.size()) || address % 4 != 0)
    {
        validFile = false;
        return;
    }
    register_values_sim[current.field_1] = memory_sim[address];
    PC++;
}
void sw_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    int address = register_values_sim[current.field_3] + stoi(current.field_2);
    if ((address >= (1 << (20)) || address < 4 * instructs.size()) || address % 4 != 0)
    {
        validFile = false;
        return;
    }
    memory_sim[address] = register_values_sim[current.field_1];
    PC++;
}
pair<bool, bool> simulate(vector<Instruction> instructs_sim, map<string, int> operation_sim, map<int, string> register_numbers_sim)
{
    PC = 0;
    initialise_Registers_simulator(register_numbers_sim);
    while (PC < instructs_sim.size())
    {
        struct Instruction curr = instructs_sim[PC];
        int action = operation_sim[curr.name];
        sim_cycles++;
        if (sim_cycles > maxClockCycles)
        {
            return {true, false}; //true in first means infinite loop found, in second means invalid file
        }
        switch (action)
        {
        case 1:
            add_sim(instructs_sim);
            break;
        case 2:
            sub_sim(instructs_sim);
            break;
        case 3:
            mul_sim(instructs_sim);
            break;
        case 4:
            beq_sim(instructs_sim);
            break;
        case 5:
            bne_sim(instructs_sim);
            break;
        case 6:
            slt_sim(instructs_sim);
            break;
        case 7:
            j_sim(instructs_sim);
            break;
        case 8:
            lw_sim(instructs_sim);
            break;
        case 9:
            sw_sim(instructs_sim);
            break;
        case 10:
            addi_sim(instructs_sim);
            break;
        }
        if (!validFile)
        {
            return {false, true};
        }
    }
    return {false, false};
}
