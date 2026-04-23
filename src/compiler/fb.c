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
typedef int64 $11FB_LANG_OPT;
typedef int64 $7FB_LANG;
struct $12FB_LANG_INFO {
	char* NAME;
	$11FB_LANG_OPT OPTIONS;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $12FB_LANG_INFO ) == 16 );
typedef int64 $10FB_BACKEND;
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
struct $7TSTRSET {
	struct $5TLIST LIST;
	struct $5THASH HASH;
};
__FB_STATIC_ASSERT( sizeof( struct $7TSTRSET ) == 88 );
typedef int64 $16FB_RESTART_FLAGS;
struct $16__FB_ARRAYDIMTB$ {
	int64 ELEMENTS;
	int64 LBOUND;
	int64 UBOUND;
};
__FB_STATIC_ASSERT( sizeof( struct $16__FB_ARRAYDIMTB$ ) == 24 );
struct $7FBARRAYIvE {
	void* DATA;
	void* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[8];
};
__FB_STATIC_ASSERT( sizeof( struct $7FBARRAYIvE ) == 240 );
typedef int64 $12FB_SYMBCLASS;
typedef int64 $13FB_SYMBATTRIB;
typedef int64 $13FB_PROCATTRIB;
typedef int64 $12FB_SYMBSTATS;
struct $9FB_SYMBID {
	char* NAME;
	char* ALIAS;
	char* MANGLED;
};
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
typedef int64 $13FB_COMPTARGET;
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
typedef int64 $12FB_WCHARCONV;
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
typedef int64 $10FB_OUTTYPE;
typedef int64 $10FB_CPUTYPE;
typedef int64 $10FB_FPUTYPE;
typedef int64 $9FB_FPMODE;
typedef int64 $17FB_VECTORIZELEVEL;
typedef int64 $12FB_ASMSYNTAX;
typedef int64 $10FB_PDCHECK;
typedef int64 $11FB_MODEVIEW;
typedef int64 $12FB_TARGETOPT;
struct $8FBTARGET {
	char* ID;
	$11FB_DATATYPE WCHAR;
	$11FB_FUNCMODE FBCALL;
	$11FB_FUNCMODE STDCALL;
	$12FB_TARGETOPT OPTIONS;
};
__FB_STATIC_ASSERT( sizeof( struct $8FBTARGET ) == 40 );
struct $15FBCPUFAMILYINFO {
	char* ID;
	int64 DEFAULTCPUTYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $15FBCPUFAMILYINFO ) == 16 );
struct $13FBCPUTYPEINFO {
	char* GCCARCH;
	char* FBCARCH;
	int64 FAMILY;
	int64 BITS;
	int64 BIGENDIAN;
};
__FB_STATIC_ASSERT( sizeof( struct $13FBCPUTYPEINFO ) == 40 );
typedef int64 $13FBFILE_FORMAT;
typedef int64 (*tmp$40)( void );
typedef void (*tmp$39)( void );
struct $6FBFILE {
	int64 NUM;
	char NAME[261];
	char* INCFILE;
	int64 ISMAIN;
	$13FBFILE_FORMAT FORMAT;
};
__FB_STATIC_ASSERT( sizeof( struct $6FBFILE ) == 296 );
typedef int64 $19FB_CVA_LIST_TYPEDEF;
struct $11TSTRSETITEM {
	FBSTRING S;
	int64 USERDATA;
	struct $8HASHITEM* HASHITEM;
};
__FB_STATIC_ASSERT( sizeof( struct $11TSTRSETITEM ) == 40 );
typedef void (*tmp$69)( char* );
struct $8FBARRAY1IlE {
	int64* DATA;
	int64* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1IlE ) == 72 );
