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
typedef void (*tmp$43)( struct $8FBSYMBOL* );
typedef void (*tmp$51)( char* );
typedef void (*tmp$50)( struct $9ASTASMTOK* );
typedef void (*tmp$61)( int64, struct $8FBSYMBOL*, int64, char* );
typedef int64 $15AST_LINK_RETURN;
typedef struct $6IRVREG* (*tmp$70)( int64, struct $8FBSYMBOL* );
typedef void (*tmp$46)( struct $6IRVREG*, struct $6IRVREG* );
typedef void (*tmp$54)( struct $6IRVREG* );
typedef void (*tmp$53)( int64, struct $6IRVREG*, struct $6IRVREG* );
struct $8NAMEINFO {
	char* NAME;
};
__FB_STATIC_ASSERT( sizeof( struct $8NAMEINFO ) == 8 );
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
void free( void* );
void fb_PrintString( int32, FBSTRING*, int32 );
FBSTRING* fb_StrInit( void*, int64, void*, int64, int32 );
FBSTRING* fb_StrAssign( void*, int64, void*, int64, int32 );
void fb_StrDelete( FBSTRING* );
FBSTRING* fb_StrConcat( FBSTRING*, void*, int64, void*, int64 );
FBSTRING* fb_StrConcatAssign( void*, int64, void*, int64, int32 );
FBSTRING* fb_StrAllocTempResult( FBSTRING* );
FBSTRING* fb_StrAllocTempDescZEx( char*, int64 );
FBSTRING* fb_LongintToStr( int64 );
FBSTRING* fb_DoubleToStr( double );
int64 fb_StrLen( void*, int64 );
FBSTRING* fb_SPACE( int64 );
static void fb_ctor__astznodezmisc( void ) __attribute__(( constructor ));
void* XALLOCATE( int32 );
void* LISTNEWNODE( struct $5TLIST* );
void LISTDELNODE( struct $5TLIST*, void* );
static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM*, struct $11TSTRSETITEM* );
int64 FBIS64BIT( void );
void ASTDELNODE( struct $7ASTNODE* );
struct $7ASTNODE* ASTCLONETREE( struct $7ASTNODE* );
void ASTDELTREE( struct $7ASTNODE* );
typedef int64 $11AST_CONVOPT;
struct $7ASTNODE* ASTNEWCONV( int64, struct $8FBSYMBOL*, struct $7ASTNODE*, $11AST_CONVOPT, int64* );
struct $7ASTNODE* ASTNEWBOP( int64, struct $7ASTNODE*, struct $7ASTNODE*, struct $8FBSYMBOL*, $9AST_OPOPT );
struct $7ASTNODE* ASTNEWUOP( int64, struct $7ASTNODE* );
struct $7ASTNODE* ASTNEWCONSTI( int64, int64, struct $8FBSYMBOL* );
void ASTFORGETBITFIELDS( struct $7ASTNODE* );
struct $7ASTNODE* ASTUPDATEBITFIELDS( struct $7ASTNODE* );
struct $7ASTNODE* ASTNEWNODE( int64, int64, struct $8FBSYMBOL* );
struct $6IRVREG* ASTLOAD( struct $7ASTNODE* );
void ASTSETTYPE( struct $7ASTNODE*, int64, struct $8FBSYMBOL* );
FBSTRING* ASTDUMPOPTOSTR( $6AST_OP );
void ASTDUMPTREE( struct $7ASTNODE*, int64 );
FBSTRING* SYMBTYPETOSTR( int64, struct $8FBSYMBOL*, int64, int64 );
static struct $9ASTASMTOK* ASTASMAPPEND( struct $9ASTASMTOK*, int64 );
static struct $7ASTNODE* _Z13HMAKEUINTMASKml( uint64, int64 );
static struct $7ASTNODE* _Z13HMAKEUINTMASKmml( uint64, uint64, int64 );
static struct $7ASTNODE* ASTSETBITFIELD( struct $8FBSYMBOL*, struct $7ASTNODE*, struct $7ASTNODE* );
static struct $7ASTNODE* ASTACCESSBITFIELD( struct $8FBSYMBOL*, struct $7ASTNODE* );
static void DBG_ASTOUTPUT( FBSTRING*, int64, int64, int64 );
static FBSTRING* HASTNODECLASSTOSTR( $13AST_NODECLASS );
static FBSTRING* HSYMBTOSTR( struct $8FBSYMBOL* );
static FBSTRING* HASTNODETYPETOSTR( struct $7ASTNODE* );
static FBSTRING* HASTNODETOSTR( struct $7ASTNODE* );
static void ASTDUMPTREEEX( struct $7ASTNODE*, int64, int64, int64 );
typedef void (*tmp$39)( void );
typedef int64 (*tmp$40)( void );
typedef int64 $14IR_OPTIONVALUE;
typedef int64 (*tmp$41)( $14IR_OPTIONVALUE );
typedef int64 (*tmp$42)( int64, int64 );
typedef void (*tmp$44)( struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef char* (*tmp$45)( void );
typedef void (*tmp$47)( int64 );
typedef void (*tmp$48)( struct $8FBSYMBOL*, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef void (*tmp$49)( struct $8FBSYMBOL*, struct $6IRVREG*, int64, int64, struct $6IRVREG* );
typedef int64 $10IR_EMITOPT;
typedef void (*tmp$52)( int64, struct $6IRVREG*, struct $6IRVREG*, struct $6IRVREG*, struct $8FBSYMBOL*, $10IR_EMITOPT );
typedef void (*tmp$55)( struct $8FBSYMBOL*, int64, struct $6IRVREG*, int64 );
typedef void (*tmp$56)( struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, int64, int64 );
typedef void (*tmp$57)( int64, struct $8FBSYMBOL* );
typedef void (*tmp$58)( struct $6IRVREG*, struct $8FBSYMBOL*, uint64*, struct $8FBSYMBOL**, int64, struct $8FBSYMBOL*, uint64, uint64 );
typedef void (*tmp$59)( int64, struct $6IRVREG*, struct $6IRVREG*, int64, int64 );
typedef void (*tmp$60)( int64, struct $6IRVREG*, struct $6IRVREG*, struct $6IRVREG* );
typedef void (*tmp$62)( struct $8FBSYMBOL*, int64 );
typedef void (*tmp$63)( struct $8FBSYMBOL*, double );
typedef void (*tmp$64)( struct $8FBSYMBOL*, struct $8FBSYMBOL*, int64 );
typedef void (*tmp$65)( int64, char*, int64, int64 );
typedef void (*tmp$66)( int64, uint32*, int64 );
typedef void (*tmp$67)( int64, int64 );
typedef void (*tmp$68)( struct $8FBSYMBOL*, int64 );
typedef void (*tmp$69)( char* );
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
struct $12AST_PROC_CTX {
	struct $7ASTNODE* HEAD;
	struct $7ASTNODE* TAIL;
	struct $7ASTNODE* CURR;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_PROC_CTX ) == 24 );
struct $12AST_CALL_CTX {
	struct $5TLIST TMPSTRLIST;
};
__FB_STATIC_ASSERT( sizeof( struct $12AST_CALL_CTX ) == 64 );
struct $16AST_GLOBINST_CTX {
	struct $5TLIST LIST;
	int64 CTORCNT;
	int64 DTORCNT;
};
__FB_STATIC_ASSERT( sizeof( struct $16AST_GLOBINST_CTX ) == 80 );
struct $16AST_DATASTMT_CTX {
	struct $8FBSYMBOL* DESC;
	struct $8FBSYMBOL* LASTSYM;
	struct $8FBSYMBOL* FIRSTSYM;
	struct $8FBSYMBOL* LASTLBL;
};
__FB_STATIC_ASSERT( sizeof( struct $16AST_DATASTMT_CTX ) == 32 );
struct $23AST_DTORLIST_SCOPESTACK {
	int64* COOKIES;
	int64 COUNT;
	int64 ROOM;
};
__FB_STATIC_ASSERT( sizeof( struct $23AST_DTORLIST_SCOPESTACK ) == 24 );
struct $6ASTCTX {
	struct $5TLIST ASTTB;
	struct $12AST_PROC_CTX PROC;
	struct $12AST_CALL_CTX CALL;
	struct $16AST_GLOBINST_CTX GLOBINST;
	struct $16AST_DATASTMT_CTX DATA;
	struct $7ASTNODE* CURRBLOCK;
	int64 DOEMIT;
	int64 TYPEINICOUNT;
	int64 BITFIELDCOUNT;
	struct $5TLIST DTORLIST;
	struct $23AST_DTORLIST_SCOPESTACK DTORLISTSCOPES;
	int64 DTORLISTCOOKIES;
	int64 FLUSHDTORLIST;
	struct $5TLIST ASMTOKLIST;
	int64 HIDEWARNINGSLEVEL;
};
__FB_STATIC_ASSERT( sizeof( struct $6ASTCTX ) == 472 );
extern struct $6ASTCTX AST$;
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
static struct $8NAMEINFO DBG_ASTNODECLASSNAMES$[45] = { { (char*)"NOP" }, { (char*)"LOAD" }, { (char*)"ASSIGN" }, { (char*)"BOP" }, { (char*)"UOP" }, { (char*)"CONV" }, { (char*)"ADDROF" }, { (char*)"BRANCH" }, { (char*)"JMPTB" }, { (char*)"CALL" }, { (char*)"CALLCTOR" }, { (char*)"STACK" }, { (char*)"MEM" }, { (char*)"LOOP" }, { (char*)"COMP" }, { (char*)"LINK" }, { (char*)"CONST" }, { (char*)"VAR" }, { (char*)"IDX" }, { (char*)"FIELD" }, { (char*)"DEREF" }, { (char*)"LABEL" }, { (char*)"ARG" }, { (char*)"OFFSET" }, { (char*)"DECL" }, { (char*)"NIDXARRAY" }, { (char*)"IIF" }, { (char*)"LIT" }, { (char*)"ASM" }, { (char*)"DATASTMT" }, { (char*)"DBG" }, { (char*)"BOUNDCHK" }, { (char*)"PTRCHK" }, { (char*)"SCOPEBEGIN" }, { (char*)"SCOPEEND" }, { (char*)"SCOPE_BREAK" }, { (char*)"TYPEINI" }, { (char*)"TYPEINI_PAD" }, { (char*)"TYPEINI_ASSIGN" }, { (char*)"TYPEINI_CTORCALL" }, { (char*)"TYPEINI_CTORLIST" }, { (char*)"TYPEINI_SCOPEINI" }, { (char*)"TYPEINI_SCOPEEND" }, { (char*)"PROC" }, { (char*)"MACRO" } };
static struct $8NAMEINFO DBG_ASTNODEOPNAMES$[122] = { { (char*)"=" }, { (char*)"+=" }, { (char*)"-=" }, { (char*)"*=" }, { (char*)"/=" }, { (char*)"\x5C=" }, { (char*)"MOD=" }, { (char*)"AND=" }, { (char*)"OR=" }, { (char*)"ANDALSO=" }, { (char*)"ORELSE=" }, { (char*)"XOR=" }, { (char*)"EQV=" }, { (char*)"IMP=" }, { (char*)"SHL=" }, { (char*)"SHR=" }, { (char*)"^=" }, { (char*)"&=" }, { (char*)"new=" }, { (char*)"new[]=" }, { (char*)"del=" }, { (char*)"del[]=" }, { (char*)"ADDROF" }, { (char*)"PTRINDEX" }, { (char*)"FOR" }, { (char*)"STEP" }, { (char*)"NEXT" }, { (char*)"CAST" }, { (char*)"+" }, { (char*)"-" }, { (char*)"*" }, { (char*)"/" }, { (char*)"\x5C" }, { (char*)"MOD" }, { (char*)"AND" }, { (char*)"OR" }, { (char*)"ANDALSO" }, { (char*)"ORELSE" }, { (char*)"XOR" }, { (char*)"EQV" }, { (char*)"IMP" }, { (char*)"SHL" }, { (char*)"SHR" }, { (char*)"^" }, { (char*)"&" }, { (char*)"==" }, { (char*)">" }, { (char*)"<" }, { (char*)"<>" }, { (char*)">=" }, { (char*)"<=" }, { (char*)"IS" }, { (char*)"NOT" }, { (char*)"BOOLNOT" }, { (char*)"+" }, { (char*)"NEG" }, { (char*)"HADD" }, { (char*)"ABS" }, { (char*)"SGN" }, { (char*)"SIN" }, { (char*)"ASIN" }, { (char*)"COS" }, { (char*)"ACOS" }, { (char*)"TAN" }, { (char*)"ATAN" }, { (char*)"ATAN2" }, { (char*)"SQRT" }, { (char*)"RSQRT" }, { (char*)"RCP" }, { (char*)"LOG" }, { (char*)"EXP" }, { (char*)"FLOOR" }, { (char*)"FIX" }, { (char*)"FRAC" }, { (char*)"LEN" }, { (char*)"CONVFD2FS" }, { (char*)"SWZREP" }, { (char*)"DEREF" }, { (char*)"FLDDEREF" }, { (char*)"NEW" }, { (char*)"NEW_VEC" }, { (char*)"DEL" }, { (char*)"DEL_VEC" }, { (char*)"TOINT" }, { (char*)"TOFLT" }, { (char*)"TOBOOL" }, { (char*)"LOAD" }, { (char*)"LOADRES" }, { (char*)"SPILLREGS" }, { (char*)"PUSH" }, { (char*)"POP" }, { (char*)"PUSHUDT" }, { (char*)"STACKALIGN" }, { (char*)"JEQ" }, { (char*)"JGT" }, { (char*)"JLT" }, { (char*)"JNE" }, { (char*)"JGE" }, { (char*)"JLE" }, { (char*)"JMP" }, { (char*)"CALL" }, { (char*)"LABEL" }, { (char*)"RET" }, { (char*)"CALLFUNCT" }, { (char*)"CALLPTR" }, { (char*)"JUMPPTR" }, { (char*)"MEMMOVE" }, { (char*)"MEMSWAP" }, { (char*)"MEMFILL" }, { (char*)"STKCLEAR" }, { (char*)"VA_START" }, { (char*)"VA_END" }, { (char*)"VA_COPY" }, { (char*)"VA_ARG" }, { (char*)"DBG_LINEINI" }, { (char*)"DBG_LINEEND" }, { (char*)"DBG_SCOPEINI" }, { (char*)"BDG_SCOPEEND" }, { (char*)"LIT_COMMENT" }, { (char*)"LIT_ASM" }, { (char*)"TOSIGNED" }, { (char*)"TOUNSIGNED" } };

struct $7ASTNODE* ASTNEWLABEL( struct $8FBSYMBOL* SYM$1, int64 DOFLUSH$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$10:;
	struct $7ASTNODE* N$1;
	struct $7ASTNODE* vr$1 = ASTNEWNODE( 21ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	N$1 = vr$1;
	*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) = SYM$1;
	*(int64*)((uint8*)N$1 + 40ll) = DOFLUSH$1;
	if( *(int64*)SYM$1 != 7ll) goto label$13;
	{
		if( *(int64*)((uint8*)SYM$1 + 104ll) != 0ll) goto label$15;
		{
			*(int64*)((uint8*)SYM$1 + 104ll) = -1ll;
			*(int64*)((uint8*)SYM$1 + 112ll) = *(int64*)((uint8*)&PARSER$ + 56ll);
			*(struct $8FBSYMBOL**)((uint8*)SYM$1 + 96ll) = *(struct $8FBSYMBOL**)((uint8*)&PARSER$ + 224ll);
		}
		label$15:;
		label$14:;
	}
	label$13:;
	label$12:;
	fb$result$1 = N$1;
	label$11:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADLABEL( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$16:;
	if( *(int64*)((uint8*)&AST$ + 272ll) == 0ll) goto label$19;
	{
		if( *(int64*)((uint8*)N$1 + 40ll) == 0ll) goto label$21;
		{
			(*(tmp$43*)((uint8*)&IR$ + 120ll))( *(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) );
		}
		goto label$20;
		label$21:;
		{
			(*(tmp$43*)((uint8*)&IR$ + 128ll))( *(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) );
		}
		label$20:;
	}
	label$19:;
	label$18:;
	fb$result$1 = (struct $6IRVREG*)0ull;
	label$17:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWLIT( char* TEXT$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$22:;
	struct $7ASTNODE* N$1;
	struct $7ASTNODE* vr$1 = ASTNEWNODE( 27ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	N$1 = vr$1;
	int64 vr$2 = fb_StrLen( (void*)TEXT$1, 0ll );
	void* vr$5 = XALLOCATE( (int32)(vr$2 + 1ll) );
	*(char**)((uint8*)N$1 + 40ll) = (char*)vr$5;
	fb_StrAssign( *(void**)((uint8*)N$1 + 40ll), 0ll, (void*)TEXT$1, 0ll, 0 );
	fb$result$1 = N$1;
	label$23:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADLIT( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$24:;
	if( *(int64*)((uint8*)&AST$ + 272ll) == 0ll) goto label$27;
	{
		(*(tmp$51*)((uint8*)&IR$ + 176ll))( *(char**)((uint8*)N$1 + 40ll) );
	}
	label$27:;
	label$26:;
	if( *(char**)((uint8*)N$1 + 40ll) == (char*)0ull) goto label$29;
	{
		free( *(void**)((uint8*)N$1 + 40ll) );
	}
	label$29:;
	label$28:;
	fb$result$1 = (struct $6IRVREG*)0ull;
	label$25:;
	return fb$result$1;
}

struct $9ASTASMTOK* ASTASMAPPENDTEXT( struct $9ASTASMTOK* TAIL$1, char* TEXT$1 )
{
	struct $9ASTASMTOK* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$34:;
	struct $9ASTASMTOK* vr$1 = ASTASMAPPEND( TAIL$1, 0ll );
	TAIL$1 = vr$1;
	int64 vr$2 = fb_StrLen( (void*)TEXT$1, 0ll );
	void* vr$5 = XALLOCATE( (int32)(vr$2 + 1ll) );
	*(char**)((uint8*)TAIL$1 + 8ll) = (char*)vr$5;
	fb_StrAssign( *(void**)((uint8*)TAIL$1 + 8ll), 0ll, (void*)TEXT$1, 0ll, 0 );
	fb$result$1 = TAIL$1;
	label$35:;
	return fb$result$1;
}

struct $9ASTASMTOK* ASTASMAPPENDSYMB( struct $9ASTASMTOK* TAIL$1, struct $8FBSYMBOL* SYM$1 )
{
	struct $9ASTASMTOK* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$36:;
	struct $9ASTASMTOK* vr$1 = ASTASMAPPEND( TAIL$1, 1ll );
	TAIL$1 = vr$1;
	*(struct $8FBSYMBOL**)((uint8*)TAIL$1 + 8ll) = SYM$1;
	fb$result$1 = TAIL$1;
	label$37:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWASM( struct $9ASTASMTOK* ASMTOKHEAD$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$38:;
	struct $7ASTNODE* N$1;
	struct $7ASTNODE* vr$1 = ASTNEWNODE( 28ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	N$1 = vr$1;
	*(struct $9ASTASMTOK**)((uint8*)N$1 + 40ll) = ASMTOKHEAD$1;
	fb$result$1 = N$1;
	label$39:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADASM( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$40:;
	if( *(int64*)((uint8*)&AST$ + 272ll) == 0ll) goto label$43;
	{
		(*(tmp$50*)((uint8*)&IR$ + 168ll))( *(struct $9ASTASMTOK**)((uint8*)N$1 + 40ll) );
	}
	label$43:;
	label$42:;
	struct $9ASTASMTOK* NODE$1;
	NODE$1 = *(struct $9ASTASMTOK**)((uint8*)N$1 + 40ll);
	label$44:;
	if( NODE$1 == (struct $9ASTASMTOK*)0ull) goto label$45;
	{
		struct $9ASTASMTOK* NXT$2;
		NXT$2 = *(struct $9ASTASMTOK**)((uint8*)NODE$1 + 16ll);
		{
			$14AST_ASMTOKTYPE TMP$95$3;
			TMP$95$3 = *($14AST_ASMTOKTYPE*)NODE$1;
			if( TMP$95$3 != 0ll) goto label$47;
			label$48:;
			{
				if( *(char**)((uint8*)NODE$1 + 8ll) == (char*)0ull) goto label$50;
				{
					free( *(void**)((uint8*)NODE$1 + 8ll) );
				}
				label$50:;
				label$49:;
			}
			label$47:;
			label$46:;
		}
		LISTDELNODE( (struct $5TLIST*)((uint8*)&AST$ + 400ll), (void*)NODE$1 );
		NODE$1 = NXT$2;
	}
	goto label$44;
	label$45:;
	fb$result$1 = (struct $6IRVREG*)0ull;
	label$41:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWDBG( int64 OP$1, int64 EX$1, char* FILENAME$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$51:;
	struct $7ASTNODE* N$1;
	if( *(int64*)((uint8*)&ENV$ + 296ll) != 0ll) goto label$54;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$52;
	}
	label$54:;
	label$53:;
	struct $7ASTNODE* vr$1 = ASTNEWNODE( 30ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	N$1 = vr$1;
	*(int64*)((uint8*)N$1 + 56ll) = OP$1;
	*(int64*)((uint8*)N$1 + 40ll) = EX$1;
	*(char**)((uint8*)N$1 + 48ll) = FILENAME$1;
	fb$result$1 = N$1;
	label$52:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADDBG( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$55:;
	if( *(int64*)((uint8*)&AST$ + 272ll) == 0ll) goto label$58;
	{
		(*(tmp$61*)((uint8*)&IR$ + 336ll))( *(int64*)((uint8*)N$1 + 56ll), *(struct $8FBSYMBOL**)((uint8*)*(struct $7ASTNODE**)((uint8*)&AST$ + 80ll) + 24ll), *(int64*)((uint8*)N$1 + 40ll), *(char**)((uint8*)N$1 + 48ll) );
	}
	label$58:;
	label$57:;
	fb$result$1 = (struct $6IRVREG*)0ull;
	label$56:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWNOP( void )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$59:;
	struct $7ASTNODE* N$1;
	struct $7ASTNODE* vr$1 = ASTNEWNODE( 0ll, 2147483648ll, (struct $8FBSYMBOL*)0ull );
	N$1 = vr$1;
	fb$result$1 = N$1;
	label$60:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADNOP( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$61:;
	fb$result$1 = (struct $6IRVREG*)0ull;
	label$62:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWNIDXARRAY( struct $7ASTNODE* EXPR$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$63:;
	struct $7ASTNODE* N$1;
	struct $7ASTNODE* vr$3 = ASTNEWNODE( 25ll, *(int64*)((uint8*)EXPR$1 + 8ll), *(struct $8FBSYMBOL**)((uint8*)EXPR$1 + 16ll) );
	N$1 = vr$3;
	*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) = *(struct $8FBSYMBOL**)((uint8*)EXPR$1 + 24ll);
	*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = EXPR$1;
	fb$result$1 = N$1;
	label$64:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADNIDXARRAY( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$65:;
	ASTDELTREE( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	fb$result$1 = (struct $6IRVREG*)0ull;
	label$66:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWLINK( struct $7ASTNODE* L$1, struct $7ASTNODE* R$1, $15AST_LINK_RETURN RET$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$67:;
	struct $7ASTNODE* N$1;
	if( RET$1 == 1ll) goto label$70;
	{
		if( L$1 == (struct $7ASTNODE*)0ull) goto label$72;
		{
			if( *(int64*)L$1 != 9ll) goto label$74;
			{
				ASTSETTYPE( L$1, 0ll, (struct $8FBSYMBOL*)0ull );
			}
			label$74:;
			label$73:;
		}
		label$72:;
		label$71:;
	}
	label$70:;
	label$69:;
	if( RET$1 == 2ll) goto label$76;
	{
		if( R$1 == (struct $7ASTNODE*)0ull) goto label$78;
		{
			if( *(int64*)R$1 != 9ll) goto label$80;
			{
				ASTSETTYPE( R$1, 0ll, (struct $8FBSYMBOL*)0ull );
			}
			label$80:;
			label$79:;
		}
		label$78:;
		label$77:;
	}
	label$76:;
	label$75:;
	if( L$1 != (struct $7ASTNODE*)0ull) goto label$82;
	{
		fb$result$1 = R$1;
		goto label$68;
	}
	label$82:;
	label$81:;
	if( R$1 != (struct $7ASTNODE*)0ull) goto label$84;
	{
		fb$result$1 = L$1;
		goto label$68;
	}
	label$84:;
	label$83:;
	{
		if( RET$1 != 0ll) goto label$86;
		label$87:;
		{
			struct $7ASTNODE* vr$3 = ASTNEWNODE( 15ll, 0ll, (struct $8FBSYMBOL*)0ull );
			N$1 = vr$3;
		}
		goto label$85;
		label$86:;
		if( RET$1 != 1ll) goto label$88;
		label$89:;
		{
			struct $7ASTNODE* vr$6 = ASTNEWNODE( 15ll, *(int64*)((uint8*)L$1 + 8ll), *(struct $8FBSYMBOL**)((uint8*)L$1 + 16ll) );
			N$1 = vr$6;
		}
		goto label$85;
		label$88:;
		if( RET$1 != 2ll) goto label$90;
		label$91:;
		{
			struct $7ASTNODE* vr$9 = ASTNEWNODE( 15ll, *(int64*)((uint8*)R$1 + 8ll), *(struct $8FBSYMBOL**)((uint8*)R$1 + 16ll) );
			N$1 = vr$9;
		}
		label$90:;
		label$85:;
	}
	*(int64*)((uint8*)N$1 + 40ll) = RET$1;
	*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = L$1;
	*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = R$1;
	fb$result$1 = N$1;
	label$68:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADLINK( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$92:;
	struct $6IRVREG* VRL$1;
	struct $6IRVREG* VRR$1;
	struct $6IRVREG* vr$2 = ASTLOAD( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	VRL$1 = vr$2;
	ASTDELNODE( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	struct $6IRVREG* vr$5 = ASTLOAD( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
	VRR$1 = vr$5;
	ASTDELNODE( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
	{
		int64 TMP$96$2;
		TMP$96$2 = *(int64*)((uint8*)N$1 + 40ll);
		if( TMP$96$2 != 1ll) goto label$95;
		label$96:;
		{
			fb$result$1 = VRL$1;
		}
		goto label$94;
		label$95:;
		if( TMP$96$2 != 2ll) goto label$97;
		label$98:;
		{
			fb$result$1 = VRR$1;
		}
		goto label$94;
		label$97:;
		{
			fb$result$1 = (struct $6IRVREG*)0ull;
		}
		label$99:;
		label$94:;
	}
	label$93:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWLOAD( struct $7ASTNODE* L$1, int64 DTYPE$1, int64 ISRESULT$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$100:;
	struct $7ASTNODE* N$1;
	struct $7ASTNODE* vr$1 = ASTNEWNODE( 1ll, DTYPE$1, (struct $8FBSYMBOL*)0ull );
	N$1 = vr$1;
	*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = L$1;
	*(int64*)((uint8*)N$1 + 40ll) = ISRESULT$1;
	fb$result$1 = N$1;
	label$101:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADLOAD( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$102:;
	struct $7ASTNODE* L$1;
	struct $6IRVREG* V1$1;
	struct $6IRVREG* VR$1;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 != (struct $7ASTNODE*)0ull) goto label$105;
	{
		fb$result$1 = (struct $6IRVREG*)0ull;
		goto label$103;
	}
	label$105:;
	label$104:;
	struct $6IRVREG* vr$2 = ASTLOAD( L$1 );
	V1$1 = vr$2;
	if( *(int64*)((uint8*)&AST$ + 272ll) == 0ll) goto label$107;
	{
		if( *(int64*)((uint8*)N$1 + 40ll) == 0ll) goto label$109;
		{
			struct $6IRVREG* vr$6 = (*(tmp$70*)((uint8*)&IR$ + 448ll))( *(int64*)((uint8*)V1$1 + 8ll), *(struct $8FBSYMBOL**)((uint8*)V1$1 + 16ll) );
			VR$1 = vr$6;
			(*(tmp$46*)((uint8*)&IR$ + 224ll))( V1$1, VR$1 );
		}
		goto label$108;
		label$109:;
		{
			(*(tmp$54*)((uint8*)&IR$ + 216ll))( V1$1 );
		}
		label$108:;
	}
	label$107:;
	label$106:;
	ASTDELNODE( L$1 );
	fb$result$1 = V1$1;
	label$103:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWFIELD( struct $7ASTNODE* L$1, struct $8FBSYMBOL* SYM$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$110:;
	struct $7ASTNODE* N$1;
	int64 DTYPE$1;
	struct $8FBSYMBOL* SUBTYPE$1;
	DTYPE$1 = *(int64*)((uint8*)L$1 + 8ll);
	SUBTYPE$1 = *(struct $8FBSYMBOL**)((uint8*)L$1 + 16ll);
	if( *(int64*)((uint8*)SYM$1 + 192ll) <= 0ll) goto label$113;
	{
		if( (DTYPE$1 & 511ll) != 1ll) goto label$115;
		{
			{
				int64 TMP$97$4;
				TMP$97$4 = *(int64*)((uint8*)SYM$1 + 80ll);
				if( TMP$97$4 != 1ll) goto label$117;
				label$118:;
				{
					DTYPE$1 = (DTYPE$1 & -512ll) | 2ll;
				}
				goto label$116;
				label$117:;
				{
					DTYPE$1 = (DTYPE$1 & -512ll) | 8ll;
				}
				label$119:;
				label$116:;
			}
		}
		goto label$114;
		label$115:;
		{
			DTYPE$1 = (DTYPE$1 & -512ll) | 9ll;
		}
		label$114:;
		SUBTYPE$1 = (struct $8FBSYMBOL*)0ull;
		*(int64*)((uint8*)&AST$ + 288ll) = *(int64*)((uint8*)&AST$ + 288ll) + 1ll;
	}
	label$113:;
	label$112:;
	if( *(int64*)L$1 != 19ll) goto label$121;
	{
		*(struct $8FBSYMBOL**)((uint8*)L$1 + 24ll) = SYM$1;
		*(int64*)((uint8*)L$1 + 8ll) = DTYPE$1;
		*(struct $8FBSYMBOL**)((uint8*)L$1 + 16ll) = SUBTYPE$1;
		fb$result$1 = L$1;
		goto label$111;
	}
	label$121:;
	label$120:;
	struct $7ASTNODE* vr$17 = ASTNEWNODE( 19ll, DTYPE$1, SUBTYPE$1 );
	N$1 = vr$17;
	*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) = SYM$1;
	*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = L$1;
	fb$result$1 = N$1;
	label$111:;
	return fb$result$1;
}

void ASTFORGETBITFIELDS( struct $7ASTNODE* N$1 )
{
	int64 TMP$98$1;
	label$122:;
	if( ((int64)-(N$1 == (struct $7ASTNODE*)0ull) | (int64)-(*(int64*)((uint8*)&AST$ + 288ll) <= 0ll)) == 0ll) goto label$125;
	{
		goto label$123;
	}
	label$125:;
	label$124:;
	if( *(int64*)N$1 != 19ll) goto label$126;
	TMP$98$1 = (int64)-(*(int64*)((uint8*)*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) + 192ll) > 0ll);
	goto label$129;
	label$126:;
	TMP$98$1 = 0ll;
	label$129:;
	if( TMP$98$1 == 0ll) goto label$128;
	{
		*(int64*)((uint8*)&AST$ + 288ll) = *(int64*)((uint8*)&AST$ + 288ll) + -1ll;
	}
	label$128:;
	label$127:;
	ASTFORGETBITFIELDS( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	ASTFORGETBITFIELDS( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
	label$123:;
}

struct $7ASTNODE* ASTUPDATEBITFIELDS( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$157:;
	if( *(int64*)((uint8*)&AST$ + 288ll) > 0ll) goto label$160;
	{
		fb$result$1 = N$1;
		goto label$158;
	}
	label$160:;
	label$159:;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$162;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$158;
	}
	label$162:;
	label$161:;
	{
		$13AST_NODECLASS TMP$100$2;
		TMP$100$2 = *($13AST_NODECLASS*)N$1;
		if( TMP$100$2 != 2ll) goto label$164;
		label$165:;
		{
			if( *(int64*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) != 19ll) goto label$167;
			{
				struct $8FBSYMBOL* BITFIELD$4;
				BITFIELD$4 = *(struct $8FBSYMBOL**)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 24ll);
				if( *(int64*)((uint8*)BITFIELD$4 + 192ll) <= 0ll) goto label$169;
				{
					*(int64*)((uint8*)&AST$ + 288ll) = *(int64*)((uint8*)&AST$ + 288ll) + -1ll;
					ASTDELNODE( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
					*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = *(struct $7ASTNODE**)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 112ll);
					struct $7ASTNODE* vr$14 = ASTSETBITFIELD( BITFIELD$4, *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
					*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$14;
				}
				label$169:;
				label$168:;
			}
			label$167:;
			label$166:;
		}
		goto label$163;
		label$164:;
		if( TMP$100$2 != 19ll) goto label$170;
		label$171:;
		{
			if( *(int64*)((uint8*)*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) + 192ll) <= 0ll) goto label$173;
			{
				struct $7ASTNODE* L$4;
				L$4 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
				struct $7ASTNODE* vr$20 = ASTACCESSBITFIELD( *(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll), L$4 );
				L$4 = vr$20;
				*(int64*)((uint8*)&AST$ + 288ll) = *(int64*)((uint8*)&AST$ + 288ll) + -1ll;
				ASTDELNODE( N$1 );
				N$1 = L$4;
				struct $7ASTNODE* vr$22 = ASTUPDATEBITFIELDS( N$1 );
				fb$result$1 = vr$22;
				goto label$158;
			}
			label$173:;
			label$172:;
		}
		label$170:;
		label$163:;
	}
	struct $7ASTNODE* vr$24 = ASTUPDATEBITFIELDS( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$24;
	struct $7ASTNODE* vr$27 = ASTUPDATEBITFIELDS( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
	*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$27;
	fb$result$1 = N$1;
	label$158:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADFIELD( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$174:;
	struct $6IRVREG* VR$1;
	struct $6IRVREG* vr$2 = ASTLOAD( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	VR$1 = vr$2;
	ASTDELNODE( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	if( *(int64*)((uint8*)&AST$ + 272ll) == 0ll) goto label$177;
	{
		*(int64*)((uint8*)VR$1 + 40ll) = *(int64*)((uint8*)N$1 + 32ll);
	}
	label$177:;
	label$176:;
	fb$result$1 = VR$1;
	label$175:;
	return fb$result$1;
}

struct $7ASTNODE* ASTNEWSTACK( int64 OP$1, struct $7ASTNODE* L$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$178:;
	struct $7ASTNODE* N$1;
	if( L$1 != (struct $7ASTNODE*)0ull) goto label$181;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$179;
	}
	label$181:;
	label$180:;
	struct $7ASTNODE* vr$2 = ASTNEWNODE( 11ll, *(int64*)((uint8*)L$1 + 8ll), (struct $8FBSYMBOL*)0ull );
	N$1 = vr$2;
	*(int64*)((uint8*)N$1 + 40ll) = OP$1;
	*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = L$1;
	fb$result$1 = N$1;
	label$179:;
	return fb$result$1;
}

struct $6IRVREG* ASTLOADSTACK( struct $7ASTNODE* N$1 )
{
	struct $6IRVREG* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$182:;
	struct $7ASTNODE* L$1;
	struct $6IRVREG* VR$1;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 != (struct $7ASTNODE*)0ull) goto label$185;
	{
		fb$result$1 = (struct $6IRVREG*)0ull;
		goto label$183;
	}
	label$185:;
	label$184:;
	struct $6IRVREG* vr$2 = ASTLOAD( L$1 );
	VR$1 = vr$2;
	if( *(int64*)((uint8*)&AST$ + 272ll) == 0ll) goto label$187;
	{
		(*(tmp$53*)((uint8*)&IR$ + 232ll))( *(int64*)((uint8*)N$1 + 40ll), VR$1, (struct $6IRVREG*)0ull );
	}
	label$187:;
	label$186:;
	ASTDELNODE( L$1 );
	fb$result$1 = VR$1;
	label$183:;
	return fb$result$1;
}

FBSTRING* ASTDUMPOPTOSTR( $6AST_OP OP$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$198:;
	if( ((int64)-(OP$1 > 121ll) | (int64)-(OP$1 < 0ll)) == 0ll) goto label$201;
	{
		FBSTRING TMP$264$2;
		FBSTRING* vr$4 = fb_LongintToStr( OP$1 );
		__builtin_memset( &TMP$264$2, 0, 24ll );
		FBSTRING* vr$7 = fb_StrConcat( &TMP$264$2, (void*)"OP:", 4ll, (void*)vr$4, -1ll );
		fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$7, -1ll, 0 );
		goto label$199;
	}
	label$201:;
	label$200:;
	fb_StrInit( (void*)&fb$result$1, -1ll, *(void**)((int64)(struct $8NAMEINFO*)DBG_ASTNODEOPNAMES$ + (OP$1 << (3ll & 63ll))), 0ll, 0 );
	goto label$199;
	label$199:;
	FBSTRING* vr$12 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$12;
}

void ASTDUMPTREE( struct $7ASTNODE* N$1, int64 COL$1 )
{
	label$271:;
	ASTDUMPTREEEX( N$1, COL$1, -1ll, 0ll );
	label$272:;
}

void ASTDUMPLIST( struct $7ASTNODE* N$1, int64 COL$1 )
{
	label$273:;
	label$275:;
	if( N$1 == (struct $7ASTNODE*)0ull) goto label$276;
	{
		ASTDUMPTREE( N$1, COL$1 );
		N$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 136ll);
	}
	goto label$275;
	label$276:;
	label$274:;
}

__attribute__(( constructor )) static void fb_ctor__astznodezmisc( void )
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

static struct $9ASTASMTOK* ASTASMAPPEND( struct $9ASTASMTOK* TAIL$1, int64 TYP$1 )
{
	struct $9ASTASMTOK* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$30:;
	struct $9ASTASMTOK* ASMTOK$1;
	void* vr$2 = LISTNEWNODE( (struct $5TLIST*)((uint8*)&AST$ + 400ll) );
	ASMTOK$1 = (struct $9ASTASMTOK*)vr$2;
	if( TAIL$1 == (struct $9ASTASMTOK*)0ull) goto label$33;
	{
		*(struct $9ASTASMTOK**)((uint8*)TAIL$1 + 16ll) = ASMTOK$1;
	}
	label$33:;
	label$32:;
	*($14AST_ASMTOKTYPE*)ASMTOK$1 = TYP$1;
	*(struct $9ASTASMTOK**)((uint8*)ASMTOK$1 + 16ll) = (struct $9ASTASMTOK*)0ull;
	fb$result$1 = ASMTOK$1;
	label$31:;
	return fb$result$1;
}

static struct $7ASTNODE* _Z13HMAKEUINTMASKml( uint64 BITS$1, int64 DTYPE$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$130:;
	uint64 MASK$1;
	__builtin_memset( &MASK$1, 0, 8ll );
	if( BITS$1 < 64ull) goto label$133;
	{
		MASK$1 = 18446744073709551615ull;
	}
	goto label$132;
	label$133:;
	{
		MASK$1 = (1ull << (BITS$1 & 63ll)) + 18446744073709551615ull;
	}
	label$132:;
	int64 vr$4 = FBIS64BIT(  );
	if( ~vr$4 == 0ll) goto label$135;
	{
		MASK$1 = (uint64)(uint32)MASK$1;
	}
	label$135:;
	label$134:;
	struct $7ASTNODE* vr$8 = ASTNEWCONSTI( (int64)MASK$1, DTYPE$1, (struct $8FBSYMBOL*)0ull );
	fb$result$1 = vr$8;
	goto label$131;
	label$131:;
	return fb$result$1;
}

static struct $7ASTNODE* _Z13HMAKEUINTMASKmml( uint64 BITS$1, uint64 BITPOS$1, int64 DTYPE$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$136:;
	struct $7ASTNODE* vr$1 = ASTNEWCONSTI( (int64)BITPOS$1, 8ll, (struct $8FBSYMBOL*)0ull );
	struct $7ASTNODE* vr$2 = _Z13HMAKEUINTMASKml( BITS$1, DTYPE$1 );
	struct $7ASTNODE* vr$3 = ASTNEWBOP( 41ll, vr$2, vr$1, (struct $8FBSYMBOL*)0ull, 1ll );
	fb$result$1 = vr$3;
	goto label$137;
	label$137:;
	return fb$result$1;
}

static struct $7ASTNODE* ASTSETBITFIELD( struct $8FBSYMBOL* BITFIELD$1, struct $7ASTNODE* L$1, struct $7ASTNODE* R$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$138:;
	if( (*(int64*)((uint8*)BITFIELD$1 + 56ll) & 511ll) != 1ll) goto label$141;
	{
		*(int64*)((uint8*)L$1 + 8ll) = (*(int64*)((uint8*)BITFIELD$1 + 56ll) & -512ll) | 3ll;
		*(struct $8FBSYMBOL**)((uint8*)L$1 + 16ll) = (struct $8FBSYMBOL*)0ull;
	}
	goto label$140;
	label$141:;
	{
		*(int64*)((uint8*)L$1 + 8ll) = *(int64*)((uint8*)BITFIELD$1 + 56ll);
		*(struct $8FBSYMBOL**)((uint8*)L$1 + 16ll) = *(struct $8FBSYMBOL**)((uint8*)BITFIELD$1 + 64ll);
	}
	label$140:;
	struct $7ASTNODE* vr$12 = ASTCLONETREE( L$1 );
	L$1 = vr$12;
	struct $7ASTNODE* vr$16 = _Z13HMAKEUINTMASKmml( *(uint64*)((uint8*)BITFIELD$1 + 192ll), *(uint64*)((uint8*)BITFIELD$1 + 184ll), *(int64*)((uint8*)L$1 + 8ll) );
	struct $7ASTNODE* vr$17 = ASTNEWUOP( 52ll, vr$16 );
	struct $7ASTNODE* vr$18 = ASTNEWBOP( 34ll, L$1, vr$17, (struct $8FBSYMBOL*)0ull, 1ll );
	L$1 = vr$18;
	if( (*(int64*)((uint8*)BITFIELD$1 + 56ll) & 511ll) != 1ll) goto label$143;
	{
		int64 TMP$99$2;
		if( *(int64*)R$1 != 5ll) goto label$144;
		TMP$99$2 = (int64)-(*(int64*)((uint8*)R$1 + 8ll) != 1ll);
		goto label$277;
		label$144:;
		TMP$99$2 = -1ll;
		label$277:;
		if( TMP$99$2 == 0ll) goto label$146;
		{
			struct $7ASTNODE* vr$24 = ASTNEWCONV( 1ll, (struct $8FBSYMBOL*)0ull, R$1, 0ll, (int64*)0ull );
			R$1 = vr$24;
		}
		label$146:;
		label$145:;
		struct $7ASTNODE* vr$25 = ASTNEWCONV( 3ll, (struct $8FBSYMBOL*)0ull, R$1, 0ll, (int64*)0ull );
		R$1 = vr$25;
		struct $7ASTNODE* vr$29 = _Z13HMAKEUINTMASKmml( *(uint64*)((uint8*)BITFIELD$1 + 192ll), *(uint64*)((uint8*)BITFIELD$1 + 184ll), *(int64*)((uint8*)R$1 + 8ll) );
		struct $7ASTNODE* vr$30 = ASTNEWBOP( 34ll, R$1, vr$29, (struct $8FBSYMBOL*)0ull, 1ll );
		R$1 = vr$30;
	}
	goto label$142;
	label$143:;
	{
		struct $7ASTNODE* vr$32 = _Z13HMAKEUINTMASKml( *(uint64*)((uint8*)BITFIELD$1 + 192ll), 9ll );
		struct $7ASTNODE* vr$33 = ASTNEWBOP( 34ll, R$1, vr$32, (struct $8FBSYMBOL*)0ull, 1ll );
		R$1 = vr$33;
		if( *(int64*)((uint8*)BITFIELD$1 + 184ll) <= 0ll) goto label$148;
		{
			struct $7ASTNODE* vr$36 = ASTNEWCONSTI( *(int64*)((uint8*)BITFIELD$1 + 184ll), 9ll, (struct $8FBSYMBOL*)0ull );
			struct $7ASTNODE* vr$37 = ASTNEWBOP( 41ll, R$1, vr$36, (struct $8FBSYMBOL*)0ull, 1ll );
			R$1 = vr$37;
		}
		label$148:;
		label$147:;
	}
	label$142:;
	struct $7ASTNODE* vr$38 = ASTNEWBOP( 35ll, L$1, R$1, (struct $8FBSYMBOL*)0ull, 1ll );
	fb$result$1 = vr$38;
	label$139:;
	return fb$result$1;
}

static struct $7ASTNODE* ASTACCESSBITFIELD( struct $8FBSYMBOL* BITFIELD$1, struct $7ASTNODE* L$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$149:;
	int64 BOOLCONV$1;
	__builtin_memset( &BOOLCONV$1, 0, 8ll );
	if( (*(int64*)((uint8*)BITFIELD$1 + 56ll) & 511ll) != 1ll) goto label$152;
	{
		*(int64*)((uint8*)L$1 + 8ll) = (*(int64*)((uint8*)L$1 + 8ll) & -512ll) | 2ll;
		*(struct $8FBSYMBOL**)((uint8*)L$1 + 16ll) = (struct $8FBSYMBOL*)0ull;
		BOOLCONV$1 = -1ll;
	}
	goto label$151;
	label$152:;
	{
		*(int64*)((uint8*)L$1 + 8ll) = (*(int64*)((uint8*)L$1 + 8ll) & -512ll) | (*(int64*)((uint8*)BITFIELD$1 + 56ll) & 511ll);
		*(struct $8FBSYMBOL**)((uint8*)L$1 + 16ll) = *(struct $8FBSYMBOL**)((uint8*)BITFIELD$1 + 64ll);
		BOOLCONV$1 = 0ll;
	}
	label$151:;
	if( *(int64*)((uint8*)BITFIELD$1 + 184ll) <= 0ll) goto label$154;
	{
		struct $7ASTNODE* vr$19 = ASTNEWCONSTI( *(int64*)((uint8*)BITFIELD$1 + 184ll), 8ll, (struct $8FBSYMBOL*)0ull );
		struct $7ASTNODE* vr$20 = ASTNEWBOP( 42ll, L$1, vr$19, (struct $8FBSYMBOL*)0ull, 1ll );
		L$1 = vr$20;
	}
	label$154:;
	label$153:;
	struct $7ASTNODE* vr$22 = _Z13HMAKEUINTMASKml( *(uint64*)((uint8*)BITFIELD$1 + 192ll), 9ll );
	struct $7ASTNODE* vr$23 = ASTNEWBOP( 34ll, L$1, vr$22, (struct $8FBSYMBOL*)0ull, 1ll );
	L$1 = vr$23;
	if( BOOLCONV$1 == 0ll) goto label$156;
	{
		*(int64*)((uint8*)L$1 + 8ll) = (*(int64*)((uint8*)L$1 + 8ll) & -512ll) | (*(int64*)((uint8*)BITFIELD$1 + 56ll) & 511ll);
		*(struct $8FBSYMBOL**)((uint8*)L$1 + 16ll) = *(struct $8FBSYMBOL**)((uint8*)BITFIELD$1 + 64ll);
		struct $7ASTNODE* vr$32 = ASTNEWCONV( 8ll, (struct $8FBSYMBOL*)0ull, L$1, 0ll, (int64*)0ull );
		L$1 = vr$32;
	}
	label$156:;
	label$155:;
	fb$result$1 = L$1;
	label$150:;
	return fb$result$1;
}

static void DBG_ASTOUTPUT( FBSTRING* S$1, int64 COL$1, int64 JUST$1, int64 DEPTH$1 )
{
	label$188:;
	int64 PAD$1;
	{
		if( JUST$1 != -1ll) goto label$191;
		label$192:;
		{
			int64 vr$0 = fb_StrLen( (void*)S$1, -1ll );
			PAD$1 = COL$1 - vr$0;
		}
		goto label$190;
		label$191:;
		if( JUST$1 != 1ll) goto label$193;
		label$194:;
		{
			PAD$1 = COL$1 + -1ll;
		}
		goto label$190;
		label$193:;
		{
			PAD$1 = COL$1;
		}
		label$195:;
		label$190:;
	}
	if( DEPTH$1 >= 0ll) goto label$197;
	{
		FBSTRING* vr$4 = fb_SPACE( PAD$1 + -1ll );
		fb_PrintString( 0, (FBSTRING*)vr$4, 0 );
		fb_PrintString( 0, (FBSTRING*)S$1, 1 );
	}
	goto label$196;
	label$197:;
	{
		FBSTRING* vr$5 = fb_LongintToStr( DEPTH$1 );
		fb_PrintString( 0, (FBSTRING*)vr$5, 0 );
		FBSTRING* vr$7 = fb_LongintToStr( DEPTH$1 );
		int64 vr$8 = fb_StrLen( (void*)vr$7, -1ll );
		FBSTRING* vr$10 = fb_SPACE( (PAD$1 + -1ll) - vr$8 );
		fb_PrintString( 0, (FBSTRING*)vr$10, 0 );
		fb_PrintString( 0, (FBSTRING*)S$1, 1 );
	}
	label$196:;
	label$189:;
}

static FBSTRING* HASTNODECLASSTOSTR( $13AST_NODECLASS C$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$202:;
	if( ((int64)-(C$1 > 44ll) | (int64)-(C$1 < 0ll)) == 0ll) goto label$205;
	{
		FBSTRING TMP$266$2;
		FBSTRING* vr$4 = fb_LongintToStr( C$1 );
		__builtin_memset( &TMP$266$2, 0, 24ll );
		FBSTRING* vr$7 = fb_StrConcat( &TMP$266$2, (void*)"CLASS:", 7ll, (void*)vr$4, -1ll );
		fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$7, -1ll, 0 );
		goto label$203;
	}
	label$205:;
	label$204:;
	fb_StrInit( (void*)&fb$result$1, -1ll, *(void**)((int64)(struct $8NAMEINFO*)DBG_ASTNODECLASSNAMES$ + (C$1 << (3ll & 63ll))), 0ll, 0 );
	goto label$203;
	label$203:;
	FBSTRING* vr$12 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$12;
}

static FBSTRING* HSYMBTOSTR( struct $8FBSYMBOL* S$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$206:;
	if( S$1 != (struct $8FBSYMBOL*)0ull) goto label$209;
	{
		fb_StrInit( (void*)&fb$result$1, -1ll, (void*)"", 1ll, 0 );
		goto label$207;
		label$209:;
	}
	if( *(char**)((uint8*)S$1 + 32ll) == (char*)0ull) goto label$211;
	{
		fb_StrInit( (void*)&fb$result$1, -1ll, *(void**)((uint8*)S$1 + 32ll), 0ll, 0 );
		goto label$207;
	}
	goto label$210;
	label$211:;
	if( *(char**)((uint8*)S$1 + 40ll) == (char*)0ull) goto label$212;
	{
		fb_StrInit( (void*)&fb$result$1, -1ll, *(void**)((uint8*)S$1 + 40ll), 0ll, 0 );
		goto label$207;
	}
	label$212:;
	label$210:;
	label$207:;
	FBSTRING* vr$9 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$9;
}

static FBSTRING* HASTNODETYPETOSTR( struct $7ASTNODE* N$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$213:;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$216;
	{
		fb_StrInit( (void*)&fb$result$1, -1ll, (void*)"", 1ll, 0 );
		goto label$214;
		label$216:;
	}
	FBSTRING* vr$4 = SYMBTYPETOSTR( *(int64*)((uint8*)N$1 + 8ll), *(struct $8FBSYMBOL**)((uint8*)N$1 + 16ll), 0ll, 0ll );
	fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$4, -1ll, 0 );
	goto label$214;
	label$214:;
	FBSTRING* vr$7 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$7;
}

static FBSTRING* HASTNODETOSTR( struct $7ASTNODE* N$1 )
{
	FBSTRING fb$result$1;
	__builtin_memset( &fb$result$1, 0, 24ll );
	label$217:;
	{
		uint64 TMP$267$2;
		TMP$267$2 = *(uint64*)N$1;
		goto label$220;
		label$221:;
		{
			FBSTRING TMP$271$3;
			FBSTRING TMP$272$3;
			FBSTRING TMP$273$3;
			char S$3[2];
			fb_StrAssign( (void*)S$3, 2ll, (void*)"", 1ll, 0 );
			{
				int64 TMP$268$4;
				TMP$268$4 = *(int64*)((uint8*)N$1 + 40ll);
				if( TMP$268$4 == 45ll) goto label$224;
				label$225:;
				if( TMP$268$4 == 48ll) goto label$224;
				label$226:;
				if( TMP$268$4 == 47ll) goto label$224;
				label$227:;
				if( TMP$268$4 == 46ll) goto label$224;
				label$228:;
				if( TMP$268$4 == 50ll) goto label$224;
				label$229:;
				if( TMP$268$4 != 49ll) goto label$223;
				label$224:;
				{
					if( (*(int64*)((uint8*)N$1 + 48ll) & 128ll) == 0ll) goto label$231;
					{
						fb_StrAssign( (void*)S$3, 2ll, (void*)"!", 2ll, 0 );
					}
					label$231:;
					label$230:;
				}
				label$223:;
				label$222:;
			}
			FBSTRING* vr$8 = HSYMBTOSTR( *(struct $8FBSYMBOL**)((uint8*)N$1 + 56ll) );
			FBSTRING* vr$10 = ASTDUMPOPTOSTR( *($6AST_OP*)((uint8*)N$1 + 40ll) );
			__builtin_memset( &TMP$271$3, 0, 24ll );
			FBSTRING* vr$14 = fb_StrConcat( &TMP$271$3, (void*)S$3, 2ll, (void*)vr$10, -1ll );
			__builtin_memset( &TMP$272$3, 0, 24ll );
			FBSTRING* vr$17 = fb_StrConcat( &TMP$272$3, (void*)vr$14, -1ll, (void*)" =-= ", 6ll );
			__builtin_memset( &TMP$273$3, 0, 24ll );
			FBSTRING* vr$20 = fb_StrConcat( &TMP$273$3, (void*)vr$17, -1ll, (void*)vr$8, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$20, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$232:;
		{
			FBSTRING* vr$23 = ASTDUMPOPTOSTR( *($6AST_OP*)((uint8*)N$1 + 40ll) );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$23, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$233:;
		{
			int64 TMP$274$3;
			FBSTRING TMP$280$3;
			FBSTRING TMP$281$3;
			FBSTRING TMP$282$3;
			if( (*(int64*)((uint8*)N$1 + 8ll) & 480ll) == 0ll) goto label$234;
			TMP$274$3 = 24ll;
			goto label$278;
			label$234:;
			TMP$274$3 = *(int64*)((uint8*)N$1 + 8ll) & 31ll;
			label$278:;
			if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$274$3 * 56ll)) != 1ll) goto label$236;
			{
				FBSTRING TMP$277$4;
				FBSTRING TMP$278$4;
				FBSTRING TMP$279$4;
				FBSTRING* vr$30 = HASTNODETYPETOSTR( N$1 );
				__builtin_memset( &TMP$277$4, 0, 24ll );
				FBSTRING* vr$33 = fb_StrConcat( &TMP$277$4, (void*)" (", 3ll, (void*)vr$30, -1ll );
				__builtin_memset( &TMP$278$4, 0, 24ll );
				FBSTRING* vr$36 = fb_StrConcat( &TMP$278$4, (void*)vr$33, -1ll, (void*)")", 2ll );
				FBSTRING* vr$38 = fb_DoubleToStr( *(double*)((uint8*)N$1 + 40ll) );
				__builtin_memset( &TMP$279$4, 0, 24ll );
				FBSTRING* vr$41 = fb_StrConcat( &TMP$279$4, (void*)vr$38, -1ll, (void*)vr$36, -1ll );
				fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$41, -1ll, 0 );
				goto label$218;
			}
			label$236:;
			label$235:;
			FBSTRING* vr$43 = HASTNODETYPETOSTR( N$1 );
			__builtin_memset( &TMP$280$3, 0, 24ll );
			FBSTRING* vr$46 = fb_StrConcat( &TMP$280$3, (void*)" (", 3ll, (void*)vr$43, -1ll );
			__builtin_memset( &TMP$281$3, 0, 24ll );
			FBSTRING* vr$49 = fb_StrConcat( &TMP$281$3, (void*)vr$46, -1ll, (void*)")", 2ll );
			FBSTRING* vr$51 = fb_LongintToStr( *(int64*)((uint8*)N$1 + 40ll) );
			__builtin_memset( &TMP$282$3, 0, 24ll );
			FBSTRING* vr$54 = fb_StrConcat( &TMP$282$3, (void*)vr$51, -1ll, (void*)vr$49, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$54, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$237:;
		{
			char* TMP$285$3;
			FBSTRING TMP$287$3;
			FBSTRING TMP$288$3;
			FBSTRING TMP$289$3;
			FBSTRING TMP$290$3;
			FBSTRING TMP$291$3;
			FBSTRING* vr$56 = HASTNODETYPETOSTR( N$1 );
			__builtin_memset( &TMP$289$3, 0, 24ll );
			FBSTRING* vr$59 = fb_StrConcat( &TMP$289$3, (void*)" (", 3ll, (void*)vr$56, -1ll );
			__builtin_memset( &TMP$290$3, 0, 24ll );
			FBSTRING* vr$62 = fb_StrConcat( &TMP$290$3, (void*)vr$59, -1ll, (void*)")", 2ll );
			if( *(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) == (struct $8FBSYMBOL*)0ull) goto label$238;
			TMP$285$3 = *(char**)((uint8*)*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) + 32ll);
			goto label$279;
			label$238:;
			TMP$285$3 = (char*)"<NULL>";
			label$279:;
			__builtin_memset( &TMP$287$3, 0, 24ll );
			FBSTRING* vr$68 = fb_StrConcat( &TMP$287$3, (void*)"VAR( ", 6ll, (void*)TMP$285$3, 0ll );
			__builtin_memset( &TMP$288$3, 0, 24ll );
			FBSTRING* vr$71 = fb_StrConcat( &TMP$288$3, (void*)vr$68, -1ll, (void*)" )", 3ll );
			__builtin_memset( &TMP$291$3, 0, 24ll );
			FBSTRING* vr$74 = fb_StrConcat( &TMP$291$3, (void*)vr$71, -1ll, (void*)vr$62, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$74, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$239:;
		{
			FBSTRING TMP$293$3;
			FBSTRING TMP$294$3;
			FBSTRING TMP$295$3;
			FBSTRING TMP$296$3;
			FBSTRING TMP$297$3;
			FBSTRING* vr$76 = HASTNODETYPETOSTR( N$1 );
			__builtin_memset( &TMP$295$3, 0, 24ll );
			FBSTRING* vr$79 = fb_StrConcat( &TMP$295$3, (void*)" (", 3ll, (void*)vr$76, -1ll );
			__builtin_memset( &TMP$296$3, 0, 24ll );
			FBSTRING* vr$82 = fb_StrConcat( &TMP$296$3, (void*)vr$79, -1ll, (void*)")", 2ll );
			__builtin_memset( &TMP$293$3, 0, 24ll );
			FBSTRING* vr$87 = fb_StrConcat( &TMP$293$3, (void*)"FIELD( ", 8ll, *(void**)((uint8*)*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) + 32ll), 0ll );
			__builtin_memset( &TMP$294$3, 0, 24ll );
			FBSTRING* vr$90 = fb_StrConcat( &TMP$294$3, (void*)vr$87, -1ll, (void*)" )", 3ll );
			__builtin_memset( &TMP$297$3, 0, 24ll );
			FBSTRING* vr$93 = fb_StrConcat( &TMP$297$3, (void*)vr$90, -1ll, (void*)vr$82, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$93, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$240:;
		{
			FBSTRING TMP$301$3;
			FBSTRING TMP$302$3;
			FBSTRING TMP$303$3;
			if( *(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) == (struct $8FBSYMBOL*)0ull) goto label$242;
			{
				FBSTRING TMP$299$4;
				FBSTRING TMP$300$4;
				__builtin_memset( &TMP$299$4, 0, 24ll );
				FBSTRING* vr$100 = fb_StrConcat( &TMP$299$4, (void*)"DECL( ", 7ll, *(void**)((uint8*)*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) + 32ll), 0ll );
				__builtin_memset( &TMP$300$4, 0, 24ll );
				FBSTRING* vr$103 = fb_StrConcat( &TMP$300$4, (void*)vr$100, -1ll, (void*)" )", 3ll );
				fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$103, -1ll, 0 );
				goto label$218;
			}
			label$242:;
			label$241:;
			FBSTRING* vr$105 = HASTNODETYPETOSTR( N$1 );
			__builtin_memset( &TMP$301$3, 0, 24ll );
			FBSTRING* vr$108 = fb_StrConcat( &TMP$301$3, (void*)" (", 3ll, (void*)vr$105, -1ll );
			__builtin_memset( &TMP$302$3, 0, 24ll );
			FBSTRING* vr$111 = fb_StrConcat( &TMP$302$3, (void*)vr$108, -1ll, (void*)")", 2ll );
			__builtin_memset( &TMP$303$3, 0, 24ll );
			FBSTRING* vr$114 = fb_StrConcat( &TMP$303$3, (void*)"DECL", 5ll, (void*)vr$111, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$114, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$243:;
		{
			FBSTRING TMP$305$3;
			FBSTRING TMP$306$3;
			__builtin_memset( &TMP$305$3, 0, 24ll );
			FBSTRING* vr$120 = fb_StrConcat( &TMP$305$3, (void*)"CALL( ", 7ll, *(void**)((uint8*)*(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) + 32ll), 0ll );
			__builtin_memset( &TMP$306$3, 0, 24ll );
			FBSTRING* vr$123 = fb_StrConcat( &TMP$306$3, (void*)vr$120, -1ll, (void*)" )", 3ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$123, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$244:;
		{
			FBSTRING TMP$308$3;
			FBSTRING* vr$126 = HSYMBTOSTR( *(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) );
			__builtin_memset( &TMP$308$3, 0, 24ll );
			FBSTRING* vr$129 = fb_StrConcat( &TMP$308$3, (void*)"LABEL: ", 8ll, (void*)vr$126, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$129, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$245:;
		{
			FBSTRING TMP$311$3;
			FBSTRING TMP$312$3;
			FBSTRING TMP$313$3;
			FBSTRING* vr$132 = HSYMBTOSTR( *(struct $8FBSYMBOL**)((uint8*)N$1 + 56ll) );
			FBSTRING* vr$134 = ASTDUMPOPTOSTR( *($6AST_OP*)((uint8*)N$1 + 40ll) );
			__builtin_memset( &TMP$311$3, 0, 24ll );
			FBSTRING* vr$137 = fb_StrConcat( &TMP$311$3, (void*)"BRANCH: ", 9ll, (void*)vr$134, -1ll );
			__builtin_memset( &TMP$312$3, 0, 24ll );
			FBSTRING* vr$140 = fb_StrConcat( &TMP$312$3, (void*)vr$137, -1ll, (void*)" ", 2ll );
			__builtin_memset( &TMP$313$3, 0, 24ll );
			FBSTRING* vr$143 = fb_StrConcat( &TMP$313$3, (void*)vr$140, -1ll, (void*)vr$132, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$143, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$246:;
		{
			FBSTRING TMP$315$3;
			FBSTRING* vr$146 = HSYMBTOSTR( *(struct $8FBSYMBOL**)((uint8*)N$1 + 24ll) );
			__builtin_memset( &TMP$315$3, 0, 24ll );
			FBSTRING* vr$149 = fb_StrConcat( &TMP$315$3, (void*)"SCOPEBEGIN: ", 13ll, (void*)vr$146, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$149, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$247:;
		{
			FBSTRING TMP$319$3;
			FBSTRING TMP$320$3;
			FBSTRING TMP$321$3;
			FBSTRING TMP$322$3;
			FBSTRING TMP$323$3;
			FBSTRING TMP$324$3;
			FBSTRING TMP$325$3;
			FBSTRING TMP$326$3;
			FBSTRING* vr$151 = HASTNODETYPETOSTR( N$1 );
			__builtin_memset( &TMP$324$3, 0, 24ll );
			FBSTRING* vr$154 = fb_StrConcat( &TMP$324$3, (void*)" (", 3ll, (void*)vr$151, -1ll );
			__builtin_memset( &TMP$325$3, 0, 24ll );
			FBSTRING* vr$157 = fb_StrConcat( &TMP$325$3, (void*)vr$154, -1ll, (void*)")", 2ll );
			FBSTRING* vr$159 = fb_LongintToStr( *(int64*)((uint8*)N$1 + 48ll) );
			FBSTRING* vr$161 = fb_LongintToStr( *(int64*)((uint8*)N$1 + 40ll) );
			FBSTRING* vr$163 = HASTNODECLASSTOSTR( *($13AST_NODECLASS*)N$1 );
			__builtin_memset( &TMP$319$3, 0, 24ll );
			FBSTRING* vr$166 = fb_StrConcat( &TMP$319$3, (void*)vr$163, -1ll, (void*)"( offset=", 10ll );
			__builtin_memset( &TMP$320$3, 0, 24ll );
			FBSTRING* vr$169 = fb_StrConcat( &TMP$320$3, (void*)vr$166, -1ll, (void*)vr$161, -1ll );
			__builtin_memset( &TMP$321$3, 0, 24ll );
			FBSTRING* vr$172 = fb_StrConcat( &TMP$321$3, (void*)vr$169, -1ll, (void*)", bytes=", 9ll );
			__builtin_memset( &TMP$322$3, 0, 24ll );
			FBSTRING* vr$175 = fb_StrConcat( &TMP$322$3, (void*)vr$172, -1ll, (void*)vr$159, -1ll );
			__builtin_memset( &TMP$323$3, 0, 24ll );
			FBSTRING* vr$178 = fb_StrConcat( &TMP$323$3, (void*)vr$175, -1ll, (void*)" ) ", 4ll );
			__builtin_memset( &TMP$326$3, 0, 24ll );
			FBSTRING* vr$181 = fb_StrConcat( &TMP$326$3, (void*)vr$178, -1ll, (void*)vr$157, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$181, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$248:;
		{
			FBSTRING TMP$328$3;
			FBSTRING TMP$329$3;
			FBSTRING TMP$330$3;
			FBSTRING TMP$331$3;
			FBSTRING TMP$332$3;
			FBSTRING* vr$183 = HASTNODETYPETOSTR( N$1 );
			__builtin_memset( &TMP$330$3, 0, 24ll );
			FBSTRING* vr$186 = fb_StrConcat( &TMP$330$3, (void*)" (", 3ll, (void*)vr$183, -1ll );
			__builtin_memset( &TMP$331$3, 0, 24ll );
			FBSTRING* vr$189 = fb_StrConcat( &TMP$331$3, (void*)vr$186, -1ll, (void*)")", 2ll );
			FBSTRING* vr$191 = ASTDUMPOPTOSTR( *($6AST_OP*)((uint8*)N$1 + 40ll) );
			__builtin_memset( &TMP$328$3, 0, 24ll );
			FBSTRING* vr$194 = fb_StrConcat( &TMP$328$3, (void*)"MACRO: ", 8ll, (void*)vr$191, -1ll );
			__builtin_memset( &TMP$329$3, 0, 24ll );
			FBSTRING* vr$197 = fb_StrConcat( &TMP$329$3, (void*)vr$194, -1ll, (void*)" ", 2ll );
			__builtin_memset( &TMP$332$3, 0, 24ll );
			FBSTRING* vr$200 = fb_StrConcat( &TMP$332$3, (void*)vr$197, -1ll, (void*)vr$189, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$200, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$249:;
		{
			FBSTRING TMP$336$3;
			FBSTRING TMP$337$3;
			FBSTRING TMP$338$3;
			FBSTRING TMP$339$3;
			FBSTRING TMP$340$3;
			FBSTRING TMP$341$3;
			FBSTRING S$3;
			fb_StrInit( (void*)&S$3, -1ll, (void*)"", 1ll, 0 );
			{
				int64 TMP$333$4;
				TMP$333$4 = *(int64*)((uint8*)N$1 + 40ll);
				if( TMP$333$4 != 1ll) goto label$251;
				label$252:;
				{
					fb_StrAssign( (void*)&S$3, -1ll, (void*)"L", 2ll, 0 );
				}
				goto label$250;
				label$251:;
				if( TMP$333$4 != 2ll) goto label$253;
				label$254:;
				{
					fb_StrAssign( (void*)&S$3, -1ll, (void*)"R", 2ll, 0 );
				}
				label$253:;
				label$250:;
			}
			FBSTRING* vr$207 = HASTNODETYPETOSTR( N$1 );
			__builtin_memset( &TMP$337$3, 0, 24ll );
			FBSTRING* vr$210 = fb_StrConcat( &TMP$337$3, (void*)" (", 3ll, (void*)vr$207, -1ll );
			__builtin_memset( &TMP$338$3, 0, 24ll );
			FBSTRING* vr$213 = fb_StrConcat( &TMP$338$3, (void*)vr$210, -1ll, (void*)")", 2ll );
			FBSTRING* vr$215 = HASTNODECLASSTOSTR( *($13AST_NODECLASS*)N$1 );
			__builtin_memset( &TMP$336$3, 0, 24ll );
			FBSTRING* vr$218 = fb_StrConcat( &TMP$336$3, (void*)"*", 2ll, (void*)vr$215, -1ll );
			__builtin_memset( &TMP$339$3, 0, 24ll );
			FBSTRING* vr$221 = fb_StrConcat( &TMP$339$3, (void*)vr$218, -1ll, (void*)vr$213, -1ll );
			__builtin_memset( &TMP$340$3, 0, 24ll );
			FBSTRING* vr$224 = fb_StrConcat( &TMP$340$3, (void*)vr$221, -1ll, (void*)"-", 2ll );
			__builtin_memset( &TMP$341$3, 0, 24ll );
			FBSTRING* vr$227 = fb_StrConcat( &TMP$341$3, (void*)vr$224, -1ll, (void*)&S$3, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$227, -1ll, 0 );
			fb_StrDelete( (FBSTRING*)&S$3 );
			goto label$218;
			fb_StrDelete( (FBSTRING*)&S$3 );
		}
		goto label$219;
		label$255:;
		{
			FBSTRING TMP$342$3;
			FBSTRING TMP$343$3;
			FBSTRING TMP$344$3;
			FBSTRING* vr$231 = HASTNODETYPETOSTR( N$1 );
			__builtin_memset( &TMP$342$3, 0, 24ll );
			FBSTRING* vr$234 = fb_StrConcat( &TMP$342$3, (void*)" (", 3ll, (void*)vr$231, -1ll );
			__builtin_memset( &TMP$343$3, 0, 24ll );
			FBSTRING* vr$237 = fb_StrConcat( &TMP$343$3, (void*)vr$234, -1ll, (void*)")", 2ll );
			FBSTRING* vr$239 = HASTNODECLASSTOSTR( *($13AST_NODECLASS*)N$1 );
			__builtin_memset( &TMP$344$3, 0, 24ll );
			FBSTRING* vr$242 = fb_StrConcat( &TMP$344$3, (void*)vr$239, -1ll, (void*)vr$237, -1ll );
			fb_StrInit( (void*)&fb$result$1, -1ll, (void*)vr$242, -1ll, 0 );
			goto label$218;
		}
		goto label$219;
		label$220:;
		static const void* tmp$351[42ll] = {
			&&label$221,
			&&label$232,
			&&label$255,
			&&label$255,
			&&label$245,
			&&label$255,
			&&label$243,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$249,
			&&label$233,
			&&label$237,
			&&label$255,
			&&label$239,
			&&label$255,
			&&label$244,
			&&label$255,
			&&label$255,
			&&label$240,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$255,
			&&label$246,
			&&label$255,
			&&label$255,
			&&label$247,
			&&label$247,
			&&label$247,
			&&label$247,
			&&label$247,
			&&label$247,
			&&label$247,
			&&label$255,
			&&label$248,
		};
		if( (TMP$267$2 - 3ull) > 41ull ) goto label$255;
		goto *tmp$351[TMP$267$2 - 3ull];
		label$219:;
	}
	label$218:;
	FBSTRING* vr$245 = fb_StrAllocTempResult( (FBSTRING*)&fb$result$1 );
	return vr$245;
}

static void ASTDUMPTREEEX( struct $7ASTNODE* N$1, int64 COL$1, int64 JUST$1, int64 DEPTH$1 )
{
	label$256:;
	if( ((int64)-(COL$1 <= 4ll) | (int64)-(COL$1 >= 76ll)) == 0ll) goto label$259;
	{
		COL$1 = 40ll;
	}
	label$259:;
	label$258:;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$261;
	{
		FBSTRING* vr$3 = fb_StrAllocTempDescZEx( (char*)"<NULL>", 6ll );
		fb_PrintString( 0, (FBSTRING*)vr$3, 1 );
		goto label$257;
	}
	label$261:;
	label$260:;
	FBSTRING S$1;
	__builtin_memset( &S$1, 0, 24ll );
	FBSTRING* vr$5 = HASTNODETOSTR( N$1 );
	fb_StrConcatAssign( (void*)&S$1, -1ll, (void*)vr$5, -1ll, 0 );
	DBG_ASTOUTPUT( &S$1, COL$1, JUST$1, DEPTH$1 );
	DEPTH$1 = DEPTH$1 + 1ll;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$263;
	{
		if( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) == (struct $7ASTNODE*)0ull) goto label$265;
		{
			FBSTRING TMP$346$3;
			__builtin_memset( &TMP$346$3, 0, 24ll );
			fb_StrAssign( (void*)&TMP$346$3, -1ll, (void*)"/ \x5C", 4ll, 0 );
			DBG_ASTOUTPUT( &TMP$346$3, COL$1 + -2ll, 0ll, -1ll );
			fb_StrDelete( (FBSTRING*)&TMP$346$3 );
		}
		goto label$264;
		label$265:;
		{
			FBSTRING TMP$347$3;
			__builtin_memset( &TMP$347$3, 0, 24ll );
			fb_StrAssign( (void*)&TMP$347$3, -1ll, (void*)"/", 2ll, 0 );
			DBG_ASTOUTPUT( &TMP$347$3, COL$1 + -2ll, 0ll, -1ll );
			fb_StrDelete( (FBSTRING*)&TMP$347$3 );
		}
		label$264:;
	}
	goto label$262;
	label$263:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) == (struct $7ASTNODE*)0ull) goto label$266;
	{
		FBSTRING TMP$349$2;
		__builtin_memset( &TMP$349$2, 0, 24ll );
		fb_StrAssign( (void*)&TMP$349$2, -1ll, (void*)"  \x5C", 4ll, 0 );
		DBG_ASTOUTPUT( &TMP$349$2, COL$1 + -2ll, 0ll, -1ll );
		fb_StrDelete( (FBSTRING*)&TMP$349$2 );
	}
	goto label$262;
	label$266:;
	{
		FBSTRING TMP$350$2;
		__builtin_memset( &TMP$350$2, 0, 24ll );
		fb_StrAssign( (void*)&TMP$350$2, -1ll, (void*)"", 1ll, 0 );
		DBG_ASTOUTPUT( &TMP$350$2, 0ll, 0ll, -1ll );
		fb_StrDelete( (FBSTRING*)&TMP$350$2 );
	}
	label$262:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$268;
	{
		ASTDUMPTREEEX( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), COL$1 + -2ll, -1ll, DEPTH$1 );
	}
	label$268:;
	label$267:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) == (struct $7ASTNODE*)0ull) goto label$270;
	{
		ASTDUMPTREEEX( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll), COL$1 + 2ll, 1ll, DEPTH$1 );
	}
	label$270:;
	label$269:;
	fb_StrDelete( (FBSTRING*)&S$1 );
	label$257:;
}
