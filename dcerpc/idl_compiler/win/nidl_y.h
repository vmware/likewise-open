
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ALIGN_KW = 258,
     BYTE_KW = 259,
     CHAR_KW = 260,
     CONST_KW = 261,
     DEFAULT_KW = 262,
     ENUM_KW = 263,
     EXCEPTIONS_KW = 264,
     FLOAT_KW = 265,
     HYPER_KW = 266,
     INT_KW = 267,
     INTERFACE_KW = 268,
     IMPORT_KW = 269,
     LIBRARY_KW = 270,
     LONG_KW = 271,
     PIPE_KW = 272,
     REF_KW = 273,
     SMALL_KW = 274,
     STRUCT_KW = 275,
     TYPEDEF_KW = 276,
     UNION_KW = 277,
     UNSIGNED_KW = 278,
     SHORT_KW = 279,
     VOID_KW = 280,
     DOUBLE_KW = 281,
     BOOLEAN_KW = 282,
     CASE_KW = 283,
     SWITCH_KW = 284,
     HANDLE_T_KW = 285,
     TRUE_KW = 286,
     FALSE_KW = 287,
     NULL_KW = 288,
     BROADCAST_KW = 289,
     COMM_STATUS_KW = 290,
     CONTEXT_HANDLE_KW = 291,
     FIRST_IS_KW = 292,
     HANDLE_KW = 293,
     IDEMPOTENT_KW = 294,
     IGNORE_KW = 295,
     CALL_AS_KW = 296,
     IID_IS_KW = 297,
     IMPLICIT_HANDLE_KW = 298,
     IN_KW = 299,
     LAST_IS_KW = 300,
     LENGTH_IS_KW = 301,
     LOCAL_KW = 302,
     MAX_IS_KW = 303,
     MAYBE_KW = 304,
     MIN_IS_KW = 305,
     MUTABLE_KW = 306,
     OUT_KW = 307,
     OBJECT_KW = 308,
     POINTER_DEFAULT_KW = 309,
     ENDPOINT_KW = 310,
     PTR_KW = 311,
     RANGE_KW = 312,
     REFLECT_DELETIONS_KW = 313,
     REMOTE_KW = 314,
     SECURE_KW = 315,
     SHAPE_KW = 316,
     SIZE_IS_KW = 317,
     STRING_KW = 318,
     SWITCH_IS_KW = 319,
     SWITCH_TYPE_KW = 320,
     TRANSMIT_AS_KW = 321,
     UNIQUE_KW = 322,
     UUID_KW = 323,
     VERSION_KW = 324,
     V1_ARRAY_KW = 325,
     V1_STRING_KW = 326,
     V1_ENUM_KW = 327,
     V1_STRUCT_KW = 328,
     CPP_QUOTE_KW = 329,
     UUID_REP = 330,
     COLON = 331,
     COMMA = 332,
     DOTDOT = 333,
     EQUAL = 334,
     LBRACE = 335,
     LBRACKET = 336,
     LPAREN = 337,
     RBRACE = 338,
     RBRACKET = 339,
     RPAREN = 340,
     SEMI = 341,
     STAR = 342,
     QUESTION = 343,
     BAR = 344,
     BARBAR = 345,
     LANGLE = 346,
     LANGLEANGLE = 347,
     RANGLE = 348,
     RANGLEANGLE = 349,
     AMP = 350,
     AMPAMP = 351,
     LESSEQUAL = 352,
     GREATEREQUAL = 353,
     EQUALEQUAL = 354,
     CARET = 355,
     PLUS = 356,
     MINUS = 357,
     NOT = 358,
     NOTEQUAL = 359,
     SLASH = 360,
     PERCENT = 361,
     TILDE = 362,
     POUND = 363,
     UNKNOWN = 364,
     IDENTIFIER = 365,
     STRING = 366,
     INTEGER_NUMERIC = 367,
     CHAR = 368,
     FLOAT_NUMERIC = 369
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 71 "nidl_y.y"

	 NAMETABLE_id_t         y_id ;          /* Identifier           */
	 long                   y_ptrlevels;	/* levels of * for pointers */
	 long					y_ptrclass;		/* class of pointer */
	 STRTAB_str_t           y_string ;      /* String               */
	 STRTAB_str_t           y_float ;       /* Float constant       */
	 AST_export_n_t*        y_export ;      /* an export node       */
	 AST_import_n_t*        y_import ;      /* Import node          */
	 AST_exception_n_t*     y_exception ;   /* Exception node       */
	 AST_constant_n_t*      y_constant;     /* Constant node        */
	 AST_parameter_n_t*     y_parameter ;   /* Parameter node       */
	 AST_type_n_t*          y_type ;        /* Type node            */
	 AST_type_p_n_t*        y_type_ptr ;    /* Type pointer node    */
	 AST_field_n_t*         y_field ;       /* Field node           */
	 AST_arm_n_t*           y_arm ;         /* Union variant arm    */
	 AST_operation_n_t*     y_operation ;   /* Routine node         */
	 AST_interface_n_t*     y_interface ;   /* Interface node       */
	 AST_case_label_n_t*    y_label ;       /* Union tags           */
	 ASTP_declarator_n_t*   y_declarator ;  /* Declarator info      */
	 ASTP_array_index_n_t*  y_index ;       /* Array index info     */
	 nidl_uuid_t            y_uuid ;        /* Universal UID        */
	 char                   y_char;         /* character constant   */
	 ASTP_attributes_t      y_attributes;   /* attributes flags     */

     	 AST_cpp_quote_n_t*     y_cpp_quote;    /* Quoted C within interface treated as one 'kind' of export node + quote outside interfaces */
	 

	 struct {
		  long            int_val ;        /* Integer constant     */
		  AST_type_k_t    int_size;
		  int             int_signed;
	 }                  y_int_info;     /* int size and signedness */
	 AST_exp_n_t           * y_exp;          /* constant expression info */



/* Line 1676 of yacc.c  */
#line 203 "nidl_y.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE nidl_yylval;


