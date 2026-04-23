typedef   signed char       int8;
typedef unsigned char      uint8;
typedef   signed short      int16;
typedef unsigned short     uint16;
typedef   signed int        int32;
typedef unsigned int       uint32;
typedef   signed long long  int64;
typedef unsigned long long uint64;
typedef struct { char *data; int64 len; int64 size; } FBSTRING;
typedef int8 boolean;
typedef int64 $13FB_COMPTARGET;
typedef int64 $12FB_SYMBCLASS;
typedef int64 $13FB_SYMBATTRIB;
typedef int64 $13FB_PROCATTRIB;
typedef int64 $12FB_SYMBSTATS;
struct $9FB_SYMBID {
	char* NAME;
	char* ALIAS;
	char* MANGLED;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $9FB_SYMBID ) == 24 );
typedef int64 $11FB_DATATYPE;
struct $8FBSYMBOL;
typedef int64 $13AST_NODECLASS;
union $7FBVALUE {
	struct $8FBSYMBOL* S;
	int64 I;
	double F;
};
__FB_STATIC_ASSERT( sizeof( union $7FBVALUE ) == 8 );
struct $14AST_NODE_CONST {
	union {
		union $7FBVALUE VALUE;
		struct $8FBSYMBOL* S;
		int64 I;
		double F;
	};
	int64 HASSUFFIX;
};
__FB_STATIC_ASSERT( sizeof( struct $14AST_NODE_CONST ) == 16 );
struct $12AST_NODE_VAR {
	int64 OFS;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_VAR ) == 8 );
struct $12AST_NODE_IDX {
	int64 OFS;
	int64 MULT;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_IDX ) == 16 );
struct $12AST_NODE_PTR {
	int64 OFS;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_PTR ) == 8 );
struct $7ASTNODE;
struct $19AST_TMPSTRLIST_ITEM;
struct $19AST_TMPSTRLIST_ITEM {
	struct $8FBSYMBOL* SYM;
	struct $7ASTNODE* SRCTREE;
	struct $19AST_TMPSTRLIST_ITEM* PREV;
};
__FB_STATIC_ASSERT( sizeof( struct $19AST_TMPSTRLIST_ITEM ) == 24 );
struct $13AST_NODE_CALL {
	int64 ISRTL;
	int64 ARGS;
	struct $8FBSYMBOL* CURRARG;
	struct $7ASTNODE* ARGTAIL;
	struct $19AST_TMPSTRLIST_ITEM* STRTAIL;
	struct $8FBSYMBOL* TMPRES;
	struct $7ASTNODE* PROFBEGIN;
	struct $7ASTNODE* PROFEND;
};
__FB_STATIC_ASSERT( sizeof( struct $13AST_NODE_CALL ) == 64 );
struct $12AST_NODE_ARG {
	int64 MODE;
	int64 LGT;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_ARG ) == 16 );
struct $12AST_NODE_IIF {
	struct $8FBSYMBOL* FALSELABEL;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_IIF ) == 8 );
typedef int64 $9AST_OPOPT;
struct $11AST_NODE_OP {
	int64 OP;
	$9AST_OPOPT OPTIONS;
	struct $8FBSYMBOL* EX;
};
__FB_STATIC_ASSERT( sizeof( struct $11AST_NODE_OP ) == 24 );
struct $13AST_NODE_LOAD {
	int64 ISRES;
};
__FB_STATIC_ASSERT( sizeof( struct $13AST_NODE_LOAD ) == 8 );
struct $14AST_NODE_LABEL {
	int64 FLUSH;
};
__FB_STATIC_ASSERT( sizeof( struct $14AST_NODE_LABEL ) == 8 );
struct $13AST_NODE_OFFS {
	int64 OFS;
};
__FB_STATIC_ASSERT( sizeof( struct $13AST_NODE_OFFS ) == 8 );
struct $12AST_NODE_LIT {
	char* TEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_LIT ) == 8 );
typedef int64 $14AST_ASMTOKTYPE;
struct $9ASTASMTOK;
struct $9ASTASMTOK {
	$14AST_ASMTOKTYPE TYPE;
	union {
		struct $8FBSYMBOL* SYM;
		char* TEXT;
	};
	struct $9ASTASMTOK* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $9ASTASMTOK ) == 24 );
struct $12AST_NODE_ASM {
	struct $9ASTASMTOK* TOKHEAD;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_ASM ) == 8 );
struct $14AST_NODE_JMPTB {
	uint64* VALUES;
	struct $8FBSYMBOL** LABELS;
	int64 LABELCOUNT;
	struct $8FBSYMBOL* DEFLABEL;
	uint64 BIAS;
	uint64 SPAN;
};
__FB_STATIC_ASSERT( sizeof( struct $14AST_NODE_JMPTB ) == 48 );
struct $12AST_NODE_DBG {
	int64 EX;
	char* FILENAME;
	int64 OP;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_DBG ) == 24 );
struct $12AST_NODE_MEM {
	int64 OP;
	int64 BYTES;
	int64 FILLCHAR;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_NODE_MEM ) == 24 );
struct $14AST_NODE_STACK {
	int64 OP;
};
__FB_STATIC_ASSERT( sizeof( struct $14AST_NODE_STACK ) == 8 );
struct $16AST_NODE_TYPEINI {
	int64 OFS;
	union {
		int64 BYTES;
		int64 ELEMENTS;
	};
	struct $8FBSYMBOL* SCP;
	struct $8FBSYMBOL* LASTSCP;
};
__FB_STATIC_ASSERT( sizeof( struct $16AST_NODE_TYPEINI ) == 32 );
struct $21AST_NODE_TYPEINISCOPE {
	int64 IS_ARRAY;
};
__FB_STATIC_ASSERT( sizeof( struct $21AST_NODE_TYPEINISCOPE ) == 8 );
struct $13AST_BREAKLIST {
	struct $7ASTNODE* HEAD;
	struct $7ASTNODE* TAIL;
};
__FB_STATIC_ASSERT( sizeof( struct $13AST_BREAKLIST ) == 16 );
struct $13AST_NODE_PROC {
	int64 ISMAIN;
	struct $7ASTNODE* DECL_LAST;
};
__FB_STATIC_ASSERT( sizeof( struct $13AST_NODE_PROC ) == 16 );
struct $14AST_NODE_BLOCK {
	struct $7ASTNODE* PARENT;
	int64 INISTMT;
	int64 ENDSTMT;
	struct $8FBSYMBOL* INITLABEL;
	struct $8FBSYMBOL* EXITLABEL;
	struct $13AST_BREAKLIST BREAKLIST;
	struct $13AST_NODE_PROC PROC;
};
__FB_STATIC_ASSERT( sizeof( struct $14AST_NODE_BLOCK ) == 72 );
struct $14AST_NODE_BREAK {
	struct $7ASTNODE* PARENT;
	int64 SCOPE;
	int64 LINENUM;
	int64 STMTNUM;
};
__FB_STATIC_ASSERT( sizeof( struct $14AST_NODE_BREAK ) == 32 );
struct $17AST_NODE_DATASTMT {
	union {
		int64 ID;
		int64 ELMTS;
	};
};
__FB_STATIC_ASSERT( sizeof( struct $17AST_NODE_DATASTMT ) == 8 );
struct $13AST_NODE_LINK {
	int64 RET;
};
__FB_STATIC_ASSERT( sizeof( struct $13AST_NODE_LINK ) == 8 );
struct $13AST_NODE_CAST {
	int64 DOCONV;
	int64 DO_CONVFD2FS;
	int64 CONVCONST;
};
__FB_STATIC_ASSERT( sizeof( struct $13AST_NODE_CAST ) == 24 );
struct $7ASTNODE {
	$13AST_NODECLASS CLASS;
	int64 DTYPE;
	struct $8FBSYMBOL* SUBTYPE;
	struct $8FBSYMBOL* SYM;
	int64 VECTOR;
	union {
		struct $14AST_NODE_CONST VAL;
		struct $12AST_NODE_VAR VAR_;
		struct $12AST_NODE_IDX IDX;
		struct $12AST_NODE_PTR PTR;
		struct $13AST_NODE_CALL CALL;
		struct $12AST_NODE_ARG ARG;
		struct $12AST_NODE_IIF IIF;
		struct $11AST_NODE_OP OP;
		struct $13AST_NODE_LOAD LOD;
		struct $14AST_NODE_LABEL LBL;
		struct $13AST_NODE_OFFS OFS;
		struct $12AST_NODE_LIT LIT;
		struct $12AST_NODE_ASM ASM;
		struct $14AST_NODE_JMPTB JMPTB;
		struct $12AST_NODE_DBG DBG;
		struct $12AST_NODE_MEM MEM;
		struct $14AST_NODE_STACK STACK;
		struct $16AST_NODE_TYPEINI TYPEINI;
		struct $21AST_NODE_TYPEINISCOPE TYPEINISCOPE;
		struct $14AST_NODE_BLOCK BLOCK;
		struct $14AST_NODE_BREAK BREAK;
		struct $17AST_NODE_DATASTMT DATA;
		struct $13AST_NODE_LINK LINK;
		struct $13AST_NODE_CAST CAST;
	};
	struct $7ASTNODE* L;
	struct $7ASTNODE* R;
	struct $7ASTNODE* PREV;
	struct $7ASTNODE* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $7ASTNODE ) == 144 );
struct $10FBARRAYDIM {
	int64 LOWER;
	int64 UPPER;
};
__FB_STATIC_ASSERT( sizeof( struct $10FBARRAYDIM ) == 16 );
struct $9FBS_ARRAY {
	int64 DIMENSIONS;
	struct $10FBARRAYDIM* DIMTB;
	int64 DIFF;
	int64 ELEMENTS;
	struct $8FBSYMBOL* DESC;
	struct $8FBSYMBOL* DESCTYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $9FBS_ARRAY ) == 48 );
struct $10FBVAR_DESC {
	struct $8FBSYMBOL* ARRAY;
};
__FB_STATIC_ASSERT( sizeof( struct $10FBVAR_DESC ) == 8 );
struct $10FBVAR_DATA {
	struct $8FBSYMBOL* PREV;
};
__FB_STATIC_ASSERT( sizeof( struct $10FBVAR_DATA ) == 8 );
struct $7FBS_VAR {
	union {
		char* LITTEXT;
		uint32* LITTEXTW;
		struct $7ASTNODE* INITREE;
	};
	struct $9FBS_ARRAY ARRAY;
	struct $10FBVAR_DESC DESC;
	int64 STMTNUM;
	int64 ALIGN;
	struct $10FBVAR_DATA DATA;
	int64 BITPOS;
	int64 BITS;
};
__FB_STATIC_ASSERT( sizeof( struct $7FBS_VAR ) == 104 );
struct $9FBS_CONST {
	union {
		union $7FBVALUE VALUE;
		struct $8FBSYMBOL* S;
		int64 I;
		double F;
	};
	int64 HASSUFFIX;
};
__FB_STATIC_ASSERT( sizeof( struct $9FBS_CONST ) == 16 );
struct $10FBSYMBOLTB {
	struct $8FBSYMBOL* OWNER;
	struct $8FBSYMBOL* HEAD;
	struct $8FBSYMBOL* TAIL;
};
__FB_STATIC_ASSERT( sizeof( struct $10FBSYMBOLTB ) == 24 );
struct $8HASHITEM;
struct $8HASHITEM {
	char* NAME;
	void* DATA;
	struct $8HASHITEM* PREV;
	struct $8HASHITEM* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $8HASHITEM ) == 32 );
struct $8HASHLIST {
	struct $8HASHITEM* HEAD;
	struct $8HASHITEM* TAIL;
};
__FB_STATIC_ASSERT( sizeof( struct $8HASHLIST ) == 16 );
struct $5THASH {
	struct $8HASHLIST* LIST;
	int64 NODES;
	int64 DELSTR;
};
__FB_STATIC_ASSERT( sizeof( struct $5THASH ) == 24 );
struct $8FBHASHTB;
struct $8FBHASHTB {
	struct $8FBSYMBOL* OWNER;
	struct $5THASH TB;
	struct $8FBHASHTB* PREV;
	struct $8FBHASHTB* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $8FBHASHTB ) == 48 );
struct $9FBSYMLIST {
	struct $8FBSYMBOL* HEAD;
	struct $8FBSYMBOL* TAIL;
};
__FB_STATIC_ASSERT( sizeof( struct $9FBSYMLIST ) == 16 );
struct $10FBSYMCHAIN;
struct $10FBSYMCHAIN {
	struct $8FBSYMBOL* SYM;
	struct $10FBSYMCHAIN* NEXT;
	int64 ISIMPORT;
	struct $10FBSYMCHAIN* PREV;
	struct $8HASHITEM* ITEM;
	struct $10FBSYMCHAIN* IMP_NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $10FBSYMCHAIN ) == 48 );