int32 fb_ArrayRedimEx( struct $7FBARRAYIvE*, uint64, int32, int32, uint64, ... );
int32 fb_ArrayErase( struct $7FBARRAYIvE* );
void* fb_ErrorThrowAt( int32, char*, void*, void* );
int32 fb_FileOpen( FBSTRING*, uint32, uint32, uint32, int32, int32 );
int32 fb_FileClose( int32 );
int32 fb_FileFree( void );
void free( void* );
void fb_PrintString( int32, FBSTRING*, int32 );
FBSTRING* fb_StrInit( void*, int64, void*, int64, int32 );
FBSTRING* fb_StrAssign( void*, int64, void*, int64, int32 );
void fb_StrDelete( FBSTRING* );
FBSTRING* fb_StrConcat( FBSTRING*, void*, int64, void*, int64 );
int32 fb_StrCompare( void*, int64, void*, int64 );
FBSTRING* fb_StrConcatAssign( void*, int64, void*, int64, int32 );
FBSTRING* fb_StrAllocTempResult( FBSTRING* );
FBSTRING* fb_StrAllocTempDescZ( char* );
FBSTRING* fb_StrAllocTempDescZEx( char*, int64 );
int64 fb_StrLen( void*, int64 );
int64 fb_StrInstr( int64, FBSTRING*, FBSTRING* );
FBSTRING* fb_LEFT( FBSTRING*, int64 );
FBSTRING* fb_RIGHT( FBSTRING*, int64 );
FBSTRING* fb_StrLcase2( FBSTRING*, int32 );
static void fb_ctor__fb( void ) __attribute__(( constructor ));
void* XALLOCATE( int32 );
void* LISTGETHEAD( struct $5TLIST* );
void* LISTGETNEXT( void* );
void STRLISTAPPEND( struct $5TLIST*, FBSTRING* );
void STRLISTINIT( struct $5TLIST*, int64 );
void HASHINIT( struct $5THASH*, int64, int64 );
void HASHEND( struct $5THASH* );
uint64 HASHHASH( char* );
void* HASHLOOKUP( struct $5THASH*, char* );
void* HASHLOOKUPEX( struct $5THASH*, char*, uint64 );
struct $8HASHITEM* HASHADD( struct $5THASH*, char*, void*, uint64 );
static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM*, struct $11TSTRSETITEM* );
void STRSETADD( struct $7TSTRSET*, FBSTRING*, int64 );
void STRSETCOPY( struct $7TSTRSET*, struct $7TSTRSET* );
void STRSETINIT( struct $7TSTRSET*, int64 );
void STRSETEND( struct $7TSTRSET* );
void ERRINIT( void );
void ERREND( void );
void ERRHIDEFURTHERERRORS( void );
int64 ERRGETCOUNT( void );
typedef int64 $12FB_ERRMSGOPT;
void ERRREPORTEX( int64, char*, int64, $12FB_ERRMSGOPT, char* );
void ERRREPORT( int64, int64, char* );
void ERRREPORTWARN( int64, char*, $12FB_ERRMSGOPT, char* );
int64 FBSHOULDCONTINUE( void );
void FBRESTARTBEGINREQUEST( $16FB_RESTART_FLAGS );
void FBRESTARTACCEPTREQUEST( $16FB_RESTART_FLAGS );
void FBSETOPTION( int64, int64 );
int64 FBGETOPTION( int64 );
void FBINCLUDEFILE( char*, int64 );
int64 FBIDENTIFYCPUFAMILY( FBSTRING* );
int64 FBIS64BIT( void );
int64 FBGETBITS( void );
int64 FBGETCPUFAMILY( void );
int64 FBISCROSSCOMP( void );
void FBMAINBEGIN( void );
void FBMAINEND( void );
FBSTRING* FBGETLANGNAME( $7FB_LANG );
int64 STACKNEW( struct $6TSTACK*, int64, int64, int64 );
int64 STACKFREE( struct $6TSTACK* );
void IRINIT( void );
void IREND( void );
void ASTINIT( void );
void ASTEND( void );
void SYMBINIT( int64 );
void SYMBEND( void );
struct $8FBSYMBOL* SYMBADDDEFINE( char*, char*, int64, int64, tmp$28, $15FB_DEFINE_FLAGS );
int64 SYMBCHECKLABELS( struct $8FBSYMBOL* );
int64 HFILEEXISTS( char* );
void HCLEARNAME( char* );
FBSTRING* HSTRIPEXT( FBSTRING* );
FBSTRING* HSTRIPPATH( char* );
FBSTRING* HSTRIPFILENAME( char* );
void HREPLACESLASH( char*, int64 );
FBSTRING* PATHSTRIPDIV( FBSTRING* );
int64 PATHISABSOLUTE( char* );
int64 HCHECKFILEFORMAT( int64 );
FBSTRING* HCURDIR( void );
FBSTRING* PATHSTRIPCURDIR( FBSTRING* );
char* HESCAPE( char*, int64 );
typedef int64 $17LEX_TKCTX_CONTEXT;
void LEXINIT( $17LEX_TKCTX_CONTEXT );
void LEXEND( void );
void LEXPUSHCTX( void );
void LEXPOPCTX( void );
void CPROGRAM( void );
void RTLINIT( void );
void RTLEND( void );
char* OBJINFOENCODE( int64 );
void PARSERINIT( void );
void PARSEREND( void );
void PARSERSETCTX( void );
static char* HFINDINCFILE( struct $5THASH*, char* );
static char* HADDINCFILE( struct $5THASH*, struct $5THASH*, char* );
static void HUPDATELANGOPTIONS( void );
static void HUPDATETARGETOPTIONS( void );
static FBSTRING* HGETTARGETID( int64, int64 );
static void FBPARSEPREDEFINES( void );
static void FBPARSEPREINCLUDES( void );
static void HAPPENDFBCTINF( char* );
static void HEMITOBJINFO( void );
static void HSHOWINCLUDE( int64, FBSTRING* );
static void HONSKIPPEDFILE( FBSTRING* );
static int64 IS_ROOTPATH( char** );
static int64 GET_ROOTPATH_LEN( char* );
static int64 SOLVE_PATH( char* );
typedef int64 $14IR_OPTIONVALUE;
typedef int64 (*tmp$41)( $14IR_OPTIONVALUE );
typedef int64 (*tmp$42)( int64, int64 );
typedef void (*tmp$43)( struct $8FBSYMBOL* );
typedef void (*tmp$44)( struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef char* (*tmp$45)( void );
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
typedef void (*tmp$46)( struct $6IRVREG*, struct $6IRVREG* );
typedef void (*tmp$47)( int64 );
typedef void (*tmp$48)( struct $8FBSYMBOL*, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef void (*tmp$49)( struct $8FBSYMBOL*, struct $6IRVREG*, int64, int64, struct $6IRVREG* );
typedef void (*tmp$50)( struct $9ASTASMTOK* );
typedef void (*tmp$51)( char* );
typedef int64 $10IR_EMITOPT;
typedef void (*tmp$52)( int64, struct $6IRVREG*, struct $6IRVREG*, struct $6IRVREG*, struct $8FBSYMBOL*, $10IR_EMITOPT );
typedef void (*tmp$53)( int64, struct $6IRVREG*, struct $6IRVREG* );
typedef void (*tmp$54)( struct $6IRVREG* );
typedef void (*tmp$55)( struct $8FBSYMBOL*, int64, struct $6IRVREG*, int64 );
typedef void (*tmp$56)( struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, int64, int64 );
typedef void (*tmp$57)( int64, struct $8FBSYMBOL* );
typedef void (*tmp$58)( struct $6IRVREG*, struct $8FBSYMBOL*, uint64*, struct $8FBSYMBOL**, int64, struct $8FBSYMBOL*, uint64, uint64 );
typedef void (*tmp$59)( int64, struct $6IRVREG*, struct $6IRVREG*, int64, int64 );
typedef void (*tmp$60)( int64, struct $6IRVREG*, struct $6IRVREG*, struct $6IRVREG* );
typedef void (*tmp$61)( int64, struct $8FBSYMBOL*, int64, char* );
typedef void (*tmp$62)( struct $8FBSYMBOL*, int64 );
typedef void (*tmp$63)( struct $8FBSYMBOL*, double );
typedef void (*tmp$64)( struct $8FBSYMBOL*, struct $8FBSYMBOL*, int64 );
typedef void (*tmp$65)( int64, char*, int64, int64 );
typedef void (*tmp$66)( int64, uint32*, int64 );
typedef void (*tmp$67)( int64, int64 );
typedef void (*tmp$68)( struct $8FBSYMBOL*, int64 );
typedef struct $6IRVREG* (*tmp$70)( int64, struct $8FBSYMBOL* );
typedef struct $6IRVREG* (*tmp$71)( int64, struct $8FBSYMBOL*, int64 );
typedef struct $6IRVREG* (*tmp$72)( int64, struct $8FBSYMBOL*, double );
typedef struct $6IRVREG* (*tmp$73)( int64, struct $8FBSYMBOL*, struct $8FBSYMBOL*, int64 );
typedef struct $6IRVREG* (*tmp$74)( int64, struct $8FBSYMBOL*, struct $8FBSYMBOL*, int64, int64, struct $6IRVREG* );
typedef struct $6IRVREG* (*tmp$75)( int64, struct $8FBSYMBOL*, int64, struct $6IRVREG* );
typedef void (*tmp$76)( struct $6IRVREG*, int64, struct $8FBSYMBOL* );
typedef uint64 (*tmp$77)( struct $6IRVREG* );
struct $7IR_VTBL {
	tmp$39 INIT;
	tmp$39 END;
	tmp$40 EMITBEGIN;
	tmp$39 EMITEND;
	tmp$41 GETOPTIONVALUE;
	tmp$42 SUPPORTSOP;
	tmp$43 PROCBEGIN;
	tmp$43 PROCEND;
	tmp$44 PROCALLOCARG;
	tmp$44 PROCALLOCLOCAL;
	tmp$45 PROCGETFRAMEREGNAME;
	tmp$43 SCOPEBEGIN;
	tmp$43 SCOPEEND;
	tmp$43 PROCALLOCSTATICVARS;
	tmp$46 EMITCONVERT;
	tmp$43 EMITLABEL;
	tmp$43 EMITLABELNF;
	tmp$47 EMITRETURN;
	tmp$44 EMITPROCBEGIN;
	tmp$48 EMITPROCEND;
	tmp$49 EMITPUSHARG;
	tmp$50 EMITASMLINE;
	tmp$51 EMITCOMMENT;
	tmp$52 EMITBOP;
	tmp$53 EMITUOP;
	tmp$46 EMITSTORE;
	tmp$39 EMITSPILLREGS;
	tmp$54 EMITLOAD;
	tmp$46 EMITLOADRES;
	tmp$53 EMITSTACK;
	tmp$53 EMITADDR;
	tmp$55 EMITCALL;
	tmp$56 EMITCALLPTR;
	tmp$47 EMITSTACKALIGN;
	tmp$54 EMITJUMPPTR;
	tmp$57 EMITBRANCH;
	tmp$58 EMITJMPTB;
	tmp$59 EMITMEM;
	tmp$60 EMITMACRO;
	tmp$43 EMITSCOPEBEGIN;
	tmp$43 EMITSCOPEEND;
	tmp$43 EMITDECL;
	tmp$61 EMITDBG;
	tmp$43 EMITVARINIBEGIN;
	tmp$43 EMITVARINIEND;
	tmp$62 EMITVARINII;
	tmp$63 EMITVARINIF;
	tmp$64 EMITVARINIOFS;
	tmp$65 EMITVARINISTR;
	tmp$66 EMITVARINIWSTR;
	tmp$67 EMITVARINIPAD;
	tmp$68 EMITVARINISCOPEBEGIN;
	tmp$39 EMITVARINISCOPEEND;
	tmp$39 EMITFBCTINFBEGIN;
	tmp$69 EMITFBCTINFSTRING;
	tmp$39 EMITFBCTINFEND;
	tmp$70 ALLOCVREG;
	tmp$71 ALLOCVRIMM;
	tmp$72 ALLOCVRIMMF;
	tmp$73 ALLOCVRVAR;
	tmp$74 ALLOCVRIDX;
	tmp$75 ALLOCVRPTR;
	tmp$73 ALLOCVROFS;
	tmp$76 SETVREGDATATYPE;
	tmp$77 GETDISTANCE;
	tmp$53 LOADVR;
	tmp$46 STOREVR;
	tmp$47 XCHGTOS;
};
__FB_STATIC_ASSERT( sizeof( struct $7IR_VTBL ) == 544 );
typedef int64 $6IR_OPT;
struct $5IRCTX {
	struct $7IR_VTBL VTBL;
	$6IR_OPT OPTIONS;
};
__FB_STATIC_ASSERT( sizeof( struct $5IRCTX ) == 552 );
extern struct $5IRCTX IR$;
typedef int64 $11AST_OPFLAGS;
struct $10AST_OPINFO {
	$13AST_NODECLASS CLASS;
	$11AST_OPFLAGS FLAGS;
	char* ID;
	$6AST_OP SELFOP;
};
__FB_STATIC_ASSERT( sizeof( struct $10AST_OPINFO ) == 32 );
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
struct $6FBMAIN {
	struct $8FBSYMBOL* PROC;
	struct $7ASTNODE* INITNODE;
};
__FB_STATIC_ASSERT( sizeof( struct $6FBMAIN ) == 16 );
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
typedef int64 $8FB_TOKEN;
typedef int64 $15FB_CMPSTMT_MASK;
struct $17FB_CMPSTMT_FORELM {
	struct $8FBSYMBOL* SYM;
	union $7FBVALUE VALUE;
	int64 DTYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $17FB_CMPSTMT_FORELM ) == 24 );
struct $13FB_CMPSTMTSTK;
struct $14FB_CMPSTMT_FOR {
	struct $7ASTNODE* OUTERSCOPENODE;
	struct $17FB_CMPSTMT_FORELM CNT;
	struct $17FB_CMPSTMT_FORELM END;
	struct $17FB_CMPSTMT_FORELM STP;
	struct $17FB_CMPSTMT_FORELM ISPOS;
	struct $8FBSYMBOL* TESTLABEL;
	struct $8FBSYMBOL* INILABEL;
	struct $8FBSYMBOL* CMPLABEL;
	struct $8FBSYMBOL* ENDLABEL;
	struct $13FB_CMPSTMTSTK* LAST;
	int64 EXPLICIT_STEP;
};
__FB_STATIC_ASSERT( sizeof( struct $14FB_CMPSTMT_FOR ) == 152 );
struct $13FB_CMPSTMT_DO {
	int64 ATTOP;
	struct $8FBSYMBOL* INILABEL;
	struct $8FBSYMBOL* CMPLABEL;
	struct $8FBSYMBOL* ENDLABEL;
	struct $13FB_CMPSTMTSTK* LAST;
};
__FB_STATIC_ASSERT( sizeof( struct $13FB_CMPSTMT_DO ) == 40 );
struct $16FB_CMPSTMT_WHILE {
	struct $8FBSYMBOL* CMPLABEL;
	struct $8FBSYMBOL* ENDLABEL;
	struct $13FB_CMPSTMTSTK* LAST;
};
__FB_STATIC_ASSERT( sizeof( struct $16FB_CMPSTMT_WHILE ) == 24 );
struct $13FB_CMPSTMT_IF {
	int64 ISSINGLE;
	struct $8FBSYMBOL* NXTLABEL;
	struct $8FBSYMBOL* ENDLABEL;
	int64 ELSECNT;
};
__FB_STATIC_ASSERT( sizeof( struct $13FB_CMPSTMT_IF ) == 32 );
struct $15FB_CMPSTMT_PROC {
	$8FB_TOKEN TKN;
	int64 IS_NESTED;
	struct $8FBSYMBOL* ENDLABEL;
	struct $13FB_CMPSTMTSTK* LAST;
};
__FB_STATIC_ASSERT( sizeof( struct $15FB_CMPSTMT_PROC ) == 32 );
struct $19FB_CMPSTMT_SELCONST {
	int64 BASE;
	struct $8FBSYMBOL* DEFLABEL;
	int64 DTYPE;
	uint64 BIAS;
};
__FB_STATIC_ASSERT( sizeof( struct $19FB_CMPSTMT_SELCONST ) == 32 );
struct $17FB_CMPSTMT_SELECT {
	int64 ISCONST;
	struct $8FBSYMBOL* SYM;
	int64 CASECNT;
	struct $19FB_CMPSTMT_SELCONST CONST_;
	struct $8FBSYMBOL* CMPLABEL;
	struct $8FBSYMBOL* ENDLABEL;
	struct $13FB_CMPSTMTSTK* LAST;
	struct $7ASTNODE* OUTERSCOPENODE;
};
__FB_STATIC_ASSERT( sizeof( struct $17FB_CMPSTMT_SELECT ) == 88 );
struct $15FB_CMPSTMT_WITH {
	struct $8FBSYMBOL* SYM;
	int64 IS_PTR;
	struct $13FB_CMPSTMTSTK* LAST;
};
__FB_STATIC_ASSERT( sizeof( struct $15FB_CMPSTMT_WITH ) == 24 );
struct $20FB_CMPSTMT_NAMESPACE {
	struct $8FBSYMBOL* SYM;
	int64 LEVELS;
};
__FB_STATIC_ASSERT( sizeof( struct $20FB_CMPSTMT_NAMESPACE ) == 16 );
typedef int64 $11FB_MANGLING;
struct $17FB_CMPSTMT_EXTERN {
	$11FB_MANGLING LASTMANG;
};
__FB_STATIC_ASSERT( sizeof( struct $17FB_CMPSTMT_EXTERN ) == 8 );
struct $16FB_CMPSTMT_SCOPE {
	int64 LASTIS_SCOPE;
};
__FB_STATIC_ASSERT( sizeof( struct $16FB_CMPSTMT_SCOPE ) == 8 );
struct $13FB_CMPSTMTSTK {
	int64 ID;
	$15FB_CMPSTMT_MASK ALLOWMASK;
	struct $7ASTNODE* SCOPENODE;
	union {
		struct $14FB_CMPSTMT_FOR FOR;
		struct $13FB_CMPSTMT_DO DO;
		struct $16FB_CMPSTMT_WHILE WHILE;
		struct $13FB_CMPSTMT_IF IF;
		struct $15FB_CMPSTMT_PROC PROC;
		struct $17FB_CMPSTMT_SELECT SELECT;
		struct $15FB_CMPSTMT_WITH WITH;
		struct $20FB_CMPSTMT_NAMESPACE NSPC;
		struct $17FB_CMPSTMT_EXTERN EXT;
		struct $16FB_CMPSTMT_SCOPE SCP;
	};
};
__FB_STATIC_ASSERT( sizeof( struct $13FB_CMPSTMTSTK ) == 176 );
struct $17FBPARSER_STMT_LET {
	struct $5TLIST LIST;
};
__FB_STATIC_ASSERT( sizeof( struct $17FBPARSER_STMT_LET ) == 64 );
struct $13FBPARSER_STMT {
	struct $6TSTACK STK;
	$8FB_TOKEN ID;
	int64 CNT;
	struct $13FB_CMPSTMTSTK* FOR;
	struct $13FB_CMPSTMTSTK* DO;
	struct $13FB_CMPSTMTSTK* WHILE;
	struct $13FB_CMPSTMTSTK* SELECT;
	struct $13FB_CMPSTMTSTK* PROC;
	struct $13FB_CMPSTMTSTK* WITH;
	struct $17FBPARSER_STMT_LET LET;
};
__FB_STATIC_ASSERT( sizeof( struct $13FBPARSER_STMT ) == 176 );
typedef int64 $12FB_PARSEROPT;
struct $9PARSERCTX {
	struct $13FBPARSER_STMT STMT;
	int64 NSPCREC;
	struct $10FBSYMCHAIN* NSPREFIX;
	uint64 STAGE;
	uint64 SCOPE;
	$11FB_MANGLING MANGLING;
	struct $8FBSYMBOL* CURRPROC;
	struct $8FBSYMBOL* CURRBLOCK;
	struct $5TLIST OVLARGLIST;
	int64 PRNTCNT;
	$12FB_PARSEROPT OPTIONS;
	int64 CTX_DTYPE;
	struct $8FBSYMBOL* CTXSYM;
	int64 HAVE_EQ_OUTSIDE_PARENS;
};
__FB_STATIC_ASSERT( sizeof( struct $9PARSERCTX ) == 336 );
extern struct $9PARSERCTX PARSER$;
struct $7FBARRAYI6FBFILEE {
	struct $6FBFILE* DATA;
	struct $6FBFILE* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[8];
};
__FB_STATIC_ASSERT( sizeof( struct $7FBARRAYI6FBFILEE ) == 240 );
static struct $7FBARRAYI6FBFILEE INFILETB$ = { (struct $6FBFILE*)0ull, (struct $6FBFILE*)0ull, 0ll, 296ll, 0ll, 8ll, {  } };
static struct $12FB_LANG_INFO LANGTB$[4] = { { (char*)"fb", 536873471ll }, { (char*)"deprecated", 738066591ll }, { (char*)"fblite", 738132125ll }, { (char*)"qb", 738131968ll } };
static struct $8FBTARGET TARGETINFO$[14] = { { (char*)"win32", 6ll, 1ll, 1ll, 276ll }, { (char*)"cygwin", 6ll, 1ll, 1ll, 277ll }, { (char*)"linux", 12ll, 3ll, 2ll, 201ll }, { (char*)"android", 12ll, 3ll, 2ll, 201ll }, { (char*)"haiku", 12ll, 3ll, 2ll, 201ll }, { (char*)"dos", 3ll, 3ll, 2ll, 264ll }, { (char*)"xbox", 12ll, 1ll, 1ll, 272ll }, { (char*)"freebsd", 12ll, 3ll, 2ll, 185ll }, { (char*)"dragonfly", 12ll, 3ll, 2ll, 153ll }, { (char*)"solaris", 12ll, 3ll, 2ll, 153ll }, { (char*)"openbsd", 12ll, 3ll, 2ll, 185ll }, { (char*)"darwin", 12ll, 3ll, 2ll, 601ll }, { (char*)"netbsd", 12ll, 3ll, 2ll, 185ll }, { (char*)"js", 12ll, 3ll, 2ll, 25ll } };
static struct $15FBCPUFAMILYINFO CPUFAMILYINFO$[11] = { { (char*)"x86", 3ll }, { (char*)"x86_64", 13ll }, { (char*)"arm", 17ll }, { (char*)"aarch64", 19ll }, { (char*)"powerpc", 20ll }, { (char*)"powerpc64", 21ll }, { (char*)"powerpc64le", 22ll }, { (char*)"riscv64", 23ll }, { (char*)"s390x", 24ll }, { (char*)"loongarch64", 25ll }, { (char*)"asmjs", 26ll } };
static struct $13FBCPUTYPEINFO CPUTYPEINFO$[27] = { { (char*)"i386", (char*)"386", 0ll, 32ll, 0ll }, { (char*)"i486", (char*)"486", 0ll, 32ll, 0ll }, { (char*)"i586", (char*)"586", 0ll, 32ll, 0ll }, { (char*)"i686", (char*)"686", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"athlon", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"athlon-xp", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"athlon-fx", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"k8-sse3", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"pentium-mmx", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"pentium2", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"pentium3", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"pentium4", 0ll, 32ll, 0ll }, { (char*)"prescott", (char*)"pentium4-sse3", 0ll, 32ll, 0ll }, { (char*)0ull, (char*)"x86-64", 1ll, 64ll, 0ll }, { (char*)0ull, (char*)"armv5te", 2ll, 32ll, 0ll }, { (char*)0ull, (char*)"armv6", 2ll, 32ll, 0ll }, { (char*)0ull, (char*)"armv6+fp", 2ll, 32ll, 0ll }, { (char*)0ull, (char*)"armv7-a", 2ll, 32ll, 0ll }, { (char*)0ull, (char*)"armv7-a+fp", 2ll, 32ll, 0ll }, { (char*)"armv8-a", (char*)"aarch64", 3ll, 64ll, 0ll }, { (char*)0ull, (char*)"powerpc", 4ll, 32ll, -1ll }, { (char*)0ull, (char*)"powerpc64", 5ll, 64ll, -1ll }, { (char*)0ull, (char*)"powerpc64le", 6ll, 64ll, 0ll }, { (char*)0ull, (char*)"riscv64", 7ll, 64ll, 0ll }, { (char*)0ull, (char*)"s390x", 8ll, 64ll, -1ll }, { (char*)0ull, (char*)"loongarch64", 9ll, 64ll, 0ll }, { (char*)0ull, (char*)"asmjs", 10ll, 32ll, 0ll } };
struct $5FBENV ENV$;

$11FB_LANG_OPT FBGETLANGOPTIONS( $7FB_LANG LANG$1 )
{
	$11FB_LANG_OPT fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$20:;
	fb$result$1 = *($11FB_LANG_OPT*)(((int64)(struct $12FB_LANG_INFO*)LANGTB$ + (LANG$1 << (4ll & 63ll))) + 8ll);
	label$21:;
	return fb$result$1;
}

FBSTRING* FBGETLANGNAME( $7FB_LANG LANG$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$22:;
	fb_StrAssign( (void*)&fb$result$1, -1ll, *(void**)((int64)(struct $12FB_LANG_INFO*)LANGTB$ + (LANG$1 << (4ll & 63ll))), 0ll, 0 );
	label$23:;
	FBSTRING* vr$4 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$4;
}

FBSTRING* FBGETBACKENDNAME( $10FB_BACKEND LANG$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$24:;
	{
		$10FB_BACKEND TMP$155$2;
		TMP$155$2 = *($10FB_BACKEND*)((uint8*)&ENV$ + 208ll);
		if( TMP$155$2 != 0ll) goto label$27;
		label$28:;
		{
			fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)"gas", 4ll, 0 );
		}
		goto label$26;
		label$27:;
		if( TMP$155$2 != 1ll) goto label$29;
		label$30:;
		{
			fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)"gcc", 4ll, 0 );
		}
		goto label$26;
		label$29:;
		if( TMP$155$2 != 2ll) goto label$31;
		label$32:;
		{
			fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)"clang", 6ll, 0 );
		}
		goto label$26;
		label$31:;
		if( TMP$155$2 != 3ll) goto label$33;
		label$34:;
		{
			fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)"llvm", 5ll, 0 );
		}
		goto label$26;
		label$33:;
		if( TMP$155$2 != 4ll) goto label$35;
		label$36:;
		{
			fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)"gas64", 6ll, 0 );
		}
		label$35:;
		label$26:;
	}
	label$25:;
	FBSTRING* vr$7 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$7;
}

