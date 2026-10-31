#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "area.hpp"
#include "tokenization.hpp"
#include "parser.hpp"
#include "generation.hpp"

int main(int argc,char* argv[]){
    if(argc!=2){
        std::cerr<<"Incorrect usage"<<std::endl;
        return EXIT_FAILURE;
    }
    std::string contents;
    std::fstream input(argv[1],std::ios::in);
    std::stringstream contents_stream;
    contents_stream << input.rdbuf();
    input.close();
    contents=contents_stream.str();
    Tokenizer tokenizer(std::move(contents));

    std::vector<Token> tokens=tokenizer.tokenize();
    Parser parser(std::move(tokens));
    std::optional<NodeProg> prog = parser.parse_prog();
    if(!prog.has_value()){
        std::cerr<<"no exit statment found"<<std::endl;
    }
    Generation generater(std::move(prog.value()));

    
    {
        std::fstream file("out.asm",std::ios::out);
        file << generater.gen_prog();
    }

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}