struct $13FBNAMESPC_EXT {
	struct $9FBSYMLIST IMPLIST;
	struct $9FBSYMLIST EXPLIST;
	int64 CNT;
	struct $10FBSYMCHAIN* IMPSYM_HEAD;
	struct $10FBSYMCHAIN* IMPSYM_TAIL;
};
__FB_STATIC_ASSERT( sizeof( struct $13FBNAMESPC_EXT ) == 56 );
struct $9FBNAMESPC {
	struct $10FBSYMBOLTB SYMTB;
	struct $8FBHASHTB HASHTB;
	struct $13FBNAMESPC_EXT* EXT;
};
__FB_STATIC_ASSERT( sizeof( struct $9FBNAMESPC ) == 80 );
typedef int64 $15FB_STRUCT_INREG;
struct $13FB_STRUCT_DBG {
	int64 TYPENUM;
};
__FB_STATIC_ASSERT( sizeof( struct $13FB_STRUCT_DBG ) == 8 );
struct $12FB_STRUCTEXT {
	struct $8FBSYMBOL* CTORHEAD;
	struct $8FBSYMBOL* DEFCTOR;
	struct $8FBSYMBOL* COPYCTOR;
	struct $8FBSYMBOL* COPYCTORCONST;
	struct $8FBSYMBOL* DTOR1;
	struct $8FBSYMBOL* DTOR0;
	struct $8FBSYMBOL* COPYLETOP;
	struct $8FBSYMBOL* COPYLETOPCONST;
	struct $8FBSYMBOL* OPOVLTB[28];
	int64 VTABLEELEMENTS;
	struct $8FBSYMBOL* VTABLE;
	struct $8FBSYMBOL* RTTI;
	int64 ABSTRACTCOUNT;
};
__FB_STATIC_ASSERT( sizeof( struct $12FB_STRUCTEXT ) == 320 );
struct $10FBS_STRUCT {
	struct $9FBNAMESPC NS;
	struct $8FBSYMBOL* BASE;
	struct $8FBSYMBOL* ANONPARENT;
	int64 NATALIGN;
	int64 UNPADLGT;
	int32 OPTIONS;
	uint8 BITPOS;
	uint8 ALIGN;
	$11FB_DATATYPE RETDTYPE;
	$15FB_STRUCT_INREG RETIN2REGS;
	struct $13FB_STRUCT_DBG DBG;
	struct $12FB_STRUCTEXT* EXT;
};
__FB_STATIC_ASSERT( sizeof( struct $10FBS_STRUCT ) == 152 );
struct $8FBS_ENUM {
	struct $9FBNAMESPC NS;
	int64 ELEMENTS;
	struct $13FB_STRUCT_DBG DBG;
};
__FB_STATIC_ASSERT( sizeof( struct $8FBS_ENUM ) == 96 );
typedef int64 $11FB_FUNCMODE;
typedef int64 $21FB_PROC_RETURN_METHOD;
typedef int64 (*tmp$34)( struct $8FBSYMBOL* );
struct $10FB_PROCRTL {
	tmp$34 CALLBACK;
};
__FB_STATIC_ASSERT( sizeof( struct $10FB_PROCRTL ) == 8 );
struct $10FB_PROCOVL {
	int16 MINPARAMS;
	int16 MAXPARAMS;
	struct $8FBSYMBOL* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $10FB_PROCOVL ) == 16 );
struct $10FB_PROCSTK {
	int64 ARGOFS;
	int64 LOCALOFS;
	int64 LOCALMAX;
};
__FB_STATIC_ASSERT( sizeof( struct $10FB_PROCSTK ) == 24 );
struct $10FB_PROCDBG {
	int64 INILINE;
	int64 ENDLINE;
	char* INCFILE;
};
__FB_STATIC_ASSERT( sizeof( struct $10FB_PROCDBG ) == 24 );
struct $10FB_PROCERR {
	struct $8FBSYMBOL* LASTHND;
	struct $8FBSYMBOL* LASTMOD;
	struct $8FBSYMBOL* LASTFUN;
};
__FB_STATIC_ASSERT( sizeof( struct $10FB_PROCERR ) == 24 );
typedef int64 $6AST_OP;
struct $12FB_PROCOPOVL {
	$6AST_OP OP;
};
__FB_STATIC_ASSERT( sizeof( struct $12FB_PROCOPOVL ) == 8 );
struct $7TLISTTB;
struct $7TLISTTB {
	struct $7TLISTTB* NEXT;
	void* NODETB;
	int64 NODES;
};
__FB_STATIC_ASSERT( sizeof( struct $7TLISTTB ) == 24 );
struct $9TLISTNODE;
struct $9TLISTNODE {
	struct $9TLISTNODE* PREV;
	struct $9TLISTNODE* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $9TLISTNODE ) == 16 );
typedef int64 $10LIST_FLAGS;
struct $5TLIST {
	struct $7TLISTTB* TBHEAD;
	struct $7TLISTTB* TBTAIL;
	int64 NODES;
	int64 NODELEN;
	struct $9TLISTNODE* FHEAD;
	void* HEAD;
	void* TAIL;
	$10LIST_FLAGS FLAGS;
};
__FB_STATIC_ASSERT( sizeof( struct $5TLIST ) == 64 );
typedef int64 $12FB_PROCSTATS;
struct $10FB_PROCGSB {
	struct $8FBSYMBOL* CTX;
};
__FB_STATIC_ASSERT( sizeof( struct $10FB_PROCGSB ) == 8 );
struct $10FB_PROCEXT {
	struct $8FBSYMBOL* RES;
	struct $10FB_PROCSTK STK;
	struct $10FB_PROCDBG DBG;
	struct $10FB_PROCERR ERR;
	struct $12FB_PROCOPOVL OPOVL;
	struct $5TLIST* STATDTOR;
	$12FB_PROCSTATS STATS;
	int64 STMTNUM;
	int64 PRIORITY;
	struct $10FB_PROCGSB GOSUB;
	struct $7ASTNODE* BASE_INITREE;
	int64 VTABLEINDEX;
	struct $8FBSYMBOL* OVERRIDDEN;
};
__FB_STATIC_ASSERT( sizeof( struct $10FB_PROCEXT ) == 152 );
struct $8FBS_PROC {
	struct $10FBSYMBOLTB SYMTB;
	int16 PARAMS;
	int16 OPTPARAMS;
	struct $10FBSYMBOLTB PARAMTB;
	$11FB_FUNCMODE MODE;
	$11FB_DATATYPE REALDTYPE;
	struct $8FBSYMBOL* REALSUBTYPE;
	$21FB_PROC_RETURN_METHOD RETURNMETHOD;
	struct $10FB_PROCRTL RTL;
	struct $10FB_PROCOVL OVL;
	struct $10FB_PROCEXT* EXT;
};
__FB_STATIC_ASSERT( sizeof( struct $8FBS_PROC ) == 120 );
typedef int64 $12FB_PARAMMODE;
struct $9FBS_PARAM {
	$12FB_PARAMMODE MODE;
	struct $8FBSYMBOL* VAR;
	struct $7ASTNODE* OPTEXPR;
	int64 BYDESCDIMENSIONS;
	struct $8FBSYMBOL* BYDESCREALSUBTYPE;
	int64 REGNUM;
};
__FB_STATIC_ASSERT( sizeof( struct $9FBS_PARAM ) == 48 );
struct $9FBS_LABEL {
	struct $8FBSYMBOL* PARENT;
	int64 DECLARED;
	int64 STMTNUM;
	boolean GOSUB;
};
__FB_STATIC_ASSERT( sizeof( struct $9FBS_LABEL ) == 32 );
struct $11FB_DEFPARAM;
struct $11FB_DEFPARAM {
	char* NAME;
	int64 NUM;
	struct $11FB_DEFPARAM* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $11FB_DEFPARAM ) == 24 );
typedef int64 $14FB_DEFTOK_TYPE;
struct $9FB_DEFTOK;
struct $9FB_DEFTOK {
	$14FB_DEFTOK_TYPE TYPE;
	union {
		char* TEXT;
		uint32* TEXTW;
		int64 PARAMNUM;
	};
	struct $9FB_DEFTOK* PREV;
	struct $9FB_DEFTOK* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $9FB_DEFTOK ) == 32 );
typedef int64 $15FB_DEFINE_FLAGS;
typedef FBSTRING* (*tmp$28)( void );
struct $8DZSTRING {
	char* DATA;
	int64 LEN;
	int64 SIZE;
};
__FB_STATIC_ASSERT( sizeof( struct $8DZSTRING ) == 24 );
struct $8DWSTRING {
	uint32* DATA;
	int64 LEN;
	int64 SIZE;
};
__FB_STATIC_ASSERT( sizeof( struct $8DWSTRING ) == 24 );
struct $9LEXPP_ARG {
	union {
		struct $8DZSTRING TEXT;
		struct $8DWSTRING TEXTW;
	};
};
__FB_STATIC_ASSERT( sizeof( struct $9LEXPP_ARG ) == 24 );
struct $11LEXPP_ARGTB {
	struct $9LEXPP_ARG TB[32];
	int64 COUNT;
};
__FB_STATIC_ASSERT( sizeof( struct $11LEXPP_ARGTB ) == 776 );
typedef FBSTRING* (*tmp$29)( struct $11LEXPP_ARGTB*, int64* );
typedef uint32* (*tmp$30)( struct $11LEXPP_ARGTB*, int64* );
struct $10FBS_DEFINE {
	int64 PARAMS;
	struct $11FB_DEFPARAM* PARAMHEAD;
	union {
		struct $9FB_DEFTOK* TOKHEAD;
		char* TEXT;
		uint32* TEXTW;
	};
	int64 ISARGLESS;
	$15FB_DEFINE_FLAGS FLAGS;
	union {
		tmp$28 DPROCZ;
		tmp$29 MPROCZ;
	};
	union {
		tmp$30 MPROCW;
	};
};
__FB_STATIC_ASSERT( sizeof( struct $10FBS_DEFINE ) == 56 );
typedef int64 $10FB_TKCLASS;
struct $11FBS_KEYWORD {
	int64 ID;
	$10FB_TKCLASS TKCLASS;
};
__FB_STATIC_ASSERT( sizeof( struct $11FBS_KEYWORD ) == 16 );
struct $8FBFWDREF;
struct $8FBFWDREF {
	struct $8FBSYMBOL* REF;
	struct $8FBFWDREF* PREV;
};
__FB_STATIC_ASSERT( sizeof( struct $8FBFWDREF ) == 16 );
struct $10FBS_FWDREF {
	struct $8FBFWDREF* TAIL;
};
__FB_STATIC_ASSERT( sizeof( struct $10FBS_FWDREF ) == 8 );
struct $11FB_SCOPEDBG {
	int64 INILINE;
	int64 ENDLINE;
	struct $8FBSYMBOL* INILABEL;
	struct $8FBSYMBOL* ENDLABEL;
};
__FB_STATIC_ASSERT( sizeof( struct $11FB_SCOPEDBG ) == 32 );
struct $12FB_SCOPEEMIT {
	int64 BASEOFS;
};
__FB_STATIC_ASSERT( sizeof( struct $12FB_SCOPEEMIT ) == 8 );
struct $9FBS_SCOPE {
	struct $7ASTNODE* BACKNODE;
	struct $10FBSYMBOLTB SYMTB;
	struct $11FB_SCOPEDBG DBG;
	struct $12FB_SCOPEEMIT EMIT;
};
__FB_STATIC_ASSERT( sizeof( struct $9FBS_SCOPE ) == 72 );
struct $13FBS_NAMESPACE {
	struct $9FBNAMESPC NS;
	int64 CNT;
	struct $8FBSYMBOL* LAST_TAIL;
};
__FB_STATIC_ASSERT( sizeof( struct $13FBS_NAMESPACE ) == 96 );
struct $12FBS_NSIMPORT {
	struct $8FBSYMBOL* IMP_NS;
	struct $8FBSYMBOL* IMP_PREV;
	struct $8FBSYMBOL* IMP_NEXT;
	struct $8FBSYMBOL* EXP_NS;
	struct $8FBSYMBOL* EXP_PREV;
	struct $8FBSYMBOL* EXP_NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $12FBS_NSIMPORT ) == 48 );
struct $9FBSYMHASH {
	struct $8FBHASHTB* TB;
	struct $8HASHITEM* ITEM;
	uint64 INDEX;
	struct $8FBSYMBOL* PREV;
	struct $8FBSYMBOL* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $9FBSYMHASH ) == 40 );
struct $8FBSYMBOL {
	$12FB_SYMBCLASS CLASS;
	$13FB_SYMBATTRIB ATTRIB;
	$13FB_PROCATTRIB PATTRIB;
	$12FB_SYMBSTATS STATS;
	struct $9FB_SYMBID ID;
	$11FB_DATATYPE TYP;
	struct $8FBSYMBOL* SUBTYPE;
	uint16 SCOPE;
	int16 MANGLING;
	int64 LGT;
	int64 OFS;
	union {
		struct $7FBS_VAR VAR_;
		struct $9FBS_CONST VAL;
		struct $10FBS_STRUCT UDT;
		struct $8FBS_ENUM ENUM_;
		struct $8FBS_PROC PROC;
		struct $9FBS_PARAM PARAM;
		struct $9FBS_LABEL LBL;
		struct $10FBS_DEFINE DEF;
		struct $11FBS_KEYWORD KEY;
		struct $10FBS_FWDREF FWD;
		struct $9FBS_SCOPE SCP;
		struct $13FBS_NAMESPACE NSPC;
		struct $12FBS_NSIMPORT NSIMP;
	};
	struct $9FBSYMHASH HASH;
	struct $10FBSYMBOLTB* SYMTB;
	struct $8FBSYMBOL* PARENT;
	struct $8FBSYMBOL* PREV;
	struct $8FBSYMBOL* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $8FBSYMBOL ) == 320 );
typedef void (*tmp$112)( int64, int64 );
struct $7FBTOKEN;
struct $7FBTOKEN {
	int64 ID;
	int64 CLASS;
	int64 DTYPE;
	union {
		char TEXT[1025];
		uint32 TEXTW[1025];
	};
	int64 LEN;
	struct $10FBSYMCHAIN* SYM_CHAIN;
	union {
		int64 PRDPOS;
		int64 HASESC;
		int64 HASSUFFIX;
	};
	int64 SUFFIXCHAR;
	int64 AFTER_SPACE;
	struct $7FBTOKEN* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $7FBTOKEN ) == 4176 );
