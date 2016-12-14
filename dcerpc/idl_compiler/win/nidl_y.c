
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         nidl_yyparse
#define yylex           nidl_yylex
#define yyerror         nidl_yyerror
#define yylval          nidl_yylval
#define yychar          nidl_yychar
#define yydebug         nidl_yydebug
#define yynerrs         nidl_yynerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "nidl_y.y"

/*
 *
 *  (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 *  (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 *  (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *                  permission to use, copy, modify, and distribute this
 *  file for any purpose is hereby granted without fee, provided that
 *  the above copyright notices and this notice appears in all source
 *  code copies, and that none of the names of Open Software
 *  Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 *  Corporation be used in advertising or publicity pertaining to
 *  distribution of the software without specific, written prior
 *  permission.  Neither Open Software Foundation, Inc., Hewlett-
 *  Packard Company, nor Digital Equipment Corporation makes any
 *  representations about the suitability of this software for any
 *  purpose.
 *
 */
/*
**
**  NAME:
**
**      IDL.Y
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      This module defines the main IDL grammar accepted
**      by the IDL compiler.
**
**  VERSION: DCE 1.0
**
*/

#ifdef vms
#  include <types.h>
#else
#  include <sys/types.h>
#endif

#include <nidl.h>
#include <nametbl.h>
#include <errors.h>
#include <ast.h>
#include <astp.h>
#include <frontend.h>
#include <flex_bison_support.h>

extern int nidl_yylineno;
extern boolean search_attributes_table ;

int yyparse(void);
int yylex(void);

/*
**  Local cells used for inter-production communication
*/
static ASTP_attr_k_t       ASTP_bound_type;    /* Array bound attribute */



/* Line 189 of yacc.c  */
#line 149 "nidl_y.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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

/* Line 214 of yacc.c  */
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



/* Line 214 of yacc.c  */
#line 336 "nidl_y.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */

/* Line 264 of yacc.c  */
#line 106 "nidl_y.y"

#if YYDEBUG
extern char const *current_file;
static void yyprint(FILE * stream, int token, YYSTYPE lval)	{
	fprintf(stream, " %s:%d", current_file, *yylineno_p);
}
#define YYPRINT yyprint
#endif




