#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
using namespace std;

void split(const string&, const char*);
void reverse(const string&);

int main(int argc, char* argv[]){
    // Check parameters
    if(argc != 3){
        cout << "usage: ./main [input file] [split token]" << endl;
        return 0;
    }

    // Load file 
    fstream file(argv[1]);

    // Read file and execute split and reverse function until encounter an exit
    while(file){
        string input_string;
        file >> input_string;
        if(strcmp(input_string.c_str(),"exit") == 0){
            return 0;
        }
        else if(strcmp(input_string.c_str(),"reverse") == 0){
            file >> input_string;
            reverse(input_string);
        }
        else if(strcmp(input_string.c_str(),"split") == 0){
            file >> input_string;
            split(input_string,argv[2]);
            cout << endl;
        }
    }

    while(1){
        string input_string;
        cin >> input_string;
        if(strcmp(input_string.c_str(),"exit") == 0){
            return 0;
        }
        else if(strcmp(input_string.c_str(),"reverse") == 0){
            cin >> input_string;
            reverse(input_string);
        }
        else if(strcmp(input_string.c_str(),"split") == 0){
            cin >> input_string;
            split(input_string,argv[2]);
            cout << endl;
        }
    }
}

void split(const string& input_string, const char* pattern){
    char* str = new char[input_string.length()+1];
    strcpy(str, input_string.c_str());
    str[input_string.length()] = '\0';
    
    char* sp_str = strtok(str, pattern);
    while(sp_str != NULL){
        cout << sp_str <<" ";
        sp_str = strtok(NULL, pattern);
    }
    delete [] sp_str;
}

void reverse(const string& input_string){
    int length = input_string.length();
    string reverse_string;
    for(int i=length-1;i>=0;i--){
        reverse_string.push_back(input_string[i]);
    }
    cout << reverse_string << endl;
}
