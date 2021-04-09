#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
using namespace std;
map<string, int> labels;
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

map<string, int> getLabels()
{
    return labels;
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
bool validLabel(string temp)
{
    if (temp.size() == 0)
    {
        return false;
    }
    int first_char = (int)temp[0];
    if (!(first_char == 95 || (first_char >= 65 && first_char <= 90) || (first_char >= 97 && first_char <= 122)))
    {
        return false;
    }
    //first letter can be underscore or a letter
    for (int i = 1; i < temp.size(); i++)
    {
        if (!(first_char == 95 || (first_char >= 65 && first_char <= 90) || (first_char >= 97 && first_char <= 122) || (first_char >= 48 && first_char <= 57)))
        {
            //rest can be letters, underscores or digits
            return false;
        }
    }
    return true;
}

pair<bool, string> ifLabel(string ins)
{
    string temp = "";
    int i = 0;
    while (i < ins.size() && (ins[i] == ' ' || ins[i] == '\t'))
        i++;
    while (i < ins.size() && ins[i] != ':' && ins[i] != ' ' && ins[i] != '\t')
    {
        temp += ins[i];
        i++;
    }
    int j = i + 1;
    while (j < ins.size())
    {
        if (ins[j] != ' ' && ins[j] != '\t')
            return {false, ""};
        j++;
    }
    if (i < ins.size() && ins[i] == ':' && validLabel(temp))
        return {true, temp};
    else
        return {false, ""};
}

string removeComments(string file_string)
{
    int i = 0;
    string temp = "";
    while (i < file_string.size() && file_string[i] != '#')
    {
        temp += file_string[i];
        i++;
    }
    return temp;
}

string changeZero(string file_string)
{
    string temp = "";
    if (file_string.size() < 4)
        return file_string;
    else
    {
        int i = 0;
        while (i < file_string.size())
        {
            if (file_string[i] != 'z')
            {
                temp += file_string[i];
                i++;
            }
            else
            {
                if (i + 3 < file_string.size() && file_string[i + 1] == 'e' && file_string[i + 2] == 'r' && file_string[i + 3] == 'o')
                {
                    temp += "r0";
                    i += 4;
                }
                else
                {
                    temp += file_string[i];
                    i++;
                }
            }
        }
        return temp;
    }
}

pair<bool, pair<string, int>> findLabel(string file_string, int pos)
{
    string temp1 = "";
    int i = pos;
    while (i < file_string.size() && (file_string[i] == ' ' || file_string[i] == '\t'))
    {
        i++;
    }
    while (i < file_string.size() && file_string[i] != ' ' && file_string[i] != '\t')
    {
        temp1 += file_string[i];
        i++;
    }
    //cout<<temp1<<endl;
    if (validLabel(temp1))
        return {true, {temp1, i}};
    else
        return {false, {"", i}};
}

bool ifEmpty(string temp)
{
    for (int i = 0; i < temp.size(); i++)
    {
        if (temp[i] != ' ' && temp[i] != '\t')
        {
            return false;
        }
    }
    return true;
}

pair<int, Instruction> Create_structs(string file_string, map<string, int> register_values, int pcValue)
{
    int i = 0;
    bool instruction_found = false;
    struct Instruction new_instr;
    // each line can contain atmost one instruction
    new_instr.name = ""; //default name
    file_string = removeComments(file_string);
    if (ifEmpty(file_string))
    {
        return {2, new_instr};
    }
    pair<bool, string> lbl = ifLabel(file_string);
    if (lbl.first)
    {
        labels[lbl.second] = pcValue + 1;
        // cout<<lbl.second<<" "<<labels[lbl.second];
        return {2, new_instr};
    }
    file_string = changeZero(file_string);
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
                return {0, new_instr};
            } //if we have already found an instruction and a character appears, file is invalid
            string ins = Match_Instruction(i, file_string.size() - 1, file_string);
            if (ins == "")
            { //invalid matching
                //validFile = false;
                return {0, new_instr};
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
                    return {0, new_instr};
                }
                string R1 = file_string.substr(reg1_start, 3);
                //now first register has been found, it must be followed by a comma and there can be 0 or more whitespaces in between
                int comma1Pos = SearchForCharacter(reg1_start + 3, file_string.size() - 1, file_string, ',');
                if (comma1Pos == -1)
                {
                    //validFile = false;
                    return {0, new_instr};
                }
                int reg2_start = SearchForRegister(comma1Pos + 1, file_string.size() - 1, file_string, register_values);
                if (reg2_start == -1)
                {
                    //validFile = false;
                    return {0, new_instr};
                }
                string R2 = file_string.substr(reg2_start, 3);
                int comma2Pos = SearchForCharacter(reg2_start + 3, file_string.size() - 1, file_string, ',');
                if (comma2Pos == -1)
                {
                    //validFile = false;
                    return {0, new_instr};
                }
                int reg3_start = SearchForRegister(comma2Pos + 1, file_string.size() - 1, file_string, register_values);
                //instead of third register, we can also have an integer value
                pair<int, int> integer_indices = SearchForInteger(comma2Pos + 1, file_string.size() - 1, file_string);
                pair<bool, pair<string, int>> labeldata = findLabel(file_string, comma2Pos + 1);
                //cout << labeldata.second.first << endl;
                int index_looped;
                if (reg3_start == -1 && integer_indices.first == -1 && !labeldata.first)
                {
                    //validFile = false;
                    return {0, new_instr};
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
                        return {0, new_instr};
                    }
                }
                else if (integer_indices.first != -1)
                {
                    if (ins == "addi" || ins == "add" || ins == "sub" || ins == "mul" || ins == "slt")
                    {
                        R3 = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first + 1);
                        index_looped = integer_indices.second + 1;
                    }
                    else
                    {

                        return {0, new_instr};
                    }
                }
                else
                {
                    if (ins == "bne" || ins == "beq")
                    {
                        R3 = labeldata.second.first;
                        index_looped = labeldata.second.second;
                        //cout<<R3<<endl;
                    }
                    else
                    {
                        return {0, new_instr};
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
                // pair<int, int> integer_indices = SearchForInteger(i + 1, file_string.size() - 1, file_string);
                // if (integer_indices.first == -1 || file_string[integer_indices.first] == '-')
                // {
                //     //validFile = false;
                //     return {false, new_instr};
                // }

                pair<bool, pair<string, int>> lblTOpc = findLabel(file_string, i + 1);
                if (!lblTOpc.first)
                {
                    return {0, new_instr};
                }
                new_instr.name = ins;
                new_instr.field_1 = lblTOpc.second.first;
                new_instr.field_2 = "";
                new_instr.field_3 = "";
                int index_looped = lblTOpc.second.second;
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
                    return {0, new_instr};
                }
                string R1 = file_string.substr(reg1_start, 3);
                //now we will search for a comma and match it
                int commaPos = SearchForCharacter(reg1_start + 3, file_string.size() - 1, file_string, ',');
                if (commaPos == -1)
                {
                    //validFile = false;
                    return {0, new_instr};
                }
                // now we will search for an integer offset and match it
                pair<int, int> integer_indices = SearchForInteger(commaPos + 1, file_string.size() - 1, file_string);
                if (integer_indices.first == -1)
                {
                    //validFile = false;
                    return {0, new_instr};
                }
                string offset = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first + 1);
                // now we will match Left parenthesis
                int lparenPos = SearchForCharacter(integer_indices.second + 1, file_string.size() - 1, file_string, '(');
                if (lparenPos == -1)
                {
                    //validFile = false;
                    return {0, new_instr};
                }
                //now we will match a register
                int reg2_start = SearchForRegister(lparenPos + 1, file_string.size() - 1, file_string, register_values);
                if (reg2_start == -1)
                {
                    //validFile = false;
                    return {0, new_instr};
                }
                string R2 = file_string.substr(reg2_start, 3);
                // now we will match the right parenthesis
                int rparenPos = SearchForCharacter(reg2_start + 3, file_string.size() - 1, file_string, ')');
                if (rparenPos == -1)
                {
                    //validFile = false;
                    return {0, new_instr};
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
        return {0, new_instr};
    }
    return {1, new_instr};
}