typedef int64 $17LEX_TKCTX_CONTEXT;
struct $9LEX_TKCTX {
	struct $7FBTOKEN TOKENTB[4];
	int64 K;
	struct $7FBTOKEN* HEAD;
	struct $7FBTOKEN* TAIL;
	uint64 CURRCHAR;
	uint64 LAHDCHAR1;
	uint64 LAHDCHAR2;
	int64 LINENUM;
	int64 LASTTK_ID;
	int64 RECLEVEL;
	struct $8FBSYMBOL* CURRMACRO;
	struct $8FBSYMBOL* KWDNS;
	$17LEX_TKCTX_CONTEXT KIND;
	int64 DEFLEN;
	union {
		struct {
			char* DEFPTR;
			struct $8DZSTRING DEFTEXT;
		};
		struct {
			uint32* DEFPTRW;
			struct $8DWSTRING DEFTEXTW;
		};
	};
	int64 BUFFLEN;
	union {
		struct {
			char* BUFFPTR;
			char BUFF[8193];
		};
		struct {
			uint32* BUFFPTRW;
			uint32 BUFFW[8193];
		};
	};
	int64 FILEPOS;
	int64 LASTFILEPOS;
	int64 PHYSFILEPOS;
	struct $8DZSTRING CURRLINE;
	int64 AFTER_SPACE;
};
__FB_STATIC_ASSERT( sizeof( struct $9LEX_TKCTX ) == 49688 );
struct $11TSTRSETITEM {
	FBSTRING S;
	int64 USERDATA;
	struct $8HASHITEM* HASHITEM;
};
__FB_STATIC_ASSERT( sizeof( struct $11TSTRSETITEM ) == 40 );
typedef int64 $12FB_DATACLASS;
struct $13SYMB_DATATYPE {
	$12FB_DATACLASS CLASS;
	int64 SIZE;
	int64 SIGNED;
	int64 INTRANK;
	$11FB_DATATYPE REMAPTYPE;
	int64 SIZETYPE;
	char* NAME;
};
__FB_STATIC_ASSERT( sizeof( struct $13SYMB_DATATYPE ) == 56 );
FBSTRING* fb_StrInit( void*, int64, void*, int64, int32 );
FBSTRING* fb_StrAssign( void*, int64, void*, int64, int32 );
void fb_StrDelete( FBSTRING* );
FBSTRING* fb_StrConcat( FBSTRING*, void*, int64, void*, int64 );
FBSTRING* fb_StrConcatAssign( void*, int64, void*, int64, int32 );
FBSTRING* fb_StrAllocTempResult( FBSTRING* );
FBSTRING* fb_LongintToStr( int64 );
FBSTRING* fb_ULongintToStr( uint64 );
static void fb_ctor__edbg_stab( void ) __attribute__(( constructor ));
static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM*, struct $11TSTRSETITEM* );
FBSTRING* FBGETENTRYPOINT( void );
typedef int64 $10FB_SYMBOPT;
struct $8FBSYMBOL* SYMBADDLABEL( char*, $10FB_SYMBOPT );
int64 SYMBGETREALSIZE( struct $8FBSYMBOL* );
char* SYMBUNIQUELABEL( void );
char* SYMBGETMANGLEDNAME( struct $8FBSYMBOL* );
char* SYMBGETDBGNAME( struct $8FBSYMBOL* );
struct $8FBSYMBOL* SYMBUDTGETFIRSTFIELD( struct $8FBSYMBOL* );
struct $8FBSYMBOL* SYMBUDTGETNEXTFIELD( struct $8FBSYMBOL* );
struct $8FBSYMBOL* SYMBGETENUMFIRSTELM( struct $8FBSYMBOL* );
struct $8FBSYMBOL* SYMBGETENUMNEXTELM( struct $8FBSYMBOL* );
int64 PATHISABSOLUTE( char* );
FBSTRING* HCURDIR( void );
char* HESCAPE( char*, int64 );
void EMITWRITESTR( char*, int64 );
void EDBGEMITLINE( struct $8FBSYMBOL*, int64, struct $8FBSYMBOL* );
void EDBGEMITPROCARG( struct $8FBSYMBOL* );
void EDBGEMITLOCALVAR( struct $8FBSYMBOL*, int64 );
void EDBGINCLUDE( char* );
static void HDECLUDT( struct $8FBSYMBOL*, int64 );
static void HDECLENUM( struct $8FBSYMBOL* );
static FBSTRING* HDECLPOINTER( int64* );
static FBSTRING* HGETDATATYPE( struct $8FBSYMBOL*, int64 );
static void HEMITSTABS( int64, char*, int64, int64, char* );
static char* HMAKESTABN( int64, int64, int64, char* );
static void HEMITSTABN( int64, int64, int64, char* );
static void HEMITSTABD( int64, int64, int64 );
static void HSTABLABEL( char* );
static void HDECLARGS( struct $8FBSYMBOL* );
static void HDECLLOCALVARS( struct $8FBSYMBOL*, struct $8FBSYMBOL*, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef int64 $11AST_OPFLAGS;
struct $10AST_OPINFO {
	$13AST_NODECLASS CLASS;
	$11AST_OPFLAGS FLAGS;
	char* ID;
	$6AST_OP SELFOP;
};
__FB_STATIC_ASSERT( sizeof( struct $10AST_OPINFO ) == 32 );
struct $16__FB_ARRAYDIMTB$ {
	int64 ELEMENTS;
	int64 LBOUND;
	int64 UBOUND;
};
__FB_STATIC_ASSERT( sizeof( struct $16__FB_ARRAYDIMTB$ ) == 24 );
struct $8FBARRAY1I10AST_OPINFOE {
	struct $10AST_OPINFO* DATA;
	struct $10AST_OPINFO* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1I10AST_OPINFOE ) == 72 );
static struct $8FBARRAY1I10AST_OPINFOE tmp$82$;
extern struct $13SYMB_DATATYPE SYMB_DTYPETB$[26];
struct $8FBARRAY1I13SYMB_DATATYPEE {
	struct $13SYMB_DATATYPE* DATA;
	struct $13SYMB_DATATYPE* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1I13SYMB_DATATYPEE ) == 72 );
static struct $8FBARRAY1I13SYMB_DATATYPEE tmp$83$;
struct $8FBARRAY2IlE {
	int64* DATA;
	int64* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[2];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY2IlE ) == 96 );
static struct $8FBARRAY2IlE tmp$84$;
typedef int64 $10FB_OUTTYPE;
typedef int64 $10FB_BACKEND;
typedef int64 $10FB_CPUTYPE;
typedef int64 $10FB_FPUTYPE;
typedef int64 $9FB_FPMODE;
typedef int64 $17FB_VECTORIZELEVEL;
typedef int64 $12FB_ASMSYNTAX;
typedef int64 $7FB_LANG;
typedef int64 $10FB_PDCHECK;
typedef int64 $11FB_MODEVIEW;
struct $12FBCMMLINEOPT {
	$10FB_OUTTYPE OUTTYPE;
	int64 PPONLY;
	$10FB_BACKEND BACKEND;
	$13FB_COMPTARGET TARGET;
	$10FB_CPUTYPE CPUTYPE;
	$10FB_FPUTYPE FPUTYPE;
	$9FB_FPMODE FPMODE;
	$17FB_VECTORIZELEVEL VECTORIZE;
	int64 OPTLEVEL;
	$12FB_ASMSYNTAX ASMSYNTAX;
	$7FB_LANG LANG;
	int64 FORCELANG;
	int64 DEBUG;
	int64 DEBUGINFO;
	int64 ASSERTIONS;
	int64 ERRORCHECK;
	int64 RESUMEERR;
	int64 EXTRAERRCHK;
	int64 ERRLOCATION;
	int64 ARRAYBOUNDCHK;
	int64 ARRAYDIMSCHK;
	int64 NULLPTRCHK;
	int64 UNWINDINFO;
	int64 PROFILE;
	int64 WARNINGLEVEL;
	int64 SHOWERROR;
	int64 MAXERRORS;
	$10FB_PDCHECK PDCHECKOPT;
	int64 GOSUBSETJMP;
	int64 VALISTASPTR;
	int64 NOTHISCALL;
	int64 NOFASTCALL;
	int64 FBRT;
	int64 EXPORT;
	int64 MSBITFIELDS;
	int64 MULTITHREADED;
	int64 FBGFX;
	int64 FBSFX;
	int64 PIC;
	int64 STACKSIZE;
	int64 OBJINFO;
	int64 SHOWINCLUDES;
	$11FB_MODEVIEW MODEVIEW;
	int64 NOCMDLINE;
	int64 RETURNINFLTS;
	int64 NOBUILTINS;
	int64 OPTABSTRACT;
};
__FB_STATIC_ASSERT( sizeof( struct $12FBCMMLINEOPT ) == 376 );
typedef int64 $12FB_TARGETOPT;
struct $8FBTARGET {
	char* ID;
	$11FB_DATATYPE WCHAR;
	$11FB_FUNCMODE FBCALL;
	$11FB_FUNCMODE STDCALL;
	$12FB_TARGETOPT OPTIONS;
};
__FB_STATIC_ASSERT( sizeof( struct $8FBTARGET ) == 40 );
typedef int64 $12FB_WCHARCONV;
typedef int64 $13FBFILE_FORMAT;
struct $6FBFILE {
	int64 NUM;
	char NAME[261];
	char* INCFILE;
	int64 ISMAIN;
	$13FBFILE_FORMAT FORMAT;
};
__FB_STATIC_ASSERT( sizeof( struct $6FBFILE ) == 296 );
struct $6FBMAIN {
	struct $8FBSYMBOL* PROC;
	struct $7ASTNODE* INITNODE;
};
__FB_STATIC_ASSERT( sizeof( struct $6FBMAIN ) == 16 );
typedef int64 $11FB_LANG_OPT;
struct $11FB_LANG_CTX {
	$11FB_LANG_OPT OPT;
	$11FB_DATATYPE INTEGERKEYWORDDTYPE;
	$11FB_DATATYPE INT15LITERALDTYPE;
	$11FB_DATATYPE INT16LITERALDTYPE;
	$11FB_DATATYPE INT31LITERALDTYPE;
	$11FB_DATATYPE INT32LITERALDTYPE;
	$11FB_DATATYPE INT63LITERALDTYPE;
	$11FB_DATATYPE INT64LITERALDTYPE;
	$11FB_DATATYPE FLOATLITERALDTYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $11FB_LANG_CTX ) == 72 );
struct $8FBOPTION {
	int64 BASE;
	int64 PARAMMODE;
	int64 EXPLICIT;
	int64 PROCPUBLIC;
	int64 PROCPROFILE;
	int64 ESCAPESTR;
	int64 DYNAMIC;
	int64 GOSUB;
};
__FB_STATIC_ASSERT( sizeof( struct $8FBOPTION ) == 64 );
typedef int64 $16FB_RESTART_FLAGS;
struct $7TSTRSET {
	struct $5TLIST LIST;
	struct $5THASH HASH;
};
__FB_STATIC_ASSERT( sizeof( struct $7TSTRSET ) == 88 );
struct $5FBENV {
	struct $5TLIST PREDEFINES;
	struct $5TLIST PREINCLUDES;
	struct $5TLIST INCLUDEPATHS;
	struct $12FBCMMLINEOPT CLOPT;
	struct $8FBTARGET TARGET;
	$12FB_WCHARCONV WCHARCONV;
	int64 UNDERSCOREPREFIX;
	int64 POINTERSIZE;
	struct $6FBFILE INF;
	struct $6FBFILE OUTF;
	int64 PPFILE_NUM;
	struct $5THASH FILENAMEHASH;
	struct $5THASH INCFILEHASH;
	struct $5THASH INCONCEHASH;
	int64 INCLUDEREC;
	char ENTRY[128];
	struct $6FBMAIN MAIN;
	struct $11FB_LANG_CTX LANG;
	struct $8FBOPTION OPT;
	int64 INITED;
	int64 MODULE_COUNT;
	$16FB_RESTART_FLAGS RESTART_REQUEST;
	$16FB_RESTART_FLAGS RESTART_ACTION;
	$16FB_RESTART_FLAGS RESTART_STATUS;
	int64 RESTART_COUNT;
	$7FB_LANG RESTART_LANG;
	struct $7TSTRSET LIBS;
	struct $7TSTRSET LIBPATHS;
	int64 FBCTINF_STARTED;
};
__FB_STATIC_ASSERT( sizeof( struct $5FBENV ) == 1832 );
extern struct $5FBENV ENV$;
struct $7LEX_CTX {
	struct $9LEX_TKCTX CTXTB[17];
	struct $9LEX_TKCTX* CTX;
	int64 INSIDEMACRO;
};
__FB_STATIC_ASSERT( sizeof( struct $7LEX_CTX ) == 844712 );
extern struct $7LEX_CTX LEX$;
struct $8REGCLASS;
typedef int64 $15IRVREGTYPE_ENUM;
typedef int64 $12IR_REGFAMILY;
struct $6IRVREG;
struct $9IRTACVREG;
struct $9IRTACVREG {
	struct $6IRVREG* VREG;
	struct $6IRVREG* PARENT;
	struct $9IRTACVREG* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $9IRTACVREG ) == 24 );
