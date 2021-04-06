#include<iostream>
#include<fstream>
#include <sstream>
#include<vector>
#include<map>
using namespace std;
// function to convert a decimal number to a hexadecimal number
long long int maxClockCycles = 10000;
bool infinite_loop =false;
string HexaNumber(int n)
{
    string ans = "";
    int rem;
    while (n != 0) {
        rem = n % 16;
        // check if rem < 10
        if (rem < 10) {
            ans+=char(rem+48);
        }
        else {
            ans+=char(rem+55);
        }
        n/=16;
    }
    if(ans=="")return "0";
    else return ans;
}

struct Instruction
{
    string name;
    string field_1;
    string field_2;
    string field_3;
};
vector <string> words;              //stores the entire file as a vector of strings
map<int,string> register_numbers;  //maps each number in 0-31 to a register
map<string,int> register_values; //stores the value of data stored in each register
bool validFile = true;          //will be false if file is invalid at any point
vector<Instruction>instructs;   //stores instructions as structs
int memory[(1<<20)]={0}; //memory used to store the data
int PC=0;               // PC pointer, points to the next instruction
int clock_cycles =0;
map<string,int> operation;
map<int,string> intTostr_operation;


void map_operations(){
    operation["add"]=1;
    operation["sub"]=2;
    operation["mul"]=3;
    operation["beq"]=4;
    operation["bne"]=5;
    operation["slt"]=6;
    operation["j"]=7;
    operation["lw"]=8;
    operation["sw"]=9;
    operation["addi"]=10;

    intTostr_operation[1]="add";
    intTostr_operation[2]="sub";
    intTostr_operation[3]="mul";
    intTostr_operation[4]="beq";
    intTostr_operation[5]="bne";
    intTostr_operation[6]="slt";
    intTostr_operation[7]="j";
    intTostr_operation[8]="lw";
    intTostr_operation[9]="sw";
    intTostr_operation[10]="addi";

}



bool is_integer(string s){
    for(int j=0;j<s.length();j++){
        if(isdigit(s[j]) == false && !(j==0 and s[j]=='-')){
            return false;
        }
    }
    return true;
}


void add(){
    struct Instruction current = instructs[PC];
    if(is_integer(current.field_3)){
        register_values[current.field_1]=register_values[current.field_2]+stoi(current.field_3);
    }
    else{
        register_values[current.field_1]=register_values[current.field_2]+register_values[current.field_3];
    }
    PC++;
}

void sub(){
    struct Instruction current = instructs[PC];
    if(is_integer(current.field_3)){
        register_values[current.field_1]=register_values[current.field_2]-stoi(current.field_3);
    }
    else{
        register_values[current.field_1]=register_values[current.field_2]-register_values[current.field_3];
    }
    PC++;
}


void mul(){
    struct Instruction current = instructs[PC];
    if(is_integer(current.field_3)){
        register_values[current.field_1]=register_values[current.field_2]*stoi(current.field_3);
    }
    else{
        register_values[current.field_1]=register_values[current.field_2]*register_values[current.field_3];
    }
    PC++;
}


void beq(){
    struct Instruction current = instructs[PC];
    if(register_values[current.field_1]==register_values[current.field_2]){
        PC=stoi(current.field_3)-1;
    }
    else PC++;
}


void bne(){
    struct Instruction current = instructs[PC];
    if(register_values[current.field_1]!=register_values[current.field_2]){
        PC=stoi(current.field_3)-1;
    }
    else PC++;
}


void slt(){
    struct Instruction current = instructs[PC];
    if(is_integer(current.field_3)){
        if(stoi(current.field_3)>register_values[current.field_2])register_values[current.field_1]=1;
        else register_values[current.field_1]=0;
    }
    else{
        if(register_values[current.field_3]>register_values[current.field_2])register_values[current.field_1]=1;
        else register_values[current.field_1]=0;
    }
    PC++;
}


void j(){
    struct Instruction current = instructs[PC];
    PC=stoi(current.field_1)-1;
}


void lw(){
    struct Instruction current = instructs[PC];
     int address = register_values[current.field_3]+stoi(current.field_2);
    if ((address >= (1<<(20)) || address < 4 * instructs.size()) || address%4!=0){validFile =false;return;}
    register_values[current.field_1]= memory[address];
    PC++;
}


