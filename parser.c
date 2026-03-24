
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokens.h"
#include "grammar.h"
extern FILE *yyin;
extern int yylex();
int lookahead;

typedef struct node
{
    char name[20];
    struct node *left, *right;
} node;
node* create_node(char *name, node* left, node* right)
{
    node* temp = (node*)malloc(sizeof(node));
    strcpy(temp->name, name);
    temp->left = left;
    temp->right = right;
    return temp;
}
void print_tree(node* root, int level)
{
    if(!root) return;
    for(int i=0;i<level;i++) printf("   ");
    printf("%s\n", root->name);
    print_tree(root->left, level+1);
    print_tree(root->right, level+1);
}

/*Take the Input and analyse it*/
void extract_all(char *input, char ops[], int *op_count, int *id_count)
{
    *op_count = 0;
    *id_count = 0;
    for(int i = 0; input[i] != '\0'; i++)
    {
        if((input[i] >= 'a' && input[i] <= 'z') ||
           (input[i] >= 'A' && input[i] <= 'Z'))
            (*id_count)++;
        if(strchr("+-*/%", input[i]))
            ops[(*op_count)++] = input[i];
    }
}

/*Actual Detection*/
int is_low(char op)
{ return (op == '+' || op == '-');}
int is_high(char op)
{ return (op == '*' || op == '/' || op == '%');}
int has_precedence_ambiguity(char ops[], int op_count)
{
    for(int i = 0; i < op_count - 1; i++)
    {
        if(is_low(ops[i]) && is_high(ops[i+1]))
            return 1;
    }
    return 0;
}
int is_same_operator(char ops[], int op_count)
{
    for(int i = 1; i < op_count; i++)
    {
        if(ops[i] != ops[0])
        return 0;
    }
    return 1;
}
int has_dangling_else(char *input)
{
    int ifc=0, elsec=0;
    for(int i=0; input[i]; i++)
    {
        if(strncmp(&input[i],"if",2)==0) ifc++;
        if(strncmp(&input[i],"else",4)==0) elsec++;
    }
    return (ifc>=2 && elsec>=1);
}

/*Ambiguity Classifier*/

/* Associativity */
node* build_left_assoc(char ops[], int n)
{
    node* root = create_node((char[]){ops[0],0},
                    create_node("ID",0,0),
                    create_node("ID",0,0));

    for(int i=1;i<n;i++)
        root = create_node((char[]){ops[i],0}, root, create_node("ID",0,0));

    return root;
}
node* build_right_assoc(char ops[], int n, int i)
{
    if(i==n) return create_node("ID",0,0);

    return create_node((char[]){ops[i],0},
            create_node("ID",0,0),
            build_right_assoc(ops,n,i+1));
}
/* Precedence */
node* build_precedence_alt(char ops[], int n)
{
    node* root = create_node((char[]){ops[1],0},
        create_node((char[]){ops[0],0},
        create_node("ID",0,0),
        create_node("ID",0,0)),
        create_node("ID",0,0));
    for(int i=2;i<n;i++)
        root = create_node((char[]){ops[i],0}, root, create_node("ID",0,0));
    return root;
}

/* Dangling else */
node* build_if_correct()
{
    return create_node("IF",
        create_node("COND",0,0),
        create_node("IF",
        create_node("COND",0,0),
        create_node("ELSE",
        create_node("S1",0,0),
        create_node("S2",0,0))));
}

node* build_if_ambiguous()
{
    return create_node("IF",
        create_node("COND",0,0),
        create_node("ELSE",
        create_node("IF",
        create_node("COND",0,0),
        create_node("S1",0,0)),
        create_node("S2",0,0)));
}

/* ---------- PARSER ---------- */
void match(int t)
{
    if(lookahead==t) lookahead=yylex();
    else { printf("Syntax Error\n"); exit(1); }
}

node* expr(); node* term(); node* factor();
node* expr()
{
    node* left = term();
    while(lookahead==PLUS || lookahead==MINUS)
    {
        int op = lookahead; match(op);
        node* right = term();
        left = create_node(op==PLUS?"+":"-", left, right);
    }
    return left;
}

node* term()
{
    node* left = factor();
    while(lookahead==MUL||lookahead==DIV||lookahead==REM)
    {
        int op=lookahead; match(op);
        node* right=factor();
        left=create_node(op==MUL?"*":op==DIV?"/":"%", left, right);
    }
    return left;
}

node* factor()
{
    if(lookahead==ID){ match(ID); return create_node("ID",0,0); }
    if(lookahead==NUM){ match(NUM); return create_node("NUM",0,0); }
    if(lookahead==LPAREN)
    {
        match(LPAREN);
        node* t=expr();
        match(RPAREN);
        return t;
    }
    printf("Syntax Error in factor\n");
    exit(1);
}

/* ---------- MAIN ---------- */
int main()
{
    int choice;

    printf("====== MENU ======\n");
    printf("1. Grammar Analysis (Left Recursion)\n");
    printf("2. Ambiguity Detection\n");
    printf("Enter choice: ");
    scanf("%d", &choice);
    getchar(); // consume newline

    /* ---------- OPTION 1 ---------- */
    if(choice == 1)
    {
        printf("\n----- Grammar Analysis Phase -----\n");
        read_grammar();
        detect_direct_left_recursion();
        detect_indirect_left_recursion();
        return 0;
    }

    /* ---------- OPTION 2 ---------- */
    else if(choice == 2)
    {
        printf("\n----- Input Analysis Phase -----\n");

        char input[100], ops[50];
        int op_count, id_count;

        printf("Enter expression:\n");
        fgets(input, sizeof(input), stdin);

        extract_all(input, ops, &op_count, &id_count);

        /* 1. Dangling else */
        if(has_dangling_else(input))
        {
            printf("\nDangling Else Ambiguity Detected!\n");

            printf("Tree 1\n");
            print_tree(build_if_correct(),0);

            printf("\nTree 2\n");
            print_tree(build_if_ambiguous(),0);

            return 0;
        }

        /* 2. Precedence */
        if(has_precedence_ambiguity(ops, op_count))
        {
            printf("\nPrecedence Ambiguity Detected!\n");

            FILE* fp = fmemopen(input, strlen(input), "r");
            yyin = fp;

            lookahead = yylex();
            node* t = expr();
            match(SEMI);

            fclose(fp);

            printf("Tree 1\n");
            print_tree(t,0);

            printf("\nTree 2\n");
            print_tree(build_precedence_alt(ops, op_count),0);

            return 0;
        }

        /* 3. Associativity */
        if(id_count >= 3 && op_count >= 2 && is_same_operator(ops, op_count))
        {
            printf("\nAssociativity Ambiguity Detected!\n");

            printf("Tree 1\n");
            print_tree(build_left_assoc(ops, op_count),0);

            printf("\nTree 2\n");
            print_tree(build_right_assoc(ops, op_count, 0),0);

            return 0;
        }

        /* 4. Normal parsing */
        FILE* fp = fmemopen(input, strlen(input), "r");
        yyin = fp;

        lookahead = yylex();
        node* t = expr();
        match(SEMI);

        fclose(fp);

        printf("\nNo Ambiguity Detected\n");
        printf("Parse Tree\n");
        print_tree(t,0);

        return 0;
    }

    else
    {
        printf("Invalid choice\n");
    }

    return 0;
}