void FBINIT( int64 ISMAIN$1, char* ENTRY$1, int64 MODULE_COUNT$1 )
{
	int64 TMP$163$1;
	label$37:;
	STRSETINIT( (struct $7TSTRSET*)((uint8*)&ENV$ + 1648ll), 5ll );
	STRSETINIT( (struct $7TSTRSET*)((uint8*)&ENV$ + 1736ll), 5ll );
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1608ll) = 0ll;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1616ll) = 0ll;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1624ll) = *(int64*)((uint8*)&ENV$ + 1624ll) & -6ll;
	int32 vr$3 = fb_ArrayRedimEx( (struct $7FBARRAYIvE*)&INFILETB$, 296ull, -1, 0, 1ull, 0ll, 15ll );
	if( (int64)vr$3 == 0ll) goto label$39;
	void* vr$5 = fb_ErrorThrowAt( 433, (char*)"/boot/home/Downloads/fbc-fbxl/src/compiler/fb.bas", (void*)0ull, (void*)0ull );
	goto *vr$5;
	label$39:;
	*(int64*)((uint8*)&ENV$ + 1304ll) = 0ll;
	*(struct $8FBSYMBOL**)((uint8*)&ENV$ + 1440ll) = (struct $8FBSYMBOL*)0ull;
	fb_StrAssign( (void*)((uint8*)&ENV$ + 1312ll), 128ll, (void*)ENTRY$1, 0ll, 0 );
	*(int64*)((uint8*)&ENV$ + 1600ll) = MODULE_COUNT$1;
	*(int64*)((uint8*)&ENV$ + 1544ll) = (int64)-(*(int64*)((uint8*)&ENV$ + 272ll) == 0ll);
	if( *(int64*)((uint8*)&ENV$ + 272ll) == 3ll) goto label$41;
	{
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1464ll) = 8ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1472ll) = 8ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1480ll) = 9ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1488ll) = 8ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1496ll) = 9ll;
		int64 vr$8 = FBIS64BIT(  );
		if( vr$8 == 0ll) goto label$43;
		{
			*($11FB_DATATYPE*)((uint8*)&ENV$ + 1504ll) = 8ll;
			*($11FB_DATATYPE*)((uint8*)&ENV$ + 1512ll) = 9ll;
		}
		goto label$42;
		label$43:;
		{
			*($11FB_DATATYPE*)((uint8*)&ENV$ + 1504ll) = 13ll;
			*($11FB_DATATYPE*)((uint8*)&ENV$ + 1512ll) = 14ll;
		}
		label$42:;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1520ll) = 16ll;
	}
	goto label$40;
	label$41:;
	{
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1464ll) = 5ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1472ll) = 5ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1480ll) = 6ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1488ll) = 11ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1496ll) = 12ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1504ll) = 13ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1512ll) = 14ll;
		*($11FB_DATATYPE*)((uint8*)&ENV$ + 1520ll) = 15ll;
	}
	label$40:;
	*(int64*)((uint8*)&ENV$ + 1536ll) = 2ll;
	*(int64*)((uint8*)&ENV$ + 1552ll) = -1ll;
	*(int64*)((uint8*)&ENV$ + 1560ll) = -1ll;
	*(int64*)((uint8*)&ENV$ + 1568ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 1576ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 1528ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 1584ll) = (int64)-(*(int64*)((uint8*)&ENV$ + 272ll) == 3ll);
	*(int64*)((uint8*)&ENV$ + 1824ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 616ll) = 0ll;
	{
		$13FB_COMPTARGET TMP$162$2;
		TMP$162$2 = *($13FB_COMPTARGET*)((uint8*)&ENV$ + 216ll);
		if( TMP$162$2 == 5ll) goto label$46;
		label$47:;
		if( TMP$162$2 == 6ll) goto label$46;
		label$48:;
		if( TMP$162$2 != 11ll) goto label$45;
		label$46:;
		{
			*(int64*)((uint8*)&ENV$ + 616ll) = -1ll;
		}
		goto label$44;
		label$45:;
		if( TMP$162$2 == 1ll) goto label$50;
		label$51:;
		if( TMP$162$2 != 0ll) goto label$49;
		label$50:;
		{
			int64 vr$10 = FBIS64BIT(  );
			*(int64*)((uint8*)&ENV$ + 616ll) = ~vr$10;
		}
		label$49:;
		label$44:;
	}
	*(int64*)((uint8*)&ENV$ + 624ll) = 0ll;
	PARSERSETCTX(  );
	SYMBINIT( ISMAIN$1 );
	ERRINIT(  );
	ASTINIT(  );
	IRINIT(  );
	if( (*(int64*)((uint8*)&ENV$ + 576ll) & 480ll) == 0ll) goto label$52;
	TMP$163$1 = 24ll;
	goto label$55;
	label$52:;
	TMP$163$1 = *(int64*)((uint8*)&ENV$ + 576ll) & 31ll;
	label$55:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$163$1 * 56ll)) + 8ll) != 4ll) goto label$54;
	{
		*($12FB_WCHARCONV*)((uint8*)&ENV$ + 608ll) = 2ll;
	}
	goto label$53;
	label$54:;
	{
		*($12FB_WCHARCONV*)((uint8*)&ENV$ + 608ll) = 0ll;
	}
	label$53:;
	HASHINIT( (struct $5THASH*)((uint8*)&ENV$ + 1232ll), 256ll, 0ll );
	HASHINIT( (struct $5THASH*)((uint8*)&ENV$ + 1256ll), 256ll, 0ll );
	HASHINIT( (struct $5THASH*)((uint8*)&ENV$ + 1280ll), 256ll, 0ll );
	STACKNEW( (struct $6TSTACK*)&PARSER$, 128ll, 176ll, 0ll );
	LEXINIT( 0ll );
	PARSERINIT(  );
	RTLINIT(  );
	*(int64*)((uint8*)&ENV$ + 1592ll) = -1ll;
	label$38:;
}

void FBEND( void )
{
	label$56:;
	*(int64*)((uint8*)&ENV$ + 1592ll) = 0ll;
	RTLEND(  );
	PARSEREND(  );
	LEXEND(  );
	STACKFREE( (struct $6TSTACK*)&PARSER$ );
	HASHEND( (struct $5THASH*)((uint8*)&ENV$ + 1232ll) );
	HASHEND( (struct $5THASH*)((uint8*)&ENV$ + 1280ll) );
	HASHEND( (struct $5THASH*)((uint8*)&ENV$ + 1256ll) );
	IREND(  );
	ASTEND(  );
	ERREND(  );
	SYMBEND(  );
	fb_ArrayErase( (struct $7FBARRAYIvE*)&INFILETB$ );
	STRSETEND( (struct $7TSTRSET*)((uint8*)&ENV$ + 1648ll) );
	STRSETEND( (struct $7TSTRSET*)((uint8*)&ENV$ + 1736ll) );
	label$57:;
}

void FBGLOBALINIT( void )
{
	label$64:;
	STRLISTINIT( (struct $5TLIST*)&ENV$, 256ll );
	STRLISTINIT( (struct $5TLIST*)((uint8*)&ENV$ + 64ll), 256ll );
	STRLISTINIT( (struct $5TLIST*)((uint8*)&ENV$ + 128ll), 256ll );
	*($10FB_OUTTYPE*)((uint8*)&ENV$ + 192ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 200ll) = 0ll;
	*($10FB_BACKEND*)((uint8*)&ENV$ + 208ll) = 1ll;
	*($13FB_COMPTARGET*)((uint8*)&ENV$ + 216ll) = 4ll;
	*($10FB_CPUTYPE*)((uint8*)&ENV$ + 224ll) = 13ll;
	*($10FB_FPUTYPE*)((uint8*)&ENV$ + 232ll) = 0ll;
	*($9FB_FPMODE*)((uint8*)&ENV$ + 240ll) = 0ll;
	*($17FB_VECTORIZELEVEL*)((uint8*)&ENV$ + 248ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 256ll) = 0ll;
	*($12FB_ASMSYNTAX*)((uint8*)&ENV$ + 264ll) = 0ll;
	*($7FB_LANG*)((uint8*)&ENV$ + 272ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 280ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 288ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 296ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 304ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 312ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 328ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 336ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 344ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 360ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 368ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 320ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 376ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 384ll) = 1ll;
	*(int64*)((uint8*)&ENV$ + 392ll) = -1ll;
	*(int64*)((uint8*)&ENV$ + 400ll) = 10ll;
	*($10FB_PDCHECK*)((uint8*)&ENV$ + 408ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 416ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 456ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 472ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 480ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 488ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 496ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 464ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 504ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 512ll) = -1ll;
	*(int64*)((uint8*)&ENV$ + 520ll) = 0ll;
	*($11FB_MODEVIEW*)((uint8*)&ENV$ + 528ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 536ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 544ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 552ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 560ll) = 0ll;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1608ll) = 0ll;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1616ll) = 0ll;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1624ll) = 0ll;
	*($7FB_LANG*)((uint8*)&ENV$ + 1640ll) = -1ll;
	*(int64*)((uint8*)&ENV$ + 1632ll) = 0ll;
	*(int64*)((uint8*)&ENV$ + 1600ll) = 0ll;
	HUPDATELANGOPTIONS(  );
	HUPDATETARGETOPTIONS(  );
	label$65:;
}

void FBADDINCLUDEPATH( FBSTRING* PATH$1 )
{
	label$66:;
	STRLISTAPPEND( (struct $5TLIST*)((uint8*)&ENV$ + 128ll), PATH$1 );
	label$67:;
}

void FBADDPREDEFINE( FBSTRING* DEF$1 )
{
	label$68:;
	STRLISTAPPEND( (struct $5TLIST*)&ENV$, DEF$1 );
	label$69:;
}

void FBADDPREINCLUDE( FBSTRING* FILE$1 )
{
	label$70:;
	STRLISTAPPEND( (struct $5TLIST*)((uint8*)&ENV$ + 64ll), FILE$1 );
	label$71:;
}

void FBSETOPTION( int64 OPT$1, int64 VALUE$1 )
{
	label$72:;
	{
		uint64 TMP$164$2;
		TMP$164$2 = (uint64)OPT$1;
		goto label$75;
		label$76:;
		{
			*($10FB_OUTTYPE*)((uint8*)&ENV$ + 192ll) = VALUE$1;
		}
		goto label$74;
		label$77:;
		{
			*(int64*)((uint8*)&ENV$ + 200ll) = VALUE$1;
		}
		goto label$74;
		label$78:;
		{
			*($10FB_BACKEND*)((uint8*)&ENV$ + 208ll) = VALUE$1;
		}
		goto label$74;
		label$79:;
		{
			*($13FB_COMPTARGET*)((uint8*)&ENV$ + 216ll) = VALUE$1;
			HUPDATETARGETOPTIONS(  );
		}
		goto label$74;
		label$80:;
		{
			*($10FB_CPUTYPE*)((uint8*)&ENV$ + 224ll) = VALUE$1;
		}
		goto label$74;
		label$81:;
		{
			*($10FB_FPUTYPE*)((uint8*)&ENV$ + 232ll) = VALUE$1;
		}
		goto label$74;
		label$82:;
		{
			*($9FB_FPMODE*)((uint8*)&ENV$ + 240ll) = VALUE$1;
		}
		goto label$74;
		label$83:;
		{
			*($17FB_VECTORIZELEVEL*)((uint8*)&ENV$ + 248ll) = VALUE$1;
		}
		goto label$74;
		label$84:;
		{
			*(int64*)((uint8*)&ENV$ + 256ll) = VALUE$1;
		}
		goto label$74;
		label$85:;
		{
			*($12FB_ASMSYNTAX*)((uint8*)&ENV$ + 264ll) = VALUE$1;
		}
		goto label$74;
		label$86:;
		{
			*($7FB_LANG*)((uint8*)&ENV$ + 272ll) = VALUE$1;
			HUPDATELANGOPTIONS(  );
		}
		goto label$74;
		label$87:;
		{
			*(int64*)((uint8*)&ENV$ + 280ll) = VALUE$1;
		}
		goto label$74;
		label$88:;
		{
			*($7FB_LANG*)((uint8*)&ENV$ + 1640ll) = VALUE$1;
		}
		goto label$74;
		label$89:;
		{
			*(int64*)((uint8*)&ENV$ + 288ll) = VALUE$1;
		}
		goto label$74;
		label$90:;
		{
			*(int64*)((uint8*)&ENV$ + 296ll) = VALUE$1;
		}
		goto label$74;
		label$91:;
		{
			*(int64*)((uint8*)&ENV$ + 304ll) = VALUE$1;
		}
		goto label$74;
		label$92:;
		{
			*(int64*)((uint8*)&ENV$ + 312ll) = VALUE$1;
		}
		goto label$74;
		label$93:;
		{
			*(int64*)((uint8*)&ENV$ + 320ll) = VALUE$1;
		}
		goto label$74;
		label$94:;
		{
			*(int64*)((uint8*)&ENV$ + 328ll) = VALUE$1;
		}
		goto label$74;
		label$95:;
		{
			*(int64*)((uint8*)&ENV$ + 336ll) = VALUE$1;
		}
		goto label$74;
		label$96:;
		{
			*(int64*)((uint8*)&ENV$ + 344ll) = VALUE$1;
		}
		goto label$74;
		label$97:;
		{
			*(int64*)((uint8*)&ENV$ + 352ll) = VALUE$1;
		}
		goto label$74;
		label$98:;
		{
			*(int64*)((uint8*)&ENV$ + 360ll) = VALUE$1;
		}
		goto label$74;
		label$99:;
		{
			*(int64*)((uint8*)&ENV$ + 368ll) = VALUE$1;
		}
		goto label$74;
		label$100:;
		{
			*(int64*)((uint8*)&ENV$ + 376ll) = VALUE$1;
		}
		goto label$74;
		label$101:;
		{
			*(int64*)((uint8*)&ENV$ + 384ll) = VALUE$1;
		}
		goto label$74;
		label$102:;
		{
			*(int64*)((uint8*)&ENV$ + 392ll) = VALUE$1;
		}
		goto label$74;
		label$103:;
		{
			*(int64*)((uint8*)&ENV$ + 400ll) = VALUE$1;
		}
		goto label$74;
		label$104:;
		{
			*($10FB_PDCHECK*)((uint8*)&ENV$ + 408ll) = VALUE$1;
		}
		goto label$74;
		label$105:;
		{
			*(int64*)((uint8*)&ENV$ + 416ll) = VALUE$1;
		}
		goto label$74;
		label$106:;
		{
			*(int64*)((uint8*)&ENV$ + 424ll) = VALUE$1;
		}
		goto label$74;
		label$107:;
		{
			*(int64*)((uint8*)&ENV$ + 432ll) = VALUE$1;
		}
		goto label$74;
		label$108:;
		{
			*(int64*)((uint8*)&ENV$ + 440ll) = VALUE$1;
		}
		goto label$74;
		label$109:;
		{
			*(int64*)((uint8*)&ENV$ + 448ll) = VALUE$1;
		}
		goto label$74;
		label$110:;
		{
			*(int64*)((uint8*)&ENV$ + 456ll) = VALUE$1;
		}
		goto label$74;
		label$111:;
		{
			*(int64*)((uint8*)&ENV$ + 464ll) = VALUE$1;
		}
		goto label$74;
		label$112:;
		{
			*(int64*)((uint8*)&ENV$ + 472ll) = VALUE$1;
		}
		goto label$74;
		label$113:;
		{
			*(int64*)((uint8*)&ENV$ + 480ll) = VALUE$1;
		}
		goto label$74;
		label$114:;
		{
			*(int64*)((uint8*)&ENV$ + 488ll) = VALUE$1;
		}
		goto label$74;
		label$115:;
		{
			*(int64*)((uint8*)&ENV$ + 496ll) = VALUE$1;
		}
		goto label$74;
		label$116:;
		{
			if( VALUE$1 >= 0ll) goto label$118;
			{
				if( *(int64*)((uint8*)&ENV$ + 504ll) != 0ll) goto label$120;
				{
					int64 TMP$165$5;
					int64 vr$0 = FBIS64BIT(  );
					if( vr$0 == 0ll) goto label$121;
					TMP$165$5 = 2097152ll;
					goto label$133;
					label$121:;
					TMP$165$5 = 1048576ll;
					label$133:;
					*(int64*)((uint8*)&ENV$ + 504ll) = TMP$165$5;
				}
				label$120:;
				label$119:;
			}
			goto label$117;
			label$118:;
			{
				int64 vr$1 = FBIS64BIT(  );
				if( vr$1 == 0ll) goto label$123;
				{
					int64 TMP$166$5;
					if( VALUE$1 >= 65536ll) goto label$124;
					TMP$166$5 = 65536ll;
					goto label$134;
					label$124:;
					TMP$166$5 = VALUE$1;
					label$134:;
					*(int64*)((uint8*)&ENV$ + 504ll) = TMP$166$5;
				}
				goto label$122;
				label$123:;
				{
					int64 TMP$167$5;
					if( VALUE$1 >= 32768ll) goto label$125;
					TMP$167$5 = 32768ll;
					goto label$135;
					label$125:;
					TMP$167$5 = VALUE$1;
					label$135:;
					*(int64*)((uint8*)&ENV$ + 504ll) = TMP$167$5;
				}
				label$122:;
			}
			label$117:;
		}
		goto label$74;
		label$126:;
		{
			*(int64*)((uint8*)&ENV$ + 512ll) = VALUE$1;
		}
		goto label$74;
		label$127:;
		{
			*(int64*)((uint8*)&ENV$ + 520ll) = VALUE$1;
		}
		goto label$74;
		label$128:;
		{
			*($11FB_MODEVIEW*)((uint8*)&ENV$ + 528ll) = VALUE$1;
		}
		goto label$74;
		label$129:;
		{
			*(int64*)((uint8*)&ENV$ + 536ll) = VALUE$1;
		}
		goto label$74;
		label$130:;
		{
			*(int64*)((uint8*)&ENV$ + 544ll) = VALUE$1;
			HUPDATETARGETOPTIONS(  );
		}
		goto label$74;
		label$131:;
		{
			*(int64*)((uint8*)&ENV$ + 552ll) = VALUE$1;
		}
		goto label$74;
		label$132:;
		{
			*(int64*)((uint8*)&ENV$ + 560ll) = VALUE$1;
		}
		goto label$74;
		label$75:;
		static const void* tmp$168[48ll] = {
			&&label$76,
			&&label$77,
			&&label$78,
			&&label$79,
			&&label$80,
			&&label$81,
			&&label$82,
			&&label$83,
			&&label$84,
			&&label$85,
			&&label$86,
			&&label$87,
			&&label$88,
			&&label$89,
			&&label$90,
			&&label$91,
			&&label$92,
			&&label$93,
			&&label$94,
			&&label$95,
			&&label$98,
			&&label$99,
			&&label$96,
			&&label$97,
			&&label$100,
			&&label$101,
			&&label$102,
			&&label$103,
			&&label$104,
			&&label$105,
			&&label$106,
			&&label$107,
			&&label$108,
			&&label$109,
			&&label$110,
			&&label$111,
			&&label$112,
			&&label$113,
			&&label$114,
			&&label$115,
			&&label$116,
			&&label$126,
			&&label$127,
			&&label$128,
			&&label$129,
			&&label$130,
			&&label$131,
			&&label$132,
		};
		if( TMP$164$2 > 47ull ) goto label$74;
		goto *tmp$168[TMP$164$2 - 0ull];
		label$74:;
	}
	label$73:;
}