struct $13IRTACVREG_GRP {
	struct $9IRTACVREG REG;
	struct $9IRTACVREG IDX;
	struct $9IRTACVREG AUX;
};
__FB_STATIC_ASSERT( sizeof( struct $13IRTACVREG_GRP ) == 72 );
struct $5IRTAC {
	int64 POS;
	$6AST_OP OP;
	struct $13IRTACVREG_GRP VR;
	struct $13IRTACVREG_GRP V1;
	struct $13IRTACVREG_GRP V2;
	struct $8FBSYMBOL* EX1;
	int64 EX2;
	char* EX3;
};
__FB_STATIC_ASSERT( sizeof( struct $5IRTAC ) == 256 );
struct $6IRVREG {
	$15IRVREGTYPE_ENUM TYP;
	$11FB_DATATYPE DTYPE;
	struct $8FBSYMBOL* SUBTYPE;
	int64 REG;
	$12IR_REGFAMILY REGFAMILY;
	int64 VECTOR;
	union $7FBVALUE VALUE;
	struct $8FBSYMBOL* SYM;
	int64 OFS;
	int64 MULT;
	struct $6IRVREG* VIDX;
	struct $6IRVREG* VAUX;
	struct $9IRTACVREG* TACVHEAD;
	struct $9IRTACVREG* TACVTAIL;
	struct $5IRTAC* TACLAST;
};
__FB_STATIC_ASSERT( sizeof( struct $6IRVREG ) == 120 );
typedef int64 (*tmp$94)( struct $8REGCLASS*, struct $6IRVREG*, struct $6IRVREG*, uint64 );
typedef int64 (*tmp$95)( struct $8REGCLASS*, int64, struct $6IRVREG*, struct $6IRVREG* );
typedef void (*tmp$96)( struct $8REGCLASS*, int64 );
typedef int64 (*tmp$97)( struct $8REGCLASS*, int64 );
typedef void (*tmp$98)( struct $8REGCLASS*, int64, struct $6IRVREG*, struct $6IRVREG* );
typedef int64 (*tmp$99)( struct $8REGCLASS* );
typedef struct $6IRVREG* (*tmp$100)( struct $8REGCLASS*, int64, struct $6IRVREG** );
typedef void (*tmp$101)( struct $8REGCLASS* );
struct $7REG_REG;
struct $7REG_REG {
	int64 NUM;
	struct $7REG_REG* PREV;
};
__FB_STATIC_ASSERT( sizeof( struct $7REG_REG ) == 16 );
typedef int64 $12REG_SIZEMASK;
struct $10REG_REGCTX {
	struct $7REG_REG* FREETAIL;
	struct $7REG_REG* USEDTAIL;
	int64 FREETB;
	struct $7REG_REG REGTB[8];
	$12REG_SIZEMASK SIZETB[8];
	uint64 NEXTTB[8];
};
__FB_STATIC_ASSERT( sizeof( struct $10REG_REGCTX ) == 280 );
struct $10REG_STKCTX {
	int64 REGTB[8];
	int64 FREGS;
};
__FB_STATIC_ASSERT( sizeof( struct $10REG_STKCTX ) == 72 );
struct $8REGCLASS {
	tmp$94 ENSURE;
	tmp$94 _ALLOCATE;
	tmp$95 ALLOCATEREG;
	tmp$96 FREE;
	tmp$97 ISFREE;
	tmp$98 SETOWNER;
	tmp$99 GETMAXREGS;
	tmp$99 GETFIRST;
	tmp$97 GETNEXT;
	tmp$100 GETVREG;
	tmp$97 GETREALREG;
	tmp$101 CLEAR;
	tmp$101 DUMP;
	int64 CLASS;
	int64 ISSTACK;
	int64 REGS;
	struct $6IRVREG* VREGTB[8];
	struct $6IRVREG* VAUXPARENT[8];
	struct $10REG_REGCTX REGCTX;
	struct $10REG_STKCTX STKCTX;
};
__FB_STATIC_ASSERT( sizeof( struct $8REGCLASS ) == 608 );
struct $10TFLISTITEM;
struct $10TFLISTITEM {
	struct $10TFLISTITEM* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $10TFLISTITEM ) == 8 );
struct $6TFLIST {
	int64 TOTITEMS;
	int64 ITEMS;
	struct $10TFLISTITEM* ITEMTB;
	int64 INDEX;
	struct $10TFLISTITEM* LASTITEM;
	struct $5TLIST LIST;
	struct $7TLISTTB* LISTTB;
};
__FB_STATIC_ASSERT( sizeof( struct $6TFLIST ) == 112 );
typedef int64 $19EMIT_NODECLASS_ENUM;
typedef int64 $10IR_EMITOPT;
struct $12EMIT_BOPNODE {
	int64 OP;
	struct $6IRVREG* DVREG;
	struct $6IRVREG* SVREG;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_BOPNODE ) == 24 );
struct $12EMIT_UOPNODE {
	int64 OP;
	struct $6IRVREG* DVREG;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_UOPNODE ) == 16 );
struct $12EMIT_RELNODE {
	int64 OP;
	struct $6IRVREG* RVREG;
	struct $8FBSYMBOL* LABEL;
	struct $6IRVREG* DVREG;
	struct $6IRVREG* SVREG;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_RELNODE ) == 40 );
struct $12EMIT_STKNODE {
	int64 OP;
	struct $6IRVREG* VREG;
	int64 EXTRA;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_STKNODE ) == 24 );
struct $12EMIT_BRCNODE {
	int64 OP;
	struct $6IRVREG* VREG;
	struct $8FBSYMBOL* SYM;
	int64 EXTRA;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_BRCNODE ) == 32 );
struct $12EMIT_SOPNODE {
	int64 OP;
	struct $8FBSYMBOL* SYM;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_SOPNODE ) == 16 );
struct $12EMIT_LITNODE {
	int64 ISASM;
	char* TEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_LITNODE ) == 16 );
struct $12EMIT_JTBNODE {
	struct $8FBSYMBOL* TBSYM;
	uint64* VALUES;
	struct $8FBSYMBOL** LABELS;
	int64 LABELCOUNT;
	struct $8FBSYMBOL* DEFLABEL;
	uint64 BIAS;
	uint64 SPAN;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_JTBNODE ) == 56 );
struct $12EMIT_MEMNODE {
	int64 OP;
	struct $6IRVREG* DVREG;
	struct $6IRVREG* SVREG;
	int64 BYTES;
	int64 EXTRA;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_MEMNODE ) == 40 );
struct $12EMIT_DBGNODE {
	int64 OP;
	struct $8FBSYMBOL* SYM;
	int64 LNUM;
	char* FILENAME;
	int64 POS;
};
__FB_STATIC_ASSERT( sizeof( struct $12EMIT_DBGNODE ) == 40 );
struct $9EMIT_NODE {
	$19EMIT_NODECLASS_ENUM CLASS;
	$10IR_EMITOPT OPTIONS;
	union {
		struct $12EMIT_BOPNODE BOP;
		struct $12EMIT_UOPNODE UOP;
		struct $12EMIT_RELNODE REL;
		struct $12EMIT_STKNODE STK;
		struct $12EMIT_BRCNODE BRC;
		struct $12EMIT_SOPNODE SOP;
		struct $12EMIT_LITNODE LIT;
		struct $12EMIT_JTBNODE JTB;
		struct $12EMIT_MEMNODE MEM;
		struct $12EMIT_DBGNODE DBG;
	};
	int64 REGFREETB[2];
};
__FB_STATIC_ASSERT( sizeof( struct $9EMIT_NODE ) == 88 );
typedef int64 (*tmp$40)( void );
typedef void (*tmp$39)( void );
typedef int64 $14IR_OPTIONVALUE;
typedef int64 (*tmp$41)( $14IR_OPTIONVALUE );
typedef int64 (*tmp$42)( int64, int64 );
typedef void (*tmp$109)( int64, int64, int64, int64* );
typedef void (*tmp$110)( int64, int64, int64*, int64* );
typedef char* (*tmp$45)( void );
typedef void (*tmp$43)( struct $8FBSYMBOL* );
typedef void (*tmp$44)( struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef void (*tmp$111)( struct $8FBSYMBOL*, int64, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef char* (*tmp$113)( int64 );
typedef char* (*tmp$114)( int64, int64 );
struct $9EMIT_VTBL {
	tmp$40 INIT;
	tmp$39 END;
	tmp$41 GETOPTIONVALUE;
	tmp$40 OPEN;
	tmp$39 CLOSE;
	tmp$42 ISREGPRESERVED;
	tmp$42 GETFREEPRESERVEDREG;
	tmp$109 GETARGREG;
	tmp$110 GETRESULTREG;
	tmp$45 PROCGETFRAMEREGNAME;
	tmp$43 PROCBEGIN;
	tmp$43 PROCEND;
	tmp$44 PROCHEADER;
	tmp$111 PROCFOOTER;
	tmp$44 PROCALLOCARG;
	tmp$44 PROCALLOCLOCAL;
	tmp$43 PROCALLOCSTATICVARS;
	tmp$43 SCOPEBEGIN;
	tmp$43 SCOPEEND;
	tmp$112 SETSECTION;
	tmp$113 GETTYPESTRING;
	tmp$114 GETSECTIONSTRING;
};
__FB_STATIC_ASSERT( sizeof( struct $9EMIT_VTBL ) == 176 );
struct $7EMITCTX {
	int64 INITED;
	int64 POS;
	struct $8REGCLASS* REGTB[2];
	struct $6TFLIST NODETB;
	struct $6TFLIST VREGTB;
	struct $9EMIT_NODE* CURNODE;
	int64 REGUSEDTB[2];
	int64 LASTSECTION;
	int64 LASTPRIORITY;
	struct $9EMIT_VTBL VTBL;
	void** OPFNTB;
};
__FB_STATIC_ASSERT( sizeof( struct $7EMITCTX ) == 480 );
extern struct $7EMITCTX EMIT$;
struct $8FBARRAY1IPvE {
	void** DATA;
	void** PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1IPvE ) == 72 );
static struct $8FBARRAY1IPvE tmp$115$;
struct $7EDBGCTX {
	uint64 TYPECNT;
	struct $8FBSYMBOL* LABEL;
	int64 LNUM;
	int64 POS;
	int64 ISNEWLINE;
	int64 FIRSTLINE;
	int64 LASTLINE;
	char FILENAME[261];
	char* INCFILE;
};
__FB_STATIC_ASSERT( sizeof( struct $7EDBGCTX ) == 328 );
static struct $7EDBGCTX CTX$;
static int64 REMAPTB$[26] = { 7ll, 16ll, 2ll, 3ll, 4ll, 5ll, 6ll, 6ll, 1ll, 8ll, 1ll, 1ll, 8ll, 9ll, 10ll, 11ll, 12ll, 13ll, 14ll, 17ll };
static char* STABSTB$[18] = { (char*)"integer:t1=-1", (char*)"void:t7=-11", (char*)"byte:t2=-6", (char*)"ubyte:t3=-5", (char*)"char:t4=-2", (char*)"short:t5=-3", (char*)"ushort:t6=-7", (char*)"uinteger:t8=-8", (char*)"longint:t9=-31", (char*)"ulongint:t10=-32", (char*)"single:t11=-12", (char*)"double:t12=-13", (char*)"string:t13=s12data:15,0,32;len:1,32,32;size:1,64,32;;", (char*)"fixstr:t14=-2", (char*)"pchar:t15=*4;", (char*)"boolean:t16=@s8;-16", (char*)"va_list:t17=-11", (char*)"wchar:t18=-30" };

void EDBGINIT( void )
{
	label$10:;
	{
		$13FB_COMPTARGET TMP$136$2;
		TMP$136$2 = *($13FB_COMPTARGET*)((uint8*)&ENV$ + 216ll);
		if( TMP$136$2 == 0ll) goto label$14;
		label$15:;
		if( TMP$136$2 == 1ll) goto label$14;
		label$16:;
		if( TMP$136$2 != 6ll) goto label$13;
		label$14:;
		{
			*(int64*)((int64)(int64*)REMAPTB$ + 56ll) = 18ll;
		}
		goto label$12;
		label$13:;
		{
			*(int64*)((int64)(int64*)REMAPTB$ + 56ll) = *(int64*)((int64)(int64*)REMAPTB$ + (*(int64*)((uint8*)&ENV$ + 576ll) << (3ll & 63ll)));
		}
		label$17:;
		label$12:;
	}
	label$11:;
}

void EDBGEMITHEADER( char* FILENAME$1 )
{
	FBSTRING TMP$150$1;
	FBSTRING TMP$151$1;
	FBSTRING TMP$152$1;
	label$28:;
	FBSTRING LNAME$1;
	__builtin_memset( &LNAME$1, 0, 24ll );
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$31;
	{
		fb_StrDelete( (FBSTRING*)&LNAME$1 );
		goto label$29;
	}
	label$31:;
	label$30:;
	*(uint64*)&CTX$ = 1ull;
	*(struct $8FBSYMBOL**)((uint8*)&CTX$ + 8ll) = (struct $8FBSYMBOL*)0ull;
	*(char**)((uint8*)&CTX$ + 320ll) = (char*)0ull;
	fb_StrAssign( (void*)((uint8*)&CTX$ + 56ll), 261ll, (void*)FILENAME$1, 0ll, 0 );
	char* vr$3 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$3, 0ll, 0 );
	__builtin_memset( &TMP$152$1, 0, 24ll );
	char* vr$6 = HESCAPE( (char*)FILENAME$1, 0ll );
	__builtin_memset( &TMP$150$1, 0, 24ll );
	FBSTRING* vr$9 = fb_StrConcat( &TMP$150$1, (void*)".file \x22", 8ll, (void*)vr$6, 0ll );
	__builtin_memset( &TMP$151$1, 0, 24ll );
	FBSTRING* vr$12 = fb_StrConcat( &TMP$151$1, (void*)vr$9, -1ll, (void*)"\x22", 2ll );
	fb_StrAssign( (void*)&TMP$152$1, -1ll, (void*)vr$12, -1ll, 0 );
	EMITWRITESTR( (char*)*(char**)&TMP$152$1, -1ll );
	fb_StrDelete( (FBSTRING*)&TMP$152$1 );
	int64 vr$15 = PATHISABSOLUTE( FILENAME$1 );
	if( vr$15 != 0ll) goto label$33;
	{
		FBSTRING TMP$153$2;
		FBSTRING TMP$154$2;
		__builtin_memset( &TMP$154$2, 0, 24ll );
		FBSTRING* vr$17 = HCURDIR(  );
		__builtin_memset( &TMP$153$2, 0, 24ll );
		FBSTRING* vr$20 = fb_StrConcat( &TMP$153$2, (void*)vr$17, -1ll, (void*)"/", 2ll );
		fb_StrAssign( (void*)&TMP$154$2, -1ll, (void*)vr$20, -1ll, 0 );
		HEMITSTABS( 100ll, (char*)*(char**)&TMP$154$2, 0ll, 0ll, (char*)*(char**)&LNAME$1 );
		fb_StrDelete( (FBSTRING*)&TMP$154$2 );
	}
	label$33:;
	label$32:;
	HEMITSTABS( 100ll, (char*)FILENAME$1, 0ll, 0ll, (char*)*(char**)&LNAME$1 );
	(*(tmp$112*)((uint8*)&EMIT$ + 448ll))( 3ll, 0ll );
	HSTABLABEL( (char*)*(char**)&LNAME$1 );
	{
		int64 I$2;
		I$2 = 0ll;
		label$37:;
		{
			HEMITSTABS( 128ll, *(char**)((int64)(char**)STABSTB$ + (I$2 << (3ll & 63ll))), 0ll, 0ll, (char*)"0" );
			*(uint64*)&CTX$ = *(uint64*)&CTX$ + 1ull;
		}
		label$35:;
		I$2 = I$2 + 1ll;
		label$34:;
		if( I$2 <= 17ll) goto label$37;
		label$36:;
	}
	EMITWRITESTR( (char*)"", 0ll );
	fb_StrDelete( (FBSTRING*)&LNAME$1 );
	label$29:;
}

