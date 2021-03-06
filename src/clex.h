#ifndef CLEX_H
#define CLEX_H

#include "cosmo.h"

typedef enum {
    // single character tokens
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_DOT_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_EOS, // end of statement

    // equality operators
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,

    // literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_NIL,
    TOKEN_TRUE,
    TOKEN_FALSE,

    // keywords & reserved words
    TOKEN_AND,
    TOKEN_DO,
    TOKEN_ELSE,
    TOKEN_ELSEIF,
    TOKEN_END,
    TOKEN_FOR,
    TOKEN_FUNCTION,
    TOKEN_IF,
    TOKEN_LOCAL,
    TOKEN_NOT,
    TOKEN_OR,
    TOKEN_RETURN,
    TOKEN_THEN,
    TOKEN_VAR,
    TOKEN_WHILE,

    TOKEN_ERROR,
    TOKEN_EOF
} CTokenType;

typedef struct {
    CTokenType type;
    const char *word;
    int len;
} CReservedWord;

typedef struct {
    CTokenType type;
    char *start;
    int length;
    int line;
} CToken;

typedef struct {
    char *currentChar;
    char *startChar;
    int line; // current line
    int lastLine; // line of the previous consumed token
    int openedBraces; // tracks open [], {}, or ()
    bool isEnd;
    CTokenType lastType;
} CLexState;

CLexState *cosmoL_newLexState(CState *state, const char *source);
void cosmoL_freeLexState(CState *state, CLexState *lstate);

CToken cosmoL_scanToken(CLexState *state);

#endif