int64 FBGETOPTION( int64 OPT$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$136:;
	{
		uint64 TMP$169$2;
		TMP$169$2 = (uint64)OPT$1;
		goto label$139;
		label$140:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 192ll);
		}
		goto label$138;
		label$141:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 200ll);
		}
		goto label$138;
		label$142:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 208ll);
		}
		goto label$138;
		label$143:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 216ll);
		}
		goto label$138;
		label$144:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 224ll);
		}
		goto label$138;
		label$145:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 232ll);
		}
		goto label$138;
		label$146:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 240ll);
		}
		goto label$138;
		label$147:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 248ll);
		}
		goto label$138;
		label$148:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 256ll);
		}
		goto label$138;
		label$149:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 264ll);
		}
		goto label$138;
		label$150:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 272ll);
		}
		goto label$138;
		label$151:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 280ll);
		}
		goto label$138;
		label$152:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 1640ll);
		}
		goto label$138;
		label$153:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 288ll);
		}
		goto label$138;
		label$154:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 296ll);
		}
		goto label$138;
		label$155:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 304ll);
		}
		goto label$138;
		label$156:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 312ll);
		}
		goto label$138;
		label$157:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 320ll);
		}
		goto label$138;
		label$158:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 328ll);
		}
		goto label$138;
		label$159:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 336ll);
		}
		goto label$138;
		label$160:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 344ll);
		}
		goto label$138;
		label$161:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 352ll);
		}
		goto label$138;
		label$162:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 360ll);
		}
		goto label$138;
		label$163:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 368ll);
		}
		goto label$138;
		label$164:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 376ll);
		}
		goto label$138;
		label$165:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 384ll);
		}
		goto label$138;
		label$166:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 392ll);
		}
		goto label$138;
		label$167:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 400ll);
		}
		goto label$138;
		label$168:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 408ll);
		}
		goto label$138;
		label$169:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 416ll);
		}
		goto label$138;
		label$170:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 424ll);
		}
		goto label$138;
		label$171:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 432ll);
		}
		goto label$138;
		label$172:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 440ll);
		}
		goto label$138;
		label$173:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 448ll);
		}
		goto label$138;
		label$174:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 456ll);
		}
		goto label$138;
		label$175:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 464ll);
		}
		goto label$138;
		label$176:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 472ll);
		}
		goto label$138;
		label$177:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 480ll);
		}
		goto label$138;
		label$178:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 488ll);
		}
		goto label$138;
		label$179:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 496ll);
		}
		goto label$138;
		label$180:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 504ll);
		}
		goto label$138;
		label$181:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 512ll);
		}
		goto label$138;
		label$182:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 520ll);
		}
		goto label$138;
		label$183:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 528ll);
		}
		goto label$138;
		label$184:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 536ll);
		}
		goto label$138;
		label$185:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 544ll);
		}
		goto label$138;
		label$186:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 552ll);
		}
		goto label$138;
		label$187:;
		{
			fb$result$1 = *(int64*)((uint8*)&ENV$ + 560ll);
		}
		goto label$138;
		label$188:;
		{
			fb$result$1 = 0ll;
		}
		goto label$138;
		label$139:;
		static const void* tmp$170[48ll] = {
			&&label$140,
			&&label$141,
			&&label$142,
			&&label$143,
			&&label$144,
			&&label$145,
			&&label$146,
			&&label$147,
			&&label$148,
			&&label$149,
			&&label$150,
			&&label$151,
			&&label$152,
			&&label$153,
			&&label$154,
			&&label$155,
			&&label$156,
			&&label$157,
			&&label$158,
			&&label$159,
			&&label$162,
			&&label$163,
			&&label$160,
			&&label$161,
			&&label$164,
			&&label$165,
			&&label$166,
			&&label$167,
			&&label$168,
			&&label$169,
			&&label$170,
			&&label$171,
			&&label$172,
			&&label$173,
			&&label$174,
			&&label$175,
			&&label$176,
			&&label$177,
			&&label$178,
			&&label$179,
			&&label$180,
			&&label$181,
			&&label$182,
			&&label$183,
			&&label$184,
			&&label$185,
			&&label$186,
			&&label$187,
		};
		if( TMP$169$2 > 47ull ) goto label$188;
		goto *tmp$170[TMP$169$2 - 0ull];
		label$138:;
	}
	label$137:;
	return fb$result$1;
}

void FBCHANGEOPTION( int64 OPT$1, int64 VALUE$1 )
{
	label$189:;
	{
		uint64 TMP$171$2;
		TMP$171$2 = (uint64)OPT$1;
		goto label$192;
		label$193:;
		{
			FBSETOPTION( OPT$1, VALUE$1 );
		}
		goto label$191;
		label$194:;
		{
			int64 vr$0 = FBGETOPTION( OPT$1 );
			if( VALUE$1 == vr$0) goto label$196;
			{
				if( *(uint64*)((uint8*)&PARSER$ + 200ll) == 0ull) goto label$198;
				{
					if( *(struct $8FBSYMBOL**)((uint8*)&PARSER$ + 216ll) == *(struct $8FBSYMBOL**)((uint8*)&ENV$ + 1440ll)) goto label$200;
					{
						ERRREPORT( 61ll, 0ll, (char*)0ull );
					}
					goto label$199;
					label$200:;
					{
						ERRREPORT( 96ll, 0ll, (char*)0ull );
					}
					label$199:;
				}
				goto label$197;
				label$198:;
				{
					if( *(int64*)((uint8*)&ENV$ + 280ll) == 0ll) goto label$202;
					{
						ERRREPORTWARN( 30ll, (char*)0ull, 1ll, (char*)0ull );
					}
					goto label$201;
					label$202:;
					{
						if( (*(int64*)((uint8*)&ENV$ + 1624ll) & 1ll) != 0ll) goto label$204;
						{
							FBSETOPTION( OPT$1, VALUE$1 );
							FBRESTARTBEGINREQUEST( 1ll );
							FBRESTARTACCEPTREQUEST( 1ll );
							ERRHIDEFURTHERERRORS(  );
						}
						goto label$203;
						label$204:;
						{
							ERRREPORTWARN( 31ll, (char*)0ull, 1ll, (char*)0ull );
						}
						label$203:;
					}
					label$201:;
				}
				label$197:;
			}
			label$196:;
			label$195:;
		}
		goto label$191;
		label$205:;
		{
			ERRREPORT( 21ll, 0ll, (char*)0ull );
		}
		goto label$191;
		label$192:;
		static const void* tmp$172[26ll] = {
			&&label$194,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$205,
			&&label$193,
		};
		if( (TMP$171$2 - 10ull) > 25ull ) goto label$205;
		goto *tmp$172[TMP$171$2 - 10ull];
		label$191:;
	}
	label$190:;
}

int64 FBISCROSSCOMP( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$206:;
	fb$result$1 = (int64)-(*(int64*)((uint8*)&ENV$ + 216ll) != 4ll);
	label$207:;
	return fb$result$1;
}

FBSTRING* FBGETTARGETID( void )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$219:;
	FBSTRING* vr$1 = HGETTARGETID( *(int64*)((uint8*)&ENV$ + 216ll), *(int64*)((uint8*)&ENV$ + 224ll) );
	fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)vr$1, -1ll, 0 );
	label$220:;
	FBSTRING* vr$4 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$4;
}

FBSTRING* FBGETHOSTID( void )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$221:;
	FBSTRING* vr$1 = HGETTARGETID( 4ll, 13ll );
	fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)vr$1, -1ll, 0 );
	label$222:;
	FBSTRING* vr$4 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$4;
}

int64 FBIDENTIFYOS( FBSTRING* OSID$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$223:;
	{
		int64 I$2;
		I$2 = 0ll;
		label$228:;
		{
			int32 vr$2 = fb_StrCompare( (void*)OSID$1, -1ll, *(void**)((int64)(struct $8FBTARGET*)TARGETINFO$ + (I$2 * 40ll)), 0ll );
			if( (int64)vr$2 != 0ll) goto label$230;
			{
				fb$result$1 = I$2;
				goto label$224;
			}
			label$230:;
			label$229:;
		}
		label$226:;
		I$2 = I$2 + 1ll;
		label$225:;
		if( I$2 <= 13ll) goto label$228;
		label$227:;
	}
	fb$result$1 = -1ll;
	label$224:;
	return fb$result$1;
}

int64 FBIDENTIFYCPUFAMILY( FBSTRING* CPUFAMILYID$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$231:;
	{
		int64 I$2;
		I$2 = 0ll;
		label$236:;
		{
			int32 vr$2 = fb_StrCompare( (void*)CPUFAMILYID$1, -1ll, *(void**)((int64)(struct $15FBCPUFAMILYINFO*)CPUFAMILYINFO$ + (I$2 << (4ll & 63ll))), 0ll );
			if( (int64)vr$2 != 0ll) goto label$238;
			{
				fb$result$1 = I$2;
				goto label$232;
			}
			label$238:;
			label$237:;
		}
		label$234:;
		I$2 = I$2 + 1ll;
		label$233:;
		if( I$2 <= 10ll) goto label$236;
		label$235:;
	}
	fb$result$1 = -1ll;
	label$232:;
	return fb$result$1;
}

int64 FBDEFAULTCPUTYPEFROMCPUFAMILYID( int64 OS$1, FBSTRING* CPUFAMILYID$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$239:;
	int64 CPUFAMILY$1;
	int64 vr$1 = FBIDENTIFYCPUFAMILY( CPUFAMILYID$1 );
	CPUFAMILY$1 = vr$1;
	if( CPUFAMILY$1 < 0ll) goto label$242;
	{
		fb$result$1 = *(int64*)(((int64)(struct $15FBCPUFAMILYINFO*)CPUFAMILYINFO$ + (CPUFAMILY$1 << (4ll & 63ll))) + 8ll);
		goto label$240;
	}
	label$242:;
	label$241:;
	fb$result$1 = -1ll;
	label$240:;
	return fb$result$1;
}

