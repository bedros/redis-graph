#ifndef _AST_ARITHMETIC_EXPRESSION_H
#define _AST_ARITHMETIC_EXPRESSION_H

#include "../rmutil/vector.h"
#include "../value.h"

/* ArExpNodeType lists the type of nodes within
 * an arithmetic expression tree. */
typedef enum {
    AST_AR_EXP_OP,
    AST_AR_EXP_OPERAND,
} AST_ArithmeticExpression_NodeType;

typedef enum {
    AST_AR_EXP_CONSTANT,
    AST_AR_EXP_VARIADIC,
} AST_ArithmeticExpression_OperandNodeType;

typedef struct {
    char *function; /* Name of operation. */
	Vector *args;	/* Vector of AST_ArithmeticExpressionNode pointers. */
} AST_ArithmeticExpressionOP;

/* OperandNode represents either a constant numeric value, 
 * or a graph entity property. */
typedef struct {
    union {
        SIValue constant;
        struct {
			char *alias;
			char *property;
		} variadic;
    };
    AST_ArithmeticExpression_OperandNodeType type;
} AST_ArithmeticExpressionOperand;

typedef struct {
	union {
		AST_ArithmeticExpressionOperand operand;
        AST_ArithmeticExpressionOP op;
    };
    AST_ArithmeticExpression_NodeType type;
} AST_ArithmeticExpressionNode;

AST_ArithmeticExpressionNode* New_AST_AR_EXP_VariableOperandNode(char* alias, char *property);
AST_ArithmeticExpressionNode* New_AST_AR_EXP_ConstOperandNode(SIValue constant);
AST_ArithmeticExpressionNode* New_AST_AR_EXP_OpNode(char *func, Vector *args);
void Free_AST_ArithmeticExpressionNode(AST_ArithmeticExpressionNode *arExpNode);

#endif
