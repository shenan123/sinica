#pragma once
#include <variant>
#include <assert.h>

#include "area.hpp"
#include "tokenization.hpp"

struct NodeTermIntlit
{
    Token int_lit;
};
struct NodeTermIdent
{
    Token ident;
};

struct NodeExpr;
struct NodeTermParen
{
    NodeExpr* expr;
};
struct NodeBinExprAdd
{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprMulti
{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprSub
{
    NodeExpr* lhs;
    NodeExpr* rhs;
};
struct NodeBinExprDiv
{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr
{
    std::variant<NodeBinExprAdd*,NodeBinExprMulti*,NodeBinExprDiv*,NodeBinExprSub*> var;
};

struct NodeTerm
{
    std::variant<NodeTermIdent*,NodeTermIntlit*,NodeTermParen*> var;
};


struct NodeExpr
{    
    std::variant<NodeTerm*,NodeBinExpr*> var;
};

struct NodeStmtExit
{
    NodeExpr* expr;
};
struct NodeStmtLet
{
    Token ident;
    NodeExpr* expr;
};
struct NodeStmt;
struct NodeScope
{
    std::vector<NodeStmt*> stmts;
};
struct NodeIfPred;
struct NodeIfPredElse
{
    NodeScope* scope;
};
struct NodeIfPredElif
{
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeIfPred
{
    std::variant<NodeIfPredElif*,NodeIfPredElse*> var;
};
struct NodeStmtIf
{
    NodeScope* scope;
    NodeExpr* expr;
    std::optional<NodeIfPred*> pred;
};

struct NodeStmtAssign
{
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt
{
    std::variant<NodeStmtExit*,NodeStmtLet*,NodeScope*,NodeStmtIf*,NodeStmtAssign*> var;
};

struct NodeProg
{
    std::vector<NodeStmt*> stmts;
};

std::optional<int> bin_prec(TokenType type){
    switch (type)
    {
    case TokenType::plus:
        return 0;
    case TokenType::star:
        return 1;
    case TokenType::sub:
        return 0;
    case TokenType::div:
        return 1;
    default:
        return {};
    }
}

class Parser
{
private:
    const std::vector<Token> m_tokens;
    int m_index = 0;
    Areaallocator m_allocator;

    [[nodiscard]] std::optional<Token> peek(int offset=0) const {
        if(m_index + offset >= m_tokens.size()){
            return {};
        }
        else{
            return m_tokens.at(m_index + offset);
        }
    }

    Token consume(){
        return m_tokens.at(m_index++);
    }
    Token try_consume(TokenType type,const std::string err_msg){
        if(peek().has_value() && peek().value().type==type){
            return consume();
        }
        error_expected(err_msg);
        return {};
    }
    std::optional<Token> try_consume(TokenType type){
        if(peek().has_value() && peek().value().type==type){
            return consume();
        }
        else{
            return {};
        }
    }

public:
    inline explicit Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens)),m_allocator(1024*1024*4){
        for(int i=0;i<10;i++){
            //std::cout<<m_tokens.at(i).value.value();
            std::cout<<"line"<<m_tokens.at(i).line<<std::endl;
        }
    }
    void error_expected(const std::string msg){
        std::cerr<<"[Parse Error] Expexted "<< msg 
        <<"on line "<< peek(-1).value().line <<std::endl;
        exit(EXIT_FAILURE);
    }

    std::optional<NodeScope*> parse_scope(){
        if(!try_consume(TokenType::open_curly).has_value()){
            return {};
        }
        auto scope = m_allocator.alloc<NodeScope>();
        
        while(auto stmt = parse_stmt()){
            scope->stmts.push_back(stmt.value());
        }
        try_consume(TokenType::close_curly,"'}'");
        return scope;
    }
    std::optional<NodeIfPred*> parse_if_pred(){
        if(try_consume(TokenType::_elif)){
            try_consume(TokenType::open_paren,"'('");
            auto elif_pred=m_allocator.alloc<NodeIfPredElif>();
            if(auto expr=parse_expr()){
                elif_pred->expr=expr.value();
            }else{
                error_expected("expression");
            }
            try_consume(TokenType::close_paren,"')'");
            if(auto scope=parse_scope()){
                elif_pred->scope=scope.value();
            }else{
                error_expected("scope");
            }
            elif_pred->pred=parse_if_pred();
            auto pred=m_allocator.alloc<NodeIfPred>();
            pred->var=elif_pred;
            return pred;
        }
        if(try_consume(TokenType::_else)){
            auto else_pred=m_allocator.alloc<NodeIfPredElse>();
            if(auto scope=parse_scope()){
                else_pred->scope=scope.value();
            }
            else{
                error_expected("scope");
            }
            auto pred=m_allocator.alloc<NodeIfPred>();
            pred->var=else_pred;
            return pred;
        }
        return {};
    }
    std::optional<NodeTerm*> parse_term(){
        if(auto int_lit=try_consume(TokenType::int_lit)){
            NodeTermIntlit* term_intlit=m_allocator.alloc<NodeTermIntlit>();
            term_intlit->int_lit=int_lit.value();
            auto term=m_allocator.alloc<NodeTerm>();
            term->var=term_intlit;
            return term;
        }
        else if(auto ident=try_consume(TokenType::ident)){
            NodeTermIdent* term_ident=m_allocator.alloc<NodeTermIdent>();
            term_ident->ident=ident.value();
            auto term=m_allocator.alloc<NodeTerm>();
            term->var=term_ident;
            return term;
        }
        else if(auto open_paren=try_consume(TokenType::open_paren)){
            auto expr=parse_expr();
            if(!expr.has_value()){
                error_expected("expression");
            }
            try_consume(TokenType::close_paren,"')'");
            auto term_paren=m_allocator.alloc<NodeTermParen>();
            auto term=m_allocator.alloc<NodeTerm>();
            term_paren->expr=expr.value();
            term->var=term_paren;
            return term;
        }
        else{
            return {};
        }
    }
    std::optional<NodeExpr*> parse_expr(int min_prec=0){
        std::optional<NodeTerm*> term_lhs=parse_term();
        if(!term_lhs.has_value()){
            return {};
        }
        auto expr_lhs=m_allocator.alloc<NodeExpr>();
        expr_lhs->var=term_lhs.value();

        while(1){
            std::optional<Token> cur_tok=peek();
            std::optional<int> prec;
            if(cur_tok.has_value()){
                prec=bin_prec(cur_tok->type);
                if(!prec.has_value()||prec<min_prec){
                    break;
                }
            }else{
                break;
            }
            Token op=consume();
            int next_min_prec=prec.value()+1;
            auto expr_rhs=parse_expr(next_min_prec);
            if(!expr_rhs.has_value()){
                error_expected("expression");
            }
            auto expr=m_allocator.alloc<NodeBinExpr>();
            auto expr_lhs2=m_allocator.alloc<NodeExpr>();
            if(op.type==TokenType::plus){
                auto add=m_allocator.alloc<NodeBinExprAdd>();
                expr_lhs2->var=expr_lhs->var;
                add->lhs=expr_lhs2;
                add->rhs=expr_rhs.value();
                expr->var=add;
            }
            else if(op.type==TokenType::star){
                auto multi=m_allocator.alloc<NodeBinExprMulti>();
                expr_lhs2->var=expr_lhs->var;
                multi->lhs=expr_lhs2;
                multi->rhs=expr_rhs.value();
                expr->var=multi;
            }else if(op.type==TokenType::sub){
                auto sub=m_allocator.alloc<NodeBinExprSub>();
                expr_lhs2->var=expr_lhs->var;
                sub->lhs=expr_lhs2;
                sub->rhs=expr_rhs.value();
                expr->var=sub;
            }else if(op.type==TokenType::div){
                auto div=m_allocator.alloc<NodeBinExprDiv>();
                expr_lhs2->var=expr_lhs->var;
                div->lhs=expr_lhs2;
                div->rhs=expr_rhs.value();
                expr->var=div;
            }
            else{
                assert(false);
            }
            expr_lhs->var=expr;
        }
        return expr_lhs;
    }
    std::optional<NodeStmt*> parse_stmt(){
        if(peek().value().type == TokenType::exit){
                if(!peek(1).has_value()
                ||peek(1).value().type!=TokenType::open_paren){
                    error_expected("expression");
                }
                consume();
                consume();
                NodeStmtExit* stmt_exit=m_allocator.alloc<NodeStmtExit>();
                if(auto node_expr = parse_expr()){
                    stmt_exit->expr=node_expr.value();
                }
                else{
                    error_expected("expression");
                }
                try_consume(TokenType::close_paren,"')'");
                try_consume(TokenType::semi,"';'");
                NodeStmt* stmt=m_allocator.alloc<NodeStmt>();
                stmt->var=stmt_exit;
                return stmt;
        }else if(peek().value().type == TokenType::let){
            if(!peek(1).has_value() || peek(1).value().type!=TokenType::ident
            ||!peek(2).has_value()||peek(2).value().type!=TokenType::eq){
                error_expected("expression");
            }
            consume();
            NodeStmtLet* stmt_let=m_allocator.alloc<NodeStmtLet>();
            stmt_let->ident=consume();
            consume();
            if(auto expr=parse_expr()){
                stmt_let->expr=expr.value();
            }else{
                error_expected("expression");
            }
            try_consume(TokenType::semi,"';'");
            NodeStmt* stmt=m_allocator.alloc<NodeStmt>();
            stmt->var=stmt_let;
            return stmt;
        }else if(peek().has_value()&&peek().value().type==TokenType::open_curly){
            if(auto scope=parse_scope()){
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var=scope.value();
                return stmt;
            }else{
                error_expected("scope");
            }   
        }else if(peek().has_value()&& peek().value().type==TokenType::ident&&peek(1).has_value()&&peek(1).value().type==TokenType::eq){
            auto assign=m_allocator.alloc<NodeStmtAssign>();
            assign->ident=consume();
            consume();
            if(auto expr=parse_expr()){
                assign->expr=expr.value();
            }else{
                error_expected("expression");
            }
            try_consume(TokenType::semi,"';'");
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var=assign;
            return stmt;
        }else if(auto if_=try_consume(TokenType::_if)){
            try_consume(TokenType::open_paren,"'('");
            auto stmt_if=m_allocator.alloc<NodeStmtIf>();
            if(auto expr = parse_expr()){
                stmt_if->expr=expr.value();
            }else{
                error_expected("expression");
            }
            try_consume(TokenType::close_paren,"')'");
            if(auto scope=parse_scope()){
                stmt_if->scope=scope.value();
            }else{
                error_expected("scope");
            }
            stmt_if->pred=parse_if_pred();
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var=stmt_if;
            return stmt;
        }
        else{
            return {};
        }
    }
    std::optional<NodeProg> parse_prog(){
        NodeProg prog;

        while(peek().has_value()){
            if(auto stmt = parse_stmt()){
                prog.stmts.push_back(stmt.value());
            }else{
                error_expected("statement");
            }
        }
        return prog;
    }

};