char* FBGETGCCARCH( void )
{
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$243:;
	char* GCCARCH$1;
	GCCARCH$1 = *(char**)((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + (*(int64*)((uint8*)&ENV$ + 224ll) * 40ll));
	if( GCCARCH$1 != (char*)0ull) goto label$246;
	{
		GCCARCH$1 = *(char**)(((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + (*(int64*)((uint8*)&ENV$ + 224ll) * 40ll)) + 8ll);
	}
	label$246:;
	label$245:;
	fb$result$1 = GCCARCH$1;
	label$244:;
	return fb$result$1;
}

char* FBGETFBCARCH( void )
{
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$247:;
	fb$result$1 = *(char**)(((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + (*(int64*)((uint8*)&ENV$ + 224ll) * 40ll)) + 8ll);
	label$248:;
	return fb$result$1;
}

int64 FBIS64BIT( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$249:;
	int64 vr$1 = FBGETBITS(  );
	fb$result$1 = (int64)-(vr$1 == 64ll);
	label$250:;
	return fb$result$1;
}

int64 FBGETBITS( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$251:;
	fb$result$1 = *(int64*)(((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + (*(int64*)((uint8*)&ENV$ + 224ll) * 40ll)) + 24ll);
	label$252:;
	return fb$result$1;
}

int64 FBGETHOSTBITS( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$253:;
	fb$result$1 = *(int64*)((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + 544ll);
	label$254:;
	return fb$result$1;
}

int64 FBGETCPUFAMILY( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$255:;
	fb$result$1 = *(int64*)(((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + (*(int64*)((uint8*)&ENV$ + 224ll) * 40ll)) + 16ll);
	label$256:;
	return fb$result$1;
}

int64 FBISHOSTBIGENDIAN( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$257:;
	fb$result$1 = *(int64*)(((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + (*(int64*)((uint8*)&ENV$ + 224ll) * 40ll)) + 32ll);
	label$258:;
	return fb$result$1;
}

int64 FBIDENTIFYFBCARCH( FBSTRING* FBCARCH$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$259:;
	{
		FBSTRING TMP$177$2;
		fb_StrInit( (void*)&TMP$177$2, -1ll, (void*)FBCARCH$1, -1ll, 0 );
		int32 vr$3 = fb_StrCompare( (void*)&TMP$177$2, -1ll, (void*)"native", 7ll );
		if( (int64)vr$3 != 0ll) goto label$262;
		label$263:;
		{
			fb$result$1 = 13ll;
			fb_StrDelete( (FBSTRING*)&TMP$177$2 );
			goto label$260;
		}
		goto label$261;
		label$262:;
		int32 vr$7 = fb_StrCompare( (void*)&TMP$177$2, -1ll, (void*)"32", 3ll );
		if( (int64)vr$7 != 0ll) goto label$264;
		label$265:;
		{
			fb$result$1 = 3ll;
			fb_StrDelete( (FBSTRING*)&TMP$177$2 );
			goto label$260;
		}
		goto label$261;
		label$264:;
		int32 vr$11 = fb_StrCompare( (void*)&TMP$177$2, -1ll, (void*)"64", 3ll );
		if( (int64)vr$11 != 0ll) goto label$266;
		label$267:;
		{
			fb$result$1 = 13ll;
			fb_StrDelete( (FBSTRING*)&TMP$177$2 );
			goto label$260;
		}
		label$266:;
		label$261:;
		fb_StrDelete( (FBSTRING*)&TMP$177$2 );
	}
	{
		int64 I$2;
		I$2 = 0ll;
		label$271:;
		{
			int32 vr$16 = fb_StrCompare( *(void**)(((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + (I$2 * 40ll)) + 8ll), 0ll, (void*)FBCARCH$1, -1ll );
			if( (int64)vr$16 != 0ll) goto label$273;
			{
				fb$result$1 = I$2;
				goto label$260;
			}
			label$273:;
			label$272:;
		}
		label$269:;
		I$2 = I$2 + 1ll;
		label$268:;
		if( I$2 <= 26ll) goto label$271;
		label$270:;
	}
	{
		FBSTRING TMP$181$2;
		fb_StrInit( (void*)&TMP$181$2, -1ll, (void*)FBCARCH$1, -1ll, 0 );
		int32 vr$21 = fb_StrCompare( (void*)&TMP$181$2, -1ll, (void*)"x86", 4ll );
		if( (int64)vr$21 != 0ll) goto label$275;
		label$276:;
		{
			fb$result$1 = 3ll;
		}
		goto label$274;
		label$275:;
		int32 vr$24 = fb_StrCompare( (void*)&TMP$181$2, -1ll, (void*)"x86_64", 7ll );
		if( (int64)vr$24 == 0ll) goto label$278;
		label$279:;
		int32 vr$27 = fb_StrCompare( (void*)&TMP$181$2, -1ll, (void*)"amd64", 6ll );
		if( (int64)vr$27 != 0ll) goto label$277;
		label$278:;
		{
			fb$result$1 = 13ll;
		}
		goto label$274;
		label$277:;
		int32 vr$30 = fb_StrCompare( (void*)&TMP$181$2, -1ll, (void*)"armv5", 6ll );
		if( (int64)vr$30 == 0ll) goto label$281;
		label$282:;
		int32 vr$33 = fb_StrCompare( (void*)&TMP$181$2, -1ll, (void*)"armeabi", 8ll );
		if( (int64)vr$33 != 0ll) goto label$280;
		label$281:;
		{
			fb$result$1 = 14ll;
		}
		goto label$274;
		label$280:;
		int32 vr$36 = fb_StrCompare( (void*)&TMP$181$2, -1ll, (void*)"armeabi-v7a", 12ll );
		if( (int64)vr$36 == 0ll) goto label$284;
		label$285:;
		int32 vr$39 = fb_StrCompare( (void*)&TMP$181$2, -1ll, (void*)"armv7a", 7ll );
		if( (int64)vr$39 == 0ll) goto label$284;
		label$286:;
		int32 vr$42 = fb_StrCompare( (void*)&TMP$181$2, -1ll, (void*)"armv7", 6ll );
		if( (int64)vr$42 != 0ll) goto label$283;
		label$284:;
		{
			fb$result$1 = 17ll;
		}
		goto label$274;
		label$283:;
		{
			fb$result$1 = -1ll;
		}
		label$287:;
		label$274:;
		fb_StrDelete( (FBSTRING*)&TMP$181$2 );
	}
	label$260:;
	return fb$result$1;
}

int64 FBTARGETSUPPORTSELF( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$288:;
	fb$result$1 = (int64)-((*(int64*)((uint8*)&ENV$ + 600ll) & 128ll) != 0ll);
	goto label$289;
	label$289:;
	return fb$result$1;
}

int64 FBTARGETSUPPORTSCOFF( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$290:;
	fb$result$1 = (int64)-((*(int64*)((uint8*)&ENV$ + 600ll) & 256ll) != 0ll);
	goto label$291;
	label$291:;
	return fb$result$1;
}

int64 FBTARGETSUPPORTSMACHO( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$292:;
	fb$result$1 = (int64)-((*(int64*)((uint8*)&ENV$ + 600ll) & 512ll) != 0ll);
	goto label$293;
	label$293:;
	return fb$result$1;
}

FBSTRING* FBGETENTRYPOINT( void )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$294:;
	int64 vr$2 = fb_StrLen( (void*)((uint8*)&ENV$ + 1312ll), 128ll );
	if( vr$2 == 0ll) goto label$297;
	{
		fb_StrInit( (void*)&fb$result$1, -1ll, (void*)((uint8*)&ENV$ + 1312ll), 128ll, 0 );
		goto label$295;
	}
	label$297:;
	label$296:;
	if( *(int64*)((uint8*)&ENV$ + 216ll) != 6ll) goto label$299;
	{
		fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)"XBoxStartup", 12ll, 0 );
	}
	goto label$298;
	label$299:;
	{
		fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)"main", 5ll, 0 );
	}
	label$298:;
	label$295:;
	FBSTRING* vr$8 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$8;
}

FBSTRING* FBGETMODULEENTRY( void )
{
	FBSTRING TMP$190$1;
	FBSTRING TMP$191$1;
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$300:;
	static FBSTRING SNAME$1;
	__builtin_memset( &TMP$191$1, 0, 24ll );
	__builtin_memset( &TMP$190$1, 0, 24ll );
	fb_StrAssign( (void*)&TMP$190$1, -1ll, (void*)((uint8*)&ENV$ + 936ll), 261ll, 0 );
	FBSTRING* vr$6 = HSTRIPEXT( &TMP$190$1 );
	fb_StrAssign( (void*)((uint8*)&ENV$ + 936ll), 261ll, (void*)&TMP$190$1, -1ll, 0 );
	fb_StrAssign( (void*)&TMP$191$1, -1ll, (void*)vr$6, -1ll, 0 );
	FBSTRING* vr$10 = HSTRIPPATH( (char*)*(char**)&TMP$191$1 );
	fb_StrAssign( (void*)&SNAME$1, -1ll, (void*)vr$10, -1ll, 0 );
	fb_StrDelete( (FBSTRING*)&TMP$191$1 );
	fb_StrDelete( (FBSTRING*)&TMP$190$1 );
	HCLEARNAME( (char*)*(char**)&SNAME$1 );
	fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)"fb_ctor__", 10ll, 0 );
	fb_StrConcatAssign( (void*)&fb$result$1, -1ll, (void*)&SNAME$1, -1ll, 0 );
	label$301:;
	FBSTRING* vr$16 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$16;
}

FBSTRING* FBGETINPUTFILEPARENTDIR( void )
{
	FBSTRING TMP$193$1;
	FBSTRING TMP$194$1;
	FBSTRING TMP$195$1;
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$302:;
	FBSTRING S$1;
	__builtin_memset( &S$1, 0, 24ll );
	int64 vr$3 = PATHISABSOLUTE( (char*)((uint8*)&ENV$ + 640ll) );
	if( vr$3 != 0ll) goto label$305;
	{
		FBSTRING* vr$4 = HCURDIR(  );
		fb_StrAssign( (void*)&S$1, -1ll, (void*)vr$4, -1ll, 0 );
		fb_StrConcatAssign( (void*)&S$1, -1ll, (void*)"/", 2ll, 0 );
	}
	label$305:;
	label$304:;
	__builtin_memset( &TMP$195$1, 0, 24ll );
	__builtin_memset( &TMP$194$1, 0, 24ll );
	__builtin_memset( &TMP$193$1, 0, 24ll );
	FBSTRING* vr$13 = fb_StrConcat( &TMP$193$1, (void*)&S$1, -1ll, (void*)((uint8*)&ENV$ + 640ll), 261ll );
	fb_StrAssign( (void*)&TMP$194$1, -1ll, (void*)vr$13, -1ll, 0 );
	FBSTRING* vr$15 = HSTRIPFILENAME( (char*)*(char**)&TMP$194$1 );
	fb_StrAssign( (void*)&TMP$195$1, -1ll, (void*)vr$15, -1ll, 0 );
	FBSTRING* vr$18 = PATHSTRIPDIV( &TMP$195$1 );
	fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)vr$18, -1ll, 0 );
	fb_StrDelete( (FBSTRING*)&TMP$195$1 );
	fb_StrDelete( (FBSTRING*)&TMP$194$1 );
	fb_StrDelete( (FBSTRING*)&S$1 );
	label$303:;
	FBSTRING* vr$24 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$24;
}

void FBADDLIB( char* LIBNAME$1 )
{
	FBSTRING TMP$197$1;
	label$306:;
	int32 vr$0 = fb_StrCompare( (void*)LIBNAME$1, 0ll, (void*)"fbgfx?", 7ll );
	if( (int64)vr$0 != 0ll) goto label$309;
	{
		FBSETOPTION( 37ll, -1ll );
		goto label$307;
	}
	label$309:;
	label$308:;
	__builtin_memset( &TMP$197$1, 0, 24ll );
	fb_StrAssign( (void*)&TMP$197$1, -1ll, (void*)LIBNAME$1, 0ll, 0 );
	STRSETADD( (struct $7TSTRSET*)((uint8*)&ENV$ + 1648ll), &TMP$197$1, 0ll );
	fb_StrDelete( (FBSTRING*)&TMP$197$1 );
	label$307:;
}

void FBADDLIBPATH( char* PATH$1 )
{
	FBSTRING TMP$198$1;
	FBSTRING TMP$199$1;
	label$310:;
	__builtin_memset( &TMP$199$1, 0, 24ll );
	__builtin_memset( &TMP$198$1, 0, 24ll );
	fb_StrAssign( (void*)&TMP$198$1, -1ll, (void*)PATH$1, 0ll, 0 );
	FBSTRING* vr$4 = PATHSTRIPDIV( &TMP$198$1 );
	fb_StrAssign( (void*)&TMP$199$1, -1ll, (void*)vr$4, -1ll, 0 );
	STRSETADD( (struct $7TSTRSET*)((uint8*)&ENV$ + 1736ll), &TMP$199$1, 0ll );
	fb_StrDelete( (FBSTRING*)&TMP$199$1 );
	fb_StrDelete( (FBSTRING*)&TMP$198$1 );
	label$311:;
}

void FBCOMPILE( char* INFNAME$1, char* OUTFNAME$1, FBSTRING* PPONLYFILE$1, int64 ISMAIN$1 )
{
	label$356:;
	fb_StrAssign( (void*)((uint8*)&ENV$ + 640ll), 261ll, (void*)INFNAME$1, 0ll, 0 );
	HREPLACESLASH( (char*)((uint8*)&ENV$ + 640ll), 47ll );
	char* vr$4 = HADDINCFILE( (struct $5THASH*)0ull, (struct $5THASH*)((uint8*)&ENV$ + 1232ll), (char*)((uint8*)&ENV$ + 640ll) );
	*(char**)((uint8*)&ENV$ + 904ll) = vr$4;
	*(int64*)((uint8*)&ENV$ + 912ll) = ISMAIN$1;
	fb_StrAssign( (void*)((uint8*)&ENV$ + 936ll), 261ll, (void*)OUTFNAME$1, 0ll, 0 );
	*(int64*)((uint8*)&ENV$ + 1208ll) = ISMAIN$1;
	int64 vr$6 = HFILEEXISTS( INFNAME$1 );
	if( vr$6 != 0ll) goto label$359;
	{
		ERRREPORTEX( 23ll, (char*)INFNAME$1, -1ll, 1ll, (char*)0ull );
		goto label$357;
	}
	label$359:;
	label$358:;
	int32 vr$7 = fb_FileFree(  );
	*(int64*)((uint8*)&ENV$ + 632ll) = (int64)vr$7;
	FBSTRING* vr$10 = fb_StrAllocTempDescZ( (char*)INFNAME$1 );
	int32 vr$11 = fb_FileOpen( (FBSTRING*)vr$10, 0u, 1u, 0u, (int32)*(int64*)((uint8*)&ENV$ + 632ll), 0 );
	if( (int64)vr$11 == 0ll) goto label$361;
	{
		ERRREPORTEX( 26ll, (char*)INFNAME$1, -1ll, 1ll, (char*)0ull );
		goto label$357;
	}
	label$361:;
	label$360:;
	if( *(int64*)((uint8*)&ENV$ + 520ll) == 0ll) goto label$363;
	{
		FBSTRING TMP$210$2;
		FBSTRING TMP$211$2;
		__builtin_memset( &TMP$211$2, 0, 24ll );
		__builtin_memset( &TMP$210$2, 0, 24ll );
		fb_StrAssign( (void*)&TMP$210$2, -1ll, (void*)INFNAME$1, 0ll, 0 );
		FBSTRING* vr$17 = PATHSTRIPCURDIR( &TMP$210$2 );
		fb_StrAssign( (void*)&TMP$211$2, -1ll, (void*)vr$17, -1ll, 0 );
		HSHOWINCLUDE( 0ll, &TMP$211$2 );
		fb_StrDelete( (FBSTRING*)&TMP$211$2 );
		fb_StrDelete( (FBSTRING*)&TMP$210$2 );
	}
	label$363:;
	label$362:;
	int64 vr$22 = HCHECKFILEFORMAT( *(int64*)((uint8*)&ENV$ + 632ll) );
	*($13FBFILE_FORMAT*)((uint8*)&ENV$ + 920ll) = vr$22;
	int64 vr$23 = (*(tmp$40*)((uint8*)&IR$ + 16ll))(  );
	if( vr$23 != 0ll) goto label$365;
	{
		ERRREPORTEX( 26ll, (char*)((uint8*)&ENV$ + 936ll), -1ll, 1ll, (char*)0ull );
		goto label$357;
	}
	label$365:;
	label$364:;
	int64 vr$25 = FBGETOPTION( 1ll );
	if( vr$25 == 0ll) goto label$367;
	{
		int32 vr$26 = fb_FileFree(  );
		*(int64*)((uint8*)&ENV$ + 1224ll) = (int64)vr$26;
		int32 vr$29 = fb_FileOpen( (FBSTRING*)PPONLYFILE$1, 3u, 0u, 0u, (int32)*(int64*)((uint8*)&ENV$ + 1224ll), 0 );
		if( (int64)vr$29 == 0ll) goto label$369;
		{
			ERRREPORTEX( 26ll, (char*)*(char**)PPONLYFILE$1, -1ll, 1ll, (char*)0ull );
			goto label$357;
		}
		label$369:;
		label$368:;
	}
	goto label$366;
	label$367:;
	{
		*(int64*)((uint8*)&ENV$ + 1224ll) = 0ll;
	}
	label$366:;
	FBMAINBEGIN(  );
	FBPARSEPREDEFINES(  );
	FBPARSEPREINCLUDES(  );
	int64 vr$32 = FBSHOULDCONTINUE(  );
	if( vr$32 == 0ll) goto label$371;
	{
		CPROGRAM(  );
	}
	label$371:;
	label$370:;
	int64 vr$33 = FBSHOULDCONTINUE(  );
	if( vr$33 == 0ll) goto label$373;
	{
		FBMAINEND(  );
	}
	label$373:;
	label$372:;
	int64 vr$34 = FBGETOPTION( 41ll );
	int64 vr$35 = FBISCROSSCOMP(  );
	if( ((vr$34 & ~vr$35) & (int64)-(*(int64*)((uint8*)&ENV$ + 192ll) == 3ll)) == 0ll) goto label$375;
	{
		HEMITOBJINFO(  );
	}
	label$375:;
	label$374:;
	(*(tmp$39*)((uint8*)&IR$ + 24ll))(  );
	if( *(int64*)((uint8*)&ENV$ + 1224ll) <= 0ll) goto label$377;
	{
		int32 vr$41 = fb_FileClose( (int32)*(int64*)((uint8*)&ENV$ + 1224ll) );
		if( (int64)vr$41 == 0ll) goto label$378;
		void* vr$43 = fb_ErrorThrowAt( 1380, (char*)"/boot/home/Downloads/fbc-fbxl/src/compiler/fb.bas", (void*)0ull, (void*)0ull );
		goto *vr$43;
		label$378:;
	}
	label$377:;
	label$376:;
	int32 vr$45 = fb_FileClose( (int32)*(int64*)((uint8*)&ENV$ + 632ll) );
	if( (int64)vr$45 == 0ll) goto label$379;
	void* vr$47 = fb_ErrorThrowAt( 1383, (char*)"/boot/home/Downloads/fbc-fbxl/src/compiler/fb.bas", (void*)0ull, (void*)0ull );
	goto *vr$47;
	label$379:;
	int64 vr$48 = FBSHOULDCONTINUE(  );
	if( vr$48 == 0ll) goto label$381;
	{
		SYMBCHECKLABELS( *(struct $8FBSYMBOL**)((uint8*)&SYMB$ + 196808ll) );
	}
	label$381:;
	label$380:;
	label$357:;
}

