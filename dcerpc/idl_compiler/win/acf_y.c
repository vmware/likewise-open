
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
#define yyparse         acf_yyparse
#define yylex           acf_yylex
#define yyerror         acf_yyerror
#define yylval          acf_yylval
#define yychar          acf_yychar
#define yydebug         acf_yydebug
#define yynerrs         acf_yynerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 44 "acf_y.y"


  /* Tank Trap to stop older yacc parsers */
  /* Bison defines the macro YYBISON      */

#ifndef YYBISON
This grammar file needs to be built with GNU Bison V1.25 or later.
  GNU Bison can be be obtained from ftp://prep.ai.mit.edu:/pub/gnu
#endif

/* Declarations in this section are copied from yacc source to y_tab.c. */

#include <nidl.h>               /* IDL compiler-wide defs */
#include <acf.h>                /* ACF include file - keep first! */

#include <ast.h>                /* Abstract Syntax Tree defs */
#include <astp.h>               /* Import AST processing routine defs */
#include <command.h>            /* Command line defs */
#include <message.h>            /* Error message defs */
#include <nidlmsg.h>            /* Error message IDs */
#include <files.h>
#include <propagat.h>
#include <checker.h>
#include <flex_bison_support.h>

extern AST_interface_n_t *the_interface;    /* Ptr to AST interface node */
extern boolean ASTP_parsing_main_idl;       /* True when parsing main IDL */
extern int acf_yylineno;
extern int acf_yylex(void);

typedef union                   /* Attributes bitmask */
{
    struct
    {
        unsigned auto_handle    : 1;
        unsigned binding_callout: 1;
        unsigned code           : 1;
        unsigned comm_status    : 1;
        unsigned cs_char        : 1;
        unsigned cs_drtag       : 1;
        unsigned cs_rtag        : 1;
        unsigned cs_stag        : 1;
        unsigned cs_tag_rtn     : 1;
        unsigned decode         : 1;
        unsigned enable_allocate: 1;
        unsigned encode         : 1;
        unsigned explicit_handle: 1;
        unsigned extern_exceps  : 1;
        unsigned fault_status   : 1;
        unsigned heap           : 1;
        unsigned implicit_handle: 1;
        unsigned in_line        : 1;
        unsigned nocode         : 1;
        unsigned out_of_line    : 1;
        unsigned represent_as   : 1;
        unsigned nocancel       : 1;
    }   bit;
    long    mask;
}   acf_attrib_t;

typedef struct acf_param_t      /* ACF parameter info structure */
{
    struct acf_param_t *next;                   /* Forward link */
    acf_attrib_t    parameter_attr;             /* Parameter attributes */
    NAMETABLE_id_t  param_id;                   /* Parameter name */
}   acf_param_t;


static acf_attrib_t interface_attr,     /* Interface attributes */
                    type_attr,          /* Type attributes */
                    operation_attr,     /* Operation attributes */
                    parameter_attr;     /* Parameter attributes */

static char const *interface_name;        /* Interface name */
static char const *impl_name;             /* Implicit handle name */
static char const *type_name;             /* Current type name */
static char const *repr_type_name;        /* Current represent_as type */
static char const *cs_char_type_name;     /* Current cs_char type */
static char const *operation_name;        /* Current operation name */
static char const *cs_tag_rtn_name;       /* Current cs_tag_rtn name */
static char const *binding_callout_name;  /* Current binding_callout name */

static boolean  named_type;             /* True if parsed type is named type */

static AST_include_n_t  *include_list,  /* List of AST include nodes */
                        *include_p;     /* Ptr to a created include node */

static acf_param_t  *parameter_list,        /* Param list for curr. operation */
                    *parameter_free_list;   /* List of available acf_param_t */
static boolean      parameter_attr_list;    /* True if param attrs specified */

static boolean      *cmd_opt;       /* Array of command option flags */
static void         **cmd_val;      /* Array of command option values */

/*
 * Forward declarations to shut up the compiler
 */

void acf_init(boolean *, void **, char *);
void acf_cleanup();
static boolean lookup_exception(NAMETABLE_id_t, boolean, AST_exception_n_t **);
static boolean lookup_type(char const *, boolean, NAMETABLE_id_t *, AST_type_n_t **);
static boolean lookup_operation(char const *, boolean, NAMETABLE_id_t *, AST_operation_n_t **);
static boolean lookup_parameter(AST_operation_n_t *, char const *, boolean, NAMETABLE_id_t *, AST_parameter_n_t **);
static boolean lookup_rep_as_name(AST_type_p_n_t *, NAMETABLE_id_t, AST_type_n_t **, char const **);
static boolean lookup_cs_char_name(AST_type_p_n_t *, NAMETABLE_id_t, AST_type_n_t **, char const * *);
static acf_param_t * alloc_param();
static void free_param(acf_param_t *);
static void free_param_list(acf_param_t **);
void add_param_to_list(acf_param_t *, acf_param_t **);
static void append_parameter(AST_operation_n_t *, char const *, acf_attrib_t *);
static void process_rep_as_type(AST_interface_n_t *, AST_type_n_t *, char const *);
static void process_cs_char_type(AST_interface_n_t *, AST_type_n_t *, char const *);
#ifdef DUMPERS
static void dump_attributes(char *, char const *, acf_attrib_t *);
#endif
/*
 * Warning and Error stuff
 */

/*
**  a c f _ e r r o r
**
**  Issues an error message, and bumps the error count.
*/

#define acf_error(...) log_error(acf_yylineno, __VA_ARGS__)

/*
**  a c f _ w a r n i n g
**
**  Issues a warning message.
*/

#define acf_warning(...) log_warning(acf_yylineno, __VA_ARGS__)



/* Line 189 of yacc.c  */
#line 220 "acf_y.c"

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
     AUTO_HANDLE_KW = 258,
     BINDING_CALLOUT_KW = 259,
     CODE_KW = 260,
     COMM_STATUS_KW = 261,
     CS_CHAR_KW = 262,
     CS_TAG_RTN_KW = 263,
     ENABLE_ALLOCATE_KW = 264,
     EXPLICIT_HANDLE_KW = 265,
     EXTERN_EXCEPS_KW = 266,
     FAULT_STATUS_KW = 267,
     HANDLE_T_KW = 268,
     HEAP_KW = 269,
     IMPLICIT_HANDLE_KW = 270,
     INCLUDE_KW = 271,
     INTERFACE_KW = 272,
     IN_LINE_KW = 273,
     NOCODE_KW = 274,
     NOCANCEL_KW = 275,
     OUT_OF_LINE_KW = 276,
     REPRESENT_AS_KW = 277,
     TYPEDEF_KW = 278,
     COMMA = 279,
     LBRACE = 280,
     LBRACKET = 281,
     LPAREN = 282,
     RBRACE = 283,
     RBRACKET = 284,
     RPAREN = 285,
     SEMI = 286,
     TILDE = 287,
     UNKNOWN = 288,
     IDENTIFIER = 289,
     STRING = 290
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 192 "acf_y.y"

    NAMETABLE_id_t  y_id;       /* Identifier */
    STRTAB_str_t    y_string;   /* Text string */



