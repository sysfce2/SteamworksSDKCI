//============ Copyright (c) Valve Corporation, All rights reserved. ============
//
// glmgrbasics.cpp
//
//===============================================================================

#include "glmgrbasics.h"
#include "dxabstract.h"

#if __MAC_OS_X_VERSION_MAX_ALLOWED <= __MAC_10_6
#include <OpenGL/CGLProfiler.h>
#include <OpenGL/CGLProfilerFunctionEnum.h>
#else
#define kCGLCPComment ((CGLContextParameter)1232)
/* param is a pointer to a NULL-terminated C-style string. */
/* Inserts a context-specific comment into the function trace stream. */
/* Availability: set only, get is ignored. */

#define kCGLCPDumpState ((CGLContextParameter)1233)
/* param ignored.  Dumps all the gl state. */
/* Availability: set only, get is ignored. */

#define kCGLCPEnableForceFlush ((CGLContextParameter)1234)
/* param is GL_TRUE to enable "force flush" mode or GL_FALSE to disable. */
/* Availability: set and get. */

#define kCGLGOComment ((CGLGlobalOption)1506)
/* param is a pointer to a NULL-terminated C-style string. */
/* Inserts a comment in the trace steam that applies to all contexts. */
/* Availability: set only, get is ignored. */

#define kCGLGOEnableFunctionTrace ((CGLGlobalOption)1507)
/* param is GL_TRUE or GL_FALSE */
/* Turns GL function call tracing on and off */
/* Availability: set and get */

#define kCGLGOResetFunctionTrace ((CGLGlobalOption)1509)
/* param is ignored */
/* Erases current function trace and starts a new one */
/* Availability: set only, get is ignored. */

#define kCGLGOEnableBreakpoint ((CGLGlobalOption)1514)
/* param is an array of 3 GLints:
 param[0] is function ID (see CGLProfilerFunctionEnum.h)
 param[1] is the logical OR of kCGLProfBreakBefore or kCGLProfBreakAfter, indicating how
 you want the breakpoint to stop: before entering OpenGL, on return from OpenGL, or both.
 param[2] is a boolean which turns the breakpoint on or off.
 */
/* Availability: set and get. */

#define kCGLProfBreakBefore  0x0001
#define kCGLProfBreakAfter   0x0002

#define kCGLFEglColor4sv 98

#endif

//===============================================================================
#define TOLOWERC( x )  (( ( x >= 'A' ) && ( x <= 'Z' ) )?( x + 32 ) : x )
int	V_stricmp(const char *s1, const char *s2 )
{
	uint8 const *pS1 = ( uint8 const * ) s1;
	uint8 const *pS2 = ( uint8 const * ) s2;
	for(;;)
	{
		int c1 = *( pS1++ );
		int c2 = *( pS2++ );
		if ( c1 == c2 )
		{
			if ( !c1 ) return 0;
		}
		else
		{
			if ( ! c2 )
			{
				return c1 - c2;
			}
			c1 = TOLOWERC( c1 );
			c2 = TOLOWERC( c2 );
			if ( c1 != c2 )
			{
				return c1 - c2;
			}
		}
		c1 = *( pS1++ );
		c2 = *( pS2++ );
		if ( c1 == c2 )
		{
			if ( !c1 ) return 0;
		}
		else
		{
			if ( ! c2 )
			{
				return c1 - c2;
			}
			c1 = TOLOWERC( c1 );
			c2 = TOLOWERC( c2 );
			if ( c1 != c2 )
			{
				return c1 - c2;
			}
		}
	}
}

inline unsigned char tolower_fast(unsigned char c)
{
	if ( (c >= 'A') && (c <= 'Z') )
		return c + ('a' - 'A');
	return c;
}