void sw(){
    struct Instruction current = instructs[PC];
    int address = register_values[current.field_3]+stoi(current.field_2);
    if ((address >= (1<<(20)) || address < 4*instructs.size()) || address%4!=0){validFile =false;return;}
    memory[address] = register_values[current.field_1];
    PC++;
}


void addi(){
    struct Instruction current = instructs[PC];
    register_values[current.field_1]=register_values[current.field_2]+stoi(current.field_3);
    PC++;
}


void map_register_numbers(){
    //maps each register to a unique number between 0-31 inclusive
    register_numbers[0]="$r0";
    register_numbers[1]="$at";
    register_numbers[2]="$v0";
    register_numbers[3]="$v1";
    for(int i=4;i<=7;i++){
        register_numbers[i]="$a"+to_string(i-4);
    }
    for(int i=8;i<=15;i++){
        register_numbers[i]="$t"+to_string(i-8);
    }
    for(int i=16;i<=23;i++){
        register_numbers[i]="$s"+to_string(i-16);
    }
    register_numbers[24]="$t8";
    register_numbers[25]="$t9";
    register_numbers[26]="$k0";
    register_numbers[27]="$k1";
    register_numbers[28]="$gp";
    register_numbers[29]="$sp";
    register_numbers[30]="$s8";
    register_numbers[31]="$ra";
}

void initialise_Registers(){
    //initialises all the registers
    for(int i=0;i<32;i++){
        register_values[register_numbers[i]]=0;
    }
    register_values["$sp"]=2147479548;
}

bool valid_register(string R){
    return register_values.find(R)!=register_values.end();
}
int SearchForRegister (int starting_index, int ending_index, string file_string){
    //this is a helper function which searches for a register from starting index and returns the starting point of it
    int start = -1;
    for (int j= starting_index; j<=ending_index; j++){
        if(file_string[j]==' ' || file_string[j] == '\t'){continue;}
        else{start =j;break;}
    }
    if(start == -1 || start + 2 > ending_index){return -1;}
    if (!valid_register(file_string.substr(start,3))){return -1;}
    return start;  //else found a valid register
}
int SearchForCharacter (int starting_index, int ending_index, string file_string ,char Matching){
    //returns the position of Matching if it is the first non-whitespace character to be found, -1 otherwise
    int start = -1;
    for (int j= starting_index; j<=ending_index; j++){
        if(file_string[j]==' ' || file_string[j] == '\t'){continue;}
        else if (file_string[j]==Matching){return j;}
        else{return -1;}
    }
    return -1; //if no character found except whitespace
}
pair<int,int> SearchForInteger (int starting_index, int ending_index, string file_string){
    //returns the starting and ending index of integer if found
    int start=-1;int end=-1;bool firstMinus = true;
    for (int j= starting_index; j<=ending_index; j++){
        if((file_string[j] == ' ' || file_string[j] == '\t') && start ==-1){continue;} //removing the starting spaces and tabs}
        if (isdigit(file_string[j]) || (file_string[j]=='-' && firstMinus)){
            firstMinus = false;
            if(start==-1){start=j;end=j;}
            else{end=j;}
        }
        else{
            return {start,end};
        }
    }
    return {start,end};
}
string Match_Instruction(int start, int end, string file_string){
    //returns the matched instruction
     if (start +3 <=end){
        string ins =file_string.substr(start,4);
        if (ins =="addi"){return ins;}
    }
    if(start + 2 <=end){
        string ins = file_string.substr(start,3);
        if (ins == "add" || ins == "sub" || ins == "mul" || ins == "slt" ||ins =="beq" || ins =="bne"){return ins;}
    }
    if (start +1 <=end){
        string ins =file_string.substr(start,2);
        if (ins =="lw" || ins == "sw"){return ins;}
    }
    if (start<=end){
        string ins= file_string.substr(start,1);
        if (ins== "j"){return ins;}
    }
    return ""; //when no valid instruction found
}