void EDBGEMITFOOTER( void )
{
	label$38:;
	static FBSTRING LNAME$1;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$41;
	{
		goto label$39;
	}
	label$41:;
	label$40:;
	(*(tmp$112*)((uint8*)&EMIT$ + 448ll))( 3ll, 0ll );
	char* vr$0 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$0, 0ll, 0 );
	HEMITSTABS( 100ll, (char*)"", 0ll, 0ll, (char*)*(char**)&LNAME$1 );
	HSTABLABEL( (char*)*(char**)&LNAME$1 );
	label$39:;
}

void EDBGLINEBEGIN( struct $8FBSYMBOL* PROC$1, int64 LNUM$1, int64 POS_$1, char* FILENAME$1 )
{
	label$42:;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$45;
	{
		goto label$43;
	}
	label$45:;
	label$44:;
	if( *(int64*)((uint8*)&CTX$ + 16ll) <= 0ll) goto label$47;
	{
		*(int64*)((uint8*)&CTX$ + 24ll) = POS_$1 - *(int64*)((uint8*)&CTX$ + 24ll);
		if( *(int64*)((uint8*)&CTX$ + 24ll) <= 0ll) goto label$49;
		{
			EDBGEMITLINE( PROC$1, *(int64*)((uint8*)&CTX$ + 16ll), *(struct $8FBSYMBOL**)((uint8*)&CTX$ + 8ll) );
			*(int64*)((uint8*)&CTX$ + 32ll) = -1ll;
		}
		label$49:;
		label$48:;
	}
	label$47:;
	label$46:;
	EDBGINCLUDE( FILENAME$1 );
	*(int64*)((uint8*)&CTX$ + 24ll) = POS_$1;
	*(int64*)((uint8*)&CTX$ + 16ll) = LNUM$1;
	if( *(int64*)((uint8*)&CTX$ + 32ll) == 0ll) goto label$51;
	{
		struct $8FBSYMBOL* vr$1 = SYMBADDLABEL( (char*)0ull, 4ll );
		*(struct $8FBSYMBOL**)((uint8*)&CTX$ + 8ll) = vr$1;
		char* vr$2 = SYMBGETMANGLEDNAME( *(struct $8FBSYMBOL**)((uint8*)&CTX$ + 8ll) );
		HSTABLABEL( vr$2 );
		*(int64*)((uint8*)&CTX$ + 32ll) = 0ll;
	}
	label$51:;
	label$50:;
	label$43:;
}

void EDBGLINEEND( struct $8FBSYMBOL* PROC$1, int64 LNUM$1, int64 POS_$1 )
{
	label$52:;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$55;
	{
		goto label$53;
	}
	label$55:;
	label$54:;
	if( *(int64*)((uint8*)&CTX$ + 16ll) <= 0ll) goto label$57;
	{
		*(int64*)((uint8*)&CTX$ + 24ll) = POS_$1 - *(int64*)((uint8*)&CTX$ + 24ll);
		if( *(int64*)((uint8*)&CTX$ + 24ll) <= 0ll) goto label$59;
		{
			EDBGEMITLINE( PROC$1, *(int64*)((uint8*)&CTX$ + 16ll), *(struct $8FBSYMBOL**)((uint8*)&CTX$ + 8ll) );
			*(int64*)((uint8*)&CTX$ + 32ll) = -1ll;
		}
		label$59:;
		label$58:;
		*(int64*)((uint8*)&CTX$ + 16ll) = 0ll;
	}
	label$57:;
	label$56:;
	label$53:;
}

void EDBGEMITLINE( struct $8FBSYMBOL* PROC$1, int64 LNUM$1, struct $8FBSYMBOL* LABEL$1 )
{
	FBSTRING TMP$156$1;
	FBSTRING TMP$157$1;
	FBSTRING TMP$158$1;
	label$60:;
	static char* S$1;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$63;
	{
		goto label$61;
	}
	label$63:;
	label$62:;
	if( *(int64*)((uint8*)&CTX$ + 40ll) != -1ll) goto label$65;
	{
		*(int64*)((uint8*)&CTX$ + 40ll) = LNUM$1;
	}
	label$65:;
	label$64:;
	*(int64*)((uint8*)&CTX$ + 48ll) = LNUM$1;
	__builtin_memset( &TMP$158$1, 0, 24ll );
	char* vr$1 = SYMBGETMANGLEDNAME( PROC$1 );
	char* vr$2 = SYMBGETMANGLEDNAME( LABEL$1 );
	__builtin_memset( &TMP$156$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$156$1, (void*)vr$2, 0ll, (void*)"-", 2ll );
	__builtin_memset( &TMP$157$1, 0, 24ll );
	FBSTRING* vr$8 = fb_StrConcat( &TMP$157$1, (void*)vr$5, -1ll, (void*)vr$1, 0ll );
	fb_StrAssign( (void*)&TMP$158$1, -1ll, (void*)vr$8, -1ll, 0 );
	char* vr$10 = HMAKESTABN( 68ll, 0ll, LNUM$1, (char*)*(char**)&TMP$158$1 );
	S$1 = vr$10;
	fb_StrDelete( (FBSTRING*)&TMP$158$1 );
	EMITWRITESTR( (char*)S$1, 0ll );
	label$61:;
}

void EDBGEMITLINEFLUSH( struct $8FBSYMBOL* PROC$1, int64 LNUM$1, struct $8FBSYMBOL* LABEL$1 )
{
	FBSTRING TMP$159$1;
	FBSTRING TMP$160$1;
	FBSTRING TMP$161$1;
	label$66:;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$69;
	{
		goto label$67;
	}
	label$69:;
	label$68:;
	__builtin_memset( &TMP$161$1, 0, 24ll );
	char* vr$1 = SYMBGETMANGLEDNAME( PROC$1 );
	char* vr$2 = SYMBGETMANGLEDNAME( LABEL$1 );
	__builtin_memset( &TMP$159$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$159$1, (void*)vr$2, 0ll, (void*)"-", 2ll );
	__builtin_memset( &TMP$160$1, 0, 24ll );
	FBSTRING* vr$8 = fb_StrConcat( &TMP$160$1, (void*)vr$5, -1ll, (void*)vr$1, 0ll );
	fb_StrAssign( (void*)&TMP$161$1, -1ll, (void*)vr$8, -1ll, 0 );
	HEMITSTABN( 68ll, 0ll, LNUM$1, (char*)*(char**)&TMP$161$1 );
	fb_StrDelete( (FBSTRING*)&TMP$161$1 );
	label$67:;
}

void EDBGSCOPEBEGIN( struct $8FBSYMBOL* S$1 )
{
	label$70:;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$73;
	{
		goto label$71;
	}
	label$73:;
	label$72:;
	*(int64*)((uint8*)S$1 + 128ll) = *(int64*)((uint8*)*(struct $9LEX_TKCTX**)((uint8*)&LEX$ + 844696ll) + 16752ll);
	struct $8FBSYMBOL* vr$2 = SYMBADDLABEL( (char*)0ull, 4ll );
	*(struct $8FBSYMBOL**)((uint8*)S$1 + 144ll) = vr$2;
	label$71:;
}

void EDBGSCOPEEND( struct $8FBSYMBOL* S$1 )
{
	label$74:;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$77;
	{
		goto label$75;
	}
	label$77:;
	label$76:;
	*(int64*)((uint8*)S$1 + 136ll) = *(int64*)((uint8*)*(struct $9LEX_TKCTX**)((uint8*)&LEX$ + 844696ll) + 16752ll);
	struct $8FBSYMBOL* vr$2 = SYMBADDLABEL( (char*)0ull, 4ll );
	*(struct $8FBSYMBOL**)((uint8*)S$1 + 152ll) = vr$2;
	label$75:;
}

void EDBGEMITSCOPEINI( struct $8FBSYMBOL* S$1 )
{
	label$78:;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$81;
	{
		goto label$79;
	}
	label$81:;
	label$80:;
	char* vr$1 = SYMBGETMANGLEDNAME( *(struct $8FBSYMBOL**)((uint8*)S$1 + 144ll) );
	HSTABLABEL( vr$1 );
	label$79:;
}

void EDBGEMITSCOPEEND( struct $8FBSYMBOL* S$1 )
{
	label$82:;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$85;
	{
		goto label$83;
	}
	label$85:;
	label$84:;
	char* vr$1 = SYMBGETMANGLEDNAME( *(struct $8FBSYMBOL**)((uint8*)S$1 + 152ll) );
	HSTABLABEL( vr$1 );
	label$83:;
}

void EDBGPROCBEGIN( struct $8FBSYMBOL* PROC$1 )
{
	label$86:;
	*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 32ll) = *(int64*)((uint8*)*(struct $9LEX_TKCTX**)((uint8*)&LEX$ + 844696ll) + 16752ll);
	label$87:;
}

void EDBGPROCEND( struct $8FBSYMBOL* PROC$1 )
{
	label$88:;
	*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 40ll) = *(int64*)((uint8*)*(struct $9LEX_TKCTX**)((uint8*)&LEX$ + 844696ll) + 16752ll);
	label$89:;
}

void EDBGPROCEMITBEGIN( struct $8FBSYMBOL* PROC$1 )
{
	label$90:;
	*(int64*)((uint8*)&CTX$ + 40ll) = -1ll;
	*(int64*)((uint8*)&CTX$ + 48ll) = -1ll;
	label$91:;
}

void EDBGEMITPROCHEADER( struct $8FBSYMBOL* PROC$1 )
{
	label$100:;
	static FBSTRING DESC$1;
	static FBSTRING PROCNAME$1;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$103;
	{
		goto label$101;
	}
	label$103:;
	label$102:;
	EDBGINCLUDE( *(char**)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 48ll) );
	if( (*(int64*)((uint8*)PROC$1 + 24ll) & 4096ll) == 0ll) goto label$105;
	{
		FBSTRING TMP$162$2;
		char* vr$4 = SYMBGETMANGLEDNAME( PROC$1 );
		__builtin_memset( &TMP$162$2, 0, 24ll );
		FBSTRING* vr$6 = FBGETENTRYPOINT(  );
		fb_StrAssign( (void*)&TMP$162$2, -1ll, (void*)vr$6, -1ll, 0 );
		HEMITSTABS( 42ll, (char*)*(char**)&TMP$162$2, 0ll, 1ll, (char*)vr$4 );
		fb_StrDelete( (FBSTRING*)&TMP$162$2 );
		HEMITSTABD( 68ll, 0ll, 1ll );
		*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 32ll) = 1ll;
		*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 40ll) = *(int64*)((uint8*)*(struct $9LEX_TKCTX**)((uint8*)&LEX$ + 844696ll) + 16752ll);
		FBSTRING* vr$14 = FBGETENTRYPOINT(  );
		fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$14, -1ll, 0 );
	}
	goto label$104;
	label$105:;
	{
		char* vr$15 = SYMBGETDBGNAME( PROC$1 );
		fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$15, 0ll, 0 );
	}
	label$104:;
	char* vr$16 = SYMBGETMANGLEDNAME( PROC$1 );
	fb_StrAssign( (void*)&PROCNAME$1, -1ll, (void*)vr$16, 0ll, 0 );
	if( (*(int64*)((uint8*)PROC$1 + 8ll) & 32ll) == 0ll) goto label$107;
	{
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)":F", 3ll, 0 );
	}
	goto label$106;
	label$107:;
	{
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)":f", 3ll, 0 );
	}
	label$106:;
	FBSTRING* vr$19 = HGETDATATYPE( PROC$1, 0ll );
	fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$19, -1ll, 0 );
	HEMITSTABS( 36ll, (char*)*(char**)&DESC$1, 0ll, *(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 32ll), (char*)*(char**)&PROCNAME$1 );
	HDECLARGS( PROC$1 );
	*(int64*)((uint8*)&CTX$ + 32ll) = -1ll;
	*(int64*)((uint8*)&CTX$ + 16ll) = 0ll;
	*(int64*)((uint8*)&CTX$ + 24ll) = 0ll;
	*(struct $8FBSYMBOL**)((uint8*)&CTX$ + 8ll) = (struct $8FBSYMBOL*)0ull;
	label$101:;
}

