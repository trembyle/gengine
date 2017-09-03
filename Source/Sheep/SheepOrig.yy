%{
#include <cstdlib>
#include <string>
#include "SheepScript.h"
#define YYSTYPE SheepNode*

char* removeQuotes(char* str)
{
	if(str[0] == '"')
	{
		str[strlen(str)-1] = 0;
		return str+1;
	}
	else // assume "|< >|" strings
	{
		str[strlen(str)-2] = 0;
		return str+2;
	}
}
%}

/* Tell bison to generate C++ output */
%skeleton "lalr1.cc"

/* We use features that require Bison 3.0+ */
%require "3.0"

/* Tell bison to write tokens to .hh file */
%defines

/* Tell bison to output parser named Sheep::Parser, instead of yy::parser */
%define api.namespace {Sheep}
%define parser_class_name {Parser}

/* When enabled w/ variants option, creates an actual "symbol" internal class. */
%define api.token.constructor

/* Use variant implementation of semantic values - fancy! */
%define api.value.type variant

/* Helps catch invalid uses. */
%define parse.assert

/* This code is added to the top of the .hh file for the parser. */
%code requires 
{
	namespace Sheep
	{
		class Scanner;
		class Driver;
	}
}

%param { Sheep::Scanner& scanner }
%param { Sheep::Driver& driver }

/* This code is added to the top of the .cc file for the parser. */
%code
{
	#include "SheepDriver.h"

	// When requesting yylex, redirect to scanner.yylex.
	#undef yylex
	#define yylex scanner.yylex

	// The C++ yytext is const, but we need to modify to remove
	// quotes in some cases. So, this is a bit of a hack.
	#undef yytext
	#define yytext scanner.GetYYText()

	void Sheep::Parser::error(const location_type& loc, const std::string& msg)
	{
		driver.error(loc, msg);
	}
}

/* Causes location to be tracked, and yylloc value is populated for use. */
%locations
/* TODO: Add %initial-action block to set filename for location variable */

%token END 0 "end of file"

/* section keywords */
%token CODE SYMBOLS

/* logical branching */
%token IF ELSE GOTO RETURN

/* various other symbols for a c-like language */
%token DOLLAR COMMA COLON SEMICOLON QUOTE
%token OPENPAREN CLOSEPAREN OPENBRACKET CLOSEBRACKET

/* pretty critical keyword for scripting synchronization (like a Unity coroutine?) */
%token WAIT

/* Some other keywords outlined in the language doc, but not sure if they are used */
%token YIELD EXPORT BREAKPOINT SITNSPIN

/* variable types */
%token INTVAR FLOATVAR STRINGVAR

/* constant types */
%token <int> INT
%token <float> FLOAT
%token <std::string> STRING
/*%token INT FLOAT STRING*/

/* symbol dictating a user-defined name (variable or function) */
%token <std::string> USERNAME
%token <std::string> SYSNAME
/*%token USERNAME SYSNAME*/

%type <int> int_exp
%type <float> float_exp

/* %left dictates left associativity, which means multiple operators at once will group
   to the left first. Ex: x OP y OP z with '%left OP' means ((x OP y) OP z).

   Also, the order here defines precendence of associativity. The ones declared later
   will group FIRST. Ex: NOT is the lowest on the list so that it will group before
   anything else.

   For pretty much ALL operators, we want left associativity 

   Since Sheep is C-like, we base the precendences off of C (http://en.cppreference.com/w/c/language/operator_precedence)
   */
%right ASSIGN
%left OR
%left AND
%left EQUAL NOTEQUAL
%left LT LTE GT GTE
%left PLUS MINUS
%left MULTIPLY DIVIDE MOD
%right NOT NEGATE

%start script

%%

/* Grammer rules */

/* ROOT: script is either empty, both symbols and code sections, or just symbols, or just code.  */
/*
script: /* empty */
	| symbols code { }
	| symbols { }
	| code { }
	;

/* Symbols section is empty (symbols { }) or has contents in it. */
symbols: SYMBOLS OPENBRACKET CLOSEBRACKET { } /* empty symbols section */
	| SYMBOLS OPENBRACKET symbols_contents CLOSEBRACKET { }
	;

/* Contents of symbols section is one or more lines of symbols. */
symbols_contents: symbols_line { }
	| symbols_contents symbols_line { }
	;

/* Each symbols line contains one or more declarations. */
symbols_line: symbol_decl { }
	| symbols_line symbol_decl { }
	;

/* Symbol declaration must ALWAYS begin with a symbol type, but we can have multiple symbols declared.
   For example, we must account for something like "int var1 = 25, var2, var3 = 10;" */
symbol_decl: symbol_type symbol_decl_list SEMICOLON { }
	;

/* From above example, we support just the var name, var name with initial assignment,
   and list-like variants of this. */
symbol_decl_list: user_name { }
	| user_name ASSIGN constant { }
	| symbol_decl_list COMMA user_name { }
	| symbol_decl_list COMMA user_name ASSIGN constant { }
	;

/* Code section is either empty (code { }) or has contents in it. */
code: CODE OPENBRACKET CLOSEBRACKET { } /* empty code section */
	| CODE OPENBRACKET code_contents CLOSEBRACKET { }
	;

