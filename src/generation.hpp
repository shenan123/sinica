#pragma once
#include <optional>
#include <unordered_map>
#include <assert.h>
#include <vector>
#include <algorithm>

#include "parser.hpp"
#include "tokenization.hpp"

class Generation
{
private:
    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_size=0;
    int m_label_count=0;

    struct Variable
    {
        std::string name;
        size_t loc;
    };

    std::vector<Variable> m_vars {};
    std::vector<size_t> m_scope {};

    void push(const std::string& reg){
       m_output << "    push "<< reg <<"\n";
       m_stack_size++;
    }
    void pop(const std::string& reg){
       m_output << "    pop "<< reg <<"\n";
       m_stack_size--;
    }

    void begin_scope(){
        m_scope.push_back(m_vars.size());
    }
    void end_scope(){
        size_t pop_count=m_vars.size()-m_scope.back();
        m_output << "    add rsp," << pop_count * 8 << "\n";
        m_stack_size -= pop_count;
        for(int i=0;i<pop_count;i++){
            m_vars.pop_back();
        }
        m_scope.pop_back();
    }

    std::string creat_label(){
        std::stringstream ss;
        ss << "label" << m_label_count++;
        return ss.str();
    }
    
 

public:
    inline Generation(NodeProg prog) 
    : m_prog(std::move(prog)){

    }

    void gen_term(const NodeTerm* term){
        struct TermVistor
        {
            Generation* gen;
            void operator()(const NodeTermIdent* term_ident) const{
                auto it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend(),[term_ident](const Variable& var)
                {return var.name==term_ident->ident.value.value();});
                if(it == gen->m_vars.cend()){
                    std::cerr<<"Undeclared identifier:"<<term_ident->ident.value.value()<<std::endl;
                    exit(EXIT_FAILURE);
                }
                std::stringstream offset;
                offset << "QWORD[rsp +" << (gen->m_stack_size - (*it).loc - 1)*8 << "]";
                gen->push(offset.str());
            }
            void operator()(const NodeTermIntlit* term_intlit) const{
                gen->m_output << "    mov rax,"<< term_intlit->int_lit.value.value()<<"\n";
                gen->push("rax");
            }
            void operator()(const NodeTermParen* term_paren) const {
                gen->gen_expr(term_paren->expr);
            }
        };