void EDBGEMITPROCFOOTER( struct $8FBSYMBOL* PROC$1, struct $8FBSYMBOL* INITLABEL$1, struct $8FBSYMBOL* EXITLABEL$1 )
{
	FBSTRING TMP$172$1;
	FBSTRING TMP$173$1;
	FBSTRING TMP$174$1;
	label$129:;
	static FBSTRING PROCNAME$1;
	static FBSTRING LNAME$1;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$132;
	{
		goto label$130;
	}
	label$132:;
	label$131:;
	char* vr$0 = SYMBGETMANGLEDNAME( PROC$1 );
	fb_StrAssign( (void*)&PROCNAME$1, -1ll, (void*)vr$0, 0ll, 0 );
	HDECLLOCALVARS( PROC$1, PROC$1, INITLABEL$1, EXITLABEL$1 );
	char* vr$1 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$1, 0ll, 0 );
	HSTABLABEL( (char*)*(char**)&LNAME$1 );
	__builtin_memset( &TMP$174$1, 0, 24ll );
	__builtin_memset( &TMP$172$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$172$1, (void*)&LNAME$1, -1ll, (void*)"-", 2ll );
	__builtin_memset( &TMP$173$1, 0, 24ll );
	FBSTRING* vr$8 = fb_StrConcat( &TMP$173$1, (void*)vr$5, -1ll, (void*)&PROCNAME$1, -1ll );
	fb_StrAssign( (void*)&TMP$174$1, -1ll, (void*)vr$8, -1ll, 0 );
	HEMITSTABS( 36ll, (char*)"", 0ll, 0ll, (char*)*(char**)&TMP$174$1 );
	fb_StrDelete( (FBSTRING*)&TMP$174$1 );
	*(int64*)((uint8*)&CTX$ + 32ll) = -1ll;
	*(int64*)((uint8*)&CTX$ + 16ll) = 0ll;
	*(int64*)((uint8*)&CTX$ + 24ll) = 0ll;
	*(struct $8FBSYMBOL**)((uint8*)&CTX$ + 8ll) = (struct $8FBSYMBOL*)0ull;
	label$130:;
}

void EDBGEMITGLOBALVAR( struct $8FBSYMBOL* SYM$1, int64 SECTION$1 )
{
	label$189:;
	int64 T$1;
	int64 ATTRIB$1;
	FBSTRING DESC$1;
	__builtin_memset( &DESC$1, 0, 24ll );
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$192;
	{
		fb_StrDelete( (FBSTRING*)&DESC$1 );
		goto label$190;
	}
	label$192:;
	label$191:;
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 128ll) == 0ll) goto label$194;
	{
		fb_StrDelete( (FBSTRING*)&DESC$1 );
		goto label$190;
	}
	label$194:;
	label$193:;
	{
		if( SECTION$1 != 0ll) goto label$196;
		label$197:;
		{
			T$1 = 36ll;
		}
		goto label$195;
		label$196:;
		if( SECTION$1 != 2ll) goto label$198;
		label$199:;
		{
			T$1 = 40ll;
		}
		goto label$195;
		label$198:;
		{
			T$1 = 38ll;
		}
		label$200:;
		label$195:;
	}
	char* vr$5 = SYMBGETDBGNAME( SYM$1 );
	fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$5, 0ll, 0 );
	if( ((int64)-((*(int64*)((uint8*)SYM$1 + 8ll) & 32ll) != 0ll) | (int64)-((*(int64*)((uint8*)SYM$1 + 8ll) & 8ll) != 0ll)) == 0ll) goto label$202;
	{
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)":G", 3ll, 0 );
	}
	goto label$201;
	label$202:;
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 2ll) == 0ll) goto label$203;
	{
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)":S", 3ll, 0 );
	}
	goto label$201;
	label$203:;
	{
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)":", 2ll, 0 );
	}
	label$201:;
	FBSTRING* vr$19 = HGETDATATYPE( SYM$1, 0ll );
	fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$19, -1ll, 0 );
	char* vr$21 = SYMBGETMANGLEDNAME( SYM$1 );
	HEMITSTABS( T$1, (char*)*(char**)&DESC$1, 0ll, 0ll, (char*)vr$21 );
	fb_StrDelete( (FBSTRING*)&DESC$1 );
	label$190:;
}

void EDBGEMITLOCALVAR( struct $8FBSYMBOL* SYM$1, int64 ISSTATIC$1 )
{
	label$204:;
	int64 T$1;
	FBSTRING DESC$1;
	__builtin_memset( &DESC$1, 0, 24ll );
	FBSTRING VALUE$1;
	__builtin_memset( &VALUE$1, 0, 24ll );
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$207;
	{
		fb_StrDelete( (FBSTRING*)&VALUE$1 );
		fb_StrDelete( (FBSTRING*)&DESC$1 );
		goto label$205;
	}
	label$207:;
	label$206:;
	fb_StrAssign( (void*)&DESC$1, -1ll, *(void**)((uint8*)SYM$1 + 32ll), 0ll, 0 );
	if( ISSTATIC$1 == 0ll) goto label$209;
	{
		if( (*(int64*)((uint8*)SYM$1 + 24ll) & 2ll) != 0ll) goto label$211;
		{
			fb_StrDelete( (FBSTRING*)&VALUE$1 );
			fb_StrDelete( (FBSTRING*)&DESC$1 );
			goto label$205;
		}
		label$211:;
		label$210:;
		if( *(struct $7ASTNODE**)((uint8*)SYM$1 + 96ll) == (struct $7ASTNODE*)0ull) goto label$213;
		{
			T$1 = 38ll;
		}
		goto label$212;
		label$213:;
		{
			T$1 = 40ll;
		}
		label$212:;
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)":V", 3ll, 0 );
		char* vr$12 = SYMBGETMANGLEDNAME( SYM$1 );
		fb_StrAssign( (void*)&VALUE$1, -1ll, (void*)vr$12, 0ll, 0 );
	}
	goto label$208;
	label$209:;
	{
		T$1 = 128ll;
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)":", 2ll, 0 );
		FBSTRING* vr$16 = fb_LongintToStr( *(int64*)((uint8*)SYM$1 + 88ll) );
		fb_StrAssign( (void*)&VALUE$1, -1ll, (void*)vr$16, -1ll, 0 );
	}
	label$208:;
	FBSTRING* vr$18 = HGETDATATYPE( SYM$1, 0ll );
	fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$18, -1ll, 0 );
	HEMITSTABS( T$1, (char*)*(char**)&DESC$1, 0ll, 0ll, (char*)*(char**)&VALUE$1 );
	fb_StrDelete( (FBSTRING*)&VALUE$1 );
	fb_StrDelete( (FBSTRING*)&DESC$1 );
	label$205:;
}

void EDBGEMITPROCARG( struct $8FBSYMBOL* SYM$1 )
{
	FBSTRING TMP$220$1;
	FBSTRING TMP$223$1;
	label$214:;
	FBSTRING DESC$1;
	__builtin_memset( &DESC$1, 0, 24ll );
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$217;
	{
		fb_StrDelete( (FBSTRING*)&DESC$1 );
		goto label$215;
	}
	label$217:;
	label$216:;
	__builtin_memset( &TMP$220$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$220$1, *(void**)((uint8*)SYM$1 + 32ll), 0ll, (void*)":", 2ll );
	fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$5, -1ll, 0 );
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 32768ll) == 0ll) goto label$219;
	{
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"p", 2ll, 0 );
	}
	goto label$218;
	label$219:;
	{
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"v", 2ll, 0 );
	}
	label$218:;
	FBSTRING* vr$11 = HGETDATATYPE( SYM$1, 0ll );
	fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$11, -1ll, 0 );
	__builtin_memset( &TMP$223$1, 0, 24ll );
	FBSTRING* vr$15 = fb_LongintToStr( *(int64*)((uint8*)SYM$1 + 88ll) );
	fb_StrAssign( (void*)&TMP$223$1, -1ll, (void*)vr$15, -1ll, 0 );
	HEMITSTABS( 160ll, (char*)*(char**)&DESC$1, 0ll, 0ll, (char*)*(char**)&TMP$223$1 );
	fb_StrDelete( (FBSTRING*)&TMP$223$1 );
	fb_StrDelete( (FBSTRING*)&DESC$1 );
	label$215:;
}

void EDBGINCLUDE( char* INCFILE$1 )
{
	label$220:;
	FBSTRING LNAME$1;
	__builtin_memset( &LNAME$1, 0, 24ll );
	if( INCFILE$1 != (char*)0ull) goto label$223;
	{
		fb_StrDelete( (FBSTRING*)&LNAME$1 );
		goto label$221;
	}
	label$223:;
	label$222:;
	if( INCFILE$1 != *(char**)((uint8*)&CTX$ + 320ll)) goto label$225;
	{
		fb_StrDelete( (FBSTRING*)&LNAME$1 );
		goto label$221;
	}
	label$225:;
	label$224:;
	(*(tmp$112*)((uint8*)&EMIT$ + 448ll))( 3ll, 0ll );
	char* vr$3 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$3, 0ll, 0 );
	HEMITSTABS( 132ll, (char*)INCFILE$1, 0ll, 0ll, (char*)*(char**)&LNAME$1 );
	HSTABLABEL( (char*)*(char**)&LNAME$1 );
	*(char**)((uint8*)&CTX$ + 320ll) = INCFILE$1;
	fb_StrDelete( (FBSTRING*)&LNAME$1 );
	label$221:;
}

__attribute__(( constructor )) static void fb_ctor__edbg_stab( void )
{
	label$0:;
	label$1:;
}

static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM* THIS$1, struct $11TSTRSETITEM* __FB_RHS__$1 )
{
	label$4:;
	fb_StrAssign( (void*)THIS$1, -1ll, (void*)__FB_RHS__$1, -1ll, 0 );
	*(int64*)((uint8*)THIS$1 + 24ll) = *(int64*)((uint8*)__FB_RHS__$1 + 24ll);
	*(struct $8HASHITEM**)((uint8*)THIS$1 + 32ll) = *(struct $8HASHITEM**)((uint8*)__FB_RHS__$1 + 32ll);
	label$5:;
}