code_contents: functions_list { }
	;

functions_list: function { }
	| functions_list function { }
	;

function: user_name OPENPAREN function_args_branch CLOSEPAREN OPENBRACKET statement_list CLOSEBRACKET { }
	;

function_args_branch: /* empty */ { }
	| function_args { }
	;

function_args: symbol_type user_name { } 
	| function_args COMMA symbol_type user_name { }
	;

statement_list: statement { }
	| statement_list statement { }
	;

statement: open_statement { }
	| closed_statement { }
	;

open_statement: IF OPENPAREN expression CLOSEPAREN statement { }
	| IF OPENPAREN expression CLOSEPAREN closed_statement ELSE open_statement { }
	;

closed_statement: simple_statement { }
	| OPENBRACKET CLOSEBRACKET { }
	| OPENBRACKET statement_list CLOSEBRACKET { }
	| IF OPENPAREN expression CLOSEPAREN closed_statement ELSE closed_statement { }
	;

simple_statement: SEMICOLON { }
	| user_name COLON { }
	| GOTO user_name SEMICOLON { }
	| expression SEMICOLON { }
	| RETURN SEMICOLON { }
	| RETURN expression SEMICOLON { }
	| wait_statement { }
	| user_name ASSIGN expression { }
	;

wait_statement: WAIT SEMICOLON { }
	| WAIT system_function_call SEMICOLON { }
	| WAIT OPENBRACKET system_function_call_list SEMICOLON CLOSEBRACKET
	;

system_function_call: sys_name OPENPAREN CLOSEPAREN { $$ = $1; }
	| sys_name OPENPAREN args_list CLOSEPAREN { $$ = $1; $$->SetChild(0, $3); }
	;

system_function_call_list: system_function_call { $$ = $1; }
	| system_function_call_list SEMICOLON system_function_call { $$->SetSibling($3); }
	;

args_list: expression { $$ = $1; }
	| args_list COMMA expression { $$->SetSibling($3); }
	;

expression: constant { }
	| system_function_call { $$ = $1; }
	| user_name { $$ = $1; }
	| OPENPAREN expression CLOSEPAREN { $$ = $2; }
	| NOT expression { $$ = SheepNode::CreateOperation(SheepOperationType::Not); 
		$$->SetChild(0, $1); }
	| NEGATE expression { $$ = SheepNode::CreateOperation(SheepOperationType::Negate); 
		$$->SetChild(0, $1); }
	| expression PLUS expression { $$ = SheepNode::CreateOperation(SheepOperationType::Add); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression MINUS expression { $$ = SheepNode::CreateOperation(SheepOperationType::Subtract); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression MULTIPLY expression { $$ = SheepNode::CreateOperation(SheepOperationType::Multiply); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression DIVIDE expression { $$ = SheepNode::CreateOperation(SheepOperationType::Divide); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression LT expression { $$ = SheepNode::CreateOperation(SheepOperationType::LessThan); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression LTE expression { $$ = SheepNode::CreateOperation(SheepOperationType::LessThanOrEqual); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression GT expression { $$ = SheepNode::CreateOperation(SheepOperationType::GreaterThan); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression GTE expression { $$ = SheepNode::CreateOperation(SheepOperationType::GreaterThanOrEqual); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression EQUAL expression { $$ = SheepNode::CreateOperation(SheepOperationType::AreEqual); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression NOTEQUAL expression { $$ = SheepNode::CreateOperation(SheepOperationType::NotEqual); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression OR expression { $$ = SheepNode::CreateOperation(SheepOperationType::LogicalOr); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	| expression AND expression { $$ = SheepNode::CreateOperation(SheepOperationType::LogicalAnd); 
		$$->SetChild(0, $1); $$->SetChild(1, $3); }
	;

expression : constant { $$ = $1; }
	| OPENPAREN expression CLOSEPAREN { $$ = $2 }
	
	/* Maths */
	| 
	;

user_name: USERNAME { $$ = $1; }
	;

sys_name: SYSNAME { $$ = $1; }
	;

num_exp: int_exp { $$ = $1; }
	| float_exp { $$ = $1; }
	;
*/

script: /* empty */
	| expression { std::cout << $1 << std::endl; }
	; 

expression: constant { $$ = $1; }
	| constant PLUS constant { $$ = $1 + $2; }
	;

/* All possible constants: int, float, string */
constant: INT { $$ = $1; }
	| FLOAT { $$ = $1; }
	| STRING { $$ = removeQuotes($1); }
	;

/*
user_name: USERNAME { $$ = SheepNode::CreateNameRef(yytext, false); }
sys_name: SYSNAME { $$ = SheepNode::CreateNameRef(yytext, true); }
*/

/* All possible symbol types: int, float, string */
/*
symbol_type: INTVAR { $$ = SheepNode::CreateTypeRef(SheepReferenceType::Int); }
	| FLOATVAR { $$ = SheepNode::CreateTypeRef(SheepReferenceType::Float); }
	| STRINGVAR { $$ = SheepNode::CreateTypeRef(SheepReferenceType::String); }
	;
*/


