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
typedef int64 $13AST_NODECLASS;
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
struct $7ASTNODE;
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
union $7FBVALUE {
	struct $8FBSYMBOL* S;
	int64 I;
	double F;
};
__FB_STATIC_ASSERT( sizeof( union $7FBVALUE ) == 8 );
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
typedef int64 $9FB_ERRMSG;
struct $11TSTRSETITEM {
	FBSTRING S;
	int64 USERDATA;
	struct $8HASHITEM* HASHITEM;
};
__FB_STATIC_ASSERT( sizeof( struct $11TSTRSETITEM ) == 40 );
FBSTRING* fb_StrAssign( void*, int64, void*, int64, int32 );
void fb_StrDelete( FBSTRING* );
static void fb_ctor__astzhelper( void ) __attribute__(( constructor ));
static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM*, struct $11TSTRSETITEM* );
void ERRPUSHPARAMLOCATION( struct $8FBSYMBOL*, int64, int64, char* );
void ERRPOPPARAMLOCATION( void );
void ERRREPORT( int64, int64, char* );
void ERRREPORTPARAM( struct $8FBSYMBOL*, int64, char*, int64 );
void ASTDELNODE( struct $7ASTNODE* );
struct $7ASTNODE* ASTCLONETREE( struct $7ASTNODE* );
void ASTDELTREE( struct $7ASTNODE* );
struct $7ASTNODE* ASTBUILDBRANCH( struct $7ASTNODE*, struct $8FBSYMBOL*, int64, int64 );
struct $7ASTNODE* ASTNEWASSIGN( struct $7ASTNODE*, struct $7ASTNODE*, $9AST_OPOPT );
typedef int64 $11AST_CONVOPT;
struct $7ASTNODE* ASTNEWCONV( int64, struct $8FBSYMBOL*, struct $7ASTNODE*, $11AST_CONVOPT, int64* );
struct $7ASTNODE* ASTNEWBOP( int64, struct $7ASTNODE*, struct $7ASTNODE*, struct $8FBSYMBOL*, $9AST_OPOPT );
struct $7ASTNODE* ASTNEWSELFBOP( int64, struct $7ASTNODE*, struct $7ASTNODE*, struct $8FBSYMBOL*, $9AST_OPOPT );
struct $7ASTNODE* ASTNEWCONSTI( int64, int64, struct $8FBSYMBOL* );
struct $7ASTNODE* ASTNEWVAR( struct $8FBSYMBOL*, int64, int64, struct $8FBSYMBOL* );
struct $7ASTNODE* ASTNEWFIELD( struct $7ASTNODE*, struct $8FBSYMBOL* );
struct $7ASTNODE* ASTNEWDEREF( struct $7ASTNODE*, int64, struct $8FBSYMBOL*, int64 );
struct $7ASTNODE* ASTNEWCALL( struct $8FBSYMBOL*, struct $7ASTNODE*, int64 );
struct $7ASTNODE* ASTNEWARG( struct $7ASTNODE*, struct $7ASTNODE*, int64, int64 );
void ASTREPLACEINSTANCEARG( struct $7ASTNODE*, struct $7ASTNODE*, int64 );
struct $7ASTNODE* ASTNEWADDROF( struct $7ASTNODE* );
struct $7ASTNODE* ASTNEWBRANCH( int64, struct $8FBSYMBOL*, struct $7ASTNODE* );
typedef int64 $15AST_LINK_RETURN;
struct $7ASTNODE* ASTNEWLINK( struct $7ASTNODE*, struct $7ASTNODE*, $15AST_LINK_RETURN );
struct $7ASTNODE* ASTNEWLABEL( struct $8FBSYMBOL*, int64 );
struct $7ASTNODE* ASTNEWMEM( int64, struct $7ASTNODE*, struct $7ASTNODE*, int64, int64 );
struct $7ASTNODE* ASTBUILDPTRCHK( struct $7ASTNODE* );
struct $7ASTNODE* ASTNEWNIDXARRAY( struct $7ASTNODE* );
int64 ASTHASSIDEFX( struct $7ASTNODE* );
struct $7ASTNODE* ASTTYPEINIBEGIN( int64, struct $8FBSYMBOL*, int64, int64 );
void ASTTYPEINIEND( struct $7ASTNODE*, int64 );
struct $7ASTNODE* ASTTYPEINIADDPAD( struct $7ASTNODE*, int64 );
struct $7ASTNODE* ASTTYPEINIADDASSIGN( struct $7ASTNODE*, struct $7ASTNODE*, struct $8FBSYMBOL*, int64, struct $8FBSYMBOL*, int64 );
struct $7ASTNODE* ASTTYPEINIADDCTORLIST( struct $7ASTNODE*, struct $8FBSYMBOL*, int64, int64, struct $8FBSYMBOL* );
struct $7ASTNODE* ASTTYPEINISCOPEBEGIN( struct $7ASTNODE*, struct $8FBSYMBOL*, int64 );
struct $7ASTNODE* ASTTYPEINISCOPEEND( struct $7ASTNODE*, struct $8FBSYMBOL* );
struct $7ASTNODE* ASTREMSIDEFX( struct $7ASTNODE** );
struct $7ASTNODE* ASTBUILDVARINC( struct $8FBSYMBOL*, int64 );
struct $7ASTNODE* _Z17ASTBUILDVARASSIGNP8FBSYMBOLll( struct $8FBSYMBOL*, int64, int64 );
struct $7ASTNODE* _Z17ASTBUILDVARASSIGNP8FBSYMBOLP7ASTNODEl( struct $8FBSYMBOL*, struct $7ASTNODE*, int64 );
struct $7ASTNODE* ASTBUILDFAKEWSTRINGACCESS( struct $8FBSYMBOL* );
struct $7ASTNODE* _Z19ASTBUILDDEREFADDROFP7ASTNODES0_lP8FBSYMBOLS2_( struct $7ASTNODE*, struct $7ASTNODE*, int64, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
struct $7ASTNODE* _Z19ASTBUILDDEREFADDROFP7ASTNODExlP8FBSYMBOLS2_( struct $7ASTNODE*, int64, int64, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
struct $7ASTNODE* ASTBUILDCALL( struct $8FBSYMBOL*, struct $7ASTNODE*, struct $7ASTNODE*, struct $7ASTNODE* );
struct $7ASTNODE* ASTBUILDVTABLELOOKUP( struct $8FBSYMBOL*, struct $7ASTNODE* );
struct $7ASTNODE* ASTBUILDDTORCALL( struct $8FBSYMBOL*, struct $7ASTNODE*, int64 );
struct $7ASTNODE* ASTCALLCTORTOCALL( struct $7ASTNODE* );
struct $7ASTNODE* ASTBUILDIMPLICITCTORCALL( struct $8FBSYMBOL*, struct $7ASTNODE*, $12FB_PARAMMODE, int64* );
void ASTDTORLISTDEL( struct $8FBSYMBOL* );
struct $7ASTNODE* ASTBUILDBYREFRESULTDEREF( struct $7ASTNODE* );
struct $8FBSYMBOL* SYMBFINDUOPOVLPROC( $6AST_OP, struct $7ASTNODE*, $9FB_ERRMSG* );
typedef int64 $14FB_SYMBFINDOPT;
struct $8FBSYMBOL* SYMBFINDCTOROVLPROC( struct $8FBSYMBOL*, struct $7ASTNODE*, $12FB_PARAMMODE, $9FB_ERRMSG*, $14FB_SYMBFINDOPT );
int64 SYMBPROCGETVTABLEINDEX( struct $8FBSYMBOL* );
struct $8FBSYMBOL* SYMBADDPROCPTRFROMFUNCTION( struct $8FBSYMBOL* );
int64 SYMBPROCRETURNSONSTACK( struct $8FBSYMBOL* );
void SYMBDELSYMBOL( struct $8FBSYMBOL*, int64 );
int64 SYMBGETREALSIZE( struct $8FBSYMBOL* );
int64 SYMBHASDTOR( struct $8FBSYMBOL* );
int64 TYPEHASDTOR( int64, struct $8FBSYMBOL* );
struct $8FBSYMBOL* SYMBGETCOMPCTORHEAD( struct $8FBSYMBOL* );
struct $8FBSYMBOL* SYMBGETCOMPDEFCTOR( struct $8FBSYMBOL* );
struct $8FBSYMBOL* SYMBGETCOMPDTOR1( struct $8FBSYMBOL* );
int64 SYMBCHECKACCESS( struct $8FBSYMBOL* );
struct $7ASTNODE* RTLSTRDELETE( struct $7ASTNODE* );
struct $7ASTNODE* RTLWSTRALLOC( struct $7ASTNODE* );
struct $7ASTNODE* RTLWSTRLEN( struct $7ASTNODE* );
struct $7ASTNODE* RTLARRAYERASE( struct $7ASTNODE*, int64, int64 );
struct $7ASTNODE* RTLARRAYBOUND( struct $7ASTNODE*, struct $7ASTNODE*, int64 );
static struct $7ASTNODE* ASTFAKEINSTPTR( struct $8FBSYMBOL* );
static struct $7ASTNODE* HCONSTBOUND( struct $7ASTNODE*, struct $7ASTNODE*, int64 );
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
struct $12FBHASHTBLIST {
	struct $8FBHASHTB* HEAD;
	struct $8FBHASHTB* TAIL;
};
__FB_STATIC_ASSERT( sizeof( struct $12FBHASHTBLIST ) == 16 );
struct $8TSTACKTB;
struct $10TSTACKNODE;
struct $10TSTACKNODE {
	struct $10TSTACKNODE* PREV;
	struct $10TSTACKNODE* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $10TSTACKNODE ) == 16 );
struct $8TSTACKTB {
	struct $8TSTACKTB* NEXT;
	struct $10TSTACKNODE* NODETB;
	int64 NODES;
};
__FB_STATIC_ASSERT( sizeof( struct $8TSTACKTB ) == 24 );
struct $6TSTACK {
	struct $8TSTACKTB* TBHEAD;
	struct $8TSTACKTB* TBTAIL;
	int64 NODES;
	int64 NODELEN;
	struct $10TSTACKNODE* TOS;
	int64 CLEAR;
};
__FB_STATIC_ASSERT( sizeof( struct $6TSTACK ) == 48 );
struct $5TPOOL {
	int64 CHUNKS;
	int64 CHUNKSIZE;
	struct $5TLIST* CHUNKTB;
};
__FB_STATIC_ASSERT( sizeof( struct $5TPOOL ) == 24 );
struct $17SYMB_DEF_UNIQUEID {
	struct $5THASH DICT;
};
__FB_STATIC_ASSERT( sizeof( struct $17SYMB_DEF_UNIQUEID ) == 24 );
struct $14SYMB_DEF_PARAM {
	struct $8HASHITEM* ITEM;
	uint64 INDEX;
};
__FB_STATIC_ASSERT( sizeof( struct $14SYMB_DEF_PARAM ) == 16 );
struct $12SYMB_DEF_CTX {
	struct $5TLIST PARAMLIST;
	struct $5TLIST TOKLIST;
	struct $17SYMB_DEF_UNIQUEID UNIQUEID;
	int64 PARAM;
	struct $5THASH PARAMHASH;
	struct $14SYMB_DEF_PARAM HASH[32];
};
__FB_STATIC_ASSERT( sizeof( struct $12SYMB_DEF_CTX ) == 696 );
struct $20FB_GLOBCTORLIST_ITEM;
struct $20FB_GLOBCTORLIST_ITEM {
	struct $8FBSYMBOL* SYM;
	struct $20FB_GLOBCTORLIST_ITEM* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $20FB_GLOBCTORLIST_ITEM ) == 16 );