static void HEMITSTABS( int64 _TYPE$1, char* _STRING$1, int64 _OTHER$1, int64 _DESC$1, char* _VALUE$1 )
{
	FBSTRING TMP$140$1;
	FBSTRING TMP$143$1;
	label$18:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)".stabs \x22", 9ll, 0 );
	char* vr$0 = HESCAPE( _STRING$1, 0ll );
	__builtin_memset( &TMP$140$1, 0, 24ll );
	FBSTRING* vr$3 = fb_StrConcat( &TMP$140$1, (void*)&OSTR$1, -1ll, (void*)vr$0, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$3, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x22,", 3ll, 0 );
	FBSTRING* vr$4 = fb_LongintToStr( _TYPE$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$4, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	FBSTRING* vr$5 = fb_LongintToStr( _OTHER$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$5, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	FBSTRING* vr$6 = fb_LongintToStr( _DESC$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$6, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	__builtin_memset( &TMP$143$1, 0, 24ll );
	FBSTRING* vr$9 = fb_StrConcat( &TMP$143$1, (void*)&OSTR$1, -1ll, (void*)_VALUE$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$9, -1ll, 0 );
	EMITWRITESTR( (char*)*(char**)&OSTR$1, -1ll );
	label$19:;
}

static char* HMAKESTABN( int64 _TYPE$1, int64 _OTHER$1, int64 _DESC$1, char* _VALUE$1 )
{
	FBSTRING TMP$145$1;
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$20:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)".stabn ", 8ll, 0 );
	FBSTRING* vr$1 = fb_LongintToStr( _TYPE$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	FBSTRING* vr$2 = fb_LongintToStr( _OTHER$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	FBSTRING* vr$3 = fb_LongintToStr( _DESC$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$3, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	__builtin_memset( &TMP$145$1, 0, 24ll );
	FBSTRING* vr$6 = fb_StrConcat( &TMP$145$1, (void*)&OSTR$1, -1ll, (void*)_VALUE$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$6, -1ll, 0 );
	fb$result$1 = (char*)*(char**)&OSTR$1;
	label$21:;
	return fb$result$1;
}

static void HEMITSTABN( int64 _TYPE$1, int64 _OTHER$1, int64 _DESC$1, char* _VALUE$1 )
{
	label$22:;
	char* vr$0 = HMAKESTABN( _TYPE$1, _OTHER$1, _DESC$1, _VALUE$1 );
	EMITWRITESTR( (char*)vr$0, -1ll );
	label$23:;
}

static void HEMITSTABD( int64 _TYPE$1, int64 _OTHER$1, int64 _DESC$1 )
{
	label$24:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)".stabd ", 8ll, 0 );
	FBSTRING* vr$0 = fb_LongintToStr( _TYPE$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$0, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	FBSTRING* vr$1 = fb_LongintToStr( _OTHER$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	FBSTRING* vr$2 = fb_LongintToStr( _DESC$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	EMITWRITESTR( (char*)*(char**)&OSTR$1, -1ll );
	label$25:;
}

static void HSTABLABEL( char* LABEL$1 )
{
	label$26:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)LABEL$1, 0ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)":", 2ll, 0 );
	EMITWRITESTR( (char*)*(char**)&OSTR$1, 0ll );
	label$27:;
}

static void HDECLARGS( struct $8FBSYMBOL* PROC$1 )
{
	label$92:;
	struct $8FBSYMBOL* S$1;
	S$1 = *(struct $8FBSYMBOL**)((uint8*)PROC$1 + 104ll);
	label$94:;
	if( S$1 == (struct $8FBSYMBOL*)0ull) goto label$95;
	{
		if( *(int64*)S$1 != 1ll) goto label$97;
		{
			if( (*(int64*)((uint8*)S$1 + 8ll) & 114688ll) == 0ll) goto label$99;
			{
				EDBGEMITPROCARG( S$1 );
			}
			label$99:;
			label$98:;
		}
		label$97:;
		label$96:;
		S$1 = *(struct $8FBSYMBOL**)((uint8*)S$1 + 312ll);
	}
	goto label$94;
	label$95:;
	label$93:;
}

static void HDECLLOCALVARS( struct $8FBSYMBOL* PROC$1, struct $8FBSYMBOL* BLK$1, struct $8FBSYMBOL* INILABEL$1, struct $8FBSYMBOL* ENDLABEL$1 )
{
	FBSTRING TMP$166$1;
	FBSTRING TMP$167$1;
	FBSTRING TMP$168$1;
	FBSTRING TMP$169$1;
	FBSTRING TMP$170$1;
	FBSTRING TMP$171$1;
	label$108:;
	struct $8FBSYMBOL* SHEAD$1;
	__builtin_memset( &SHEAD$1, 0, 8ll );
	struct $8FBSYMBOL* S$1;
	__builtin_memset( &S$1, 0, 8ll );
	static int64 SCOPECNT$1;
	if( *(int64*)BLK$1 != 3ll) goto label$111;
	{
		SHEAD$1 = *(struct $8FBSYMBOL**)((uint8*)BLK$1 + 104ll);
	}
	goto label$110;
	label$111:;
	{
		SHEAD$1 = *(struct $8FBSYMBOL**)((uint8*)BLK$1 + 112ll);
	}
	label$110:;
	SCOPECNT$1 = 0ll;
	S$1 = SHEAD$1;
	label$112:;
	if( S$1 == (struct $8FBSYMBOL*)0ull) goto label$113;
	{
		{
			$12FB_SYMBCLASS TMP$165$3;
			TMP$165$3 = *($12FB_SYMBCLASS*)S$1;
			if( TMP$165$3 != 1ll) goto label$115;
			label$116:;
			{
				if( (*(int64*)((uint8*)S$1 + 8ll) & 249860ll) != 0ll) goto label$118;
				{
					if( (*(int64*)((uint8*)S$1 + 24ll) & 16ll) != 0ll) goto label$120;
					{
						EDBGEMITLOCALVAR( S$1, (int64)-((*(int64*)((uint8*)S$1 + 8ll) & 2ll) != 0ll) );
					}
					label$120:;
					label$119:;
				}
				label$118:;
				label$117:;
			}
			goto label$114;
			label$115:;
			if( TMP$165$3 != 15ll) goto label$121;
			label$122:;
			{
				SCOPECNT$1 = SCOPECNT$1 + 1ll;
			}
			label$121:;
			label$114:;
		}
		S$1 = *(struct $8FBSYMBOL**)((uint8*)S$1 + 312ll);
	}
	goto label$112;
	label$113:;
	__builtin_memset( &TMP$168$1, 0, 24ll );
	char* vr$16 = SYMBGETMANGLEDNAME( PROC$1 );
	char* vr$17 = SYMBGETMANGLEDNAME( INILABEL$1 );
	__builtin_memset( &TMP$166$1, 0, 24ll );
	FBSTRING* vr$20 = fb_StrConcat( &TMP$166$1, (void*)vr$17, 0ll, (void*)"-", 2ll );
	__builtin_memset( &TMP$167$1, 0, 24ll );
	FBSTRING* vr$23 = fb_StrConcat( &TMP$167$1, (void*)vr$20, -1ll, (void*)vr$16, 0ll );
	fb_StrAssign( (void*)&TMP$168$1, -1ll, (void*)vr$23, -1ll, 0 );
	HEMITSTABN( 192ll, 0ll, 0ll, (char*)*(char**)&TMP$168$1 );
	fb_StrDelete( (FBSTRING*)&TMP$168$1 );
	if( SCOPECNT$1 <= 0ll) goto label$124;
	{
		S$1 = SHEAD$1;
		label$125:;
		if( S$1 == (struct $8FBSYMBOL*)0ull) goto label$126;
		{
			if( *(int64*)S$1 != 15ll) goto label$128;
			{
				HDECLLOCALVARS( PROC$1, S$1, *(struct $8FBSYMBOL**)((uint8*)S$1 + 144ll), *(struct $8FBSYMBOL**)((uint8*)S$1 + 152ll) );
			}
			label$128:;
			label$127:;
			S$1 = *(struct $8FBSYMBOL**)((uint8*)S$1 + 312ll);
		}
		goto label$125;
		label$126:;
	}
	label$124:;
	label$123:;
	__builtin_memset( &TMP$171$1, 0, 24ll );
	char* vr$31 = SYMBGETMANGLEDNAME( PROC$1 );
	char* vr$32 = SYMBGETMANGLEDNAME( ENDLABEL$1 );
	__builtin_memset( &TMP$169$1, 0, 24ll );
	FBSTRING* vr$35 = fb_StrConcat( &TMP$169$1, (void*)vr$32, 0ll, (void*)"-", 2ll );
	__builtin_memset( &TMP$170$1, 0, 24ll );
	FBSTRING* vr$38 = fb_StrConcat( &TMP$170$1, (void*)vr$35, -1ll, (void*)vr$31, 0ll );
	fb_StrAssign( (void*)&TMP$171$1, -1ll, (void*)vr$38, -1ll, 0 );
	HEMITSTABN( 224ll, 0ll, 0ll, (char*)*(char**)&TMP$171$1 );
	fb_StrDelete( (FBSTRING*)&TMP$171$1 );
	label$109:;
}

static FBSTRING* HDECLPOINTER( int64* DTYPE$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$133:;
	static FBSTRING DESC$1;
	fb_StrAssign( (void*)&DESC$1, -1ll, (void*)"", 1ll, 0 );
	label$135:;
	if( (*DTYPE$1 & 480ll) == 0ll) goto label$136;
	{
		*DTYPE$1 = (((*DTYPE$1 & 31ll) | ((*DTYPE$1 & 480ll) + -32ll)) | (((*DTYPE$1 & 261632ll) >> (1ll & 63ll)) & 261632ll)) | (*DTYPE$1 & 32505856ll);
		FBSTRING* vr$18 = fb_ULongintToStr( *(uint64*)&CTX$ );
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$18, -1ll, 0 );
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"=*", 3ll, 0 );
		*(uint64*)&CTX$ = *(uint64*)&CTX$ + 1ull;
	}
	goto label$135;
	label$136:;
	fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)&DESC$1, -1ll, 0 );
	label$134:;
	FBSTRING* vr$22 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$22;
}

static FBSTRING* HGETDATATYPE( struct $8FBSYMBOL* SYM$1, int64 REQUESTEDDIMTBELEMENTS$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$137:;
	int64 DTYPE$1;
	int64 DIMTBELEMENTS$1;
	struct $8FBSYMBOL* SUBTYPE$1;
	FBSTRING DESC$1;
	__builtin_memset( &DESC$1, 0, 24ll );
	if( SYM$1 != (struct $8FBSYMBOL*)0ull) goto label$140;
	{
		FBSTRING* vr$2 = fb_LongintToStr( *(int64*)REMAPTB$ );
		fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$2, -1ll, 0 );
		fb_StrDelete( (FBSTRING*)&DESC$1 );
		goto label$138;
	}
	label$140:;
	label$139:;
	DIMTBELEMENTS$1 = 0ll;
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 16384ll) == 0ll) goto label$142;
	{
		DTYPE$1 = 20ll;
		SUBTYPE$1 = *(struct $8FBSYMBOL**)((uint8*)SYM$1 + 144ll);
		DIMTBELEMENTS$1 = *(int64*)((uint8*)SYM$1 + 104ll);
	}
	goto label$141;
	label$142:;
	{
		DTYPE$1 = *(int64*)((uint8*)SYM$1 + 56ll) & 511ll;
		SUBTYPE$1 = *(struct $8FBSYMBOL**)((uint8*)SYM$1 + 64ll);
		if( ((int64)-(*(int64*)SYM$1 == 1ll) | (int64)-(*(int64*)SYM$1 == 12ll)) == 0ll) goto label$144;
		{
			if( (*(int64*)((uint8*)SYM$1 + 8ll) & 262144ll) == 0ll) goto label$146;
			{
				DTYPE$1 = (((DTYPE$1 & 31ll) | ((DTYPE$1 & 480ll) + 32ll)) | ((DTYPE$1 & 261632ll) << (1ll & 63ll))) | (DTYPE$1 & 32505856ll);
			}
			label$146:;
			label$145:;
			if( *(int64*)((uint8*)SYM$1 + 104ll) <= 0ll) goto label$148;
			{
				FBSTRING* vr$29 = fb_ULongintToStr( *(uint64*)&CTX$ );
				fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$29, -1ll, 0 );
				fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"=", 2ll, 0 );
				*(uint64*)&CTX$ = *(uint64*)&CTX$ + 1ull;
				if( REQUESTEDDIMTBELEMENTS$1 <= 0ll) goto label$150;
				{
					fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"ar1;", 5ll, 0 );
					fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"0;", 3ll, 0 );
					FBSTRING* vr$36 = fb_LongintToStr( REQUESTEDDIMTBELEMENTS$1 + -1ll );
					fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$36, -1ll, 0 );
					fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)";", 2ll, 0 );
				}
				goto label$149;
				label$150:;
				{
					{
						int64 I$6;
						I$6 = 0ll;
						int64 TMP$180$6;
						TMP$180$6 = *(int64*)((uint8*)SYM$1 + 104ll) + -1ll;
						goto label$151;
						label$154:;
						{
							FBSTRING TMP$181$7;
							FBSTRING TMP$182$7;
							FBSTRING TMP$183$7;
							FBSTRING TMP$184$7;
							fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"ar1;", 5ll, 0 );
							FBSTRING* vr$45 = fb_LongintToStr( *(int64*)((uint8*)*(struct $10FBARRAYDIM**)((uint8*)SYM$1 + 112ll) + (I$6 << (4ll & 63ll))) );
							__builtin_memset( &TMP$181$7, 0, 24ll );
							FBSTRING* vr$48 = fb_StrConcat( &TMP$181$7, (void*)vr$45, -1ll, (void*)";", 2ll );
							__builtin_memset( &TMP$182$7, 0, 24ll );
							FBSTRING* vr$52 = fb_StrConcat( &TMP$182$7, (void*)&DESC$1, -1ll, (void*)vr$48, -1ll );
							fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$52, -1ll, 0 );
							FBSTRING* vr$57 = fb_LongintToStr( *(int64*)((uint8*)((uint8*)*(struct $10FBARRAYDIM**)((uint8*)SYM$1 + 112ll) + (I$6 << (4ll & 63ll))) + 8ll) );
							__builtin_memset( &TMP$183$7, 0, 24ll );
							FBSTRING* vr$60 = fb_StrConcat( &TMP$183$7, (void*)vr$57, -1ll, (void*)";", 2ll );
							__builtin_memset( &TMP$184$7, 0, 24ll );
							FBSTRING* vr$64 = fb_StrConcat( &TMP$184$7, (void*)&DESC$1, -1ll, (void*)vr$60, -1ll );
							fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$64, -1ll, 0 );
						}
						label$152:;
						I$6 = I$6 + 1ll;
						label$151:;
						if( I$6 <= TMP$180$6) goto label$154;
						label$153:;
					}
				}
				label$149:;
			}
			label$148:;
			label$147:;
			if( (*(int64*)((uint8*)SYM$1 + 8ll) & 8192ll) == 0ll) goto label$156;
			{
				DIMTBELEMENTS$1 = *(int64*)((uint8*)*(struct $8FBSYMBOL**)((uint8*)SYM$1 + 152ll) + 104ll);
			}
			label$156:;
			label$155:;
		}
		label$144:;
		label$143:;
	}
	label$141:;
	if( DIMTBELEMENTS$1 >= 0ll) goto label$158;
	{
		DIMTBELEMENTS$1 = 1ll;
	}
	label$158:;
	label$157:;
	if( (DTYPE$1 & 480ll) == 0ll) goto label$160;
	{
		FBSTRING* vr$73 = HDECLPOINTER( &DTYPE$1 );
		fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$73, -1ll, 0 );
	}
	label$160:;
	label$159:;
	DTYPE$1 = DTYPE$1 & -513ll;
	{
		uint64 TMP$185$2;
		TMP$185$2 = (uint64)DTYPE$1;
		goto label$162;
		label$163:;
		{
			FBSTRING TMP$186$3;
			if( *(int64*)((uint8*)SUBTYPE$1 + 232ll) != -1ll) goto label$165;
			{
				HDECLUDT( SUBTYPE$1, DIMTBELEMENTS$1 );
			}
			label$165:;
			label$164:;
			FBSTRING* vr$78 = fb_LongintToStr( *(int64*)((uint8*)SUBTYPE$1 + 232ll) );
			__builtin_memset( &TMP$186$3, 0, 24ll );
			FBSTRING* vr$82 = fb_StrConcat( &TMP$186$3, (void*)&DESC$1, -1ll, (void*)vr$78, -1ll );
			fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$82, -1ll, 0 );
		}
		goto label$161;
		label$166:;
		{
			FBSTRING TMP$187$3;
			if( *(int64*)((uint8*)SUBTYPE$1 + 184ll) != -1ll) goto label$168;
			{
				HDECLENUM( SUBTYPE$1 );
			}
			label$168:;
			label$167:;
			FBSTRING* vr$86 = fb_LongintToStr( *(int64*)((uint8*)SUBTYPE$1 + 184ll) );
			__builtin_memset( &TMP$187$3, 0, 24ll );
			FBSTRING* vr$90 = fb_StrConcat( &TMP$187$3, (void*)&DESC$1, -1ll, (void*)vr$86, -1ll );
			fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$90, -1ll, 0 );
		}
		goto label$161;
		label$169:;
		{
			FBSTRING* vr$92 = fb_ULongintToStr( *(uint64*)&CTX$ );
			fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$92, -1ll, 0 );
			fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"=f", 3ll, 0 );
			*(uint64*)&CTX$ = *(uint64*)&CTX$ + 1ull;
			FBSTRING* vr$96 = HGETDATATYPE( SUBTYPE$1, 0ll );
			fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$96, -1ll, 0 );
		}
		goto label$161;
		label$170:;
		{
			FBSTRING* vr$98 = fb_LongintToStr( *(int64*)REMAPTB$ );
			fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$98, -1ll, 0 );
		}
		goto label$161;
		label$171:;
		{
			int64 TMP$189$3;
			if( *(int64*)((uint8*)SYM$1 + 80ll) <= 0ll) goto label$172;
			TMP$189$3 = (int64)-(((*(int64*)((uint8*)SYM$1 + 56ll) & 511ll) & 480ll) == 0ll);
			goto label$226;
			label$172:;
			TMP$189$3 = 0ll;
			label$226:;
			if( TMP$189$3 == 0ll) goto label$174;
			{
				int64 TMP$190$4;
				FBSTRING TMP$191$4;
				FBSTRING TMP$192$4;
				FBSTRING* vr$105 = fb_ULongintToStr( *(uint64*)&CTX$ );
				fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$105, -1ll, 0 );
				fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"=", 2ll, 0 );
				*(uint64*)&CTX$ = *(uint64*)&CTX$ + 1ull;
				fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"ar1;", 5ll, 0 );
				fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)"0;", 3ll, 0 );
				if( (DTYPE$1 & 480ll) == 0ll) goto label$175;
				TMP$190$4 = 24ll;
				goto label$227;
				label$175:;
				TMP$190$4 = DTYPE$1 & 31ll;
				label$227:;
				FBSTRING* vr$117 = fb_LongintToStr( (*(int64*)((uint8*)SYM$1 + 80ll) / *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$190$4 * 56ll)) + 8ll)) + -1ll );
				__builtin_memset( &TMP$191$4, 0, 24ll );
				FBSTRING* vr$120 = fb_StrConcat( &TMP$191$4, (void*)vr$117, -1ll, (void*)";", 2ll );
				__builtin_memset( &TMP$192$4, 0, 24ll );
				FBSTRING* vr$124 = fb_StrConcat( &TMP$192$4, (void*)&DESC$1, -1ll, (void*)vr$120, -1ll );
				fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$124, -1ll, 0 );
				FBSTRING* vr$127 = fb_LongintToStr( *(int64*)((int64)(int64*)REMAPTB$ + (DTYPE$1 << (3ll & 63ll))) );
				fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$127, -1ll, 0 );
			}
			goto label$173;
			label$174:;
			{
				FBSTRING* vr$130 = fb_LongintToStr( *(int64*)((int64)(int64*)REMAPTB$ + (DTYPE$1 << (3ll & 63ll))) );
				fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$130, -1ll, 0 );
			}
			label$173:;
		}
		goto label$161;
		label$176:;
		{
			FBSTRING* vr$133 = fb_LongintToStr( *(int64*)((int64)(int64*)REMAPTB$ + (DTYPE$1 << (3ll & 63ll))) );
			fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)vr$133, -1ll, 0 );
		}
		goto label$161;
		label$162:;
		static const void* tmp$224[20ll] = {
			&&label$171,
			&&label$176,
			&&label$176,
			&&label$171,
			&&label$176,
			&&label$176,
			&&label$166,
			&&label$176,
			&&label$176,
			&&label$176,
			&&label$176,
			&&label$176,
			&&label$176,
			&&label$176,
			&&label$171,
			&&label$176,
			&&label$163,
			&&label$176,
			&&label$169,
			&&label$170,
		};
		if( (TMP$185$2 - 4ull) > 19ull ) goto label$176;
		goto *tmp$224[TMP$185$2 - 4ull];
		label$161:;
	}
	fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)&DESC$1, -1ll, 0 );
	fb_StrDelete( (FBSTRING*)&DESC$1 );
	label$138:;
	FBSTRING* vr$139 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$139;
}

