/* TODO: Handle errors during tokenization
 * TODO: Handle errors during parsing
   - Parsing would fail if theren't enough tokens before the first operator
   - Parsing should fail if there are more operators/operands than necessary
     - [(1 2 +) +]
     - [1 (2 3 +)]
   - Division by zero
 * TODO: Make a function to evaluate the AST
 * TODO: Handle errors during parsing
   - Integer overflows
   - Integer underflows
 * TODO: Make a text-based visualizer for RPN expressions
 */

#include <stdio.h>
#include <ctype.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

typedef enum { EC_OK, EC_REACHED_EOF } ErrorCode;

typedef enum {
  TK_NUMBER, TK_OP_PLUS, TK_OP_MINUS, TK_OP_STAR, TK_OP_SLASH
} TokenType;

typedef struct {
  TokenType type;
  union {
    size_t number;
    char string[1024];
  } literal;
} Token;

typedef enum {ABOT_ADD, ABOT_SUBTRACT, ABOT_MULTIPLY, ABOT_DIVIDE} ASTBinaryOpType;

typedef enum {ANT_INTEGER, ANT_BINARY_OP} NodeType;

typedef struct ASTNode {
  NodeType type;
  union {
    size_t integer;
    struct {
      ASTBinaryOpType type;
      char literal[1024];
      struct ASTNode *rhs;
      struct ASTNode *lhs;
    } binary_op;
  } data;
} ASTNode;

ASTNode *make_ast_integer_node(size_t *integer) {
  ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));

  node->type = ANT_INTEGER;

  node->data.integer = *integer;

  return node;
}

ASTNode *make_ast_binary_op_node(ASTBinaryOpType type, char (* literal)[1024], struct ASTNode **left,
                                 struct ASTNode **right) {
  ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));

  node->type = ANT_BINARY_OP;

  node->data.binary_op.type = type;
  snprintf(node->data.binary_op.literal, sizeof(node->data.binary_op.literal), "%.*s", 1, *literal);
  node->data.binary_op.rhs = *right;
  node->data.binary_op.lhs = *left;

  return node;
}

void print_ast_node(struct ASTNode *node, unsigned indent_level) {
  switch (node->type) {
  case ANT_INTEGER:
    printf("%*s[INT NODE: %zu]\n", indent_level * 4, "", node->data.integer);
    break;
  case ANT_BINARY_OP:
    printf("%*s[BOP NODE: %s]\n", indent_level * 4, "",
           node->data.binary_op.literal);
    print_ast_node(node->data.binary_op.lhs, indent_level + 1);
    print_ast_node(node->data.binary_op.rhs, indent_level + 1);
    break;
  }
}

char* shift(char** *argv) {
  return *(*argv)++;
}

void print_usage(char* *program) {
  printf("Usage: %s <code>\n", *program);
}

char* peek(char **code) {
  if (*(*code + 1) != '\0') {
    return *code + 1;
  } else {
    return NULL;
  }
}

ErrorCode consume_until(char **chars, int *offset, int (*predicate)(int)) {
  char cur_char;

  do {
    cur_char = *(*chars + *offset);
    if ((*predicate)(cur_char)) (*offset)++;
    else return EC_OK;
  } while (cur_char != '\0');

  return EC_REACHED_EOF;
}

void tokenize_code(char* code, Token* *tokens) {
  size_t code_length = strlen(code);

  printf("[INFO] Tokenizing code=\"%s\" length=%d\n", code, (int)code_length);

  char *cursor = code;
  size_t cycle = 0;

  while (*cursor != '\0') {
    char cur_char = *cursor;

    if (isdigit(cur_char)) {
      int offset = 0;

      if (consume_until(&cursor, &offset, isdigit) == EC_REACHED_EOF) {
        fprintf(stderr, "ERROR: reached EOF while trying to parse digit\n");
        exit(1);
      }

      Token* temp_token = (Token *) malloc(sizeof(Token));
      temp_token->type = TK_NUMBER;

      char number_str[1024];
      snprintf(number_str, sizeof(number_str), "%.*s", offset, cursor);

      temp_token->literal.number = atoi(number_str);

      arrput(*tokens, *temp_token);

      cursor = cursor + offset;
      continue;
    }

    Token* temp_token = (Token *) malloc(sizeof(Token));

    switch (cur_char) {
    case '+': temp_token->type = TK_OP_PLUS;
    case '-': temp_token->type = TK_OP_MINUS;
    case '*': temp_token->type = TK_OP_STAR;
    case '/':
      temp_token->type = TK_OP_SLASH;
      snprintf(temp_token->literal.string, sizeof(temp_token->literal.string), "%.*s", 1, cursor);
      arrput(*tokens, *temp_token);
      break;
    case ' ':
    case '\t':
    case '\n':
      free(temp_token);
      break;
    default:
      fprintf(stderr, "Unknown character: '%c'\n", cur_char);
      free(temp_token);
    }

    cursor++;
    cycle++;
  }
}

void parse_tokens(Token **tokens) {
  ASTNode **stack = NULL;

  for (int i = 0; i < arrlen(*tokens); i++) {
    Token token = (*tokens)[i];
    ASTNode* node;
    ASTBinaryOpType abot_type;

    switch (token.type) {
    case TK_NUMBER:
      printf("Found a number: %zu\n", token.literal.number);
      node = make_ast_integer_node(&(token.literal.number));
      arrput(stack, node);
      break;
    case TK_OP_PLUS:
      printf("Found a PLUS (+) operator: %s\n", token.literal.string);
      abot_type = ABOT_ADD;
    case TK_OP_MINUS:
      printf("Found a MINUS (-) operator: %s\n", token.literal.string);
      abot_type = ABOT_SUBTRACT;
    case TK_OP_STAR:
      printf("Found a STAR (*) operator: %s\n", token.literal.string);
      abot_type = ABOT_MULTIPLY;
    case TK_OP_SLASH:
      printf("Found a SLASH (/) operator: %s\n", token.literal.string);
      abot_type = ABOT_DIVIDE;

      ASTNode *right = arrpop(stack);
      ASTNode *left = arrpop(stack);

      node = make_ast_binary_op_node(abot_type, &(token.literal.string), &left, &right);

      arrput(stack, node);

      break;
    }

    printf("------------------\n");
    printf("[#%d AST ITERATION]\n", i);
    for (int j = 0; j < arrlen(stack); j++) {
      print_ast_node(stack[j], 0);
    }
    printf("------------------\n");
  }
}

int main(int argc, char** argv) {
  // [INFO] argv[argc] is NULL (C standard)

  char* program_name = shift(&argv);

  char* code = shift(&argv);
  if (!code) {
    printf("[ERROR] <code> was not provided\n");
    print_usage(&program_name);
    exit(EXIT_FAILURE);
  }

  printf("[INFO] Provided code is \"%s\"\n", code);

  Token* tokens = NULL;

  tokenize_code(code, &tokens);

  for (int i = 0; i < arrlen(tokens); i++) {
    Token token = tokens[i];
    if (token.type != TK_NUMBER) {
      printf("[#%d] Token with type=%d, literal='%s'\n", i, token.type,
             token.literal.string);
    } else {
      printf("[#%d] Token with type=%d, literal=%zu\n", i, token.type,
             token.literal.number);
    }
  }

  parse_tokens(&tokens);

  return 0;
}