//handle the case when integer is beyond instruction memory at execution time
void Create_structs(string file_string){
    int i=0;
    bool instruction_found = false;
    // each line can contain atmost one instruction
    while(i<file_string.size()){
        if(file_string[i]==' ' || file_string[i] == '\t'){i++;continue;}
        else{
            if(instruction_found){validFile=false;return;}   //if we have already found an instruction and a character appears, file is invalid
        string ins = Match_Instruction(i, file_string.size()-1, file_string);
        if (ins ==""){  //invalid matching
            validFile = false;
            return;
        }
        if(ins == "add" || ins == "sub" || ins == "mul" || ins == "slt" || ins == "beq" || ins == "bne" || ins == "addi"){
            //now, there must be three registers ahead, delimited by comma
            int reg1_start;
            if (ins =="addi"){reg1_start= SearchForRegister(i+4, file_string.size()-1, file_string);}
            else{reg1_start = SearchForRegister(i+3, file_string.size()-1, file_string);}
            if(reg1_start==-1){validFile=false;return;}
            string R1=file_string.substr(reg1_start,3);
            //now first register has been found, it must be followed by a comma and there can be 0 or more whitespaces in between
            int comma1Pos=SearchForCharacter(reg1_start+3,file_string.size()-1, file_string, ',');
            if (comma1Pos==-1){validFile=false;return;}
            int reg2_start=SearchForRegister(comma1Pos+1, file_string.size()-1, file_string);
            if(reg2_start==-1){validFile=false;return;}
            string R2=file_string.substr(reg2_start,3);
            int comma2Pos= SearchForCharacter(reg2_start+3, file_string.size()-1, file_string, ',');
            if (comma2Pos==-1){validFile=false;return;}
            int reg3_start= SearchForRegister(comma2Pos+1, file_string.size()-1, file_string);
            //instead of third register, we can also have an integer value
            pair<int,int> integer_indices = SearchForInteger(comma2Pos+1,file_string.size()-1, file_string);
            int index_looped;
            if(reg3_start==-1 && integer_indices.first==-1){validFile=false;return;} //neither an integer nor a string
            string R3;
            if (reg3_start!=-1){
                if(ins!="beq" && ins!="bne" && ins!="addi"){       //is a register and instruction is not bne,beq or addi
                    R3=file_string.substr(reg3_start,3);
                    index_looped = reg3_start +3;
                }
                else{       //beq,bne and addi must have the third argument as an integer
                    validFile=false;
                    return;
                }
            }
            else{
                if(ins=="beq" || ins == "bne"){
                    if(file_string[integer_indices.first]=='-'){validFile=false; return;}
                    else{
                        R3= file_string.substr(integer_indices.first, integer_indices.second- integer_indices.first+1);
                        index_looped = integer_indices.second+1;
                    }
                }
                else{
                    R3= file_string.substr(integer_indices.first, integer_indices.second- integer_indices.first+1);
                    index_looped = integer_indices.second+1;
                }
            }
            struct Instruction new_instr;
            new_instr.name=ins;
            new_instr.field_1 = R1;
            new_instr.field_2 = R2;
            new_instr.field_3 = R3;
            i = index_looped; //increment i
            instruction_found=true;
            instructs.push_back(new_instr);
            continue;
        }
        else if (ins == "j"){
            pair<int,int> integer_indices = SearchForInteger(i+1 ,file_string.size()-1, file_string);
            if (integer_indices.first == -1 || file_string[integer_indices.first]=='-'){validFile=false; return;}
                    struct Instruction new_instr;
                    new_instr.name=ins;
                    new_instr.field_1 = file_string.substr(integer_indices.first, integer_indices.second- integer_indices.first+1);
                    new_instr.field_2 = "";
                    new_instr.field_3 = "";
                    int index_looped = integer_indices.second +1;
                    i = index_looped;
                    instruction_found =true;
                    instructs.push_back(new_instr);
        }
        else if (ins == "lw" || ins == "sw"){
            //this has the format lw $t0, offset($register_name)
            // first of all search for the first register
            int reg1_start=SearchForRegister(i+2, file_string.size()-1, file_string);
            if(reg1_start==-1){validFile=false;return;}
            string R1=file_string.substr(reg1_start,3);
            //now we will search for a comma and match it
            int commaPos=SearchForCharacter(reg1_start+3,file_string.size()-1, file_string, ',');
            if (commaPos==-1){validFile=false;return;}
            // now we will search for an integer offset and match it
            pair<int,int> integer_indices = SearchForInteger(commaPos+1,file_string.size()-1, file_string);
            if(integer_indices.first==-1){validFile=false;return;}
            string offset = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first+1);
            // now we will match Left parenthesis
            int lparenPos=SearchForCharacter(integer_indices.second+1 ,file_string.size()-1, file_string, '(');
            if (lparenPos==-1){validFile=false;return;}
            //now we will match a register
            int reg2_start=SearchForRegister(lparenPos+1, file_string.size()-1, file_string);
            if(reg2_start==-1){validFile=false;return;}
            string R2=file_string.substr(reg2_start,3);
            // now we will match the right parenthesis
            int rparenPos = SearchForCharacter(reg2_start+3, file_string.size()-1, file_string, ')');
            if (rparenPos ==-1){validFile = false;return;}
            struct Instruction new_instr;
            new_instr.name=ins;
            new_instr.field_1 = R1;
            new_instr.field_2 = offset;
            new_instr.field_3 = R2;
            i= rparenPos+1;
            instruction_found=true;
            instructs.push_back(new_instr);
    }
}
    }
}
void print_registers(){
    for (int i =0 ;i <32; i++){
        if (i==0) {cout<<register_numbers[i]<<" "<< std:: hex <<(register_values[register_numbers[i]]);}
        else {cout<<", "<<register_numbers[i]<<" "<< std:: hex <<(register_values[register_numbers[i]]);}
    }
    cout<<"\n";
}
void Number_of_times(int ins_count[],int op_count[]){
    cout<<"The number of times each instruction was executed is given below : \n"<<endl;
    for(int i=0; i< instructs.size() ;i++){
        cout<<"Instruction no: "<< std::dec <<i+1<<" was executed "<<std::dec <<ins_count[i]<<" times."<<endl;
    }
    
    cout<<"\nThe number of times each type of instruction was executed is given below : \n"<<endl;
    for(int i=1; i< 11 ;i++){
        cout<<"Operation "<< intTostr_operation[i]<<" was executed "<<std::dec << op_count[i]<<" times."<<endl;
    }
    cout<<"\nThe total number of clock cycles elapsed is : "<<std::dec <<clock_cycles<<"\n\n";

}