static void HDECLUDT( struct $8FBSYMBOL* SYM$1, int64 DIMTBELEMENTS$1 )
{
	FBSTRING TMP$195$1;
	FBSTRING TMP$196$1;
	FBSTRING TMP$197$1;
	FBSTRING TMP$198$1;
	label$177:;
	struct $8FBSYMBOL* FLD$1;
	FBSTRING DESC$1;
	__builtin_memset( &DESC$1, 0, 24ll );
	*(int64*)((uint8*)SYM$1 + 232ll) = *(int64*)&CTX$;
	*(uint64*)&CTX$ = *(uint64*)&CTX$ + 1ull;
	char* vr$3 = SYMBGETDBGNAME( SYM$1 );
	fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$3, 0ll, 0 );
	FBSTRING* vr$6 = fb_LongintToStr( *(int64*)((uint8*)SYM$1 + 80ll) );
	FBSTRING* vr$8 = fb_LongintToStr( *(int64*)((uint8*)SYM$1 + 232ll) );
	__builtin_memset( &TMP$195$1, 0, 24ll );
	FBSTRING* vr$11 = fb_StrConcat( &TMP$195$1, (void*)":Tt", 4ll, (void*)vr$8, -1ll );
	__builtin_memset( &TMP$196$1, 0, 24ll );
	FBSTRING* vr$14 = fb_StrConcat( &TMP$196$1, (void*)vr$11, -1ll, (void*)"=s", 3ll );
	__builtin_memset( &TMP$197$1, 0, 24ll );
	FBSTRING* vr$17 = fb_StrConcat( &TMP$197$1, (void*)vr$14, -1ll, (void*)vr$6, -1ll );
	__builtin_memset( &TMP$198$1, 0, 24ll );
	FBSTRING* vr$21 = fb_StrConcat( &TMP$198$1, (void*)&DESC$1, -1ll, (void*)vr$17, -1ll );
	fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$21, -1ll, 0 );
	struct $8FBSYMBOL* vr$23 = SYMBUDTGETFIRSTFIELD( SYM$1 );
	FLD$1 = vr$23;
	label$179:;
	if( FLD$1 == (struct $8FBSYMBOL*)0ull) goto label$180;
	{
		if( (*(int64*)((uint8*)FLD$1 + 8ll) & 4ll) != 0ll) goto label$182;
		{
			FBSTRING TMP$199$3;
			FBSTRING TMP$200$3;
			FBSTRING TMP$201$3;
			int64 TMP$202$3;
			FBSTRING TMP$203$3;
			FBSTRING TMP$204$3;
			int64 TMP$205$3;
			FBSTRING TMP$206$3;
			FBSTRING TMP$207$3;
			FBSTRING* vr$26 = HGETDATATYPE( FLD$1, DIMTBELEMENTS$1 );
			__builtin_memset( &TMP$199$3, 0, 24ll );
			FBSTRING* vr$30 = fb_StrConcat( &TMP$199$3, *(void**)((uint8*)FLD$1 + 32ll), 0ll, (void*)":", 2ll );
			__builtin_memset( &TMP$200$3, 0, 24ll );
			FBSTRING* vr$33 = fb_StrConcat( &TMP$200$3, (void*)vr$30, -1ll, (void*)vr$26, -1ll );
			__builtin_memset( &TMP$201$3, 0, 24ll );
			FBSTRING* vr$37 = fb_StrConcat( &TMP$201$3, (void*)&DESC$1, -1ll, (void*)vr$33, -1ll );
			fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$37, -1ll, 0 );
			if( *(int64*)((uint8*)FLD$1 + 192ll) <= 0ll) goto label$183;
			TMP$202$3 = *(int64*)((uint8*)FLD$1 + 184ll);
			goto label$228;
			label$183:;
			TMP$202$3 = 0ll;
			label$228:;
			FBSTRING* vr$44 = fb_LongintToStr( (*(int64*)((uint8*)FLD$1 + 88ll) << (3ll & 63ll)) + TMP$202$3 );
			__builtin_memset( &TMP$203$3, 0, 24ll );
			FBSTRING* vr$47 = fb_StrConcat( &TMP$203$3, (void*)",", 2ll, (void*)vr$44, -1ll );
			__builtin_memset( &TMP$204$3, 0, 24ll );
			FBSTRING* vr$51 = fb_StrConcat( &TMP$204$3, (void*)&DESC$1, -1ll, (void*)vr$47, -1ll );
			fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$51, -1ll, 0 );
			if( *(int64*)((uint8*)FLD$1 + 192ll) <= 0ll) goto label$184;
			TMP$205$3 = *(int64*)((uint8*)FLD$1 + 192ll);
			goto label$229;
			label$184:;
			int64 vr$55 = SYMBGETREALSIZE( FLD$1 );
			TMP$205$3 = vr$55 << (3ll & 63ll);
			label$229:;
			FBSTRING* vr$57 = fb_LongintToStr( TMP$205$3 );
			__builtin_memset( &TMP$206$3, 0, 24ll );
			FBSTRING* vr$60 = fb_StrConcat( &TMP$206$3, (void*)",", 2ll, (void*)vr$57, -1ll );
			__builtin_memset( &TMP$207$3, 0, 24ll );
			FBSTRING* vr$64 = fb_StrConcat( &TMP$207$3, (void*)&DESC$1, -1ll, (void*)vr$60, -1ll );
			fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$64, -1ll, 0 );
			fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)";", 2ll, 0 );
		}
		label$182:;
		label$181:;
		struct $8FBSYMBOL* vr$67 = SYMBUDTGETNEXTFIELD( FLD$1 );
		FLD$1 = vr$67;
	}
	goto label$179;
	label$180:;
	fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)";", 2ll, 0 );
	HEMITSTABS( 128ll, (char*)*(char**)&DESC$1, 0ll, 0ll, (char*)"0" );
	fb_StrDelete( (FBSTRING*)&DESC$1 );
	label$178:;
}

static void HDECLENUM( struct $8FBSYMBOL* SYM$1 )
{
	FBSTRING TMP$210$1;
	FBSTRING TMP$211$1;
	FBSTRING TMP$212$1;
	label$185:;
	struct $8FBSYMBOL* E$1;
	__builtin_memset( &E$1, 0, 8ll );
	FBSTRING DESC$1;
	__builtin_memset( &DESC$1, 0, 24ll );
	*(int64*)((uint8*)SYM$1 + 184ll) = *(int64*)&CTX$;
	*(uint64*)&CTX$ = *(uint64*)&CTX$ + 1ull;
	char* vr$4 = SYMBGETDBGNAME( SYM$1 );
	fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$4, 0ll, 0 );
	FBSTRING* vr$7 = fb_LongintToStr( *(int64*)((uint8*)SYM$1 + 184ll) );
	__builtin_memset( &TMP$210$1, 0, 24ll );
	FBSTRING* vr$10 = fb_StrConcat( &TMP$210$1, (void*)":T", 3ll, (void*)vr$7, -1ll );
	__builtin_memset( &TMP$211$1, 0, 24ll );
	FBSTRING* vr$13 = fb_StrConcat( &TMP$211$1, (void*)vr$10, -1ll, (void*)"=e", 3ll );
	__builtin_memset( &TMP$212$1, 0, 24ll );
	FBSTRING* vr$17 = fb_StrConcat( &TMP$212$1, (void*)&DESC$1, -1ll, (void*)vr$13, -1ll );
	fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$17, -1ll, 0 );
	struct $8FBSYMBOL* vr$19 = SYMBGETENUMFIRSTELM( SYM$1 );
	E$1 = vr$19;
	label$187:;
	if( E$1 == (struct $8FBSYMBOL*)0ull) goto label$188;
	{
		FBSTRING TMP$213$2;
		FBSTRING TMP$214$2;
		FBSTRING TMP$215$2;
		FBSTRING TMP$216$2;
		FBSTRING* vr$21 = fb_LongintToStr( *(int64*)((uint8*)E$1 + 96ll) );
		__builtin_memset( &TMP$213$2, 0, 24ll );
		FBSTRING* vr$25 = fb_StrConcat( &TMP$213$2, *(void**)((uint8*)E$1 + 32ll), 0ll, (void*)":", 2ll );
		__builtin_memset( &TMP$214$2, 0, 24ll );
		FBSTRING* vr$28 = fb_StrConcat( &TMP$214$2, (void*)vr$25, -1ll, (void*)vr$21, -1ll );
		__builtin_memset( &TMP$215$2, 0, 24ll );
		FBSTRING* vr$31 = fb_StrConcat( &TMP$215$2, (void*)vr$28, -1ll, (void*)",", 2ll );
		__builtin_memset( &TMP$216$2, 0, 24ll );
		FBSTRING* vr$35 = fb_StrConcat( &TMP$216$2, (void*)&DESC$1, -1ll, (void*)vr$31, -1ll );
		fb_StrAssign( (void*)&DESC$1, -1ll, (void*)vr$35, -1ll, 0 );
		struct $8FBSYMBOL* vr$37 = SYMBGETENUMNEXTELM( E$1 );
		E$1 = vr$37;
	}
	goto label$187;
	label$188:;
	fb_StrConcatAssign( (void*)&DESC$1, -1ll, (void*)";", 2ll, 0 );
	HEMITSTABS( 128ll, (char*)*(char**)&DESC$1, 0ll, 0ll, (char*)"0" );
	fb_StrDelete( (FBSTRING*)&DESC$1 );
	label$186:;
}
