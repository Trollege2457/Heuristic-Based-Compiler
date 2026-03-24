#define MAX_RULES 20
#define MAX_PROD  20
#define MAX_LEN   50
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char nonterminals[MAX_RULES];
char productions[MAX_RULES][MAX_PROD][MAX_LEN];
int prod_count[MAX_RULES];
int rule_count = 0;

void read_grammar()
{
    FILE *fp = fopen("grammar.txt", "r");
    if(!fp)
    {
        printf("Error opening grammar file\n");
        exit(1);
    }

    char line[100];
    rule_count = 0;

    while(fgets(line, sizeof(line), fp))
    {
        nonterminals[rule_count] = line[0];

        char *token = strtok(line+3, "|\n");
        int j = 0;

        while(token)
        {
            strcpy(productions[rule_count][j++], token);
            token = strtok(NULL, "|\n");
        }

        prod_count[rule_count++] = j;
    }

    fclose(fp);
}
void detect_direct_left_recursion()
{
    for(int i = 0; i < rule_count; i++)
    {
        for(int j = 0; j < prod_count[i]; j++)
        {
            if(productions[i][j][0] == nonterminals[i])
            {
                printf("Direct Left Recursion in %c -> %s\n",
                       nonterminals[i], productions[i][j]);
            }
        }
    }
}

int visited[MAX_RULES];

int find_index(char nt)
{
    for(int i=0;i<rule_count;i++)
        if(nonterminals[i]==nt) return i;
    return -1;
}

int dfs(int start, int current)
{
    if(visited[current]) return 0;
    visited[current] = 1;

    for(int j=0;j<prod_count[current];j++)
    {
        char first = productions[current][j][0];

        int idx = find_index(first);
        if(idx == -1) continue;

        if(idx == start) return 1;

        if(dfs(start, idx)) return 1;
    }
    return 0;
}

void detect_indirect_left_recursion()
{
    for(int i=0;i<rule_count;i++)
    {
        for(int k=0;k<rule_count;k++) visited[k]=0;

        if(dfs(i,i))
        {
            printf("Indirect Left Recursion involving %c\n", nonterminals[i]);
        }
    }
}