/* Line 264 of yacc.c  */
#line 361 "nidl_y.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  16
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   684

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  115
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  118
/* YYNRULES -- Number of rules.  */
#define YYNRULES  289
/* YYNRULES -- Number of states.  */
#define YYNSTATES  484

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   369

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    12,    14,    17,    18,
      21,    26,    30,    31,    34,    35,    39,    41,    44,    48,
      50,    51,    54,    56,    59,    62,    66,    70,    72,    76,
      78,    80,    84,    87,    90,    93,    95,    98,   103,   109,
     111,   114,   119,   121,   123,   125,   127,   129,   131,   133,
     135,   137,   139,   141,   143,   145,   147,   149,   151,   153,
     154,   156,   157,   159,   161,   162,   164,   166,   168,   170,
     172,   175,   178,   180,   183,   186,   189,   191,   193,   195,
     197,   199,   201,   204,   212,   216,   225,   234,   244,   247,
     251,   255,   257,   261,   263,   267,   269,   272,   274,   278,
     280,   283,   285,   289,   292,   296,   302,   304,   309,   314,
     320,   323,   325,   329,   335,   339,   341,   342,   347,   349,
     353,   356,   359,   360,   363,   365,   369,   371,   373,   376,
     378,   381,   383,   386,   390,   393,   396,   400,   404,   410,
     416,   422,   428,   433,   436,   441,   443,   445,   447,   451,
     452,   457,   461,   463,   464,   466,   468,   470,   472,   474,
     479,   483,   484,   486,   490,   491,   494,   497,   503,   509,
     514,   516,   521,   523,   525,   531,   537,   539,   541,   543,
     545,   547,   549,   553,   555,   559,   561,   563,   568,   573,
     575,   577,   579,   581,   583,   585,   587,   591,   593,   594,
     596,   598,   601,   604,   605,   609,   612,   615,   617,   621,
     623,   625,   627,   629,   631,   633,   638,   640,   642,   647,
     649,   654,   659,   665,   667,   669,   671,   673,   675,   677,
     679,   686,   688,   690,   695,   700,   705,   710,   712,   717,
     722,   727,   729,   731,   733,   735,   741,   743,   747,   749,
     753,   755,   759,   761,   765,   767,   771,   773,   777,   781,
     783,   787,   791,   795,   799,   801,   805,   809,   811,   815,
     819,   821,   825,   829,   833,   835,   837,   840,   843,   846,
     849,   852,   856,   858,   860,   862,   864,   866,   868,   870
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     116,     0,    -1,   117,   118,    -1,   127,    -1,    -1,   117,
     119,    -1,   119,    -1,   134,   118,    -1,    -1,   118,   120,
      -1,   123,   121,   122,   124,    -1,   198,    13,   110,    -1,
      -1,    76,   110,    -1,    -1,    80,   125,    83,    -1,     1,
      -1,     1,    83,    -1,   126,   132,   145,    -1,   128,    -1,
      -1,   128,   118,    -1,   129,    -1,   128,   129,    -1,    14,
       1,    -1,    14,     1,    86,    -1,    14,   130,    86,    -1,
     131,    -1,   130,    77,   131,    -1,   111,    -1,   133,    -1,
     132,   145,   133,    -1,   137,    86,    -1,   135,    86,    -1,
     186,    86,    -1,   134,    -1,     1,    86,    -1,    74,    82,
     111,    85,    -1,     6,   139,   181,    79,   136,    -1,   218,
      -1,    21,   138,    -1,   214,   139,   180,   144,    -1,   140,
      -1,   141,    -1,   143,    -1,   149,    -1,   150,    -1,   151,
      -1,   152,    -1,   153,    -1,   142,    -1,   154,    -1,   170,
      -1,   157,    -1,   173,    -1,   178,    -1,   110,    -1,    10,
      -1,    26,    -1,    -1,    77,    -1,    -1,    86,    -1,    23,
      -1,    -1,    19,    -1,    24,    -1,    16,    -1,    11,    -1,
     147,    -1,    23,   147,    -1,   147,    23,    -1,   148,    -1,
     148,    12,    -1,   146,    12,    -1,   146,     5,    -1,    27,
      -1,     4,    -1,    25,    -1,    30,    -1,    80,    -1,    83,
      -1,    22,   158,    -1,    22,    29,    82,   140,   110,    85,
     159,    -1,    22,   110,   158,    -1,    22,    29,    82,   140,
     110,    85,   110,   159,    -1,    22,   110,    29,    82,   140,
     110,    85,   159,    -1,    22,   110,    29,    82,   140,   110,
      85,   110,   159,    -1,    22,   110,    -1,   155,   160,   156,
      -1,   155,   161,   156,    -1,   162,    -1,   160,   145,   162,
      -1,   163,    -1,   161,   145,   163,    -1,   168,    -1,   165,
     169,    -1,   166,    -1,   164,    77,   166,    -1,   167,    -1,
     165,   167,    -1,   136,    -1,    28,   136,    76,    -1,     7,
      76,    -1,   193,   215,    86,    -1,   193,   215,   139,   181,
      86,    -1,    86,    -1,   214,   139,   181,    86,    -1,    20,
     155,   171,   156,    -1,    20,   110,   155,   171,   156,    -1,
      20,   110,    -1,   172,    -1,   171,   145,   172,    -1,   214,
     139,   197,   180,    86,    -1,     8,   174,   175,    -1,   110,
      -1,    -1,    80,   176,   144,    83,    -1,   177,    -1,   176,
      77,   177,    -1,   110,   179,    -1,    17,   139,    -1,    -1,
      79,   218,    -1,   181,    -1,   180,    77,   181,    -1,   182,
      -1,   184,    -1,   183,   184,    -1,    87,    -1,    87,   183,
      -1,   110,    -1,   184,   185,    -1,    82,   181,    85,    -1,
     184,   187,    -1,    81,    84,    -1,    81,    87,    84,    -1,
      81,   136,    84,    -1,    81,    87,    78,    87,    84,    -1,
      81,    87,    78,   136,    84,    -1,    81,   136,    78,    87,
      84,    -1,    81,   136,    78,   136,    84,    -1,   214,   139,
     180,   144,    -1,     1,   180,    -1,   188,   190,   144,   189,
      -1,    82,    -1,    85,    -1,   191,    -1,   190,    77,   191,
      -1,    -1,   214,   139,   197,   192,    -1,     1,   197,   192,
      -1,   181,    -1,    -1,    81,    -1,    84,    -1,    82,    -1,
      85,    -1,   214,    -1,   193,   199,   144,   194,    -1,   193,
       1,   194,    -1,    -1,   200,    -1,   199,    77,   200,    -1,
      -1,    68,     1,    -1,    68,    75,    -1,    55,    82,   204,
     144,    85,    -1,     9,    82,   205,   144,    85,    -1,    69,
      82,   203,    85,    -1,    47,    -1,    54,    82,   202,    85,
      -1,    53,    -1,   201,    -1,    43,    82,    30,   110,    85,
      -1,    43,    82,   110,   110,    85,    -1,    18,    -1,    56,
      -1,    67,    -1,   112,    -1,   114,    -1,   206,    -1,   204,
      77,   206,    -1,   207,    -1,   205,    77,   207,    -1,   111,
      -1,   110,    -1,   209,   195,   210,   196,    -1,   212,   195,
     213,   196,    -1,    37,    -1,    45,    -1,    46,    -1,    48,
      -1,    50,    -1,    62,    -1,   211,    -1,   210,    77,   211,
      -1,   218,    -1,    -1,    64,    -1,   110,    -1,    87,   110,
      -1,   193,   215,    -1,    -1,   216,   144,   194,    -1,     1,
     194,    -1,     1,    86,    -1,   217,    -1,   216,    77,   217,
      -1,   208,    -1,    34,    -1,    49,    -1,    39,    -1,    58,
      -1,    47,    -1,    41,    82,   110,    85,    -1,    56,    -1,
      44,    -1,    44,    82,    61,    85,    -1,    52,    -1,    52,
      82,    61,    85,    -1,    42,    82,   110,    85,    -1,    42,
      82,    87,   110,    85,    -1,    70,    -1,    63,    -1,    71,
      -1,    67,    -1,    18,    -1,    40,    -1,    36,    -1,    57,
      82,   218,    77,   218,    85,    -1,    73,    -1,    72,    -1,
       3,    82,    19,    85,    -1,     3,    82,    24,    85,    -1,
       3,    82,    16,    85,    -1,     3,    82,    11,    85,    -1,
      38,    -1,    66,    82,   140,    85,    -1,    65,    82,   140,
      85,    -1,    28,    82,   164,    85,    -1,     7,    -1,   110,
      -1,   219,    -1,   220,    -1,   220,    88,   218,    76,   219,
      -1,   221,    -1,   220,    90,   221,    -1,   222,    -1,   221,
      96,   222,    -1,   223,    -1,   222,    89,   223,    -1,   224,
      -1,   223,   100,   224,    -1,   225,    -1,   224,    95,   225,
      -1,   226,    -1,   225,    99,   226,    -1,   225,   104,   226,
      -1,   227,    -1,   226,    91,   227,    -1,   226,    93,   227,
      -1,   226,    97,   227,    -1,   226,    98,   227,    -1,   228,
      -1,   227,    92,   228,    -1,   227,    94,   228,    -1,   229,
      -1,   228,   101,   229,    -1,   228,   102,   229,    -1,   230,
      -1,   229,    87,   230,    -1,   229,   105,   230,    -1,   229,
     106,   230,    -1,   231,    -1,   232,    -1,   101,   232,    -1,
     102,   232,    -1,   107,   232,    -1,   103,   232,    -1,    87,
     232,    -1,    82,   218,    85,    -1,   112,    -1,   113,    -1,
     110,    -1,   111,    -1,    33,    -1,    31,    -1,    32,    -1,
     114,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   261,   261,   267,   269,   285,   287,   293,   300,   306,
     315,   324,   336,   339,   347,   368,   370,   375,   383,   396,
     398,   404,   418,   419,   428,   432,   436,   443,   444,   455,
     470,   471,   481,   485,   490,   496,   501,   508,   516,   525,
     536,   543,   553,   554,   558,   559,   560,   561,   562,   563,
     564,   565,   570,   571,   572,   573,   578,   585,   589,   595,
     597,   601,   603,   608,   609,   613,   618,   623,   628,   636,
     638,   643,   651,   653,   655,   663,   668,   673,   678,   683,
     688,   695,   703,   713,   722,   731,   740,   749,   758,   765,
     771,   778,   779,   787,   788,   797,   803,   810,   811,   819,
     820,   829,   835,   839,   846,   851,   859,   863,   886,   890,
     894,   901,   902,   911,   921,   928,   931,   935,   942,   943,
     952,   959,   967,   970,   978,   982,   993,   998,  1000,  1009,
    1011,  1016,  1018,  1024,  1028,  1056,  1061,  1066,  1071,  1076,
    1081,  1086,  1095,  1104,  1112,  1119,  1126,  1134,  1135,  1143,
    1149,  1183,  1191,  1194,  1209,  1216,  1223,  1230,  1242,  1262,
    1263,  1268,  1272,  1273,  1274,  1278,  1282,  1291,  1297,  1303,
    1312,  1320,  1327,  1333,  1340,  1348,  1358,  1359,  1360,  1364,
    1371,  1388,  1389,  1393,  1397,  1406,  1413,  1431,  1436,  1444,
    1448,  1452,  1456,  1460,  1464,  1472,  1473,  1483,  1488,  1494,
    1501,  1505,  1515,  1519,  1527,  1528,  1538,  1552,  1555,  1574,
    1577,  1579,  1581,  1583,  1585,  1587,  1590,  1592,  1594,  1598,
    1600,  1604,  1609,  1616,  1618,  1620,  1622,  1624,  1626,  1628,
    1630,  1637,  1639,  1641,  1644,  1647,  1650,  1653,  1655,  1660,
    1667,  1672,  1675,  1691,  1696,  1698,  1705,  1707,  1714,  1716,
    1723,  1725,  1732,  1734,  1741,  1743,  1750,  1752,  1756,  1764,
    1766,  1770,  1774,  1778,  1786,  1788,  1792,  1800,  1802,  1807,
    1814,  1816,  1826,  1830,  1837,  1842,  1844,  1848,  1852,  1856,
    1860,  1867,  1869,  1875,  1879,  1883,  1887,  1892,  1897,  1901
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ALIGN_KW", "BYTE_KW", "CHAR_KW",
  "CONST_KW", "DEFAULT_KW", "ENUM_KW", "EXCEPTIONS_KW", "FLOAT_KW",
  "HYPER_KW", "INT_KW", "INTERFACE_KW", "IMPORT_KW", "LIBRARY_KW",
  "LONG_KW", "PIPE_KW", "REF_KW", "SMALL_KW", "STRUCT_KW", "TYPEDEF_KW",
  "UNION_KW", "UNSIGNED_KW", "SHORT_KW", "VOID_KW", "DOUBLE_KW",
  "BOOLEAN_KW", "CASE_KW", "SWITCH_KW", "HANDLE_T_KW", "TRUE_KW",
  "FALSE_KW", "NULL_KW", "BROADCAST_KW", "COMM_STATUS_KW",
  "CONTEXT_HANDLE_KW", "FIRST_IS_KW", "HANDLE_KW", "IDEMPOTENT_KW",
  "IGNORE_KW", "CALL_AS_KW", "IID_IS_KW", "IMPLICIT_HANDLE_KW", "IN_KW",
  "LAST_IS_KW", "LENGTH_IS_KW", "LOCAL_KW", "MAX_IS_KW", "MAYBE_KW",
  "MIN_IS_KW", "MUTABLE_KW", "OUT_KW", "OBJECT_KW", "POINTER_DEFAULT_KW",
  "ENDPOINT_KW", "PTR_KW", "RANGE_KW", "REFLECT_DELETIONS_KW", "REMOTE_KW",
  "SECURE_KW", "SHAPE_KW", "SIZE_IS_KW", "STRING_KW", "SWITCH_IS_KW",
  "SWITCH_TYPE_KW", "TRANSMIT_AS_KW", "UNIQUE_KW", "UUID_KW", "VERSION_KW",
  "V1_ARRAY_KW", "V1_STRING_KW", "V1_ENUM_KW", "V1_STRUCT_KW",
  "CPP_QUOTE_KW", "UUID_REP", "COLON", "COMMA", "DOTDOT", "EQUAL",
  "LBRACE", "LBRACKET", "LPAREN", "RBRACE", "RBRACKET", "RPAREN", "SEMI",
  "STAR", "QUESTION", "BAR", "BARBAR", "LANGLE", "LANGLEANGLE", "RANGLE",
  "RANGLEANGLE", "AMP", "AMPAMP", "LESSEQUAL", "GREATEREQUAL",
  "EQUALEQUAL", "CARET", "PLUS", "MINUS", "NOT", "NOTEQUAL", "SLASH",
  "PERCENT", "TILDE", "POUND", "UNKNOWN", "IDENTIFIER", "STRING",
  "INTEGER_NUMERIC", "CHAR", "FLOAT_NUMERIC", "$accept", "grammar_start",
  "interfaces", "cpp_quotes", "interface_plus", "interface",
  "interface_start", "interface_ancestor", "interface_init",
  "interface_tail", "interface_body", "optional_imports",
  "optional_imports_cppquotes", "imports", "import", "import_files",
  "import_file", "exports", "export", "cpp_quote", "const_dcl",
  "const_exp", "type_dcl", "type_declarator", "type_spec",
  "simple_type_spec", "constructed_type_spec", "named_type_spec",
  "floating_point_type_spec", "extraneous_comma", "extraneous_semi",
  "optional_unsigned_kw", "integer_size_spec", "integer_modifiers",
  "integer_type_spec", "char_type_spec", "boolean_type_spec",
  "byte_type_spec", "void_type_spec", "handle_type_spec",
  "push_name_space", "pop_name_space", "union_type_spec", "ne_union_body",
  "union_body", "ne_union_cases", "union_cases", "ne_union_case",
  "union_case", "ne_union_case_list", "union_case_list",
  "ne_union_case_label", "union_case_label", "ne_union_member",
  "union_member", "struct_type_spec", "member_list", "member",
  "enum_type_spec", "optional_tag", "enum_body", "enum_ids", "enum_id",
  "pipe_type_spec", "optional_value", "declarators", "declarator",
  "declarator1", "pointer", "direct_declarator", "array_bounds",
  "operation_dcl", "parameter_dcls", "param_names", "end_param_names",
  "param_list", "param_dcl", "declarator_or_null", "attribute_opener",
  "attribute_closer", "bounds_opener", "bounds_closer",
  "old_attribute_syntax", "interface_attributes", "interface_attr_list",
  "interface_attr", "acf_interface_attr", "pointer_class",
  "version_number", "port_list", "excep_list", "port_spec", "excep_spec",
  "fp_attribute", "array_bound_type", "array_bound_id_list",
  "array_bound_id", "neu_switch_type", "neu_switch_id", "attributes",
  "rest_of_attribute_list", "attribute_list", "attribute", "expression",
  "conditional_expression", "logical_OR_expression",
  "logical_AND_expression", "inclusive_OR_expression",
  "exclusive_OR_expression", "AND_expression", "equality_expression",
  "relational_expression", "shift_expression", "additive_expression",
  "multiplicative_expression", "cast_expression", "unary_expression",
  "primary_expression", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   115,   116,   116,   116,   117,   117,   118,   118,   119,
     120,   121,   122,   122,   123,   124,   124,   124,   125,   126,
     126,   127,   128,   128,   129,   129,   129,   130,   130,   131,
     132,   132,   133,   133,   133,   133,   133,   134,   135,   136,
     137,   138,   139,   139,   140,   140,   140,   140,   140,   140,
     140,   140,   141,   141,   141,   141,   142,   143,   143,   144,
     144,   145,   145,   146,   146,   147,   147,   147,   147,   148,
     148,   148,   149,   149,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   157,   157,   157,   157,   157,   157,   158,
     159,   160,   160,   161,   161,   162,   163,   164,   164,   165,
     165,   166,   167,   167,   168,   168,   169,   169,   170,   170,
     170,   171,   171,   172,   173,   174,   174,   175,   176,   176,
     177,   178,   179,   179,   180,   180,   181,   182,   182,   183,
     183,   184,   184,   184,   184,   185,   185,   185,   185,   185,
     185,   185,   186,   186,   187,   188,   189,   190,   190,   190,
     191,   191,   192,   192,   193,   194,   195,   196,   197,   198,
     198,   198,   199,   199,   199,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   201,   201,   202,   202,   202,   203,
     203,   204,   204,   205,   205,   206,   207,   208,   208,   209,
     209,   209,   209,   209,   209,   210,   210,   211,   211,   212,
     213,   213,   214,   214,   215,   215,   215,   216,   216,   217,
     217,   217,   217,   217,   217,   217,   217,   217,   217,   217,
     217,   217,   217,   217,   217,   217,   217,   217,   217,   217,
     217,   217,   217,   217,   217,   217,   217,   217,   217,   217,
     217,   217,   217,   218,   219,   219,   220,   220,   221,   221,
     222,   222,   223,   223,   224,   224,   225,   225,   225,   226,
     226,   226,   226,   226,   227,   227,   227,   228,   228,   228,
     229,   229,   229,   229,   230,   231,   231,   231,   231,   231,
     231,   232,   232,   232,   232,   232,   232,   232,   232,   232
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     2,     1,     2,     0,     2,
       4,     3,     0,     2,     0,     3,     1,     2,     3,     1,
       0,     2,     1,     2,     2,     3,     3,     1,     3,     1,
       1,     3,     2,     2,     2,     1,     2,     4,     5,     1,
       2,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     0,     1,     1,     0,     1,     1,     1,     1,     1,
       2,     2,     1,     2,     2,     2,     1,     1,     1,     1,
       1,     1,     2,     7,     3,     8,     8,     9,     2,     3,
       3,     1,     3,     1,     3,     1,     2,     1,     3,     1,
       2,     1,     3,     2,     3,     5,     1,     4,     4,     5,
       2,     1,     3,     5,     3,     1,     0,     4,     1,     3,
       2,     2,     0,     2,     1,     3,     1,     1,     2,     1,
       2,     1,     2,     3,     2,     2,     3,     3,     5,     5,
       5,     5,     4,     2,     4,     1,     1,     1,     3,     0,
       4,     3,     1,     0,     1,     1,     1,     1,     1,     4,
       3,     0,     1,     3,     0,     2,     2,     5,     5,     4,
       1,     4,     1,     1,     5,     5,     1,     1,     1,     1,
       1,     1,     3,     1,     3,     1,     1,     4,     4,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     0,     1,
       1,     2,     2,     0,     3,     2,     2,     1,     3,     1,
       1,     1,     1,     1,     1,     4,     1,     1,     4,     1,
       4,     4,     5,     1,     1,     1,     1,     1,     1,     1,
       6,     1,     1,     4,     4,     4,     4,     1,     4,     4,
       4,     1,     1,     1,     1,     5,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     1,     1,     2,     2,     2,     2,
       2,     3,     1,     1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       8,     0,     0,     0,     8,    14,     6,     3,     8,    22,
       8,    24,    29,     0,    27,     0,     1,    14,     5,     9,
     161,    21,    23,     7,    25,     0,    26,     0,   154,    12,
       0,     0,    28,    37,     0,     0,     0,     0,     0,   170,
     172,     0,     0,     0,     0,    59,   162,   173,     0,    13,
      16,    20,    10,   155,   160,     0,     0,     0,     0,   165,
     166,     0,    60,     0,    11,    17,     0,     0,    19,   186,
      59,   183,     0,     0,   176,   177,   178,     0,   185,    59,
     181,   179,   180,     0,   163,   159,    15,     0,    64,   203,
      61,    30,    35,     0,     0,     0,     0,    64,    60,     0,
       0,     0,   171,    60,     0,   169,     0,    36,   129,   131,
     143,   124,   126,     0,   127,    77,   116,    57,    68,    67,
      64,    65,     0,     0,    63,    66,    78,    58,    76,    79,
      56,     0,    42,    43,    50,    44,     0,    69,    72,    45,
      46,    47,    48,    49,    51,    53,    52,    54,    55,    40,
      64,    62,     0,    33,    32,    34,     0,     0,   241,   227,
       0,   210,   229,   189,   237,   212,   228,     0,     0,   217,
     190,   191,   214,   192,   211,   193,   219,   216,     0,   213,
     194,   224,   199,     0,     0,   226,   223,   225,   232,   231,
     242,   209,     0,     0,   202,    59,   207,     0,   184,   168,
     174,   175,   182,   167,     0,   130,     0,   128,     0,   145,
     132,   134,     0,   115,     0,   121,    80,   110,   203,     0,
      88,     0,    82,    70,     0,    75,    74,    71,    73,     0,
      31,   206,   205,     0,     0,     0,     0,     0,     0,     0,
      64,    64,   156,   198,     0,    60,     0,    59,   133,   125,
     287,   288,   286,     0,   135,     0,     0,     0,     0,     0,
     284,   285,   282,   283,   289,     0,    39,   243,   244,   246,
     248,   250,   252,   254,   256,   259,   264,   267,   270,   274,
     275,   203,    59,   147,    64,     0,   114,   203,    61,   111,
      64,    64,     0,    84,    61,    91,    95,     0,     0,    59,
       0,     0,     0,     0,     0,   101,     0,    97,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   195,   197,     0,
     200,     0,   208,   204,    60,   142,     0,     0,   136,   280,
     276,   277,   279,   278,     0,   137,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   153,   158,     0,     0,   203,
     122,    59,   118,    61,    81,   203,   108,   203,     0,    64,
       0,    89,    64,    38,    41,   236,   235,   233,   234,     0,
     240,   215,     0,   221,   218,   220,     0,   239,   238,   198,
     157,   187,   201,   188,   281,     0,     0,     0,     0,     0,
     247,   249,   251,   253,   255,   257,   258,   260,   261,   262,
     263,   265,   266,   268,   269,   271,   272,   273,   152,   151,
     148,   146,   144,   153,     0,   120,    60,     0,   109,   112,
       0,     0,     0,    92,   104,     0,    98,   222,     0,   196,
     138,   139,   140,   141,     0,   150,   123,   119,   117,     0,
       0,     0,     0,   230,   245,   113,     0,     0,    83,     0,
     105,    85,     0,     0,    61,    93,   203,    99,     0,    86,
     103,     0,     0,    90,   106,   100,    96,    64,    87,   102,
      94,     0,     0,   107
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     5,     6,    19,    29,    35,    20,    52,
      66,    67,     7,     8,     9,    13,    14,    90,    91,    10,
      93,   305,    94,   149,   131,   132,   133,   134,   135,    63,
     365,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     457,   366,   145,   222,   458,   294,   464,   295,   465,   306,
     466,   307,   467,   296,   476,   146,   288,   289,   147,   214,
     286,   361,   362,   148,   425,   110,   111,   112,   113,   114,
     210,    95,   211,   212,   422,   282,   283,   419,    96,    54,
     243,   391,   355,    31,    45,    46,    47,    77,    83,    79,
      70,    80,    71,   191,   192,   316,   317,   193,   321,   290,
     194,   195,   196,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -306
