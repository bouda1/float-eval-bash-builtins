/*
 * =====================================================================================
 *
 *       Filename:  float-eval.c
 *
 *    Description:  Transform a string with arithmetic operations to a double
 *
 *        Version:  1.0
 *        Created:  02/07/2015 03:54:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Corentin Peuvrel (cpe), cpeuvrel@exosec.fr
 *
 * =====================================================================================
 */

#include "float_eval.h"
static double computeAst(binTree* ast);
static void tokenify(char *str, binTree* ast, char* op, int pass, int start, char prevOp);
static void writeOp(char* target, char* str);

int float_eval_builtin(WORD_LIST *list)
{
    char res[SIZE_SLOT] = "";
    int flags = 0;

    if (!HAS_WORD(list)) {
        builtin_usage();
        return EX_USAGE;
    }

    while (HAS_WORD(list->next)) {
        if (strncmp("-v", list->word->word, SIZE_SLOT) == 0 ||
            strncmp("--verbose", list->word->word, SIZE_SLOT) == 0)
            flags |= FLOAT_OPT_VERBOSE;

        list = list->next;
    }

    strncpy(res, list->word->word , SIZE_SLOT);
    snprintf(res, SIZE_SLOT,"%f", float_eval(res, flags));

    if (bind_variable("REPLY", res, 0) == NULL) {
        builtin_error("Failed to bind variable: REPLY");
        return EXECUTION_FAILURE;
    }

    return EXECUTION_SUCCESS;
}

double float_eval(char* str, int flags)
{
    double res = 0;
    binTree *ast = malloc(sizeof(binTree));
    initBinTree(ast);

    tokenify(str, ast, "", 0, 1, 0);

    res = computeAst(ast);
    if (flags & FLOAT_OPT_VERBOSE)
        printBinTree(ast);

    freeBinTree(ast);

    return res;
}

static double computeAst(binTree* ast)
{
    //TODO: Check endptr to see if we miss sth
    if (!ast)
        return 0;
    else if (! isOp(ast->val[0]) || (ast->val[0] == '-' && ast->val[1]))
        return strtod(ast->val, NULL);

    switch (ast->val[0]) {
        case '+':
            return computeAst(ast->next1) + computeAst(ast->next2);
        case '-':
            return computeAst(ast->next1) - computeAst(ast->next2);
        case '*':
            return computeAst(ast->next1) * computeAst(ast->next2);
        case '/':
            return computeAst(ast->next1) / computeAst(ast->next2);
        case '%':
            return (int)computeAst(ast->next1) % (int)computeAst(ast->next2);
        case '&':
            return (int)computeAst(ast->next1) & (int)computeAst(ast->next2);
        case '^':
            return (int)computeAst(ast->next1) ^ (int)computeAst(ast->next2);
        case '|':
            return (int)computeAst(ast->next1) | (int)computeAst(ast->next2);
    }
    return 0;
}