struct $15FB_GLOBCTORLIST {
	struct $20FB_GLOBCTORLIST_ITEM* HEAD;
	struct $20FB_GLOBCTORLIST_ITEM* TAIL;
	struct $5TLIST LIST;
};
__FB_STATIC_ASSERT( sizeof( struct $15FB_GLOBCTORLIST ) == 80 );
struct $10SYMB_OVLOP {
	struct $8FBSYMBOL* HEAD;
};
__FB_STATIC_ASSERT( sizeof( struct $10SYMB_OVLOP ) == 8 );
struct $10FB_RTTICTX {
	struct $8FBSYMBOL* FB_RTTI;
	struct $8FBSYMBOL* FB_OBJECT;
};
__FB_STATIC_ASSERT( sizeof( struct $10FB_RTTICTX ) == 16 );
struct $7SYMBCTX {
	int64 INITED;
	struct $5TLIST SYMLIST;
	struct $12FBHASHTBLIST HASHLIST;
	struct $10FBSYMCHAIN CHAINPOOL[4096];
	int64 CHAINPOOLHEAD;
	struct $8FBSYMBOL GLOBNSPC;
	struct $8FBSYMBOL* NAMESPC;
	struct $8FBHASHTB* HASHTB;
	struct $10FBSYMBOLTB* SYMTB;
	struct $6TSTACK NESTSTK;
	struct $5THASH IMPHASHTB;
	struct $5TLIST IMPHASHLIST;
	struct $5TPOOL NAMEPOOL;
	struct $5TLIST FWDLIST;
	struct $5TLIST NSEXTLIST;
	int64 FWDREFCNT;
	struct $12SYMB_DEF_CTX DEF;
	struct $8FBSYMBOL* LASTLBL;
	struct $15FB_GLOBCTORLIST GLOBCTORLIST;
	struct $15FB_GLOBCTORLIST GLOBDTORLIST;
	struct $10SYMB_OVLOP GLOBOPOVLTB[122];
	int64 FBARRAY_DATA;
	int64 FBARRAY_PTR;
	int64 FBARRAY_SIZE;
	int64 FBARRAY_DIMTB;
	struct $8FBSYMBOL* FBARRAYDIM;
	int64 FBARRAYDIM_LBOUND;
	int64 FBARRAYDIM_UBOUND;
	struct $10FB_RTTICTX RTTI;
};
__FB_STATIC_ASSERT( sizeof( struct $7SYMBCTX ) == 199256 );
extern struct $7SYMBCTX SYMB$;
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
typedef int64 $13FB_COMPTARGET;
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

