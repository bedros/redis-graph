#include "query_executor.h"
#include "parser/grammar.h"

QE_FilterNode* BuildFiltersTree(const FilterNode* root) {
	
	QE_FilterNode* filterNode = (QE_FilterNode*)malloc(sizeof(QE_FilterNode));

    if(root->t == N_PRED) {
    	CmpFunc compareFunc = NULL;
	    
	    // Find out which compare function should we use.
	    switch(root->pn.val.type) {
	      case T_STRING:
	        compareFunc = cmp_string;
	        break;
	      case T_INT32:
	        compareFunc = cmp_int;
	        break;
	      case T_INT64:
	        compareFunc = cmp_long;
	        break;
	      case T_UINT:
	        compareFunc = cmp_uint;
	        break;
	      case T_BOOL:
	        compareFunc = cmp_int;
	        break;
	      case T_FLOAT:
	        compareFunc = cmp_float;
	        break;
	      case T_DOUBLE:
	        compareFunc = cmp_double;
	        break;
	    }

	    // Couldn't figure out which compare function to use.
	    if(compareFunc == NULL) {
	    	// ERROR.
	    	return NULL;
	    }

	    // Create predicate node
	    filterNode->t = QE_N_PRED;
	    
	    filterNode->pred.alias = 
	    	(char*)malloc(sizeof(char) * (strlen(root->pn.alias) + 1));

	    filterNode->pred.property = 
	    	(char*)malloc(sizeof(char) * (strlen(root->pn.property) + 1));
	    
	    strcpy(filterNode->pred.alias, root->pn.alias);
	    strcpy(filterNode->pred.property, root->pn.property);

	    filterNode->pred.op = root->pn.op;
	    filterNode->pred.val = root->pn.val; // Not sure about this assignmeant

	    filterNode->pred.cf = compareFunc;
	    return filterNode;
	}

    // root->t == N_COND

    // Create condition node
	filterNode->t = QE_N_COND;
	filterNode->cond.op = root->cn.op;
	
	filterNode->cond.left = BuildFiltersTree(root->cn.left);
	filterNode->cond.right = BuildFiltersTree(root->cn.right);
	return filterNode;
}

int applyFilters(RedisModuleCtx *ctx, Triplet* result, char** aliases, QE_FilterNode* root) {
    // Scan tree.
    if(root->t == QE_N_PRED) {
        char* elementID = NULL;
        
        if (strcmp(aliases[0], root->pred.alias) == 0) {
            elementID = result->subject;
        }
        if (strcmp(aliases[2], root->pred.alias) == 0) {
            elementID = result->object;
        }

        return applyFilter(ctx, elementID, &root->pred);
    }

    // root->t == QE_N_COND

    // Visit left subtree
    int pass = applyFilters(ctx, result, aliases, root->cond.left);
    
    if(root->cond.op == AND && pass == 1) {
        // Visit right subtree
        pass *= applyFilters(ctx, result, aliases, root->cond.right);
    }

    if(root->cond.op == OR && pass == 0) {
        // Visit right subtree
        pass = applyFilters(ctx, result, aliases, root->cond.right);   
    }

    return pass;
}

// Applies a single filter to a single result.
int applyFilter(RedisModuleCtx *ctx, const char* elementID, QE_PredicateNode* node) {
	RedisModuleString* keyStr =
        RedisModule_CreateString(ctx, elementID, strlen(elementID));

    RedisModuleString* elementProp =
        RedisModule_CreateString(ctx, node->property, strlen(node->property));

    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyStr, REDISMODULE_READ);
    
    if(key == NULL) {
        RedisModule_FreeString(ctx, keyStr);
        RedisModule_FreeString(ctx, elementProp);
        return 0;
    }

    RedisModuleString* propValue;

    RedisModule_HashGet(key, REDISMODULE_HASH_NONE, elementProp, &propValue, NULL);

    RedisModule_CloseKey(key);
    RedisModule_FreeString(ctx, keyStr);
    RedisModule_FreeString(ctx, elementProp);

    size_t strProplen;
    const char* strPropValue = RedisModule_StringPtrLen(propValue, &strProplen);
    RedisModule_FreeString(ctx, propValue);

    SIValue propVal = {.type = node->val.type};

    if (!SI_ParseValue(&propVal, strPropValue, strProplen)) {
      RedisModule_Log(ctx, "error", "Could not parse %.*s\n", (int)strProplen, strPropValue);
      return RedisModule_ReplyWithError(ctx, "Invalid value given");
    }

    // Relation between prop value and value.
    int rel = node->cf(&propVal, &node->val);
    switch(node->op) {
        case EQ:
            return rel == 0;

        case GT:
            return rel > 0;

        case GE:
            return rel >= 0;

        case LT:
            return rel < 0;

        case LE:
            return rel <= 0;

        default:
            RedisModule_Log(ctx, "error", "Unknown comparison operator %d\n", node->op);
            return RedisModule_ReplyWithError(ctx, "Invalid comparison operator");
    }
}