static void tokenify(char *str, binTree* ast, char* op, int pass, int start, char prevOp)
{
    int pos_curr = 0, parentheses = 0, found_next_op = 0;
    char curr[SIZE_SLOT] = {0}, tmp[SIZE_SLOT] = {0};
    binTree *fst = ast;

    int beginParentheses = str[0] == '(' ? 1 : 0;

    int beginSub = str[0] == '-' ? 1 : 0;
    if (beginSub) {
        if (start && str[1] == '(') {
            snprintf(tmp, SIZE_SLOT,"0%s", str);
            beginParentheses = 0;
            beginSub = 0;
            strncpy(str, tmp , SIZE_SLOT);
            str[SIZE_SLOT-1] = 0;
        }
        else if (pass == 3 && (start || op[0] != 0))
            str++;
    }

    if (!start) {
        fst = findFirstEmpty(ast);

        if (op[0] == 0 && !beginParentheses) {
            snprintf(fst->val, SIZE_SLOT,"%s", str);
            return;
        }
        writeOp(fst->val, op);
    }

    for (;;str++) {
        switch (*str) {
            case '(':
                if ( parentheses == 0 && beginParentheses)
                    beginParentheses++;
                parentheses++;
                break;
            case ')':
                parentheses--;
                break;
            case '\t':
            case ' ':
                continue;
        }

        if (parentheses == 0 && isOpPrioAbove(str,pass))
            found_next_op++;

        if (*str != '\0' && ((*str >= '0' && *str <= '9') || parentheses != 0 ||
                *str == '.' ||
                (*str == ')' && (beginParentheses != 2 || *(str+1))) ||
                (*str != ')' && !isOpCurrentPrio(str, pass)))) {
            curr[pos_curr++] = *str;
            continue;
        }

        curr[pos_curr] = '\0';

        if (prevOp && *str && (*str == '%' || prevOp == '%') ) {
            binTree *newRoot = malloc(sizeof(binTree));
            initBinTree(newRoot);
            binTree *fstTmp = malloc(sizeof(binTree));
            initBinTree(fstTmp);
            binTree *t = fst;

            t = findNodeToSwapModulo(fst, NULL);

            strncpy(fstTmp->val, t->val, SIZE_SLOT);
            fstTmp->next1 = t->next1;
            fstTmp->next2 = t->next2;

            newRoot->next1 = fstTmp;

            strncpy(t->val, newRoot->val, SIZE_SLOT);
            t->next1 = newRoot->next1;
            t->next2 = newRoot->next2;

            free(newRoot);
        }

        if (beginSub && pass == 3) {
            beginSub = 0;
            snprintf(tmp, SIZE_SLOT,"-%s", curr);
            tokenify(tmp, ast, str, 0, 0, 0);
        }
        else if (beginParentheses == 2 && *str == ')')
            tokenify(1+curr, fst, "", 0, 1, 0);
        else if (*str == '\0' && found_next_op)
            tokenify(curr, ast, "", ++pass, 1, 0);
        else if (start || (!start && curr[0]))
            tokenify(curr, fst, str, 0, 0, prevOp);

        pos_curr = 0 ;

        if (isOp(*str))
           prevOp = *str;

        if (! *str)
            break;
    }
}

int isOpPrioAbove(char* op, int prio)
{
    int res = 0;

    if ((op[0] >= '0' && op[0] <= '9') || op[0] == '.' || op[0] == '\0')
        return 0;

    while ((res = isOpCurrentPrio(op,prio++)) == 0) {}

    if (res == -1)
        return 0;
    return 1;

}

int isOpCurrentPrio(char* op, int prio)
{
    char* opPrio[SIZE_SLOT] = {
        "|",
        "^",
        "&",
        "+-",
        "*/%",
        ""
    };
    int i = 0;

    if ((op[0] >= '0' && op[0] <= '9') || op[0] == '.' || op[0] == '\0')
        return 0;

    if (!opPrio[prio][0])
       return -1;

    while (opPrio[prio][i]) {
        if (op[0] == opPrio[prio][i++])
            return 1;
    }

    return 0;
}

binTree* findNodeToSwapModulo(binTree* t, binTree* save)
{
    binTree* left = NULL;
    if (save == NULL)
        save = t;

    if (!(left = findFirstLeftEmpty(t->next1)) && !isOpCurrentPrio(t->val, 4))
        return findNodeToSwapModulo(t->next2, NULL);

    if (left)
        return findNodeToSwapModulo(t->next1, NULL);
    else if (t->next2)
        return findNodeToSwapModulo(t->next2, save);

    return save;
}

static void writeOp(char* target, char* str)
{
    int i = 0;
    char op2[32][3] = {
        "||",
        "&&",
        ""
    };

    for (i = 0; op2[i][0] ; i++) {
        if (strncmp(op2[i], str , 2) == 0){
            strncpy(target, op2[i] , 2);
            target[2] = 0;
            return;
        }
    }

    target[0] = str[0];
    target[1] = 0;
}