struct $7ASTNODE* _Z17ASTBUILDVARASSIGNP8FBSYMBOLll( struct $8FBSYMBOL* LHS$1, int64 RHS$1, int64 OPTIONS$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$10:;
	struct $7ASTNODE* vr$1 = ASTNEWCONSTI( RHS$1, 8ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$2 = ASTNEWVAR( LHS$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$3 = ASTNEWASSIGN( vr$2, vr$1, OPTIONS$1 );
	fb$result$1 = vr$3;
	label$11:;
	return fb$result$1;
}

struct $7ASTNODE* _Z17ASTBUILDVARASSIGNP8FBSYMBOLP7ASTNODEl( struct $8FBSYMBOL* LHS$1, struct $7ASTNODE* RHS$1, int64 OPTIONS$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$12:;
	struct $7ASTNODE* vr$1 = ASTNEWVAR( LHS$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$2 = ASTNEWASSIGN( vr$1, RHS$1, OPTIONS$1 );
	fb$result$1 = vr$2;
	label$13:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDFAKEWSTRINGACCESS( struct $8FBSYMBOL* SYM$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$14:;
	struct $7ASTNODE* vr$1 = ASTNEWVAR( SYM$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$2 = ASTNEWDEREF( vr$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	fb$result$1 = vr$2;
	label$15:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDFAKEWSTRINGASSIGN( struct $8FBSYMBOL* SYM$1, struct $7ASTNODE* EXPR$1, int64 OPTIONS$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$16:;
	struct $7ASTNODE* T$1;
	T$1 = (struct $7ASTNODE*)0ull;
	int64 vr$1 = ASTHASSIDEFX( EXPR$1 );
	if( vr$1 == 0ll) goto label$19;
	{
		struct $7ASTNODE* vr$3 = ASTREMSIDEFX( &EXPR$1 );
		struct $7ASTNODE* vr$4 = ASTNEWLINK( T$1, vr$3, 2ll );
		T$1 = vr$4;
	}
	label$19:;
	label$18:;
	struct $7ASTNODE* vr$5 = ASTCLONETREE( EXPR$1 );
	struct $7ASTNODE* vr$6 = RTLWSTRLEN( vr$5 );
	struct $7ASTNODE* vr$7 = RTLWSTRALLOC( vr$6 );
	struct $7ASTNODE* vr$8 = _Z17ASTBUILDVARASSIGNP8FBSYMBOLP7ASTNODEl( SYM$1, vr$7, OPTIONS$1 );
	struct $7ASTNODE* vr$9 = ASTNEWLINK( T$1, vr$8, 0ll );
	T$1 = vr$9;
	struct $7ASTNODE* vr$10 = ASTBUILDFAKEWSTRINGACCESS( SYM$1 );
	struct $7ASTNODE* vr$11 = ASTNEWASSIGN( vr$10, EXPR$1, OPTIONS$1 );
	struct $7ASTNODE* vr$12 = ASTNEWLINK( T$1, vr$11, 2ll );
	T$1 = vr$12;
	fb$result$1 = T$1;
	label$17:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDVARINC( struct $8FBSYMBOL* LHS$1, int64 RHS$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$20:;
	$9AST_OPOPT OPTIONS$1;
	$6AST_OP OP$1;
	OPTIONS$1 = 1ll;
	if( ((*(int64*)((uint8*)LHS$1 + 56ll) & 511ll) & 480ll) == 0ll) goto label$23;
	{
		OPTIONS$1 = OPTIONS$1 | 2ll;
	}
	label$23:;
	label$22:;
	if( RHS$1 <= 0ll) goto label$25;
	{
		OP$1 = 1ll;
	}
	goto label$24;
	label$25:;
	{
		OP$1 = 2ll;
		RHS$1 = -RHS$1;
	}
	label$24:;
	struct $7ASTNODE* vr$6 = ASTNEWCONSTI( RHS$1, 8ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$7 = ASTNEWVAR( LHS$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$8 = ASTNEWSELFBOP( OP$1, vr$7, vr$6, (struct $8FBSYMBOL*)0ull, OPTIONS$1 );
	fb$result$1 = vr$8;
	label$21:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDVARDEREF( struct $8FBSYMBOL* SYM$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$26:;
	struct $7ASTNODE* vr$1 = ASTNEWVAR( SYM$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$2 = ASTNEWDEREF( vr$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	fb$result$1 = vr$2;
	label$27:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDVARADDROF( struct $8FBSYMBOL* SYM$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$28:;
	struct $7ASTNODE* vr$1 = ASTNEWVAR( SYM$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$2 = ASTNEWADDROF( vr$1 );
	fb$result$1 = vr$2;
	label$29:;
	return fb$result$1;
}

struct $7ASTNODE* _Z19ASTBUILDVARDTORCALLP7ASTNODE( struct $7ASTNODE* VAREXPR$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$30:;
	if( (*(int64*)((uint8*)VAREXPR$1 + 8ll) & 511ll) != 17ll) goto label$33;
	{
		struct $7ASTNODE* vr$3 = RTLSTRDELETE( VAREXPR$1 );
		fb$result$1 = vr$3;
	}
	goto label$32;
	label$33:;
	int64 vr$6 = TYPEHASDTOR( *(int64*)((uint8*)VAREXPR$1 + 8ll), *(struct $8FBSYMBOL**)((uint8*)VAREXPR$1 + 16ll) );
	if( vr$6 == 0ll) goto label$34;
	{
		struct $7ASTNODE* vr$8 = ASTBUILDDTORCALL( *(struct $8FBSYMBOL**)((uint8*)VAREXPR$1 + 16ll), VAREXPR$1, 0ll );
		fb$result$1 = vr$8;
	}
	label$34:;
	label$32:;
	label$31:;
	return fb$result$1;
}

struct $7ASTNODE* _Z19ASTBUILDVARDTORCALLP8FBSYMBOLl( struct $8FBSYMBOL* S$1, int64 CHECK_ACCESS$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$35:;
	fb$result$1 = (struct $7ASTNODE*)0ull;
	if( *(int64*)((uint8*)S$1 + 104ll) == 0ll) goto label$38;
	{
		struct $7ASTNODE* vr$5 = ASTNEWVAR( S$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
		struct $7ASTNODE* vr$6 = ASTNEWNIDXARRAY( vr$5 );
		struct $7ASTNODE* vr$7 = RTLARRAYERASE( vr$6, (int64)-((*(int64*)((uint8*)S$1 + 8ll) & 4ll) != 0ll), CHECK_ACCESS$1 );
		fb$result$1 = vr$7;
	}
	goto label$37;
	label$38:;
	{
		{
			int64 TMP$95$3;
			TMP$95$3 = *(int64*)((uint8*)S$1 + 56ll) & 511ll;
			if( TMP$95$3 != 17ll) goto label$40;
			label$41:;
			{
				struct $7ASTNODE* vr$10 = ASTNEWVAR( S$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
				struct $7ASTNODE* vr$11 = RTLSTRDELETE( vr$10 );
				fb$result$1 = vr$11;
			}
			goto label$39;
			label$40:;
			if( TMP$95$3 != 39ll) goto label$42;
			label$43:;
			{
				struct $7ASTNODE* vr$12 = ASTNEWVAR( S$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
				struct $7ASTNODE* vr$13 = RTLSTRDELETE( vr$12 );
				fb$result$1 = vr$13;
			}
			goto label$39;
			label$42:;
			{
				int64 vr$14 = SYMBHASDTOR( S$1 );
				if( vr$14 == 0ll) goto label$46;
				{
					if( CHECK_ACCESS$1 == 0ll) goto label$48;
					{
						struct $8FBSYMBOL* vr$16 = SYMBGETCOMPDTOR1( *(struct $8FBSYMBOL**)((uint8*)S$1 + 64ll) );
						int64 vr$17 = SYMBCHECKACCESS( vr$16 );
						if( vr$17 != 0ll) goto label$50;
						{
							ERRREPORT( 206ll, 0ll, (char*)0ull );
						}
						label$50:;
						label$49:;
					}
					label$48:;
					label$47:;
					struct $7ASTNODE* vr$18 = ASTNEWVAR( S$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
					struct $7ASTNODE* vr$20 = ASTBUILDDTORCALL( *(struct $8FBSYMBOL**)((uint8*)S$1 + 64ll), vr$18, 0ll );
					fb$result$1 = vr$20;
				}
				label$46:;
				label$45:;
			}
			label$44:;
			label$39:;
		}
	}
	label$37:;
	label$36:;
	return fb$result$1;
}

struct $7ASTNODE* _Z19ASTBUILDDEREFADDROFP7ASTNODES0_lP8FBSYMBOLS2_( struct $7ASTNODE* N$1, struct $7ASTNODE* OFFSETEXPR$1, int64 DTYPE$1, struct $8FBSYMBOL* SUBTYPE$1, struct $8FBSYMBOL* MAYBEAFIELD$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$51:;
	struct $7ASTNODE* vr$1 = ASTNEWADDROF( N$1 );
	N$1 = vr$1;
	if( OFFSETEXPR$1 == (struct $7ASTNODE*)0ull) goto label$54;
	{
		struct $7ASTNODE* vr$2 = ASTNEWBOP( 28ll, N$1, OFFSETEXPR$1, (struct $8FBSYMBOL*)0ull, 1ll );
		N$1 = vr$2;
	}
	label$54:;
	label$53:;
	struct $7ASTNODE* vr$12 = ASTNEWCONV( (((DTYPE$1 & 31ll) | ((DTYPE$1 & 480ll) + 32ll)) | ((DTYPE$1 & 261632ll) << (1ll & 63ll))) | (DTYPE$1 & 32505856ll), SUBTYPE$1, N$1, 24ll, (int64*)0ull );
	N$1 = vr$12;
	struct $7ASTNODE* vr$13 = ASTNEWDEREF( N$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	N$1 = vr$13;
	if( MAYBEAFIELD$1 == (struct $8FBSYMBOL*)0ull) goto label$56;
	{
		if( *(int64*)MAYBEAFIELD$1 != 12ll) goto label$58;
		{
			struct $7ASTNODE* vr$15 = ASTNEWFIELD( N$1, MAYBEAFIELD$1 );
			N$1 = vr$15;
		}
		label$58:;
		label$57:;
	}
	label$56:;
	label$55:;
	fb$result$1 = N$1;
	label$52:;
	return fb$result$1;
}

struct $7ASTNODE* _Z19ASTBUILDDEREFADDROFP7ASTNODExlP8FBSYMBOLS2_( struct $7ASTNODE* N$1, int64 OFFSET$1, int64 DTYPE$1, struct $8FBSYMBOL* SUBTYPE$1, struct $8FBSYMBOL* MAYBEAFIELD$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$59:;
	struct $7ASTNODE* OFFSETEXPR$1;
	if( OFFSET$1 != 0ll) goto label$62;
	{
		OFFSETEXPR$1 = (struct $7ASTNODE*)0ull;
	}
	goto label$61;
	label$62:;
	{
		struct $7ASTNODE* vr$1 = ASTNEWCONSTI( OFFSET$1, 8ll, (struct $8FBSYMBOL*)0ull );
		OFFSETEXPR$1 = vr$1;
	}
	label$61:;
	struct $7ASTNODE* vr$2 = _Z19ASTBUILDDEREFADDROFP7ASTNODES0_lP8FBSYMBOLS2_( N$1, OFFSETEXPR$1, DTYPE$1, SUBTYPE$1, MAYBEAFIELD$1 );
	fb$result$1 = vr$2;
	label$60:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDVARFIELD( struct $8FBSYMBOL* SYM$1, struct $8FBSYMBOL* FLD$1, int64 OFFSET$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$63:;
	struct $7ASTNODE* N$1;
	if( ((int64)-((*(int64*)((uint8*)SYM$1 + 8ll) & 65536ll) != 0ll) | (int64)-((*(int64*)((uint8*)SYM$1 + 8ll) & 512ll) != 0ll)) == 0ll) goto label$66;
	{
		struct $7ASTNODE* vr$22 = ASTNEWVAR( SYM$1, 0ll, (((*(int64*)((uint8*)SYM$1 + 56ll) & 31ll) | ((*(int64*)((uint8*)SYM$1 + 56ll) & 480ll) + 32ll)) | ((*(int64*)((uint8*)SYM$1 + 56ll) & 261632ll) << (1ll & 63ll))) | (*(int64*)((uint8*)SYM$1 + 56ll) & 32505856ll), *(struct $8FBSYMBOL**)((uint8*)SYM$1 + 64ll) );
		struct $7ASTNODE* vr$23 = ASTNEWDEREF( vr$22, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
		N$1 = vr$23;
	}
	goto label$65;
	label$66:;
	{
		struct $7ASTNODE* vr$24 = ASTNEWVAR( SYM$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
		N$1 = vr$24;
	}
	label$65:;
	if( FLD$1 == (struct $8FBSYMBOL*)0ull) goto label$68;
	{
		OFFSET$1 = OFFSET$1 + *(int64*)((uint8*)FLD$1 + 88ll);
		struct $7ASTNODE* vr$29 = _Z19ASTBUILDDEREFADDROFP7ASTNODExlP8FBSYMBOLS2_( N$1, OFFSET$1, *(int64*)((uint8*)FLD$1 + 56ll), *(struct $8FBSYMBOL**)((uint8*)FLD$1 + 64ll), FLD$1 );
		N$1 = vr$29;
	}
	goto label$67;
	label$68:;
	{
		struct $7ASTNODE* vr$32 = _Z19ASTBUILDDEREFADDROFP7ASTNODExlP8FBSYMBOLS2_( N$1, OFFSET$1, *(int64*)((uint8*)SYM$1 + 56ll), *(struct $8FBSYMBOL**)((uint8*)SYM$1 + 64ll), (struct $8FBSYMBOL*)0ull );
		N$1 = vr$32;
	}
	label$67:;
	fb$result$1 = N$1;
	label$64:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDTEMPVARCLEAR( struct $8FBSYMBOL* SYM$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$69:;
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 2ll) == 0ll) goto label$72;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$70;
	}
	label$72:;
	label$71:;
	struct $7ASTNODE* vr$4 = ASTNEWCONSTI( *(int64*)((uint8*)SYM$1 + 80ll), 8ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$5 = ASTNEWVAR( SYM$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$6 = ASTNEWMEM( 108ll, vr$5, vr$4, 0ll, 0ll );
	fb$result$1 = vr$6;
	label$70:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDWHILECOUNTERBEGIN( struct $7ASTNODE* TREE$1, struct $8FBSYMBOL* CNT$1, struct $8FBSYMBOL* LABEL$1, struct $8FBSYMBOL* EXITLABEL$1, struct $7ASTNODE* INITEXPR$1, int64 FLUSH_LABEL$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$73:;
	struct $7ASTNODE* vr$1 = _Z17ASTBUILDVARASSIGNP8FBSYMBOLP7ASTNODEl( CNT$1, INITEXPR$1, 0ll );
	struct $7ASTNODE* vr$2 = ASTNEWLINK( TREE$1, vr$1, 0ll );
	TREE$1 = vr$2;
	struct $7ASTNODE* vr$3 = ASTNEWLABEL( LABEL$1, FLUSH_LABEL$1 );
	struct $7ASTNODE* vr$4 = ASTNEWLINK( TREE$1, vr$3, 0ll );
	TREE$1 = vr$4;
	struct $7ASTNODE* vr$5 = ASTNEWCONSTI( 0ll, 8ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$6 = ASTNEWVAR( CNT$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$7 = ASTNEWBOP( 45ll, vr$6, vr$5, (struct $8FBSYMBOL*)0ull, 1ll );
	struct $7ASTNODE* vr$8 = ASTBUILDBRANCH( vr$7, EXITLABEL$1, -1ll, 0ll );
	struct $7ASTNODE* vr$9 = ASTNEWLINK( TREE$1, vr$8, 0ll );
	TREE$1 = vr$9;
	fb$result$1 = TREE$1;
	label$74:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDWHILECOUNTEREND( struct $7ASTNODE* TREE$1, struct $8FBSYMBOL* CNT$1, struct $8FBSYMBOL* LABEL$1, struct $8FBSYMBOL* EXITLABEL$1, int64 FLUSH_LABEL$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$75:;
	struct $7ASTNODE* vr$1 = ASTBUILDVARINC( CNT$1, -1ll );
	struct $7ASTNODE* vr$2 = ASTNEWLINK( TREE$1, vr$1, 0ll );
	TREE$1 = vr$2;
	struct $7ASTNODE* vr$3 = ASTNEWBRANCH( 99ll, LABEL$1, (struct $7ASTNODE*)0ull );
	struct $7ASTNODE* vr$4 = ASTNEWLINK( TREE$1, vr$3, 0ll );
	TREE$1 = vr$4;
	struct $7ASTNODE* vr$5 = ASTNEWLABEL( EXITLABEL$1, FLUSH_LABEL$1 );
	struct $7ASTNODE* vr$6 = ASTNEWLINK( TREE$1, vr$5, 0ll );
	TREE$1 = vr$6;
	fb$result$1 = TREE$1;
	label$76:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDFORBEGIN( struct $7ASTNODE* TREE$1, struct $8FBSYMBOL* CNT$1, struct $8FBSYMBOL* LABEL$1, int64 INIVALUE$1, int64 FLUSH_LABEL$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$77:;
	struct $7ASTNODE* vr$1 = _Z17ASTBUILDVARASSIGNP8FBSYMBOLll( CNT$1, INIVALUE$1, 0ll );
	struct $7ASTNODE* vr$2 = ASTNEWLINK( TREE$1, vr$1, 0ll );
	TREE$1 = vr$2;
	struct $7ASTNODE* vr$3 = ASTNEWLABEL( LABEL$1, FLUSH_LABEL$1 );
	struct $7ASTNODE* vr$4 = ASTNEWLINK( TREE$1, vr$3, 0ll );
	TREE$1 = vr$4;
	fb$result$1 = TREE$1;
	label$78:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDFOREND( struct $7ASTNODE* TREE$1, struct $8FBSYMBOL* CNT$1, struct $8FBSYMBOL* LABEL$1, struct $7ASTNODE* ENDVALUE$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$79:;
	struct $7ASTNODE* vr$1 = ASTBUILDVARINC( CNT$1, 1ll );
	struct $7ASTNODE* vr$2 = ASTNEWLINK( TREE$1, vr$1, 0ll );
	TREE$1 = vr$2;
	struct $7ASTNODE* vr$3 = ASTNEWVAR( CNT$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$4 = ASTNEWBOP( 45ll, vr$3, ENDVALUE$1, (struct $8FBSYMBOL*)0ull, 1ll );
	struct $7ASTNODE* vr$5 = ASTBUILDBRANCH( vr$4, LABEL$1, 0ll, 0ll );
	struct $7ASTNODE* vr$6 = ASTNEWLINK( TREE$1, vr$5, 0ll );
	TREE$1 = vr$6;
	fb$result$1 = TREE$1;
	label$80:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDVTABLELOOKUP( struct $8FBSYMBOL* PROC$1, struct $7ASTNODE* THISEXPR$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$81:;
	struct $7ASTNODE* P$1;
	int64 VTABLEINDEX$1;
	if( (*(int64*)((uint8*)PROC$1 + 16ll) & 256ll) == 0ll) goto label$84;
	{
		int64 vr$3 = SYMBPROCGETVTABLEINDEX( PROC$1 );
		VTABLEINDEX$1 = vr$3;
		struct $7ASTNODE* vr$4 = ASTCLONETREE( THISEXPR$1 );
		P$1 = vr$4;
		struct $7ASTNODE* vr$5 = ASTNEWADDROF( P$1 );
		P$1 = vr$5;
		struct $7ASTNODE* vr$6 = ASTNEWCONV( 96ll, (struct $8FBSYMBOL*)0ull, P$1, 8ll, (int64*)0ull );
		P$1 = vr$6;
		struct $7ASTNODE* vr$7 = ASTNEWDEREF( P$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
		P$1 = vr$7;
		struct $7ASTNODE* vr$9 = ASTNEWCONSTI( VTABLEINDEX$1 + -2ll, 8ll, (struct $8FBSYMBOL*)0ull );
		struct $7ASTNODE* vr$10 = ASTNEWBOP( 28ll, P$1, vr$9, (struct $8FBSYMBOL*)0ull, 7ll );
		P$1 = vr$10;
		struct $7ASTNODE* vr$11 = ASTNEWDEREF( P$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
		P$1 = vr$11;
		struct $8FBSYMBOL* vr$12 = SYMBADDPROCPTRFROMFUNCTION( PROC$1 );
		struct $7ASTNODE* vr$13 = ASTNEWCONV( 54ll, vr$12, P$1, 0ll, (int64*)0ull );
		P$1 = vr$13;
		if( *(int64*)((uint8*)&ENV$ + 360ll) == 0ll) goto label$86;
		{
			if( (*(int64*)((uint8*)PROC$1 + 16ll) & 512ll) == 0ll) goto label$88;
			{
				struct $7ASTNODE* vr$16 = ASTBUILDPTRCHK( P$1 );
				P$1 = vr$16;
			}
			label$88:;
			label$87:;
		}
		label$86:;
		label$85:;
	}
	goto label$83;
	label$84:;
	{
		P$1 = (struct $7ASTNODE*)0ull;
	}
	label$83:;
	fb$result$1 = P$1;
	label$82:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDCALL( struct $8FBSYMBOL* PROC$1, struct $7ASTNODE* ARG1$1, struct $7ASTNODE* ARG2$1, struct $7ASTNODE* ARG3$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$89:;
	struct $7ASTNODE* P$1;
	struct $7ASTNODE* PTREXPR$1;
	if( (*(int64*)((uint8*)PROC$1 + 16ll) & 256ll) == 0ll) goto label$92;
	{
		struct $7ASTNODE* vr$3 = ASTBUILDVTABLELOOKUP( PROC$1, ARG1$1 );
		PTREXPR$1 = vr$3;
	}
	goto label$91;
	label$92:;
	{
		PTREXPR$1 = (struct $7ASTNODE*)0ull;
	}
	label$91:;
	struct $7ASTNODE* vr$4 = ASTNEWCALL( PROC$1, PTREXPR$1, -1ll );
	P$1 = vr$4;
	if( ARG1$1 == (struct $7ASTNODE*)0ull) goto label$94;
	{
		struct $7ASTNODE* vr$5 = ASTNEWARG( P$1, ARG1$1, 2147483648ll, -1ll );
		if( vr$5 != (struct $7ASTNODE*)0ull) goto label$96;
		{
			fb$result$1 = (struct $7ASTNODE*)0ull;
			goto label$90;
		}
		label$96:;
		label$95:;
	}
	label$94:;
	label$93:;
	if( ARG2$1 == (struct $7ASTNODE*)0ull) goto label$98;
	{
		struct $7ASTNODE* vr$6 = ASTNEWARG( P$1, ARG2$1, 2147483648ll, -1ll );
		if( vr$6 != (struct $7ASTNODE*)0ull) goto label$100;
		{
			fb$result$1 = (struct $7ASTNODE*)0ull;
			goto label$90;
		}
		label$100:;
		label$99:;
	}
	label$98:;
	label$97:;
	if( ARG3$1 == (struct $7ASTNODE*)0ull) goto label$102;
	{
		struct $7ASTNODE* vr$7 = ASTNEWARG( P$1, ARG3$1, 2147483648ll, -1ll );
		if( vr$7 != (struct $7ASTNODE*)0ull) goto label$104;
		{
			fb$result$1 = (struct $7ASTNODE*)0ull;
			goto label$90;
		}
		label$104:;
		label$103:;
	}
	label$102:;
	label$101:;
	struct $7ASTNODE* vr$8 = ASTBUILDBYREFRESULTDEREF( P$1 );
	P$1 = vr$8;
	fb$result$1 = P$1;
	label$90:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDCTORCALL( struct $8FBSYMBOL* SYM$1, struct $7ASTNODE* THISEXPR$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$105:;
	struct $8FBSYMBOL* CTOR$1;
	struct $7ASTNODE* PROC$1;
	int64 PARAMS$1;
	struct $8FBSYMBOL* vr$1 = SYMBGETCOMPDEFCTOR( SYM$1 );
	CTOR$1 = vr$1;
	if( CTOR$1 != (struct $8FBSYMBOL*)0ull) goto label$108;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$106;
	}
	label$108:;
	label$107:;
	struct $7ASTNODE* vr$2 = ASTNEWCALL( CTOR$1, (struct $7ASTNODE*)0ull, -1ll );
	PROC$1 = vr$2;
	ASTNEWARG( PROC$1, THISEXPR$1, 2147483648ll, -1ll );
	PARAMS$1 = (int64)*(int16*)((uint8*)CTOR$1 + 120ll) + -1ll;
	label$109:;
	if( PARAMS$1 <= 0ll) goto label$110;
	{
		ASTNEWARG( PROC$1, (struct $7ASTNODE*)0ull, 2147483648ll, -1ll );
		PARAMS$1 = PARAMS$1 + -1ll;
	}
	goto label$109;
	label$110:;
	fb$result$1 = PROC$1;
	label$106:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDDTORCALL( struct $8FBSYMBOL* SYM$1, struct $7ASTNODE* THISEXPR$1, int64 IGNORE_VIRTUAL$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$111:;
	struct $8FBSYMBOL* DTOR$1;
	struct $7ASTNODE* CALLEXPR$1;
	struct $8FBSYMBOL* vr$1 = SYMBGETCOMPDTOR1( SYM$1 );
	DTOR$1 = vr$1;
	if( IGNORE_VIRTUAL$1 == 0ll) goto label$114;
	{
		struct $7ASTNODE* vr$2 = ASTNEWCALL( DTOR$1, (struct $7ASTNODE*)0ull, -1ll );
		CALLEXPR$1 = vr$2;
	}
	goto label$113;
	label$114:;
	{
		struct $7ASTNODE* vr$3 = ASTBUILDVTABLELOOKUP( DTOR$1, THISEXPR$1 );
		struct $7ASTNODE* vr$4 = ASTNEWCALL( DTOR$1, vr$3, -1ll );
		CALLEXPR$1 = vr$4;
	}
	label$113:;
	ASTNEWARG( CALLEXPR$1, THISEXPR$1, 2147483648ll, -1ll );
	fb$result$1 = CALLEXPR$1;
	label$112:;
	return fb$result$1;
}

struct $7ASTNODE* ASTPATCHCTORCALL( struct $7ASTNODE* PROCEXPR$1, struct $7ASTNODE* THISEXPR$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$117:;
	ASTREPLACEINSTANCEARG( PROCEXPR$1, THISEXPR$1, -1ll );
	fb$result$1 = PROCEXPR$1;
	label$118:;
	return fb$result$1;
}

struct $7ASTNODE* ASTCALLCTORTOCALL( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$119:;
	struct $8FBSYMBOL* SYM$1;
	struct $7ASTNODE* PROCEXPR$1;
	SYM$1 = *(struct $8FBSYMBOL**)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) + 24ll);
	PROCEXPR$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	struct $7ASTNODE* vr$5 = ASTFAKEINSTPTR( *(struct $8FBSYMBOL**)((uint8*)SYM$1 + 64ll) );
	ASTREPLACEINSTANCEARG( PROCEXPR$1, vr$5, 1ll );
	ASTDELTREE( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
	ASTDTORLISTDEL( SYM$1 );
	SYMBDELSYMBOL( SYM$1, 0ll );
	ASTDELNODE( N$1 );
	fb$result$1 = PROCEXPR$1;
	label$120:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDIMPLICITCTORCALL( struct $8FBSYMBOL* SUBTYPE$1, struct $7ASTNODE* EXPR$1, $12FB_PARAMMODE ARG_MODE$1, int64* IS_CTORCALL$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$121:;
	int64 ERR_NUM$1;
	struct $8FBSYMBOL* PROC$1;
	struct $8FBSYMBOL* vr$2 = SYMBFINDCTOROVLPROC( SUBTYPE$1, EXPR$1, ARG_MODE$1, ($9FB_ERRMSG*)&ERR_NUM$1, 0ll );
	PROC$1 = vr$2;
	if( PROC$1 != (struct $8FBSYMBOL*)0ull) goto label$124;
	{
		*IS_CTORCALL$1 = 0ll;
		if( ERR_NUM$1 == 0ll) goto label$126;
		{
			struct $8FBSYMBOL* vr$4 = SYMBGETCOMPCTORHEAD( SUBTYPE$1 );
			ERRREPORTPARAM( vr$4, 0ll, (char*)0ull, ERR_NUM$1 );
			fb$result$1 = (struct $7ASTNODE*)0ull;
			goto label$122;
		}
		label$126:;
		label$125:;
		fb$result$1 = EXPR$1;
		goto label$122;
	}
	label$124:;
	label$123:;
	int64 vr$5 = SYMBCHECKACCESS( PROC$1 );
	if( vr$5 != 0ll) goto label$128;
	{
		ERRREPORT( 205ll, 0ll, (char*)0ull );
	}
	label$128:;
	label$127:;
	struct $7ASTNODE* PROCEXPR$1;
	struct $7ASTNODE* vr$6 = ASTNEWCALL( PROC$1, (struct $7ASTNODE*)0ull, -1ll );
	PROCEXPR$1 = vr$6;
	struct $7ASTNODE* vr$7 = ASTFAKEINSTPTR( SUBTYPE$1 );
	ASTNEWARG( PROCEXPR$1, vr$7, 2147483648ll, 1ll );
	ASTNEWARG( PROCEXPR$1, EXPR$1, 2147483648ll, ARG_MODE$1 );
	int64 PARAMS$1;
	PARAMS$1 = (int64)*(int16*)((uint8*)PROC$1 + 120ll) + -2ll;
	label$129:;
	if( PARAMS$1 <= 0ll) goto label$130;
	{
		ASTNEWARG( PROCEXPR$1, (struct $7ASTNODE*)0ull, 2147483648ll, -1ll );
		PARAMS$1 = PARAMS$1 + -1ll;
	}
	goto label$129;
	label$130:;
	*IS_CTORCALL$1 = -1ll;
	fb$result$1 = PROCEXPR$1;
	label$122:;
	return fb$result$1;
}

$12FB_PARAMMODE ASTBYDESCARRAYARG( struct $7ASTNODE* ARG$1 )
{
	$12FB_PARAMMODE fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$131:;
	fb$result$1 = -1ll;
	if( *(int64*)ARG$1 != 25ll) goto label$134;
	{
		fb$result$1 = 3ll;
	}
	label$134:;
	label$133:;
	label$132:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDIMPLICITCTORCALLEX( struct $8FBSYMBOL* SYM$1, struct $7ASTNODE* EXPR$1, $12FB_PARAMMODE ARG_MODE$1, int64* IS_CTORCALL$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$135:;
	struct $8FBSYMBOL* SUBTYPE$1;
	SUBTYPE$1 = *(struct $8FBSYMBOL**)((uint8*)SYM$1 + 64ll);
	if( *(int64*)EXPR$1 != 10ll) goto label$138;
	{
		if( *(struct $8FBSYMBOL**)((uint8*)EXPR$1 + 16ll) != SUBTYPE$1) goto label$140;
		{
			*IS_CTORCALL$1 = -1ll;
			struct $7ASTNODE* vr$5 = ASTCALLCTORTOCALL( EXPR$1 );
			fb$result$1 = vr$5;
			goto label$136;
		}
		label$140:;
		label$139:;
	}
	label$138:;
	label$137:;
	struct $7ASTNODE* vr$6 = ASTBUILDIMPLICITCTORCALL( SUBTYPE$1, EXPR$1, ARG_MODE$1, IS_CTORCALL$1 );
	fb$result$1 = vr$6;
	label$136:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDPROCADDROF( struct $8FBSYMBOL* PROC$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$141:;
	*($12FB_SYMBSTATS*)((uint8*)PROC$1 + 24ll) = *(int64*)((uint8*)PROC$1 + 24ll) | 2ll;
	struct $7ASTNODE* vr$4 = ASTNEWVAR( PROC$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$5 = ASTNEWADDROF( vr$4 );
	fb$result$1 = vr$5;
	label$142:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDPROCRESULTVAR( struct $8FBSYMBOL* PROC$1, struct $8FBSYMBOL* RES$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$143:;
	int64 vr$1 = SYMBPROCRETURNSONSTACK( PROC$1 );
	if( vr$1 == 0ll) goto label$146;
	{
		struct $7ASTNODE* vr$3 = ASTNEWVAR( RES$1, 0ll, 52ll, *(struct $8FBSYMBOL**)((uint8*)RES$1 + 64ll) );
		struct $7ASTNODE* vr$4 = ASTNEWDEREF( vr$3, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
		fb$result$1 = vr$4;
	}
	goto label$145;
	label$146:;
	{
		struct $7ASTNODE* vr$5 = ASTNEWVAR( RES$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
		fb$result$1 = vr$5;
	}
	label$145:;
	label$144:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDTYPEINICTORLIST( struct $8FBSYMBOL* SYM$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$147:;
	struct $7ASTNODE* TREE$1;
	__builtin_memset( &TREE$1, 0, 8ll );
	struct $7ASTNODE* vr$4 = ASTTYPEINIBEGIN( *(int64*)((uint8*)SYM$1 + 56ll), *(struct $8FBSYMBOL**)((uint8*)SYM$1 + 64ll), -1ll, 0ll );
	TREE$1 = vr$4;
	ASTTYPEINIADDCTORLIST( TREE$1, SYM$1, *(int64*)((uint8*)SYM$1 + 128ll), 2147483648ll, (struct $8FBSYMBOL*)0ull );
	ASTTYPEINIEND( TREE$1, -1ll );
	fb$result$1 = TREE$1;
	label$148:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDMULTIDEREF( int64 CNT$1, struct $7ASTNODE* EXPR$1, int64 DTYPE$1, struct $8FBSYMBOL* SUBTYPE$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$149:;
	label$151:;
	if( CNT$1 <= 0ll) goto label$152;
	{
		if( (DTYPE$1 & 480ll) != 0ll) goto label$154;
		{
			if( *(struct $8FBSYMBOL**)((uint8*)&SYMB$ + 198824ll) != (struct $8FBSYMBOL*)0ull) goto label$156;
			{
				ERRREPORT( 28ll, -1ll, (char*)0ull );
				goto label$152;
			}
			label$156:;
			label$155:;
			struct $8FBSYMBOL* PROC$3;
			$9FB_ERRMSG ERR_NUM$3;
			struct $8FBSYMBOL* vr$3 = SYMBFINDUOPOVLPROC( 77ll, EXPR$1, &ERR_NUM$3 );
			PROC$3 = vr$3;
			if( PROC$3 == (struct $8FBSYMBOL*)0ull) goto label$158;
			{
				struct $7ASTNODE* vr$4 = ASTBUILDCALL( PROC$3, EXPR$1, (struct $7ASTNODE*)0ull, (struct $7ASTNODE*)0ull );
				EXPR$1 = vr$4;
				if( EXPR$1 != (struct $7ASTNODE*)0ull) goto label$160;
				{
					fb$result$1 = (struct $7ASTNODE*)0ull;
					goto label$150;
				}
				label$160:;
				label$159:;
				DTYPE$1 = *(int64*)((uint8*)EXPR$1 + 8ll);
				SUBTYPE$1 = *(struct $8FBSYMBOL**)((uint8*)EXPR$1 + 16ll);
			}
			goto label$157;
			label$158:;
			{
				ERRREPORT( 28ll, -1ll, (char*)0ull );
				goto label$152;
			}
			label$157:;
		}
		goto label$153;
		label$154:;
		{
			DTYPE$1 = (((DTYPE$1 & 31ll) | ((DTYPE$1 & 480ll) + -32ll)) | (((DTYPE$1 & 261632ll) >> (1ll & 63ll)) & 261632ll)) | (DTYPE$1 & 32505856ll);
			{
				int64 TMP$96$4;
				int64 TMP$97$4;
				if( (DTYPE$1 & 480ll) == 0ll) goto label$161;
				TMP$96$4 = 24ll;
				goto label$170;
				label$161:;
				TMP$96$4 = DTYPE$1 & 31ll;
				label$170:;
				TMP$97$4 = TMP$96$4;
				if( TMP$97$4 == 0ll) goto label$164;
				label$165:;
				if( TMP$97$4 != 23ll) goto label$163;
				label$164:;
				{
					ERRREPORT( 71ll, -1ll, (char*)0ull );
					DTYPE$1 = 2ll;
				}
				goto label$162;
				label$163:;
				if( TMP$97$4 != 22ll) goto label$166;
				label$167:;
				{
					ERRREPORT( 20ll, -1ll, (char*)0ull );
					DTYPE$1 = 2ll;
				}
				label$166:;
				label$162:;
			}
			if( *(int64*)((uint8*)&ENV$ + 360ll) == 0ll) goto label$169;
			{
				struct $7ASTNODE* vr$19 = ASTBUILDPTRCHK( EXPR$1 );
				EXPR$1 = vr$19;
			}
			label$169:;
			label$168:;
			struct $7ASTNODE* vr$20 = ASTNEWDEREF( EXPR$1, DTYPE$1, SUBTYPE$1, 0ll );
			EXPR$1 = vr$20;
		}
		label$153:;
		CNT$1 = CNT$1 + -1ll;
	}
	goto label$151;
	label$152:;
	fb$result$1 = EXPR$1;
	label$150:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDARRAYDESCINITREE( struct $8FBSYMBOL* DESC$1, struct $8FBSYMBOL* ARRAY$1, struct $7ASTNODE* ARRAY_EXPR$1 )
{
	int64 TMP$98$1;
	int64 TMP$99$1;
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$171:;
	struct $7ASTNODE* TREE$1;
	int64 DTYPE$1;
	int64 DIMENSIONS$1;
	int64 MAX_DIMENSIONS$1;
	int64 FLAGS$1;
	FLAGS$1 = 0ll;
	struct $8FBSYMBOL* ELM$1;
	struct $8FBSYMBOL* DIMTB$1;
	struct $8FBSYMBOL* SUBTYPE$1;
	if( ((int64)-((*(int64*)((uint8*)ARRAY$1 + 8ll) & 8ll) != 0ll) | (int64)-((*(int64*)((uint8*)ARRAY$1 + 8ll) & 16ll) != 0ll)) == 0ll) goto label$174;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$172;
	}
	label$174:;
	label$173:;
	struct $7ASTNODE* vr$13 = ASTTYPEINIBEGIN( *(int64*)((uint8*)DESC$1 + 56ll), *(struct $8FBSYMBOL**)((uint8*)DESC$1 + 64ll), (int64)-(*(int64*)DESC$1 != 12ll), *(int64*)((uint8*)DESC$1 + 88ll) );
	TREE$1 = vr$13;
	DTYPE$1 = *(int64*)((uint8*)ARRAY$1 + 56ll);
	SUBTYPE$1 = *(struct $8FBSYMBOL**)((uint8*)ARRAY$1 + 64ll);
	ELM$1 = *(struct $8FBSYMBOL**)((uint8*)*(struct $8FBSYMBOL**)((uint8*)DESC$1 + 64ll) + 104ll);
	if( (*(int64*)((uint8*)ARRAY$1 + 8ll) & 4ll) == 0ll) goto label$176;
	{
		struct $7ASTNODE* vr$29 = ASTNEWCONSTI( 0ll, (((DTYPE$1 & 31ll) | ((DTYPE$1 & 480ll) + 32ll)) | ((DTYPE$1 & 261632ll) << (1ll & 63ll))) | (DTYPE$1 & 32505856ll), SUBTYPE$1 );
		ARRAY_EXPR$1 = vr$29;
	}
	goto label$175;
	label$176:;
	{
		if( ARRAY_EXPR$1 == (struct $7ASTNODE*)0ull) goto label$178;
		{
		}
		goto label$177;
		label$178:;
		{
			struct $7ASTNODE* vr$30 = ASTNEWVAR( ARRAY$1, 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
			ARRAY_EXPR$1 = vr$30;
		}
		label$177:;
		struct $7ASTNODE* vr$31 = ASTNEWADDROF( ARRAY_EXPR$1 );
		ARRAY_EXPR$1 = vr$31;
		FLAGS$1 = FLAGS$1 | 32ll;
	}
	label$175:;
	ASTTYPEINISCOPEBEGIN( TREE$1, DESC$1, 0ll );
	if( (*(int64*)((uint8*)ARRAY$1 + 8ll) & 4ll) == 0ll) goto label$179;
	TMP$98$1 = 0ll;
	goto label$191;
	label$179:;
	TMP$98$1 = *(int64*)((uint8*)ARRAY$1 + 120ll);
	label$191:;
	struct $7ASTNODE* vr$36 = ASTNEWCONSTI( TMP$98$1, 8ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$37 = ASTCLONETREE( ARRAY_EXPR$1 );
	struct $7ASTNODE* vr$38 = ASTNEWBOP( 28ll, vr$37, vr$36, (struct $8FBSYMBOL*)0ull, 1ll );
	ASTTYPEINIADDASSIGN( TREE$1, vr$38, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	ELM$1 = *(struct $8FBSYMBOL**)((uint8*)ELM$1 + 312ll);
	ASTTYPEINIADDASSIGN( TREE$1, ARRAY_EXPR$1, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	ELM$1 = *(struct $8FBSYMBOL**)((uint8*)ELM$1 + 312ll);
	if( (*(int64*)((uint8*)ARRAY$1 + 8ll) & 4ll) == 0ll) goto label$180;
	TMP$99$1 = 0ll;
	goto label$192;
	label$180:;
	int64 vr$43 = SYMBGETREALSIZE( ARRAY$1 );
	TMP$99$1 = vr$43;
	label$192:;
	struct $7ASTNODE* vr$44 = ASTNEWCONSTI( TMP$99$1, 8ll, (struct $8FBSYMBOL*)0ull );
	ASTTYPEINIADDASSIGN( TREE$1, vr$44, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	ELM$1 = *(struct $8FBSYMBOL**)((uint8*)ELM$1 + 312ll);
	struct $7ASTNODE* vr$47 = ASTNEWCONSTI( *(int64*)((uint8*)ARRAY$1 + 80ll), 8ll, (struct $8FBSYMBOL*)0ull );
	ASTTYPEINIADDASSIGN( TREE$1, vr$47, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	ELM$1 = *(struct $8FBSYMBOL**)((uint8*)ELM$1 + 312ll);
	DIMENSIONS$1 = *(int64*)((uint8*)ARRAY$1 + 104ll);
	if( DIMENSIONS$1 != -1ll) goto label$182;
	{
		DIMENSIONS$1 = 0ll;
		MAX_DIMENSIONS$1 = 8ll;
	}
	goto label$181;
	label$182:;
	{
		MAX_DIMENSIONS$1 = DIMENSIONS$1;
		FLAGS$1 = FLAGS$1 | 16ll;
	}
	label$181:;
	struct $7ASTNODE* vr$51 = ASTNEWCONSTI( DIMENSIONS$1, 8ll, (struct $8FBSYMBOL*)0ull );
	ASTTYPEINIADDASSIGN( TREE$1, vr$51, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	ELM$1 = *(struct $8FBSYMBOL**)((uint8*)ELM$1 + 312ll);
	FLAGS$1 = FLAGS$1 | (MAX_DIMENSIONS$1 & 15ll);
	struct $7ASTNODE* vr$55 = ASTNEWCONSTI( FLAGS$1, 8ll, (struct $8FBSYMBOL*)0ull );
	ASTTYPEINIADDASSIGN( TREE$1, vr$55, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	ELM$1 = *(struct $8FBSYMBOL**)((uint8*)ELM$1 + 312ll);
	struct $8FBSYMBOL* DIMTBFIELD$1;
	DIMTBFIELD$1 = ELM$1;
	DIMTB$1 = *(struct $8FBSYMBOL**)((uint8*)*(struct $8FBSYMBOL**)((uint8*)ELM$1 + 64ll) + 104ll);
	ASTTYPEINISCOPEBEGIN( TREE$1, DIMTBFIELD$1, -1ll );
	if( (*(int64*)((uint8*)ARRAY$1 + 8ll) & 16388ll) != 0ll) goto label$184;
	{
		{
			int64 I$3;
			I$3 = 0ll;
			int64 TMP$100$3;
			TMP$100$3 = *(int64*)((uint8*)ARRAY$1 + 104ll) + -1ll;
			goto label$185;
			label$188:;
			{
				ELM$1 = DIMTB$1;
				ASTTYPEINISCOPEBEGIN( TREE$1, DIMTBFIELD$1, 0ll );
				struct $7ASTNODE* vr$71 = ASTNEWCONSTI( (*(int64*)((uint8*)((uint8*)*(struct $10FBARRAYDIM**)((uint8*)ARRAY$1 + 112ll) + (I$3 << (4ll & 63ll))) + 8ll) - *(int64*)((uint8*)*(struct $10FBARRAYDIM**)((uint8*)ARRAY$1 + 112ll) + (I$3 << (4ll & 63ll)))) + 1ll, 8ll, (struct $8FBSYMBOL*)0ull );
				ASTTYPEINIADDASSIGN( TREE$1, vr$71, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
				ELM$1 = *(struct $8FBSYMBOL**)((uint8*)ELM$1 + 312ll);
				struct $7ASTNODE* vr$76 = ASTNEWCONSTI( *(int64*)((uint8*)*(struct $10FBARRAYDIM**)((uint8*)ARRAY$1 + 112ll) + (I$3 << (4ll & 63ll))), 8ll, (struct $8FBSYMBOL*)0ull );
				ASTTYPEINIADDASSIGN( TREE$1, vr$76, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
				ELM$1 = *(struct $8FBSYMBOL**)((uint8*)ELM$1 + 312ll);
				struct $7ASTNODE* vr$81 = ASTNEWCONSTI( *(int64*)((uint8*)((uint8*)*(struct $10FBARRAYDIM**)((uint8*)ARRAY$1 + 112ll) + (I$3 << (4ll & 63ll))) + 8ll), 8ll, (struct $8FBSYMBOL*)0ull );
				ASTTYPEINIADDASSIGN( TREE$1, vr$81, ELM$1, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
				ASTTYPEINISCOPEEND( TREE$1, DIMTBFIELD$1 );
			}
			label$186:;
			I$3 = I$3 + 1ll;
			label$185:;
			if( I$3 <= TMP$100$3) goto label$188;
			label$187:;
		}
	}
	goto label$183;
	label$184:;
	{
		DIMENSIONS$1 = *(int64*)((uint8*)ARRAY$1 + 104ll);
		if( DIMENSIONS$1 != -1ll) goto label$190;
		{
			DIMENSIONS$1 = 8ll;
		}
		label$190:;
		label$189:;
		ASTTYPEINIADDPAD( TREE$1, DIMENSIONS$1 * *(int64*)((uint8*)*(struct $8FBSYMBOL**)((uint8*)&SYMB$ + 199216ll) + 80ll) );
	}
	label$183:;
	ASTTYPEINISCOPEEND( TREE$1, DIMTBFIELD$1 );
	ASTTYPEINISCOPEEND( TREE$1, DESC$1 );
	ASTTYPEINIEND( TREE$1, -1ll );
	fb$result$1 = TREE$1;
	label$172:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDARRAYBOUND( struct $7ASTNODE* ARRAYEXPR$1, struct $7ASTNODE* DIMEXPR$1, int64 TK$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$217:;
	struct $7ASTNODE* EXPR$1;
	ERRPUSHPARAMLOCATION( (struct $8FBSYMBOL*)0ull, TK$1, 2ll, (char*)"dimension" );
	struct $7ASTNODE* vr$1 = ASTNEWCONV( 8ll, (struct $8FBSYMBOL*)0ull, DIMEXPR$1, 0ll, (int64*)0ull );
	DIMEXPR$1 = vr$1;
	ERRPOPPARAMLOCATION(  );
	struct $7ASTNODE* vr$3 = HCONSTBOUND( ARRAYEXPR$1, DIMEXPR$1, (int64)-(TK$1 == 495ll) );
	EXPR$1 = vr$3;
	if( EXPR$1 != (struct $7ASTNODE*)0ull) goto label$220;
	{
		struct $7ASTNODE* vr$5 = RTLARRAYBOUND( ARRAYEXPR$1, DIMEXPR$1, (int64)-(TK$1 == 495ll) );
		EXPR$1 = vr$5;
	}
	label$220:;
	label$219:;
	fb$result$1 = EXPR$1;
	label$218:;
	return fb$result$1;
}

struct $7ASTNODE* ASTBUILDSTRPTR( struct $7ASTNODE* LHS$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$221:;
	struct $7ASTNODE* EXPR$1;
	int64 DTYPE$1;
	DTYPE$1 = 4ll;
	if( (*(int64*)((uint8*)LHS$1 + 8ll) & 512ll) == 0ll) goto label$224;
	{
		DTYPE$1 = DTYPE$1 | 512ll;
	}
	label$224:;
	label$223:;
	DTYPE$1 = ((((DTYPE$1 & 31ll) | ((DTYPE$1 & 480ll) + 32ll)) | ((DTYPE$1 & 261632ll) << (1ll & 63ll))) | (DTYPE$1 & 32505856ll)) | 512ll;
	DTYPE$1 = (((DTYPE$1 & 31ll) | ((DTYPE$1 & 480ll) + 32ll)) | ((DTYPE$1 & 261632ll) << (1ll & 63ll))) | (DTYPE$1 & 32505856ll);
	struct $7ASTNODE* vr$23 = ASTNEWADDROF( LHS$1 );
	struct $7ASTNODE* vr$24 = ASTNEWCONV( DTYPE$1, (struct $8FBSYMBOL*)0ull, vr$23, 16ll, (int64*)0ull );
	struct $7ASTNODE* vr$25 = ASTNEWDEREF( vr$24, 2147483648ll, (struct $8FBSYMBOL*)0ull, 0ll );
	EXPR$1 = vr$25;
	fb$result$1 = EXPR$1;
	goto label$222;
	label$222:;
	return fb$result$1;
}

static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM* THIS$1, struct $11TSTRSETITEM* __FB_RHS__$1 )
{
	label$4:;
	fb_StrAssign( (void*)THIS$1, -1ll, (void*)__FB_RHS__$1, -1ll, 0 );
	*(int64*)((uint8*)THIS$1 + 24ll) = *(int64*)((uint8*)__FB_RHS__$1 + 24ll);
	*(struct $8HASHITEM**)((uint8*)THIS$1 + 32ll) = *(struct $8HASHITEM**)((uint8*)__FB_RHS__$1 + 32ll);
	label$5:;
}

static struct $7ASTNODE* ASTFAKEINSTPTR( struct $8FBSYMBOL* SUBTYPE$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$115:;
	struct $7ASTNODE* vr$1 = ASTNEWCONSTI( 0ll, 52ll, SUBTYPE$1 );
	fb$result$1 = vr$1;
	label$116:;
	return fb$result$1;
}

static struct $7ASTNODE* HCONSTBOUND( struct $7ASTNODE* ARRAYEXPR$1, struct $7ASTNODE* DIMEXPR$1, int64 IS_LBOUND$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$193:;
	struct $8FBSYMBOL* ARRAY$1;
	int64 DIMENSION$1;
	int64 BOUND$1;
	fb$result$1 = (struct $7ASTNODE*)0ull;
	{
		$13AST_NODECLASS TMP$101$2;
		TMP$101$2 = *($13AST_NODECLASS*)ARRAYEXPR$1;
		if( TMP$101$2 == 17ll) goto label$197;
		label$198:;
		if( TMP$101$2 == 19ll) goto label$197;
		label$199:;
		if( TMP$101$2 != 25ll) goto label$196;
		label$197:;
		{
		}
		goto label$195;
		label$196:;
		{
			goto label$194;
		}
		label$200:;
		label$195:;
	}
	ARRAY$1 = *(struct $8FBSYMBOL**)((uint8*)ARRAYEXPR$1 + 24ll);
	if( ARRAY$1 != (struct $8FBSYMBOL*)0ull) goto label$202;
	{
		goto label$194;
	}
	label$202:;
	label$201:;
	if( (*(int64*)((uint8*)ARRAY$1 + 8ll) & 16388ll) == 0ll) goto label$204;
	{
		goto label$194;
	}
	label$204:;
	label$203:;
	if( *(int64*)DIMEXPR$1 == 16ll) goto label$206;
	{
		goto label$194;
	}
	label$206:;
	label$205:;
	DIMENSION$1 = *(int64*)((uint8*)DIMEXPR$1 + 40ll) + -1ll;
	if( ((int64)-(DIMENSION$1 >= 0ll) & (int64)-(DIMENSION$1 < *(int64*)((uint8*)ARRAY$1 + 104ll))) == 0ll) goto label$208;
	{
		if( IS_LBOUND$1 == 0ll) goto label$210;
		{
			BOUND$1 = *(int64*)((uint8*)*(struct $10FBARRAYDIM**)((uint8*)ARRAY$1 + 112ll) + (DIMENSION$1 << (4ll & 63ll)));
		}
		goto label$209;
		label$210:;
		{
			BOUND$1 = *(int64*)((uint8*)((uint8*)*(struct $10FBARRAYDIM**)((uint8*)ARRAY$1 + 112ll) + (DIMENSION$1 << (4ll & 63ll))) + 8ll);
			if( BOUND$1 != (int64)-9223372036854775808ull) goto label$212;
			{
				goto label$194;
			}
			label$212:;
			label$211:;
		}
		label$209:;
	}
	goto label$207;
	label$208:;
	{
		if( DIMENSION$1 != -1ll) goto label$214;
		{
			int64 TMP$102$3;
			if( IS_LBOUND$1 == 0ll) goto label$215;
			TMP$102$3 = 1ll;
			goto label$225;
			label$215:;
			TMP$102$3 = *(int64*)((uint8*)ARRAY$1 + 104ll);
			label$225:;
			BOUND$1 = TMP$102$3;
		}
		goto label$213;
		label$214:;
		{
			int64 TMP$103$3;
			if( IS_LBOUND$1 == 0ll) goto label$216;
			TMP$103$3 = 0ll;
			goto label$226;
			label$216:;
			TMP$103$3 = -1ll;
			label$226:;
			BOUND$1 = TMP$103$3;
		}
		label$213:;
	}
	label$207:;
	struct $7ASTNODE* vr$19 = ASTNEWCONSTI( BOUND$1, 8ll, (struct $8FBSYMBOL*)0ull );
	fb$result$1 = vr$19;
	label$194:;
	return fb$result$1;
}