        TermVistor vistor({.gen=this});
        std::visit(vistor,term->var);
        
    }
    void gen_bin_expr(const NodeBinExpr* bin_expr){
        struct BinExprVistor
        {
            Generation* gen;
            void operator()(const NodeBinExprAdd* add) const{
                gen->gen_expr(add->rhs);
                gen->gen_expr(add->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    add rax, rbx\n";
                gen->push("rax");
            }
            void operator()(const NodeBinExprSub* sub) const{
                gen->gen_expr(sub->rhs);
                gen->gen_expr(sub->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    sub rax, rbx\n";
                gen->push("rax");
            }
            void operator()(const NodeBinExprMulti* multi) const {
                gen->gen_expr(multi->rhs);
                gen->gen_expr(multi->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    mul rbx\n";
                gen->push("rax");
            }
            void operator()(const NodeBinExprDiv* div) const{
                gen->gen_expr(div->rhs);
                gen->gen_expr(div->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "     div rbx\n";
                gen->push("rax");
            }
        };
        BinExprVistor visitor{.gen=this};
        std::visit(visitor,bin_expr->var);
        
    }
    void gen_if_pred(const NodeIfPred* pred,const std::string end_label){
        struct PredVistor
        {
            Generation& gen;
            const std::string& end_label;
            void operator()(const NodeIfPredElif* _elif) const {
                gen.gen_expr(_elif->expr);
                gen.pop("rax");
                std::string label=gen.creat_label();
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jz "<<label<<"\n";
                gen.gen_scope(_elif->scope);
                gen.m_output << "    jmp "<<end_label<<"\n";
                if(_elif->pred.has_value()){
                    gen.m_output << label <<":\n";
                    gen.gen_if_pred(_elif->pred.value(),end_label);
                }
                gen.m_output << "    ;; /elif\n";
            }
            void operator()(const NodeIfPredElse* _else) const {
                gen.gen_scope(_else->scope);
                gen.m_output << "    ;; /else\n";
            }
        };
        PredVistor visitor{.gen=*this,.end_label=end_label};
        std::visit(visitor,pred->var);
        
    }
    void gen_expr(const NodeExpr* expr){
        struct ExprVistor
        {
            Generation* gen;
            void operator()(const NodeTerm* term)const{
                gen->gen_term(term);
            }
            void operator()(const NodeBinExpr* bin_expr){
                gen->gen_bin_expr(bin_expr);
            }
        };
        ExprVistor visitor{.gen=this};
        std::visit(visitor,expr->var);
        
    }

    void gen_scope(const NodeScope* scope){
        begin_scope();
        for(auto stmt : scope->stmts){
            gen_stmt(stmt);
        }
        end_scope();
    }
    void gen_stmt(const NodeStmt* stmt){
        struct StmtVistor
        {
            Generation* gen;
            void operator()(const NodeStmtExit* stmt_exit) const {
                gen->m_output << "    ;; /exit\n";
                gen->gen_expr(stmt_exit->expr);
                gen->m_output<<"    mov rax,60\n";
                gen->pop("rdi");
                gen->m_output<<"    syscall\n";
            }
            void operator()(const NodeStmtLet* stmt_let) const{
                auto it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend()
                ,[stmt_let](const Variable& var)
                {return var.name==stmt_let->ident.value.value();});
                if(it != gen->m_vars.cend()){
                    std::cerr<<"Identifier already used: "<<stmt_let->ident.value.value()<<std::endl;
                    exit(EXIT_FAILURE);
                }
                gen->m_vars.push_back({.name=stmt_let->ident.value.value(), .loc=gen->m_stack_size});
                gen->gen_expr(stmt_let->expr);
            }
            void operator()(const NodeScope* scope) const {
                gen->gen_scope(scope);

            }
            void operator()(const NodeStmtIf* stmt_if) const{
                gen->gen_expr(stmt_if->expr);
                gen->pop("rax");
                std::string label=gen->creat_label();
                gen->m_output << "    test rax, rax\n";
                gen->m_output << "    jz "<<label<<"\n";
                gen->gen_scope(stmt_if->scope);
                if(stmt_if->pred.has_value()){
                    const std::string end_label = gen->creat_label();
                    gen->m_output << "    jmp "<<end_label<<"\n";
                    gen->m_output << label <<":\n";
                    gen->gen_if_pred(stmt_if->pred.value(),end_label);
                    gen->m_output << end_label <<":\n";
                }else{
                    gen->m_output << label <<":\n";
                }
                gen->m_output << "    ;; /if\n";
            }

            void operator()(const NodeStmtAssign* assign) const {
                auto it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend(),[assign](const Variable& var){
                return var.name== assign->ident.value.value();});
                if(it == gen->m_vars.end()){
                    std::cerr<<"Undeclared identifier:"<<assign->ident.value.value()<<std::endl;
                    exit(EXIT_FAILURE);
                }
                gen->gen_expr(assign->expr);
                gen->pop("rax");
                gen->m_output << "    mov [rsp + "<<(gen->m_stack_size-it->loc-1)*8<<"],rax\n";





            }
        };
        
        StmtVistor visitor{.gen=this};
        std::visit(visitor,stmt->var);
        
    }

    [[nodiscard]] inline std::string gen_prog(){
        m_output << "global _start\n_start:\n";
        for(const NodeStmt* stmt:m_prog.stmts){
            gen_stmt(stmt);
        }
        m_output << "    ;; /exit\n";
        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall";
        return m_output.str();
    }
};


