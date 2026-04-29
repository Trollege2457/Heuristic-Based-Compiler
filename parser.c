#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokens.h"
#include "grammar.h"

typedef void* YY_BUFFER_STATE;

extern int yylex();
extern char *yytext;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

#define MAX_TOKENS 200

typedef struct {
    int type;
    char lexeme[32];
} Token;

Token tokens[MAX_TOKENS];
int tok_count = 0;

/*RECOMMENDATION SYSTEM*/
void recommend_fix(int type, char *input, char ops[], int op_count)
{
    printf("\n================ RECOMMENDATION ================\n");
    printf("Input Expression: %s\n\n", input);

    if (type == 1)
    {
        printf("TYPE: PRECEDENCE AMBIGUITY\n");
        printf("---------------------------------------------\n");
        printf("PROBLEM:\nMixed operators without defined precedence.\n\n");
        printf("WHY THIS IS AMBIGUOUS:\n");
        printf("a + b * c → (a + b) * c OR a + (b * c)\n\n");
        printf("FIX:\nE -> E + T | E - T | T\nT -> T * F | T / F | F\nF -> (E) | ID | NUM\n\n");
    }
    else if (type == 2)
    {
        printf("TYPE: ASSOCIATIVITY AMBIGUITY\n");
        printf("---------------------------------------------\n");
        printf("PROBLEM:\nRepeated operator causes multiple grouping.\n\n");
        printf("OPERATOR: '%c'\n\n", ops[0]);
        printf("EXAMPLE:\n");
        printf("a %c b %c c → (a %c b) %c c OR a %c (b %c c)\n\n",
               ops[0], ops[0], ops[0], ops[0], ops[0], ops[0]);
        printf("FIX:\nLeft: E -> E %c T | T\nRight: E -> T %c E | T\n\n",
               ops[0], ops[0]);
    }
    else if (type == 3)
    {
        printf("TYPE: DANGLING ELSE AMBIGUITY\n");
        printf("---------------------------------------------\n");
        printf("PROBLEM:\nElse can bind to multiple if statements.\n\n");
        printf("EXAMPLE:\n");
        printf("if(a)\n    if(b)\n        x;\n    else\n        y;\n\n");
        printf("FIX:\nstmt -> matched | unmatched\nmatched -> if(cond) matched else matched | other\nunmatched -> if(cond) stmt | if(cond) matched else unmatched\n\n");
    }
    printf("================================================\n");
}

/* TOKEN STREAM  */
void build_token_stream()
{
    tok_count = 0;
    int t;
    while ((t = yylex()) != 0)
    {
        if (tok_count >= MAX_TOKENS) break;
        tokens[tok_count].type = t;
        strncpy(tokens[tok_count].lexeme, yytext, 31);
        tokens[tok_count].lexeme[31] = '\0';
        tok_count++;
    }
}

/* VALIDATION */
int is_valid_expression()
{
    int expect_operand = 1;
    for (int i = 0; i < tok_count; i++)
    {
        int t = tokens[i].type;
        if (t == ID || t == NUM)
        {
            if (!expect_operand) return 0;
            expect_operand = 0;
        }
        else if (t == PLUS || t == MINUS || t == MUL || t == DIV || t == REM)
        {
            if (expect_operand) return 0;
            expect_operand = 1;
        }
        else if (t == LPAREN || t == RPAREN || t == IF || t == ELSE)
        {
            continue;   // IMPORTANT: allow statement tokens
        }
        else if (t == SEMI)
        {
            if (i != tok_count - 1) return 0;
        }
        else
        {
            return 0;
        }
    }
    return !expect_operand;
}

/* ---------------- OPS ---------------- */
void extract_ops(char ops[], int *op_count)
{
    *op_count = 0;
    for (int i = 0; i < tok_count; i++)
    {
        int t = tokens[i].type;
        if (t == PLUS) ops[(*op_count)++] = '+';
        else if (t == MINUS) ops[(*op_count)++] = '-';
        else if (t == MUL) ops[(*op_count)++] = '*';
        else if (t == DIV) ops[(*op_count)++] = '/';
        else if (t == REM) ops[(*op_count)++] = '%';
    }
}

/*  DETECTION  */
int is_low(char op) { return (op == '+' || op == '-'); }
int is_high(char op) { return (op == '*' || op == '/' || op == '%'); }

int has_precedence_ambiguity(char ops[], int n)
{
    for (int i = 0; i < n - 1; i++)
        if (is_low(ops[i]) && is_high(ops[i + 1]))
            return 1;
    return 0;
}

int is_same_operator(char ops[], int n)
{
    for (int i = 1; i < n; i++)
        if (ops[i] != ops[0])
            return 0;
    return 1;
}

/* DANGLING ELSE */
int has_dangling_else(char *input)
{
    int if_count = 0;
    int else_count = 0;
    for (int i = 0; input[i]; i++)
    {
        if (strncmp(&input[i], "if", 2) == 0)
            if_count++;
        if (strncmp(&input[i], "else", 4) == 0)
            else_count++;
    }
    return (if_count >= 2 && else_count >= 1);
}

/* MAIN */
int main()
{
    int choice;
    char line[100];
    if (!fgets(line, sizeof(line), stdin))
        return 0;
    choice = atoi(line);
    if (choice == 1)
    {
        read_grammar();
        detect_direct_left_recursion();
        detect_indirect_left_recursion();
        return 0;
    }
    if (choice == 2)
    {
        char expr[200];
        if (!fgets(expr, sizeof(expr), stdin))
            return 0;
        YY_BUFFER_STATE buffer = yy_scan_string(expr);
        build_token_stream();
        yy_delete_buffer(buffer);
                char ops[100];
        int op_count = 0;
        extract_ops(ops, &op_count);
        if (has_dangling_else(expr))
        {
            printf("Ambiguity Detected\nClassification: Dangling Else\n");
            recommend_fix(3, expr, ops, op_count);
            return 0;
        }
        if (!is_valid_expression())
        {
            printf("Invalid Expression\n");
            return 0;
        }
        if (op_count >= 2 && has_precedence_ambiguity(ops, op_count))
        {
            printf("Ambiguity Detected\nClassification: Precedence\n");
            recommend_fix(1, expr, ops, op_count);
            return 0;
        }
        if (op_count >= 2 && is_same_operator(ops, op_count))
        {
            printf("Ambiguity Detected\nClassification: Associativity\n");
            recommend_fix(2, expr, ops, op_count);
            return 0;
        }
        printf("No Ambiguity Detected\n");
        return 0;
    }
    return 0;
}
