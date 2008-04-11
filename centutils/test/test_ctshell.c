#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctshell.h>

#include <moonunit/interface.h>

#define MU_TRY_CTERR(_e_)					\
    do {							\
	CENTERROR ceError = (_e_);				\
	if (ceError)						\
	{							\
	    const char* name = CTErrorName(ceError);		\
	    if (!name)						\
		name = "Unknown";				\
	    MU_FAILURE("Expression '%s' failed: %s (%x)",	\
		       #_e_, name, (unsigned int) ceError);	\
	}						        \
    } while (0)

#define MU_TRY_EXPECT_CTERR(_e_, _err_)			\
    do {						\
        CENTERROR ceError = (_e_);			\
	CENTERROR expected = (_err_);			\
	if (ceError != expected)			\
	{						\
	    const char* gotName = CTErrorName(ceError);	\
	    const char* expectName = CTErrorName(expected); \
	    if (!gotName) gotName = "Unknown";		    \
	    if (!expectName) expectName = "Unknown";	    \
	    MU_FAILURE("Expression '%s': expected %s, got %s",	\
		       #_e_, expectName, gotName);		\
	}							\
    } while (0)							\

MU_TEST(CTShell, command_failed)
{
    MU_TRY_EXPECT_CTERR(
	CTShell("false"),
	CENTERROR_COMMAND_FAILED);
}

MU_TEST(CTShell, capture)
{
    PSTR out;
    MU_TRY_CTERR(
	CTShell("echo hello >%out",
		CTSHELL_BUFFER(out, &out)));
    CTStripWhitespace(out);

    MU_ASSERT_EQUAL(MU_STRING, out, "hello");
}

MU_TEST(CTShell, capture2)
{
    PSTR out, out2;
    MU_TRY_CTERR(
	CTShell("echo hello >%out; echo goodbye >%out2",
		CTSHELL_BUFFER(out, &out),
		CTSHELL_BUFFER(out2, &out2)));
    CTStripWhitespace(out);
    CTStripWhitespace(out2);

    MU_ASSERT_EQUAL(MU_STRING, out, "hello");
    MU_ASSERT_EQUAL(MU_STRING, out2, "goodbye");
}

MU_TEST(CTShell, capture_env)
{
    static const char* penv[] =
    {
	"FOO=zamboni",
	NULL
    };

    PSTR out;

    MU_TRY_CTERR(
	CTShellEx(penv, "echo $FOO >%out",
		  CTSHELL_BUFFER(out, &out)));
    CTStripWhitespace(out);

    MU_ASSERT_EQUAL(MU_STRING, out, "zamboni");
}

MU_TEST(CTShell, escape)
{
    char in[2] = {0,0};
    unsigned int c, o;
    PSTR out = NULL;

    for (c = 1; c <= 255; c++)
    {
	in[0] = (char) c;
	
	MU_TRY_CTERR(
	    CTShell("printf '%s' %in >%out",
		    CTSHELL_STRING(in, in),
		    CTSHELL_BUFFER(out, &out)));

	o = (unsigned int) (unsigned char) out[0];
	MU_ASSERT_EQUAL(MU_INTEGER, c, o);
	CT_SAFE_FREE_STRING(out);
    }
}

MU_TEST(CTShell, double)
{
    PSTR out = NULL;

    MU_TRY_CTERR(
	CTShell("printf \"a %in b\" >%out",
		CTSHELL_STRING(in, "hi \" there"),
		CTSHELL_BUFFER(out, &out)));
    MU_ASSERT_EQUAL(MU_STRING, out, "a hi \" there b");
}

MU_TEST(CTShell, zero)
{
    PSTR out = NULL;

    MU_TRY_CTERR(
	CTShell("printf %zero '%s' 'hello' >%out",
		CTSHELL_ZERO(zero),
		CTSHELL_BUFFER(out, &out)));
    
    MU_ASSERT_EQUAL(MU_STRING, out, "hello");
}