int64 FBSHOULDRESTART( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$382:;
	if( (*(int64*)((uint8*)&ENV$ + 1608ll) & 10ll) == 0ll) goto label$385;
	{
		if( (*(int64*)((uint8*)&ENV$ + 1624ll) & 10ll) != 0ll) goto label$387;
		{
			FBRESTARTACCEPTREQUEST( 10ll );
			ERRHIDEFURTHERERRORS(  );
			fb$result$1 = -1ll;
			goto label$383;
		}
		label$387:;
		label$386:;
	}
	label$385:;
	label$384:;
	fb$result$1 = (int64)-(*(int64*)((uint8*)&ENV$ + 1616ll) != 0ll);
	goto label$383;
	label$383:;
	return fb$result$1;
}

int64 FBSHOULDCONTINUE( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$388:;
	int64 vr$2 = ERRGETCOUNT(  );
	fb$result$1 = (int64)-(*(int64*)((uint8*)&ENV$ + 1616ll) == 0ll) & (int64)-(vr$2 < *(int64*)((uint8*)&ENV$ + 400ll));
	goto label$389;
	label$389:;
	return fb$result$1;
}

void FBRESTARTBEGINREQUEST( $16FB_RESTART_FLAGS FLAGS$1 )
{
	label$390:;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1608ll) = *(int64*)((uint8*)&ENV$ + 1608ll) | FLAGS$1;
	label$391:;
}

void FBRESTARTACCEPTREQUEST( $16FB_RESTART_FLAGS FILTER$1 )
{
	label$392:;
	if( (*(int64*)((uint8*)&ENV$ + 1608ll) & FILTER$1) == 0ll) goto label$395;
	{
		if( ((*(int64*)((uint8*)&ENV$ + 1624ll) & *(int64*)((uint8*)&ENV$ + 1608ll)) & FILTER$1) != 0ll) goto label$397;
		{
			*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1616ll) = *(int64*)((uint8*)&ENV$ + 1616ll) | (*(int64*)((uint8*)&ENV$ + 1608ll) & FILTER$1);
		}
		label$397:;
		label$396:;
	}
	label$395:;
	label$394:;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1608ll) = *(int64*)((uint8*)&ENV$ + 1608ll) & ~FILTER$1;
	label$393:;
}

void FBRESTARTENDREQUEST( $16FB_RESTART_FLAGS FILTER$1 )
{
	label$398:;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1624ll) = *(int64*)((uint8*)&ENV$ + 1624ll) | (*(int64*)((uint8*)&ENV$ + 1616ll) & FILTER$1);
	*(int64*)((uint8*)&ENV$ + 1632ll) = *(int64*)((uint8*)&ENV$ + 1632ll) + 1ll;
	*($16FB_RESTART_FLAGS*)((uint8*)&ENV$ + 1616ll) = *(int64*)((uint8*)&ENV$ + 1616ll) & ~FILTER$1;
	label$399:;
}

int64 FBRESTARTGETCOUNT( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$400:;
	fb$result$1 = *(int64*)((uint8*)&ENV$ + 1632ll);
	goto label$401;
	label$401:;
	return fb$result$1;
}

void FBSETLIBS( struct $7TSTRSET* LIBS$1, struct $7TSTRSET* LIBPATHS$1 )
{
	label$402:;
	STRSETCOPY( (struct $7TSTRSET*)((uint8*)&ENV$ + 1648ll), LIBS$1 );
	STRSETCOPY( (struct $7TSTRSET*)((uint8*)&ENV$ + 1736ll), LIBPATHS$1 );
	label$403:;
}

void FBGETLIBS( struct $7TSTRSET* LIBS$1, struct $7TSTRSET* LIBPATHS$1 )
{
	label$404:;
	STRSETCOPY( LIBS$1, (struct $7TSTRSET*)((uint8*)&ENV$ + 1648ll) );
	STRSETCOPY( LIBPATHS$1, (struct $7TSTRSET*)((uint8*)&ENV$ + 1736ll) );
	label$405:;
}

void FBPRAGMAONCE( void )
{
	label$406:;
	int32 vr$1 = fb_StrCompare( (void*)((uint8*)&ENV$ + 640ll), 261ll, (void*)"", 1ll );
	if( (int64)vr$1 <= 0ll) goto label$409;
	{
		char* vr$5 = HFINDINCFILE( (struct $5THASH*)((uint8*)&ENV$ + 1280ll), (char*)((uint8*)&ENV$ + 640ll) );
		if( vr$5 != (char*)0ull) goto label$411;
		{
			HADDINCFILE( (struct $5THASH*)((uint8*)&ENV$ + 1280ll), (struct $5THASH*)((uint8*)&ENV$ + 1232ll), (char*)((uint8*)&ENV$ + 640ll) );
		}
		label$411:;
		label$410:;
	}
	label$409:;
	label$408:;
	label$407:;
}

void FBINCLUDEFILE( char* FILENAME$1, int64 ISONCE$1 )
{
	FBSTRING TMP$215$1;
	char* TMP$225$1;
	char* TMP$228$1;
	label$436:;
	static char INCFILE$1[260];
	char* FILEIDX$1;
	__builtin_memset( &FILEIDX$1, 0, 8ll );
	if( *(int64*)((uint8*)&ENV$ + 1304ll) < 16ll) goto label$439;
	{
		ERRREPORT( 27ll, 0ll, (char*)0ull );
		ERRHIDEFURTHERERRORS(  );
		goto label$437;
	}
	label$439:;
	label$438:;
	FBSTRING* vr$2 = HSTRIPFILENAME( (char*)((uint8*)&ENV$ + 640ll) );
	fb_StrAssign( (void*)INCFILE$1, 260ll, (void*)vr$2, -1ll, 0 );
	__builtin_memset( &TMP$215$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$215$1, (void*)INCFILE$1, 260ll, (void*)FILENAME$1, 0ll );
	fb_StrAssign( (void*)INCFILE$1, 260ll, (void*)vr$5, -1ll, 0 );
	int64 vr$6 = HFILEEXISTS( (char*)INCFILE$1 );
	if( vr$6 != 0ll) goto label$441;
	{
		int64 vr$7 = HFILEEXISTS( FILENAME$1 );
		if( vr$7 != 0ll) goto label$443;
		{
			FBSTRING* PATH$3;
			void* vr$9 = LISTGETHEAD( (struct $5TLIST*)((uint8*)&ENV$ + 128ll) );
			PATH$3 = (FBSTRING*)vr$9;
			label$444:;
			if( PATH$3 == (FBSTRING*)0ull) goto label$445;
			{
				FBSTRING TMP$216$4;
				FBSTRING TMP$217$4;
				__builtin_memset( &TMP$216$4, 0, 24ll );
				FBSTRING* vr$12 = fb_StrConcat( &TMP$216$4, (void*)PATH$3, -1ll, (void*)"/", 2ll );
				__builtin_memset( &TMP$217$4, 0, 24ll );
				FBSTRING* vr$15 = fb_StrConcat( &TMP$217$4, (void*)vr$12, -1ll, (void*)FILENAME$1, 0ll );
				fb_StrAssign( (void*)INCFILE$1, 260ll, (void*)vr$15, -1ll, 0 );
				int64 vr$16 = HFILEEXISTS( (char*)INCFILE$1 );
				if( vr$16 == 0ll) goto label$447;
				{
					goto label$445;
				}
				label$447:;
				label$446:;
				void* vr$17 = LISTGETNEXT( (void*)PATH$3 );
				PATH$3 = (FBSTRING*)vr$17;
			}
			goto label$444;
			label$445:;
			if( PATH$3 != (FBSTRING*)0ull) goto label$449;
			{
				FBSTRING TMP$222$4;
				FBSTRING TMP$223$4;
				FBSTRING TMP$224$4;
				if( *(int64*)((uint8*)&ENV$ + 520ll) == 0ll) goto label$451;
				{
					FBSTRING TMP$219$5;
					FBSTRING TMP$220$5;
					__builtin_memset( &TMP$220$5, 0, 24ll );
					__builtin_memset( &TMP$219$5, 0, 24ll );
					FBSTRING* vr$21 = fb_StrConcat( &TMP$219$5, (void*)FILENAME$1, 0ll, (void*)" (not found in include dirs)", 29ll );
					fb_StrAssign( (void*)&TMP$220$5, -1ll, (void*)vr$21, -1ll, 0 );
					HSHOWINCLUDE( *(int64*)((uint8*)&ENV$ + 1304ll) + 1ll, &TMP$220$5 );
					fb_StrDelete( (FBSTRING*)&TMP$220$5 );
				}
				label$451:;
				label$450:;
				__builtin_memset( &TMP$224$4, 0, 24ll );
				__builtin_memset( &TMP$222$4, 0, 24ll );
				FBSTRING* vr$29 = fb_StrConcat( &TMP$222$4, (void*)"\x22", 2ll, (void*)FILENAME$1, 0ll );
				__builtin_memset( &TMP$223$4, 0, 24ll );
				FBSTRING* vr$32 = fb_StrConcat( &TMP$223$4, (void*)vr$29, -1ll, (void*)"\x22", 2ll );
				fb_StrAssign( (void*)&TMP$224$4, -1ll, (void*)vr$32, -1ll, 0 );
				ERRREPORTEX( 23ll, (char*)*(char**)&TMP$224$4, 0ll, 1ll, (char*)0ull );
				fb_StrDelete( (FBSTRING*)&TMP$224$4 );
				ERRHIDEFURTHERERRORS(  );
				goto label$437;
			}
			label$449:;
			label$448:;
		}
		goto label$442;
		label$443:;
		{
			fb_StrAssign( (void*)INCFILE$1, 260ll, (void*)FILENAME$1, 0ll, 0 );
		}
		label$442:;
	}
	label$441:;
	label$440:;
	TMP$225$1 = (char*)INCFILE$1;
	int64 vr$36 = IS_ROOTPATH( &TMP$225$1 );
	if( vr$36 != 0ll) goto label$453;
	{
		FBSTRING TMP$226$2;
		FBSTRING TMP$227$2;
		FBSTRING* vr$37 = HCURDIR(  );
		__builtin_memset( &TMP$226$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$226$2, (void*)vr$37, -1ll, (void*)"/", 2ll );
		__builtin_memset( &TMP$227$2, 0, 24ll );
		FBSTRING* vr$43 = fb_StrConcat( &TMP$227$2, (void*)vr$40, -1ll, (void*)INCFILE$1, 260ll );
		fb_StrAssign( (void*)INCFILE$1, 260ll, (void*)vr$43, -1ll, 0 );
	}
	label$453:;
	label$452:;
	TMP$228$1 = (char*)INCFILE$1;
	int64 vr$45 = IS_ROOTPATH( &TMP$228$1 );
	if( vr$45 != 0ll) goto label$455;
	{
		FBSTRING TMP$229$2;
		FBSTRING TMP$230$2;
		FBSTRING TMP$231$2;
		__builtin_memset( &TMP$231$2, 0, 24ll );
		__builtin_memset( &TMP$229$2, 0, 24ll );
		FBSTRING* vr$49 = fb_StrConcat( &TMP$229$2, (void*)"\x22", 2ll, (void*)INCFILE$1, 260ll );
		__builtin_memset( &TMP$230$2, 0, 24ll );
		FBSTRING* vr$52 = fb_StrConcat( &TMP$230$2, (void*)vr$49, -1ll, (void*)"\x22", 2ll );
		fb_StrAssign( (void*)&TMP$231$2, -1ll, (void*)vr$52, -1ll, 0 );
		ERRREPORTEX( 23ll, (char*)*(char**)&TMP$231$2, 0ll, 1ll, (char*)0ull );
		fb_StrDelete( (FBSTRING*)&TMP$231$2 );
		ERRHIDEFURTHERERRORS(  );
		goto label$437;
	}
	label$455:;
	label$454:;
	int64 vr$55 = SOLVE_PATH( (char*)INCFILE$1 );
	if( vr$55 != 0ll) goto label$457;
	{
		FBSTRING TMP$232$2;
		FBSTRING TMP$233$2;
		FBSTRING TMP$234$2;
		__builtin_memset( &TMP$234$2, 0, 24ll );
		__builtin_memset( &TMP$232$2, 0, 24ll );
		FBSTRING* vr$59 = fb_StrConcat( &TMP$232$2, (void*)"\x22", 2ll, (void*)INCFILE$1, 260ll );
		__builtin_memset( &TMP$233$2, 0, 24ll );
		FBSTRING* vr$62 = fb_StrConcat( &TMP$233$2, (void*)vr$59, -1ll, (void*)"\x22", 2ll );
		fb_StrAssign( (void*)&TMP$234$2, -1ll, (void*)vr$62, -1ll, 0 );
		ERRREPORTEX( 23ll, (char*)*(char**)&TMP$234$2, 0ll, 1ll, (char*)0ull );
		fb_StrDelete( (FBSTRING*)&TMP$234$2 );
		ERRHIDEFURTHERERRORS(  );
		goto label$437;
	}
	label$457:;
	label$456:;
	HREPLACESLASH( (char*)INCFILE$1, 47ll );
	if( ISONCE$1 == 0ll) goto label$459;
	{
		char* vr$66 = HFINDINCFILE( (struct $5THASH*)((uint8*)&ENV$ + 1256ll), (char*)INCFILE$1 );
		if( vr$66 == (char*)0ull) goto label$461;
		{
			FBSTRING TMP$235$3;
			__builtin_memset( &TMP$235$3, 0, 24ll );
			fb_StrAssign( (void*)&TMP$235$3, -1ll, (void*)INCFILE$1, 260ll, 0 );
			HONSKIPPEDFILE( &TMP$235$3 );
			fb_StrAssign( (void*)INCFILE$1, 260ll, (void*)&TMP$235$3, -1ll, 0 );
			fb_StrDelete( (FBSTRING*)&TMP$235$3 );
			goto label$437;
		}
		label$461:;
		label$460:;
	}
	label$459:;
	label$458:;
	char* vr$73 = HFINDINCFILE( (struct $5THASH*)((uint8*)&ENV$ + 1280ll), (char*)INCFILE$1 );
	if( vr$73 == (char*)0ull) goto label$463;
	{
		FBSTRING TMP$236$2;
		__builtin_memset( &TMP$236$2, 0, 24ll );
		fb_StrAssign( (void*)&TMP$236$2, -1ll, (void*)INCFILE$1, 260ll, 0 );
		HONSKIPPEDFILE( &TMP$236$2 );
		fb_StrAssign( (void*)INCFILE$1, 260ll, (void*)&TMP$236$2, -1ll, 0 );
		fb_StrDelete( (FBSTRING*)&TMP$236$2 );
		goto label$437;
	}
	label$463:;
	label$462:;
	char* vr$81 = HADDINCFILE( (struct $5THASH*)((uint8*)&ENV$ + 1256ll), (struct $5THASH*)((uint8*)&ENV$ + 1232ll), (char*)INCFILE$1 );
	FILEIDX$1 = vr$81;
	__builtin_memcpy( (struct $6FBFILE*)((*(int64*)((uint8*)&ENV$ + 1304ll) * 296ll) + *(int64*)&INFILETB$), (struct $6FBFILE*)((uint8*)&ENV$ + 632ll), 296 );
	*(int64*)((uint8*)&ENV$ + 1304ll) = *(int64*)((uint8*)&ENV$ + 1304ll) + 1ll;
	fb_StrAssign( (void*)((uint8*)&ENV$ + 640ll), 261ll, (void*)INCFILE$1, 260ll, 0 );
	*(char**)((uint8*)&ENV$ + 904ll) = FILEIDX$1;
	if( *(int64*)((uint8*)&ENV$ + 520ll) == 0ll) goto label$465;
	{
		FBSTRING TMP$237$2;
		FBSTRING TMP$238$2;
		__builtin_memset( &TMP$238$2, 0, 24ll );
		__builtin_memset( &TMP$237$2, 0, 24ll );
		fb_StrAssign( (void*)&TMP$237$2, -1ll, (void*)INCFILE$1, 260ll, 0 );
		FBSTRING* vr$92 = PATHSTRIPCURDIR( &TMP$237$2 );
		fb_StrAssign( (void*)INCFILE$1, 260ll, (void*)&TMP$237$2, -1ll, 0 );
		fb_StrAssign( (void*)&TMP$238$2, -1ll, (void*)vr$92, -1ll, 0 );
		HSHOWINCLUDE( *(int64*)((uint8*)&ENV$ + 1304ll), &TMP$238$2 );
		fb_StrDelete( (FBSTRING*)&TMP$238$2 );
		fb_StrDelete( (FBSTRING*)&TMP$237$2 );
	}
	label$465:;
	label$464:;
	int32 vr$98 = fb_FileFree(  );
	*(int64*)((uint8*)&ENV$ + 632ll) = (int64)vr$98;
	FBSTRING* vr$101 = fb_StrAllocTempDescZ( (char*)INCFILE$1 );
	int32 vr$102 = fb_FileOpen( (FBSTRING*)vr$101, 0u, 1u, 0u, (int32)*(int64*)((uint8*)&ENV$ + 632ll), 0 );
	if( (int64)vr$102 == 0ll) goto label$467;
	{
		FBSTRING TMP$239$2;
		FBSTRING TMP$240$2;
		FBSTRING TMP$241$2;
		__builtin_memset( &TMP$241$2, 0, 24ll );
		__builtin_memset( &TMP$239$2, 0, 24ll );
		FBSTRING* vr$107 = fb_StrConcat( &TMP$239$2, (void*)"\x22", 2ll, (void*)FILENAME$1, 0ll );
		__builtin_memset( &TMP$240$2, 0, 24ll );
		FBSTRING* vr$110 = fb_StrConcat( &TMP$240$2, (void*)vr$107, -1ll, (void*)"\x22", 2ll );
		fb_StrAssign( (void*)&TMP$241$2, -1ll, (void*)vr$110, -1ll, 0 );
		ERRREPORTEX( 23ll, (char*)*(char**)&TMP$241$2, 0ll, 1ll, (char*)0ull );
		fb_StrDelete( (FBSTRING*)&TMP$241$2 );
		ERRHIDEFURTHERERRORS(  );
		goto label$437;
	}
	label$467:;
	label$466:;
	int64 vr$113 = HCHECKFILEFORMAT( *(int64*)((uint8*)&ENV$ + 632ll) );
	*($13FBFILE_FORMAT*)((uint8*)&ENV$ + 920ll) = vr$113;
	LEXPUSHCTX(  );
	LEXINIT( 1ll );
	CPROGRAM(  );
	LEXPOPCTX(  );
	int32 vr$115 = fb_FileClose( (int32)*(int64*)((uint8*)&ENV$ + 632ll) );
	if( (int64)vr$115 == 0ll) goto label$468;
	void* vr$117 = fb_ErrorThrowAt( 1730, (char*)"/boot/home/Downloads/fbc-fbxl/src/compiler/fb.bas", (void*)0ull, (void*)0ull );
	goto *vr$117;
	label$468:;
	*(int64*)((uint8*)&ENV$ + 1304ll) = *(int64*)((uint8*)&ENV$ + 1304ll) + -1ll;
	__builtin_memcpy( (struct $6FBFILE*)((uint8*)&ENV$ + 632ll), (struct $6FBFILE*)((*(int64*)((uint8*)&ENV$ + 1304ll) * 296ll) + *(int64*)&INFILETB$), 296 );
	label$437:;
}