//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test
//-----------------------------------------------------------------------------
char const* V_stristr( char const* pStr, char const* pSearch )
{
	//AssertValidStringPtr(pStr);
	//AssertValidStringPtr(pSearch);

	if (!pStr || !pSearch) 
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower_fast((unsigned char)*pLetter) == tolower_fast((unsigned char)*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower_fast((unsigned char)*pMatch) != tolower_fast((unsigned char)*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

char* V_stristr( char* pStr, char const* pSearch )
{
	//AssertValidStringPtr( pStr );
	//AssertValidStringPtr( pSearch );

	return (char*)V_stristr( (char const*)pStr, pSearch );
}

//===============================================================================

// convars for GLM

//ConVar gl_errorcheckall		( "gl_errorcheckall", "0" );
//ConVar gl_errorcheckqueries	( "gl_errorcheckqueries", "0" );

int gl_errorcheckall = 0;
int gl_errorcheckqueries = 0;

// this one overrides the other two.
// i.e. you can set this one true, and no errors will be checked, period.
//ConVar gl_errorchecknone	( "gl_errorchecknone", "0" );
int gl_errorchecknone = 0;

// this decides whether the engine will try to use fast context mode on 10.6.3 or later.
// fast context mode means that a single GL context is used both for the window and the engine, saving on sync and flushes.
// it's only a suggestion; if the OS is below 10.6.2 it will be ignored.
//ConVar gl_singlecontext		( "gl_singlecontext", "1" );
int gl_singlecontext = 1;

//===============================================================================
// decoding tables for debug

typedef struct
{
	unsigned long	value;
	const char		*name;
}	GLMValueEntry_t;

#define	TERMVALUE	0x31415926
	// terminator for value tables
	
#define	VE( x )		{ x, #x }
	// "value entry"

GLMValueEntry_t	g_d3d_devtypes[] = 
{
	VE( D3DDEVTYPE_HAL ),
	VE( D3DDEVTYPE_REF ),

	VE( TERMVALUE )
};

GLMValueEntry_t g_d3d_formats[] = 
{
	VE( D3DFMT_INDEX16 ),
	VE( D3DFMT_D16 ),
	VE( D3DFMT_D24S8 ),
	VE( D3DFMT_A8R8G8B8 ),
	VE( D3DFMT_A4R4G4B4 ),
	VE( D3DFMT_X8R8G8B8 ),
	VE( D3DFMT_R5G6R5 ),
	VE( D3DFMT_X1R5G5B5 ),
	VE( D3DFMT_A1R5G5B5 ),
	VE( D3DFMT_L8 ),
	VE( D3DFMT_A8L8 ),
	VE( D3DFMT_A ),
	VE( D3DFMT_DXT1 ),
	VE( D3DFMT_DXT3 ),
	VE( D3DFMT_DXT5 ),
	VE( D3DFMT_V8U8 ),
	VE( D3DFMT_Q8W8V8U8 ),
	VE( D3DFMT_X8L8V8U8 ),
	VE( D3DFMT_A16B16G16R16F ),
	VE( D3DFMT_A16B16G16R16 ),
	VE( D3DFMT_R32F ),
	VE( D3DFMT_A32B32G32R32F ),
	VE( D3DFMT_R8G8B8 ),
	VE( D3DFMT_D24X4S4 ),
	VE( D3DFMT_A8 ),
	VE( D3DFMT_R5G6B5 ),
	VE( D3DFMT_D15S1 ),
	VE( D3DFMT_D24X8 ),
	VE( D3DFMT_VERTEXDATA ),
	VE( D3DFMT_INDEX32 ),

	// vendor specific formats (fourcc's)
	VE( D3DFMT_NV_INTZ ),
	VE( D3DFMT_NV_RAWZ ),
	VE( D3DFMT_NV_NULL ),
	VE( D3DFMT_ATI_D16 ),
	VE( D3DFMT_ATI_D24S8 ),
	VE( D3DFMT_ATI_2N ),
	VE( D3DFMT_ATI_1N ),

	VE( D3DFMT_UNKNOWN ),

	VE( TERMVALUE )
};

GLMValueEntry_t	g_d3d_rtypes[] =
{
	VE( D3DRTYPE_SURFACE ),
	VE( D3DRTYPE_TEXTURE ),
	VE( D3DRTYPE_VOLUMETEXTURE ),
	VE( D3DRTYPE_CUBETEXTURE ),
	VE( D3DRTYPE_VERTEXBUFFER ),
	VE( D3DRTYPE_INDEXBUFFER ),

	VE( TERMVALUE )
};

GLMValueEntry_t g_d3d_usages[] = 
{
	VE( D3DUSAGE_RENDERTARGET ),
	VE( D3DUSAGE_DEPTHSTENCIL ),
	VE( D3DUSAGE_DYNAMIC ),
	VE( D3DUSAGE_AUTOGENMIPMAP ),
	//VE( D3DUSAGE_DMAP ),
	//VE( D3DUSAGE_QUERY_LEGACYBUMPMAP ),
	VE( D3DUSAGE_QUERY_SRGBREAD ),
	VE( D3DUSAGE_QUERY_FILTER ),
	VE( D3DUSAGE_QUERY_SRGBWRITE ),
	VE( D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING ),
	VE( D3DUSAGE_QUERY_VERTEXTEXTURE ),
	//VE( D3DUSAGE_QUERY_WRAPANDMIP ),
	VE( D3DUSAGE_WRITEONLY ),
	VE( D3DUSAGE_SOFTWAREPROCESSING ),
	VE( D3DUSAGE_DONOTCLIP ),
	VE( D3DUSAGE_POINTS ),
	VE( D3DUSAGE_RTPATCHES ),
	VE( D3DUSAGE_NPATCHES ),

	VE( TERMVALUE )
};

GLMValueEntry_t g_d3d_rstates[] = 
{
	VE( D3DRS_ZENABLE ),
	VE( D3DRS_FILLMODE ),
	VE( D3DRS_SHADEMODE ),
	VE( D3DRS_ZWRITEENABLE ),
	VE( D3DRS_ALPHATESTENABLE ),
	VE( D3DRS_LASTPIXEL ),
	VE( D3DRS_SRCBLEND ),
	VE( D3DRS_DESTBLEND ),
	VE( D3DRS_CULLMODE ),
	VE( D3DRS_ZFUNC ),
	VE( D3DRS_ALPHAREF ),
	VE( D3DRS_ALPHAFUNC ),
	VE( D3DRS_DITHERENABLE ),
	VE( D3DRS_ALPHABLENDENABLE ),
	VE( D3DRS_FOGENABLE ),
	VE( D3DRS_SPECULARENABLE ),
	VE( D3DRS_FOGCOLOR ),
	VE( D3DRS_FOGTABLEMODE ),
	VE( D3DRS_FOGSTART ),
	VE( D3DRS_FOGEND ),
	VE( D3DRS_FOGDENSITY ),
	VE( D3DRS_RANGEFOGENABLE ),
	VE( D3DRS_STENCILENABLE ),
	VE( D3DRS_STENCILFAIL ),
	VE( D3DRS_STENCILZFAIL ),
	VE( D3DRS_STENCILPASS ),
	VE( D3DRS_STENCILFUNC ),
	VE( D3DRS_STENCILREF ),
	VE( D3DRS_STENCILMASK ),
	VE( D3DRS_STENCILWRITEMASK ),
	VE( D3DRS_TEXTUREFACTOR ),
	VE( D3DRS_WRAP0 ),
	VE( D3DRS_WRAP1 ),
	VE( D3DRS_WRAP2 ),
	VE( D3DRS_WRAP3 ),
	VE( D3DRS_WRAP4 ),
	VE( D3DRS_WRAP5 ),
	VE( D3DRS_WRAP6 ),
	VE( D3DRS_WRAP7 ),
	VE( D3DRS_CLIPPING ),
	VE( D3DRS_LIGHTING ),
	VE( D3DRS_AMBIENT ),
	VE( D3DRS_FOGVERTEXMODE ),
	VE( D3DRS_COLORVERTEX ),
	VE( D3DRS_LOCALVIEWER ),
	VE( D3DRS_NORMALIZENORMALS ),
	VE( D3DRS_DIFFUSEMATERIALSOURCE ),
	VE( D3DRS_SPECULARMATERIALSOURCE ),
	VE( D3DRS_AMBIENTMATERIALSOURCE ),
	VE( D3DRS_EMISSIVEMATERIALSOURCE ),
	VE( D3DRS_VERTEXBLEND ),
	VE( D3DRS_CLIPPLANEENABLE ),
	VE( D3DRS_POINTSIZE ),
	VE( D3DRS_POINTSIZE_MIN ),
	VE( D3DRS_POINTSPRITEENABLE ),
	VE( D3DRS_POINTSCALEENABLE ),
	VE( D3DRS_POINTSCALE_A ),
	VE( D3DRS_POINTSCALE_B ),
	VE( D3DRS_POINTSCALE_C ),
	VE( D3DRS_MULTISAMPLEANTIALIAS ),
	VE( D3DRS_MULTISAMPLEMASK ),
	VE( D3DRS_PATCHEDGESTYLE ),
	VE( D3DRS_DEBUGMONITORTOKEN ),
	VE( D3DRS_POINTSIZE_MAX ),
	VE( D3DRS_INDEXEDVERTEXBLENDENABLE ),
	VE( D3DRS_COLORWRITEENABLE ),
	VE( D3DRS_TWEENFACTOR ),
	VE( D3DRS_BLENDOP ),
	VE( D3DRS_POSITIONDEGREE ),
	VE( D3DRS_NORMALDEGREE ),
	VE( D3DRS_SCISSORTESTENABLE ),
	VE( D3DRS_SLOPESCALEDEPTHBIAS ),
	VE( D3DRS_ANTIALIASEDLINEENABLE ),
	VE( D3DRS_MINTESSELLATIONLEVEL ),
	VE( D3DRS_MAXTESSELLATIONLEVEL ),
	VE( D3DRS_ADAPTIVETESS_X ),
	VE( D3DRS_ADAPTIVETESS_Y ),
	VE( D3DRS_ADAPTIVETESS_Z ),
	VE( D3DRS_ADAPTIVETESS_W ),
	VE( D3DRS_ENABLEADAPTIVETESSELLATION ),
	VE( D3DRS_TWOSIDEDSTENCILMODE ),
	VE( D3DRS_CCW_STENCILFAIL ),
	VE( D3DRS_CCW_STENCILZFAIL ),
	VE( D3DRS_CCW_STENCILPASS ),
	VE( D3DRS_CCW_STENCILFUNC ),
	VE( D3DRS_COLORWRITEENABLE1 ),
	VE( D3DRS_COLORWRITEENABLE2 ),
	VE( D3DRS_COLORWRITEENABLE3 ),
	VE( D3DRS_BLENDFACTOR ),
	VE( D3DRS_SRGBWRITEENABLE ),
	VE( D3DRS_DEPTHBIAS ),
	VE( D3DRS_WRAP8 ),
	VE( D3DRS_WRAP9 ),
	VE( D3DRS_WRAP10 ),
	VE( D3DRS_WRAP11 ),
	VE( D3DRS_WRAP12 ),
	VE( D3DRS_WRAP13 ),
	VE( D3DRS_WRAP14 ),
	VE( D3DRS_WRAP15 ),
	VE( D3DRS_SEPARATEALPHABLENDENABLE ),
	VE( D3DRS_SRCBLENDALPHA ),
	VE( D3DRS_DESTBLENDALPHA ),
	VE( D3DRS_BLENDOPALPHA ),
	
	VE( TERMVALUE )
};

GLMValueEntry_t g_d3d_opcodes[] = 
{
	VE( D3DSIO_NOP ),
	VE( D3DSIO_PHASE ),
	VE( D3DSIO_RET ),
	VE( D3DSIO_ELSE ),
	VE( D3DSIO_ENDIF ),
	VE( D3DSIO_ENDLOOP ),
	VE( D3DSIO_ENDREP ),
	VE( D3DSIO_BREAK ),
	VE( D3DSIO_TEXDEPTH ),
	VE( D3DSIO_TEXKILL ),
	VE( D3DSIO_BEM ),
	VE( D3DSIO_TEXBEM ),
	VE( D3DSIO_TEXBEML ),
	VE( D3DSIO_TEXDP3 ),
	VE( D3DSIO_TEXDP3TEX ),
	VE( D3DSIO_TEXM3x2DEPTH ),
	VE( D3DSIO_TEXM3x2TEX ),
	VE( D3DSIO_TEXM3x3 ),
	VE( D3DSIO_TEXM3x3PAD ),
	VE( D3DSIO_TEXM3x3TEX ),
	VE( D3DSIO_TEXM3x3VSPEC ),
	VE( D3DSIO_TEXREG2AR ),
	VE( D3DSIO_TEXREG2GB ),
	VE( D3DSIO_TEXREG2RGB ),
	VE( D3DSIO_LABEL ),
	VE( D3DSIO_CALL ),
	VE( D3DSIO_IF ),
	VE( D3DSIO_LOOP ),
	VE( D3DSIO_REP ),
	VE( D3DSIO_BREAKP ),
	VE( D3DSIO_DSX ),
	VE( D3DSIO_DSY ),
	VE( D3DSIO_NRM ),
	VE( D3DSIO_MOVA ),
	VE( D3DSIO_MOV ),
	VE( D3DSIO_RCP ),
	VE( D3DSIO_RSQ ),
	VE( D3DSIO_EXP ),
	VE( D3DSIO_EXPP ),
	VE( D3DSIO_LOG ),
	VE( D3DSIO_LOGP ),
	VE( D3DSIO_FRC ),
	VE( D3DSIO_LIT ),
	VE( D3DSIO_ABS ),
	VE( D3DSIO_TEXM3x3SPEC ),
	VE( D3DSIO_M4x4 ),
	VE( D3DSIO_M4x3 ),
	VE( D3DSIO_M3x4 ),
	VE( D3DSIO_M3x3 ),
	VE( D3DSIO_M3x2 ),
	VE( D3DSIO_CALLNZ ),
	VE( D3DSIO_IFC ),
	VE( D3DSIO_BREAKC ),
	VE( D3DSIO_SETP ),
	VE( D3DSIO_TEXLDL ),
	VE( D3DSIO_ADD ),
	VE( D3DSIO_SUB ),
	VE( D3DSIO_MUL ),
	VE( D3DSIO_DP3 ),
	VE( D3DSIO_DP4 ),
	VE( D3DSIO_MIN ),
	VE( D3DSIO_MAX ),
	VE( D3DSIO_DST ),
	VE( D3DSIO_SLT ),
	VE( D3DSIO_SGE ),
	VE( D3DSIO_CRS ),
	VE( D3DSIO_POW ),
	VE( D3DSIO_DP2ADD ),
	VE( D3DSIO_LRP ),
	VE( D3DSIO_SGN ),
	VE( D3DSIO_CND ),
	VE( D3DSIO_CMP ),
	VE( D3DSIO_SINCOS ),
	VE( D3DSIO_MAD ),
	VE( D3DSIO_TEXLDD ),
	VE( D3DSIO_TEXCOORD ),
	VE( D3DSIO_TEX ),
	VE( D3DSIO_DCL ),
	VE( D3DSTT_UNKNOWN ),
	VE( D3DSTT_2D ),
	VE( D3DSTT_CUBE ),
	VE( D3DSTT_VOLUME ),
	VE( D3DSIO_DEFB ),
	VE( D3DSIO_DEFI ),
	VE( D3DSIO_DEF ),
	VE( D3DSIO_COMMENT ),
	VE( D3DSIO_END ),
};


GLMValueEntry_t g_d3d_vtxdeclusages[] = 
{
	{ D3DDECLUSAGE_POSITION		,"POSN" },		// P
	{ D3DDECLUSAGE_BLENDWEIGHT	,"BLWT" },		// W
	{ D3DDECLUSAGE_BLENDINDICES	,"BLIX" },		// I
	{ D3DDECLUSAGE_NORMAL		,"NORM" },		// N
	{ D3DDECLUSAGE_PSIZE		,"PSIZ" },		// S
	{ D3DDECLUSAGE_TEXCOORD		,"TEXC" },		// T
	{ D3DDECLUSAGE_TANGENT		,"TANG" },		// G
	{ D3DDECLUSAGE_BINORMAL		,"BINO" },		// B
	{ D3DDECLUSAGE_TESSFACTOR	,"TESS" },		// S
	{ D3DDECLUSAGE_PLUGH		,"????" },		// ?
	{ D3DDECLUSAGE_COLOR		,"COLR" },		// C
	{ D3DDECLUSAGE_FOG			,"FOG " },		// F
	{ D3DDECLUSAGE_DEPTH		,"DEPT" },		// D
	{ D3DDECLUSAGE_SAMPLE		,"SAMP" }		// M
};

GLMValueEntry_t g_d3d_vtxdeclusages_short[] = 
{
	{ D3DDECLUSAGE_POSITION		,"P" },
	{ D3DDECLUSAGE_BLENDWEIGHT	,"W" },
	{ D3DDECLUSAGE_BLENDINDICES	,"I" },
	{ D3DDECLUSAGE_NORMAL		,"N" },
	{ D3DDECLUSAGE_PSIZE		,"S" },
	{ D3DDECLUSAGE_TEXCOORD		,"T" },
	{ D3DDECLUSAGE_TANGENT		,"G" },
	{ D3DDECLUSAGE_BINORMAL		,"B" },
	{ D3DDECLUSAGE_TESSFACTOR	,"S" },
	{ D3DDECLUSAGE_PLUGH		,"?" },
	{ D3DDECLUSAGE_COLOR		,"C" },
	{ D3DDECLUSAGE_FOG			,"F" },
	{ D3DDECLUSAGE_DEPTH		,"D" },
	{ D3DDECLUSAGE_SAMPLE		,"M" }
};

GLMValueEntry_t	g_cgl_rendids[] =			// need to mask with 0xFFFFFF00 to match on these (ex: 8800GT == 0x00022608 
{
	VE( kCGLRendererGenericID ),
	VE( kCGLRendererGenericFloatID ),
	VE( kCGLRendererAppleSWID ),
	VE( kCGLRendererATIRage128ID ),
	VE( kCGLRendererATIRadeonID ),
	VE( kCGLRendererATIRageProID ),
	VE( kCGLRendererATIRadeon8500ID ),
	VE( kCGLRendererATIRadeon9700ID ),
	VE( kCGLRendererATIRadeonX1000ID ),
	VE( kCGLRendererATIRadeonX2000ID ),
	VE( kCGLRendererGeForce2MXID ),
	VE( kCGLRendererGeForce3ID ),
	VE( kCGLRendererGeForceFXID ),			// also for GF6 and GF7
	VE( kCGLRendererGeForce8xxxID ),
	VE( kCGLRendererVTBladeXP2ID ),
	VE( kCGLRendererIntel900ID ),
	VE( kCGLRendererMesa3DFXID ),

	VE( TERMVALUE )
};

GLMValueEntry_t g_gl_errors[] = 
{
	VE( GL_INVALID_ENUM ),
	VE( GL_INVALID_VALUE ),
	VE( GL_INVALID_OPERATION ),
	VE( GL_STACK_OVERFLOW ),
	VE( GL_STACK_UNDERFLOW ),
	VE( GL_OUT_OF_MEMORY ),
	VE( GL_INVALID_FRAMEBUFFER_OPERATION_EXT ),
	VE( GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT ),
	VE( GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT ),
	VE( GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT ),
	VE( GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT ),
	VE( GL_FRAMEBUFFER_UNSUPPORTED_EXT ),
	VE( GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT ),
	VE( GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT )
};

// there are some ARB/EXT dupes in this table but that doesn't matter too much
GLMValueEntry_t g_gl_enums[] = 
{
	{	0x0000,	"GL_ZERO"	},
	{	0x0001,	"GL_ONE"	},
	{	0x0004,	"GL_TRIANGLES"	},
	{	0x0005,	"GL_TRIANGLE_STRIP"	},
	{	0x0006,	"GL_TRIANGLE_FAN"	},
	{	0x0007,	"GL_QUADS"	},
	{	0x0008,	"GL_QUAD_STRIP"	},
	{	0x0009,	"GL_POLYGON"	},
	{	0x0200,	"GL_NEVER"	},
	{	0x0201,	"GL_LESS"	},
	{	0x0202,	"GL_EQUAL"	},
	{	0x0203,	"GL_LEQUAL"	},
	{	0x0204,	"GL_GREATER"	},
	{	0x0205,	"GL_NOTEQUAL"	},
	{	0x0206,	"GL_GEQUAL"	},
	{	0x0207,	"GL_ALWAYS"	},
	{	0x0300,	"GL_SRC_COLOR"	},
	{	0x0301,	"GL_ONE_MINUS_SRC_COLOR"	},
	{	0x0302,	"GL_SRC_ALPHA"	},
	{	0x0303,	"GL_ONE_MINUS_SRC_ALPHA"	},
	{	0x0304,	"GL_DST_ALPHA"	},
	{	0x0305,	"GL_ONE_MINUS_DST_ALPHA"	},
	{	0x0306,	"GL_DST_COLOR"	},
	{	0x0307,	"GL_ONE_MINUS_DST_COLOR"	},
	{	0x0308,	"GL_SRC_ALPHA_SATURATE"	},
	{	0x0400,	"GL_FRONT_LEFT"	},
	{	0x0401,	"GL_FRONT_RIGHT"	},
	{	0x0402,	"GL_BACK_LEFT"	},
	{	0x0403,	"GL_BACK_RIGHT"	},
	{	0x0404,	"GL_FRONT"	},
	{	0x0405,	"GL_BACK"	},
	{	0x0406,	"GL_LEFT"	},
	{	0x0407,	"GL_RIGHT"	},
	{	0x0408,	"GL_FRONT_AND_BACK"	},
	{	0x0409,	"GL_AUX0"	},
	{	0x040A,	"GL_AUX1"	},
	{	0x040B,	"GL_AUX2"	},
	{	0x040C,	"GL_AUX3"	},
	{	0x0500,	"GL_INVALID_ENUM"	},
	{	0x0501,	"GL_INVALID_VALUE"	},
	{	0x0502,	"GL_INVALID_OPERATION"	},
	{	0x0503,	"GL_STACK_OVERFLOW"	},
	{	0x0504,	"GL_STACK_UNDERFLOW"	},
	{	0x0505,	"GL_OUT_OF_MEMORY"	},
	{	0x0506,	"GL_INVALID_FRAMEBUFFER_OPERATION"	},
	{	0x0600,	"GL_2D"	},
	{	0x0601,	"GL_3D"	},
	{	0x0602,	"GL_3D_COLOR"	},
	{	0x0603,	"GL_3D_COLOR_TEXTURE"	},
	{	0x0604,	"GL_4D_COLOR_TEXTURE"	},
	{	0x0700,	"GL_PASS_THROUGH_TOKEN"	},
	{	0x0701,	"GL_POINT_TOKEN"	},
	{	0x0702,	"GL_LINE_TOKEN"	},
	{	0x0703,	"GL_POLYGON_TOKEN"	},
	{	0x0704,	"GL_BITMAP_TOKEN"	},
	{	0x0705,	"GL_DRAW_PIXEL_TOKEN"	},
	{	0x0706,	"GL_COPY_PIXEL_TOKEN"	},
	{	0x0707,	"GL_LINE_RESET_TOKEN"	},
	{	0x0800,	"GL_EXP"	},
	{	0x0801,	"GL_EXP2"	},
	{	0x0900,	"GL_CW"	},
	{	0x0901,	"GL_CCW"	},
	{	0x0A00,	"GL_COEFF"	},
	{	0x0A01,	"GL_ORDER"	},
	{	0x0A02,	"GL_DOMAIN"	},
	{	0x0B00,	"GL_CURRENT_COLOR"	},
	{	0x0B01,	"GL_CURRENT_INDEX"	},
	{	0x0B02,	"GL_CURRENT_NORMAL"	},
	{	0x0B03,	"GL_CURRENT_TEXTURE_COORDS"	},
	{	0x0B04,	"GL_CURRENT_RASTER_COLOR"	},
	{	0x0B05,	"GL_CURRENT_RASTER_INDEX"	},
	{	0x0B06,	"GL_CURRENT_RASTER_TEXTURE_COORDS"	},
	{	0x0B07,	"GL_CURRENT_RASTER_POSITION"	},
	{	0x0B08,	"GL_CURRENT_RASTER_POSITION_VALID"	},
	{	0x0B09,	"GL_CURRENT_RASTER_DISTANCE"	},
	{	0x0B10,	"GL_POINT_SMOOTH"	},
	{	0x0B11,	"GL_POINT_SIZE"	},
	{	0x0B12,	"GL_POINT_SIZE_RANGE"	},
	{	0x0B12,	"GL_SMOOTH_POINT_SIZE_RANGE"	},
	{	0x0B13,	"GL_POINT_SIZE_GRANULARITY"	},
	{	0x0B13,	"GL_SMOOTH_POINT_SIZE_GRANULARITY"	},
	{	0x0B20,	"GL_LINE_SMOOTH"	},
	{	0x0B21,	"GL_LINE_WIDTH"	},
	{	0x0B22,	"GL_LINE_WIDTH_RANGE"	},
	{	0x0B22,	"GL_SMOOTH_LINE_WIDTH_RANGE"	},
	{	0x0B23,	"GL_LINE_WIDTH_GRANULARITY"	},
	{	0x0B23,	"GL_SMOOTH_LINE_WIDTH_GRANULARITY"	},
	{	0x0B24,	"GL_LINE_STIPPLE"	},
	{	0x0B25,	"GL_LINE_STIPPLE_PATTERN"	},
	{	0x0B26,	"GL_LINE_STIPPLE_REPEAT"	},
	{	0x0B30,	"GL_LIST_MODE"	},
	{	0x0B31,	"GL_MAX_LIST_NESTING"	},
	{	0x0B32,	"GL_LIST_BASE"	},
	{	0x0B33,	"GL_LIST_INDEX"	},
	{	0x0B40,	"GL_POLYGON_MODE"	},
	{	0x0B41,	"GL_POLYGON_SMOOTH"	},
	{	0x0B42,	"GL_POLYGON_STIPPLE"	},
	{	0x0B43,	"GL_EDGE_FLAG"	},
	{	0x0B44,	"GL_CULL_FACE"	},
	{	0x0B45,	"GL_CULL_FACE_MODE"	},
	{	0x0B46,	"GL_FRONT_FACE"	},
	{	0x0B50,	"GL_LIGHTING"	},
	{	0x0B51,	"GL_LIGHT_MODEL_LOCAL_VIEWER"	},
	{	0x0B52,	"GL_LIGHT_MODEL_TWO_SIDE"	},
	{	0x0B53,	"GL_LIGHT_MODEL_AMBIENT"	},
	{	0x0B54,	"GL_SHADE_MODEL"	},
	{	0x0B55,	"GL_COLOR_MATERIAL_FACE"	},
	{	0x0B56,	"GL_COLOR_MATERIAL_PARAMETER"	},
	{	0x0B57,	"GL_COLOR_MATERIAL"	},
	{	0x0B60,	"GL_FOG"	},
	{	0x0B61,	"GL_FOG_INDEX"	},
	{	0x0B62,	"GL_FOG_DENSITY"	},
	{	0x0B63,	"GL_FOG_START"	},
	{	0x0B64,	"GL_FOG_END"	},
	{	0x0B65,	"GL_FOG_MODE"	},
	{	0x0B66,	"GL_FOG_COLOR"	},
	{	0x0B70,	"GL_DEPTH_RANGE"	},
	{	0x0B71,	"GL_DEPTH_TEST"	},
	{	0x0B72,	"GL_DEPTH_WRITEMASK"	},
	{	0x0B73,	"GL_DEPTH_CLEAR_VALUE"	},
	{	0x0B74,	"GL_DEPTH_FUNC"	},
	{	0x0B80,	"GL_ACCUM_CLEAR_VALUE"	},
	{	0x0B90,	"GL_STENCIL_TEST"	},
	{	0x0B91,	"GL_STENCIL_CLEAR_VALUE"	},
	{	0x0B92,	"GL_STENCIL_FUNC"	},
	{	0x0B93,	"GL_STENCIL_VALUE_MASK"	},
	{	0x0B94,	"GL_STENCIL_FAIL"	},
	{	0x0B95,	"GL_STENCIL_PASS_DEPTH_FAIL"	},
	{	0x0B96,	"GL_STENCIL_PASS_DEPTH_PASS"	},
	{	0x0B97,	"GL_STENCIL_REF"	},
	{	0x0B98,	"GL_STENCIL_WRITEMASK"	},
	{	0x0BA0,	"GL_MATRIX_MODE"	},
	{	0x0BA1,	"GL_NORMALIZE"	},
	{	0x0BA2,	"GL_VIEWPORT"	},
	{	0x0BA3,	"GL_MODELVIEW_STACK_DEPTH"	},
	{	0x0BA4,	"GL_PROJECTION_STACK_DEPTH"	},
	{	0x0BA5,	"GL_TEXTURE_STACK_DEPTH"	},
	{	0x0BA6,	"GL_MODELVIEW_MATRIX"	},
	{	0x0BA7,	"GL_PROJECTION_MATRIX"	},
	{	0x0BA8,	"GL_TEXTURE_MATRIX"	},
	{	0x0BB0,	"GL_ATTRIB_STACK_DEPTH"	},
	{	0x0BB1,	"GL_CLIENT_ATTRIB_STACK_DEPTH"	},
	{	0x0BC0,	"GL_ALPHA_TEST"	},
	{	0x0BC1,	"GL_ALPHA_TEST_FUNC"	},
	{	0x0BC2,	"GL_ALPHA_TEST_REF"	},
	{	0x0BD0,	"GL_DITHER"	},
	{	0x0BE0,	"GL_BLEND_DST"	},
	{	0x0BE1,	"GL_BLEND_SRC"	},
	{	0x0BE2,	"GL_BLEND"	},
	{	0x0BF0,	"GL_LOGIC_OP_MODE"	},
	{	0x0BF1,	"GL_INDEX_LOGIC_OP"	},
	{	0x0BF2,	"GL_COLOR_LOGIC_OP"	},
	{	0x0C00,	"GL_AUX_BUFFERS"	},
	{	0x0C01,	"GL_DRAW_BUFFER"	},
	{	0x0C02,	"GL_READ_BUFFER"	},
	{	0x0C10,	"GL_SCISSOR_BOX"	},
	{	0x0C11,	"GL_SCISSOR_TEST"	},
	{	0x0C20,	"GL_INDEX_CLEAR_VALUE"	},
	{	0x0C21,	"GL_INDEX_WRITEMASK"	},
	{	0x0C22,	"GL_COLOR_CLEAR_VALUE"	},
	{	0x0C23,	"GL_COLOR_WRITEMASK"	},
	{	0x0C30,	"GL_INDEX_MODE"	},
	{	0x0C31,	"GL_RGBA_MODE"	},
	{	0x0C32,	"GL_DOUBLEBUFFER"	},
	{	0x0C33,	"GL_STEREO"	},
	{	0x0C40,	"GL_RENDER_MODE"	},
	{	0x0C50,	"GL_PERSPECTIVE_CORRECTION_HINT"	},
	{	0x0C51,	"GL_POINT_SMOOTH_HINT"	},
	{	0x0C52,	"GL_LINE_SMOOTH_HINT"	},
	{	0x0C53,	"GL_POLYGON_SMOOTH_HINT"	},
	{	0x0C54,	"GL_FOG_HINT"	},
	{	0x0C60,	"GL_TEXTURE_GEN_S"	},
	{	0x0C61,	"GL_TEXTURE_GEN_T"	},
	{	0x0C62,	"GL_TEXTURE_GEN_R"	},
	{	0x0C63,	"GL_TEXTURE_GEN_Q"	},
	{	0x0C70,	"GL_PIXEL_MAP_I_TO_I"	},
	{	0x0C71,	"GL_PIXEL_MAP_S_TO_S"	},
	{	0x0C72,	"GL_PIXEL_MAP_I_TO_R"	},
	{	0x0C73,	"GL_PIXEL_MAP_I_TO_G"	},
	{	0x0C74,	"GL_PIXEL_MAP_I_TO_B"	},
	{	0x0C75,	"GL_PIXEL_MAP_I_TO_A"	},
	{	0x0C76,	"GL_PIXEL_MAP_R_TO_R"	},
	{	0x0C77,	"GL_PIXEL_MAP_G_TO_G"	},
	{	0x0C78,	"GL_PIXEL_MAP_B_TO_B"	},
	{	0x0C79,	"GL_PIXEL_MAP_A_TO_A"	},
	{	0x0CB0,	"GL_PIXEL_MAP_I_TO_I_SIZE"	},
	{	0x0CB1,	"GL_PIXEL_MAP_S_TO_S_SIZE"	},
	{	0x0CB2,	"GL_PIXEL_MAP_I_TO_R_SIZE"	},
	{	0x0CB3,	"GL_PIXEL_MAP_I_TO_G_SIZE"	},
	{	0x0CB4,	"GL_PIXEL_MAP_I_TO_B_SIZE"	},
	{	0x0CB5,	"GL_PIXEL_MAP_I_TO_A_SIZE"	},
	{	0x0CB6,	"GL_PIXEL_MAP_R_TO_R_SIZE"	},
	{	0x0CB7,	"GL_PIXEL_MAP_G_TO_G_SIZE"	},
	{	0x0CB8,	"GL_PIXEL_MAP_B_TO_B_SIZE"	},
	{	0x0CB9,	"GL_PIXEL_MAP_A_TO_A_SIZE"	},
	{	0x0CF0,	"GL_UNPACK_SWAP_BYTES"	},
	{	0x0CF1,	"GL_UNPACK_LSB_FIRST"	},
	{	0x0CF2,	"GL_UNPACK_ROW_LENGTH"	},
	{	0x0CF3,	"GL_UNPACK_SKIP_ROWS"	},
	{	0x0CF4,	"GL_UNPACK_SKIP_PIXELS"	},
	{	0x0CF5,	"GL_UNPACK_ALIGNMENT"	},
	{	0x0D00,	"GL_PACK_SWAP_BYTES"	},
	{	0x0D01,	"GL_PACK_LSB_FIRST"	},
	{	0x0D02,	"GL_PACK_ROW_LENGTH"	},
	{	0x0D03,	"GL_PACK_SKIP_ROWS"	},
	{	0x0D04,	"GL_PACK_SKIP_PIXELS"	},
	{	0x0D05,	"GL_PACK_ALIGNMENT"	},
	{	0x0D10,	"GL_MAP_COLOR"	},
	{	0x0D11,	"GL_MAP_STENCIL"	},
	{	0x0D12,	"GL_INDEX_SHIFT"	},
	{	0x0D13,	"GL_INDEX_OFFSET"	},
	{	0x0D14,	"GL_RED_SCALE"	},
	{	0x0D15,	"GL_RED_BIAS"	},
	{	0x0D16,	"GL_ZOOM_X"	},
	{	0x0D17,	"GL_ZOOM_Y"	},
	{	0x0D18,	"GL_GREEN_SCALE"	},
	{	0x0D19,	"GL_GREEN_BIAS"	},
	{	0x0D1A,	"GL_BLUE_SCALE"	},
	{	0x0D1B,	"GL_BLUE_BIAS"	},
	{	0x0D1C,	"GL_ALPHA_SCALE"	},
	{	0x0D1D,	"GL_ALPHA_BIAS"	},
	{	0x0D1E,	"GL_DEPTH_SCALE"	},
	{	0x0D1F,	"GL_DEPTH_BIAS"	},
	{	0x0D30,	"GL_MAX_EVAL_ORDER"	},
	{	0x0D31,	"GL_MAX_LIGHTS"	},
	{	0x0D32,	"GL_MAX_CLIP_PLANES"	},
	{	0x0D33,	"GL_MAX_TEXTURE_SIZE"	},
	{	0x0D34,	"GL_MAX_PIXEL_MAP_TABLE"	},
	{	0x0D35,	"GL_MAX_ATTRIB_STACK_DEPTH"	},
	{	0x0D36,	"GL_MAX_MODELVIEW_STACK_DEPTH"	},
	{	0x0D37,	"GL_MAX_NAME_STACK_DEPTH"	},
	{	0x0D38,	"GL_MAX_PROJECTION_STACK_DEPTH"	},
	{	0x0D39,	"GL_MAX_TEXTURE_STACK_DEPTH"	},
	{	0x0D3A,	"GL_MAX_VIEWPORT_DIMS"	},
	{	0x0D3B,	"GL_MAX_CLIENT_ATTRIB_STACK_DEPTH"	},
	{	0x0D50,	"GL_SUBPIXEL_BITS"	},
	{	0x0D51,	"GL_INDEX_BITS"	},
	{	0x0D52,	"GL_RED_BITS"	},
	{	0x0D53,	"GL_GREEN_BITS"	},
	{	0x0D54,	"GL_BLUE_BITS"	},
	{	0x0D55,	"GL_ALPHA_BITS"	},
	{	0x0D56,	"GL_DEPTH_BITS"	},
	{	0x0D57,	"GL_STENCIL_BITS"	},
	{	0x0D58,	"GL_ACCUM_RED_BITS"	},
	{	0x0D59,	"GL_ACCUM_GREEN_BITS"	},
	{	0x0D5A,	"GL_ACCUM_BLUE_BITS"	},
	{	0x0D5B,	"GL_ACCUM_ALPHA_BITS"	},
	{	0x0D70,	"GL_NAME_STACK_DEPTH"	},
	{	0x0D80,	"GL_AUTO_NORMAL"	},
	{	0x0D90,	"GL_MAP1_COLOR_4"	},
	{	0x0D91,	"GL_MAP1_INDEX"	},
	{	0x0D92,	"GL_MAP1_NORMAL"	},
	{	0x0D93,	"GL_MAP1_TEXTURE_COORD_1"	},
	{	0x0D94,	"GL_MAP1_TEXTURE_COORD_2"	},
	{	0x0D95,	"GL_MAP1_TEXTURE_COORD_3"	},
	{	0x0D96,	"GL_MAP1_TEXTURE_COORD_4"	},
	{	0x0D97,	"GL_MAP1_VERTEX_3"	},
	{	0x0D98,	"GL_MAP1_VERTEX_4"	},
	{	0x0DB0,	"GL_MAP2_COLOR_4"	},
	{	0x0DB1,	"GL_MAP2_INDEX"	},
	{	0x0DB2,	"GL_MAP2_NORMAL"	},
	{	0x0DB3,	"GL_MAP2_TEXTURE_COORD_1"	},
	{	0x0DB4,	"GL_MAP2_TEXTURE_COORD_2"	},
	{	0x0DB5,	"GL_MAP2_TEXTURE_COORD_3"	},
	{	0x0DB6,	"GL_MAP2_TEXTURE_COORD_4"	},
	{	0x0DB7,	"GL_MAP2_VERTEX_3"	},
	{	0x0DB8,	"GL_MAP2_VERTEX_4"	},
	{	0x0DD0,	"GL_MAP1_GRID_DOMAIN"	},
	{	0x0DD1,	"GL_MAP1_GRID_SEGMENTS"	},
	{	0x0DD2,	"GL_MAP2_GRID_DOMAIN"	},
	{	0x0DD3,	"GL_MAP2_GRID_SEGMENTS"	},
	{	0x0DE0,	"GL_TEXTURE_1D"	},
	{	0x0DE1,	"GL_TEXTURE_2D"	},
	{	0x0DF0,	"GL_FEEDBACK_BUFFER_POINTER"	},
	{	0x0DF1,	"GL_FEEDBACK_BUFFER_SIZE"	},
	{	0x0DF2,	"GL_FEEDBACK_BUFFER_TYPE"	},
	{	0x0DF3,	"GL_SELECTION_BUFFER_POINTER"	},
	{	0x0DF4,	"GL_SELECTION_BUFFER_SIZE"	},
	{	0x1000,	"GL_TEXTURE_WIDTH"	},
	{	0x1001,	"GL_TEXTURE_HEIGHT"	},
	{	0x1003,	"GL_TEXTURE_INTERNAL_FORMAT"	},
	{	0x1004,	"GL_TEXTURE_BORDER_COLOR"	},
	{	0x1005,	"GL_TEXTURE_BORDER"	},
	{	0x1100,	"GL_DONT_CARE"	},
	{	0x1101,	"GL_FASTEST"	},
	{	0x1102,	"GL_NICEST"	},
	{	0x1200,	"GL_AMBIENT"	},
	{	0x1201,	"GL_DIFFUSE"	},
	{	0x1202,	"GL_SPECULAR"	},
	{	0x1203,	"GL_POSITION"	},
	{	0x1204,	"GL_SPOT_DIRECTION"	},
	{	0x1205,	"GL_SPOT_EXPONENT"	},
	{	0x1206,	"GL_SPOT_CUTOFF"	},
	{	0x1207,	"GL_CONSTANT_ATTENUATION"	},
	{	0x1208,	"GL_LINEAR_ATTENUATION"	},
	{	0x1209,	"GL_QUADRATIC_ATTENUATION"	},
	{	0x1300,	"GL_COMPILE"	},
	{	0x1301,	"GL_COMPILE_AND_EXECUTE"	},
	{	0x1400,	"GL_BYTE "	},
	{	0x1401,	"GL_UBYTE"	},
	{	0x1402,	"GL_SHORT"	},
	{	0x1403,	"GL_USHRT"	},
	{	0x1404,	"GL_INT  "	},
	{	0x1405,	"GL_UINT "	},
	{	0x1406,	"GL_FLOAT"	},
	{	0x1407,	"GL_2_BYTES"	},
	{	0x1408,	"GL_3_BYTES"	},
	{	0x1409,	"GL_4_BYTES"	},
	{	0x140A,	"GL_DOUBLE"	},
	{	0x140B,	"GL_HALF_FLOAT"	},
	{	0x1500,	"GL_CLEAR"	},
	{	0x1501,	"GL_AND"	},
	{	0x1502,	"GL_AND_REVERSE"	},
	{	0x1503,	"GL_COPY"	},
	{	0x1504,	"GL_AND_INVERTED"	},
	{	0x1505,	"GL_NOOP"	},
	{	0x1506,	"GL_XOR"	},
	{	0x1507,	"GL_OR"	},
	{	0x1508,	"GL_NOR"	},
	{	0x1509,	"GL_EQUIV"	},
	{	0x150A,	"GL_INVERT"	},
	{	0x150B,	"GL_OR_REVERSE"	},
	{	0x150C,	"GL_COPY_INVERTED"	},
	{	0x150D,	"GL_OR_INVERTED"	},
	{	0x150E,	"GL_NAND"	},
	{	0x150F,	"GL_SET"	},
	{	0x1600,	"GL_EMISSION"	},
	{	0x1601,	"GL_SHININESS"	},
	{	0x1602,	"GL_AMBIENT_AND_DIFFUSE"	},
	{	0x1603,	"GL_COLOR_INDEXES"	},
	{	0x1700,	"GL_MODELVIEW"	},
	{	0x1700,	"GL_MODELVIEW0_ARB"	},
	{	0x1701,	"GL_PROJECTION"	},
	{	0x1702,	"GL_TEXTURE"	},
	{	0x1800,	"GL_COLOR"	},
	{	0x1801,	"GL_DEPTH"	},
	{	0x1802,	"GL_STENCIL"	},
	{	0x1900,	"GL_COLOR_INDEX"	},
	{	0x1901,	"GL_STENCIL_INDEX"	},
	{	0x1902,	"GL_DEPTH_COMPONENT"	},
	{	0x1903,	"GL_RED"	},
	{	0x1904,	"GL_GREEN"	},
	{	0x1905,	"GL_BLUE"	},
	{	0x1906,	"GL_ALPHA"	},
	{	0x1907,	"GL_RGB"	},
	{	0x1908,	"GL_RGBA"	},
	{	0x1909,	"GL_LUMINANCE"	},
	{	0x190A,	"GL_LUMINANCE_ALPHA"	},
	{	0x1A00,	"GL_BITMAP"	},
	{	0x1B00,	"GL_POINT"	},
	{	0x1B01,	"GL_LINE"	},
	{	0x1B02,	"GL_FILL"	},
	{	0x1C00,	"GL_RENDER"	},
	{	0x1C01,	"GL_FEEDBACK"	},
	{	0x1C02,	"GL_SELECT"	},
	{	0x1D00,	"GL_FLAT"	},
	{	0x1D01,	"GL_SMOOTH"	},
	{	0x1E00,	"GL_KEEP"	},
	{	0x1E01,	"GL_REPLACE"	},
	{	0x1E02,	"GL_INCR"	},
	{	0x1E03,	"GL_DECR"	},
	{	0x1F00,	"GL_VENDOR"	},
	{	0x1F01,	"GL_RENDERER"	},
	{	0x1F02,	"GL_VERSION"	},
	{	0x1F03,	"GL_EXTENSIONS"	},
	{	0x2000,	"GL_S"	},
	{	0x2001,	"GL_T"	},
	{	0x2002,	"GL_R"	},
	{	0x2003,	"GL_Q"	},
	{	0x2100,	"GL_MODULATE"	},
	{	0x2101,	"GL_DECAL"	},
	{	0x2200,	"GL_TEXTURE_ENV_MODE"	},
	{	0x2201,	"GL_TEXTURE_ENV_COLOR"	},
	{	0x2300,	"GL_TEXTURE_ENV"	},
	{	0x2400,	"GL_EYE_LINEAR"	},
	{	0x2401,	"GL_OBJECT_LINEAR"	},
	{	0x2402,	"GL_SPHERE_MAP"	},
	{	0x2500,	"GL_TEXTURE_GEN_MODE"	},
	{	0x2501,	"GL_OBJECT_PLANE"	},
	{	0x2502,	"GL_EYE_PLANE"	},
	{	0x2600,	"GL_NEAREST"	},
	{	0x2601,	"GL_LINEAR"	},
	{	0x2700,	"GL_NEAREST_MIPMAP_NEAREST"	},
	{	0x2701,	"GL_LINEAR_MIPMAP_NEAREST"	},
	{	0x2702,	"GL_NEAREST_MIPMAP_LINEAR"	},
	{	0x2703,	"GL_LINEAR_MIPMAP_LINEAR"	},
	{	0x2800,	"GL_TEXTURE_MAG_FILTER"	},
	{	0x2801,	"GL_TEXTURE_MIN_FILTER"	},
	{	0x2802,	"GL_TEXTURE_WRAP_S"	},
	{	0x2803,	"GL_TEXTURE_WRAP_T"	},
	{	0x2900,	"GL_CLAMP"	},
	{	0x2901,	"GL_REPEAT"	},
	{	0x2A00,	"GL_POLYGON_OFFSET_UNITS"	},
	{	0x2A01,	"GL_POLYGON_OFFSET_POINT"	},
	{	0x2A02,	"GL_POLYGON_OFFSET_LINE"	},
	{	0x2A10,	"GL_R3_G3_B2"	},
	{	0x2A20,	"GL_V2F"	},
	{	0x2A21,	"GL_V3F"	},
	{	0x2A22,	"GL_C4UB_V2F"	},
	{	0x2A23,	"GL_C4UB_V3F"	},
	{	0x2A24,	"GL_C3F_V3F"	},
	{	0x2A25,	"GL_N3F_V3F"	},
	{	0x2A26,	"GL_C4F_N3F_V3F"	},
	{	0x2A27,	"GL_T2F_V3F"	},
	{	0x2A28,	"GL_T4F_V4F"	},
	{	0x2A29,	"GL_T2F_C4UB_V3F"	},
	{	0x2A2A,	"GL_T2F_C3F_V3F"	},
	{	0x2A2B,	"GL_T2F_N3F_V3F"	},
	{	0x2A2C,	"GL_T2F_C4F_N3F_V3F"	},
	{	0x2A2D,	"GL_T4F_C4F_N3F_V4F"	},
	{	0x3000,	"GL_CLIP_PLANE0"	},
	{	0x3001,	"GL_CLIP_PLANE1"	},
	{	0x3002,	"GL_CLIP_PLANE2"	},
	{	0x3003,	"GL_CLIP_PLANE3"	},
	{	0x3004,	"GL_CLIP_PLANE4"	},
	{	0x3005,	"GL_CLIP_PLANE5"	},
	{	0x4000,	"GL_LIGHT0"	},
	{	0x4001,	"GL_LIGHT1"	},
	{	0x4002,	"GL_LIGHT2"	},
	{	0x4003,	"GL_LIGHT3"	},
	{	0x4004,	"GL_LIGHT4"	},
	{	0x4005,	"GL_LIGHT5"	},
	{	0x4006,	"GL_LIGHT6"	},
	{	0x4007,	"GL_LIGHT7"	},
	{	0x8000,	"GL_ABGR_EXT"	},
	{	0x8001,	"GL_CONSTANT_COLOR"	},
	{	0x8002,	"GL_ONE_MINUS_CONSTANT_COLOR"	},
	{	0x8003,	"GL_CONSTANT_ALPHA"	},
	{	0x8004,	"GL_ONE_MINUS_CONSTANT_ALPHA"	},
	{	0x8005,	"GL_BLEND_COLOR"	},
	{	0x8006,	"GL_FUNC_ADD"	},
	{	0x8007,	"GL_MIN"	},
	{	0x8008,	"GL_MAX"	},
	{	0x8009,	"GL_BLEND_EQUATION_RGB"	},
	{	0x8009,	"GL_BLEND_EQUATION"	},
	{	0x800A,	"GL_FUNC_SUBTRACT"	},
	{	0x800B,	"GL_FUNC_REVERSE_SUBTRACT"	},
	{	0x8010,	"GL_CONVOLUTION_1D"	},
	{	0x8011,	"GL_CONVOLUTION_2D"	},
	{	0x8012,	"GL_SEPARABLE_2D"	},
	{	0x8013,	"GL_CONVOLUTION_BORDER_MODE"	},
	{	0x8014,	"GL_CONVOLUTION_FILTER_SCALE"	},
	{	0x8015,	"GL_CONVOLUTION_FILTER_BIAS"	},
	{	0x8016,	"GL_REDUCE"	},
	{	0x8017,	"GL_CONVOLUTION_FORMAT"	},
	{	0x8018,	"GL_CONVOLUTION_WIDTH"	},
	{	0x8019,	"GL_CONVOLUTION_HEIGHT"	},
	{	0x801A,	"GL_MAX_CONVOLUTION_WIDTH"	},
	{	0x801B,	"GL_MAX_CONVOLUTION_HEIGHT"	},
	{	0x801C,	"GL_POST_CONVOLUTION_RED_SCALE"	},
	{	0x801D,	"GL_POST_CONVOLUTION_GREEN_SCALE"	},
	{	0x801E,	"GL_POST_CONVOLUTION_BLUE_SCALE"	},
	{	0x801F,	"GL_POST_CONVOLUTION_ALPHA_SCALE"	},
	{	0x8020,	"GL_POST_CONVOLUTION_RED_BIAS"	},
	{	0x8021,	"GL_POST_CONVOLUTION_GREEN_BIAS"	},
	{	0x8022,	"GL_POST_CONVOLUTION_BLUE_BIAS"	},
	{	0x8023,	"GL_POST_CONVOLUTION_ALPHA_BIAS"	},
	{	0x8024,	"GL_HISTOGRAM"	},
	{	0x8025,	"GL_PROXY_HISTOGRAM"	},
	{	0x8026,	"GL_HISTOGRAM_WIDTH"	},
	{	0x8027,	"GL_HISTOGRAM_FORMAT"	},
	{	0x8028,	"GL_HISTOGRAM_RED_SIZE"	},
	{	0x8029,	"GL_HISTOGRAM_GREEN_SIZE"	},
	{	0x802A,	"GL_HISTOGRAM_BLUE_SIZE"	},
	{	0x802B,	"GL_HISTOGRAM_ALPHA_SIZE"	},
	{	0x802C,	"GL_HISTOGRAM_LUMINANCE_SIZE"	},
	{	0x802D,	"GL_HISTOGRAM_SINK"	},
	{	0x802E,	"GL_MINMAX"	},
	{	0x802F,	"GL_MINMAX_FORMAT"	},
	{	0x8030,	"GL_MINMAX_SINK"	},
	{	0x8031,	"GL_TABLE_TOO_LARGE"	},
	{	0x8032,	"GL_UNSIGNED_BYTE_3_3_2"	},
	{	0x8033,	"GL_UNSIGNED_SHORT_4_4_4_4"	},
	{	0x8034,	"GL_UNSIGNED_SHORT_5_5_5_1"	},
	{	0x8035,	"GL_UNSIGNED_INT_8_8_8_8"	},
	{	0x8036,	"GL_UNSIGNED_INT_10_10_10_2"	},
	{	0x8037,	"GL_POLYGON_OFFSET_FILL"	},
	{	0x8038,	"GL_POLYGON_OFFSET_FACTOR"	},
	{	0x803A,	"GL_RESCALE_NORMAL"	},
	{	0x803B,	"GL_ALPHA4"	},
	{	0x803C,	"GL_ALPHA8"	},
	{	0x803D,	"GL_ALPHA12"	},
	{	0x803E,	"GL_ALPHA16"	},
	{	0x803F,	"GL_LUMINANCE4"	},
	{	0x8040,	"GL_LUMINANCE8"	},
	{	0x8041,	"GL_LUMINANCE12"	},
	{	0x8042,	"GL_LUMINANCE16"	},
	{	0x8043,	"GL_LUMINANCE4_ALPHA4"	},
	{	0x8044,	"GL_LUMINANCE6_ALPHA2"	},
	{	0x8045,	"GL_LUMINANCE8_ALPHA8"	},
	{	0x8046,	"GL_LUMINANCE12_ALPHA4"	},
	{	0x8047,	"GL_LUMINANCE12_ALPHA12"	},
	{	0x8048,	"GL_LUMINANCE16_ALPHA16"	},
	{	0x8049,	"GL_INTENSITY"	},
	{	0x804A,	"GL_INTENSITY4"	},
	{	0x804B,	"GL_INTENSITY8"	},
	{	0x804C,	"GL_INTENSITY12"	},
	{	0x804D,	"GL_INTENSITY16"	},
	{	0x804F,	"GL_RGB4"	},
	{	0x8050,	"GL_RGB5"	},
	{	0x8051,	"GL_RGB8"	},
	{	0x8052,	"GL_RGB10"	},
	{	0x8053,	"GL_RGB12"	},
	{	0x8054,	"GL_RGB16"	},
	{	0x8055,	"GL_RGBA2"	},
	{	0x8056,	"GL_RGBA4"	},
	{	0x8057,	"GL_RGB5_A1"	},
	{	0x8058,	"GL_RGBA8"	},
	{	0x8059,	"GL_RGB10_A2"	},
	{	0x805A,	"GL_RGBA12"	},
	{	0x805B,	"GL_RGBA16"	},
	{	0x805C,	"GL_TEXTURE_RED_SIZE"	},
	{	0x805D,	"GL_TEXTURE_GREEN_SIZE"	},
	{	0x805E,	"GL_TEXTURE_BLUE_SIZE"	},
	{	0x805F,	"GL_TEXTURE_ALPHA_SIZE"	},
	{	0x8060,	"GL_TEXTURE_LUMINANCE_SIZE"	},
	{	0x8061,	"GL_TEXTURE_INTENSITY_SIZE"	},
	{	0x8063,	"GL_PROXY_TEXTURE_1D"	},
	{	0x8064,	"GL_PROXY_TEXTURE_2D"	},
	{	0x8066,	"GL_TEXTURE_PRIORITY"	},
	{	0x8067,	"GL_TEXTURE_RESIDENT"	},
	{	0x8068,	"GL_TEXTURE_BINDING_1D"	},
	{	0x8069,	"GL_TEXTURE_BINDING_2D"	},
	{	0x806A,	"GL_TEXTURE_BINDING_3D"	},
	{	0x806B,	"GL_PACK_SKIP_IMAGES"	},
	{	0x806C,	"GL_PACK_IMAGE_HEIGHT"	},
	{	0x806D,	"GL_UNPACK_SKIP_IMAGES"	},
	{	0x806E,	"GL_UNPACK_IMAGE_HEIGHT"	},
	{	0x806F,	"GL_TEXTURE_3D"	},
	{	0x8070,	"GL_PROXY_TEXTURE_3D"	},
	{	0x8071,	"GL_TEXTURE_DEPTH"	},
	{	0x8072,	"GL_TEXTURE_WRAP_R"	},
	{	0x8073,	"GL_MAX_3D_TEXTURE_SIZE"	},
	{	0x8074,	"GL_VERTEX_ARRAY"	},
	{	0x8075,	"GL_NORMAL_ARRAY"	},
	{	0x8076,	"GL_COLOR_ARRAY"	},
	{	0x8077,	"GL_INDEX_ARRAY"	},
	{	0x8078,	"GL_TEXTURE_COORD_ARRAY"	},
	{	0x8079,	"GL_EDGE_FLAG_ARRAY"	},
	{	0x807A,	"GL_VERTEX_ARRAY_SIZE"	},
	{	0x807B,	"GL_VERTEX_ARRAY_TYPE"	},
	{	0x807C,	"GL_VERTEX_ARRAY_STRIDE"	},
	{	0x807E,	"GL_NORMAL_ARRAY_TYPE"	},
	{	0x807F,	"GL_NORMAL_ARRAY_STRIDE"	},
	{	0x8081,	"GL_COLOR_ARRAY_SIZE"	},
	{	0x8082,	"GL_COLOR_ARRAY_TYPE"	},
	{	0x8083,	"GL_COLOR_ARRAY_STRIDE"	},
	{	0x8085,	"GL_INDEX_ARRAY_TYPE"	},
	{	0x8086,	"GL_INDEX_ARRAY_STRIDE"	},
	{	0x8088,	"GL_TEXTURE_COORD_ARRAY_SIZE"	},
	{	0x8089,	"GL_TEXTURE_COORD_ARRAY_TYPE"	},
	{	0x808A,	"GL_TEXTURE_COORD_ARRAY_STRIDE"	},
	{	0x808C,	"GL_EDGE_FLAG_ARRAY_STRIDE"	},
	{	0x808E,	"GL_VERTEX_ARRAY_POINTER"	},
	{	0x808F,	"GL_NORMAL_ARRAY_POINTER"	},
	{	0x8090,	"GL_COLOR_ARRAY_POINTER"	},
	{	0x8091,	"GL_INDEX_ARRAY_POINTER"	},
	{	0x8092,	"GL_TEXTURE_COORD_ARRAY_POINTER"	},
	{	0x8093,	"GL_EDGE_FLAG_ARRAY_POINTER"	},
	{	0x809D,	"GL_MULTISAMPLE_ARB"	},
	{	0x809D,	"GL_MULTISAMPLE"	},
	{	0x809E,	"GL_SAMPLE_ALPHA_TO_COVERAGE_ARB"	},
	{	0x809E,	"GL_SAMPLE_ALPHA_TO_COVERAGE"	},
	{	0x809F,	"GL_SAMPLE_ALPHA_TO_ONE_ARB"	},
	{	0x809F,	"GL_SAMPLE_ALPHA_TO_ONE"	},
	{	0x80A0,	"GL_SAMPLE_COVERAGE_ARB"	},
	{	0x80A0,	"GL_SAMPLE_COVERAGE"	},
	{	0x80A0,	"GL_SAMPLE_MASK_EXT"	},
	{	0x80A1,	"GL_1PASS_EXT"	},
	{	0x80A2,	"GL_2PASS_0_EXT"	},
	{	0x80A3,	"GL_2PASS_1_EXT"	},
	{	0x80A4,	"GL_4PASS_0_EXT"	},
	{	0x80A5,	"GL_4PASS_1_EXT"	},
	{	0x80A6,	"GL_4PASS_2_EXT"	},
	{	0x80A7,	"GL_4PASS_3_EXT"	},
	{	0x80A8,	"GL_SAMPLE_BUFFERS"	},
	{	0x80A9,	"GL_SAMPLES"	},
	{	0x80AA,	"GL_SAMPLE_COVERAGE_VALUE"	},
	{	0x80AB,	"GL_SAMPLE_COVERAGE_INVERT"	},
	{	0x80AC,	"GL_SAMPLE_PATTERN_EXT"	},
	{	0x80B1,	"GL_COLOR_MATRIX"	},
	{	0x80B2,	"GL_COLOR_MATRIX_STACK_DEPTH"	},
	{	0x80B3,	"GL_MAX_COLOR_MATRIX_STACK_DEPTH"	},
	{	0x80B4,	"GL_POST_COLOR_MATRIX_RED_SCALE"	},
	{	0x80B5,	"GL_POST_COLOR_MATRIX_GREEN_SCALE"	},
	{	0x80B6,	"GL_POST_COLOR_MATRIX_BLUE_SCALE"	},
	{	0x80B7,	"GL_POST_COLOR_MATRIX_ALPHA_SCALE"	},
	{	0x80B8,	"GL_POST_COLOR_MATRIX_RED_BIAS"	},
	{	0x80B9,	"GL_POST_COLOR_MATRIX_GREEN_BIAS"	},
	{	0x80BA,	"GL_POST_COLOR_MATRIX_BLUE_BIAS"	},
	{	0x80BB,	"GL_POST_COLOR_MATRIX_ALPHA_BIAS"	},
	{	0x80BF,	"GL_TEXTURE_COMPARE_FAIL_VALUE_ARB"	},
	{	0x80C8,	"GL_BLEND_DST_RGB"	},
	{	0x80C9,	"GL_BLEND_SRC_RGB"	},
	{	0x80CA,	"GL_BLEND_DST_ALPHA"	},
	{	0x80CB,	"GL_BLEND_SRC_ALPHA"	},
	{	0x80CC,	"GL_422_EXT"	},
	{	0x80CD,	"GL_422_REV_EXT"	},
	{	0x80CE,	"GL_422_AVERAGE_EXT"	},
	{	0x80CF,	"GL_422_REV_AVERAGE_EXT"	},
	{	0x80D0,	"GL_COLOR_TABLE"	},
	{	0x80D1,	"GL_POST_CONVOLUTION_COLOR_TABLE"	},
	{	0x80D2,	"GL_POST_COLOR_MATRIX_COLOR_TABLE"	},
	{	0x80D3,	"GL_PROXY_COLOR_TABLE"	},
	{	0x80D4,	"GL_PROXY_POST_CONVOLUTION_COLOR_TABLE"	},
	{	0x80D5,	"GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE"	},
	{	0x80D6,	"GL_COLOR_TABLE_SCALE"	},
	{	0x80D7,	"GL_COLOR_TABLE_BIAS"	},
	{	0x80D8,	"GL_COLOR_TABLE_FORMAT"	},
	{	0x80D9,	"GL_COLOR_TABLE_WIDTH"	},
	{	0x80DA,	"GL_COLOR_TABLE_RED_SIZE"	},
	{	0x80DB,	"GL_COLOR_TABLE_GREEN_SIZE"	},
	{	0x80DC,	"GL_COLOR_TABLE_BLUE_SIZE"	},
	{	0x80DD,	"GL_COLOR_TABLE_ALPHA_SIZE"	},
	{	0x80DE,	"GL_COLOR_TABLE_LUMINANCE_SIZE"	},
	{	0x80DF,	"GL_COLOR_TABLE_INTENSITY_SIZE"	},
	{	0x80E0,	"GL_BGR_EXT"	},
	{	0x80E0,	"GL_BGR"	},
	{	0x80E1,	"GL_BGRA_EXT"	},
	{	0x80E1,	"GL_BGRA"	},
	{	0x80E1,	"GL_BGRA"	},
	{	0x80E2,	"GL_COLOR_INDEX1_EXT"	},
	{	0x80E3,	"GL_COLOR_INDEX2_EXT"	},
	{	0x80E4,	"GL_COLOR_INDEX4_EXT"	},
	{	0x80E5,	"GL_COLOR_INDEX8_EXT"	},
	{	0x80E6,	"GL_COLOR_INDEX12_EXT"	},
	{	0x80E7,	"GL_COLOR_INDEX16_EXT"	},
	{	0x80E8,	"GL_MAX_ELEMENTS_VERTICES_EXT"	},
	{	0x80E8,	"GL_MAX_ELEMENTS_VERTICES"	},
	{	0x80E9,	"GL_MAX_ELEMENTS_INDICES_EXT"	},
	{	0x80E9,	"GL_MAX_ELEMENTS_INDICES"	},
	{	0x80ED,	"GL_TEXTURE_INDEX_SIZE_EXT"	},
	{	0x80F0,	"GL_CLIP_VOLUME_CLIPPING_HINT_EXT"	},
	{	0x8126,	"GL_POINT_SIZE_MIN_ARB"	},
	{	0x8126,	"GL_POINT_SIZE_MIN"	},
	{	0x8127,	"GL_POINT_SIZE_MAX_ARB"	},
	{	0x8127,	"GL_POINT_SIZE_MAX"	},
	{	0x8128,	"GL_POINT_FADE_THRESHOLD_SIZE_ARB"	},
	{	0x8128,	"GL_POINT_FADE_THRESHOLD_SIZE"	},
	{	0x8129,	"GL_POINT_DISTANCE_ATTENUATION_ARB"	},
	{	0x8129,	"GL_POINT_DISTANCE_ATTENUATION"	},
	{	0x812D,	"GL_CLAMP_TO_BORDER_ARB"	},
	{	0x812D,	"GL_CLAMP_TO_BORDER"	},
	{	0x812F,	"GL_CLAMP_TO_EDGE"	},
	{	0x813A,	"GL_TEXTURE_MIN_LOD"	},
	{	0x813B,	"GL_TEXTURE_MAX_LOD"	},
	{	0x813C,	"GL_TEXTURE_BASE_LEVEL"	},
	{	0x813D,	"GL_TEXTURE_MAX_LEVEL"	},
	{	0x8151,	"GL_CONSTANT_BORDER"	},
	{	0x8153,	"GL_REPLICATE_BORDER"	},
	{	0x8154,	"GL_CONVOLUTION_BORDER_COLOR"	},
	{	0x8191,	"GL_GENERATE_MIPMAP"	},
	{	0x8192,	"GL_GENERATE_MIPMAP_HINT"	},
	{	0x81A5,	"GL_DEPTH_COMPONENT16_ARB"	},
	{	0x81A5,	"GL_DEPTH_COMPONENT16"	},
	{	0x81A6,	"GL_DEPTH_COMPONENT24_ARB"	},
	{	0x81A6,	"GL_DEPTH_COMPONENT24"	},
	{	0x81A7,	"GL_DEPTH_COMPONENT32_ARB"	},
	{	0x81A7,	"GL_DEPTH_COMPONENT32"	},
	{	0x81A8,	"GL_ARRAY_ELEMENT_LOCK_FIRST_EXT"	},
	{	0x81A9,	"GL_ARRAY_ELEMENT_LOCK_COUNT_EXT"	},
	{	0x81AA,	"GL_CULL_VERTEX_EXT"	},
	{	0x81AB,	"GL_CULL_VERTEX_EYE_POSITION_EXT"	},
	{	0x81AC,	"GL_CULL_VERTEX_OBJECT_POSITION_EXT"	},
	{	0x81AD,	"GL_IUI_V2F_EXT"	},
	{	0x81AE,	"GL_IUI_V3F_EXT"	},
	{	0x81AF,	"GL_IUI_N3F_V2F_EXT"	},
	{	0x81B0,	"GL_IUI_N3F_V3F_EXT"	},
	{	0x81B1,	"GL_T2F_IUI_V2F_EXT"	},
	{	0x81B2,	"GL_T2F_IUI_V3F_EXT"	},
	{	0x81B3,	"GL_T2F_IUI_N3F_V2F_EXT"	},
	{	0x81B4,	"GL_T2F_IUI_N3F_V3F_EXT"	},
	{	0x81B5,	"GL_INDEX_TEST_EXT"	},
	{	0x81B6,	"GL_INDEX_TEST_FUNC_EXT"	},
	{	0x81B7,	"GL_INDEX_TEST_REF_EXT"	},
	{	0x81B8,	"GL_INDEX_MATERIAL_EXT"	},
	{	0x81B9,	"GL_INDEX_MATERIAL_PARAMETER_EXT"	},
	{	0x81BA,	"GL_INDEX_MATERIAL_FACE_EXT"	},
	{	0x81F8,	"GL_LIGHT_MODEL_COLOR_CONTROL_EXT"	},
	{	0x81F8,	"GL_LIGHT_MODEL_COLOR_CONTROL"	},
	{	0x81F9,	"GL_SINGLE_COLOR_EXT"	},
	{	0x81F9,	"GL_SINGLE_COLOR"	},
	{	0x81FA,	"GL_SEPARATE_SPECULAR_COLOR_EXT"	},
	{	0x81FA,	"GL_SEPARATE_SPECULAR_COLOR"	},
	{	0x81FB,	"GL_SHARED_TEXTURE_PALETTE_EXT"	},
	{	0x8210,	"GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING"	},
	{	0x8211,	"GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE"	},
	{	0x8212,	"GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE"	},
	{	0x8213,	"GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE"	},
	{	0x8214,	"GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE"	},
	{	0x8215,	"GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE"	},
	{	0x8216,	"GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE"	},
	{	0x8217,	"GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE"	},
	{	0x8218,	"GL_FRAMEBUFFER_DEFAULT"	},
	{	0x8219,	"GL_FRAMEBUFFER_UNDEFINED"	},
	{	0x821A,	"GL_DEPTH_STENCIL_ATTACHMENT"	},
	{	0x8225,	"GL_COMPRESSED_RED"	},
	{	0x8226,	"GL_COMPRESSED_RG"	},
	{	0x8227,	"GL_RG"	},
	{	0x8228,	"GL_RG_INTEGER"	},
	{	0x8229,	"GL_R8"	},
	{	0x822A,	"GL_R16"	},
	{	0x822B,	"GL_RG8"	},
	{	0x822C,	"GL_RG16"	},
	{	0x822D,	"GL_R16F"	},
	{	0x822E,	"GL_R32F"	},
	{	0x822F,	"GL_RG16F"	},
	{	0x8230,	"GL_RG32F"	},
	{	0x8231,	"GL_R8I"	},
	{	0x8232,	"GL_R8UI"	},
	{	0x8233,	"GL_R16I"	},
	{	0x8234,	"GL_R16UI"	},
	{	0x8235,	"GL_R32I"	},
	{	0x8236,	"GL_R32UI"	},
	{	0x8237,	"GL_RG8I"	},
	{	0x8238,	"GL_RG8UI"	},
	{	0x8239,	"GL_RG16I"	},
	{	0x823A,	"GL_RG16UI"	},
	{	0x823B,	"GL_RG32I"	},
	{	0x823C,	"GL_RG32UI"	},
	{	0x8330,	"GL_PIXEL_TRANSFORM_2D_EXT"	},
	{	0x8331,	"GL_PIXEL_MAG_FILTER_EXT"	},
	{	0x8332,	"GL_PIXEL_MIN_FILTER_EXT"	},
	{	0x8333,	"GL_PIXEL_CUBIC_WEIGHT_EXT"	},
	{	0x8334,	"GL_CUBIC_EXT"	},
	{	0x8335,	"GL_AVERAGE_EXT"	},
	{	0x8336,	"GL_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT"	},
	{	0x8337,	"GL_MAX_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT"	},
	{	0x8338,	"GL_PIXEL_TRANSFORM_2D_MATRIX_EXT"	},
	{	0x8349,	"GL_FRAGMENT_MATERIAL_EXT"	},
	{	0x834A,	"GL_FRAGMENT_NORMAL_EXT"	},
	{	0x834C,	"GL_FRAGMENT_COLOR_EXT"	},
	{	0x834D,	"GL_ATTENUATION_EXT"	},
	{	0x834E,	"GL_SHADOW_ATTENUATION_EXT"	},
	{	0x834F,	"GL_TEXTURE_APPLICATION_MODE_EXT"	},
	{	0x8350,	"GL_TEXTURE_LIGHT_EXT"	},
	{	0x8351,	"GL_TEXTURE_MATERIAL_FACE_EXT"	},
	{	0x8352,	"GL_TEXTURE_MATERIAL_PARAMETER_EXT"	},
	{	0x8362,	"GL_UNSIGNED_BYTE_2_3_3_REV"	},
	{	0x8363,	"GL_UNSIGNED_SHORT_5_6_5"	},
	{	0x8364,	"GL_UNSIGNED_SHORT_5_6_5_REV"	},
	{	0x8365,	"GL_UNSIGNED_SHORT_4_4_4_4_REV"	},
	{	0x8366,	"GL_UNSIGNED_SHORT_1_5_5_5_REV"	},
	{	0x8367,	"GL_UNSIGNED_INT_8_8_8_8_REV"	},
	{	0x8368,	"GL_UNSIGNED_INT_2_10_10_10_REV"	},
	{	0x8370,	"GL_MIRRORED_REPEAT_ARB"	},
	{	0x8370,	"GL_MIRRORED_REPEAT"	},
	{	0x83F0,	"GL_COMPRESSED_RGB_S3TC_DXT1_EXT"	},
	{	0x83F1,	"GL_COMPRESSED_RGBA_S3TC_DXT1_EXT"	},
	{	0x83F2,	"GL_COMPRESSED_RGBA_S3TC_DXT3_EXT"	},
	{	0x83F3,	"GL_COMPRESSED_RGBA_S3TC_DXT5_EXT"	},
	{	0x8439,	"GL_TANGENT_ARRAY_EXT"	},
	{	0x843A,	"GL_BINORMAL_ARRAY_EXT"	},
	{	0x843B,	"GL_CURRENT_TANGENT_EXT"	},
	{	0x843C,	"GL_CURRENT_BINORMAL_EXT"	},
	{	0x843E,	"GL_TANGENT_ARRAY_TYPE_EXT"	},
	{	0x843F,	"GL_TANGENT_ARRAY_STRIDE_EXT"	},
	{	0x8440,	"GL_BINORMAL_ARRAY_TYPE_EXT"	},
	{	0x8441,	"GL_BINORMAL_ARRAY_STRIDE_EXT"	},
	{	0x8442,	"GL_TANGENT_ARRAY_POINTER_EXT"	},
	{	0x8443,	"GL_BINORMAL_ARRAY_POINTER_EXT"	},
	{	0x8444,	"GL_MAP1_TANGENT_EXT"	},
	{	0x8445,	"GL_MAP2_TANGENT_EXT"	},
	{	0x8446,	"GL_MAP1_BINORMAL_EXT"	},
	{	0x8447,	"GL_MAP2_BINORMAL_EXT"	},
	{	0x8450,	"GL_FOG_COORD_SRC"	},
	{	0x8450,	"GL_FOG_COORDINATE_SOURCE_EXT"	},
	{	0x8450,	"GL_FOG_COORDINATE_SOURCE"	},
	{	0x8451,	"GL_FOG_COORD"	},
	{	0x8451,	"GL_FOG_COORDINATE_EXT"	},
	{	0x8451,	"GL_FOG_COORDINATE"	},
	{	0x8452,	"GL_FRAGMENT_DEPTH_EXT"	},
	{	0x8452,	"GL_FRAGMENT_DEPTH"	},
	{	0x8453  ,	"GL_CURRENT_FOG_COORD"	},
	{	0x8453  ,	"GL_CURRENT_FOG_COORDINATE"	},
	{	0x8453,	"GL_CURRENT_FOG_COORDINATE_EXT"	},
	{	0x8454,	"GL_FOG_COORD_ARRAY_TYPE"	},
	{	0x8454,	"GL_FOG_COORDINATE_ARRAY_TYPE_EXT"	},
	{	0x8454,	"GL_FOG_COORDINATE_ARRAY_TYPE"	},
	{	0x8455,	"GL_FOG_COORD_ARRAY_STRIDE"	},
	{	0x8455,	"GL_FOG_COORDINATE_ARRAY_STRIDE_EXT"	},
	{	0x8455,	"GL_FOG_COORDINATE_ARRAY_STRIDE"	},
	{	0x8456,	"GL_FOG_COORD_ARRAY_POINTER"	},
	{	0x8456,	"GL_FOG_COORDINATE_ARRAY_POINTER_EXT"	},
	{	0x8456,	"GL_FOG_COORDINATE_ARRAY_POINTER"	},
	{	0x8457,	"GL_FOG_COORD_ARRAY"	},
	{	0x8457,	"GL_FOG_COORDINATE_ARRAY_EXT"	},
	{	0x8457,	"GL_FOG_COORDINATE_ARRAY"	},
	{	0x8458,	"GL_COLOR_SUM_ARB"	},
	{	0x8458,	"GL_COLOR_SUM_EXT"	},
	{	0x8458,	"GL_COLOR_SUM"	},
	{	0x8459,	"GL_CURRENT_SECONDARY_COLOR_EXT"	},
	{	0x8459,	"GL_CURRENT_SECONDARY_COLOR"	},
	{	0x845A,	"GL_SECONDARY_COLOR_ARRAY_SIZE_EXT"	},
	{	0x845A,	"GL_SECONDARY_COLOR_ARRAY_SIZE"	},
	{	0x845B,	"GL_SECONDARY_COLOR_ARRAY_TYPE_EXT"	},
	{	0x845B,	"GL_SECONDARY_COLOR_ARRAY_TYPE"	},
	{	0x845C,	"GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT"	},
	{	0x845C,	"GL_SECONDARY_COLOR_ARRAY_STRIDE"	},
	{	0x845D,	"GL_SECONDARY_COLOR_ARRAY_POINTER_EXT"	},
	{	0x845D,	"GL_SECONDARY_COLOR_ARRAY_POINTER"	},
	{	0x845E,	"GL_SECONDARY_COLOR_ARRAY_EXT"	},
	{	0x845E,	"GL_SECONDARY_COLOR_ARRAY"	},
	{	0x845F,	"GL_CURRENT_RASTER_SECONDARY_COLOR"	},
	{	0x846D,	"GL_ALIASED_POINT_SIZE_RANGE"	},
	{	0x846E,	"GL_ALIASED_LINE_WIDTH_RANGE"	},
	{	0x84C0,	"GL_TEXTURE0"	},
	{	0x84C1,	"GL_TEXTURE1"	},
	{	0x84C2,	"GL_TEXTURE2"	},
	{	0x84C3,	"GL_TEXTURE3"	},
	{	0x84C4,	"GL_TEXTURE4"	},
	{	0x84C5,	"GL_TEXTURE5"	},
	{	0x84C6,	"GL_TEXTURE6"	},
	{	0x84C7,	"GL_TEXTURE7"	},
	{	0x84C8,	"GL_TEXTURE8"	},
	{	0x84C9,	"GL_TEXTURE9"	},
	{	0x84CA,	"GL_TEXTURE10"	},
	{	0x84CB,	"GL_TEXTURE11"	},
	{	0x84CC,	"GL_TEXTURE12"	},
	{	0x84CD,	"GL_TEXTURE13"	},
	{	0x84CE,	"GL_TEXTURE14"	},
	{	0x84CF,	"GL_TEXTURE15"	},
	{	0x84D0,	"GL_TEXTURE16"	},
	{	0x84D1,	"GL_TEXTURE17"	},
	{	0x84D2,	"GL_TEXTURE18"	},
	{	0x84D3,	"GL_TEXTURE19"	},
	{	0x84D4,	"GL_TEXTURE20"	},
	{	0x84D5,	"GL_TEXTURE21"	},
	{	0x84D6,	"GL_TEXTURE22"	},
	{	0x84D7,	"GL_TEXTURE23"	},
	{	0x84D8,	"GL_TEXTURE24"	},
	{	0x84D9,	"GL_TEXTURE25"	},
	{	0x84DA,	"GL_TEXTURE26"	},
	{	0x84DB,	"GL_TEXTURE27"	},
	{	0x84DC,	"GL_TEXTURE28"	},
	{	0x84DD,	"GL_TEXTURE29"	},
	{	0x84DE,	"GL_TEXTURE30"	},
	{	0x84DF,	"GL_TEXTURE31"	},
	{	0x84E0,	"GL_ACTIVE_TEXTURE"	},
	{	0x84E1,	"GL_CLIENT_ACTIVE_TEXTURE"	},
	{	0x84E2,	"GL_MAX_TEXTURE_UNITS"	},
	{	0x84E3,	"GL_TRANSPOSE_MODELVIEW_MATRIX"	},
	{	0x84E4,	"GL_TRANSPOSE_PROJECTION_MATRIX"	},
	{	0x84E5,	"GL_TRANSPOSE_TEXTURE_MATRIX"	},
	{	0x84E6,	"GL_TRANSPOSE_COLOR_MATRIX"	},
	{	0x84E7,	"GL_SUBTRACT"	},
	{	0x84E8,	"GL_MAX_RENDERBUFFER_SIZE"	},
	{	0x84E9,	"GL_COMPRESSED_ALPHA"	},
	{	0x84EA,	"GL_COMPRESSED_LUMINANCE"	},
	{	0x84EB,	"GL_COMPRESSED_LUMINANCE_ALPHA"	},
	{	0x84EC,	"GL_COMPRESSED_INTENSITY"	},
	{	0x84ED,	"GL_COMPRESSED_RGB"	},
	{	0x84EE,	"GL_COMPRESSED_RGBA"	},
	{	0x84EF,	"GL_TEXTURE_COMPRESSION_HINT"	},
	{	0x84F5,	"GL_TEXTURE_RECTANGLE_EXT"	},
	{	0x84F6,	"GL_TEXTURE_BINDING_RECTANGLE_EXT"	},
	{	0x84F7,	"GL_PROXY_TEXTURE_RECTANGLE_EXT"	},
	{	0x84F8,	"GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT"	},
	{	0x84F9,	"GL_DEPTH_STENCIL"	},
	{	0x84FA,	"GL_UNSIGNED_INT_24_8"	},
	{	0x84FD,	"GL_MAX_TEXTURE_LOD_BIAS"	},
	{	0x84FE,	"GL_TEXTURE_MAX_ANISOTROPY_EXT"	},
	{	0x84FF,	"GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT"	},
	{	0x8500,	"GL_TEXTURE_FILTER_CONTROL"	},
	{	0x8501,	"GL_TEXTURE_LOD_BIAS"	},
	{	0x8502,	"GL_MODELVIEW1_STACK_DEPTH_EXT"	},
	{	0x8506,	"GL_MODELVIEW_MATRIX1_EXT"	},
	{	0x8507,	"GL_INCR_WRAP"	},
	{	0x8508,	"GL_DECR_WRAP"	},
	{	0x8509,	"GL_VERTEX_WEIGHTING_EXT"	},
	{	0x850A,	"GL_MODELVIEW1_ARB"	},
	{	0x850B,	"GL_CURRENT_VERTEX_WEIGHT_EXT"	},
	{	0x850C,	"GL_VERTEX_WEIGHT_ARRAY_EXT"	},
	{	0x850D,	"GL_VERTEX_WEIGHT_ARRAY_SIZE_EXT"	},
	{	0x850E,	"GL_VERTEX_WEIGHT_ARRAY_TYPE_EXT"	},
	{	0x850F,	"GL_VERTEX_WEIGHT_ARRAY_STRIDE_EXT"	},
	{	0x8510,	"GL_VERTEX_WEIGHT_ARRAY_POINTER_EXT"	},
	{	0x8511,	"GL_NORMAL_MAP_ARB"	},
	{	0x8511,	"GL_NORMAL_MAP_EXT"	},
	{	0x8511,	"GL_NORMAL_MAP"	},
	{	0x8512,	"GL_REFLECTION_MAP_ARB"	},
	{	0x8512,	"GL_REFLECTION_MAP_EXT"	},
	{	0x8512,	"GL_REFLECTION_MAP"	},
	{	0x8513,	"GL_TEXTURE_CUBE_MAP_ARB"	},
	{	0x8513,	"GL_TEXTURE_CUBE_MAP_EXT"	},
	{	0x8513,	"GL_TEXTURE_CUBE_MAP"	},
	{	0x8514,	"GL_TEXTURE_BINDING_CUBE_MAP_ARB"	},
	{	0x8514,	"GL_TEXTURE_BINDING_CUBE_MAP_EXT"	},
	{	0x8514,	"GL_TEXTURE_BINDING_CUBE_MAP"	},
	{	0x8515,	"GL_TEXTURE_CUBE_MAP_POSITIVE_X"	},
	{	0x8516,	"GL_TEXTURE_CUBE_MAP_NEGATIVE_X"	},
	{	0x8517,	"GL_TEXTURE_CUBE_MAP_POSITIVE_Y"	},
	{	0x8518,	"GL_TEXTURE_CUBE_MAP_NEGATIVE_Y"	},
	{	0x8519,	"GL_TEXTURE_CUBE_MAP_POSITIVE_Z"	},
	{	0x851A,	"GL_TEXTURE_CUBE_MAP_NEGATIVE_Z"	},
	{	0x851B,	"GL_PROXY_TEXTURE_CUBE_MAP"	},
	{	0x851C,	"GL_MAX_CUBE_MAP_TEXTURE_SIZE"	},
	{	0x851D,	"GL_VERTEX_ARRAY_RANGE_APPLE"	},
	{	0x851E,	"GL_VERTEX_ARRAY_RANGE_LENGTH_APPLE"	},
	{	0x851F,	"GL_VERTEX_ARRAY_STORAGE_HINT_APPLE"	},
	{	0x8520,	"GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_APPLE"	},
	{	0x8521,	"GL_VERTEX_ARRAY_RANGE_POINTER_APPLE"	},
	{	0x8570,	"GL_COMBINE_ARB"	},
	{	0x8570,	"GL_COMBINE_EXT"	},
	{	0x8570,	"GL_COMBINE"	},
	{	0x8571,	"GL_COMBINE_RGB_ARB"	},
	{	0x8571,	"GL_COMBINE_RGB_EXT"	},
	{	0x8571,	"GL_COMBINE_RGB"	},
	{	0x8572,	"GL_COMBINE_ALPHA_ARB"	},
	{	0x8572,	"GL_COMBINE_ALPHA_EXT"	},
	{	0x8572,	"GL_COMBINE_ALPHA"	},
	{	0x8573,	"GL_RGB_SCALE_ARB"	},
	{	0x8573,	"GL_RGB_SCALE_EXT"	},
	{	0x8573,	"GL_RGB_SCALE"	},
	{	0x8574,	"GL_ADD_SIGNED_ARB"	},
	{	0x8574,	"GL_ADD_SIGNED_EXT"	},
	{	0x8574,	"GL_ADD_SIGNED"	},
	{	0x8575,	"GL_INTERPOLATE_ARB"	},
	{	0x8575,	"GL_INTERPOLATE_EXT"	},
	{	0x8575,	"GL_INTERPOLATE"	},
	{	0x8576,	"GL_CONSTANT_ARB"	},
	{	0x8576,	"GL_CONSTANT_EXT"	},
	{	0x8576,	"GL_CONSTANT"	},
	{	0x8577,	"GL_PRIMARY_COLOR_ARB"	},
	{	0x8577,	"GL_PRIMARY_COLOR_EXT"	},
	{	0x8577,	"GL_PRIMARY_COLOR"	},
	{	0x8578,	"GL_PREVIOUS_ARB"	},
	{	0x8578,	"GL_PREVIOUS_EXT"	},
	{	0x8578,	"GL_PREVIOUS"	},
	{	0x8580,	"GL_SOURCE0_RGB_ARB"	},
	{	0x8580,	"GL_SOURCE0_RGB_EXT"	},
	{	0x8580,	"GL_SOURCE0_RGB"	},
	{	0x8580,	"GL_SRC0_RGB"	},
	{	0x8581,	"GL_SOURCE1_RGB_ARB"	},
	{	0x8581,	"GL_SOURCE1_RGB_EXT"	},
	{	0x8581,	"GL_SOURCE1_RGB"	},
	{	0x8581,	"GL_SRC1_RGB"	},
	{	0x8582,	"GL_SOURCE2_RGB_ARB"	},
	{	0x8582,	"GL_SOURCE2_RGB_EXT"	},
	{	0x8582,	"GL_SOURCE2_RGB"	},
	{	0x8582,	"GL_SRC2_RGB"	},
	{	0x8583,	"GL_SOURCE3_RGB_ARB"	},
	{	0x8583,	"GL_SOURCE3_RGB_EXT"	},
	{	0x8583,	"GL_SOURCE3_RGB"	},
	{	0x8583,	"GL_SRC3_RGB"	},
	{	0x8584,	"GL_SOURCE4_RGB_ARB"	},
	{	0x8584,	"GL_SOURCE4_RGB_EXT"	},
	{	0x8584,	"GL_SOURCE4_RGB"	},
	{	0x8584,	"GL_SRC4_RGB"	},
	{	0x8585,	"GL_SOURCE5_RGB_ARB"	},
	{	0x8585,	"GL_SOURCE5_RGB_EXT"	},
	{	0x8585,	"GL_SOURCE5_RGB"	},
	{	0x8585,	"GL_SRC5_RGB"	},
	{	0x8586,	"GL_SOURCE6_RGB_ARB"	},
	{	0x8586,	"GL_SOURCE6_RGB_EXT"	},
	{	0x8586,	"GL_SOURCE6_RGB"	},
	{	0x8586,	"GL_SRC6_RGB"	},
	{	0x8587,	"GL_SOURCE7_RGB_ARB"	},
	{	0x8587,	"GL_SOURCE7_RGB_EXT"	},
	{	0x8587,	"GL_SOURCE7_RGB"	},
	{	0x8587,	"GL_SRC7_RGB"	},
	{	0x8588,	"GL_SOURCE0_ALPHA_ARB"	},
	{	0x8588,	"GL_SOURCE0_ALPHA_EXT"	},
	{	0x8588,	"GL_SOURCE0_ALPHA"	},
	{	0x8588,	"GL_SRC0_ALPHA"	},
	{	0x8589,	"GL_SOURCE1_ALPHA_ARB"	},
	{	0x8589,	"GL_SOURCE1_ALPHA_EXT"	},
	{	0x8589,	"GL_SOURCE1_ALPHA"	},
	{	0x8589,	"GL_SRC1_ALPHA"	},
	{	0x858A,	"GL_SOURCE2_ALPHA_ARB"	},
	{	0x858A,	"GL_SOURCE2_ALPHA_EXT"	},
	{	0x858A,	"GL_SOURCE2_ALPHA"	},
	{	0x858A,	"GL_SRC2_ALPHA"	},
	{	0x858B,	"GL_SOURCE3_ALPHA_ARB"	},
	{	0x858B,	"GL_SOURCE3_ALPHA_EXT"	},
	{	0x858B,	"GL_SOURCE3_ALPHA"	},
	{	0x858B,	"GL_SRC3_ALPHA"	},
	{	0x858C,	"GL_SOURCE4_ALPHA_ARB"	},
	{	0x858C,	"GL_SOURCE4_ALPHA_EXT"	},
	{	0x858C,	"GL_SOURCE4_ALPHA"	},
	{	0x858C,	"GL_SRC4_ALPHA"	},
	{	0x858D,	"GL_SOURCE5_ALPHA_ARB"	},
	{	0x858D,	"GL_SOURCE5_ALPHA_EXT"	},
	{	0x858D,	"GL_SOURCE5_ALPHA"	},
	{	0x858D,	"GL_SRC5_ALPHA"	},
	{	0x858E,	"GL_SOURCE6_ALPHA_ARB"	},
	{	0x858E,	"GL_SOURCE6_ALPHA_EXT"	},
	{	0x858E,	"GL_SOURCE6_ALPHA"	},
	{	0x858E,	"GL_SRC6_ALPHA"	},
	{	0x858F,	"GL_SOURCE7_ALPHA_ARB"	},
	{	0x858F,	"GL_SOURCE7_ALPHA_EXT"	},
	{	0x858F,	"GL_SOURCE7_ALPHA"	},
	{	0x858F,	"GL_SRC7_ALPHA"	},
	{	0x8590,	"GL_OPERAND0_RGB_ARB"	},
	{	0x8590,	"GL_OPERAND0_RGB_EXT"	},
	{	0x8590,	"GL_OPERAND0_RGB"	},
	{	0x8591,	"GL_OPERAND1_RGB_ARB"	},
	{	0x8591,	"GL_OPERAND1_RGB_EXT"	},
	{	0x8591,	"GL_OPERAND1_RGB"	},
	{	0x8592,	"GL_OPERAND2_RGB_ARB"	},
	{	0x8592,	"GL_OPERAND2_RGB_EXT"	},
	{	0x8592,	"GL_OPERAND2_RGB"	},
	{	0x8593,	"GL_OPERAND3_RGB_ARB"	},
	{	0x8593,	"GL_OPERAND3_RGB_EXT"	},
	{	0x8593,	"GL_OPERAND3_RGB"	},
	{	0x8594,	"GL_OPERAND4_RGB_ARB"	},
	{	0x8594,	"GL_OPERAND4_RGB_EXT"	},
	{	0x8594,	"GL_OPERAND4_RGB"	},
	{	0x8595,	"GL_OPERAND5_RGB_ARB"	},
	{	0x8595,	"GL_OPERAND5_RGB_EXT"	},
	{	0x8595,	"GL_OPERAND5_RGB"	},
	{	0x8596,	"GL_OPERAND6_RGB_ARB"	},
	{	0x8596,	"GL_OPERAND6_RGB_EXT"	},
	{	0x8596,	"GL_OPERAND6_RGB"	},
	{	0x8597,	"GL_OPERAND7_RGB_ARB"	},
	{	0x8597,	"GL_OPERAND7_RGB_EXT"	},
	{	0x8597,	"GL_OPERAND7_RGB"	},
	{	0x8598,	"GL_OPERAND0_ALPHA_ARB"	},
	{	0x8598,	"GL_OPERAND0_ALPHA_EXT"	},
	{	0x8598,	"GL_OPERAND0_ALPHA"	},
	{	0x8599,	"GL_OPERAND1_ALPHA_ARB"	},
	{	0x8599,	"GL_OPERAND1_ALPHA_EXT"	},
	{	0x8599,	"GL_OPERAND1_ALPHA"	},
	{	0x859A,	"GL_OPERAND2_ALPHA_ARB"	},
	{	0x859A,	"GL_OPERAND2_ALPHA_EXT"	},
	{	0x859A,	"GL_OPERAND2_ALPHA"	},
	{	0x859B,	"GL_OPERAND3_ALPHA_ARB"	},
	{	0x859B,	"GL_OPERAND3_ALPHA_EXT"	},
	{	0x859B,	"GL_OPERAND3_ALPHA"	},
	{	0x859C,	"GL_OPERAND4_ALPHA_ARB"	},
	{	0x859C,	"GL_OPERAND4_ALPHA_EXT"	},
	{	0x859C,	"GL_OPERAND4_ALPHA"	},
	{	0x859D,	"GL_OPERAND5_ALPHA_ARB"	},
	{	0x859D,	"GL_OPERAND5_ALPHA_EXT"	},
	{	0x859D,	"GL_OPERAND5_ALPHA"	},
	{	0x859E,	"GL_OPERAND6_ALPHA_ARB"	},
	{	0x859E,	"GL_OPERAND6_ALPHA_EXT"	},
	{	0x859E,	"GL_OPERAND6_ALPHA"	},
	{	0x859F,	"GL_OPERAND7_ALPHA_ARB"	},
	{	0x859F,	"GL_OPERAND7_ALPHA_EXT"	},
	{	0x859F,	"GL_OPERAND7_ALPHA"	},
	{	0x85AE,	"GL_PERTURB_EXT"	},
	{	0x85AF,	"GL_TEXTURE_NORMAL_EXT"	},
	{	0x85B4,	"GL_STORAGE_CLIENT_APPLE"	},
	{	0x85B5,	"GL_VERTEX_ARRAY_BINDING_APPLE"	},
	{	0x85BD,	"GL_STORAGE_PRIVATE_APPLE"	},
	{	0x85BE,	"GL_STORAGE_CACHED_APPLE"	},
	{	0x85BF,	"GL_STORAGE_SHARED_APPLE"	},
	{	0x8620,	"GL_VERTEX_PROGRAM_ARB"	},
	{	0x8620,	"GL_VERTEX_PROGRAM_NV"	},
	{	0x8621,	"GL_VERTEX_STATE_PROGRAM_NV"	},
	{	0x8622,	"GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB"	},
	{	0x8622,	"GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB"	},
	{	0x8622,	"GL_VERTEX_ATTRIB_ARRAY_ENABLED"	},
	{	0x8623,	"GL_ATTRIB_ARRAY_SIZE_NV"	},
	{	0x8623,	"GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB"	},
	{	0x8623,	"GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB"	},
	{	0x8623,	"GL_VERTEX_ATTRIB_ARRAY_SIZE"	},
	{	0x8624,	"GL_ATTRIB_ARRAY_STRIDE_NV"	},
	{	0x8624,	"GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB"	},
	{	0x8624,	"GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB"	},
	{	0x8624,	"GL_VERTEX_ATTRIB_ARRAY_STRIDE"	},
	{	0x8625,	"GL_ATTRIB_ARRAY_TYPE_NV"	},
	{	0x8625,	"GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB"	},
	{	0x8625,	"GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB"	},
	{	0x8625,	"GL_VERTEX_ATTRIB_ARRAY_TYPE"	},
	{	0x8626,	"GL_CURRENT_ATTRIB_NV"	},
	{	0x8626,	"GL_CURRENT_VERTEX_ATTRIB_ARB"	},
	{	0x8626,	"GL_CURRENT_VERTEX_ATTRIB_ARB"	},
	{	0x8626,	"GL_CURRENT_VERTEX_ATTRIB"	},
	{	0x8627,	"GL_PROGRAM_LENGTH_ARB"	},
	{	0x8627,	"GL_PROGRAM_LENGTH_NV"	},
	{	0x8628,	"GL_PROGRAM_STRING_ARB"	},
	{	0x8628,	"GL_PROGRAM_STRING_NV"	},
	{	0x8629,	"GL_MODELVIEW_PROJECTION_NV"	},
	{	0x862A,	"GL_IDENTITY_NV"	},
	{	0x862B,	"GL_INVERSE_NV"	},
	{	0x862C,	"GL_TRANSPOSE_NV"	},
	{	0x862D,	"GL_INVERSE_TRANSPOSE_NV"	},
	{	0x862E,	"GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB"	},
	{	0x862E,	"GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV"	},
	{	0x862F,	"GL_MAX_PROGRAM_MATRICES_ARB"	},
	{	0x862F,	"GL_MAX_TRACK_MATRICES_NV"	},
	{	0x8630,	"GL_MATRIX0_NV"	},
	{	0x8631,	"GL_MATRIX1_NV"	},
	{	0x8632,	"GL_MATRIX2_NV"	},
	{	0x8633,	"GL_MATRIX3_NV"	},
	{	0x8634,	"GL_MATRIX4_NV"	},
	{	0x8635,	"GL_MATRIX5_NV"	},
	{	0x8636,	"GL_MATRIX6_NV"	},
	{	0x8637,	"GL_MATRIX7_NV"	},
	{	0x8640,	"GL_CURRENT_MATRIX_STACK_DEPTH_ARB"	},
	{	0x8640,	"GL_CURRENT_MATRIX_STACK_DEPTH_NV"	},
	{	0x8641,	"GL_CURRENT_MATRIX_ARB"	},
	{	0x8641,	"GL_CURRENT_MATRIX_NV"	},
	{	0x8642,	"GL_PROGRAM_POINT_SIZE_EXT"	},
	{	0x8642,	"GL_VERTEX_PROGRAM_POINT_SIZE_ARB"	},
	{	0x8642,	"GL_VERTEX_PROGRAM_POINT_SIZE_ARB"	},
	{	0x8642,	"GL_VERTEX_PROGRAM_POINT_SIZE_NV"	},
	{	0x8642,	"GL_VERTEX_PROGRAM_POINT_SIZE"	},
	{	0x8643,	"GL_VERTEX_PROGRAM_TWO_SIDE_ARB"	},
	{	0x8643,	"GL_VERTEX_PROGRAM_TWO_SIDE_ARB"	},
	{	0x8643,	"GL_VERTEX_PROGRAM_TWO_SIDE_NV"	},
	{	0x8643,	"GL_VERTEX_PROGRAM_TWO_SIDE"	},
	{	0x8644,	"GL_PROGRAM_PARAMETER_NV"	},
	{	0x8645,	"GL_ATTRIB_ARRAY_POINTER_NV"	},
	{	0x8645,	"GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB"	},
	{	0x8645,	"GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB"	},
	{	0x8645,	"GL_VERTEX_ATTRIB_ARRAY_POINTER"	},
	{	0x8646,	"GL_PROGRAM_TARGET_NV"	},
	{	0x8647,	"GL_PROGRAM_RESIDENT_NV"	},
	{	0x8648,	"GL_TRACK_MATRIX_NV"	},
	{	0x8649,	"GL_TRACK_MATRIX_TRANSFORM_NV"	},
	{	0x864A,	"GL_VERTEX_PROGRAM_BINDING_NV"	},
	{	0x864B,	"GL_PROGRAM_ERROR_POSITION_ARB"	},
	{	0x864B,	"GL_PROGRAM_ERROR_POSITION_NV"	},
	{	0x8650,	"GL_VERTEX_ATTRIB_ARRAY0_NV"	},
	{	0x8651,	"GL_VERTEX_ATTRIB_ARRAY1_NV"	},
	{	0x8652,	"GL_VERTEX_ATTRIB_ARRAY2_NV"	},
	{	0x8653,	"GL_VERTEX_ATTRIB_ARRAY3_NV"	},
	{	0x8654,	"GL_VERTEX_ATTRIB_ARRAY4_NV"	},
	{	0x8655,	"GL_VERTEX_ATTRIB_ARRAY5_NV"	},
	{	0x8656,	"GL_VERTEX_ATTRIB_ARRAY6_NV"	},
	{	0x8657,	"GL_VERTEX_ATTRIB_ARRAY7_NV"	},
	{	0x8658,	"GL_VERTEX_ATTRIB_ARRAY8_NV"	},
	{	0x8659,	"GL_VERTEX_ATTRIB_ARRAY9_NV"	},
	{	0x865A,	"GL_VERTEX_ATTRIB_ARRAY10_NV"	},
	{	0x865B,	"GL_VERTEX_ATTRIB_ARRAY11_NV"	},
	{	0x865C,	"GL_VERTEX_ATTRIB_ARRAY12_NV"	},
	{	0x865D,	"GL_VERTEX_ATTRIB_ARRAY13_NV"	},
	{	0x865E,	"GL_VERTEX_ATTRIB_ARRAY14_NV"	},
	{	0x865F,	"GL_VERTEX_ATTRIB_ARRAY15_NV"	},
	{	0x8660,	"GL_MAP1_VERTEX_ATTRIB0_4_NV"	},
	{	0x8661,	"GL_MAP1_VERTEX_ATTRIB1_4_NV"	},
	{	0x8662,	"GL_MAP1_VERTEX_ATTRIB2_4_NV"	},
	{	0x8663,	"GL_MAP1_VERTEX_ATTRIB3_4_NV"	},
	{	0x8664,	"GL_MAP1_VERTEX_ATTRIB4_4_NV"	},
	{	0x8665,	"GL_MAP1_VERTEX_ATTRIB5_4_NV"	},
	{	0x8666,	"GL_MAP1_VERTEX_ATTRIB6_4_NV"	},
	{	0x8667,	"GL_MAP1_VERTEX_ATTRIB7_4_NV"	},
	{	0x8668,	"GL_MAP1_VERTEX_ATTRIB8_4_NV"	},
	{	0x8669,	"GL_MAP1_VERTEX_ATTRIB9_4_NV"	},
	{	0x866A,	"GL_MAP1_VERTEX_ATTRIB10_4_NV"	},
	{	0x866B,	"GL_MAP1_VERTEX_ATTRIB11_4_NV"	},
	{	0x866C,	"GL_MAP1_VERTEX_ATTRIB12_4_NV"	},
	{	0x866D,	"GL_MAP1_VERTEX_ATTRIB13_4_NV"	},
	{	0x866E,	"GL_MAP1_VERTEX_ATTRIB14_4_NV"	},
	{	0x866F,	"GL_MAP1_VERTEX_ATTRIB15_4_NV"	},
	{	0x8670,	"GL_MAP2_VERTEX_ATTRIB0_4_NV"	},
	{	0x8671,	"GL_MAP2_VERTEX_ATTRIB1_4_NV"	},
	{	0x8672,	"GL_MAP2_VERTEX_ATTRIB2_4_NV"	},
	{	0x8673,	"GL_MAP2_VERTEX_ATTRIB3_4_NV"	},
	{	0x8674,	"GL_MAP2_VERTEX_ATTRIB4_4_NV"	},
	{	0x8675,	"GL_MAP2_VERTEX_ATTRIB5_4_NV"	},
	{	0x8676,	"GL_MAP2_VERTEX_ATTRIB6_4_NV"	},
	{	0x8677,	"GL_MAP2_VERTEX_ATTRIB7_4_NV"	},
	{	0x8677,	"GL_PROGRAM_BINDING_ARB"	},
	{	0x8677,	"GL_PROGRAM_NAME_ARB"	},
	{	0x8678,	"GL_MAP2_VERTEX_ATTRIB8_4_NV"	},
	{	0x8679,	"GL_MAP2_VERTEX_ATTRIB9_4_NV"	},
	{	0x867A,	"GL_MAP2_VERTEX_ATTRIB10_4_NV"	},
	{	0x867B,	"GL_MAP2_VERTEX_ATTRIB11_4_NV"	},
	{	0x867C,	"GL_MAP2_VERTEX_ATTRIB12_4_NV"	},
	{	0x867D,	"GL_MAP2_VERTEX_ATTRIB13_4_NV"	},
	{	0x867E,	"GL_MAP2_VERTEX_ATTRIB14_4_NV"	},
	{	0x867F,	"GL_MAP2_VERTEX_ATTRIB15_4_NV"	},
	{	0x86A0,	"GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB"	},
	{	0x86A0,	"GL_TEXTURE_COMPRESSED_IMAGE_SIZE"	},
	{	0x86A1,	"GL_TEXTURE_COMPRESSED_ARB"	},
	{	0x86A1,	"GL_TEXTURE_COMPRESSED"	},
	{	0x86A2,	"GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB"	},
	{	0x86A2,	"GL_NUM_COMPRESSED_TEXTURE_FORMATS"	},
	{	0x86A3,	"GL_COMPRESSED_TEXTURE_FORMATS_ARB"	},
	{	0x86A3,	"GL_COMPRESSED_TEXTURE_FORMATS"	},
	{	0x86A4,	"GL_MAX_VERTEX_UNITS_ARB"	},
	{	0x86A5,	"GL_ACTIVE_VERTEX_UNITS_ARB"	},
	{	0x86A6,	"GL_WEIGHT_SUM_UNITY_ARB"	},
	{	0x86A7,	"GL_VERTEX_BLEND_ARB"	},
	{	0x86A8,	"GL_CURRENT_WEIGHT_ARB"	},
	{	0x86A9,	"GL_WEIGHT_ARRAY_TYPE_ARB"	},
	{	0x86AA,	"GL_WEIGHT_ARRAY_STRIDE_ARB"	},
	{	0x86AB,	"GL_WEIGHT_ARRAY_SIZE_ARB"	},
	{	0x86AC,	"GL_WEIGHT_ARRAY_POINTER_ARB"	},
	{	0x86AD,	"GL_WEIGHT_ARRAY_ARB"	},
	{	0x86AE,	"GL_DOT3_RGB_ARB"	},
	{	0x86AE,	"GL_DOT3_RGB"	},
	{	0x86AF,	"GL_DOT3_RGBA_ARB"	},
	{	0x86AF,	"GL_DOT3_RGBA"	},
	{	0x8722,	"GL_MODELVIEW2_ARB"	},
	{	0x8723,	"GL_MODELVIEW3_ARB"	},
	{	0x8724,	"GL_MODELVIEW4_ARB"	},
	{	0x8725,	"GL_MODELVIEW5_ARB"	},
	{	0x8726,	"GL_MODELVIEW6_ARB"	},
	{	0x8727,	"GL_MODELVIEW7_ARB"	},
	{	0x8728,	"GL_MODELVIEW8_ARB"	},
	{	0x8729,	"GL_MODELVIEW9_ARB"	},
	{	0x872A,	"GL_MODELVIEW10_ARB"	},
	{	0x872B,	"GL_MODELVIEW11_ARB"	},
	{	0x872C,	"GL_MODELVIEW12_ARB"	},
	{	0x872D,	"GL_MODELVIEW13_ARB"	},
	{	0x872E,	"GL_MODELVIEW14_ARB"	},
	{	0x872F,	"GL_MODELVIEW15_ARB"	},
	{	0x8730,	"GL_MODELVIEW16_ARB"	},
	{	0x8731,	"GL_MODELVIEW17_ARB"	},
	{	0x8732,	"GL_MODELVIEW18_ARB"	},
	{	0x8733,	"GL_MODELVIEW19_ARB"	},
	{	0x8734,	"GL_MODELVIEW20_ARB"	},
	{	0x8735,	"GL_MODELVIEW21_ARB"	},
	{	0x8736,	"GL_MODELVIEW22_ARB"	},
	{	0x8737,	"GL_MODELVIEW23_ARB"	},
	{	0x8738,	"GL_MODELVIEW24_ARB"	},
	{	0x8739,	"GL_MODELVIEW25_ARB"	},
	{	0x873A,	"GL_MODELVIEW26_ARB"	},
	{	0x873B,	"GL_MODELVIEW27_ARB"	},
	{	0x873C,	"GL_MODELVIEW28_ARB"	},
	{	0x873D,	"GL_MODELVIEW29_ARB"	},
	{	0x873E,	"GL_MODELVIEW30_ARB"	},
	{	0x873F,	"GL_MODELVIEW31_ARB"	},
	{	0x8742,	"GL_MIRROR_CLAMP_EXT"	},
	{	0x8743,	"GL_MIRROR_CLAMP_TO_EDGE_EXT"	},
	{	0x8764,	"GL_BUFFER_SIZE_ARB"	},
	{	0x8764,	"GL_BUFFER_SIZE"	},
	{	0x8765,	"GL_BUFFER_USAGE_ARB"	},
	{	0x8765,	"GL_BUFFER_USAGE"	},
	{	0x8780,	"GL_VERTEX_SHADER_EXT"	},
	{	0x8781,	"GL_VERTEX_SHADER_BINDING_EXT"	},
	{	0x8782,	"GL_OP_INDEX_EXT"	},
	{	0x8783,	"GL_OP_NEGATE_EXT"	},
	{	0x8784,	"GL_OP_DOT3_EXT"	},
	{	0x8785,	"GL_OP_DOT4_EXT"	},
	{	0x8786,	"GL_OP_MUL_EXT"	},
	{	0x8787,	"GL_OP_ADD_EXT"	},
	{	0x8788,	"GL_OP_MADD_EXT"	},
	{	0x8789,	"GL_OP_FRAC_EXT"	},
	{	0x878A,	"GL_OP_MAX_EXT"	},
	{	0x878B,	"GL_OP_MIN_EXT"	},
	{	0x878C,	"GL_OP_SET_GE_EXT"	},
	{	0x878D,	"GL_OP_SET_LT_EXT"	},
	{	0x878E,	"GL_OP_CLAMP_EXT"	},
	{	0x878F,	"GL_OP_FLOOR_EXT"	},
	{	0x8790,	"GL_OP_ROUND_EXT"	},
	{	0x8791,	"GL_OP_EXP_BASE_2_EXT"	},
	{	0x8792,	"GL_OP_LOG_BASE_2_EXT"	},
	{	0x8793,	"GL_OP_POWER_EXT"	},
	{	0x8794,	"GL_OP_RECIP_EXT"	},
	{	0x8795,	"GL_OP_RECIP_SQRT_EXT"	},
	{	0x8796,	"GL_OP_SUB_EXT"	},
	{	0x8797,	"GL_OP_CROSS_PRODUCT_EXT"	},
	{	0x8798,	"GL_OP_MULTIPLY_MATRIX_EXT"	},
	{	0x8799,	"GL_OP_MOV_EXT"	},
	{	0x879A,	"GL_OUTPUT_VERTEX_EXT"	},
	{	0x879B,	"GL_OUTPUT_COLOR0_EXT"	},
	{	0x879C,	"GL_OUTPUT_COLOR1_EXT"	},
	{	0x879D,	"GL_OUTPUT_TEXTURE_COORD0_EXT"	},
	{	0x879E,	"GL_OUTPUT_TEXTURE_COORD1_EXT"	},
	{	0x879F,	"GL_OUTPUT_TEXTURE_COORD2_EXT"	},
	{	0x87A0,	"GL_OUTPUT_TEXTURE_COORD3_EXT"	},
	{	0x87A1,	"GL_OUTPUT_TEXTURE_COORD4_EXT"	},
	{	0x87A2,	"GL_OUTPUT_TEXTURE_COORD5_EXT"	},
	{	0x87A3,	"GL_OUTPUT_TEXTURE_COORD6_EXT"	},
	{	0x87A4,	"GL_OUTPUT_TEXTURE_COORD7_EXT"	},
	{	0x87A5,	"GL_OUTPUT_TEXTURE_COORD8_EXT"	},
	{	0x87A6,	"GL_OUTPUT_TEXTURE_COORD9_EXT"	},
	{	0x87A7,	"GL_OUTPUT_TEXTURE_COORD10_EXT"	},
	{	0x87A8,	"GL_OUTPUT_TEXTURE_COORD11_EXT"	},
	{	0x87A9,	"GL_OUTPUT_TEXTURE_COORD12_EXT"	},
	{	0x87AA,	"GL_OUTPUT_TEXTURE_COORD13_EXT"	},
	{	0x87AB,	"GL_OUTPUT_TEXTURE_COORD14_EXT"	},
	{	0x87AC,	"GL_OUTPUT_TEXTURE_COORD15_EXT"	},
	{	0x87AD,	"GL_OUTPUT_TEXTURE_COORD16_EXT"	},
	{	0x87AE,	"GL_OUTPUT_TEXTURE_COORD17_EXT"	},
	{	0x87AF,	"GL_OUTPUT_TEXTURE_COORD18_EXT"	},
	{	0x87B0,	"GL_OUTPUT_TEXTURE_COORD19_EXT"	},
	{	0x87B1,	"GL_OUTPUT_TEXTURE_COORD20_EXT"	},
	{	0x87B2,	"GL_OUTPUT_TEXTURE_COORD21_EXT"	},
	{	0x87B3,	"GL_OUTPUT_TEXTURE_COORD22_EXT"	},
	{	0x87B4,	"GL_OUTPUT_TEXTURE_COORD23_EXT"	},
	{	0x87B5,	"GL_OUTPUT_TEXTURE_COORD24_EXT"	},
	{	0x87B6,	"GL_OUTPUT_TEXTURE_COORD25_EXT"	},
	{	0x87B7,	"GL_OUTPUT_TEXTURE_COORD26_EXT"	},
	{	0x87B8,	"GL_OUTPUT_TEXTURE_COORD27_EXT"	},
	{	0x87B9,	"GL_OUTPUT_TEXTURE_COORD28_EXT"	},
	{	0x87BA,	"GL_OUTPUT_TEXTURE_COORD29_EXT"	},
	{	0x87BB,	"GL_OUTPUT_TEXTURE_COORD30_EXT"	},
	{	0x87BC,	"GL_OUTPUT_TEXTURE_COORD31_EXT"	},
	{	0x87BD,	"GL_OUTPUT_FOG_EXT"	},
	{	0x87BE,	"GL_SCALAR_EXT"	},
	{	0x87BF,	"GL_VECTOR_EXT"	},
	{	0x87C0,	"GL_MATRIX_EXT"	},
	{	0x87C1,	"GL_VARIANT_EXT"	},
	{	0x87C2,	"GL_INVARIANT_EXT"	},
	{	0x87C3,	"GL_LOCAL_CONSTANT_EXT"	},
	{	0x87C4,	"GL_LOCAL_EXT"	},
	{	0x87C5,	"GL_MAX_VERTEX_SHADER_INSTRUCTIONS_EXT"	},
	{	0x87C6,	"GL_MAX_VERTEX_SHADER_VARIANTS_EXT"	},
	{	0x87C7,	"GL_MAX_VERTEX_SHADER_INVARIANTS_EXT"	},
	{	0x87C8,	"GL_MAX_VERTEX_SHADER_LOCAL_CONSTANTS_EXT"	},
	{	0x87C9,	"GL_MAX_VERTEX_SHADER_LOCALS_EXT"	},
	{	0x87CA,	"GL_MAX_OPTIMIZED_VERTEX_SHADER_INSTRUCTIONS_EXT"	},
	{	0x87CB,	"GL_MAX_OPTIMIZED_VERTEX_SHADER_VARIANTS_EXT"	},
	{	0x87CC,	"GL_MAX_OPTIMIZED_VERTEX_SHADER_LOCAL_CONSTANTS_EXT"	},
	{	0x87CD,	"GL_MAX_OPTIMIZED_VERTEX_SHADER_INVARIANTS_EXT"	},
	{	0x87CE,	"GL_MAX_OPTIMIZED_VERTEX_SHADER_LOCALS_EXT"	},
	{	0x87CF,	"GL_VERTEX_SHADER_INSTRUCTIONS_EXT"	},
	{	0x87D0,	"GL_VERTEX_SHADER_VARIANTS_EXT"	},
	{	0x87D1,	"GL_VERTEX_SHADER_INVARIANTS_EXT"	},
	{	0x87D2,	"GL_VERTEX_SHADER_LOCAL_CONSTANTS_EXT"	},
	{	0x87D3,	"GL_VERTEX_SHADER_LOCALS_EXT"	},
	{	0x87D4,	"GL_VERTEX_SHADER_OPTIMIZED_EXT"	},
	{	0x87D5,	"GL_X_EXT"	},
	{	0x87D6,	"GL_Y_EXT"	},
	{	0x87D7,	"GL_Z_EXT"	},
	{	0x87D8,	"GL_W_EXT"	},
	{	0x87D9,	"GL_NEGATIVE_X_EXT"	},
	{	0x87DA,	"GL_NEGATIVE_Y_EXT"	},
	{	0x87DB,	"GL_NEGATIVE_Z_EXT"	},
	{	0x87DC,	"GL_NEGATIVE_W_EXT"	},
	{	0x87DF,	"GL_NEGATIVE_ONE_EXT"	},
	{	0x87E0,	"GL_NORMALIZED_RANGE_EXT"	},
	{	0x87E1,	"GL_FULL_RANGE_EXT"	},
	{	0x87E2,	"GL_CURRENT_VERTEX_EXT"	},
	{	0x87E3,	"GL_MVP_MATRIX_EXT"	},
	{	0x87E4,	"GL_VARIANT_VALUE_EXT"	},
	{	0x87E5,	"GL_VARIANT_DATATYPE_EXT"	},
	{	0x87E6,	"GL_VARIANT_ARRAY_STRIDE_EXT"	},
	{	0x87E7,	"GL_VARIANT_ARRAY_TYPE_EXT"	},
	{	0x87E8,	"GL_VARIANT_ARRAY_EXT"	},
	{	0x87E9,	"GL_VARIANT_ARRAY_POINTER_EXT"	},
	{	0x87EA,	"GL_INVARIANT_VALUE_EXT"	},
	{	0x87EB,	"GL_INVARIANT_DATATYPE_EXT"	},
	{	0x87EC,	"GL_LOCAL_CONSTANT_VALUE_EXT"	},
	{	0x87Ed,	"GL_LOCAL_CONSTANT_DATATYPE_EXT"	},
	{	0x8800,	"GL_STENCIL_BACK_FUNC_ATI"	},
	{	0x8800,	"GL_STENCIL_BACK_FUNC"	},
	{	0x8801,	"GL_STENCIL_BACK_FAIL_ATI"	},
	{	0x8801,	"GL_STENCIL_BACK_FAIL"	},
	{	0x8802,	"GL_STENCIL_BACK_PASS_DEPTH_FAIL_ATI"	},
	{	0x8802,	"GL_STENCIL_BACK_PASS_DEPTH_FAIL"	},
	{	0x8803,	"GL_STENCIL_BACK_PASS_DEPTH_PASS_ATI"	},
	{	0x8803,	"GL_STENCIL_BACK_PASS_DEPTH_PASS"	},
	{	0x8804,	"GL_FRAGMENT_PROGRAM_ARB"	},
	{	0x8805,	"GL_PROGRAM_ALU_INSTRUCTIONS_ARB"	},
	{	0x8806,	"GL_PROGRAM_TEX_INSTRUCTIONS_ARB"	},
	{	0x8807,	"GL_PROGRAM_TEX_INDIRECTIONS_ARB"	},
	{	0x8808,	"GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB"	},
	{	0x8809,	"GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB"	},
	{	0x880A,	"GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB"	},
	{	0x880B,	"GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB"	},
	{	0x880C,	"GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB"	},
	{	0x880D,	"GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB"	},
	{	0x880E,	"GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB"	},
	{	0x880F,	"GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB"	},
	{	0x8810,	"GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB"	},
	{	0x8814,	"GL_RGBA_FLOAT32_APPLE"	},
	{	0x8814,	"GL_RGBA_FLOAT32_ATI"	},
	{	0x8814,	"GL_RGBA32F_ARB"	},
	{	0x8815,	"GL_RGB_FLOAT32_APPLE"	},
	{	0x8815,	"GL_RGB_FLOAT32_ATI"	},
	{	0x8815,	"GL_RGB32F_ARB"	},
	{	0x8816,	"GL_ALPHA_FLOAT32_APPLE"	},
	{	0x8816,	"GL_ALPHA_FLOAT32_ATI"	},
	{	0x8816,	"GL_ALPHA32F_ARB"	},
	{	0x8817,	"GL_INTENSITY_FLOAT32_APPLE"	},
	{	0x8817,	"GL_INTENSITY_FLOAT32_ATI"	},
	{	0x8817,	"GL_INTENSITY32F_ARB"	},
	{	0x8818,	"GL_LUMINANCE_FLOAT32_APPLE"	},
	{	0x8818,	"GL_LUMINANCE_FLOAT32_ATI"	},
	{	0x8818,	"GL_LUMINANCE32F_ARB"	},
	{	0x8819,	"GL_LUMINANCE_ALPHA_FLOAT32_APPLE"	},
	{	0x8819,	"GL_LUMINANCE_ALPHA_FLOAT32_ATI"	},
	{	0x8819,	"GL_LUMINANCE_ALPHA32F_ARB"	},
	{	0x881A,	"GL_RGBA_FLOAT16_APPLE"	},
	{	0x881A,	"GL_RGBA_FLOAT16_ATI"	},
	{	0x881A,	"GL_RGBA16F_ARB"	},
	{	0x881B,	"GL_RGB_FLOAT16_APPLE"	},
	{	0x881B,	"GL_RGB_FLOAT16_ATI"	},
	{	0x881B,	"GL_RGB16F_ARB"	},
	{	0x881C,	"GL_ALPHA_FLOAT16_APPLE"	},
	{	0x881C,	"GL_ALPHA_FLOAT16_ATI"	},
	{	0x881C,	"GL_ALPHA16F_ARB"	},
	{	0x881D,	"GL_INTENSITY_FLOAT16_APPLE"	},
	{	0x881D,	"GL_INTENSITY_FLOAT16_ATI"	},
	{	0x881D,	"GL_INTENSITY16F_ARB"	},
	{	0x881E,	"GL_LUMINANCE_FLOAT16_APPLE"	},
	{	0x881E,	"GL_LUMINANCE_FLOAT16_ATI"	},
	{	0x881E,	"GL_LUMINANCE16F_ARB"	},
	{	0x881F,	"GL_LUMINANCE_ALPHA_FLOAT16_APPLE"	},
	{	0x881F,	"GL_LUMINANCE_ALPHA_FLOAT16_ATI"	},
	{	0x881F,	"GL_LUMINANCE_ALPHA16F_ARB"	},
	{	0x8820,	"GL_RGBA_FLOAT_MODE_ARB"	},
	{	0x8824,	"GL_MAX_DRAW_BUFFERS_ARB"	},
	{	0x8824,	"GL_MAX_DRAW_BUFFERS"	},
	{	0x8825,	"GL_DRAW_BUFFER0_ARB"	},
	{	0x8825,	"GL_DRAW_BUFFER0"	},
	{	0x8826,	"GL_DRAW_BUFFER1_ARB"	},
	{	0x8826,	"GL_DRAW_BUFFER1"	},
	{	0x8827,	"GL_DRAW_BUFFER2_ARB"	},
	{	0x8827,	"GL_DRAW_BUFFER2"	},
	{	0x8828,	"GL_DRAW_BUFFER3_ARB"	},
	{	0x8828,	"GL_DRAW_BUFFER3"	},
	{	0x8829,	"GL_DRAW_BUFFER4_ARB"	},
	{	0x8829,	"GL_DRAW_BUFFER4"	},
	{	0x882A,	"GL_DRAW_BUFFER5_ARB"	},
	{	0x882A,	"GL_DRAW_BUFFER5"	},
	{	0x882B,	"GL_DRAW_BUFFER6_ARB"	},
	{	0x882B,	"GL_DRAW_BUFFER6"	},
	{	0x882C,	"GL_DRAW_BUFFER7_ARB"	},
	{	0x882C,	"GL_DRAW_BUFFER7"	},
	{	0x882D,	"GL_DRAW_BUFFER8_ARB"	},
	{	0x882D,	"GL_DRAW_BUFFER8"	},
	{	0x882E,	"GL_DRAW_BUFFER9_ARB"	},
	{	0x882E,	"GL_DRAW_BUFFER9"	},
	{	0x882F,	"GL_DRAW_BUFFER10_ARB"	},
	{	0x882F,	"GL_DRAW_BUFFER10"	},
	{	0x8830,	"GL_DRAW_BUFFER11_ARB"	},
	{	0x8830,	"GL_DRAW_BUFFER11"	},
	{	0x8831,	"GL_DRAW_BUFFER12_ARB"	},
	{	0x8831,	"GL_DRAW_BUFFER12"	},
	{	0x8832,	"GL_DRAW_BUFFER13_ARB"	},
	{	0x8832,	"GL_DRAW_BUFFER13"	},
	{	0x8833,	"GL_DRAW_BUFFER14_ARB"	},
	{	0x8833,	"GL_DRAW_BUFFER14"	},
	{	0x8834,	"GL_DRAW_BUFFER15_ARB"	},
	{	0x8834,	"GL_DRAW_BUFFER15"	},
	{	0x883D,	"GL_ALPHA_BLEND_EQUATION_ATI"	},
	{	0x883D,	"GL_BLEND_EQUATION_ALPHA_EXT"	},
	{	0x883D,	"GL_BLEND_EQUATION_ALPHA"	},
	{	0x884A,	"GL_TEXTURE_DEPTH_SIZE_ARB"	},
	{	0x884A,	"GL_TEXTURE_DEPTH_SIZE"	},
	{	0x884B,	"GL_DEPTH_TEXTURE_MODE_ARB"	},
	{	0x884B,	"GL_DEPTH_TEXTURE_MODE"	},
	{	0x884C,	"GL_TEXTURE_COMPARE_MODE_ARB"	},
	{	0x884C,	"GL_TEXTURE_COMPARE_MODE"	},
	{	0x884D,	"GL_TEXTURE_COMPARE_FUNC_ARB"	},
	{	0x884D,	"GL_TEXTURE_COMPARE_FUNC"	},
	{	0x884E,	"GL_COMPARE_R_TO_TEXTURE_ARB"	},
	{	0x884E,	"GL_COMPARE_R_TO_TEXTURE"	},
	{	0x884E,	"GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT"	},
	{	0x8861,	"GL_POINT_SPRITE_ARB"	},
	{	0x8861,	"GL_POINT_SPRITE"	},
	{	0x8862,	"GL_COORD_REPLACE_ARB"	},
	{	0x8862,	"GL_COORD_REPLACE"	},
	{	0x8864,	"GL_QUERY_COUNTER_BITS_ARB"	},
	{	0x8864,	"GL_QUERY_COUNTER_BITS"	},
	{	0x8865,	"GL_CURRENT_QUERY_ARB"	},
	{	0x8865,	"GL_CURRENT_QUERY"	},
	{	0x8866,	"GL_QUERY_RESULT_ARB"	},
	{	0x8866,	"GL_QUERY_RESULT"	},
	{	0x8867,	"GL_QUERY_RESULT_AVAILABLE_ARB"	},
	{	0x8867,	"GL_QUERY_RESULT_AVAILABLE"	},
	{	0x8869,	"GL_MAX_VERTEX_ATTRIBS_ARB"	},
	{	0x8869,	"GL_MAX_VERTEX_ATTRIBS_ARB"	},
	{	0x8869,	"GL_MAX_VERTEX_ATTRIBS"	},
	{	0x886A,	"GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB"	},
	{	0x886A,	"GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB"	},
	{	0x886A,	"GL_VERTEX_ATTRIB_ARRAY_NORMALIZED"	},
	{	0x8871,	"GL_MAX_TEXTURE_COORDS_ARB"	},
	{	0x8871,	"GL_MAX_TEXTURE_COORDS_ARB"	},
	{	0x8871,	"GL_MAX_TEXTURE_COORDS_ARB"	},
	{	0x8871,	"GL_MAX_TEXTURE_COORDS"	},
	{	0x8872,	"GL_MAX_TEXTURE_IMAGE_UNITS_ARB"	},
	{	0x8872,	"GL_MAX_TEXTURE_IMAGE_UNITS_ARB"	},
	{	0x8872,	"GL_MAX_TEXTURE_IMAGE_UNITS_ARB"	},
	{	0x8872,	"GL_MAX_TEXTURE_IMAGE_UNITS"	},
	{	0x8874,	"GL_PROGRAM_ERROR_STRING_ARB"	},
	{	0x8875,	"GL_PROGRAM_FORMAT_ASCII_ARB"	},
	{	0x8876,	"GL_PROGRAM_FORMAT_ARB"	},
	{	0x8890,	"GL_DEPTH_BOUNDS_TEST_EXT"	},
	{	0x8891,	"GL_DEPTH_BOUNDS_EXT"	},
	{	0x8892,	"GL_ARRAY_BUFFER_ARB"	},
	{	0x8892,	"GL_ARRAY_BUFFER"	},
	{	0x8893,	"GL_ELEMENT_ARRAY_BUFFER_ARB"	},
	{	0x8893,	"GL_ELEMENT_ARRAY_BUFFER"	},
	{	0x8894,	"GL_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x8894,	"GL_ARRAY_BUFFER_BINDING"	},
	{	0x8895,	"GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x8895,	"GL_ELEMENT_ARRAY_BUFFER_BINDING"	},
	{	0x8896,	"GL_VERTEX_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x8896,	"GL_VERTEX_ARRAY_BUFFER_BINDING"	},
	{	0x8897,	"GL_NORMAL_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x8897,	"GL_NORMAL_ARRAY_BUFFER_BINDING"	},
	{	0x8898,	"GL_COLOR_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x8898,	"GL_COLOR_ARRAY_BUFFER_BINDING"	},
	{	0x8899,	"GL_INDEX_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x8899,	"GL_INDEX_ARRAY_BUFFER_BINDING"	},
	{	0x889A,	"GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x889A,	"GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING"	},
	{	0x889B,	"GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x889B,	"GL_EDGE_FLAG_ARRAY_BUFFER_BINDING"	},
	{	0x889C,	"GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x889C,	"GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING"	},
	{	0x889D,	"GL_FOG_COORD_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x889D,	"GL_FOG_COORD_ARRAY_BUFFER_BINDING"	},
	{	0x889D,	"GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x889D,	"GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING"	},
	{	0x889E,	"GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x889E,	"GL_WEIGHT_ARRAY_BUFFER_BINDING"	},
	{	0x889F,	"GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB"	},
	{	0x889F,	"GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING"	},
	{	0x88A0,	"GL_PROGRAM_INSTRUCTIONS_ARB"	},
	{	0x88A1,	"GL_MAX_PROGRAM_INSTRUCTIONS_ARB"	},
	{	0x88A2,	"GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB"	},
	{	0x88A3,	"GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB"	},
	{	0x88A4,	"GL_PROGRAM_TEMPORARIES_ARB"	},
	{	0x88A5,	"GL_MAX_PROGRAM_TEMPORARIES_ARB"	},
	{	0x88A6,	"GL_PROGRAM_NATIVE_TEMPORARIES_ARB"	},
	{	0x88A7,	"GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB"	},
	{	0x88A8,	"GL_PROGRAM_PARAMETERS_ARB"	},
	{	0x88A9,	"GL_MAX_PROGRAM_PARAMETERS_ARB"	},
	{	0x88AA,	"GL_PROGRAM_NATIVE_PARAMETERS_ARB"	},
	{	0x88AB,	"GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB"	},
	{	0x88AC,	"GL_PROGRAM_ATTRIBS_ARB"	},
	{	0x88AD,	"GL_MAX_PROGRAM_ATTRIBS_ARB"	},
	{	0x88AE,	"GL_PROGRAM_NATIVE_ATTRIBS_ARB"	},
	{	0x88AF,	"GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB"	},
	{	0x88B0,	"GL_PROGRAM_ADDRESS_REGISTERS_ARB"	},
	{	0x88B1,	"GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB"	},
	{	0x88B2,	"GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB"	},
	{	0x88B3,	"GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB"	},
	{	0x88B4,	"GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB"	},
	{	0x88B5,	"GL_MAX_PROGRAM_ENV_PARAMETERS_ARB"	},
	{	0x88B6,	"GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB"	},
	{	0x88B7,	"GL_TRANSPOSE_CURRENT_MATRIX_ARB"	},
	{	0x88B8,	"GL_READ_ONLY_ARB"	},
	{	0x88B8,	"GL_READ_ONLY"	},
	{	0x88B9,	"GL_WRITE_ONLY_ARB"	},
	{	0x88B9,	"GL_WRITE_ONLY"	},
	{	0x88BA,	"GL_READ_WRITE_ARB"	},
	{	0x88BA,	"GL_READ_WRITE"	},
	{	0x88BB,	"GL_BUFFER_ACCESS_ARB"	},
	{	0x88BB,	"GL_BUFFER_ACCESS"	},
	{	0x88BC,	"GL_BUFFER_MAPPED_ARB"	},
	{	0x88BC,	"GL_BUFFER_MAPPED"	},
	{	0x88BD,	"GL_BUFFER_MAP_POINTER_ARB"	},
	{	0x88BD,	"GL_BUFFER_MAP_POINTER"	},
	{	0x88C0,	"GL_MATRIX0_ARB"	},
	{	0x88C1,	"GL_MATRIX1_ARB"	},
	{	0x88C2,	"GL_MATRIX2_ARB"	},
	{	0x88C3,	"GL_MATRIX3_ARB"	},
	{	0x88C4,	"GL_MATRIX4_ARB"	},
	{	0x88C5,	"GL_MATRIX5_ARB"	},
	{	0x88C6,	"GL_MATRIX6_ARB"	},
	{	0x88C7,	"GL_MATRIX7_ARB"	},
	{	0x88C8,	"GL_MATRIX8_ARB"	},
	{	0x88C9,	"GL_MATRIX9_ARB"	},
	{	0x88CA,	"GL_MATRIX10_ARB"	},
	{	0x88CB,	"GL_MATRIX11_ARB"	},
	{	0x88CC,	"GL_MATRIX12_ARB"	},
	{	0x88CD,	"GL_MATRIX13_ARB"	},
	{	0x88CE,	"GL_MATRIX14_ARB"	},
	{	0x88CF,	"GL_MATRIX15_ARB"	},
	{	0x88D0,	"GL_MATRIX16_ARB"	},
	{	0x88D1,	"GL_MATRIX17_ARB"	},
	{	0x88D2,	"GL_MATRIX18_ARB"	},
	{	0x88D3,	"GL_MATRIX19_ARB"	},
	{	0x88D4,	"GL_MATRIX20_ARB"	},
	{	0x88D5,	"GL_MATRIX21_ARB"	},
	{	0x88D6,	"GL_MATRIX22_ARB"	},
	{	0x88D7,	"GL_MATRIX23_ARB"	},
	{	0x88D8,	"GL_MATRIX24_ARB"	},
	{	0x88D9,	"GL_MATRIX25_ARB"	},
	{	0x88DA,	"GL_MATRIX26_ARB"	},
	{	0x88DB,	"GL_MATRIX27_ARB"	},
	{	0x88DC,	"GL_MATRIX28_ARB"	},
	{	0x88DD,	"GL_MATRIX29_ARB"	},
	{	0x88DE,	"GL_MATRIX30_ARB"	},
	{	0x88DF,	"GL_MATRIX31_ARB"	},
	{	0x88E0,	"GL_STREAM_DRAW_ARB"	},
	{	0x88E0,	"GL_STREAM_DRAW"	},
	{	0x88E1,	"GL_STREAM_READ_ARB"	},
	{	0x88E1,	"GL_STREAM_READ"	},
	{	0x88E2,	"GL_STREAM_COPY_ARB"	},
	{	0x88E2,	"GL_STREAM_COPY"	},
	{	0x88E4,	"GL_STATIC_DRAW_ARB"	},
	{	0x88E4,	"GL_STATIC_DRAW"	},
	{	0x88E5,	"GL_STATIC_READ_ARB"	},
	{	0x88E5,	"GL_STATIC_READ"	},
	{	0x88E6,	"GL_STATIC_COPY_ARB"	},
	{	0x88E6,	"GL_STATIC_COPY"	},
	{	0x88E8,	"GL_DYNAMIC_DRAW_ARB"	},
	{	0x88E8,	"GL_DYNAMIC_DRAW"	},
	{	0x88E9,	"GL_DYNAMIC_READ_ARB"	},
	{	0x88E9,	"GL_DYNAMIC_READ"	},
	{	0x88EA,	"GL_DYNAMIC_COPY_ARB"	},
	{	0x88EA,	"GL_DYNAMIC_COPY"	},
	{	0x88EB,	"GL_PIXEL_PACK_BUFFER_ARB"	},
	{	0x88EB,	"GL_PIXEL_PACK_BUFFER"	},
	{	0x88EC,	"GL_PIXEL_UNPACK_BUFFER_ARB"	},
	{	0x88EC,	"GL_PIXEL_UNPACK_BUFFER"	},
	{	0x88ED,	"GL_PIXEL_PACK_BUFFER_BINDING_ARB"	},
	{	0x88ED,	"GL_PIXEL_PACK_BUFFER_BINDING"	},
	{	0x88EF,	"GL_PIXEL_UNPACK_BUFFER_BINDING_ARB"	},
	{	0x88EF,	"GL_PIXEL_UNPACK_BUFFER_BINDING"	},
	{	0x88F0,	"GL_DEPTH24_STENCIL8_EXT"	},
	{	0x88F0,	"GL_DEPTH24_STENCIL8"	},
	{	0x88F1,	"GL_TEXTURE_STENCIL_SIZE_EXT"	},
	{	0x88F1,	"GL_TEXTURE_STENCIL_SIZE"	},
	{	0x88FD,	"GL_VERTEX_ATTRIB_ARRAY_INTEGER_EXT"	},
	{	0x88FE,	"GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ARB"	},
	{	0x88FF,	"GL_MAX_ARRAY_TEXTURE_LAYERS_EXT"	},
	{	0x8904,	"GL_MIN_PROGRAM_TEXEL_OFFSET_EXT"	},
	{	0x8905,	"GL_MAX_PROGRAM_TEXEL_OFFSET_EXT"	},
	{	0x8910,	"GL_STENCIL_TEST_TWO_SIDE_EXT"	},
	{	0x8911,	"GL_ACTIVE_STENCIL_FACE_EXT"	},
	{	0x8912,	"GL_MIRROR_CLAMP_TO_BORDER_EXT"	},
	{	0x8914,	"GL_SAMPLES_PASSED_ARB"	},
	{	0x8914,	"GL_SAMPLES_PASSED"	},
	{	0x891A,	"GL_CLAMP_VERTEX_COLOR_ARB"	},
	{	0x891B,	"GL_CLAMP_FRAGMENT_COLOR_ARB"	},
	{	0x891C,	"GL_CLAMP_READ_COLOR_ARB"	},
	{	0x891D,	"GL_FIXED_ONLY_ARB"	},
	{	0x8920,	"GL_FRAGMENT_SHADER_EXT"	},
	{	0x896D,	"GL_SECONDARY_INTERPOLATOR_EXT"	},
	{	0x896E,	"GL_NUM_FRAGMENT_REGISTERS_EXT"	},
	{	0x896F,	"GL_NUM_FRAGMENT_CONSTANTS_EXT"	},
	{	0x8A0C,	"GL_ELEMENT_ARRAY_APPLE"	},
	{	0x8A0D,	"GL_ELEMENT_ARRAY_TYPE_APPLE"	},
	{	0x8A0E,	"GL_ELEMENT_ARRAY_POINTER_APPLE"	},
	{	0x8A0F,	"GL_COLOR_FLOAT_APPLE"	},
	{	0x8A11,	"GL_UNIFORM_BUFFER"	},
	{	0x8A28,	"GL_UNIFORM_BUFFER_BINDING"	},
	{	0x8A29,	"GL_UNIFORM_BUFFER_START"	},
	{	0x8A2A,	"GL_UNIFORM_BUFFER_SIZE"	},
	{	0x8A2B,	"GL_MAX_VERTEX_UNIFORM_BLOCKS"	},
	{	0x8A2C,	"GL_MAX_GEOMETRY_UNIFORM_BLOCKS"	},
	{	0x8A2D,	"GL_MAX_FRAGMENT_UNIFORM_BLOCKS"	},
	{	0x8A2E,	"GL_MAX_COMBINED_UNIFORM_BLOCKS"	},
	{	0x8A2F,	"GL_MAX_UNIFORM_BUFFER_BINDINGS"	},
	{	0x8A30,	"GL_MAX_UNIFORM_BLOCK_SIZE"	},
	{	0x8A31,	"GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS"	},
	{	0x8A32,	"GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS"	},
	{	0x8A33,	"GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS"	},
	{	0x8A34,	"GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT"	},
	{	0x8A35,	"GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH"	},
	{	0x8A36,	"GL_ACTIVE_UNIFORM_BLOCKS"	},
	{	0x8A37,	"GL_UNIFORM_TYPE"	},
	{	0x8A38,	"GL_UNIFORM_SIZE"	},
	{	0x8A39,	"GL_UNIFORM_NAME_LENGTH"	},
	{	0x8A3A,	"GL_UNIFORM_BLOCK_INDEX"	},
	{	0x8A3B,	"GL_UNIFORM_OFFSET"	},
	{	0x8A3C,	"GL_UNIFORM_ARRAY_STRIDE"	},
	{	0x8A3D,	"GL_UNIFORM_MATRIX_STRIDE"	},
	{	0x8A3E,	"GL_UNIFORM_IS_ROW_MAJOR"	},
	{	0x8A3F,	"GL_UNIFORM_BLOCK_BINDING"	},
	{	0x8A40,	"GL_UNIFORM_BLOCK_DATA_SIZE"	},
	{	0x8A41,	"GL_UNIFORM_BLOCK_NAME_LENGTH"	},
	{	0x8A42,	"GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS"	},
	{	0x8A43,	"GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES"	},
	{	0x8A44,	"GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER"	},
	{	0x8A45,	"GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER"	},
	{	0x8A46,	"GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER"	},
	{	0x8B30,	"GL_FRAGMENT_SHADER_ARB"	},
	{	0x8B30,	"GL_FRAGMENT_SHADER"	},
	{	0x8B31,	"GL_VERTEX_SHADER_ARB"	},
	{	0x8B31,	"GL_VERTEX_SHADER"	},
	{	0x8B40,	"GL_PROGRAM_OBJECT_ARB"	},
	{	0x8B48,	"GL_SHADER_OBJECT_ARB"	},
	{	0x8B49,	"GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB"	},
	{	0x8B49,	"GL_MAX_FRAGMENT_UNIFORM_COMPONENTS"	},
	{	0x8B4A,	"GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB"	},
	{	0x8B4A,	"GL_MAX_VERTEX_UNIFORM_COMPONENTS"	},
	{	0x8B4B,	"GL_MAX_VARYING_COMPONENTS_EXT"	},
	{	0x8B4B,	"GL_MAX_VARYING_FLOATS_ARB"	},
	{	0x8B4B,	"GL_MAX_VARYING_FLOATS"	},
	{	0x8B4C,	"GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB"	},
	{	0x8B4C,	"GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS"	},
	{	0x8B4D,	"GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB"	},
	{	0x8B4D,	"GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS"	},
	{	0x8B4E,	"GL_OBJECT_TYPE_ARB"	},
	{	0x8B4F,	"GL_OBJECT_SUBTYPE_ARB"	},
	{	0x8B4F,	"GL_SHADER_TYPE"	},
	{	0x8B50,	"GL_FLOAT_VEC2_ARB"	},
	{	0x8B50,	"GL_FLOAT_VEC2"	},
	{	0x8B51,	"GL_FLOAT_VEC3_ARB"	},
	{	0x8B51,	"GL_FLOAT_VEC3"	},
	{	0x8B52,	"GL_FLOAT_VEC4_ARB"	},
	{	0x8B52,	"GL_FLOAT_VEC4"	},
	{	0x8B53,	"GL_INT_VEC2_ARB"	},
	{	0x8B53,	"GL_INT_VEC2"	},
	{	0x8B54,	"GL_INT_VEC3_ARB"	},
	{	0x8B54,	"GL_INT_VEC3"	},
	{	0x8B55,	"GL_INT_VEC4_ARB"	},
	{	0x8B55,	"GL_INT_VEC4"	},
	{	0x8B56,	"GL_BOOL_ARB"	},
	{	0x8B56,	"GL_BOOL"	},
	{	0x8B57,	"GL_BOOL_VEC2_ARB"	},
	{	0x8B57,	"GL_BOOL_VEC2"	},
	{	0x8B58,	"GL_BOOL_VEC3_ARB"	},
	{	0x8B58,	"GL_BOOL_VEC3"	},
	{	0x8B59,	"GL_BOOL_VEC4_ARB"	},
	{	0x8B59,	"GL_BOOL_VEC4"	},
	{	0x8B5A,	"GL_FLOAT_MAT2_ARB"	},
	{	0x8B5A,	"GL_FLOAT_MAT2"	},
	{	0x8B5B,	"GL_FLOAT_MAT3_ARB"	},
	{	0x8B5B,	"GL_FLOAT_MAT3"	},
	{	0x8B5C,	"GL_FLOAT_MAT4_ARB"	},
	{	0x8B5C,	"GL_FLOAT_MAT4"	},
	{	0x8B5D,	"GL_SAMPLER_1D_ARB"	},
	{	0x8B5D,	"GL_SAMPLER_1D"	},
	{	0x8B5E,	"GL_SAMPLER_2D_ARB"	},
	{	0x8B5E,	"GL_SAMPLER_2D"	},
	{	0x8B5F,	"GL_SAMPLER_3D_ARB"	},
	{	0x8B5F,	"GL_SAMPLER_3D"	},
	{	0x8B60,	"GL_SAMPLER_CUBE_ARB"	},
	{	0x8B60,	"GL_SAMPLER_CUBE"	},
	{	0x8B61,	"GL_SAMPLER_1D_SHADOW_ARB"	},
	{	0x8B61,	"GL_SAMPLER_1D_SHADOW"	},
	{	0x8B62,	"GL_SAMPLER_2D_SHADOW_ARB"	},
	{	0x8B62,	"GL_SAMPLER_2D_SHADOW"	},
	{	0x8B63,	"GL_SAMPLER_2D_RECT_ARB"	},
	{	0x8B64,	"GL_SAMPLER_2D_RECT_SHADOW_ARB"	},
	{	0x8B65,	"GL_FLOAT_MAT2x3"	},
	{	0x8B66,	"GL_FLOAT_MAT2x4"	},
	{	0x8B67,	"GL_FLOAT_MAT3x2"	},
	{	0x8B68,	"GL_FLOAT_MAT3x4"	},
	{	0x8B69,	"GL_FLOAT_MAT4x2"	},
	{	0x8B6A,	"GL_FLOAT_MAT4x3"	},
	{	0x8B80,	"GL_DELETE_STATUS"	},
	{	0x8B80,	"GL_OBJECT_DELETE_STATUS_ARB"	},
	{	0x8B81,	"GL_COMPILE_STATUS"	},
	{	0x8B81,	"GL_OBJECT_COMPILE_STATUS_ARB"	},
	{	0x8B82,	"GL_LINK_STATUS"	},
	{	0x8B82,	"GL_OBJECT_LINK_STATUS_ARB"	},
	{	0x8B83,	"GL_OBJECT_VALIDATE_STATUS_ARB"	},
	{	0x8B83,	"GL_VALIDATE_STATUS"	},
	{	0x8B84,	"GL_INFO_LOG_LENGTH"	},
	{	0x8B84,	"GL_OBJECT_INFO_LOG_LENGTH_ARB"	},
	{	0x8B85,	"GL_ATTACHED_SHADERS"	},
	{	0x8B85,	"GL_OBJECT_ATTACHED_OBJECTS_ARB"	},
	{	0x8B86,	"GL_ACTIVE_UNIFORMS"	},
	{	0x8B86,	"GL_OBJECT_ACTIVE_UNIFORMS_ARB"	},
	{	0x8B87,	"GL_ACTIVE_UNIFORM_MAX_LENGTH"	},
	{	0x8B87,	"GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB"	},
	{	0x8B88,	"GL_OBJECT_SHADER_SOURCE_LENGTH_ARB"	},
	{	0x8B88,	"GL_SHADER_SOURCE_LENGTH"	},
	{	0x8B89,	"GL_ACTIVE_ATTRIBUTES"	},
	{	0x8B89,	"GL_OBJECT_ACTIVE_ATTRIBUTES_ARB"	},
	{	0x8B8A,	"GL_ACTIVE_ATTRIBUTE_MAX_LENGTH"	},
	{	0x8B8A,	"GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB"	},
	{	0x8B8B,	"GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB"	},
	{	0x8B8B,	"GL_FRAGMENT_SHADER_DERIVATIVE_HINT"	},
	{	0x8B8C,	"GL_SHADING_LANGUAGE_VERSION_ARB"	},
	{	0x8B8C,	"GL_SHADING_LANGUAGE_VERSION"	},
	{	0x8B8D,	"GL_CURRENT_PROGRAM"	},
	{	0x8C10,	"GL_TEXTURE_RED_TYPE_ARB"	},
	{	0x8C10,	"GL_TEXTURE_RED_TYPE"	},
	{	0x8C11,	"GL_TEXTURE_GREEN_TYPE_ARB"	},
	{	0x8C11,	"GL_TEXTURE_GREEN_TYPE"	},
	{	0x8C12,	"GL_TEXTURE_BLUE_TYPE_ARB"	},
	{	0x8C12,	"GL_TEXTURE_BLUE_TYPE"	},
	{	0x8C13,	"GL_TEXTURE_ALPHA_TYPE_ARB"	},
	{	0x8C13,	"GL_TEXTURE_ALPHA_TYPE"	},
	{	0x8C14,	"GL_TEXTURE_LUMINANCE_TYPE_ARB"	},
	{	0x8C15,	"GL_TEXTURE_INTENSITY_TYPE_ARB"	},
	{	0x8C16,	"GL_TEXTURE_DEPTH_TYPE_ARB"	},
	{	0x8C16,	"GL_TEXTURE_DEPTH_TYPE"	},
	{	0x8C17,	"GL_UNSIGNED_NORMALIZED_ARB"	},
	{	0x8C17,	"GL_UNSIGNED_NORMALIZED"	},
	{	0x8C18,	"GL_TEXTURE_1D_ARRAY_EXT"	},
	{	0x8C19,	"GL_PROXY_TEXTURE_1D_ARRAY_EXT"	},
	{	0x8C1A,	"GL_TEXTURE_2D_ARRAY_EXT"	},
	{	0x8C1B,	"GL_PROXY_TEXTURE_2D_ARRAY_EXT"	},
	{	0x8C1C,	"GL_TEXTURE_BINDING_1D_ARRAY_EXT"	},
	{	0x8C1D,	"GL_TEXTURE_BINDING_2D_ARRAY_EXT"	},
	{	0x8C29,	"GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT"	},
	{	0x8C3A,	"GL_R11F_G11F_B10F_EXT"	},
	{	0x8C3B,	"GL_UNSIGNED_INT_10F_11F_11F_REV_EXT"	},
	{	0x8C3C,	"GL_RGBA_SIGNED_COMPONENTS_EXT"	},
	{	0x8C3D,	"GL_RGB9_E5_EXT"	},
	{	0x8C3E,	"GL_UNSIGNED_INT_5_9_9_9_REV_EXT"	},
	{	0x8C3F,	"GL_TEXTURE_SHARED_SIZE_EXT"	},
	{	0x8C40,	"GL_SRGB_EXT"	},
	{	0x8C40,	"GL_SRGB"	},
	{	0x8C41,	"GL_SRGB8_EXT"	},
	{	0x8C41,	"GL_SRGB8"	},
	{	0x8C42,	"GL_SRGB_ALPHA_EXT"	},
	{	0x8C42,	"GL_SRGB_ALPHA"	},
	{	0x8C43,	"GL_SRGB8_ALPHA8_EXT"	},
	{	0x8C43,	"GL_SRGB8_ALPHA8"	},
	{	0x8C44,	"GL_SLUMINANCE_ALPHA_EXT"	},
	{	0x8C44,	"GL_SLUMINANCE_ALPHA"	},
	{	0x8C45,	"GL_SLUMINANCE8_ALPHA8_EXT"	},
	{	0x8C45,	"GL_SLUMINANCE8_ALPHA8"	},
	{	0x8C46,	"GL_SLUMINANCE_EXT"	},
	{	0x8C46,	"GL_SLUMINANCE"	},
	{	0x8C47,	"GL_SLUMINANCE8_EXT"	},
	{	0x8C47,	"GL_SLUMINANCE8"	},
	{	0x8C48,	"GL_COMPRESSED_SRGB_EXT"	},
	{	0x8C48,	"GL_COMPRESSED_SRGB"	},
	{	0x8C49,	"GL_COMPRESSED_SRGB_ALPHA_EXT"	},
	{	0x8C49,	"GL_COMPRESSED_SRGB_ALPHA"	},
	{	0x8C4A,	"GL_COMPRESSED_SLUMINANCE_EXT"	},
	{	0x8C4A,	"GL_COMPRESSED_SLUMINANCE"	},
	{	0x8C4B,	"GL_COMPRESSED_SLUMINANCE_ALPHA_EXT"	},
	{	0x8C4B,	"GL_COMPRESSED_SLUMINANCE_ALPHA"	},
	{	0x8C4C,	"GL_COMPRESSED_SRGB_S3TC_DXT1_EXT"	},
	{	0x8C4D,	"GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT"	},
	{	0x8C4E,	"GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT"	},
	{	0x8C4F,	"GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT"	},
	{	0x8C76,	"GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH_EXT"	},
	{	0x8C7F,	"GL_TRANSFORM_FEEDBACK_BUFFER_MODE_EXT"	},
	{	0x8C80,	"GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT"	},
	{	0x8C83,	"GL_TRANSFORM_FEEDBACK_VARYINGS_EXT"	},
	{	0x8C84,	"GL_TRANSFORM_FEEDBACK_BUFFER_START_EXT"	},
	{	0x8C85,	"GL_TRANSFORM_FEEDBACK_BUFFER_SIZE_EXT"	},
	{	0x8C87,	"GL_PRIMITIVES_GENERATED_EXT"	},
	{	0x8C88,	"GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT"	},
	{	0x8C89,	"GL_RASTERIZER_DISCARD_EXT"	},
	{	0x8C8A,	"GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT"	},
	{	0x8C8B,	"GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_EXT"	},
	{	0x8C8C,	"GL_INTERLEAVED_ATTRIBS_EXT"	},
	{	0x8C8D,	"GL_SEPARATE_ATTRIBS_EXT"	},
	{	0x8C8E,	"GL_TRANSFORM_FEEDBACK_BUFFER_EXT"	},
	{	0x8C8F,	"GL_TRANSFORM_FEEDBACK_BUFFER_BINDING_EXT"	},
	{	0x8CA0,	"GL_POINT_SPRITE_COORD_ORIGIN"	},
	{	0x8CA1,	"GL_LOWER_LEFT"	},
	{	0x8CA2,	"GL_UPPER_LEFT"	},
	{	0x8CA3,	"GL_STENCIL_BACK_REF"	},
	{	0x8CA4,	"GL_STENCIL_BACK_VALUE_MASK"	},
	{	0x8CA5,	"GL_STENCIL_BACK_WRITEMASK"	},
	{	0x8CA6,	"GL_DRAW_FRAMEBUFFER_BINDING_EXT"	},
	{	0x8CA6,	"GL_FRAMEBUFFER_BINDING_EXT"	},
	{	0x8CA6,	"GL_FRAMEBUFFER_BINDING"	},
	{	0x8CA7,	"GL_RENDERBUFFER_BINDING_EXT"	},
	{	0x8CA7,	"GL_RENDERBUFFER_BINDING"	},
	{	0x8CA8,	"GL_READ_FRAMEBUFFER_EXT"	},
	{	0x8CA8,	"GL_READ_FRAMEBUFFER"	},
	{	0x8CA9,	"GL_DRAW_FRAMEBUFFER_EXT"	},
	{	0x8CA9,	"GL_DRAW_FRAMEBUFFER"	},
	{	0x8CAA,	"GL_READ_FRAMEBUFFER_BINDING_EXT"	},
	{	0x8CAA,	"GL_READ_FRAMEBUFFER_BINDING"	},
	{	0x8CAB,	"GL_RENDERBUFFER_SAMPLES_EXT"	},
	{	0x8CAB,	"GL_RENDERBUFFER_SAMPLES"	},
	{	0x8CAC,	"GL_DEPTH_COMPONENT32F"	},
	{	0x8CAD,	"GL_DEPTH32F_STENCIL8"	},
	{	0x8CD0,	"GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT"	},
	{	0x8CD0,	"GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE"	},
	{	0x8CD1,	"GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT"	},
	{	0x8CD1,	"GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME"	},
	{	0x8CD2,	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT"	},
	{	0x8CD2,	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL"	},
	{	0x8CD3,	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT"	},
	{	0x8CD3,	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE"	},
	{	0x8CD4,	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT"	},
	{	0x8CD4,	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT"	},
	{	0x8CD4,	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER"	},
	{	0x8CD5,	"GL_FRAMEBUFFER_COMPLETE_EXT"	},
	{	0x8CD5,	"GL_FRAMEBUFFER_COMPLETE"	},
	{	0x8CD6,	"GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT"	},
	{	0x8CD6,	"GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"	},
	{	0x8CD7,	"GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT"	},
	{	0x8CD7,	"GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"	},
	{	0x8CD9,	"GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT"	},
	{	0x8CDA,	"GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT"	},
	{	0x8CDB,	"GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT"	},
	{	0x8CDB,	"GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"	},
	{	0x8CDC,	"GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT"	},
	{	0x8CDC,	"GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"	},
	{	0x8CDD,	"GL_FRAMEBUFFER_UNSUPPORTED_EXT"	},
	{	0x8CDD,	"GL_FRAMEBUFFER_UNSUPPORTED"	},
	{	0x8CDF,	"GL_MAX_COLOR_ATTACHMENTS_EXT"	},
	{	0x8CDF,	"GL_MAX_COLOR_ATTACHMENTS"	},
	{	0x8CE0,	"GL_COLOR_ATTACHMENT0_EXT"	},
	{	0x8CE0,	"GL_COLOR_ATTACHMENT0"	},
	{	0x8CE1,	"GL_COLOR_ATTACHMENT1_EXT"	},
	{	0x8CE1,	"GL_COLOR_ATTACHMENT1"	},
	{	0x8CE2,	"GL_COLOR_ATTACHMENT2_EXT"	},
	{	0x8CE2,	"GL_COLOR_ATTACHMENT2"	},
	{	0x8CE3,	"GL_COLOR_ATTACHMENT3_EXT"	},
	{	0x8CE3,	"GL_COLOR_ATTACHMENT3"	},
	{	0x8CE4,	"GL_COLOR_ATTACHMENT4_EXT"	},
	{	0x8CE4,	"GL_COLOR_ATTACHMENT4"	},
	{	0x8CE5,	"GL_COLOR_ATTACHMENT5_EXT"	},
	{	0x8CE5,	"GL_COLOR_ATTACHMENT5"	},
	{	0x8CE6,	"GL_COLOR_ATTACHMENT6_EXT"	},
	{	0x8CE6,	"GL_COLOR_ATTACHMENT6"	},
	{	0x8CE7,	"GL_COLOR_ATTACHMENT7_EXT"	},
	{	0x8CE7,	"GL_COLOR_ATTACHMENT7"	},
	{	0x8CE8,	"GL_COLOR_ATTACHMENT8_EXT"	},
	{	0x8CE8,	"GL_COLOR_ATTACHMENT8"	},
	{	0x8CE9,	"GL_COLOR_ATTACHMENT9_EXT"	},
	{	0x8CE9,	"GL_COLOR_ATTACHMENT9"	},
	{	0x8CEA,	"GL_COLOR_ATTACHMENT10_EXT"	},
	{	0x8CEA,	"GL_COLOR_ATTACHMENT10"	},
	{	0x8CEB,	"GL_COLOR_ATTACHMENT11_EXT"	},
	{	0x8CEB,	"GL_COLOR_ATTACHMENT11"	},
	{	0x8CEC,	"GL_COLOR_ATTACHMENT12_EXT"	},
	{	0x8CEC,	"GL_COLOR_ATTACHMENT12"	},
	{	0x8CED,	"GL_COLOR_ATTACHMENT13_EXT"	},
	{	0x8CED,	"GL_COLOR_ATTACHMENT13"	},
	{	0x8CEE,	"GL_COLOR_ATTACHMENT14_EXT"	},
	{	0x8CEE,	"GL_COLOR_ATTACHMENT14"	},
	{	0x8CEF,	"GL_COLOR_ATTACHMENT15_EXT"	},
	{	0x8CEF,	"GL_COLOR_ATTACHMENT15"	},
	{	0x8D00,	"GL_DEPTH_ATTACHMENT_EXT"	},
	{	0x8D00,	"GL_DEPTH_ATTACHMENT"	},
	{	0x8D20,	"GL_STENCIL_ATTACHMENT_EXT"	},
	{	0x8D20,	"GL_STENCIL_ATTACHMENT"	},
	{	0x8D40,	"GL_FRAMEBUFFER_EXT"	},
	{	0x8D40,	"GL_FRAMEBUFFER"	},
	{	0x8D41,	"GL_RENDERBUFFER_EXT"	},
	{	0x8D41,	"GL_RENDERBUFFER"	},
	{	0x8D42,	"GL_RENDERBUFFER_WIDTH_EXT"	},
	{	0x8D42,	"GL_RENDERBUFFER_WIDTH"	},
	{	0x8D43,	"GL_RENDERBUFFER_HEIGHT_EXT"	},
	{	0x8D43,	"GL_RENDERBUFFER_HEIGHT"	},
	{	0x8D44,	"GL_RENDERBUFFER_INTERNAL_FORMAT_EXT"	},
	{	0x8D44,	"GL_RENDERBUFFER_INTERNAL_FORMAT"	},
	{	0x8D46,	"GL_STENCIL_INDEX1_EXT"	},
	{	0x8D46,	"GL_STENCIL_INDEX1"	},
	{	0x8D47,	"GL_STENCIL_INDEX4_EXT"	},
	{	0x8D47,	"GL_STENCIL_INDEX4"	},
	{	0x8D48,	"GL_STENCIL_INDEX8_EXT"	},
	{	0x8D48,	"GL_STENCIL_INDEX8"	},
	{	0x8D49,	"GL_STENCIL_INDEX16_EXT"	},
	{	0x8D49,	"GL_STENCIL_INDEX16"	},
	{	0x8D50,	"GL_RENDERBUFFER_RED_SIZE_EXT"	},
	{	0x8D50,	"GL_RENDERBUFFER_RED_SIZE"	},
	{	0x8D51,	"GL_RENDERBUFFER_GREEN_SIZE_EXT"	},
	{	0x8D51,	"GL_RENDERBUFFER_GREEN_SIZE"	},
	{	0x8D52,	"GL_RENDERBUFFER_BLUE_SIZE_EXT"	},
	{	0x8D52,	"GL_RENDERBUFFER_BLUE_SIZE"	},
	{	0x8D53,	"GL_RENDERBUFFER_ALPHA_SIZE_EXT"	},
	{	0x8D53,	"GL_RENDERBUFFER_ALPHA_SIZE"	},
	{	0x8D54,	"GL_RENDERBUFFER_DEPTH_SIZE_EXT"	},
	{	0x8D54,	"GL_RENDERBUFFER_DEPTH_SIZE"	},
	{	0x8D55,	"GL_RENDERBUFFER_STENCIL_SIZE_EXT"	},
	{	0x8D55,	"GL_RENDERBUFFER_STENCIL_SIZE"	},
	{	0x8D56,	"GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT"	},
	{	0x8D56,	"GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"	},
	{	0x8D57,	"GL_MAX_SAMPLES_EXT"	},
	{	0x8D57,	"GL_MAX_SAMPLES"	},
	{	0x8D70,	"GL_RGBA32UI_EXT"	},
	{	0x8D71,	"GL_RGB32UI_EXT"	},
	{	0x8D72,	"GL_ALPHA32UI_EXT"	},
	{	0x8D73,	"GL_INTENSITY32UI_EXT"	},
	{	0x8D74,	"GL_LUMINANCE32UI_EXT"	},
	{	0x8D75,	"GL_LUMINANCE_ALPHA32UI_EXT"	},
	{	0x8D76,	"GL_RGBA16UI_EXT"	},
	{	0x8D77,	"GL_RGB16UI_EXT"	},
	{	0x8D78,	"GL_ALPHA16UI_EXT"	},
	{	0x8D79,	"GL_INTENSITY16UI_EXT"	},
	{	0x8D7A,	"GL_LUMINANCE16UI_EXT"	},
	{	0x8D7B,	"GL_LUMINANCE_ALPHA16UI_EXT"	},
	{	0x8D7C,	"GL_RGBA8UI_EXT"	},
	{	0x8D7D,	"GL_RGB8UI_EXT"	},
	{	0x8D7E,	"GL_ALPHA8UI_EXT"	},
	{	0x8D7F,	"GL_INTENSITY8UI_EXT"	},
	{	0x8D80,	"GL_LUMINANCE8UI_EXT"	},
	{	0x8D81,	"GL_LUMINANCE_ALPHA8UI_EXT"	},
	{	0x8D82,	"GL_RGBA32I_EXT"	},
	{	0x8D83,	"GL_RGB32I_EXT"	},
	{	0x8D84,	"GL_ALPHA32I_EXT"	},
	{	0x8D85,	"GL_INTENSITY32I_EXT"	},
	{	0x8D86,	"GL_LUMINANCE32I_EXT"	},
	{	0x8D87,	"GL_LUMINANCE_ALPHA32I_EXT"	},
	{	0x8D88,	"GL_RGBA16I_EXT"	},
	{	0x8D89,	"GL_RGB16I_EXT"	},
	{	0x8D8A,	"GL_ALPHA16I_EXT"	},
	{	0x8D8B,	"GL_INTENSITY16I_EXT"	},
	{	0x8D8C,	"GL_LUMINANCE16I_EXT"	},
	{	0x8D8D,	"GL_LUMINANCE_ALPHA16I_EXT"	},
	{	0x8D8E,	"GL_RGBA8I_EXT"	},
	{	0x8D8F,	"GL_RGB8I_EXT"	},
	{	0x8D90,	"GL_ALPHA8I_EXT"	},
	{	0x8D91,	"GL_INTENSITY8I_EXT"	},
	{	0x8D92,	"GL_LUMINANCE8I_EXT"	},
	{	0x8D93,	"GL_LUMINANCE_ALPHA8I_EXT"	},
	{	0x8D94,	"GL_RED_INTEGER_EXT"	},
	{	0x8D95,	"GL_GREEN_INTEGER_EXT"	},
	{	0x8D96,	"GL_BLUE_INTEGER_EXT"	},
	{	0x8D97,	"GL_ALPHA_INTEGER_EXT"	},
	{	0x8D98,	"GL_RGB_INTEGER_EXT"	},
	{	0x8D99,	"GL_RGBA_INTEGER_EXT"	},
	{	0x8D9A,	"GL_BGR_INTEGER_EXT"	},
	{	0x8D9B,	"GL_BGRA_INTEGER_EXT"	},
	{	0x8D9C,	"GL_LUMINANCE_INTEGER_EXT"	},
	{	0x8D9D,	"GL_LUMINANCE_ALPHA_INTEGER_EXT"	},
	{	0x8D9E,	"GL_RGBA_INTEGER_MODE_EXT"	},
	{	0x8DA7,	"GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT"	},
	{	0x8DA8,	"GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT"	},
	{	0x8DA9,	"GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT"	},
	{	0x8DAD,	"GL_FLOAT_32_UNSIGNED_INT_24_8_REV"	},
	{	0x8DB9,	"GL_FRAMEBUFFER_SRGB_EXT"	},
	{	0x8DBA,	"GL_FRAMEBUFFER_SRGB_CAPABLE_EXT"	},
	{	0x8DBB,	"GL_COMPRESSED_RED_RGTC1"	},
	{	0x8DBC,	"GL_COMPRESSED_SIGNED_RED_RGTC1"	},
	{	0x8DBD,	"GL_COMPRESSED_RG_RGTC2"	},
	{	0x8DBE,	"GL_COMPRESSED_SIGNED_RG_RGTC2"	},
	{	0x8DC0,	"GL_SAMPLER_1D_ARRAY_EXT"	},
	{	0x8DC1,	"GL_SAMPLER_2D_ARRAY_EXT"	},
	{	0x8DC2,	"GL_SAMPLER_BUFFER_EXT"	},
	{	0x8DC3,	"GL_SAMPLER_1D_ARRAY_SHADOW_EXT"	},
	{	0x8DC4,	"GL_SAMPLER_2D_ARRAY_SHADOW_EXT"	},
	{	0x8DC5,	"GL_SAMPLER_CUBE_SHADOW_EXT"	},
	{	0x8DC6,	"GL_UNSIGNED_INT_VEC2_EXT"	},
	{	0x8DC7,	"GL_UNSIGNED_INT_VEC3_EXT"	},
	{	0x8DC8,	"GL_UNSIGNED_INT_VEC4_EXT"	},
	{	0x8DC9,	"GL_INT_SAMPLER_1D_EXT"	},
	{	0x8DCA,	"GL_INT_SAMPLER_2D_EXT"	},
	{	0x8DCB,	"GL_INT_SAMPLER_3D_EXT"	},
	{	0x8DCC,	"GL_INT_SAMPLER_CUBE_EXT"	},
	{	0x8DCD,	"GL_INT_SAMPLER_2D_RECT_EXT"	},
	{	0x8DCE,	"GL_INT_SAMPLER_1D_ARRAY_EXT"	},
	{	0x8DCF,	"GL_INT_SAMPLER_2D_ARRAY_EXT"	},
	{	0x8DD0,	"GL_INT_SAMPLER_BUFFER_EXT"	},
	{	0x8DD1,	"GL_UNSIGNED_INT_SAMPLER_1D_EXT"	},
	{	0x8DD2,	"GL_UNSIGNED_INT_SAMPLER_2D_EXT"	},
	{	0x8DD3,	"GL_UNSIGNED_INT_SAMPLER_3D_EXT"	},
	{	0x8DD4,	"GL_UNSIGNED_INT_SAMPLER_CUBE_EXT"	},
	{	0x8DD5,	"GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT"	},
	{	0x8DD6,	"GL_UNSIGNED_INT_SAMPLER_1D_ARRAY_EXT"	},
	{	0x8DD7,	"GL_UNSIGNED_INT_SAMPLER_2D_ARRAY_EXT"	},
	{	0x8DD8,	"GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT"	},
	{	0x8DD9,	"GL_GEOMETRY_SHADER_EXT"	},
	{	0x8DDA,	"GL_GEOMETRY_VERTICES_OUT_EXT"	},
	{	0x8DDB,	"GL_GEOMETRY_INPUT_TYPE_EXT"	},
	{	0x8DDC,	"GL_GEOMETRY_OUTPUT_TYPE_EXT"	},
	{	0x8DDD,	"GL_MAX_GEOMETRY_VARYING_COMPONENTS_EXT"	},
	{	0x8DDE,	"GL_MAX_VERTEX_VARYING_COMPONENTS_EXT"	},
	{	0x8DDF,	"GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT"	},
	{	0x8DE0,	"GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT"	},
	{	0x8DE1,	"GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT"	},
	{	0x8DE2,	"GL_MAX_VERTEX_BINDABLE_UNIFORMS_EXT"	},
	{	0x8DE3,	"GL_MAX_FRAGMENT_BINDABLE_UNIFORMS_EXT"	},
	{	0x8DE4,	"GL_MAX_GEOMETRY_BINDABLE_UNIFORMS_EXT"	},
	{	0x8DED,	"GL_MAX_BINDABLE_UNIFORM_SIZE_EXT"	},
	{	0x8DEE,	"GL_UNIFORM_BUFFER_EXT"	},
	{	0x8DEF,	"GL_UNIFORM_BUFFER_BINDING_EXT"	},
	{	0x8E4C,	"GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION_EXT"	},
	{	0x8E4D,	"GL_FIRST_VERTEX_CONVENTION_EXT"	},
	{	0x8E4E,	"GL_LAST_VERTEX_CONVENTION_EXT"	},
	{	0x8E4F,	"GL_PROVOKING_VERTEX_EXT"	},
	
	VE( TERMVALUE )
};

GLMValueEntry_t g_gl_renderers[] = 
{
	{ 0x00020200, "Generic" },
	{ 0x00020400, "GenericFloat" },
	{ 0x00020600, "AppleSW" },
	{ 0x00021000, "ATIRage128" },
	{ 0x00021200, "ATIRadeon" },
	{ 0x00021400, "ATIRagePro" },
	{ 0x00021600, "ATIRadeon8500" },
	{ 0x00021800, "ATIRadeon9700" },
	{ 0x00021900, "ATIRadeonX1000" },
	{ 0x00021A00, "ATIRadeonX2000" },
	{ 0x00022000, "NVGeForce2MX" },
	{ 0x00022200, "NVGeForce3" },
	{ 0x00022400, "NVGeForceFX" },
	{ 0x00022600, "NVGeForce8xxx" },
	{ 0x00023000, "VTBladeXP2" },
	{ 0x00024000, "Intel900" },
	{ 0x00024200, "IntelX3100" },
	{ 0x00040000, "Mesa3DFX" },

	VE( TERMVALUE )
};


//===============================================================================
// decode helper funcs

char	s_glmStrScratch[65536];
int		s_glmStrCursor = 0;

const char *	GLMDecode( GLMThing_t thingtype, unsigned long value )
{
	GLMValueEntry_t *table = NULL;
	char			isflags = 0;

	switch( thingtype )
	{
		case	eD3D_DEVTYPE:	table = g_d3d_devtypes;
		break;
		
		case	eD3D_FORMAT:	table = g_d3d_formats;
		break;
			
		case	eD3D_RTYPE:		table = g_d3d_rtypes;
		break;
			
		case	eD3D_USAGE:		table = g_d3d_usages;
		break;
		
		case 	eD3D_RSTATE:	table = g_d3d_rstates;
		break;
		
		case	eD3D_SIO:		table = g_d3d_opcodes;			
		break;
		
		case	eD3D_VTXDECLUSAGE:	table = g_d3d_vtxdeclusages;
		break;
		
		case	eCGL_RENDID:	table = g_cgl_rendids;
		break;
			
		case	eGL_ERROR:		table = g_gl_errors;
		break;
		
		case	eGL_ENUM:		table = g_gl_enums;
		break;

		case	eGL_RENDERER:	table = g_gl_renderers;
		break;

		default:
			GLMStop();
			return "UNKNOWNTYPE";
		break;
	}
	
	if (table)
	{
		while( table->value != TERMVALUE )
		{
			if (table->value == value)
			{
				return table->name;
			}
			table++;
		}
	}
	return "UNKNOWN";
}

const char	*GLMDecodeMask( GLMThing_t kind, unsigned long value )
{
	// if cursor to scratch buffer is within 1K of EOB, rewind
	// nobody is going to decode 63K of flag string values in a single call..
	
	// this means that strings returned by this function have a short lifetime.. print them and do not save the pointer..
	
	if ( (sizeof(s_glmStrScratch) - s_glmStrCursor) < 1000 )
	{
		s_glmStrCursor = 0;
	}
	
	char	*start = &s_glmStrScratch[ s_glmStrCursor ];
	char	*dest = start;
	char	first = 1;
	
	DWORD mask = (1L<<31);
	while(mask)
	{
		if (mask & value)
		{
			sprintf(dest,"%s%s", (first) ? "" : "|", GLMDecode( kind, value&mask ) );
			first = 0;
			
			dest += strlen(dest);	// leaves dest pointing at the end null
		}
		mask >>= 1;
	}
	s_glmStrCursor = (dest - s_glmStrScratch) + 1;	// +1 so the next decoded flag set doesn't land on the ending null
	return start;
	
}

#undef VE
#undef TERMVALUE

//===============================================================================

bool	GLMDetectOGLP( void )
{
	bool result;

	GLint forceFlush;
	CGLError error = CGLGetParameter(CGLGetCurrentContext(), kCGLCPEnableForceFlush, &forceFlush);
	result = error == 0;
	if (result)
	{
        // enable a breakpoint on color4sv
		GLint oglp_bkpt[3] = { kCGLFEglColor4sv, kCGLProfBreakBefore, 1 };
		
		CGLSetGlobalOption( kCGLGOEnableBreakpoint, oglp_bkpt );
	}
	
	return result;
}


// from http://blog.timac.org/?p=190

#include <stdbool.h>  
#include <sys/types.h>  
#include <unistd.h>  
#include <sys/sysctl.h>  
  
// From Technical Q&A QA1361  
// Returns true if the current process  
// is being debugged (either running  
// under the debugger or has a debugger  
// attached post facto).  

bool	GLMDetectGDB( void )			// aka AmIBeingDebugged()
{
	bool				result;	
    int                 junk;  
    int                 mib[4];  
    struct kinfo_proc   info;  
    size_t              size;  
  
    // Initialize the flags so that,  
    // if sysctl fails for some bizarre  
    // reason, we get a predictable result.  
  
    info.kp_proc.p_flag = 0;  
  
    // Initialize mib, which tells sysctl the info  
    // we want, in this case we're looking for  
    // information about a specific process ID.  
  
    mib[0] = CTL_KERN;  
    mib[1] = KERN_PROC;  
    mib[2] = KERN_PROC_PID;  
    mib[3] = getpid();  
  
    // Call sysctl.  
  
    size = sizeof(info);  
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);  
  
    assert(junk == 0);  
  
    // We're being debugged if the P_TRACED  
    // flag is set.  
  
    result = ( (info.kp_proc.p_flag & P_TRACED) != 0 );  
	
	return result;
}


static uint		g_glmDebugChannelMask = 0;		// which output channels are available (can be more than one)
static uint		g_glmDebugFlavorMask = 0;		// which message flavors are enabled for output (can be more than one)

uint	GLMDetectAvailableChannels( void )
{
	uint result = 0;
	
	// printf is always available (except maybe in release... ?)
	result |= (1 << ePrintf);

	// gdb
	if (GLMDetectGDB())
	{
		result |= (1 << eDebugger);
		printf("\n############# GDB Detected");
	}
	
	// oglp
	if (GLMDetectOGLP())
	{
		result |= (1 << eGLProfiler);
		printf("\n############# OGLP Detected");
	}	
	
	return result;
}


static bool	g_debugInitDone	= false;

#if GLMDEBUG

	// following funcs vanish if GLMDEBUG not set

void	GLMDebugInitialize( bool forceReinit )
{
	if ( !g_debugInitDone || forceReinit )
	{
		// detect channels
		uint channelMask = GLMDetectAvailableChannels();
		
		// finally, disable all of them if commandline did not say "enable spew"
		if (0 /* !CommandLine()->FindParm("-glmspew") */)	//FIXME change back to 1 later
		{
			channelMask = 0;
		}

		// set the output channel mask
		GLMDebugChannelMask( &channelMask );

		// if any channels are enabled, enable some output
		if ( channelMask )
		{
			// start mostly quiet unless the -glmbootspew option is there
			if ( 0 /*CommandLine()->FindParm( "-glmbootspew" )*/ )
			{
				g_glmDebugFlavorMask = 0xFFFFFFFF;
			}
			else
			{
				g_glmDebugFlavorMask =
					(1<<eAllFlavors)
					| (1<<eDebugDump)			// -D- stuff
				//	| (1<<eFrameBufData)		// -F-
					| (1<<eDXStuff)				// -X-
				//	| (1<<eTenure)				// > <
				//	| (1<<eAllocations)			// -A-
				//	| (1<<eSlowness)			// -Z-
					| (1<<eDefaultFlavor);		// adjust to suit
			}
		}
		else
		{
			g_glmDebugFlavorMask = 0;
		}
	}
}

uint	GLMDebugChannelMask( uint *newValue )
{
	if (newValue)
	{
		g_glmDebugChannelMask = *newValue;
	}
	
	uint result = g_glmDebugChannelMask;
	
	// leave space for any override / mute mechanism we might want to inject here
	
	return result;
}

uint	GLMDebugFlavorMask( uint *newValue )
{
	if (newValue)
	{
		g_glmDebugFlavorMask = *newValue;
	}
	
	uint result = g_glmDebugFlavorMask;
	
	// leave space for any override / mute mechanism we might want to inject here
	
	return result;
}

#endif

//===============================================================================

void GLMStop( void )
{
	GLMDebugger();
}

void	GLMCheckError( bool noStop, bool noLog )	// caller can force off, but not on.  Debugger can force all to be off (flip the chars).
{

// errors are not checked in release mode.
// we will probably want to introduce a separate call for more critical error checks.
// getting to zero glGetError calls per render loop is important for keeping the MTGL driver humming.

	if ( (GLMDEBUG || (gl_errorcheckall != 0) ) && (gl_errorchecknone==0) )
	{
		static char log_errors = 1;
		static char stop_on_error = 1;
		
		#if GLMDEBUG
			if (GLMDebugChannelMask() & (1<<eGLProfiler) )
			{
				return;	// no error checking when under OGLP - use the break on error switch if you need it
			}
		#endif
		
		char errbuf[1024];

		GLenum errorcode = (GLenum)glGetError();
		GLenum errorcode2 = 0;
		if (errorcode != GL_NO_ERROR)
		{
			const char	*decodedStr = GLMDecode( eGL_ERROR, errorcode );
			const char	*decodedStr2 = "";
					
			if (errorcode == GL_INVALID_FRAMEBUFFER_OPERATION_EXT)
			{
				// dig up the more detailed FBO status
				errorcode2 = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
				
				decodedStr2 = GLMDecode( eGL_ERROR, errorcode2 );

				sprintf( errbuf, "\nGL Error %08x/%08x = '%s / %s'", errorcode, errorcode2, decodedStr, decodedStr2 );
			}
			else
			{
				sprintf( errbuf, "\nGL Error %08x = '%s'", errorcode, decodedStr );
			}

			if (log_errors && !noLog )
			{
				printf("%s", errbuf );
			}
			
			if (stop_on_error && !noStop)
			{
				Debugger();
			}
		}
	}
}

//===============================================================================
void GLMEnableTrace( bool on )
{
#if GLMDEBUG
	if ( GLMDebugChannelMask() & (1<<eGLProfiler) )
	{
		CGLSetOption(kCGLGOEnableFunctionTrace, on ? GL_TRUE : GL_FALSE );
	}
#endif
}

//===============================================================================

#if GLMDEBUG
	// following funcs vanish if GLMDEBUG not set
	
void	GLMStringOut( char *string )
{
	if ( GLMDebugChannelMask() & ( (1<<ePrintf) | (1<<eDebugger) ) )
	{
		puts( string );
	}

	if ( GLMDebugChannelMask() & (1<<eGLProfiler) )
	{
		CGLSetGlobalOption( kCGLGOComment, (GLint*)string );
	}
}

int		g_glm_indent = 0;
int		g_glm_indent_max = 40;		// 40 tabs max

EGLMDebugFlavor	GLMAssessFlavor( char *str )
{
	EGLMDebugFlavor flavor = eDefaultFlavor;

	if (strnstr(str, "---", 4))	// note we check four chars worth so you can do >-M- and it will indent and be filterable
	{
		// comment
		flavor = eComment;
	}
	else if (strnstr(str, "-D-", 4))
	{
		// debug dump
		flavor = eDebugDump;
	}
	else if (strnstr(str, "-M-", 4))
	{
		// matrix data
		flavor = eMatrixData;
	}
	else if (strnstr(str, "-S-", 4))
	{
		// shader data
		flavor = eShaderData;
	}
	else if (strnstr(str, "-F-", 4))
	{
		// framebuf data
		flavor = eFrameBufData;
	}
	else if (strnstr(str, "-X-", 4))
	{
		// DirectX data
		flavor = eDXStuff;
	}
	else if (strnstr(str, "-A-", 4))
	{
		// allocation data
		flavor = eAllocations;
	}
	else if (strnstr(str, "-Z-", 4))
	{
		// allocation data
		flavor = eSlowness;
	}
	else if (str[0] == '<' || str[0] == '>')
	{
		// entry/exit (aka tenure)
		flavor = eTenure;
	}

	return flavor;
}

void	GLMPrintfVA( const char *fmt, va_list vargs )
{
	// if no channels open, return
	uint channelMask = GLMDebugChannelMask();
	if (!channelMask)
		return;
	
	// if "all flavors" is off, return
	uint flavorMask = GLMDebugFlavorMask();
	if (! ( flavorMask & (1<<eAllFlavors) ) )
		return;

	// characterize the flavor of the string
	// if flavor hits against the flavor mask, print it
	EGLMDebugFlavor flavor = GLMAssessFlavor( (char *)fmt );
	if ( !(flavorMask & (1<<flavor)) )
		return;

	// print the formatted string, with indenting
	// if first char is a '>' - raise indent level after print.
	// if first char is a '<' - lower indent level before print.
	
	char buf[100000];
	
	if (fmt[0] == '<')
	{
		GLMIncIndent( -1 );
	}

	memset( buf, '\t', g_glm_indent );
	vsprintf( buf+g_glm_indent, fmt, vargs );
	GLMStringOut( buf );
	
	if (fmt[0] == '>')
	{
		GLMIncIndent( 1 );
	}
}

void	GLMPrintf( const char *fmt, ... )
{
	// if no channels open, return
	uint channelMask = GLMDebugChannelMask();
	if (!channelMask)
		return;
	
	// if "all flavors" is off, return
	uint flavorMask = GLMDebugFlavorMask();
	if (! ( flavorMask & (1<<eAllFlavors) ) )
		return;

	va_list	vargs;
	va_start(vargs, fmt);
	GLMPrintfVA( fmt, vargs );
	va_end( vargs );
}

void	GLMPrintStr( const char *str, EGLMDebugFlavor flavor )					// will indent
{
	// if no channels open, return
	uint channelMask = GLMDebugChannelMask();
	if (!channelMask)
		return;
	
	// if "all flavors" is off, return
	uint flavorMask = GLMDebugFlavorMask();
	if (! ( flavorMask & (1<<eAllFlavors) ) )
		return;

	// if flavor argument hits against the flavor mask, print it
	if ( !(flavorMask & (1<<flavor)) )
		return;

	// just print the plain string, with indenting
	// if first char is a '>' - raise indent level after print.
	// if first char is a '<' - lower indent level before print.
	
	char buf[64000];
	
	if (str[0] == '<')
	{
		GLMIncIndent( -1 );
	}

	memset( buf, '\t', g_glm_indent );
	
	if (strlen(str) < sizeof(buf)-g_glm_indent-1)
	{
		strcpy( buf + g_glm_indent, str );
	}
	else
	{
		Debugger();
	}

	
	GLMStringOut( buf );		// single string out with indenting
	
	if (str[0] == '>')
	{
		GLMIncIndent( 1 );
	}
}


void GLMPrintText( const char *str, EGLMDebugFlavor flavor, uint options )
{
	// if no channels open, return
	uint channelMask = GLMDebugChannelMask();
	if (!channelMask)
		return;
	
	// if "all flavors" is off, return
	uint flavorMask = GLMDebugFlavorMask();
	if (! ( flavorMask & (1<<eAllFlavors) ) )
		return;

	// if flavor argument hits against the flavor mask, print it
	if ( !(flavorMask & (1<<flavor)) )
		return;

	char	buf[64000];
	char	lineout[64000];
	int		linenum = 1;
	
	strncpy( buf, str, sizeof(buf) );
	
	// walk the text and treat each newline as an indentation opportunity..
	const char *mark = buf;
	const char *end = mark + strlen(buf);
	const char *next = NULL;
	
	while(mark < end)
	{
		// starting at mark, see if there is a newline between there and end
		char *next = strchr( mark, '\n' );
		const char *printfrom = mark;
		if (next)
		{
			// print text from mark up through next.  move next to char after newline.
			*next = 0;
			mark = next+1;
		}
		else
		{
			// print all text after mark and stop
			mark = end;
		}
		if (options & GLMPRINTTEXT_NUMBEREDLINES)
		{
			sprintf( lineout, "%-5d| %s", linenum, printfrom );
			GLMPrintStr( lineout, flavor );
			linenum++;
		}
		else
		{
			GLMPrintStr( printfrom, flavor );
		}
	}
}

int		GLMIncIndent( int indentDelta )
{
	g_glm_indent += indentDelta;
	
	if (indentDelta>0)
	{
		if (g_glm_indent > g_glm_indent_max)
		{
			g_glm_indent = g_glm_indent_max;
		}
	}
	else
	{
		if (g_glm_indent < 0)
		{
			g_glm_indent = 0;
		}
	}
	return g_glm_indent;
}

int	GLMGetIndent( void )
{
	return g_glm_indent;
}

void	GLMSetIndent( int indent )
{
	g_glm_indent = indent;
}

#endif

// PIX tracking - you can call these outside of GLMDEBUG=true
char sg_pPIXName[128];

void GLMBeginPIXEvent( const char *str )
{
	V_strncpy( sg_pPIXName, str, 128 );

	/*
	if (CommandLine()->FindParm("-glmpix2oglp"))
	{
		char temp[256];
		// route PIX event strings to OGLP
		sprintf( temp,"> %s",sg_pPIXName );
		CGLSetOption( kCGLGOComment, (GLint)temp );
	}
	*/
}

void GLMEndPIXEvent( void )
{
	/*
	if (CommandLine()->FindParm("-glmpix2oglp"))
	{
		char temp[256];
		// route PIX event strings to OGLP
		sprintf( temp,"< %s",sg_pPIXName );
		CGLSetOption( kCGLGOComment, (GLint)temp );
	}
	*/
	sg_pPIXName[0] = '\0';
}




//===============================================================================

// helpers for CGLSetOption - no op if no profiler
void	GLMProfilerClearTrace( void )
{
	CGLSetOption( kCGLGOResetFunctionTrace, 0 );
}

void	GLMProfilerEnableTrace( bool enable )
{
	CGLSetOption( kCGLGOEnableFunctionTrace, enable ? GL_TRUE : GL_FALSE );
}

// helpers for CGLSetParameter - no op if no profiler
void	GLMProfilerDumpState( void )
{
	CGLContextObj curr = CGLGetCurrentContext();
	CGLSetParameter( curr, kCGLCPDumpState, (const GLint*)1 );
}


//===============================================================================

CGLMFileMirror::CGLMFileMirror( char *fullpath )
{
	m_path = strdup( fullpath );
	m_data = (char *)malloc(1);
	m_size = 0;
	UpdateStatInfo();
	if (m_exists)
	{
		ReadFile();
	}
}

CGLMFileMirror::~CGLMFileMirror( )
{
	if (m_path)
	{
		free (m_path);
		m_path = NULL;
	}
	
	if (m_data)
	{
		free (m_data);
		m_data = NULL;
	}
}

bool CGLMFileMirror::HasData( void )
{
	return (m_size != 0);
}


// return direct pointer to buffer.  Will be invalidated if file is re-loaded or if data is written to
void CGLMFileMirror::GetData( char **dataPtr, uint *dataSizePtr )
{
	*dataPtr		= m_data;
	*dataSizePtr	= m_size;
}


void CGLMFileMirror::SetData( char *data, uint dataSize )
{
	if (m_data)
	{
		free( m_data );
		m_data = NULL;
	}

	m_size = dataSize;
	
	m_data = (char *)malloc( m_size +1 );
	m_data[ m_size ] = 0;			// extra NULL terminator, no charge
	
	memcpy( m_data, data, m_size );	// copy data in
	
	WriteFile();					// keep disk copy sync'd
}

static bool stat_diff( struct stat *a, struct stat *b )
{
	if (a->st_size != b->st_size)
	{
		return true;
	}
	
	if (memcmp( &a->st_mtimespec, &b->st_mtimespec, sizeof( struct timespec ) ) )
	{
		return true;
	}
	
	return false;
}

bool CGLMFileMirror::PollForChanges( void )
{
	// snapshot old stat
	bool		old_exists = m_exists;
	struct stat	old_stat = m_stat;
	
	UpdateStatInfo();
	
	if (m_exists)
	{
		if ( stat_diff( &old_stat, &m_stat ) )
		{
			// initial difference detected.  continue to poll at 0.1s intervals until it stops changing, then read it.
			int stablecount = 0;
			do
			{
				usleep(100000);
				
				struct stat last_stat = m_stat;
				UpdateStatInfo();
				
				if (stat_diff( &last_stat, &m_stat ))
				{
					stablecount = 0;
				}
				else
				{
					stablecount++;
				}
			} while(stablecount<3);
			
			// changes have settled down, now re-read it
			ReadFile();
			return true;
		}
		else
		{
			return false;	// no change
		}
	}
	else
	{
		// file does not exist.  remake it. not considered to be a change.
		WriteFile();
		return false;
	}
}



void CGLMFileMirror::UpdateStatInfo( void )
{
	// stat the path
	struct stat newstat;
	memset (&newstat, 0, sizeof(newstat) );
	int result = stat( m_path, &newstat );

	if (!result)
	{
		m_exists = true;
		m_stat = newstat;
	}
	else
	{
		m_exists = false;
		memset( &m_stat, 0, sizeof( m_stat ) );
	}
}


void CGLMFileMirror::ReadFile( void )
{
	// unconditional - we discard any old buffer, make a new one, 
	UpdateStatInfo();
	
	if (m_data)
	{
		free( m_data );
		m_data = NULL;
	}

	if (m_exists)
	{
		FILE *infile = fopen( m_path, "rb" );
		if (infile)
		{
			// get size from stat
			m_size = m_stat.st_size;
			
			m_data = (char *)malloc( m_size +1 );
			m_data[ m_size ] = 0;	// extra NULL terminator, no charge
			
			fread( m_data, 1, m_size, infile );
			
			fclose( infile );
		}
		else
		{
			GLMDebugger();	// ouch
		}

	}
	else
	{
		// hmmmmmm
		m_data = (char *)malloc(1);
		m_data[0] = 0;
		m_size = 0;
	}
}


void CGLMFileMirror::WriteFile( void )
{
	FILE *outfile = fopen( m_path, "wb" );
	
	if (outfile)
	{
		fwrite( m_data, 1, m_size, outfile );		
		fclose( outfile );
				
		UpdateStatInfo();	// sets m_stat and m_exists
	}
	else
	{
		GLMDebugger();	// ouch
	}
}

void	CGLMFileMirror::OpenInEditor( bool foreground )
{
	char temp[64000];
	
	// pass -b if no desire to bring editor to foreground
	sprintf(temp,"/usr/bin/bbedit %s %s", foreground ? "" : "-b", m_path );
	system( temp );
}



CGLMEditableTextItem::CGLMEditableTextItem( char *text, uint size, bool forceOverwrite, char *prefix, char *suffix )
{
	// clone input text (exact size copy)
	m_origSize = size;
	m_origText = (char *)malloc( m_origSize );
	memcpy( m_origText, text, m_origSize );
	
	// null out munged form til we generate it
	m_mungedSize = 0;
	m_mungedText = NULL;
	
	// null out mirror until we create it
	m_mirrorBaseName = NULL;
	m_mirrorFullPath = NULL;
	m_mirror = NULL;
	
	GenHashOfOrigText();						// will fill out m_origDigest
	GenMungedText( false );
	GenBaseNameAndFullPath( prefix, suffix );	// figure out where the mirror will go
	
	if (!strcmp(m_mirrorBaseName, "96c7e9d2faf76b1148f7274afd684d4b.fsh"))
	{
		printf("\nhello there\n");
	}

	// make the mirror from the filename.
	// see if there was any content on disk
	// if so, honor that content *unless* the force-option is set.
	m_mirror = new CGLMFileMirror( m_mirrorFullPath );
	
	// the logic is simple.
	// the only time we will choose the copy on disk, is if
	// a - forceOverwrite is false
	// AND b - the copy on disk is bigger than 10 bytes.
	
	bool	replaceDiskCopy = true;
	
	char	*mirrorData = NULL;
	uint	mirrorSize = 0;

	if (!forceOverwrite)
	{
		if (m_mirror->HasData())
		{
			// peek at it, and use it if it is more than some minimum number of bytes.
			m_mirror->GetData( &mirrorData, &mirrorSize );
			if (mirrorSize > 10)
			{
				replaceDiskCopy = false;
			}
		}
	}
	
	if (replaceDiskCopy)
	{
		// push our generated data to the mirror - disk copy is overwritten
		m_mirror->SetData( m_mungedText, m_mungedSize );
	}
	else
	{
		GenMungedText( true );
	}

}	
	
CGLMEditableTextItem::~CGLMEditableTextItem( )
{
	if (m_origText)
	{
		free (m_origText);
	}
	
	if (m_mungedText)
	{
		free (m_mungedText);
	}
	
	if (m_mirrorBaseName)
	{
		free (m_mirrorBaseName);
	}
	
	if (m_mirrorFullPath)
	{
		free (m_mirrorFullPath);
	}
	
	if (m_mirror)
	{
		free( m_mirror );
	}
}

bool	CGLMEditableTextItem::HasData( void )
{
	return m_mirror->HasData();
}
	
bool	CGLMEditableTextItem::PollForChanges( void )
{
	bool changed = m_mirror->PollForChanges();
	if (changed)
	{
		// re-gen munged text from mirror (means "copy")
		GenMungedText( true );
	}
	return changed;
}

void	CGLMEditableTextItem::GetCurrentText( char **textOut, uint *sizeOut )
{
	if (!m_mungedText)	GLMDebugger();
	
	*textOut = m_mungedText;
	*sizeOut = m_mungedSize;
}

void	CGLMEditableTextItem::OpenInEditor( bool foreground )
{
	m_mirror->OpenInEditor( foreground );
}


void	CGLMEditableTextItem::GenHashOfOrigText( void )
{
	// bring this code back if you need the live shader edit/debug mode.
	#if 0
		MD5Context_t md5ctx;
		MD5Init( &md5ctx );
		MD5Update( &md5ctx, (unsigned char*)m_origText, m_origSize ); 
		MD5Final( m_origDigest, &md5ctx );
	#endif
}


void	CGLMEditableTextItem::GenBaseNameAndFullPath(  char *prefix, char *suffix  )
{
	// base name is hash digest in hex, plus the suffix.
	char	temp[5000];

	// bring this code back if you need the live shader edit/debug mode.
	#if 0	
		V_binarytohex( m_origDigest, sizeof(m_origDigest), temp, sizeof( temp ) );
		if (suffix)
		{
			strcat( temp, suffix );
		}
		if (m_mirrorBaseName)	free(m_mirrorBaseName);
		m_mirrorBaseName = strdup( temp );
	
		sprintf( temp, "%s%s", prefix, m_mirrorBaseName );
		if (m_mirrorFullPath)	free(m_mirrorFullPath);
		m_mirrorFullPath = strdup( temp );
	#endif
}


void	CGLMEditableTextItem::GenMungedText( bool fromMirror )
{
	if (fromMirror)
	{
		// just import the text as is from the mirror file.
		
		char	*mirrorData = NULL;
		uint	mirrorSize = 0;

		if (m_mirror->HasData())
		{
			// peek at it, and use it if it is more than some minimum number of bytes.
			m_mirror->GetData( &mirrorData, &mirrorSize );
			
			if (m_mungedText)
			{
				free( m_mungedText );
				m_mungedText = NULL;
			}
			
			m_mungedText = (char *)malloc( mirrorSize+1 );
			m_mungedText[ mirrorSize ] = 0;
			memcpy( m_mungedText, mirrorData, mirrorSize );
			
			m_mungedSize = mirrorSize;
		}
		else
		{
			GLMDebugger();
		}
	}
	else
	{
		#if 1
			// we don't actually clone/munge any more.
			if (m_mungedText)
			{
				free( m_mungedText );
				m_mungedText = NULL;
			}
			
			m_mungedText = (char *)malloc( m_origSize+1 );
			m_mungedText[ m_origSize ] = 0;
			memcpy( m_mungedText, m_origText, m_origSize );
			
			m_mungedSize = m_origSize;
			
		#else
			// take pure 'orig' text that came in from the engine, and clone it
			// do not clone the first line
			char	temp[100000];
			char	*dst = temp;
			char	*lim = &temp[ sizeof(temp) ];
			
			// zero temp
			memset( temp, 0, sizeof(temp) );

			// write orig text to temp
			if (m_origSize >= (sizeof(temp)/2) )
			{
				GLMDebugger();
			}
			
			memcpy( dst, m_origText, m_origSize );
			dst += m_origSize;
			
			// add a newline if the last character wasn't
			if ( (*(dst-1)) != '\n' )
			{
				*dst++ = '\n';
			}
			
			// walk orig text again and copy it over, with these caveats
			// don't copy the first line
			// insert a # before all the other lines.
			char *src = temp;

			// walk to end of first line
			char *firstNewline = strchr( src, '\n' );
			if (!firstNewline)
			{
				GLMDebugger();
			}
			else
			{
				// advance 'src' to that newline- we're not copying the !! line
				src = firstNewline;
			}

			
			// now walk the rest - insert a # after each newline
			while( (dst < lim) && ((src-temp) < m_origSize) )
			{
				switch( *src )
				{
					case '\n':
						*dst++ = *src++;
						*dst++ = '#';
						break;

					default:
						*dst++ = *src++;
				}
			}
			if (dst >= lim)
			{
				GLMDebugger();
			}
			
			// final newline
			*dst++ = '\n';
			
			// copyout
			if (m_mungedText)
			{
				free( m_mungedText );
				m_mungedText = NULL;
			}
			
			m_mungedSize = dst - temp;
			m_mungedText = (char *)malloc( m_mungedSize );
			memcpy( m_mungedText, temp, m_mungedSize );
		#endif
	}
}

//===============================================================================

// class for cracking multi-part text blobs
// sections are demarcated by beginning-of-line markers submitted in a table by the caller
// typically
//		asm flavors have first-line rules so we use those tags as is
//	!!ARBvp (etc)
//	!!ARBfp (etc)
//	//!!GLSLF			// slashes required
//	//!GLSLV

// maybe also introduce "present but disabled" markers like these
//	-!!ARBvp (etc)
//	-!!ARBfp (etc)
//	-//!!GLSLF
//	-//!GLSLV

// resolved.  there is no default section for text that doesn't have a marker in front of it.  mark it or miss it.

CGLMTextSectioner::CGLMTextSectioner( char *text, int textLength, char **markers )
{
	// find lines
	// for each line, see if it starts with a marker
	// if so, open a new section based at that line

	GLMTextSection *curSection = NULL;		// no current section until we see a marker
	
	char *cursor = text;
	char *textLimit = text+textLength;
	
	int foundMarker;
	char **markerCursor;
	while( cursor < textLimit )
	{
		// top of loop.  cursor points to start of a line.
		// find the end of the line and keep that handy.
		char *eol = strchr( cursor, '\n' );
		int charsInLine = (eol) ? (eol-cursor)+1 : strlen(cursor);
		
		//see if any of the marker strings is located here.
		foundMarker = -1;
		markerCursor = markers;
		
		while( (foundMarker<0) && (*markerCursor!=NULL) )
		{
			// see if the n'th marker is a hit
			int markerLen = strlen(*markerCursor);
			
			if (!strncmp( cursor, *markerCursor, markerLen  ) )
			{
				// hit
				foundMarker = markerCursor - markers;
			}
			markerCursor++;
		}
		
		// outcome is either "marker spotted" or "no".
		// if marker seen, open new section using that marker.
		// else, grow active section if underway.
		// then, move cursor to next line.
		
		if (foundMarker >= 0)
		{
			// found marker.  start new section.
			// no need to do anything special with prior section - it was up to date before seeing this marker.
			GLMTextSection temp;

			temp.m_markerIndex	= foundMarker;
			temp.m_textOffset	= cursor - text;	// text includes the marker
			temp.m_textLength	= charsInLine;		// this line goes in the tally, later lines add to it
			
			m_sectionTable.push_back( temp );
			
			curSection = &m_sectionTable[ m_sectionTable.size() - 1 ];
		}
		else
		{
			// add this line to current section if live
			if (curSection)
			{
				curSection->m_textLength += charsInLine;
			}
		}
		cursor += charsInLine;
	}
}

CGLMTextSectioner::~CGLMTextSectioner( )
{
	// not much to do.
}

					
int	CGLMTextSectioner::Count( void )
{
	return m_sectionTable.size();
}

void CGLMTextSectioner::GetSection( int index, uint *offsetOut, uint *lengthOut, int *markerIndexOut )
{
	Assert( index < m_sectionTable.Count() );
	
	GLMTextSection	*section = &m_sectionTable[ index ];
	
	*offsetOut = section->m_textOffset;
	*lengthOut = section->m_textLength;
	*markerIndexOut = section->m_markerIndex;
}

//===============================================================================

// how to make a compiled-in font:
// a. type in a matrix of characters in your fav editor
// b. take a screen shot of the characters (128x128 pixels in this case)
// c. export as TIFF raw.
// d. hex dump it
// e. column-copy just the hex data
// f. find and replace: chop out all the spaces and line feeds, change FFFFFF and 000000 to your marker chars of choice.
// g. wrap each line with quotes and a comma.

unsigned char g_glmDebugFontMap[ 128 * 128 ] = 
{
"                                                                                               *                                "
"                          *                 *      *    *                                      *                                "
"        *    * *   * *   ***   *  *  **     *     *      *    *                               *                                 "
"        *    * *  ***** * * * * * * *  *    *     *      *  * * *   *                         *                                 "
"        *    * *   * *  * *    * *  * *          *        *  ***    *                        *                                  "
"        *         *****  ***    *    *           *        * * * * *****       *****          *                                  "
"        *          * *    * *  * *  * * *        *        *   *     *                       *                                   "
"                        * * * * * * *  *          *      *          *    **           **    *                                   "
"        *                ***  *  *   ** *         *      *               **           **   *                                    "
"                          *                        *    *                 *                *                                    "
"                                                                         *                                                      "
"                                                                                                                                "
"                                                                                                                                "
" ***    *    ***   ***     *  *****  ***  *****  ***   ***                  *        *     ***                                  "
"*   *  **   *   * *   *   **  *     *         * *   * *   *   **   **      *          *   *   *                                 "
"*  **   *       *     *  * *  ****  ****      * *   * *   *   **   **     *   *****    *      *                                 "
"* * *   *      *    **  *  *      * *   *    *   ***  *   *              *              *    *                                  "
"**  *   *     *       * *****     * *   *   *   *   *  ****               *   *****    *    *                                   "
"*   *   *    *    *   *    *  *   * *   *   *   *   *     *   **   **      *          *                                         "
" ***    *   *****  ***     *   ***   ***    *    ***   ***    **   **       *        *      *                                   "
"                                                                    *                                                           "
"                                                                   *                                                            "
"                                                                                                                                "
" ***                                                                                                                            "
"*   *  ***  ****   ***  ****  ***** *****  ***  *   *   *       * *   * *     *   * *   *  ***                                  "
"*   * *   * *   * *   * *   * *     *     *   * *   *   *       * *  *  *     ** ** **  * *   *                                 "
"* * * *   * *   * *     *   * *     *     *     *   *   *       * * *   *     * * * * * * *   *                                 "
"*** * ***** ****  *     *   * ****  ****  *  ** *****   *       * **    *     *   * *  ** *   *                                 "
"* **  *   * *   * *     *   * *     *     *   * *   *   *   *   * * *   *     *   * *   * *   *                                 "
"*     *   * *   * *   * *   * *     *     *   * *   *   *   *   * *  *  *     *   * *   * *   *                                 "
"*   * *   * ****   ***  ****  ***** *      ***  *   *   *    ***  *   * ***** *   * *   *  ***                                  "
" ***                                                                                                                            "
"                                                                                                                                "
"                                                                        *                                                       "
"                                                                    **  *       **     *                                        "
"****   ***  ****   ***  ***** *   * *   * *   * *   * *   * *****   *    *       *    * *                                       "
"*   * *   * *   * *   *   *   *   * *   * *   *  * *  *   *     *   *    *       *   *   *                                      "
"*   * *   * *   * *       *   *   * *   * *   *   *   *   *    *    *     *      *                                              "
"****  *   * ****   ***    *   *   *  * *  *   *   *    * *    *     *     *      *                                              "
"*     *   * *   *     *   *   *   *  * *  * * *   *     *    *      *      *     *                                              "
"*     *   * *   * *   *   *   *   *   *   ** **  * *    *   *       *      *     *                                              "
"*      ***  *   *  ***    *    ***    *   *   * *   *   *   *****   *       *    *        ******                                "
"          *                                                         **      *   **                                              "
"                                                                                                                                "
"                                                                                                                                "
" *                                                                                                                              "
"  *         *               *          **       *       *     *   *       *                                                     "
"   *        *               *         *         *                 *       *                                                     "
"       **** ****   ***   ****  ***   ***   **** ****    *     *   *  *    *   ****  * **   ***                                  "
"      *   * *   * *   * *   * *   *   *   *   * *   *   *     *   * *     *   * * * **  * *   *                                 "
"      *   * *   * *     *   * *****   *   *   * *   *   *     *   ***     *   * * * *   * *   *                                 "
"      *  ** *   * *     *   * *       *   *   * *   *   *     *   *  *    *   * * * *   * *   *                                 "
"       ** * ****   ****  ****  ****   *    **** *   *   *     *   *   *   **  * * * *   *  ***                                  "
"                                              *               *                                                                 "
"                                           ***              **                                                                  "
"                                                                          *                                                     "
"                                                                     **   *   **                                                "
"                          *                                         *     *     *    ** *                                       "
"                          *                                         *     *     *   * **   ***                                  "
"****   **** * **   ****  **** *   * *   * * * * *   * *   * *****   *     *     *         *****                                 "
"*   * *   * **  * *       *   *   * *   * * * *  * *  *   *    *  **      *      **       *****                                 "
"*   * *   * *      ***    *   *   *  * *  * * *   *   *   *   *     *     *     *         *****                                 "
"*   * *   * *         *   *   *  **  * *  * * *  * *  *   *  *      *     *     *          ***                                  "
"****   **** *     ****     **  ** *   *    * *  *   *  **** *****   *     *     *                                               "
"*         *                                               *          **   *   **                                                "
"*         *                                            ***                *                                                     "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
"                                                                                                                                "
};








