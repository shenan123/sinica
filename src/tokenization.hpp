#pragma once
#include <vector>
#include <string>
#include <optional>

enum class TokenType{
    _return,
    exit,
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq,
    plus,
    star,
    div,
    sub,
    _if,
    _elif,
    _else,
    open_curly,
    close_curly
};

struct Token{
    TokenType type;
    int line;
    std::optional<std::string> value;
};

class Tokenizer
{
private:
    const std::string m_src;
    size_t m_index = 0;
    int line_count = 1;

    [[nodiscard]] std::optional<char> peek(int offset=0) const {
        if(m_index+offset >= m_src.length()){
            return {};
        }
        else{
            return m_src.at(m_index + offset);
        }
    }

    inline char consume(){
        return m_src.at(m_index++);
    }

public:
    inline explicit Tokenizer(std::string src)
    :m_src(std::move(src)){};
    
    inline std::vector<Token> tokenize(){
        std::vector<Token> res;
        std::string buf;
        while(peek().has_value()){
            if(std::isalpha(peek().value())){
                buf.push_back(consume());
                while(peek().has_value() && std::isalnum(peek().value())){
                    buf.push_back(consume());
                }
                if(buf=="exit"){
                    res.push_back({.type=TokenType::exit,.line=line_count});
                    buf.clear();
                }else if(buf=="return"){
                    res.push_back({.type=TokenType::_return,.line=line_count});
                    buf.clear();
                }else if(buf=="let"){
                    res.push_back({.type=TokenType::let,.line=line_count});
                    buf.clear();
                }else if(buf == "if"){
                    res.push_back({.type=TokenType::_if,.line=line_count});
                    buf.clear();
                }else if(buf=="elif"){
                    res.push_back({.type=TokenType::_elif,.line=line_count});
                    buf.clear();
                }else if(buf=="else"){
                    res.push_back({.type=TokenType::_else,.line=line_count});
                    buf.clear();
                }else{
                    res.push_back({.type=TokenType::ident,.line=line_count,.value=buf});
                    buf.clear();
                }
            }else if(std::isdigit(peek().value())){
                buf.push_back(consume());
                while(peek().has_value() && std::isdigit(peek().value())){
                    buf.push_back(consume());
                }
                res.push_back({.type=TokenType::int_lit,.line=line_count,.value=buf});
                buf.clear();
            }else if(peek().value()=='/'&&peek(1).has_value()&&peek(1).value()=='/'){
                consume();
                consume();
                while(peek().has_value()&&peek().value()!='\n'){
                    consume();
                }
            }else if(peek().value()=='/'&&peek(1).has_value()&&peek(1).value()=='*'){
                consume();
                consume();
                while(peek().has_value()){
                    if(peek().value()=='*'&&peek(1).has_value()&&peek(1).value()=='/'){
                        break;
                    }
                    consume();
                }
                consume();
                consume();
            }else if(peek().value() == '('){
                consume();
                res.push_back({.type=TokenType::open_paren,.line=line_count});
            }else if(peek().value()==')'){
                consume();
                res.push_back({.type=TokenType::close_paren,.line=line_count});
            }else if(peek().value() ==';'){
                consume();
                res.push_back({.type=TokenType::semi,.line=line_count});
            }else if(peek().value()=='\n'){
                consume();
                line_count++;
            }else if(std::isspace(peek().value())){
                consume();
            }else if(peek().value()=='='){
                res.push_back({.type=TokenType::eq,.line=line_count});
                consume();
            }else if(peek().value()=='+'){
                consume();
                res.push_back({.type=TokenType::plus,.line=line_count});
            }else if(peek().value()=='*'){
                consume();
                res.push_back({.type=TokenType::star,.line=line_count});
            }else if(peek().value()=='-'){
                consume();
                res.push_back({.type=TokenType::sub,.line=line_count});
            }else if(peek().value()=='/'){
                consume();
                res.push_back({.type=TokenType::div,.line=line_count});
            }else if(peek().value()=='{'){
                consume();
                res.push_back({.type=TokenType::open_curly,.line=line_count});
            }else if(peek().value()=='}'){
                consume();
                res.push_back({.type=TokenType::close_curly,.line=line_count});
            }else{
                std::cerr<<"You messed up!"<<std::endl;
                exit(EXIT_FAILURE);
            }
        }
        m_index=0;
        return res;
    }
    
};