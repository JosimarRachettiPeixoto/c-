#include <iostream>
#include <string>
using namespace std;

class Computer{
    public:
        int ram;
        string marca;
        int armazenamento;
        string modelo;
};

int main(){
    Computer myComputer;
    cout << "abaixo descreva seu computador"<< endl;
    cout << "Ram: ";
    cin >> myComputer.ram;
    cout << "memoria ram: " << myComputer.ram <<'\n';



    exit(0);
}