/* Line 214 of yacc.c  */
#line 298 "acf_y.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 310 "acf_y.c"

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
#define YYFINAL  30
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   145

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  59
/* YYNRULES -- Number of rules.  */
#define YYNRULES  114
/* YYNRULES -- Number of states.  */
#define YYNSTATES  164

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

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
      35
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,    10,    14,    15,    17,    21,    23,
      25,    27,    29,    31,    33,    35,    37,    39,    41,    43,
      45,    50,    53,    55,    57,    59,    61,    66,    68,    70,
      74,    76,    78,    82,    85,    87,    90,    92,    95,    98,
     101,   104,   107,   110,   113,   115,   119,   121,   124,   128,
     130,   134,   136,   139,   140,   143,   146,   149,   151,   155,
     157,   159,   161,   163,   165,   170,   172,   177,   179,   182,
     184,   188,   193,   197,   198,   200,   204,   206,   208,   210,
     212,   214,   216,   218,   220,   222,   227,   229,   234,   236,
     238,   239,   241,   245,   248,   252,   253,   255,   259,   261,
     263,   265,   267,   269,   271,   273,   275,   277,   279,   281,
     283,   285,   287,   289,   291
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      37,     0,    -1,    38,    51,    -1,    39,    17,    50,    -1,
      26,    40,    29,    -1,    -1,    41,    -1,    40,    24,    41,
      -1,    85,    -1,    86,    -1,    74,    -1,    76,    -1,    88,
      -1,    89,    -1,    91,    -1,    92,    -1,    42,    -1,    84,
      -1,    47,    -1,    34,    -1,    15,    27,    43,    30,    -1,
      44,    46,    -1,    45,    -1,    34,    -1,    13,    -1,    34,
      -1,    11,    27,    48,    30,    -1,    11,    -1,    49,    -1,
      48,    24,    49,    -1,    34,    -1,    34,    -1,    25,    52,
      28,    -1,    25,    28,    -1,     1,    -1,     1,    28,    -1,
      53,    -1,    52,    53,    -1,    54,    31,    -1,    57,    31,
      -1,    68,    31,    -1,     1,    31,    -1,    16,    55,    -1,
      16,     1,    -1,    56,    -1,    55,    24,    56,    -1,    35,
      -1,    23,     1,    -1,    23,    60,    58,    -1,    59,    -1,
      58,    24,    59,    -1,    34,    -1,    26,    61,    -1,    -1,
      62,    29,    -1,     1,    31,    -1,     1,    29,    -1,    63,
      -1,    62,    24,    63,    -1,    64,    -1,    66,    -1,    90,
      -1,    91,    -1,    92,    -1,    22,    27,    65,    30,    -1,
      34,    -1,     7,    27,    67,    30,    -1,    34,    -1,    71,
      69,    -1,    70,    -1,    69,    24,    70,    -1,    34,    27,
      78,    30,    -1,    26,    72,    29,    -1,    -1,    73,    -1,
      72,    24,    73,    -1,    93,    -1,    85,    -1,    86,    -1,
      76,    -1,    87,    -1,    88,    -1,    89,    -1,    94,    -1,
      34,    -1,     4,    27,    75,    30,    -1,    34,    -1,     8,
      27,    77,    30,    -1,    34,    -1,    79,    -1,    -1,    80,
      -1,    79,    24,    80,    -1,    81,    34,    -1,    26,    82,
      29,    -1,    -1,    83,    -1,    82,    24,    83,    -1,    93,
      -1,    94,    -1,    90,    -1,    91,    -1,    92,    -1,    34,
      -1,     3,    -1,     5,    -1,    19,    -1,     9,    -1,    10,
      -1,    20,    -1,    14,    -1,    18,    -1,    21,    -1,     6,
      -1,    12,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   256,   256,   260,   354,   355,   359,   360,   364,   370,
     376,   382,   388,   394,   400,   406,   412,   418,   424,   430,
     450,   454,   458,   462,   470,   474,   481,   482,   498,   499,
     503,   513,   520,   521,   522,   524,   529,   530,   534,   535,
     536,   537,   549,   557,   562,   563,   567,   610,   612,   621,
     625,   632,   682,   683,   687,   688,   692,   699,   700,   704,
     710,   716,   722,   728,   737,   741,   748,   752,   759,   767,
     768,   772,   929,   930,   934,   935,   939,   945,   951,   957,
     963,   969,   975,   981,   987,  1007,  1011,  1018,  1022,  1029,
    1030,  1034,  1035,  1039,  1072,  1077,  1083,  1084,  1088,  1094,
    1100,  1106,  1112,  1118,  1143,  1144,  1145,  1146,  1147,  1148,
    1149,  1150,  1151,  1152,  1153
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "AUTO_HANDLE_KW", "BINDING_CALLOUT_KW",
  "CODE_KW", "COMM_STATUS_KW", "CS_CHAR_KW", "CS_TAG_RTN_KW",
  "ENABLE_ALLOCATE_KW", "EXPLICIT_HANDLE_KW", "EXTERN_EXCEPS_KW",
  "FAULT_STATUS_KW", "HANDLE_T_KW", "HEAP_KW", "IMPLICIT_HANDLE_KW",
  "INCLUDE_KW", "INTERFACE_KW", "IN_LINE_KW", "NOCODE_KW", "NOCANCEL_KW",
  "OUT_OF_LINE_KW", "REPRESENT_AS_KW", "TYPEDEF_KW", "COMMA", "LBRACE",
  "LBRACKET", "LPAREN", "RBRACE", "RBRACKET", "RPAREN", "SEMI", "TILDE",
  "UNKNOWN", "IDENTIFIER", "STRING", "$accept", "acf_interface",
  "acf_interface_header", "acf_interface_attr_list", "acf_interface_attrs",
  "acf_interface_attr", "acf_implicit_handle_attr", "acf_implicit_handle",
  "acf_impl_type", "acf_handle_type", "acf_impl_name",
  "acf_extern_exceps_attr", "acf_ext_excep_list", "acf_ext_excep",
  "acf_interface_name", "acf_interface_body", "acf_body_elements",
  "acf_body_element", "acf_include", "acf_include_list",
  "acf_include_name", "acf_type_declaration", "acf_named_type_list",
  "acf_named_type", "acf_type_attr_list", "acf_rest_of_attr_list",
  "acf_type_attrs", "acf_type_attr", "acf_represent_attr", "acf_repr_type",
  "acf_cs_char_attr", "acf_cs_char_type", "acf_operation_declaration",
  "acf_operations", "acf_operation", "acf_op_attr_list", "acf_op_attrs",
  "acf_op_attr", "acf_binding_callout_attr", "acf_binding_callout_name",
  "acf_cs_tag_rtn_attr", "acf_cs_tag_rtn_name", "acf_parameter_list",
  "acf_parameters", "acf_parameter", "acf_param_attr_list",
  "acf_param_attrs", "acf_param_attr", "acf_auto_handle_attr",
  "acf_code_attr", "acf_nocode_attr", "acf_enable_allocate_attr",
  "acf_explicit_handle_attr", "acf_nocancel_attr", "acf_heap_attr",
  "acf_inline_attr", "acf_outofline_attr", "acf_commstat_attr",
  "acf_faultstat_attr", 0
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
     285,   286,   287,   288,   289,   290
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    36,    37,    38,    39,    39,    40,    40,    41,    41,
      41,    41,    41,    41,    41,    41,    41,    41,    41,    41,
      42,    43,    44,    44,    45,    46,    47,    47,    48,    48,
      49,    50,    51,    51,    51,    51,    52,    52,    53,    53,
      53,    53,    54,    54,    55,    55,    56,    57,    57,    58,
      58,    59,    60,    60,    61,    61,    61,    62,    62,    63,
      63,    63,    63,    63,    64,    65,    66,    67,    68,    69,
      69,    70,    71,    71,    72,    72,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    74,    75,    76,    77,    78,
      78,    79,    79,    80,    81,    81,    82,    82,    83,    83,
      83,    83,    83,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     3,     3,     0,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     2,     1,     1,     1,     1,     4,     1,     1,     3,
       1,     1,     3,     2,     1,     2,     1,     2,     2,     2,
       2,     2,     2,     2,     1,     3,     1,     2,     3,     1,
       3,     1,     2,     0,     2,     2,     2,     1,     3,     1,
       1,     1,     1,     1,     4,     1,     4,     1,     2,     1,
       3,     4,     3,     0,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     4,     1,     4,     1,     1,
       0,     1,     3,     2,     3,     0,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     0,     0,     0,   104,     0,   105,     0,   108,
      27,     0,   111,   106,   109,   112,    19,     0,     6,    16,
      18,    10,    11,    17,     8,     9,    12,    13,    14,    15,
       1,    34,     0,     2,     0,     0,     0,     0,     0,     0,
       4,    35,     0,     0,     0,     0,    33,     0,    36,     0,
       0,     0,     0,    31,     3,    86,     0,    88,     0,    30,
       0,    28,    24,    23,     0,     0,    22,     7,    41,    43,
      46,    42,    44,    47,     0,     0,   113,   107,   114,    84,
       0,    74,    79,    77,    78,    80,    81,    82,    76,    83,
      32,    37,    38,    39,    40,     0,    68,    69,    85,    87,
       0,    26,    20,    25,    21,     0,     0,     0,   110,     0,
      52,     0,    57,    59,    60,    61,    62,    63,    51,    48,
      49,     0,    72,    90,     0,    29,    45,    56,    55,     0,
       0,     0,    54,     0,    75,     0,     0,    89,    91,     0,
      70,    67,     0,    65,     0,    58,    50,   103,     0,    96,
     100,   101,   102,    98,    99,    71,    95,    93,    66,    64,
       0,    94,    92,    97
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,    17,    18,    19,    64,    65,    66,
     104,    20,    60,    61,    54,    33,    47,    48,    49,    71,
      72,    50,   119,   120,    75,   110,   111,   112,   113,   144,
     114,   142,    51,    96,    97,    52,    80,    81,    21,    56,
      22,    58,   136,   137,   138,   139,   148,   149,    23,    24,
      25,    85,    26,    27,   115,    28,    29,    88,    89
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -128
static const yytype_int8 yypact[] =
{
      -5,    80,    56,    25,    23,  -128,    41,  -128,    62,  -128,
      65,    66,  -128,  -128,  -128,  -128,  -128,    36,  -128,  -128,
    -128,  -128,  -128,  -128,  -128,  -128,  -128,  -128,  -128,  -128,
    -128,    69,     8,  -128,    60,    68,    70,    71,    15,    80,
    -128,  -128,    44,     6,    12,    10,  -128,     9,  -128,    75,
      77,    78,    79,  -128,  -128,  -128,    82,  -128,    85,  -128,
     -13,  -128,  -128,  -128,    86,    83,  -128,  -128,  -128,  -128,
    -128,    94,  -128,  -128,    52,    87,  -128,  -128,  -128,  -128,
      40,  -128,  -128,  -128,  -128,  -128,  -128,  -128,  -128,  -128,
    -128,  -128,  -128,  -128,  -128,    92,    96,  -128,  -128,  -128,
      71,  -128,  -128,  -128,  -128,    88,   -17,    95,  -128,    97,
    -128,    47,  -128,  -128,  -128,  -128,  -128,  -128,  -128,   101,
    -128,    10,  -128,    29,    79,  -128,  -128,  -128,  -128,    93,
      98,    89,  -128,    87,  -128,    33,    99,   102,  -128,   100,
    -128,  -128,   103,  -128,   105,  -128,  -128,  -128,    48,  -128,
    -128,  -128,  -128,  -128,  -128,  -128,   104,  -128,  -128,  -128,
      33,  -128,  -128,  -128
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -128,  -128,  -128,  -128,  -128,   106,  -128,  -128,  -128,  -128,
    -128,  -128,  -128,    28,  -128,  -128,  -128,    84,  -128,  -128,
      31,  -128,  -128,     4,  -128,  -128,  -128,     7,  -128,  -128,
    -128,  -128,  -128,  -128,    16,  -128,  -128,    18,  -128,  -128,
     -43,  -128,  -128,  -128,   -15,  -128,  -128,   -18,  -128,   -42,
     -41,  -128,   -40,   -39,  -127,   -74,   -73,  -112,  -108
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -96
static const yytype_int16 yytable[] =
{
     116,   117,    82,    83,    84,    86,    87,    69,   150,    42,
      42,   100,   127,    73,   128,     7,    76,   101,     8,    77,
       9,     1,    78,   153,    43,    43,    31,   154,    62,    13,
      14,    44,    44,   150,    45,    45,    46,    90,    74,    76,
      34,    70,   -73,   -73,    79,    78,   -53,   108,   153,    63,
      32,    12,   154,   106,    15,   135,    30,   116,   117,   107,
      39,   151,   152,   -95,   121,    40,   108,   147,    35,   122,
      12,   131,   160,    15,   109,    68,   132,   161,    82,    83,
      84,    86,    87,     5,     6,     7,   151,   152,     8,    36,
       9,    10,    37,    38,    53,    11,   107,    41,    12,    13,
      14,    15,    55,   108,    57,    59,    92,    12,    93,    94,
      15,   109,    98,    95,    16,    99,   102,   103,   105,   123,
     124,   118,   129,    70,   130,   133,   156,   141,   125,   155,
     135,    91,   143,   158,   157,   159,   126,   146,   145,   134,
     140,   162,   163,     0,     0,    67
};

static const yytype_int16 yycheck[] =
{
      74,    74,    45,    45,    45,    45,    45,     1,   135,     1,
       1,    24,    29,     1,    31,     5,     6,    30,     8,     9,
      10,    26,    12,   135,    16,    16,     1,   135,    13,    19,
      20,    23,    23,   160,    26,    26,    28,    28,    26,     6,
      17,    35,    34,    34,    34,    12,    34,    14,   160,    34,
      25,    18,   160,     1,    21,    26,     0,   131,   131,     7,
      24,   135,   135,    34,    24,    29,    14,    34,    27,    29,
      18,    24,    24,    21,    22,    31,    29,    29,   121,   121,
     121,   121,   121,     3,     4,     5,   160,   160,     8,    27,
      10,    11,    27,    27,    34,    15,     7,    28,    18,    19,
      20,    21,    34,    14,    34,    34,    31,    18,    31,    31,
      21,    22,    30,    34,    34,    30,    30,    34,    24,    27,
      24,    34,    27,    35,    27,    24,    24,    34,   100,    30,
      26,    47,    34,    30,    34,    30,   105,   133,   131,   121,
     124,   156,   160,    -1,    -1,    39
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    26,    37,    38,    39,     3,     4,     5,     8,    10,
      11,    15,    18,    19,    20,    21,    34,    40,    41,    42,
      47,    74,    76,    84,    85,    86,    88,    89,    91,    92,
       0,     1,    25,    51,    17,    27,    27,    27,    27,    24,
      29,    28,     1,    16,    23,    26,    28,    52,    53,    54,
      57,    68,    71,    34,    50,    34,    75,    34,    77,    34,
      48,    49,    13,    34,    43,    44,    45,    41,    31,     1,
      35,    55,    56,     1,    26,    60,     6,     9,    12,    34,
      72,    73,    76,    85,    86,    87,    88,    89,    93,    94,
      28,    53,    31,    31,    31,    34,    69,    70,    30,    30,
      24,    30,    30,    34,    46,    24,     1,     7,    14,    22,
      61,    62,    63,    64,    66,    90,    91,    92,    34,    58,
      59,    24,    29,    27,    24,    49,    56,    29,    31,    27,
      27,    24,    29,    24,    73,    26,    78,    79,    80,    81,
      70,    34,    67,    34,    65,    63,    59,    34,    82,    83,
      90,    91,    92,    93,    94,    30,    24,    34,    30,    30,
      24,    29,    80,    83
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
        case 3:

/* Line 1455 of yacc.c  */
#line 261 "acf_y.y"
    {
        char const      *ast_int_name;  /* Interface name in AST node */
        NAMETABLE_id_t  impl_name_id;   /* Nametable id of impl_handle var */

#ifdef DUMPERS
        if (cmd_opt[opt_dump_acf])
            dump_attributes("ACF interface", interface_name, &interface_attr);
#endif

        /* Store source information. */
        if (the_interface->fe_info != NULL)
        {
            the_interface->fe_info->acf_file = error_file_name_id;
            the_interface->fe_info->acf_source_line = acf_yylineno;
        }

        /*
         *  Interface attributes are saved for main and imported interfaces.
         *  the_interface = pointer to main or imported interface node
         *
         *  Make sure that the interface name in the ACF agrees with the
         *  interface name in the main IDL file.  Then set the parsed
         *  attributes in the interface node.
         *
         *  interface_attr = bitmask of interface attributes parsed.
         *  interface_name = ACF interface name parsed.
         */

        NAMETABLE_id_to_string(the_interface->name, &ast_int_name);

        if (strcmp(interface_name, ast_int_name) != 0)
        {
            char const *acf_int_name;   /* Ptr to permanent copy */
            NAMETABLE_id_t name_id;     /* Handle on permanent copy */
            char const *file_name;      /* Related file name */

            name_id = NAMETABLE_add_id(interface_name);
            NAMETABLE_id_to_string(name_id, &acf_int_name);

            STRTAB_str_to_string(the_interface->fe_info->file, &file_name);

            acf_error(NIDL_INTNAMDIF, acf_int_name, ast_int_name);
            acf_error(NIDL_NAMEDECLAT, ast_int_name, file_name,
                      the_interface->fe_info->source_line);
        }
        else
        {
            if (interface_attr.bit.code)
                AST_SET_CODE(the_interface);
            if (interface_attr.bit.nocode)
                AST_SET_NO_CODE(the_interface);
            if (interface_attr.bit.decode)
                AST_SET_DECODE(the_interface);
            if (interface_attr.bit.encode)
                AST_SET_ENCODE(the_interface);
            if (interface_attr.bit.explicit_handle)
                AST_SET_EXPLICIT_HANDLE(the_interface);
            if (interface_attr.bit.in_line)
                AST_SET_IN_LINE(the_interface);
            if (interface_attr.bit.out_of_line)
                AST_SET_OUT_OF_LINE(the_interface);
            if (interface_attr.bit.auto_handle)
                AST_SET_AUTO_HANDLE(the_interface);
            if (interface_attr.bit.nocancel)
                AST_SET_NO_CANCEL(the_interface);

            if (interface_attr.bit.cs_tag_rtn)
                the_interface->cs_tag_rtn_name = NAMETABLE_add_id(cs_tag_rtn_name);
            if (interface_attr.bit.binding_callout)
                the_interface->binding_callout_name = NAMETABLE_add_id(binding_callout_name);

            if (interface_attr.bit.implicit_handle)
            {
                /* Store the [implicit_handle] variable name in nametbl. */
                impl_name_id = NAMETABLE_add_id(impl_name);
		
					 ASTP_set_implicit_handle(the_interface,
					 	named_type ? NAMETABLE_add_id(type_name) : NAMETABLE_NIL_ID,
						impl_name_id);

            }
        }

        interface_name = NULL;
        type_name = NULL;
        impl_name = NULL;
        binding_callout_name = NULL;
        cs_tag_rtn_name = NULL;
        interface_attr.mask = 0;        /* Reset attribute mask */
    ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 365 "acf_y.y"
    {
        if (interface_attr.bit.code)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        interface_attr.bit.code = TRUE;
    ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 371 "acf_y.y"
    {
        if (interface_attr.bit.nocode)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        interface_attr.bit.nocode = TRUE;
    ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 377 "acf_y.y"
    {
        if (interface_attr.bit.binding_callout)
            log_error(acf_yylineno, NIDL_ATTRUSEMULT, NULL);
        interface_attr.bit.binding_callout = TRUE;
    ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 383 "acf_y.y"
    {
        if (interface_attr.bit.cs_tag_rtn)
            log_error(acf_yylineno, NIDL_ATTRUSEMULT, NULL);
        interface_attr.bit.cs_tag_rtn = TRUE;
    ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 389 "acf_y.y"
    {
        if (interface_attr.bit.explicit_handle)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        interface_attr.bit.explicit_handle = TRUE;
    ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 395 "acf_y.y"
    {
        if (interface_attr.bit.nocancel)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        interface_attr.bit.nocancel = TRUE;
    ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 401 "acf_y.y"
    {
        if (interface_attr.bit.in_line)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        interface_attr.bit.in_line = TRUE;
    ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 407 "acf_y.y"
    {
        if (interface_attr.bit.out_of_line)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        interface_attr.bit.out_of_line = TRUE;
    ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 413 "acf_y.y"
    {
        if (interface_attr.bit.implicit_handle)
            log_error(acf_yylineno, NIDL_ATTRUSEMULT, NULL);
        interface_attr.bit.implicit_handle = TRUE;
    ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 419 "acf_y.y"
    {
        if (interface_attr.bit.auto_handle)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        interface_attr.bit.auto_handle = TRUE;
    ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 425 "acf_y.y"
    {
        if (interface_attr.bit.extern_exceps)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        interface_attr.bit.extern_exceps = TRUE;
    ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 431 "acf_y.y"
    {
        if (NAMETABLE_add_id("decode") == (yyvsp[(1) - (1)].y_id))
        {
            if (interface_attr.bit.decode)
                log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
            interface_attr.bit.decode = TRUE;
        }
        else if (NAMETABLE_add_id("encode") == (yyvsp[(1) - (1)].y_id))
        {
            if (interface_attr.bit.encode)
                log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
            interface_attr.bit.encode = TRUE;
        }
        else
            log_error(acf_yylineno, NIDL_ERRINATTR, NULL);
    ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 459 "acf_y.y"
    {
        named_type = FALSE;
    ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 463 "acf_y.y"
    {
        NAMETABLE_id_to_string((yyvsp[(1) - (1)].y_id), &type_name);
        named_type = TRUE;
    ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 475 "acf_y.y"
    {
        NAMETABLE_id_to_string((yyvsp[(1) - (1)].y_id), &impl_name);
    ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 483 "acf_y.y"
    {
        if (ASTP_parsing_main_idl)
        {
            AST_exception_n_t *excep_p;
            for (excep_p = the_interface->exceptions;
                 excep_p != NULL;
                 excep_p = excep_p->next)
            {
                AST_SET_EXTERN(excep_p);
            }
        }
    ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 504 "acf_y.y"
    {
        AST_exception_n_t *excep_p;
        if (ASTP_parsing_main_idl)
            if (lookup_exception((yyvsp[(1) - (1)].y_id), TRUE, &excep_p))
                AST_SET_EXTERN(excep_p);
    ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 514 "acf_y.y"
    {
        NAMETABLE_id_to_string((yyvsp[(1) - (1)].y_id), &interface_name);
    ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 523 "acf_y.y"
    { log_error(acf_yylineno, NIDL_SYNTAXERR, NULL); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 525 "acf_y.y"
    { log_error(acf_yylineno, NIDL_SYNTAXERR, NULL); ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 538 "acf_y.y"
    {
            log_error(acf_yylineno, NIDL_SYNTAXERR, NULL);
            /* Re-initialize attr masks to avoid sticky attributes */
            interface_attr.mask = 0;
            type_attr.mask      = 0;
            operation_attr.mask = 0;
            parameter_attr.mask = 0;
        ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 550 "acf_y.y"
    {
        if (ASTP_parsing_main_idl)
            the_interface->includes = (AST_include_n_t *)
                AST_concat_element((ASTP_node_t *)the_interface->includes,
                                   (ASTP_node_t *)include_list);
        include_list = NULL;
        ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 558 "acf_y.y"
    { log_error(acf_yylineno, NIDL_SYNTAXERR, NULL); ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 568 "acf_y.y"
    {
        if (ASTP_parsing_main_idl)
        {
            char const      *parsed_include_file;
            char            include_type[PATH_MAX];
            char            include_file[PATH_MAX];
            STRTAB_str_t    include_file_id;

            STRTAB_str_to_string((yyvsp[(1) - (1)].y_string), &parsed_include_file);

            /*
             * Log warning if include name contains a file extension.
             * Tack on the correct extension based on the -lang option.
             */
            FILE_parse(parsed_include_file, (char *)NULL, (char *)NULL,
                       include_type);
            if (include_type[0] != '\0')
                acf_warning(NIDL_INCLUDEXT);

            FILE_form_filespec(parsed_include_file, (char *)NULL,
			       ".h",
                               (char *)NULL, include_file);

            /* Create an include node. */
            include_file_id = STRTAB_add_string(include_file);
            include_p = AST_include_node(include_file_id, (yyvsp[(1) - (1)].y_string));

            /* Store source information. */
            if (include_p->fe_info != NULL)
            {
                include_p->fe_info->acf_file = error_file_name_id;
                include_p->fe_info->acf_source_line = acf_yylineno;
            }

            include_list = (AST_include_n_t *)
                AST_concat_element((ASTP_node_t *)include_list,
                                   (ASTP_node_t *)include_p);
        }
    ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 611 "acf_y.y"
    { log_error(acf_yylineno, NIDL_SYNTAXERR, NULL); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 613 "acf_y.y"
    {
        type_attr.mask = 0;             /* Reset attribute mask */
        repr_type_name = NULL;          /* Reset represent_as type name */
        cs_char_type_name = NULL;       /* Reset cs_char type name */
    ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 622 "acf_y.y"
    {
        type_name = NULL;               /* Reset type name */
    ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 626 "acf_y.y"
    {
        type_name = NULL;               /* Reset type name */
    ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 633 "acf_y.y"
    {
        NAMETABLE_id_t  type_id;        /* Nametable id of type name */
        AST_type_n_t    *type_p;        /* Ptr to AST type node */

        NAMETABLE_id_to_string((yyvsp[(1) - (1)].y_id), &type_name);

#ifdef DUMPERS
        if (cmd_opt[opt_dump_acf])
            dump_attributes("ACF type", type_name, &type_attr);
#endif

        /*
         *  Lookup the type_name parsed and verify that it is a valid type
         *  node.  Then set the parsed attributes in the type node.
         *
         *  type_attr = bitmask of type attributes parsed.
         *  type_name = name of type_t node to look up.
         *  [repr_type_name] = name of represent_as type.
         *  [cs_char_type_name] = name of cs_char type.
         */

        if (lookup_type(type_name, TRUE, &type_id, &type_p))
        {
            /* Store source information. */
            if (type_p->fe_info != NULL)
            {
                type_p->fe_info->acf_file = error_file_name_id;
                type_p->fe_info->acf_source_line = acf_yylineno;
            }

            if (type_attr.bit.heap
                && type_p->kind != AST_pipe_k
                && !AST_CONTEXT_RD_SET(type_p))
                PROP_set_type_attr(type_p,AST_HEAP);
            if (type_attr.bit.in_line)
                PROP_set_type_attr(type_p,AST_IN_LINE);
            if ((type_attr.bit.out_of_line) &&
                (type_p->kind != AST_pointer_k) &&
                (type_p->xmit_as_type == NULL))
                PROP_set_type_attr(type_p,AST_OUT_OF_LINE);
            if (type_attr.bit.represent_as)
                process_rep_as_type(the_interface, type_p, repr_type_name);
            if (type_attr.bit.cs_char)
                process_cs_char_type(the_interface, type_p, cs_char_type_name);
        }
    ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 689 "acf_y.y"
    {
        log_error(acf_yylineno, NIDL_MISSONATTR, NULL);
        ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 693 "acf_y.y"
    {
        log_error(acf_yylineno, NIDL_ERRINATTR, NULL);
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 705 "acf_y.y"
    {
        if (type_attr.bit.represent_as)
            log_error(acf_yylineno, NIDL_ATTRUSEMULT, NULL);
        type_attr.bit.represent_as = TRUE;
    ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 711 "acf_y.y"
    {
        if (type_attr.bit.cs_char)
            log_error(acf_yylineno, NIDL_ATTRUSEMULT, NULL);
        type_attr.bit.cs_char = TRUE;
    ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 717 "acf_y.y"
    {
        if (type_attr.bit.heap)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        type_attr.bit.heap = TRUE;
    ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 723 "acf_y.y"
    {
        if (type_attr.bit.in_line)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        type_attr.bit.in_line = TRUE;
    ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 729 "acf_y.y"
    {
        if (type_attr.bit.out_of_line)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        type_attr.bit.out_of_line = TRUE;
    ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 742 "acf_y.y"
    {
        NAMETABLE_id_to_string((yyvsp[(1) - (1)].y_id), &repr_type_name);
    ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 753 "acf_y.y"
    {
        NAMETABLE_id_to_string((yyvsp[(1) - (1)].y_id), &cs_char_type_name);
    ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 760 "acf_y.y"
    {
        operation_attr.mask = 0;        /* Reset attribute mask */
        cs_tag_rtn_name     = NULL;     /* Reset cs_tag_rtn name */
    ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 773 "acf_y.y"
    {
        acf_param_t         *p;         /* Ptr to local parameter structure */
        NAMETABLE_id_t      op_id;      /* Nametable id of operation name */
        NAMETABLE_id_t      param_id;   /* Nametable id of parameter name */
        AST_operation_n_t   *op_p;      /* Ptr to AST operation node */
        AST_parameter_n_t   *param_p;   /* Ptr to AST parameter node */
        boolean             do_log_error;  /* TRUE => error if name not found */
        char const          *param_name;/* character string of param id */

        NAMETABLE_id_to_string((yyvsp[(1) - (4)].y_id), &operation_name);
#ifdef DUMPERS
        if (cmd_opt[opt_dump_acf])
            dump_attributes("ACF operation", operation_name, &operation_attr);
#endif

        /*
         *  Operation and parameter attributes are ignored for imported
         *  interfaces.  Operations and parameters within imported interfaces
         *  are not put in the AST.
         */
        if (ASTP_parsing_main_idl)
        {
            /*
             *  Lookup the operation_name parsed and verify that it is a valid
             *  operation node.  Then set the parsed attributes in the operation
             *  node.  For each parameter_name that was parsed for this
             *  operation, chase the parameter list off the AST operation node
             *  to verify that it is a valid parameter for that operation.
             *  Then set the parsed attributes for that parameter into the
             *  relevant parameter node.
             *
             *  operation_attr = bitmask of operation attributes parsed.
             *  operation_name = name of routine_t node to look up.
             *  [cs_tag_rtn_name] = cs_tag_rtn name.
             *  parameter_list = linked list of parameter information.
             */

            if (lookup_operation(operation_name, TRUE, &op_id, &op_p))
            {
                /* Store source information. */
                if (op_p->fe_info != NULL)
                {
                    op_p->fe_info->acf_file = error_file_name_id;
                    op_p->fe_info->acf_source_line = acf_yylineno;
                }

                if (operation_attr.bit.comm_status)
                {
                    /*
                     * Assume the AST Builder always builds a result param,
                     * even for void operations.
                     */
                    AST_SET_COMM_STATUS(op_p->result);
                }
                if (operation_attr.bit.fault_status)
                    AST_SET_FAULT_STATUS(op_p->result);

                if (operation_attr.bit.code)
                    AST_SET_CODE(op_p);
                if (operation_attr.bit.nocode)
                    AST_SET_NO_CODE(op_p);
                if (operation_attr.bit.decode)
                    AST_SET_DECODE(op_p);
                if (operation_attr.bit.encode)
                    AST_SET_ENCODE(op_p);
                if (operation_attr.bit.enable_allocate)
                    AST_SET_ENABLE_ALLOCATE(op_p);
                if (operation_attr.bit.explicit_handle)
                    AST_SET_EXPLICIT_HANDLE(op_p);
                if (operation_attr.bit.nocancel)
                    AST_SET_NO_CANCEL(op_p);
                if (operation_attr.bit.cs_tag_rtn)
                    op_p->cs_tag_rtn_name = NAMETABLE_add_id(cs_tag_rtn_name);

                for (p = parameter_list ; p != NULL ; p = p->next)
                {
                    /*
                     * Most parameter attributes, if present, require that the
                     * referenced parameter be defined in the IDL.  If only
                     * [comm_status] and/or [fault_status] is present, the
                     * parameter  needn't be IDL-defined.
                     */
                    if (!p->parameter_attr.bit.heap
                        &&  !p->parameter_attr.bit.in_line
                        &&  !p->parameter_attr.bit.out_of_line
                        &&  !p->parameter_attr.bit.cs_stag
                        &&  !p->parameter_attr.bit.cs_drtag
                        &&  !p->parameter_attr.bit.cs_rtag
                        &&  (p->parameter_attr.bit.comm_status
                             || p->parameter_attr.bit.fault_status))
                        do_log_error = FALSE;
                    else
                        do_log_error = TRUE;

                    NAMETABLE_id_to_string(p->param_id, &param_name);
                    if (lookup_parameter(op_p, param_name, do_log_error,
                                         &param_id, &param_p))
                    {
                        /* Store source information. */
                        if (param_p->fe_info != NULL)
                        {
                            param_p->fe_info->acf_file = error_file_name_id;
                            param_p->fe_info->acf_source_line = acf_yylineno;
                        }

                        if (p->parameter_attr.bit.comm_status)
                            AST_SET_COMM_STATUS(param_p);
                        if (p->parameter_attr.bit.fault_status)
                            AST_SET_FAULT_STATUS(param_p);
                        if (p->parameter_attr.bit.heap)
                        {
                            AST_type_n_t *ref_type_p;
                            ref_type_p = param_follow_ref_ptr(param_p,
                                                              CHK_follow_ref);
                            if (ref_type_p->kind != AST_pipe_k
                                && !AST_CONTEXT_SET(param_p)
                                && !AST_CONTEXT_RD_SET(ref_type_p)
                                && !type_is_scalar(ref_type_p))
                                AST_SET_HEAP(param_p);
                        }
                        if (p->parameter_attr.bit.in_line)
                            AST_SET_IN_LINE(param_p);
                        /*
                         * We parse the [out_of_line] parameter attribute,
                         * but disallow it.
                         */
                        if (p->parameter_attr.bit.out_of_line)
                            acf_error(NIDL_INVOOLPRM);
                        if (p->parameter_attr.bit.cs_stag)
                            AST_SET_CS_STAG(param_p);
                        if (p->parameter_attr.bit.cs_drtag)
                            AST_SET_CS_DRTAG(param_p);
                        if (p->parameter_attr.bit.cs_rtag)
                            AST_SET_CS_RTAG(param_p);
                    }
                    else if (do_log_error == FALSE)
                    {
                        /*
                         * Lookup failed, but OK since the parameter only has
                         * attribute(s) that specify an additional parameter.
                         * Append a parameter to the operation parameter list.
                         */
                        NAMETABLE_id_to_string(p->param_id, &param_name);
                        append_parameter(op_p, param_name, &p->parameter_attr);
                    }
                }
            }
        }

        free_param_list(&parameter_list);       /* Free parameter list */

        operation_name = NULL;
    ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 940 "acf_y.y"
    {
        if (operation_attr.bit.comm_status)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        operation_attr.bit.comm_status = TRUE;
    ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 946 "acf_y.y"
    {
        if (operation_attr.bit.code)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        operation_attr.bit.code = TRUE;
    ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 952 "acf_y.y"
    {
        if (operation_attr.bit.nocode)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        operation_attr.bit.nocode = TRUE;
    ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 958 "acf_y.y"
    {
        if (operation_attr.bit.cs_tag_rtn)
            log_error(acf_yylineno, NIDL_ATTRUSEMULT, NULL);
        operation_attr.bit.cs_tag_rtn = TRUE;
    ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 964 "acf_y.y"
    {
        if (operation_attr.bit.enable_allocate)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        operation_attr.bit.enable_allocate = TRUE;
    ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 970 "acf_y.y"
    {
        if (operation_attr.bit.explicit_handle)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        operation_attr.bit.explicit_handle = TRUE;
    ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 976 "acf_y.y"
    {
        if (operation_attr.bit.nocancel)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        operation_attr.bit.nocancel = TRUE;
    ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 982 "acf_y.y"
    {
        if (operation_attr.bit.fault_status)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        operation_attr.bit.fault_status = TRUE;
    ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 988 "acf_y.y"
    {
        if (NAMETABLE_add_id("decode") == (yyvsp[(1) - (1)].y_id))
        {
            if (operation_attr.bit.decode)
                log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
            operation_attr.bit.decode = TRUE;
        }
        else if (NAMETABLE_add_id("encode") == (yyvsp[(1) - (1)].y_id))
        {
            if (operation_attr.bit.encode)
                log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
            operation_attr.bit.encode = TRUE;
        }
        else
            log_error(acf_yylineno, NIDL_ERRINATTR, NULL);
    ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 1012 "acf_y.y"
    {
        NAMETABLE_id_to_string((yyvsp[(1) - (1)].y_id), &binding_callout_name);
    ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1023 "acf_y.y"
    {
        NAMETABLE_id_to_string((yyvsp[(1) - (1)].y_id), &cs_tag_rtn_name);
    ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1040 "acf_y.y"
    {
#ifdef DUMPERS
        if (cmd_opt[opt_dump_acf])
        {
            char const *param_name;
            NAMETABLE_id_to_string((yyvsp[(2) - (2)].y_id), &param_name);
            dump_attributes("ACF parameter", param_name, &parameter_attr);
        }
#endif

        if (parameter_attr_list)        /* If there were param attributes: */
        {
            acf_param_t *p;             /* Pointer to parameter record */

            /*
             * Allocate and initialize a parameter record.
             */
            p = alloc_param();
            p->parameter_attr = parameter_attr;
            p->param_id = (yyvsp[(2) - (2)].y_id);

            /*
             * Add to end of parameter list.
             */
            add_param_to_list(p, &parameter_list);

            parameter_attr.mask = 0;
        }
    ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1073 "acf_y.y"
    {
        parameter_attr_list = TRUE;     /* Flag that we have param attributes */
    ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1077 "acf_y.y"
    {
        parameter_attr_list = FALSE;
    ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1089 "acf_y.y"
    {
        if (parameter_attr.bit.comm_status)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        parameter_attr.bit.comm_status = TRUE;
    ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1095 "acf_y.y"
    {
        if (parameter_attr.bit.fault_status)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        parameter_attr.bit.fault_status = TRUE;
    ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1101 "acf_y.y"
    {
        if (parameter_attr.bit.heap)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        parameter_attr.bit.heap = TRUE;
    ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1107 "acf_y.y"
    {
        if (parameter_attr.bit.in_line)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        parameter_attr.bit.in_line = TRUE;
    ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1113 "acf_y.y"
    {
        if (parameter_attr.bit.out_of_line)
            log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
        parameter_attr.bit.out_of_line = TRUE;
    ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1119 "acf_y.y"
    {
        if (NAMETABLE_add_id("cs_stag") == (yyvsp[(1) - (1)].y_id))
        {
            if (parameter_attr.bit.cs_stag)
                log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
            parameter_attr.bit.cs_stag = TRUE;
        }
        else if (NAMETABLE_add_id("cs_drtag") == (yyvsp[(1) - (1)].y_id))
        {
            if (parameter_attr.bit.cs_drtag)
                log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
            parameter_attr.bit.cs_drtag = TRUE;
        }
        else if (NAMETABLE_add_id("cs_rtag") == (yyvsp[(1) - (1)].y_id))
        {
            if (parameter_attr.bit.cs_rtag)
                log_warning(acf_yylineno, NIDL_MULATTRDEF, NULL);
            parameter_attr.bit.cs_rtag = TRUE;
        }
        else
            log_error(acf_yylineno, NIDL_ERRINATTR, NULL);
    ;}
    break;



/* Line 1455 of yacc.c  */
#line 2689 "acf_y.c"
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
#line 1155 "acf_y.y"


/***************************
 *  yacc programs section  *
 ***************************/

/*
 *  a c f _ i n i t
 *
 *  Function:   Called before ACF parsing to initialize variables.
 *
 */

void acf_init
#ifdef PROTO
(
    boolean     *cmd_opt_arr,   /* [in] Array of command option flags */
    void        **cmd_val_arr,  /* [in] Array of command option values */
    char        *acf_file       /* [in] ACF file name */
)
#else
(cmd_opt_arr, cmd_val_arr, acf_file)
    boolean     *cmd_opt_arr;   /* [in] Array of command option flags */
    void        **cmd_val_arr;  /* [in] Array of command option values */
    char        *acf_file;      /* [in] ACF file name */
#endif

{
    /* Save passed command array and interface node addrs in static storage. */
    cmd_opt = cmd_opt_arr;
    cmd_val = cmd_val_arr;

    /* Set global (STRTAB_str_t error_file_name_id) for error processing. */
    set_name_for_errors(acf_file);

    interface_attr.mask = 0;
    type_attr.mask      = 0;
    operation_attr.mask = 0;
    parameter_attr.mask = 0;

    interface_name      = NULL;
    type_name           = NULL;
    repr_type_name      = NULL;
    cs_char_type_name   = NULL;
    operation_name      = NULL;
    binding_callout_name= NULL;
    cs_tag_rtn_name     = NULL;

    include_list        = NULL;

    parameter_list      = NULL;
    parameter_free_list = NULL;
}

/*
 *  a c f _ c l e a n u p
 *
 *  Function:   Called after ACF parsing to free allocated memory.
 *
 */

#ifdef PROTO
void acf_cleanup(void)
#else
void acf_cleanup()
#endif

{
    acf_param_t *p, *q;     /* Ptrs to parameter record */

    p = parameter_free_list;

    while (p != NULL)
    {
        q = p;
        p = p->next;
        FREE(q);
    }
}




/*
**  l o o k u p _ e x c e p t i o n
**
**  Looks up a name in the nametable, and if it is bound to a valid exception
**  node, returns the address of the exception node.
**
**  Returns:    TRUE if lookup succeeds, FALSE otherwise.
*/

static boolean lookup_exception
(
    NAMETABLE_id_t  excep_id,     /* [in] Nametable id of exception name */
    boolean         do_log_error,    /* [in] TRUE => log error if name not found */
    AST_exception_n_t **excep_ptr /*[out] Ptr to AST exception node */
)
{
    AST_exception_n_t *excep_p;     /* Ptr to node bound to looked up name */
    char const      *perm_excep_name;   /* Ptr to permanent copy */
// Adam: Not used, delete    NAMETABLE_id_t  name_id ATTRIBUTE_UNUSED;            /* Handle on permanent copy */

    if (excep_id != NAMETABLE_NIL_ID)
    {
        excep_p = (AST_exception_n_t *)NAMETABLE_lookup_binding(excep_id);

        if (excep_p != NULL && excep_p->fe_info->node_kind == fe_exception_n_k)
        {
            *excep_ptr = excep_p;
            return TRUE;
        }
    }

    if (do_log_error)
    {
        NAMETABLE_id_to_string(excep_id, &perm_excep_name);
        acf_error(NIDL_EXCNOTDEF, perm_excep_name);
    }

    *excep_ptr = NULL;
    return FALSE;
}

/*
**  l o o k u p _ t y p e
**
**  Looks up a name in the nametable, and if it is bound to a valid type
**  node, returns the address of the type node.
**
**  Returns:    TRUE if lookup succeeds, FALSE otherwise.
*/

static boolean lookup_type
(
    char const      *type_name, /* [in] Name to look up */
    boolean         do_log_error,  /* [in] TRUE => log error if name not found */
    NAMETABLE_id_t  *type_id,   /*[out] Nametable id of type name */
    AST_type_n_t    **type_ptr  /*[out] Ptr to AST type node */
)
{
    AST_type_n_t    *type_p;    /* Ptr to node bound to looked up name */
    char const      *perm_type_name;    /* Ptr to permanent copy */
    NAMETABLE_id_t  name_id;            /* Handle on permanent copy */

    *type_id = NAMETABLE_lookup_id(type_name);

    if (*type_id != NAMETABLE_NIL_ID)
    {
        type_p = (AST_type_n_t *)NAMETABLE_lookup_binding(*type_id);

        if (type_p != NULL && type_p->fe_info->node_kind == fe_type_n_k)
        {
            *type_ptr = type_p;
            return TRUE;
        }
    }

    if (do_log_error)
    {
        name_id = NAMETABLE_add_id(type_name);
        NAMETABLE_id_to_string(name_id, &perm_type_name);
        acf_error(NIDL_TYPNOTDEF, perm_type_name);
    }

    *type_ptr = NULL;
    return FALSE;
}

/*
**  l o o k u p _ o p e r a t i o n
**
**  Looks up a name in the nametable, and if it is bound to a valid operation
**  node, returns the address of the operation node.
**
**  Returns:    TRUE if lookup succeeds, FALSE otherwise.
*/

static boolean lookup_operation
(
    char const      *op_name,   /* [in] Name to look up */
    boolean         do_log_error,  /* [in] TRUE => log error if name not found */
    NAMETABLE_id_t  *op_id,     /*[out] Nametable id of operation name */
    AST_operation_n_t **op_ptr  /*[out] Ptr to AST operation node */
)
{
    AST_operation_n_t   *op_p;  /* Ptr to node bound to looked up name */
    char const      *perm_op_name;      /* Ptr to permanent copy */
    NAMETABLE_id_t  name_id;            /* Handle on permanent copy */

    *op_id = NAMETABLE_lookup_id(op_name);

    if (*op_id != NAMETABLE_NIL_ID)
    {
        op_p = (AST_operation_n_t *)NAMETABLE_lookup_binding(*op_id);

        if (op_p != NULL && op_p->fe_info->node_kind == fe_operation_n_k)
        {
            *op_ptr = op_p;
            return TRUE;
        }
    }

    if (do_log_error)
    {
        name_id = NAMETABLE_add_id(op_name);
        NAMETABLE_id_to_string(name_id, &perm_op_name);
        acf_error(NIDL_OPNOTDEF, perm_op_name);
    }

    *op_ptr = NULL;
    return FALSE;
}

/*
**  l o o k u p _ p a r a m e t e r
**
**  Searches an operation node's parameter list for the parameter name passed.
**  If found, returns the address of the parameter node.
**
**  Returns:    TRUE if lookup succeeds, FALSE otherwise.
*/

static boolean lookup_parameter
(
    AST_operation_n_t   *op_p,          /* [in] Ptr to AST operation node */
    char const          *param_name,    /* [in] Parameter name to look up */
    boolean             do_log_error,      /* [in] TRUE=> log error if not found */
    NAMETABLE_id_t      *param_id,      /*[out] Nametable id of param name */
    AST_parameter_n_t   **param_ptr     /*[out] Ptr to AST parameter node */
)
{
    AST_parameter_n_t   *param_p;       /* Ptr to operation parameter node */
    char const          *op_param_name; /* Name of an operation parameter */
    char const          *op_name;       /* Operation name */
    char const      *perm_param_name;   /* Ptr to permanent copy */
    NAMETABLE_id_t  name_id;            /* Handle on permanent copy */

    for (param_p = op_p->parameters ; param_p != NULL ; param_p = param_p->next)
    {
        NAMETABLE_id_to_string(param_p->name, &op_param_name);

        if (strcmp(param_name, op_param_name) == 0)
        {
            *param_id   = param_p->name;
            *param_ptr  = param_p;
            return TRUE;
        }
    }

    if (do_log_error)
    {
        char const *file_name;     /* Related file name */

        NAMETABLE_id_to_string(op_p->name, &op_name);
        name_id = NAMETABLE_add_id(param_name);
        NAMETABLE_id_to_string(name_id, &perm_param_name);

        STRTAB_str_to_string(op_p->fe_info->file, &file_name);

        acf_error(NIDL_PRMNOTDEF, perm_param_name, op_name);
        acf_error(NIDL_NAMEDECLAT, op_name, file_name,
                  op_p->fe_info->source_line);
    }

    return FALSE;
}

/*
**  l o o k u p _ r e p _ a s _ n a m e
**
**  Scans a list of type nodes that have represent_as types for a match with
**  the type name given by the parameter repr_name_id.  If so, returns the
**  address of the found type node and a pointer to the associated
**  represent_as type name.
**
**  Returns:    TRUE if lookup succeeds, FALSE otherwise.
*/

static boolean lookup_rep_as_name
(
    AST_type_p_n_t  *typep_p,           /* [in] Listhead of type ptr nodes */
    NAMETABLE_id_t  repr_name_id,       /* [in] represent_as name to look up */
    AST_type_n_t    **ret_type_p,       /*[out] Type node if found */
    char const      **ret_type_name     /*[out] Type name if found */
)
{
    AST_type_n_t    *type_p;            /* Ptr to a type node */

    for ( ; typep_p != NULL ; typep_p = typep_p->next )
    {
        type_p = typep_p->type;
        if (type_p->name == repr_name_id)
        {
            *ret_type_p = type_p;
            NAMETABLE_id_to_string(type_p->rep_as_type->type_name,
                                   ret_type_name);
            return TRUE;
        }
    }

    return FALSE;
}

/*
**  l o o k u p _ c s _ c h a r _ n a m e
**
**  Scans a list of type nodes that have cs_char types for a match with
**  the type name given by the parameter cs_char_name_id.  If so, returns the
**  address of the found type node and a pointer to the associated
**  cs_char type name.
**
**  Returns:    TRUE if lookup succeeds, FALSE otherwise.
*/

static boolean lookup_cs_char_name
(
    AST_type_p_n_t  *typep_p,           /* [in] Listhead of type ptr nodes */
    NAMETABLE_id_t  cs_char_name_id,    /* [in] cs_char name to look up */
    AST_type_n_t    **ret_type_p,       /*[out] Type node if found */
    char const      **ret_type_name     /*[out] Type name if found */
)
{
    AST_type_n_t    *type_p;            /* Ptr to a type node */

    for ( ; typep_p != NULL ; typep_p = typep_p->next )
    {
        type_p = typep_p->type;
        if (type_p->name == cs_char_name_id)
        {
            *ret_type_p = type_p;
            NAMETABLE_id_to_string(type_p->cs_char_type->type_name,
                                   ret_type_name);
            return TRUE;
        }
    }

    return FALSE;
}

/*
 *  a c f _ a l l o c _ p a r a m
 *
 *  Function:   Allocates an acf_param_t, either from the free list or heap.
 *
 *  Returns:    Address of acf_param_t
 *
 *  Globals:    parameter_free_list - listhead for free list
 *
 *  Side Effects:   Exits program if unable to allocate memory.
 */

#ifdef PROTO
static acf_param_t *alloc_param(void)
#else
static acf_param_t *alloc_param()
#endif

{
    acf_param_t *p;     /* Ptr to parameter record */

    if (parameter_free_list != NULL)
    {
        p = parameter_free_list;
        parameter_free_list = parameter_free_list->next;
    }
    else
    {
        p = NEW (acf_param_t);
        p->next                 = NULL;
        p->parameter_attr.mask  = 0;
        p->param_id             = NAMETABLE_NIL_ID;
    }

    return p;
}

/*
 *  a c f _ f r e e _ p a r a m
 *
 *  Function:   Frees an acf_param_t by reinitilizing it and returning it to
 *              the head of the free list.
 *
 *  Input:      p - Pointer to acf_param_t record
 *
 *  Globals:    parameter_free_list - listhead for free list
 */

static void free_param
#ifdef PROTO
(
    acf_param_t *p              /* [in] Pointer to acf_param_t record */
)
#else
(p)
    acf_param_t *p;             /* [in] Pointer to acf_param_t record */
#endif

{
    p->parameter_attr.mask  = 0;
    p->param_id             = NAMETABLE_NIL_ID;

    p->next                 = parameter_free_list;
    parameter_free_list     = p;
}


/*
 *  a c f _ f r e e _ p a r a m _ l i s t
 *
 *  Function:   Frees a list of acf_param_t records.
 *
 *  Input:      list - Address of list pointer
 *
 *  Output:     list pointer = NULL
 */

static void free_param_list
#ifdef PROTO
(
    acf_param_t **list          /* [in] Address of list pointer */
)
#else
(list)
    acf_param_t **list;         /* [in] Address of list pointer */
#endif

{
    acf_param_t *p, *q;     /* Ptrs to parameter record */

    p = *list;

    while (p != NULL)
    {
        q = p;
        p = p->next;
        free_param(q);
    }

    *list = NULL;            /* List now empty */
}

/*
 *  a d d _ p a r a m _ t o _ l i s t
 *
 *  Function:   Add a acf_param_t record to the end of a list.
 *
 *  Inputs:     p - Pointer to parameter record
 *              list - Address of list pointer
 *
 *  Outputs:    List is modified.
 */

void add_param_to_list
#ifdef PROTO
(
    acf_param_t *p,             /* [in] Pointer to parameter record */
    acf_param_t **list          /* [in] Address of list pointer */
)
#else
(p, list)
    acf_param_t *p;             /* [in] Pointer to parameter record */
    acf_param_t **list;         /* [in] Address of list pointer */
#endif

{
    acf_param_t *q;         /* Ptr to parameter record */

    if (*list == NULL)      /* If list empty */
        *list = p;          /* then list now points at param */
    else
    {
        for (q = *list ; q->next != NULL ; q = q->next)
            ;
        q->next = p;        /* else last record in list now points at param */
    }

    p->next = NULL;         /* Param is now last in list */
}

/*
**  a p p e n d _ p a r a m e t e r
**
**  Appends a parameter to an operation's parameter list.
*/

static void append_parameter
(
    AST_operation_n_t   *op_p,          /* [in] Ptr to AST operation node */
    char const          *param_name,    /* [in] Parameter name */
    acf_attrib_t        *param_attr     /* [in] Parameter attributes */
)
{
    NAMETABLE_id_t      new_param_id;   /* Nametable id of new parameter name */
    AST_parameter_n_t   *new_param_p;   /* Ptr to new parameter node */
    AST_type_n_t        *new_type_p;    /* Ptr to new parameter type node */
    AST_pointer_n_t     *new_ptr_p;     /* Ptr to new pointer node */
    NAMETABLE_id_t      status_id;      /* Nametable id of status_t */
    AST_type_n_t        *status_type_p; /* Type node bound to status_t name */
    AST_parameter_n_t   *param_p;       /* Ptr to operation parameter node */

    /* Look up error_status_t type. */
    status_id = NAMETABLE_add_id("error_status_t");
    status_type_p = (AST_type_n_t *)NAMETABLE_lookup_binding(status_id);
    if (status_type_p == NULL)
    {
        acf_error(NIDL_ERRSTATDEF, "error_status_t", "nbase.idl");
        return;
    }

    /*
     * Have to create an '[out] error_status_t *param_name' parameter
     * that has the specified parameter attributes.
     */
    new_param_id = NAMETABLE_add_id(param_name);
    new_param_p = AST_parameter_node(new_param_id);
    new_type_p  = AST_type_node(AST_pointer_k);
    new_ptr_p   = AST_pointer_node(status_type_p);

    new_type_p->type_structure.pointer = new_ptr_p;
    AST_SET_REF(new_type_p);

    new_param_p->name = new_param_id;
    new_param_p->type = new_type_p;
    new_param_p->uplink = op_p;
    if (param_attr->bit.comm_status)
        AST_SET_ADD_COMM_STATUS(new_param_p);
    if (param_attr->bit.fault_status)
        AST_SET_ADD_FAULT_STATUS(new_param_p);
    AST_SET_OUT(new_param_p);
    AST_SET_REF(new_param_p);

    param_p = op_p->parameters;
    if (param_p == NULL)
    {
        /* Was null param list, now has one param. */
        op_p->parameters = new_param_p;
    }
    else if (param_p->last == NULL)
    {
        /* Was one param, now have two params. */
        param_p->next = new_param_p;
        param_p->last = new_param_p;
    }
    else
    {
        /* Was more than one param, now have one more. */
        param_p->last->next = new_param_p;
        param_p->last = new_param_p;
    }
}

/*
**  p r o c e s s _ r e p _ a s _ t y p e
**
**  Processes a [represent_as] clause applied to a type.  Validates that
**  [represent_as] types are not nested.  Adds the type to a list of types
**  that have the [represent_as] attribute.
*/

static void process_rep_as_type
(
    AST_interface_n_t   *int_p,     /* [in] Ptr to AST interface node */
    AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
    char const      *ref_type_name  /* [in] Name in represent_as() clause */
)
{
    NAMETABLE_id_t  ref_type_id;    /* Nametable id of referenced name */
    char const      *file_name;     /* Related file name */
    char const      *perm_name;     /* Permanent copy of referenced name */
    AST_type_n_t    *parent_type_p; /* Parent type with same attribute */
    char const      *parent_name;   /* Name of parent type */

    ref_type_id = NAMETABLE_add_id(ref_type_name);

    /*
     * Report error if the type name referenced in the attribute is an AST
     * type which also has the same attribute, i.e. types with this attribute
     * cannot nest.
     */
    if (lookup_rep_as_name(int_p->ra_types, ref_type_id, &parent_type_p,
                           &perm_name))
    {
        NAMETABLE_id_to_string(parent_type_p->name, &parent_name);
        STRTAB_str_to_string(parent_type_p->fe_info->acf_file, &file_name);

        acf_error(NIDL_REPASNEST);
        acf_error(NIDL_TYPEREPAS, parent_name, perm_name);
        acf_error(NIDL_NAMEDECLAT, parent_name, file_name,
                  parent_type_p->fe_info->acf_source_line);
    }

    /*
     * If the type node already has a type name for this attribute,
     * this one must duplicate that same name.
     */
    if (type_p->rep_as_type != NULL)
    {
        NAMETABLE_id_to_string(type_p->rep_as_type->type_name, &perm_name);

        if (strcmp(perm_name, ref_type_name) != 0)
        {
            char const *new_ref_type_name; /* Ptr to permanent copy */
            NAMETABLE_id_t  name_id;       /* Handle on perm copy */

            name_id = NAMETABLE_add_id(ref_type_name);
            NAMETABLE_id_to_string(name_id, &new_ref_type_name);

            STRTAB_str_to_string(
                            type_p->rep_as_type->fe_info->acf_file, &file_name);

            acf_error(NIDL_CONFREPRTYPE, new_ref_type_name, perm_name);
            acf_error(NIDL_NAMEDECLAT, perm_name, file_name,
                      type_p->rep_as_type->fe_info->acf_source_line);
        }
    }
    else
    {
        /*
         * Process valid [represent_as] clause.
         */
        AST_type_p_n_t  *typep_p;       /* Used to link type nodes */
        AST_rep_as_n_t  *repas_p;       /* Ptr to represent_as node */

        /* Add represent_as type name and build rep_as AST node. */

        repas_p = type_p->rep_as_type = AST_represent_as_node(ref_type_id);
        /* Store source information. */
        if (repas_p->fe_info != NULL)
        {
            repas_p->fe_info->acf_file = error_file_name_id;
            repas_p->fe_info->acf_source_line = acf_yylineno;
        }

        /* Check for associated def-as-tag node. */

        if (type_p->fe_info->tag_ptr != NULL)
            type_p->fe_info->tag_ptr->rep_as_type = type_p->rep_as_type;

        /* Link type node into list of represent_as types. */

        typep_p = AST_type_ptr_node();
        typep_p->type = type_p;

        int_p->ra_types = (AST_type_p_n_t *)AST_concat_element(
                                                (ASTP_node_t *)int_p->ra_types,
                                                (ASTP_node_t *)typep_p);
    }
}

/*
**  p r o c e s s _ c s _ c h a r _ t y p e
**
**  Processes a [cs_char] clause applied to a type.  Validates that
**  [cs_char] types are not nested.  Adds the type to a list of types
**  that have the [cs_char] attribute.
*/

static void process_cs_char_type
(
    AST_interface_n_t   *int_p,     /* [in] Ptr to AST interface node */
    AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
    char const      *ref_type_name  /* [in] Name in cs_char() clause */
)
{
    NAMETABLE_id_t  ref_type_id;    /* Nametable id of referenced name */
    char const      *file_name;     /* Related file name */
    char const      *perm_name;     /* Permanent copy of referenced name */
    AST_type_n_t    *parent_type_p; /* Parent type with same attribute */
    char const      *parent_name;   /* Name of parent type */

    ref_type_id = NAMETABLE_add_id(ref_type_name);

    /*
     * Report error if the type name referenced in the attribute is an AST
     * type which also has the same attribute, i.e. types with this attribute
     * cannot nest.
     */
    if (lookup_cs_char_name(int_p->cs_types, ref_type_id, &parent_type_p,
                            &perm_name))
    {
        NAMETABLE_id_to_string(parent_type_p->name, &parent_name);
        STRTAB_str_to_string(parent_type_p->fe_info->acf_file, &file_name);

        /*** This needs updating ***/
        acf_error(NIDL_REPASNEST);
        acf_error(NIDL_TYPEREPAS, parent_name, perm_name);
        acf_error(NIDL_NAMEDECLAT, parent_name, file_name,
                  parent_type_p->fe_info->acf_source_line);
    }

    /*
     * If the type node already has a type name for this attribute,
     * this one must duplicate that same name.
     */
    if (type_p->cs_char_type != NULL)
    {
        NAMETABLE_id_to_string(type_p->cs_char_type->type_name, &perm_name);

        if (strcmp(perm_name, ref_type_name) != 0)
        {
            char const *new_ref_type_name; /* Ptr to permanent copy */
            NAMETABLE_id_t  name_id;    /* Handle on perm copy */

            name_id = NAMETABLE_add_id(ref_type_name);
            NAMETABLE_id_to_string(name_id, &new_ref_type_name);

            STRTAB_str_to_string(
                        type_p->cs_char_type->fe_info->acf_file, &file_name);

            /*** This needs updating ***/
            acf_error(NIDL_CONFREPRTYPE, new_ref_type_name, perm_name);
            acf_error(NIDL_NAMEDECLAT, perm_name, file_name,
                      type_p->cs_char_type->fe_info->acf_source_line);
        }
    }
    else
    {
        /*
         * Process valid [cs_char] clause.
         */
        AST_type_p_n_t  *typep_p;       /* Used to link type nodes */
        AST_cs_char_n_t *cschar_p;      /* Ptr to cs_char node */

        /* Add cs_char type name and build cs_char AST node. */

        cschar_p = type_p->cs_char_type = AST_cs_char_node(ref_type_id);
        /* Store source information. */
        if (cschar_p->fe_info != NULL)
        {
            cschar_p->fe_info->acf_file = error_file_name_id;
            cschar_p->fe_info->acf_source_line = acf_yylineno;
        }

        /* Check for associated def-as-tag node. */

        if (type_p->fe_info->tag_ptr != NULL)
            type_p->fe_info->tag_ptr->cs_char_type = type_p->cs_char_type;

        /* Link type node into list of cs_char types. */

        typep_p = AST_type_ptr_node();
        typep_p->type = type_p;

        int_p->cs_types = (AST_type_p_n_t *)AST_concat_element(
                                                (ASTP_node_t *)int_p->cs_types,
                                                (ASTP_node_t *)typep_p);
    }
}

#ifdef DUMPERS
/*
 *  d u m p _ a t t r i b u t e s
 *
 *  Function:   Prints list of attributes parsed for a particular node type
 *
 *  Inputs:     header_text - Initial text before node name and attributes
 *              node_name   - Name of interface, type, operation, or parameter
 *              node_attr_p - Address of node attributes structure
 *
 *  Globals:    repr_type_name  - represent_as type name, used if bit is set
 *              cs_char_type_name - cs_char type name, used if bit is set
 *              cs_tag_rtn_name - cs_tag_rtn name, used if bit is set
 *              binding_callout_name - binding_callout name, used if bit is set
 */

static void dump_attributes
#ifdef PROTO
(
    char            *header_text,       /* [in] Initial output text */
    char const            *node_name,         /* [in] Name of tree node */
    acf_attrib_t    *node_attr_p        /* [in] Node attributes ptr */
)
#else
(header_text, node_name, node_attr_p)
    char            *header_text;       /* [in] Initial output text */
    char            *node_name;         /* [in] Name of tree node */
    acf_attrib_t    *node_attr_p;       /* [in] Node attributes ptr */
#endif

#define MAX_ATTR_TEXT   1024    /* Big enough for lots of extern_exceptions */

{
    char            attr_text[MAX_ATTR_TEXT];   /* Buf for formatting attrs */
    int             pos;                /* Position in buffer */
    acf_attrib_t    node_attr;          /* Node attributes */

    node_attr = *node_attr_p;

    printf("%s %s", header_text, node_name);

    if (node_attr.mask == 0)
        printf("\n");
    else
    {
        printf(" attributes: ");
        strcpy(attr_text, "[");

        if (node_attr.bit.auto_handle)
            strcat(attr_text, "auto_handle, ");
        if (node_attr.bit.code)
            strcat(attr_text, "code, ");
        if (node_attr.bit.nocode)
            strcat(attr_text, "nocode, ");
        if (node_attr.bit.comm_status)
            strcat(attr_text, "comm_status, ");
        if (node_attr.bit.decode)
            strcat(attr_text, "decode, ");
        if (node_attr.bit.enable_allocate)
            strcat(attr_text, "enable_allocate, ");
        if (node_attr.bit.encode)
            strcat(attr_text, "encode, ");
        if (node_attr.bit.explicit_handle)
            strcat(attr_text, "explicit_handle, ");
        if (node_attr.bit.nocancel)
            strcat(attr_text, "nocancel, ");
        if (node_attr.bit.extern_exceps && ASTP_parsing_main_idl)
        {
            AST_exception_n_t   *excep_p;
            char const               *name;
            strcat(attr_text, "extern_exceptions(");
            for (excep_p = the_interface->exceptions;
                 excep_p != NULL;
                 excep_p = excep_p->next)
            {
                if (AST_EXTERN_SET(excep_p))
                {
                    NAMETABLE_id_to_string(excep_p->name, &name);
                    strcat(attr_text, name);
                    strcat(attr_text, ",");
                }
            }
            attr_text[strlen(attr_text)-1] = '\0';  /* overwrite trailing ',' */
            strcat(attr_text, "), ");
        }
        if (node_attr.bit.fault_status)
            strcat(attr_text, "fault_status, ");
        if (node_attr.bit.heap)
            strcat(attr_text, "heap, ");
        if (node_attr.bit.implicit_handle)
            strcat(attr_text, "implicit_handle, ");
        if (node_attr.bit.in_line)
            strcat(attr_text, "in_line, ");
        if (node_attr.bit.out_of_line)
            strcat(attr_text, "out_of_line, ");
        if (node_attr.bit.cs_stag)
            strcat(attr_text, "cs_stag, ");
        if (node_attr.bit.cs_drtag)
            strcat(attr_text, "cs_drtag, ");
        if (node_attr.bit.cs_rtag)
            strcat(attr_text, "cs_rtag, ");
        if (node_attr.bit.represent_as)
        {
            strcat(attr_text, "represent_as(");
            strcat(attr_text, repr_type_name);
            strcat(attr_text, "), ");
        }
        if (node_attr.bit.cs_char)
        {
            strcat(attr_text, "cs_char(");
            strcat(attr_text, cs_char_type_name);
            strcat(attr_text, "), ");
        }
        if (node_attr.bit.cs_tag_rtn)
        {
            strcat(attr_text, "cs_tag_rtn(");
            strcat(attr_text, cs_tag_rtn_name);
            strcat(attr_text, "), ");
        }
        if (node_attr.bit.binding_callout)
        {
            strcat(attr_text, "binding_callout(");
            strcat(attr_text, binding_callout_name);
            strcat(attr_text, "), ");
        }


        /* Overwrite trailing ", " with "]" */

        pos = strlen(attr_text) - strlen(", ");
        attr_text[pos] = ']';
        attr_text[pos+1] = '\0';

        printf("%s\n", attr_text);
    }
}
#endif

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

struct acf_bisonparser_state 
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


typedef struct acf_bisonparser_state acf_bisonparser_activation_record;
 
/*****************************************************************
 *
 * Basic constructors/destructors for FLEX activation states
 *
 *****************************************************************/
 
void *
new_acf_bisonparser_activation_record()
  {
    return (malloc(sizeof(acf_bisonparser_activation_record)));
  }
 
void
delete_acf_bisonparser_activation_record(void * p)
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
get_current_acf_bisonparser_activation()
  {
    acf_bisonparser_activation_record * p;

    p = (acf_bisonparser_activation_record * )
                new_acf_bisonparser_activation_record();

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
set_current_acf_bisonparser_activation(void * ptr)
  {

    acf_bisonparser_activation_record * p =
      (acf_bisonparser_activation_record *)ptr;

    /* restore the statics */


     yychar = p->yychar;
     yynerrs = p->yynerrs;
     yylval = p->yylval;


  }

void
init_new_acf_bisonparser_activation()
  {
    /* set some initial conditions for a new Bison parser state */

    yynerrs = 0;

  }

/* preserve coding style vim: set tw=78 sw=4 : */