static const yytype_int16 yypact[] =
{
      26,     9,   -59,   103,   -12,  -306,  -306,  -306,    19,  -306,
     -12,    54,  -306,    -7,  -306,   -38,  -306,   166,  -306,  -306,
      38,  -306,  -306,  -306,  -306,    78,  -306,    92,  -306,   117,
     588,   188,  -306,  -306,   108,    27,   123,   131,   161,  -306,
    -306,   177,   195,    34,   201,   210,  -306,  -306,   183,  -306,
     211,   285,  -306,  -306,  -306,   191,   -18,    77,   194,  -306,
    -306,     3,   486,   123,  -306,  -306,   219,   363,   285,  -306,
     231,  -306,   205,   206,  -306,  -306,  -306,   235,  -306,   244,
    -306,  -306,  -306,   237,  -306,  -306,  -306,    66,   287,    38,
     238,  -306,  -306,   239,   243,   249,   208,   287,   191,   251,
     252,   255,  -306,   194,   265,  -306,    59,  -306,   264,  -306,
     277,  -306,  -306,   -46,   143,  -306,   245,  -306,  -306,  -306,
     287,  -306,   -34,   -13,   154,  -306,  -306,  -306,  -306,  -306,
    -306,    59,  -306,  -306,  -306,  -306,   187,   333,   345,  -306,
    -306,  -306,  -306,  -306,  -306,  -306,  -306,  -306,  -306,  -306,
     287,  -306,   322,  -306,  -306,  -306,   149,   276,  -306,  -306,
     278,  -306,  -306,  -306,  -306,  -306,  -306,   279,   280,   281,
    -306,  -306,  -306,  -306,  -306,  -306,   283,  -306,   284,  -306,
    -306,  -306,  -306,   288,   290,  -306,  -306,  -306,  -306,  -306,
    -306,  -306,   294,   294,  -306,   282,  -306,    59,  -306,  -306,
    -306,  -306,  -306,  -306,   307,  -306,    59,   143,   414,  -306,
    -306,  -306,    33,  -306,   324,  -306,  -306,   328,    38,   329,
      32,    38,  -306,  -306,   339,  -306,  -306,  -306,  -306,    59,
    -306,  -306,  -306,   179,   471,   309,   -45,   360,   361,   471,
     112,   112,  -306,   471,   -39,   420,   123,   347,  -306,  -306,
    -306,  -306,  -306,   471,  -306,   516,    73,    73,    73,    73,
    -306,  -306,  -306,  -306,  -306,    97,  -306,  -306,   151,   330,
     336,   331,   338,    98,   130,   190,   136,    62,  -306,  -306,
    -306,    38,   357,  -306,   287,   325,  -306,    38,   133,  -306,
     287,   112,   354,  -306,   133,  -306,  -306,   208,   471,   347,
     355,   356,   358,   364,    73,  -306,    44,  -306,   365,   332,
     366,   367,   368,   362,   370,   378,    79,  -306,  -306,   379,
    -306,   389,  -306,  -306,    59,  -306,   395,   504,  -306,  -306,
    -306,  -306,  -306,  -306,   537,  -306,   471,   471,   471,   471,
     471,   471,   471,   471,   471,   471,   471,   471,   471,   471,
     471,   471,   471,   471,   471,    59,  -306,   390,   403,    38,
     415,   422,  -306,   133,  -306,    38,  -306,    38,   387,   112,
      38,  -306,    64,  -306,  -306,  -306,  -306,  -306,  -306,   471,
    -306,  -306,   421,  -306,  -306,  -306,   471,  -306,  -306,   471,
    -306,  -306,  -306,  -306,  -306,   397,   428,   570,   429,   438,
     330,   336,   331,   338,    98,   130,   130,   190,   190,   190,
     190,   136,   136,    62,    62,  -306,  -306,  -306,  -306,  -306,
    -306,  -306,  -306,    59,   471,  -306,   325,   435,  -306,  -306,
      59,   434,   410,  -306,  -306,    59,  -306,  -306,   437,  -306,
    -306,  -306,  -306,  -306,   471,  -306,  -306,  -306,  -306,    85,
     -33,   446,   419,  -306,  -306,  -306,   328,   106,  -306,   -11,
    -306,  -306,   447,   471,   133,  -306,   119,  -306,   328,  -306,
    -306,   456,   106,  -306,  -306,  -306,  -306,   287,  -306,  -306,
    -306,    59,   448,  -306
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -306,  -306,  -306,   202,   534,  -306,  -306,  -306,  -306,  -306,
    -306,  -306,  -306,   491,    43,  -306,   518,  -306,   392,   -54,
    -306,  -202,  -306,  -306,   -96,  -209,  -306,  -306,  -306,   -65,
     -86,  -306,   426,  -306,  -306,  -306,  -306,  -306,  -306,  -306,
     -93,  -285,  -306,   326,  -305,  -306,  -306,   175,    80,  -306,
    -306,   172,    90,  -306,  -306,  -306,   270,   196,  -306,  -306,
    -306,  -306,   134,  -306,  -306,  -190,  -104,  -306,   451,   449,
    -306,  -306,  -306,  -306,  -306,  -306,   207,   140,   -17,   -55,
     372,   246,  -187,  -306,  -306,   509,  -306,  -306,  -306,  -306,
    -306,   463,   477,  -306,  -306,  -306,   198,  -306,  -306,   -67,
     291,  -306,   334,  -228,   132,  -306,   240,   242,   253,   250,
     254,   -80,  -115,   -63,   -62,   -85,  -306,  -238
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -204
static const yytype_int16 yytable[] =
{
      97,   197,   204,    30,   152,    99,   265,   247,    85,   371,
      11,   313,    72,    92,   104,   318,   219,   329,   330,   331,
     332,   333,   150,    15,   215,   326,    -4,   224,    50,   218,
     221,   314,   315,     1,   281,    59,   106,  -203,  -203,   299,
       1,  -203,   309,  -203,  -203,  -203,   216,   216,   319,  -203,
    -203,    22,  -203,  -203,   229,  -203,  -203,  -203,  -203,  -203,
    -203,   292,     2,  -203,   109,   310,   329,   216,   115,   216,
      25,   320,   116,    27,   117,   118,   217,   456,   428,    26,
     119,   120,   368,   121,   122,    97,   123,   124,   125,   126,
     127,   128,    73,     2,   129,    74,   373,   220,    92,   468,
       2,   232,   249,    16,   250,   251,   252,    51,   399,    60,
    -149,    22,   216,   462,    28,    81,   115,    82,  -149,    28,
      12,   379,   117,   118,   287,   396,   462,   221,   119,   380,
     246,   121,   398,    75,   463,   124,   125,   126,   127,   128,
      24,   106,   129,  -203,    76,   284,   108,   463,   106,   352,
     434,   461,   107,   108,   469,   253,   389,   329,   438,   329,
     432,   318,   206,   478,   390,   118,    -2,   353,   354,   109,
     119,   455,   423,   121,   130,   334,   109,    33,   125,   473,
     430,   335,   325,   260,   261,   262,   263,   264,   359,    12,
     300,   323,   225,    34,   367,   301,   446,   342,   302,   226,
      28,    48,   343,   303,   297,   474,    17,    53,   370,   156,
      21,   157,    23,    55,   356,   158,   364,   358,    49,   151,
     249,   344,   130,   345,   208,   209,   159,   346,   347,   407,
     408,   409,   410,    53,   374,   231,   160,   350,   351,   336,
     449,   337,   161,    56,   162,   163,   164,   165,   166,   167,
     168,   418,   169,   170,   171,   172,   173,   174,   175,    57,
     176,   471,   405,   406,   177,   178,   179,   415,   416,   417,
     180,   181,   182,   183,   184,   185,   435,    58,   186,   187,
     188,   189,   348,    61,   349,   411,   412,    62,   413,   414,
     284,   115,   356,    64,    65,   116,   427,   117,   118,     1,
     356,    69,    86,   119,   120,    78,   121,   122,    98,   123,
     124,   125,   126,   127,   128,   100,   101,   129,   190,   418,
     102,   103,   105,    87,   151,   153,  -203,  -203,    88,   154,
    -203,   452,  -203,  -203,  -203,   155,   199,   200,  -203,  -203,
     201,  -203,  -203,    89,  -203,  -203,  -203,  -203,  -203,  -203,
     203,   108,  -203,   297,   206,   213,   227,   228,   233,   245,
     234,   235,   236,   237,    87,   238,   239,  -203,  -203,    88,
     240,  -203,   241,  -203,  -203,  -203,   242,   482,   472,  -203,
    -203,   481,  -203,  -203,    89,  -203,  -203,  -203,  -203,  -203,
    -203,   281,   248,  -203,  -203,  -203,     2,   130,  -203,   477,
    -203,  -203,  -203,    28,   285,   -18,  -203,  -203,   216,  -203,
    -203,   291,  -203,  -203,  -203,  -203,  -203,  -203,   298,   308,
    -203,   311,   312,   157,   324,   339,   338,   158,   250,   251,
     252,   340,  -203,   341,   357,   360,   369,     2,   159,   386,
     375,   376,   382,   377,    28,   250,   251,   252,   160,   378,
     381,   383,   384,   385,   161,   387,   162,   163,   164,   165,
     166,   167,   168,   388,   169,   170,   171,   172,   173,   174,
     175,    28,   176,  -203,   390,   -60,   177,   178,   179,   253,
     394,   440,   180,   181,   182,   183,   184,   185,   421,   392,
     186,   187,   188,   189,   424,    37,   253,   431,   254,   426,
    -203,   255,   250,   251,   252,   460,   437,   260,   261,   262,
     263,   264,   441,   443,   444,   256,   257,   258,   448,   450,
     451,   259,   453,   470,   260,   261,   262,   263,   264,    38,
     190,   459,   479,    39,   483,   250,   251,   252,    18,    40,
      41,    42,    68,    32,   230,   433,   293,   250,   251,   252,
     223,   436,   480,   253,    43,    44,   475,   363,   304,   205,
     447,   429,   207,   445,   420,   244,   202,   393,   250,   251,
     252,    84,   256,   257,   258,   198,   454,   400,   259,   322,
     401,   260,   261,   262,   263,   264,   253,   439,   372,    36,
     403,   395,   402,     0,   327,   404,     0,    37,   253,     0,
     328,   250,   251,   252,     0,   256,   257,   258,     0,     0,
       0,   259,     0,     0,   260,   261,   262,   263,   264,   253,
       0,     0,     0,     0,   397,     0,   260,   261,   262,   263,
     264,    38,     0,     0,     0,    39,     0,     0,   256,   257,
     258,    40,    41,    42,   259,     0,     0,   260,   261,   262,
     263,   264,   253,     0,   442,     0,    43,    44,     0,     0,
       0,     0,     0,     0,     0,  -164,     0,     0,     0,     0,
       0,     0,  -164,     0,     0,     0,     0,     0,     0,     0,
     260,   261,   262,   263,   264
};

static const yytype_int16 yycheck[] =
{
      67,    97,   106,    20,    90,    70,   208,   197,    63,   294,
       1,   239,    30,    67,    79,   243,    29,   255,   256,   257,
     258,   259,    89,    82,   120,   253,     0,   131,     1,   122,
     123,   240,   241,    14,     1,     1,    82,     4,     5,   229,
      14,     8,    87,    10,    11,    12,    80,    80,    87,    16,
      17,     8,    19,    20,   150,    22,    23,    24,    25,    26,
      27,    29,    74,    30,   110,   110,   304,    80,     4,    80,
      77,   110,     8,   111,    10,    11,   110,   110,   363,    86,
      16,    17,   291,    19,    20,   152,    22,    23,    24,    25,
      26,    27,   110,    74,    30,    18,   298,   110,   152,   110,
      74,   156,   206,     0,    31,    32,    33,    80,   336,    75,
      77,    68,    80,     7,    81,   112,     4,   114,    85,    81,
     111,    77,    10,    11,   217,   327,     7,   220,    16,    85,
     195,    19,   334,    56,    28,    23,    24,    25,    26,    27,
      86,    82,    30,   110,    67,   212,    87,    28,    82,    87,
      86,   456,    86,    87,   459,    82,    77,   395,   386,   397,
     369,   389,    77,   468,    85,    11,     0,   105,   106,   110,
      16,    86,   359,    19,   110,    78,   110,    85,    24,   464,
     367,    84,   247,   110,   111,   112,   113,   114,   284,   111,
      11,   246,     5,    76,   290,    16,   424,    99,    19,    12,
      81,    13,   104,    24,   221,    86,     4,    84,   294,     1,
       8,     3,    10,    82,   281,     7,    83,   282,   110,    86,
     324,    91,   110,    93,    81,    82,    18,    97,    98,   344,
     345,   346,   347,    84,   299,    86,    28,   101,   102,    88,
     430,    90,    34,    82,    36,    37,    38,    39,    40,    41,
      42,   355,    44,    45,    46,    47,    48,    49,    50,    82,
      52,   463,   342,   343,    56,    57,    58,   352,   353,   354,
      62,    63,    64,    65,    66,    67,   372,    82,    70,    71,
      72,    73,    92,    82,    94,   348,   349,    77,   350,   351,
     357,     4,   359,   110,    83,     8,   361,    10,    11,    14,
     367,   110,    83,    16,    17,   111,    19,    20,    77,    22,
      23,    24,    25,    26,    27,   110,   110,    30,   110,   423,
      85,    77,    85,     1,    86,    86,     4,     5,     6,    86,
       8,   435,    10,    11,    12,    86,    85,    85,    16,    17,
      85,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      85,    87,    30,   370,    77,   110,    23,    12,    82,    77,
      82,    82,    82,    82,     1,    82,    82,     4,     5,     6,
      82,     8,    82,    10,    11,    12,    82,   481,   464,    16,
      17,   477,    19,    20,    21,    22,    23,    24,    25,    26,
      27,     1,    85,    30,     4,     5,    74,   110,     8,   466,
      10,    11,    12,    81,    80,    83,    16,    17,    80,    19,
      20,    82,    22,    23,    24,    25,    26,    27,    79,   110,
      30,    61,    61,     3,    77,    89,    96,     7,    31,    32,
      33,   100,   110,    95,    77,   110,    82,    74,    18,    77,
      85,    85,   110,    85,    81,    31,    32,    33,    28,    85,
      85,    85,    85,    85,    34,    85,    36,    37,    38,    39,
      40,    41,    42,    85,    44,    45,    46,    47,    48,    49,
      50,    81,    52,   110,    85,    85,    56,    57,    58,    82,
      85,    84,    62,    63,    64,    65,    66,    67,    85,   110,
      70,    71,    72,    73,    79,     9,    82,   110,    84,    77,
     110,    87,    31,    32,    33,    86,    85,   110,   111,   112,
     113,   114,    84,    84,    76,   101,   102,   103,    83,    85,
     110,   107,    85,    76,   110,   111,   112,   113,   114,    43,
     110,    85,    76,    47,    86,    31,    32,    33,     4,    53,
      54,    55,    51,    25,   152,   370,   220,    31,    32,    33,
     124,   379,   472,    82,    68,    69,   466,   287,    87,   108,
     426,   365,   113,   423,   357,   193,   103,   321,    31,    32,
      33,    62,   101,   102,   103,    98,   444,   337,   107,   245,
     338,   110,   111,   112,   113,   114,    82,   389,   297,     1,
     340,    87,   339,    -1,    78,   341,    -1,     9,    82,    -1,
      84,    31,    32,    33,    -1,   101,   102,   103,    -1,    -1,
      -1,   107,    -1,    -1,   110,   111,   112,   113,   114,    82,
      -1,    -1,    -1,    -1,    87,    -1,   110,   111,   112,   113,
     114,    43,    -1,    -1,    -1,    47,    -1,    -1,   101,   102,
     103,    53,    54,    55,   107,    -1,    -1,   110,   111,   112,
     113,   114,    82,    -1,    84,    -1,    68,    69,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    77,    -1,    -1,    -1,    -1,
      -1,    -1,    84,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     110,   111,   112,   113,   114
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    14,    74,   116,   117,   118,   119,   127,   128,   129,
     134,     1,   111,   130,   131,    82,     0,   118,   119,   120,
     123,   118,   129,   118,    86,    77,    86,   111,    81,   121,
     193,   198,   131,    85,    76,   122,     1,     9,    43,    47,
      53,    54,    55,    68,    69,   199,   200,   201,    13,   110,
       1,    80,   124,    84,   194,    82,    82,    82,    82,     1,
      75,    82,    77,   144,   110,    83,   125,   126,   128,   110,
     205,   207,    30,   110,    18,    56,    67,   202,   111,   204,
     206,   112,   114,   203,   200,   194,    83,     1,     6,    21,
     132,   133,   134,   135,   137,   186,   193,   214,    77,   144,
     110,   110,    85,    77,   144,    85,    82,    86,    87,   110,
     180,   181,   182,   183,   184,     4,     8,    10,    11,    16,
      17,    19,    20,    22,    23,    24,    25,    26,    27,    30,
     110,   139,   140,   141,   142,   143,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   157,   170,   173,   178,   138,
     214,    86,   145,    86,    86,    86,     1,     3,     7,    18,
      28,    34,    36,    37,    38,    39,    40,    41,    42,    44,
      45,    46,    47,    48,    49,    50,    52,    56,    57,    58,
      62,    63,    64,    65,    66,    67,    70,    71,    72,    73,
     110,   208,   209,   212,   215,   216,   217,   139,   207,    85,
      85,    85,   206,    85,   181,   183,    77,   184,    81,    82,
     185,   187,   188,   110,   174,   139,    80,   110,   155,    29,
     110,   155,   158,   147,   181,     5,    12,    23,    12,   139,
     133,    86,   194,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,   195,   195,    77,   144,   180,    85,   181,
      31,    32,    33,    82,    84,    87,   101,   102,   103,   107,
     110,   111,   112,   113,   114,   136,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,     1,   190,   191,   214,    80,   175,   155,   171,   172,
     214,    82,    29,   158,   160,   162,   168,   193,    79,   180,
      11,    16,    19,    24,    87,   136,   164,   166,   110,    87,
     110,    61,    61,   218,   140,   140,   210,   211,   218,    87,
     110,   213,   217,   194,    77,   144,   218,    78,    84,   232,
     232,   232,   232,   232,    78,    84,    88,    90,    96,    89,
     100,    95,    99,   104,    91,    93,    97,    98,    92,    94,
     101,   102,    87,   105,   106,   197,   214,    77,   144,   139,
     110,   176,   177,   171,    83,   145,   156,   139,   140,    82,
     145,   156,   215,   136,   144,    85,    85,    85,    85,    77,
      85,    85,   110,    85,    85,    85,    77,    85,    85,    77,
      85,   196,   110,   196,    85,    87,   136,    87,   136,   218,
     221,   222,   223,   224,   225,   226,   226,   227,   227,   227,
     227,   228,   228,   229,   229,   230,   230,   230,   181,   192,
     191,    85,   189,   197,    79,   179,    77,   144,   156,   172,
     197,   110,   140,   162,    86,   139,   166,    85,   218,   211,
      84,    84,    84,    84,    76,   192,   218,   177,    83,   180,
      85,   110,   181,    85,   219,    86,   110,   155,   159,    85,
      86,   159,     7,    28,   161,   163,   165,   167,   110,   159,
      76,   136,   145,   156,    86,   167,   169,   214,   159,    76,
     163,   139,   181,    86
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 262 "nidl_y.y"
    {
				global_cppquotes_post = (AST_cpp_quote_n_t*)AST_concat_element(
					(ASTP_node_t*)global_cppquotes_post, (ASTP_node_t*)(yyvsp[(2) - (2)].y_cpp_quote));
			;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 269 "nidl_y.y"
    {
            			(yyval.y_import) = (AST_import_n_t *)NULL;
		        ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 294 "nidl_y.y"
    {        		
		(yyval.y_cpp_quote) = (AST_cpp_quote_n_t *) AST_concat_element(
                                                (ASTP_node_t *) (yyvsp[(1) - (2)].y_cpp_quote),
                                                (ASTP_node_t *) (yyvsp[(2) - (2)].y_cpp_quote));				
        ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 300 "nidl_y.y"
    {	(yyval.y_cpp_quote) = (AST_cpp_quote_n_t *)NULL;		
	;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 307 "nidl_y.y"
    {
		global_cppquotes = (AST_cpp_quote_n_t*)AST_concat_element(
			(ASTP_node_t*)global_cppquotes, (ASTP_node_t*)(yyvsp[(1) - (2)].y_cpp_quote));		
	;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 316 "nidl_y.y"
    {
            AST_finish_interface_node(the_interface);
        ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 325 "nidl_y.y"
    {
	    AST_type_n_t * interface_type = AST_type_node(AST_interface_k);
	    interface_type->type_structure.interface = the_interface;
	    interface_type->name = (yyvsp[(3) - (3)].y_id);
            the_interface->name = (yyvsp[(3) - (3)].y_id);
            ASTP_add_name_binding (the_interface->name, interface_type);
        ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 336 "nidl_y.y"
    {
		 the_interface->inherited_interface_name = NAMETABLE_NIL_ID;
	;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 340 "nidl_y.y"
    {
		 the_interface->inherited_interface_name = (yyvsp[(2) - (2)].y_id);
	;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 347 "nidl_y.y"
    {
            STRTAB_str_t nidl_idl_str;
            AST_interface_n_t* old = the_interface;

            nidl_idl_str = STRTAB_add_string (AUTO_IMPORT_FILE);
	    the_interface = AST_interface_node();			  		
            the_interface->prev = old;
	    the_interface->exports = NULL;
            the_interface->imports = AST_import_node(nidl_idl_str);
            the_interface->imports->interface = FE_parse_import (nidl_idl_str);
            if (the_interface->imports->interface != NULL)
            {
                AST_CLR_OUT_OF_LINE(the_interface->imports->interface);
                AST_SET_IN_LINE(the_interface->imports->interface);
            }
        ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 369 "nidl_y.y"
    { (yyval.y_interface) = (yyvsp[(2) - (3)].y_interface); ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 371 "nidl_y.y"
    {
            (yyval.y_interface) = NULL;
            log_error(nidl_yylineno,NIDL_MISSONINTER, NULL);
        ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 376 "nidl_y.y"
    {
            (yyval.y_interface) = NULL;
        ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 384 "nidl_y.y"
    {
            /* May already be an import of nbase, so concat */
            the_interface->imports = (AST_import_n_t *) AST_concat_element(
                                        (ASTP_node_t *) the_interface->imports,
                                        (ASTP_node_t *) (yyvsp[(1) - (3)].y_import));
            the_interface->exports = (AST_export_n_t*)AST_concat_element(
			(ASTP_node_t*)the_interface->exports,
			(ASTP_node_t*)(yyvsp[(2) - (3)].y_export));
        ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 398 "nidl_y.y"
    {
            (yyval.y_import) = (AST_import_n_t *)NULL;
        ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 405 "nidl_y.y"
    {
#if 0
	 			global_imports = (AST_import_n_t*)AST_concat_element(
	 				 (ASTP_node_t*)global_imports, (ASTP_node_t*)(yyvsp[(1) - (2)].y_import));
#endif

				global_cppquotes_post = (AST_cpp_quote_n_t*)AST_concat_element(
					(ASTP_node_t*)global_cppquotes_post, (ASTP_node_t*)(yyvsp[(2) - (2)].y_cpp_quote));
	;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 420 "nidl_y.y"
    {
                (yyval.y_import) = (AST_import_n_t *) AST_concat_element(
                                                (ASTP_node_t *) (yyvsp[(1) - (2)].y_import),
                                                (ASTP_node_t *) (yyvsp[(2) - (2)].y_import));
        ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 429 "nidl_y.y"
    {
            (yyval.y_import) = (AST_import_n_t *)NULL;
        ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 433 "nidl_y.y"
    {
            (yyval.y_import) = (AST_import_n_t *)NULL;
        ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 437 "nidl_y.y"
    {
            (yyval.y_import) = (yyvsp[(2) - (3)].y_import);
        ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 445 "nidl_y.y"
    {
                (yyval.y_import) = (AST_import_n_t *) AST_concat_element(
                                                (ASTP_node_t *) (yyvsp[(1) - (3)].y_import),
                                                (ASTP_node_t *) (yyvsp[(3) - (3)].y_import));
        ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 456 "nidl_y.y"
    {
            AST_interface_n_t  *int_p;
            int_p = FE_parse_import ((yyvsp[(1) - (1)].y_string));
            if (int_p != (AST_interface_n_t *)NULL)
            {
                (yyval.y_import) = AST_import_node((yyvsp[(1) - (1)].y_string));
                (yyval.y_import)->interface = int_p;
            }
            else
                (yyval.y_import) = (AST_import_n_t *)NULL;
        ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 472 "nidl_y.y"
    {
                (yyval.y_export) = (AST_export_n_t *) AST_concat_element(
                                            (ASTP_node_t *) (yyvsp[(1) - (3)].y_export),
                                            (ASTP_node_t *) (yyvsp[(3) - (3)].y_export)) ;
        ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 482 "nidl_y.y"
    {
                (yyval.y_export) = AST_types_to_exports ((yyvsp[(1) - (2)].y_type_ptr));
        ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 486 "nidl_y.y"
    {
                (yyval.y_export) = AST_export_node (
                        (ASTP_node_t *) (yyvsp[(1) - (2)].y_constant), AST_constant_k);
        ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 491 "nidl_y.y"
    {
            if (ASTP_parsing_main_idl)
                (yyval.y_export) = AST_export_node (
                        (ASTP_node_t *) (yyvsp[(1) - (2)].y_operation), AST_operation_k);
        ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 497 "nidl_y.y"
    {
            (yyval.y_export) = AST_export_node (
                (ASTP_node_t *) (yyvsp[(1) - (1)].y_cpp_quote), AST_cpp_quote_k);
        ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 502 "nidl_y.y"
    {
            (yyval.y_export) = (AST_export_n_t *)NULL;
        ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 509 "nidl_y.y"
    {
        	(yyval.y_cpp_quote) = AST_cpp_quote_node((yyvsp[(3) - (4)].y_string));	
		
        ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 518 "nidl_y.y"
    {
           (yyval.y_constant) = AST_finish_constant_node ((yyvsp[(5) - (5)].y_constant),
                                        (yyvsp[(3) - (5)].y_declarator), (yyvsp[(2) - (5)].y_type));
        ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 526 "nidl_y.y"
    {
				(yyval.y_constant) = AST_constant_from_exp((yyvsp[(1) - (1)].y_exp));
				if ((yyval.y_constant) == NULL)	{
					 log_error(nidl_yylineno, NIDL_EXPNOTCONST, NULL);
				}
        ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 537 "nidl_y.y"
    {
            (yyval.y_type_ptr) = (yyvsp[(2) - (2)].y_type_ptr);
        ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 544 "nidl_y.y"
    {
            (yyval.y_type_ptr)  = AST_declarators_to_types(the_interface, (yyvsp[(2) - (4)].y_type),
                        (yyvsp[(3) - (4)].y_declarator), &(yyvsp[(1) - (4)].y_attributes)) ;
            ASTP_free_simple_list((ASTP_node_t *)(yyvsp[(1) - (4)].y_attributes).bounds);
        ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 579 "nidl_y.y"
    {
            (yyval.y_type) = AST_lookup_named_type((yyvsp[(1) - (1)].y_id));
        ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 586 "nidl_y.y"
    {
            (yyval.y_type) = AST_lookup_type_node(AST_short_float_k);
        ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 590 "nidl_y.y"
    {
            (yyval.y_type) = AST_lookup_type_node(AST_long_float_k);
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 598 "nidl_y.y"
    { log_warning(nidl_yylineno, NIDL_EXTRAPUNCT, ",", NULL);;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 604 "nidl_y.y"
    { log_warning(nidl_yylineno, NIDL_EXTRAPUNCT, ";", NULL);;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 608 "nidl_y.y"
    { (yyval.y_int_info).int_signed = false; ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 609 "nidl_y.y"
    { (yyval.y_int_info).int_signed = true; ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 614 "nidl_y.y"
    {
            (yyval.y_int_info).int_size = AST_small_integer_k;
            (yyval.y_int_info).int_signed = true;
        ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 619 "nidl_y.y"
    {
            (yyval.y_int_info).int_size = AST_short_integer_k;
            (yyval.y_int_info).int_signed = true;
        ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 624 "nidl_y.y"
    {
            (yyval.y_int_info).int_size = AST_long_integer_k;
            (yyval.y_int_info).int_signed = true;
        ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 629 "nidl_y.y"
    {
            (yyval.y_int_info).int_size = AST_hyper_integer_k;
            (yyval.y_int_info).int_signed = true;
        ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 637 "nidl_y.y"
    { (yyval.y_int_info) = (yyvsp[(1) - (1)].y_int_info); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 639 "nidl_y.y"
    {
            (yyval.y_int_info).int_size = (yyvsp[(2) - (2)].y_int_info).int_size;
            (yyval.y_int_info).int_signed = false;
        ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 644 "nidl_y.y"
    {
            (yyval.y_int_info).int_size = (yyvsp[(1) - (2)].y_int_info).int_size;
            (yyval.y_int_info).int_signed = false;
        ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 652 "nidl_y.y"
    { (yyval.y_type) = AST_lookup_integer_type_node((yyvsp[(1) - (1)].y_int_info).int_size,(yyvsp[(1) - (1)].y_int_info).int_signed); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 654 "nidl_y.y"
    { (yyval.y_type) = AST_lookup_integer_type_node((yyvsp[(1) - (2)].y_int_info).int_size,(yyvsp[(1) - (2)].y_int_info).int_signed); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 656 "nidl_y.y"
    {
            log_warning(nidl_yylineno,NIDL_INTSIZEREQ, NULL);
            (yyval.y_type) = AST_lookup_integer_type_node(AST_long_integer_k,(yyvsp[(1) - (2)].y_int_info).int_signed);
        ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 664 "nidl_y.y"
    { (yyval.y_type) = AST_lookup_type_node(AST_character_k); ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 669 "nidl_y.y"
    { (yyval.y_type) = AST_lookup_type_node(AST_boolean_k); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 674 "nidl_y.y"
    { (yyval.y_type) = AST_lookup_type_node(AST_byte_k); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 679 "nidl_y.y"
    { (yyval.y_type) = AST_lookup_type_node(AST_void_k); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 684 "nidl_y.y"
    { (yyval.y_type) = AST_lookup_type_node(AST_handle_k); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 689 "nidl_y.y"
    {
            NAMETABLE_push_level ();
        ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 696 "nidl_y.y"
    {
            ASTP_patch_field_reference ();
            NAMETABLE_pop_level ();
        ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 704 "nidl_y.y"
    {
        (yyval.y_type) = AST_disc_union_node(
                         NAMETABLE_NIL_ID,      /* tag name          */
                         NAMETABLE_NIL_ID,      /* union name        */
                         NAMETABLE_NIL_ID,      /* discriminant name */
                         NULL,                  /* discriminant type */
                         (yyvsp[(2) - (2)].y_arm) );           /* the arm list      */
        ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 714 "nidl_y.y"
    {
        (yyval.y_type) = AST_disc_union_node(
                         NAMETABLE_NIL_ID,      /* tag name          */
                         ASTP_tagged_union_id,  /* union name        */
                         (yyvsp[(5) - (7)].y_id),              /* discriminant name */
                         (yyvsp[(4) - (7)].y_type),            /* discriminant type */
                         (yyvsp[(7) - (7)].y_arm) );           /* the arm list      */
        ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 723 "nidl_y.y"
    {
        (yyval.y_type) = AST_disc_union_node(
                         (yyvsp[(2) - (3)].y_id),              /* tag name          */
                         NAMETABLE_NIL_ID,      /* union name        */
                         NAMETABLE_NIL_ID,      /* discriminant name */
                         NULL,                  /* discriminant type */
                         (yyvsp[(3) - (3)].y_arm) );           /* the arm list      */
        ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 732 "nidl_y.y"
    {
        (yyval.y_type) = AST_disc_union_node(
                         NAMETABLE_NIL_ID,      /* tag name          */
                         (yyvsp[(7) - (8)].y_id),              /* union name        */
                         (yyvsp[(5) - (8)].y_id),              /* discriminant name */
                         (yyvsp[(4) - (8)].y_type),            /* discriminant type */
                         (yyvsp[(8) - (8)].y_arm) );           /* the arm list      */
        ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 741 "nidl_y.y"
    {
        (yyval.y_type) = AST_disc_union_node(
                         (yyvsp[(2) - (8)].y_id),              /* tag name          */
                         ASTP_tagged_union_id,  /* union name        */
                         (yyvsp[(6) - (8)].y_id),              /* discriminant name */
                         (yyvsp[(5) - (8)].y_type),            /* discriminant type */
                         (yyvsp[(8) - (8)].y_arm) );           /* the arm list      */
        ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 750 "nidl_y.y"
    {
        (yyval.y_type) = AST_disc_union_node(
                         (yyvsp[(2) - (9)].y_id),              /* tag name          */
                         (yyvsp[(8) - (9)].y_id),              /* union name        */
                         (yyvsp[(6) - (9)].y_id),              /* discriminant name */
                         (yyvsp[(5) - (9)].y_type),            /* discriminant type */
                         (yyvsp[(9) - (9)].y_arm) );           /* the arm list      */
        ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 759 "nidl_y.y"
    {
            (yyval.y_type) = AST_type_from_tag (AST_disc_union_k, (yyvsp[(2) - (2)].y_id));
        ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 766 "nidl_y.y"
    {
                (yyval.y_arm) = (yyvsp[(2) - (3)].y_arm);
        ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 772 "nidl_y.y"
    {
                (yyval.y_arm) = (yyvsp[(2) - (3)].y_arm);
        ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 780 "nidl_y.y"
    {
            (yyval.y_arm) = (AST_arm_n_t *) AST_concat_element(
                                        (ASTP_node_t *) (yyvsp[(1) - (3)].y_arm),
                                        (ASTP_node_t *) (yyvsp[(3) - (3)].y_arm));
        ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 789 "nidl_y.y"
    {
            (yyval.y_arm) = (AST_arm_n_t *) AST_concat_element(
                                        (ASTP_node_t *) (yyvsp[(1) - (3)].y_arm),
                                        (ASTP_node_t *) (yyvsp[(3) - (3)].y_arm));
        ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 798 "nidl_y.y"
    {
            (yyval.y_arm) = (yyvsp[(1) - (1)].y_arm);
        ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 804 "nidl_y.y"
    {
            (yyval.y_arm) = AST_label_arm((yyvsp[(2) - (2)].y_arm), (yyvsp[(1) - (2)].y_label)) ;
        ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 812 "nidl_y.y"
    {
            (yyval.y_label) = (AST_case_label_n_t *) AST_concat_element(
                                        (ASTP_node_t *) (yyvsp[(1) - (3)].y_label),
                                        (ASTP_node_t *) (yyvsp[(3) - (3)].y_label));
        ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 821 "nidl_y.y"
    {
            (yyval.y_label) = (AST_case_label_n_t *) AST_concat_element(
                                        (ASTP_node_t *) (yyvsp[(1) - (2)].y_label),
                                        (ASTP_node_t *) (yyvsp[(2) - (2)].y_label));
        ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 830 "nidl_y.y"
    {
            (yyval.y_label) = AST_case_label_node((yyvsp[(1) - (1)].y_constant));
        ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 836 "nidl_y.y"
    {
            (yyval.y_label) = AST_case_label_node((yyvsp[(2) - (3)].y_constant));
        ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 840 "nidl_y.y"
    {
            (yyval.y_label) = AST_default_case_label_node();
        ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 847 "nidl_y.y"
    {
            (yyval.y_arm) = AST_declarator_to_arm(NULL, NULL, &(yyvsp[(2) - (3)].y_attributes));
            ASTP_free_simple_list((ASTP_node_t *)(yyvsp[(2) - (3)].y_attributes).bounds);
        ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 852 "nidl_y.y"
    {
            (yyval.y_arm) = AST_declarator_to_arm((yyvsp[(3) - (5)].y_type),
                                (yyvsp[(4) - (5)].y_declarator), &(yyvsp[(2) - (5)].y_attributes));
            ASTP_free_simple_list((ASTP_node_t *)(yyvsp[(2) - (5)].y_attributes).bounds);
        ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 860 "nidl_y.y"
    {
            (yyval.y_arm) = AST_arm_node(NAMETABLE_NIL_ID,NULL,NULL);
        ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 864 "nidl_y.y"
    {
            if (ASTP_TEST_ATTR(&(yyvsp[(1) - (4)].y_attributes), ASTP_CASE))
            {
                ASTP_attr_flag_t attr1 = ASTP_CASE;
                log_error(nidl_yylineno, NIDL_EUMEMATTR,
                      KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
		      NULL);
            }
            if (ASTP_TEST_ATTR(&(yyvsp[(1) - (4)].y_attributes), ASTP_DEFAULT))
            {
                ASTP_attr_flag_t attr1 = ASTP_DEFAULT;
                log_error(nidl_yylineno, NIDL_EUMEMATTR,
                      KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
		      NULL);
            }
            (yyval.y_arm) = AST_declarator_to_arm((yyvsp[(2) - (4)].y_type),
                                (yyvsp[(3) - (4)].y_declarator), &(yyvsp[(1) - (4)].y_attributes));
            ASTP_free_simple_list((ASTP_node_t *)(yyvsp[(1) - (4)].y_attributes).bounds);
        ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 887 "nidl_y.y"
    {
            (yyval.y_type) = AST_structure_node((yyvsp[(3) - (4)].y_field), NAMETABLE_NIL_ID) ;
        ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 891 "nidl_y.y"
    {
            (yyval.y_type) = AST_structure_node((yyvsp[(4) - (5)].y_field), (yyvsp[(2) - (5)].y_id)) ;
        ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 895 "nidl_y.y"
    {
            (yyval.y_type) = AST_type_from_tag (AST_structure_k, (yyvsp[(2) - (2)].y_id));
        ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 903 "nidl_y.y"
    {
            (yyval.y_field) = (AST_field_n_t *)AST_concat_element(
                                    (ASTP_node_t *) (yyvsp[(1) - (3)].y_field),
                                    (ASTP_node_t *) (yyvsp[(3) - (3)].y_field)) ;
        ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 912 "nidl_y.y"
    {
            (yyval.y_field) = AST_declarators_to_fields((yyvsp[(4) - (5)].y_declarator),
                                                    (yyvsp[(2) - (5)].y_type),
                                                    &(yyvsp[(1) - (5)].y_attributes));
            ASTP_free_simple_list((ASTP_node_t *)(yyvsp[(1) - (5)].y_attributes).bounds);
        ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 922 "nidl_y.y"
    {
             (yyval.y_type) = AST_enumerator_node((yyvsp[(3) - (3)].y_constant), AST_short_integer_k);
        ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 929 "nidl_y.y"
    {
		;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 936 "nidl_y.y"
    {
            (yyval.y_constant) = (yyvsp[(2) - (4)].y_constant) ;
        ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 944 "nidl_y.y"
    {
            (yyval.y_constant) = (AST_constant_n_t *) AST_concat_element(
                                    (ASTP_node_t *) (yyvsp[(1) - (3)].y_constant),
                                    (ASTP_node_t *) (yyvsp[(3) - (3)].y_constant)) ;
        ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 953 "nidl_y.y"
    {
            (yyval.y_constant) = AST_enum_constant((yyvsp[(1) - (2)].y_id), (yyvsp[(2) - (2)].y_exp)) ;
        ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 960 "nidl_y.y"
    {
            (yyval.y_type) = AST_pipe_node ((yyvsp[(2) - (2)].y_type));
        ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 967 "nidl_y.y"
    {
			 (yyval.y_exp) = AST_exp_integer_constant(0, true);
		;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 971 "nidl_y.y"
    {
	 		 ASTP_validate_integer((yyvsp[(2) - (2)].y_exp));
			 (yyval.y_exp) = (yyvsp[(2) - (2)].y_exp);
		;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 979 "nidl_y.y"
    {
				(yyval.y_declarator) =  (yyvsp[(1) - (1)].y_declarator);
		  ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 983 "nidl_y.y"
    {
            (yyval.y_declarator) = (ASTP_declarator_n_t *) AST_concat_element(
                                            (ASTP_node_t *) (yyvsp[(1) - (3)].y_declarator),
                                            (ASTP_node_t *) (yyvsp[(3) - (3)].y_declarator)) ;
        ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 994 "nidl_y.y"
    { (yyval.y_declarator) = (yyvsp[(1) - (1)].y_declarator); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 999 "nidl_y.y"
    { (yyval.y_declarator) = (yyvsp[(1) - (1)].y_declarator); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1001 "nidl_y.y"
    {
                (yyval.y_declarator) = (yyvsp[(2) - (2)].y_declarator);
                AST_declarator_operation((yyval.y_declarator), AST_pointer_k,
                        (ASTP_node_t *)NULL, (yyvsp[(1) - (2)].y_ptrlevels) );
            ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1010 "nidl_y.y"
    { (yyval.y_ptrlevels) = 1;;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1012 "nidl_y.y"
    { (yyval.y_ptrlevels) = (yyvsp[(2) - (2)].y_ptrlevels) + 1; ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1017 "nidl_y.y"
    { (yyval.y_declarator) = AST_declarator_node ( (yyvsp[(1) - (1)].y_id) ); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1019 "nidl_y.y"
    {
                (yyval.y_declarator) = (yyval.y_declarator);
                AST_declarator_operation((yyval.y_declarator), AST_array_k,
                        (ASTP_node_t *) (yyvsp[(2) - (2)].y_index), 0 );
            ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1025 "nidl_y.y"
    {
            (yyval.y_declarator) = (yyvsp[(2) - (3)].y_declarator);
            ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1029 "nidl_y.y"
    {
                (yyval.y_declarator) = (yyval.y_declarator);
                AST_declarator_operation((yyval.y_declarator), AST_function_k,
                        (ASTP_node_t *) (yyvsp[(2) - (2)].y_parameter), 0 );
            ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1057 "nidl_y.y"
    {
            (yyval.y_index) = ASTP_array_index_node ( NULL, ASTP_default_bound,
                                                 NULL, ASTP_open_bound);
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1062 "nidl_y.y"
    {
            (yyval.y_index) = ASTP_array_index_node  ( NULL, ASTP_default_bound,
                                                 NULL, ASTP_open_bound);
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1067 "nidl_y.y"
    {
            (yyval.y_index) = ASTP_array_index_node  ( NULL, ASTP_default_bound,
                                                 (yyvsp[(2) - (3)].y_constant), ASTP_constant_bound);
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1072 "nidl_y.y"
    {
            (yyval.y_index) = ASTP_array_index_node  ( NULL, ASTP_open_bound,
                                                 NULL, ASTP_open_bound);
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1077 "nidl_y.y"
    {
            (yyval.y_index) = ASTP_array_index_node  ( NULL, ASTP_open_bound,
                                                 (yyvsp[(4) - (5)].y_constant), ASTP_constant_bound);
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1082 "nidl_y.y"
    {
            (yyval.y_index) = ASTP_array_index_node  ( (yyvsp[(2) - (5)].y_constant), ASTP_constant_bound,
                                                 NULL, ASTP_open_bound);
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1087 "nidl_y.y"
    {
            (yyval.y_index) = ASTP_array_index_node  ( (yyvsp[(2) - (5)].y_constant), ASTP_constant_bound,
                                                 (yyvsp[(4) - (5)].y_constant), ASTP_constant_bound);
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1096 "nidl_y.y"
    {
            if (ASTP_parsing_main_idl)
                (yyval.y_operation) = AST_operation_node (
                                    (yyvsp[(2) - (4)].y_type),         /*The type node*/
                                    (yyvsp[(3) - (4)].y_declarator),   /* Declarator list */
                                   &(yyvsp[(1) - (4)].y_attributes));  /* attributes */
            ASTP_free_simple_list((ASTP_node_t *)(yyvsp[(1) - (4)].y_attributes).bounds);
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1105 "nidl_y.y"
    {
        log_error(nidl_yylineno,NIDL_MISSONOP, NULL);
        (yyval.y_operation) = NULL;
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1113 "nidl_y.y"
    {
            (yyval.y_parameter) = (yyvsp[(2) - (4)].y_parameter);
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1120 "nidl_y.y"
    {
        NAMETABLE_push_level ();
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1127 "nidl_y.y"
    {
        ASTP_patch_field_reference ();
        NAMETABLE_pop_level ();
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1136 "nidl_y.y"
    {
            if (ASTP_parsing_main_idl)
                (yyval.y_parameter) = (AST_parameter_n_t *) AST_concat_element(
                                    (ASTP_node_t *) (yyvsp[(1) - (3)].y_parameter),
                                    (ASTP_node_t *) (yyvsp[(3) - (3)].y_parameter));
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1143 "nidl_y.y"
    {
            (yyval.y_parameter) = (AST_parameter_n_t *)NULL;
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1150 "nidl_y.y"
    {
            /*
             * We have to use special code here to allow (void) as a parameter
             * specification.  If there are no declarators, then we need to
             * make sure that the type is void and that there are no attributes .
             */
            if ((yyvsp[(4) - (4)].y_declarator) == NULL)
            {
                /*
                 * If the type is not void or some attribute is specified,
                 * there is a syntax error.  Force a yacc error, so that
                 * we can safely recover from the lack of a declarator.
                 */
                if (((yyvsp[(2) - (4)].y_type)->kind != AST_void_k) ||
                   ((yyvsp[(1) - (4)].y_attributes).bounds != NULL) ||
                   ((yyvsp[(1) - (4)].y_attributes).attr_flags != 0))
                {
                    yywhere();  /* Issue a syntax error for this line */
                    YYERROR;    /* Allow natural error recovery */
                }

                (yyval.y_parameter) = (AST_parameter_n_t *)NULL;
            }
            else
            {
                if (ASTP_parsing_main_idl)
                    (yyval.y_parameter) = AST_declarator_to_param(
                                            &(yyvsp[(1) - (4)].y_attributes),
                                            (yyvsp[(2) - (4)].y_type),
                                            (yyvsp[(4) - (4)].y_declarator));
            }
            ASTP_free_simple_list((ASTP_node_t *)(yyvsp[(1) - (4)].y_attributes).bounds);
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1184 "nidl_y.y"
    {
            log_error(nidl_yylineno, NIDL_MISSONPARAM, NULL);
            (yyval.y_parameter) = (AST_parameter_n_t *)NULL;
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1192 "nidl_y.y"
    { (yyval.y_declarator) = (yyvsp[(1) - (1)].y_declarator); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1194 "nidl_y.y"
    { (yyval.y_declarator) = NULL; ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1210 "nidl_y.y"
    {
            search_attributes_table = true;
        ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1217 "nidl_y.y"
    {
            search_attributes_table = false;
        ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1224 "nidl_y.y"
    {
            search_attributes_table = false;
        ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1231 "nidl_y.y"
    {
            search_attributes_table = true;
        ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1243 "nidl_y.y"
    {
            /* Give an error on notranslated sources */
            if (((yyvsp[(1) - (1)].y_attributes).bounds != NULL) ||
               ((yyvsp[(1) - (1)].y_attributes).attr_flags != 0))
            {
                log_error(nidl_yylineno,NIDL_ATTRTRANS, NULL);
                ASTP_free_simple_list((ASTP_node_t *)(yyvsp[(1) - (1)].y_attributes).bounds);
            }
        ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1264 "nidl_y.y"
    {
            log_error(nidl_yylineno,NIDL_ERRINATTR, NULL);
        ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1279 "nidl_y.y"
    {
            log_error(nidl_yylineno,NIDL_SYNTAXUUID, NULL);
        ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1283 "nidl_y.y"
    {
            {
                if (ASTP_IF_AF_SET(the_interface,ASTP_IF_UUID))
                        log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
                ASTP_SET_IF_AF(the_interface,ASTP_IF_UUID);
                the_interface->uuid = (yyvsp[(2) - (2)].y_uuid);
            }
        ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1292 "nidl_y.y"
    {
            if (ASTP_IF_AF_SET(the_interface,ASTP_IF_PORT))
                    log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
            ASTP_SET_IF_AF(the_interface,ASTP_IF_PORT);
        ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1298 "nidl_y.y"
    {
            if (ASTP_IF_AF_SET(the_interface, ASTP_IF_EXCEPTIONS))
                log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
            ASTP_SET_IF_AF(the_interface, ASTP_IF_EXCEPTIONS);
        ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1304 "nidl_y.y"
    {
            {
                if (ASTP_IF_AF_SET(the_interface,ASTP_IF_VERSION))
                        log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
                ASTP_SET_IF_AF(the_interface,ASTP_IF_VERSION);
            }

        ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1313 "nidl_y.y"
    {
            {
                if (AST_LOCAL_SET(the_interface))
                        log_warning(nidl_yylineno, NIDL_MULATTRDEF, NULL);
                AST_SET_LOCAL(the_interface);
            }
        ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1321 "nidl_y.y"
    {
            if (the_interface->pointer_default != 0)
                    log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
            the_interface->pointer_default = (yyvsp[(3) - (4)].y_ptrclass);
        ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1328 "nidl_y.y"
    {
				if (AST_OBJECT_SET(the_interface))
					 log_warning(nidl_yylineno, NIDL_MULATTRDEF, NULL);
				AST_SET_OBJECT(the_interface);
			;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1334 "nidl_y.y"
    {
				/* complain about compat here */
			;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1341 "nidl_y.y"
    {
		if (the_interface->implicit_handle_name != NAMETABLE_NIL_ID)
			 log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);

		ASTP_set_implicit_handle(the_interface, NAMETABLE_NIL_ID, (yyvsp[(4) - (5)].y_id));
	;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1349 "nidl_y.y"
    {
		if (the_interface->implicit_handle_name != NAMETABLE_NIL_ID)
			log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
	
		ASTP_set_implicit_handle(the_interface, (yyvsp[(3) - (5)].y_id), (yyvsp[(4) - (5)].y_id));
	;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1358 "nidl_y.y"
    { (yyval.y_ptrclass) = ASTP_REF; ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1359 "nidl_y.y"
    { (yyval.y_ptrclass) = ASTP_PTR; ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1360 "nidl_y.y"
    { (yyval.y_ptrclass) = ASTP_UNIQUE; ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1365 "nidl_y.y"
    {
            the_interface->version = (yyvsp[(1) - (1)].y_int_info).int_val;
            if (the_interface->version > /*(unsigned int)*/ASTP_C_USHORT_MAX)
                log_error(nidl_yylineno, NIDL_MAJORTOOLARGE,
			  ASTP_C_USHORT_MAX, NULL);
        ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1372 "nidl_y.y"
    {
            char const *float_text;
            unsigned int            major_version,minor_version;
            STRTAB_str_to_string((yyvsp[(1) - (1)].y_string), &float_text);
            sscanf(float_text,"%d.%d",&major_version,&minor_version);
            if (major_version > (unsigned int)ASTP_C_USHORT_MAX)
                log_error(nidl_yylineno, NIDL_MAJORTOOLARGE,
			  ASTP_C_USHORT_MAX, NULL);
            if (minor_version > (unsigned int)ASTP_C_USHORT_MAX)
                log_error(nidl_yylineno, NIDL_MINORTOOLARGE,
			  ASTP_C_USHORT_MAX, NULL);
            the_interface->version = (minor_version * 65536) + major_version;
        ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1394 "nidl_y.y"
    {
            the_interface->exceptions = (yyvsp[(1) - (1)].y_exception);
        ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1398 "nidl_y.y"
    {
            (yyval.y_exception) = (AST_exception_n_t *) AST_concat_element(
                                (ASTP_node_t *) the_interface->exceptions,
                                (ASTP_node_t *) (yyvsp[(3) - (3)].y_exception) );
        ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1407 "nidl_y.y"
    {
            ASTP_parse_port(the_interface,(yyvsp[(1) - (1)].y_string));
        ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1414 "nidl_y.y"
    {
            if (ASTP_parsing_main_idl)
                (yyval.y_exception) = AST_exception_node((yyvsp[(1) - (1)].y_id));
            else
                (yyval.y_exception) = NULL;
        ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1432 "nidl_y.y"
    {
            (yyval.y_attributes).bounds = (yyvsp[(3) - (4)].y_attributes).bounds;
            (yyval.y_attributes).attr_flags = 0;
        ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1437 "nidl_y.y"
    {
            (yyval.y_attributes).bounds = (yyvsp[(3) - (4)].y_attributes).bounds;
            (yyval.y_attributes).attr_flags = 0;
        ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1445 "nidl_y.y"
    {
            ASTP_bound_type = first_is_k;
        ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1449 "nidl_y.y"
    {
            ASTP_bound_type = last_is_k;
        ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1453 "nidl_y.y"
    {
            ASTP_bound_type = length_is_k;
        ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1457 "nidl_y.y"
    {
            ASTP_bound_type = max_is_k;
        ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1461 "nidl_y.y"
    {
            ASTP_bound_type = min_is_k;
        ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1465 "nidl_y.y"
    {
            ASTP_bound_type = size_is_k;
        ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1474 "nidl_y.y"
    {
        (yyval.y_attributes).bounds = (ASTP_type_attr_n_t *) AST_concat_element (
                                (ASTP_node_t*) (yyvsp[(1) - (3)].y_attributes).bounds,
                                (ASTP_node_t*) (yyvsp[(3) - (3)].y_attributes).bounds);
        ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1484 "nidl_y.y"
    {
				 (yyval.y_attributes).bounds = AST_array_bound_from_expr((yyvsp[(1) - (1)].y_exp), ASTP_bound_type);
			;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1488 "nidl_y.y"
    {
        (yyval.y_attributes).bounds = AST_array_bound_info (NAMETABLE_NIL_ID, ASTP_bound_type, FALSE);
        ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1495 "nidl_y.y"
    {
            ASTP_bound_type = switch_is_k;
        ;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1502 "nidl_y.y"
    {
        (yyval.y_attributes).bounds = AST_array_bound_info((yyvsp[(1) - (1)].y_id), ASTP_bound_type, FALSE);
        ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1506 "nidl_y.y"
    {
        (yyval.y_attributes).bounds = AST_array_bound_info((yyvsp[(2) - (2)].y_id), ASTP_bound_type, TRUE);
        ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1516 "nidl_y.y"
    { (yyval.y_attributes) = (yyvsp[(2) - (2)].y_attributes); ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1519 "nidl_y.y"
    {
        (yyval.y_attributes).bounds = NULL;
        (yyval.y_attributes).attr_flags = 0;
        ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1529 "nidl_y.y"
    {
        /*
         * Can't tell if we had any valid attributes in the list, so return
         * none.
         */
        (yyval.y_attributes).bounds = NULL;
        (yyval.y_attributes).attr_flags = 0;
        log_error(nidl_yylineno, NIDL_ERRINATTR, NULL);
        ;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1539 "nidl_y.y"
    {
        /*
         * No closer to the attribute, so give a different message.
         */
        (yyval.y_attributes).bounds = NULL;
        (yyval.y_attributes).attr_flags = 0;
        log_error(nidl_yylineno, NIDL_MISSONATTR, NULL);
        search_attributes_table = false;
        ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1553 "nidl_y.y"
    { (yyval.y_attributes) = (yyvsp[(1) - (1)].y_attributes); ;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1556 "nidl_y.y"
    {
          /*
           * If the same bit has been specified more than once, then issue
           * a message.
           */
          if (((yyvsp[(1) - (3)].y_attributes).attr_flags & (yyvsp[(3) - (3)].y_attributes).attr_flags) != 0)
                log_warning(nidl_yylineno, NIDL_MULATTRDEF, NULL);
          (yyval.y_attributes).attr_flags = (yyvsp[(1) - (3)].y_attributes).attr_flags |
                                        (yyvsp[(3) - (3)].y_attributes).attr_flags;
          (yyval.y_attributes).bounds = (ASTP_type_attr_n_t *) AST_concat_element (
                                (ASTP_node_t*) (yyvsp[(1) - (3)].y_attributes).bounds,
                                (ASTP_node_t*) (yyvsp[(3) - (3)].y_attributes).bounds);
        ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1574 "nidl_y.y"
    { (yyval.y_attributes) = (yyvsp[(1) - (1)].y_attributes); ;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1577 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_BROADCAST;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1579 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_MAYBE;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1581 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_IDEMPOTENT;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1583 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_REFLECT_DELETIONS;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1585 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_LOCAL;
	                               (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1587 "nidl_y.y"
    {	;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1590 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_PTR;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1592 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_IN;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1595 "nidl_y.y"
    { (yyval.y_attributes).attr_flags =
                                        ASTP_IN | ASTP_IN_SHAPE;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1598 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_OUT;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1601 "nidl_y.y"
    { (yyval.y_attributes).attr_flags =
                                        ASTP_OUT | ASTP_OUT_SHAPE;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1605 "nidl_y.y"
    { (yyval.y_attributes).iid_is_name = (yyvsp[(3) - (4)].y_id); 
                                   (yyval.y_attributes).bounds = NULL;
                                   (yyval.y_attributes).attr_flags = 0;
											;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1610 "nidl_y.y"
    { (yyval.y_attributes).iid_is_name = (yyvsp[(4) - (5)].y_id); 
                                   (yyval.y_attributes).bounds = NULL;
                                   (yyval.y_attributes).attr_flags = 0;
											;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1616 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_SMALL;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1618 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_STRING;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1620 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_STRING0;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1622 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_UNIQUE;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1624 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_REF;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1626 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_IGNORE;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1628 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_CONTEXT;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1631 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_RANGE;
                                  (yyval.y_attributes).bounds =
                                     AST_range_from_expr((yyvsp[(3) - (6)].y_exp), (yyvsp[(5) - (6)].y_exp));
                                ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1637 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_UNALIGN;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1639 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_V1_ENUM;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1642 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_ALIGN_SMALL;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1645 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_ALIGN_SHORT;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1648 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_ALIGN_LONG;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1651 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_ALIGN_HYPER;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1653 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_HANDLE;
                                  (yyval.y_attributes).bounds = NULL;       ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1656 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_TRANSMIT_AS;
                                  (yyval.y_attributes).bounds = NULL;
                                  ASTP_transmit_as_type = (yyvsp[(3) - (4)].y_type);
                                ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1661 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_SWITCH_TYPE;
                                  (yyval.y_attributes).bounds = NULL;
                                  ASTP_switch_type = (yyvsp[(3) - (4)].y_type);
                                ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1668 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_CASE;
                                  (yyval.y_attributes).bounds = NULL;
                                  ASTP_case = (yyvsp[(3) - (4)].y_label);
                                ;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1672 "nidl_y.y"
    { (yyval.y_attributes).attr_flags = ASTP_DEFAULT;
                                  (yyval.y_attributes).bounds = NULL;
                                ;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1676 "nidl_y.y"
    {
                char const *identifier; /* place to receive the identifier text */
                NAMETABLE_id_to_string ((yyvsp[(1) - (1)].y_id), &identifier);
                log_error (nidl_yylineno, NIDL_UNKNOWNATTR, identifier, NULL);
                (yyval.y_attributes).attr_flags = 0;
                (yyval.y_attributes).bounds = NULL;
        ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1692 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1697 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1699 "nidl_y.y"
    {
	 			(yyval.y_exp) = AST_expression(AST_EXP_TERNARY_OP, (yyvsp[(1) - (5)].y_exp), (yyvsp[(3) - (5)].y_exp), (yyvsp[(5) - (5)].y_exp));
        ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1706 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1708 "nidl_y.y"
    {
	 			(yyval.y_exp) = AST_expression(AST_EXP_BINARY_LOG_OR, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1715 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1717 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_LOG_AND, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1724 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1726 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_OR, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1733 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1735 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_XOR, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1742 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1744 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_AND, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1751 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1753 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_EQUAL, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1757 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_NE, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);

        ;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1765 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1767 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_LT, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1771 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_GT, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1775 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_LE, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1779 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_GE, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);

        ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1787 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1789 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_LSHIFT, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1793 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_RSHIFT, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);

        ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1801 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1803 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_PLUS, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);

        ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1808 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_MINUS, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1815 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1817 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_STAR, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
				/*
            if (($<y_exp>$.exp.constant.val.integer < $<y_exp>1.exp.constant.val.integer) &&
                ($<y_exp>$.exp.constant.val.integer < $<y_exp>3.exp.constant.val.integer))
                log_error (nidl_yylineno, NIDL_INTOVERFLOW,
			   KEYWORDS_lookup_text(LONG_KW), NULL);
					*/
        ;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1827 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_SLASH, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
        ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1831 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_BINARY_PERCENT, (yyvsp[(1) - (3)].y_exp), (yyvsp[(3) - (3)].y_exp), NULL);
            /*    log_error (nidl_yylineno, NIDL_INTDIVBY0, NULL); */
        ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1838 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1843 "nidl_y.y"
    {(yyval.y_exp) = (yyvsp[(1) - (1)].y_exp);;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1845 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_UNARY_PLUS, (yyvsp[(2) - (2)].y_exp), NULL, NULL);
		  ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1849 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_UNARY_MINUS, (yyvsp[(2) - (2)].y_exp), NULL, NULL);
        ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1853 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_UNARY_TILDE, (yyvsp[(2) - (2)].y_exp), NULL, NULL);
        ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1857 "nidl_y.y"
    {
				(yyval.y_exp) = AST_expression(AST_EXP_UNARY_NOT, (yyvsp[(2) - (2)].y_exp), NULL, NULL);
        ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1861 "nidl_y.y"
    {
			  (yyval.y_exp) = AST_expression(AST_EXP_UNARY_STAR, (yyvsp[(2) - (2)].y_exp), NULL, NULL);
		  ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1868 "nidl_y.y"
    { (yyval.y_exp) = (yyvsp[(2) - (3)].y_exp); ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1870 "nidl_y.y"
    {
				(yyval.y_exp) = AST_exp_integer_constant(
					(yyvsp[(1) - (1)].y_int_info).int_val,
					(yyvsp[(1) - (1)].y_int_info).int_signed);
        ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1876 "nidl_y.y"
    {
				(yyval.y_exp) = AST_exp_char_constant((yyvsp[(1) - (1)].y_char));
        ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1880 "nidl_y.y"
    {
			  	(yyval.y_exp) = AST_exp_identifier((yyvsp[(1) - (1)].y_id));
        ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1884 "nidl_y.y"
    {
            (yyval.y_exp) = AST_exp_string_constant((yyvsp[(1) - (1)].y_string));
        ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1888 "nidl_y.y"
    {
            (yyval.y_exp) = AST_exp_null_constant();
        ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1893 "nidl_y.y"
    {
            (yyval.y_exp) = AST_exp_boolean_constant(true);
        ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1898 "nidl_y.y"
    {
            (yyval.y_exp) = AST_exp_boolean_constant(false);
        ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1902 "nidl_y.y"
    {
				(yyval.y_exp) = AST_exp_integer_constant(0,0);
            log_error(nidl_yylineno, NIDL_FLOATCONSTNOSUP, NULL);
        ;}
    break;



/* Line 1455 of yacc.c  */
#line 4530 "nidl_y.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1907 "nidl_y.y"


/*****************************************************************
 *
 *  Helper functions for managing multiple BISON parser contexts
 *
 *  GNU Bison v1.25 support for DCE 1.2.2 idl_compiler
 *  added 07-11-97 Jim Doyle, Boston University, <jrd@bu.edu>
 *
 *  Maintainance note:
 *
 *    The set of bison-specific static and global variables
 *    managed by the following code may need to changed for versions
 *    GNU Bison earlier or newer than V1.25.
 *
 *
 *****************************************************************/

/*****************************************************************
 *
 * Data structure to store the state of a BISON lexxer context
 *
 *****************************************************************/

struct nidl_bisonparser_state
  {

    /*
     * BISON parser globals that need to preserved whenever
     * we switch into a new parser context (i.e. multiple,
     * nested parsers).
     */

    int yychar;
    int yynerrs;
    YYSTYPE yylval;

  };


typedef struct nidl_bisonparser_state nidl_bisonparser_activation_record;

/*****************************************************************
 *
 * Basic constructors/destructors for FLEX activation states
 *
 *****************************************************************/

void *
new_nidl_bisonparser_activation_record()
  {
    return (malloc(sizeof(nidl_bisonparser_activation_record)));
  }

void
delete_nidl_bisonparser_activation_record(void * p)
{
 if (p)
    free((void *)p);
}

/*****************************************************************
 *
 * Get/Set/Initialize methods
 *
 *****************************************************************/

void *
get_current_nidl_bisonparser_activation()
  {
    nidl_bisonparser_activation_record * p;

    p = (nidl_bisonparser_activation_record * )
                new_nidl_bisonparser_activation_record();

    /*
     * save the statics internal to the parser
     *
     */

     p->yychar = yychar;
     p->yynerrs = yynerrs;
     p->yylval = yylval;

     return (void *)p;
  }

void
set_current_nidl_bisonparser_activation(void * ptr)
  {

    nidl_bisonparser_activation_record * p =
      (nidl_bisonparser_activation_record *)ptr;

    // restore the statics


     yychar = p->yychar;
     yynerrs = p->yynerrs;
     yylval = p->yylval;


  }

void
init_new_nidl_bisonparser_activation()
  {
    // set some initial conditions for a new Bison parser state

    yynerrs = 0;

  }

/* preserve coding style vim: set tw=78 sw=3 ts=3 : */