void perform_operations(bool flag){
    // flag= true means we intend to print the register values, else we are simply counting the number of clock cycles
    int execution_no=1;
    int op_count[11]={0};
    int ins_count[instructs.size()];
    for (int j=0; j< instructs.size();j++){ins_count[j]=0;}
    if (flag) { cout<< "\n";}
    while(PC<instructs.size()){
        ins_count[PC]++;
        struct Instruction current = instructs[PC];
        int action= operation[current.name];
        op_count[action]++;
        clock_cycles++;
        if (clock_cycles > maxClockCycles){
            infinite_loop =true;
            return;
        }
        switch (action){
            case 1: add();break;
            case 2: sub();break;
            case 3: mul();break;
            case 4: beq();break;
            case 5: bne();break;
            case 6: slt();break;
            case 7: j();break;
            case 8: lw();break;
            case 9: sw();break;
            case 10:addi();break;
        }
        if (!validFile) {return;}
        if (flag){
        cout<<"Execution no. "<< std::dec << execution_no++<<"\n\n";
        print_registers();
            cout<<"\n";
        }
    }
    if (flag){
        Number_of_times(ins_count,op_count);
    }
    
}



int main(){
    
      ifstream file("mips_program.txt");
      string current_line;
      map_register_numbers();
      initialise_Registers();
      validFile = true;
      infinite_loop = false;
      while(getline(file,current_line)){
          Create_structs(current_line);
      }
    map_operations();
    if(!validFile){
        cout<<"Invalid MIPS program"<<endl;
        return -1;
    }
    perform_operations(false);
    
    if (infinite_loop){
        cout<<"Time limit exceeded !"<<endl;
        return -1;
    }
    if (!validFile){
        cout<<"Invalid MIPS program"<<endl;  //due to wrong lw and sw addresses
        return -1;
    }
    PC = 0;
    clock_cycles = 0;
    initialise_Registers();
    perform_operations (true);
    
    /*each instruction occupies 4 bytes. So, we will first of all maintain an array of instructions
     to get the instruction starting at memory address i (in the form of a struct). Rest of the memory is used in RAM*/
    /*so memory stores instructions (as structs here) and data as integers in decimal format*/
    
    return 0;
}