void FBOVERRIDEFILENAME( char* FILENAME$1 )
{
	label$469:;
	fb_StrAssign( (void*)((uint8*)&ENV$ + 640ll), 261ll, (void*)FILENAME$1, 0ll, 0 );
	char* vr$2 = HADDINCFILE( (struct $5THASH*)0ull, (struct $5THASH*)((uint8*)&ENV$ + 1232ll), FILENAME$1 );
	*(char**)((uint8*)&ENV$ + 904ll) = vr$2;
	label$470:;
}

$7FB_LANG FBGETLANGID( char* TXT$1 )
{
	$7FB_LANG fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$471:;
	{
		FBSTRING TMP$242$2;
		FBSTRING* vr$1 = fb_StrAllocTempDescZ( (char*)TXT$1 );
		FBSTRING* vr$2 = fb_StrLcase2( (FBSTRING*)vr$1, 0 );
		fb_StrInit( (void*)&TMP$242$2, -1ll, (void*)vr$2, -1ll, 0 );
		int32 vr$5 = fb_StrCompare( (void*)&TMP$242$2, -1ll, (void*)"fb", 3ll );
		if( (int64)vr$5 != 0ll) goto label$474;
		label$475:;
		{
			fb$result$1 = 0ll;
		}
		goto label$473;
		label$474:;
		int32 vr$8 = fb_StrCompare( (void*)&TMP$242$2, -1ll, (void*)"deprecated", 11ll );
		if( (int64)vr$8 != 0ll) goto label$476;
		label$477:;
		{
			fb$result$1 = 1ll;
		}
		goto label$473;
		label$476:;
		int32 vr$11 = fb_StrCompare( (void*)&TMP$242$2, -1ll, (void*)"fblite", 7ll );
		if( (int64)vr$11 != 0ll) goto label$478;
		label$479:;
		{
			fb$result$1 = 2ll;
		}
		goto label$473;
		label$478:;
		int32 vr$14 = fb_StrCompare( (void*)&TMP$242$2, -1ll, (void*)"qb", 3ll );
		if( (int64)vr$14 != 0ll) goto label$480;
		label$481:;
		{
			fb$result$1 = 3ll;
		}
		goto label$473;
		label$480:;
		{
			fb$result$1 = -1ll;
		}
		label$482:;
		label$473:;
		fb_StrDelete( (FBSTRING*)&TMP$242$2 );
	}
	label$472:;
	return fb$result$1;
}

$19FB_CVA_LIST_TYPEDEF FBGETBACKENDVALISTTYPE( void )
{
	$19FB_CVA_LIST_TYPEDEF fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$483:;
	$19FB_CVA_LIST_TYPEDEF TYPEDEF$1;
	TYPEDEF$1 = 0ll;
	{
		$10FB_BACKEND TMP$243$2;
		TMP$243$2 = *($10FB_BACKEND*)((uint8*)&ENV$ + 208ll);
		if( TMP$243$2 == 1ll) goto label$487;
		label$488:;
		if( TMP$243$2 != 2ll) goto label$486;
		label$487:;
		{
			{
				int64 TMP$244$4;
				int64 vr$1 = FBGETCPUFAMILY(  );
				TMP$244$4 = vr$1;
				if( TMP$244$4 != 0ll) goto label$490;
				label$491:;
				{
					TYPEDEF$1 = 2ll;
				}
				goto label$489;
				label$490:;
				if( TMP$244$4 != 1ll) goto label$492;
				label$493:;
				{
					{
						$13FB_COMPTARGET TMP$245$6;
						TMP$245$6 = *($13FB_COMPTARGET*)((uint8*)&ENV$ + 216ll);
						if( TMP$245$6 != 0ll) goto label$495;
						label$496:;
						{
							TYPEDEF$1 = 2ll;
						}
						goto label$494;
						label$495:;
						{
							TYPEDEF$1 = 3ll;
						}
						label$497:;
						label$494:;
					}
				}
				goto label$489;
				label$492:;
				if( TMP$244$4 != 2ll) goto label$498;
				label$499:;
				{
					TYPEDEF$1 = 5ll;
				}
				goto label$489;
				label$498:;
				if( TMP$244$4 != 3ll) goto label$500;
				label$501:;
				{
					TYPEDEF$1 = 4ll;
				}
				goto label$489;
				label$500:;
				if( TMP$244$4 != 4ll) goto label$502;
				label$503:;
				{
					TYPEDEF$1 = 2ll;
				}
				goto label$489;
				label$502:;
				if( TMP$244$4 == 5ll) goto label$505;
				label$506:;
				if( TMP$244$4 != 6ll) goto label$504;
				label$505:;
				{
					TYPEDEF$1 = 2ll;
				}
				goto label$489;
				label$504:;
				{
					TYPEDEF$1 = 2ll;
				}
				label$507:;
				label$489:;
			}
		}
		goto label$485;
		label$486:;
		if( TMP$243$2 != 0ll) goto label$508;
		label$509:;
		{
			TYPEDEF$1 = 1ll;
		}
		goto label$485;
		label$508:;
		if( TMP$243$2 != 3ll) goto label$510;
		label$511:;
		{
			TYPEDEF$1 = 1ll;
		}
		goto label$485;
		label$510:;
		if( TMP$243$2 != 4ll) goto label$512;
		label$513:;
		{
			{
				$13FB_COMPTARGET TMP$246$4;
				TMP$246$4 = *($13FB_COMPTARGET*)((uint8*)&ENV$ + 216ll);
				if( TMP$246$4 != 0ll) goto label$515;
				label$516:;
				{
					TYPEDEF$1 = 2ll;
				}
				goto label$514;
				label$515:;
				{
					TYPEDEF$1 = 3ll;
				}
				label$517:;
				label$514:;
			}
		}
		goto label$485;
		label$512:;
		{
			TYPEDEF$1 = 1ll;
		}
		label$518:;
		label$485:;
	}
	if( TYPEDEF$1 != 2ll) goto label$520;
	{
		int64 vr$2 = FBGETOPTION( 30ll );
		if( vr$2 == 0ll) goto label$522;
		{
			TYPEDEF$1 = 1ll;
		}
		label$522:;
		label$521:;
	}
	label$520:;
	label$519:;
	fb$result$1 = TYPEDEF$1;
	label$484:;
	return fb$result$1;
}

__attribute__(( constructor )) static void fb_ctor__fb( void )
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

static char* HFINDINCFILE( struct $5THASH* INCFILEHASH$1, char* FILENAME$1 )
{
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$10:;
	static FBSTRING FNAME$1;
	fb_StrAssign( (void*)&FNAME$1, -1ll, (void*)FILENAME$1, 0ll, 0 );
	void* vr$1 = HASHLOOKUP( INCFILEHASH$1, (char*)*(char**)&FNAME$1 );
	fb$result$1 = (char*)vr$1;
	label$11:;
	return fb$result$1;
}

static char* HADDINCFILE( struct $5THASH* INCFILEHASH$1, struct $5THASH* FILENAMEHASH$1, char* FILENAME$1 )
{
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$12:;
	static char* FNAME$1;
	static char* RES$1;
	static uint64 INDEX$1;
	int64 vr$1 = fb_StrLen( (void*)FILENAME$1, 0ll );
	void* vr$4 = XALLOCATE( (int32)(vr$1 + 1ll) );
	FNAME$1 = (char*)vr$4;
	fb_StrAssign( (void*)FNAME$1, 0ll, (void*)FILENAME$1, 0ll, 0 );
	uint64 vr$5 = HASHHASH( (char*)FNAME$1 );
	INDEX$1 = vr$5;
	void* vr$6 = HASHLOOKUPEX( FILENAMEHASH$1, (char*)FNAME$1, INDEX$1 );
	RES$1 = (char*)vr$6;
	if( RES$1 != (char*)0ull) goto label$15;
	{
		HASHADD( FILENAMEHASH$1, (char*)FNAME$1, (void*)FNAME$1, INDEX$1 );
	}
	goto label$14;
	label$15:;
	{
		free( (void*)FNAME$1 );
		FNAME$1 = RES$1;
	}
	label$14:;
	if( INCFILEHASH$1 == (struct $5THASH*)0ull) goto label$17;
	{
		void* vr$7 = HASHLOOKUPEX( INCFILEHASH$1, (char*)FNAME$1, INDEX$1 );
		RES$1 = (char*)vr$7;
		if( RES$1 != (char*)0ull) goto label$19;
		{
			HASHADD( INCFILEHASH$1, (char*)FNAME$1, (void*)FNAME$1, INDEX$1 );
		}
		label$19:;
		label$18:;
	}
	label$17:;
	label$16:;
	fb$result$1 = FNAME$1;
	label$13:;
	return fb$result$1;
}

static void HUPDATELANGOPTIONS( void )
{
	label$58:;
	*($11FB_LANG_OPT*)((uint8*)&ENV$ + 1456ll) = *($11FB_LANG_OPT*)(((int64)(struct $12FB_LANG_INFO*)LANGTB$ + (*(int64*)((uint8*)&ENV$ + 272ll) << (4ll & 63ll))) + 8ll);
	label$59:;
}

static void HUPDATETARGETOPTIONS( void )
{
	label$60:;
	__builtin_memcpy( (struct $8FBTARGET*)((uint8*)&ENV$ + 568ll), (struct $8FBTARGET*)((int64)(struct $8FBTARGET*)TARGETINFO$ + (*(int64*)((uint8*)&ENV$ + 216ll) * 40ll)), 40 );
	if( *(int64*)((uint8*)&ENV$ + 544ll) == 0ll) goto label$63;
	{
		*($12FB_TARGETOPT*)((uint8*)&ENV$ + 600ll) = *(int64*)((uint8*)&ENV$ + 600ll) | 32ll;
	}
	label$63:;
	label$62:;
	label$61:;
}

