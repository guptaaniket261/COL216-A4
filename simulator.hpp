#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include "parser.hpp"
using namespace std;
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
        if(labels.find(current.field_3) == labels.end()){
            validFile = false;
            return;
        }
        PC = labels[current.field_3] - 1;
    }
    else
        PC++;
}
void bne_sim(vector<Instruction> instructs)
{
    struct Instruction current = instructs[PC];
    if (register_values_sim[current.field_1] != register_values_sim[current.field_2])
    {
        if(labels.find(current.field_3) == labels.end()){
            validFile = false;
            return;
        }
        PC = labels[current.field_3] - 1;
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
    if(labels.find(current.field_1) == labels.end()){
        validFile = false;
        return;
    }
    PC = labels[current.field_1] - 1;
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