static FBSTRING* HGETTARGETID( int64 OS$1, int64 CPUTYPE$1 )
{
	FBSTRING TMP$175$1;
	FBSTRING TMP$176$1;
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$208:;
	int64 CPUFAMILY$1;
	CPUFAMILY$1 = *(int64*)(((int64)(struct $13FBCPUTYPEINFO*)CPUTYPEINFO$ + (CPUTYPE$1 * 40ll)) + 16ll);
	FBSTRING OSID$1;
	fb_StrInit( (void*)&OSID$1, -1ll, *(void**)((int64)(struct $8FBTARGET*)TARGETINFO$ + (OS$1 * 40ll)), 0ll, 0 );
	{
		if( OS$1 == 0ll) goto label$212;
		label$213:;
		if( OS$1 == 5ll) goto label$212;
		label$214:;
		if( OS$1 != 6ll) goto label$211;
		label$212:;
		{
			if( CPUFAMILY$1 != 0ll) goto label$216;
			{
				fb_StrInit( (void*)&fb$result$1, -1ll, (void*)&OSID$1, -1ll, 0 );
				fb_StrDelete( (FBSTRING*)&OSID$1 );
				goto label$209;
			}
			label$216:;
			label$215:;
		}
		label$211:;
		label$210:;
	}
	if( ((int64)-(OS$1 == 0ll) & (int64)-(CPUFAMILY$1 == 1ll)) == 0ll) goto label$218;
	{
		fb_StrInit( (void*)&fb$result$1, -1ll, (void*)"win64", 6ll, 0 );
		fb_StrDelete( (FBSTRING*)&OSID$1 );
		goto label$209;
	}
	label$218:;
	label$217:;
	__builtin_memset( &TMP$175$1, 0, 24ll );
	FBSTRING* vr$16 = fb_StrConcat( &TMP$175$1, (void*)&OSID$1, -1ll, (void*)"-", 2ll );
	__builtin_memset( &TMP$176$1, 0, 24ll );
	FBSTRING* vr$19 = fb_StrConcat( &TMP$176$1, (void*)vr$16, -1ll, *(void**)((int64)(struct $15FBCPUFAMILYINFO*)CPUFAMILYINFO$ + (CPUFAMILY$1 << (4ll & 63ll))), 0ll );
	fb_StrAssign( (void*)&fb$result$1, -1ll, (void*)vr$19, -1ll, 0 );
	fb_StrDelete( (FBSTRING*)&OSID$1 );
	label$209:;
	FBSTRING* vr$23 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$23;
}

static void FBPARSEPREDEFINES( void )
{
	label$312:;
	FBSTRING DEFID$1;
	__builtin_memset( &DEFID$1, 0, 24ll );
	FBSTRING DEFTEXT$1;
	__builtin_memset( &DEFTEXT$1, 0, 24ll );
	FBSTRING* DEF$1;
	void* vr$3 = LISTGETHEAD( (struct $5TLIST*)&ENV$ );
	DEF$1 = (FBSTRING*)vr$3;
	label$314:;
	if( DEF$1 == (FBSTRING*)0ull) goto label$315;
	{
		int64 IDLENGTH$2;
		FBSTRING* vr$4 = fb_StrAllocTempDescZEx( (char*)"=", 1ll );
		int64 vr$5 = fb_StrInstr( 1ll, (FBSTRING*)DEF$1, (FBSTRING*)vr$4 );
		IDLENGTH$2 = vr$5 + -1ll;
		if( IDLENGTH$2 >= 0ll) goto label$317;
		{
			int64 vr$7 = fb_StrLen( (void*)DEF$1, -1ll );
			IDLENGTH$2 = vr$7;
		}
		label$317:;
		label$316:;
		FBSTRING* vr$8 = fb_LEFT( (FBSTRING*)DEF$1, IDLENGTH$2 );
		fb_StrAssign( (void*)&DEFID$1, -1ll, (void*)vr$8, -1ll, 0 );
		int64 vr$10 = fb_StrLen( (void*)DEF$1, -1ll );
		FBSTRING* vr$13 = fb_RIGHT( (FBSTRING*)DEF$1, (vr$10 - IDLENGTH$2) + -1ll );
		fb_StrAssign( (void*)&DEFTEXT$1, -1ll, (void*)vr$13, -1ll, 0 );
		int64 vr$16 = fb_StrLen( (void*)&DEFTEXT$1, -1ll );
		if( vr$16 != 0ll) goto label$319;
		{
			fb_StrAssign( (void*)&DEFTEXT$1, -1ll, (void*)"1", 2ll, 0 );
		}
		label$319:;
		label$318:;
		int64 vr$19 = fb_StrLen( (void*)&DEFTEXT$1, -1ll );
		SYMBADDDEFINE( (char*)*(char**)&DEFID$1, (char*)*(char**)&DEFTEXT$1, vr$19, 0ll, (tmp$28)0ull, 0ll );
		void* vr$20 = LISTGETNEXT( (void*)DEF$1 );
		DEF$1 = (FBSTRING*)vr$20;
	}
	goto label$314;
	label$315:;
	fb_StrDelete( (FBSTRING*)&DEFTEXT$1 );
	fb_StrDelete( (FBSTRING*)&DEFID$1 );
	label$313:;
}

static void FBPARSEPREINCLUDES( void )
{
	label$320:;
	FBSTRING* FILE$1;
	void* vr$1 = LISTGETHEAD( (struct $5TLIST*)((uint8*)&ENV$ + 64ll) );
	FILE$1 = (FBSTRING*)vr$1;
	label$322:;
	int64 vr$3 = FBSHOULDCONTINUE(  );
	if( ((int64)-(FILE$1 != (FBSTRING*)0ull) & vr$3) == 0ll) goto label$323;
	{
		FBINCLUDEFILE( (char*)*(char**)FILE$1, -1ll );
		void* vr$6 = LISTGETNEXT( (void*)FILE$1 );
		FILE$1 = (FBSTRING*)vr$6;
	}
	goto label$322;
	label$323:;
	label$321:;
}

static void HAPPENDFBCTINF( char* VALUE$1 )
{
	label$324:;
	if( *(int64*)((uint8*)&ENV$ + 1824ll) != 0ll) goto label$327;
	{
		*(int64*)((uint8*)&ENV$ + 1824ll) = -1ll;
		(*(tmp$39*)((uint8*)&IR$ + 424ll))(  );
	}
	label$327:;
	label$326:;
	(*(tmp$69*)((uint8*)&IR$ + 432ll))( VALUE$1 );
	label$325:;
}

static void HEMITOBJINFO( void )
{
	label$328:;
	struct $11TSTRSETITEM* I$1;
	void* vr$1 = LISTGETHEAD( (struct $5TLIST*)((uint8*)&ENV$ + 1648ll) );
	I$1 = (struct $11TSTRSETITEM*)vr$1;
	label$330:;
	if( I$1 == (struct $11TSTRSETITEM*)0ull) goto label$331;
	{
		if( *(int64*)((uint8*)I$1 + 24ll) != 0ll) goto label$333;
		{
			char* vr$3 = OBJINFOENCODE( 0ll );
			HAPPENDFBCTINF( (char*)vr$3 );
			HAPPENDFBCTINF( (char*)*(char**)I$1 );
		}
		label$333:;
		label$332:;
		void* vr$5 = LISTGETNEXT( (void*)I$1 );
		I$1 = (struct $11TSTRSETITEM*)vr$5;
	}
	goto label$330;
	label$331:;
	void* vr$7 = LISTGETHEAD( (struct $5TLIST*)((uint8*)&ENV$ + 1736ll) );
	I$1 = (struct $11TSTRSETITEM*)vr$7;
	label$334:;
	if( I$1 == (struct $11TSTRSETITEM*)0ull) goto label$335;
	{
		if( *(int64*)((uint8*)I$1 + 24ll) != 0ll) goto label$337;
		{
			char* vr$9 = OBJINFOENCODE( 1ll );
			HAPPENDFBCTINF( (char*)vr$9 );
			char* vr$11 = HESCAPE( (char*)*(char**)I$1, 0ll );
			HAPPENDFBCTINF( vr$11 );
		}
		label$337:;
		label$336:;
		void* vr$12 = LISTGETNEXT( (void*)I$1 );
		I$1 = (struct $11TSTRSETITEM*)vr$12;
	}
	goto label$334;
	label$335:;
	if( *(int64*)((uint8*)&ENV$ + 472ll) == 0ll) goto label$339;
	{
		char* vr$13 = OBJINFOENCODE( 2ll );
		HAPPENDFBCTINF( (char*)vr$13 );
	}
	label$339:;
	label$338:;
	if( *(int64*)((uint8*)&ENV$ + 480ll) == 0ll) goto label$341;
	{
		char* vr$14 = OBJINFOENCODE( 3ll );
		HAPPENDFBCTINF( (char*)vr$14 );
	}
	label$341:;
	label$340:;
	if( *(int64*)((uint8*)&ENV$ + 272ll) == 0ll) goto label$343;
	{
		FBSTRING TMP$201$2;
		char* vr$15 = OBJINFOENCODE( 4ll );
		HAPPENDFBCTINF( (char*)vr$15 );
		__builtin_memset( &TMP$201$2, 0, 24ll );
		FBSTRING* vr$17 = FBGETLANGNAME( *($7FB_LANG*)((uint8*)&ENV$ + 272ll) );
		fb_StrAssign( (void*)&TMP$201$2, -1ll, (void*)vr$17, -1ll, 0 );
		HAPPENDFBCTINF( (char*)*(char**)&TMP$201$2 );
		fb_StrDelete( (FBSTRING*)&TMP$201$2 );
	}
	label$343:;
	label$342:;
	if( *(int64*)((uint8*)&ENV$ + 1824ll) == 0ll) goto label$345;
	{
		(*(tmp$39*)((uint8*)&IR$ + 440ll))(  );
	}
	label$345:;
	label$344:;
	label$329:;
}

static void HSHOWINCLUDE( int64 INCLUDELEVEL$1, FBSTRING* MESSAGE$1 )
{
	FBSTRING TMP$204$1;
	label$346:;
	FBSTRING LN$1;
	__builtin_memset( &LN$1, 0, 24ll );
	{
		int64 I$2;
		I$2 = 1ll;
		int64 TMP$202$2;
		TMP$202$2 = INCLUDELEVEL$1;
		goto label$348;
		label$351:;
		{
			fb_StrConcatAssign( (void*)&LN$1, -1ll, (void*)" |  ", 5ll, 0 );
		}
		label$349:;
		I$2 = I$2 + 1ll;
		label$348:;
		if( I$2 <= TMP$202$2) goto label$351;
		label$350:;
	}
	__builtin_memset( &TMP$204$1, 0, 24ll );
	FBSTRING* vr$6 = fb_StrConcat( &TMP$204$1, (void*)&LN$1, -1ll, (void*)MESSAGE$1, -1ll );
	fb_StrAssign( (void*)&LN$1, -1ll, (void*)vr$6, -1ll, 0 );
	fb_PrintString( 0, (FBSTRING*)&LN$1, 1 );
	fb_StrDelete( (FBSTRING*)&LN$1 );
	label$347:;
}

static void HONSKIPPEDFILE( FBSTRING* FILENAME$1 )
{
	label$352:;
	if( *(int64*)((uint8*)&ENV$ + 520ll) == 0ll) goto label$355;
	{
		FBSTRING TMP$207$2;
		FBSTRING TMP$208$2;
		FBSTRING TMP$209$2;
		__builtin_memset( &TMP$209$2, 0, 24ll );
		FBSTRING* vr$1 = PATHSTRIPCURDIR( FILENAME$1 );
		__builtin_memset( &TMP$207$2, 0, 24ll );
		FBSTRING* vr$4 = fb_StrConcat( &TMP$207$2, (void*)"(", 2ll, (void*)vr$1, -1ll );
		__builtin_memset( &TMP$208$2, 0, 24ll );
		FBSTRING* vr$7 = fb_StrConcat( &TMP$208$2, (void*)vr$4, -1ll, (void*)")", 2ll );
		fb_StrAssign( (void*)&TMP$209$2, -1ll, (void*)vr$7, -1ll, 0 );
		HSHOWINCLUDE( *(int64*)((uint8*)&ENV$ + 1304ll) + 1ll, &TMP$209$2 );
		fb_StrDelete( (FBSTRING*)&TMP$209$2 );
	}
	label$355:;
	label$354:;
	label$353:;
}

static int64 IS_ROOTPATH( char** PATH$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$412:;
	fb$result$1 = 0ll;
	if( *PATH$1 != (char*)0ull) goto label$415;
	{
		goto label$413;
	}
	label$415:;
	label$414:;
	fb$result$1 = (int64)-((uint64)(int64)(uint8)*(*PATH$1) == 47ull);
	label$413:;
	return fb$result$1;
}

static int64 GET_ROOTPATH_LEN( char* PATH$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$416:;
	fb$result$1 = 0ll;
	if( (int64)(uint8)*PATH$1 != 0ll) goto label$419;
	{
		goto label$417;
	}
	label$419:;
	label$418:;
	fb$result$1 = 1ll;
	label$417:;
	return fb$result$1;
}

static int64 SOLVE_PATH( char* PATH$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$420:;
	static int64 CIDX$1[131];
	static struct $8FBARRAY1IlE tmp$212$1 = { (int64*)CIDX$1, (int64*)CIDX$1, 1048ll, 8ll, 1ll, 49ll, { { 131ll, 0ll, 130ll } } };
	int64 STK$1;
	int64 S$1;
	int64 N$1;
	int64 D$1;
	int64 R$1;
	int64 W$1;
	int64 C$1;
	STK$1 = 0ll;
	int64 vr$1 = GET_ROOTPATH_LEN( PATH$1 );
	S$1 = vr$1;
	*(int64*)((int64)(int64*)CIDX$1 + (STK$1 << (3ll & 63ll))) = S$1;
	W$1 = S$1;
	N$1 = 0ll;
	D$1 = 0ll;
	{
		R$1 = S$1;
		int64 TMP$213$2;
		int64 vr$3 = fb_StrLen( (void*)PATH$1, 0ll );
		TMP$213$2 = vr$3 + -1ll;
		goto label$422;
		label$425:;
		{
			C$1 = (int64)(uint8)*(char*)((uint8*)PATH$1 + R$1);
			if( ((int64)-((uint64)C$1 == 47ull) | (int64)-((uint64)C$1 == 92ull)) == 0ll) goto label$427;
			{
				if( N$1 != 0ll) goto label$429;
				{
				}
				goto label$428;
				label$429:;
				if( ((int64)-(D$1 == 1ll) & (int64)-(N$1 == 1ll)) == 0ll) goto label$430;
				{
					W$1 = W$1 + -1ll;
				}
				goto label$428;
				label$430:;
				if( ((int64)-(D$1 == 2ll) & (int64)-(N$1 == 2ll)) == 0ll) goto label$431;
				{
					if( STK$1 == 0ll) goto label$433;
					{
						STK$1 = STK$1 + -1ll;
					}
					label$433:;
					label$432:;
					W$1 = *(int64*)((int64)(int64*)CIDX$1 + (STK$1 << (3ll & 63ll)));
				}
				goto label$428;
				label$431:;
				{
					*(char*)((uint8*)PATH$1 + W$1) = (uint8)C$1;
					W$1 = W$1 + 1ll;
					STK$1 = STK$1 + 1ll;
					*(int64*)((int64)(int64*)CIDX$1 + (STK$1 << (3ll & 63ll))) = W$1;
				}
				label$428:;
				N$1 = 0ll;
				D$1 = 0ll;
			}
			goto label$426;
			label$427:;
			{
				if( (uint64)C$1 != 46ull) goto label$435;
				{
					D$1 = D$1 + 1ll;
				}
				label$435:;
				label$434:;
				N$1 = N$1 + 1ll;
				*(char*)((uint8*)PATH$1 + W$1) = (uint8)C$1;
				W$1 = W$1 + 1ll;
			}
			label$426:;
		}
		label$423:;
		R$1 = R$1 + 1ll;
		label$422:;
		if( R$1 <= TMP$213$2) goto label$425;
		label$424:;
	}
	*(char*)((uint8*)PATH$1 + W$1) = (uint8)0u;
	fb$result$1 = -1ll;
	label$421:;
	return fb$result$1;
}
