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
typedef FBSTRING* (*tmp$29)( void*, int64* );
typedef uint32* (*tmp$30)( void*, int64* );
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
typedef int64 $11AST_OPFLAGS;
struct $10AST_OPINFO {
	$13AST_NODECLASS CLASS;
	$11AST_OPFLAGS FLAGS;
	char* ID;
	$6AST_OP SELFOP;
};
__FB_STATIC_ASSERT( sizeof( struct $10AST_OPINFO ) == 32 );
struct $11TSTRSETITEM {
	FBSTRING S;
	int64 USERDATA;
	struct $8HASHITEM* HASHITEM;
};
__FB_STATIC_ASSERT( sizeof( struct $11TSTRSETITEM ) == 40 );
FBSTRING* fb_StrAssign( void*, int64, void*, int64, int32 );
void fb_StrDelete( FBSTRING* );
static void fb_ctor__astzoptimize( void ) __attribute__(( constructor ));
static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM*, struct $11TSTRSETITEM* );
int64 FBGETCPUFAMILY( void );
void ASTDELNODE( struct $7ASTNODE* );
struct $7ASTNODE* ASTCLONETREE( struct $7ASTNODE* );
void ASTDELTREE( struct $7ASTNODE* );
int64 ASTISTREEEQUAL( struct $7ASTNODE*, struct $7ASTNODE* );
struct $7ASTNODE* ASTNEWNOP( void );
typedef int64 $11AST_CONVOPT;
struct $7ASTNODE* ASTNEWCONV( int64, struct $8FBSYMBOL*, struct $7ASTNODE*, $11AST_CONVOPT, int64* );
struct $7ASTNODE* ASTSKIPNOCONVCAST( struct $7ASTNODE* );
struct $7ASTNODE* ASTNEWBOP( int64, struct $7ASTNODE*, struct $7ASTNODE*, struct $8FBSYMBOL*, $9AST_OPOPT );
struct $7ASTNODE* ASTNEWUOP( int64, struct $7ASTNODE* );
struct $7ASTNODE* ASTNEWCONSTI( int64, int64, struct $8FBSYMBOL* );
int64 ASTCONSTFLUSHTOINT( struct $7ASTNODE*, int64 );
typedef int64 $15AST_LINK_RETURN;
struct $7ASTNODE* ASTNEWLINK( struct $7ASTNODE*, struct $7ASTNODE*, $15AST_LINK_RETURN );
int64 ASTINCOFFSET( struct $7ASTNODE*, int64 );
struct $7ASTNODE* ASTOPTASSIGNMENT( struct $7ASTNODE* );
struct $7ASTNODE* ASTUPDSTRCONCAT( struct $7ASTNODE* );
int64 ASTHASSIDEFX( struct $7ASTNODE* );
int64 ASTISSYMBOLONTREE( struct $8FBSYMBOL*, struct $7ASTNODE* );
void ASTSETTYPE( struct $7ASTNODE*, int64, struct $8FBSYMBOL* );
int64 ASTGETOFFSETCHILDOFS( struct $7ASTNODE* );
int64 TYPETOUNSIGNED( int64 );
struct $7ASTNODE* RTLSTRASSIGN( struct $7ASTNODE*, struct $7ASTNODE*, int64 );
struct $7ASTNODE* RTLWSTRASSIGN( struct $7ASTNODE*, struct $7ASTNODE*, int64 );
struct $7ASTNODE* RTLSTRCONCATASSIGN( struct $7ASTNODE*, struct $7ASTNODE*, int64 );
struct $7ASTNODE* RTLWSTRCONCATASSIGN( struct $7ASTNODE*, struct $7ASTNODE* );
static void HOPTCONSTREMNEG( struct $7ASTNODE* );
static struct $7ASTNODE* HCONSTACCUMADDSUB( struct $7ASTNODE*, struct $7ASTNODE**, int64 );
static struct $7ASTNODE* HCONSTACCUMMUL( struct $7ASTNODE*, struct $7ASTNODE** );
static struct $7ASTNODE* HOPTCONSTACCUM1( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTCONSTACCUM2( struct $7ASTNODE* );
static struct $7ASTNODE* HCONSTDISTMUL( struct $7ASTNODE*, struct $7ASTNODE** );
static struct $7ASTNODE* HOPTCONSTDISTMUL( struct $7ASTNODE* );
static void HOPTCONSTIDXMULT( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTDEREFADDR( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTCONSTIDX( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTASSOCADD( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTASSOCMUL( struct $7ASTNODE* );
static void HDIVTOSHIFT_SIGNED( struct $7ASTNODE*, int64 );
static int64 HTOPOW2( uint64 );
static void HOPTTOSHIFT( struct $7ASTNODE* );
static struct $7ASTNODE* HTRYREMOVECAST( struct $7ASTNODE*, $11FB_DATATYPE );
static $11FB_DATATYPE HGETOPERANDAS32BIT( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTBOP32( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTCONSTCONV( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTNULLOP( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTLOGIC( struct $7ASTNODE* );
static struct $7ASTNODE* HDOOPTREMCONV( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTSTRMULTCONCAT( struct $7ASTNODE*, struct $7ASTNODE*, struct $7ASTNODE*, int64 );
static int64 HISMULTSTRCONCAT( struct $7ASTNODE*, struct $7ASTNODE* );
static struct $7ASTNODE* HOPTSTRASSIGNMENT( struct $7ASTNODE*, struct $7ASTNODE*, struct $7ASTNODE* );
struct $7ASTNODE* HOPTSELFASSIGN( struct $7ASTNODE* );
struct $7ASTNODE* HOPTSELFCOMPARE( struct $7ASTNODE* );
static struct $7ASTNODE* HOPTRECIPROCAL( struct $7ASTNODE* );
typedef void (*tmp$39)( void );
typedef int64 (*tmp$40)( void );
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
typedef void (*tmp$69)( char* );
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
extern struct $10AST_OPINFO AST_OPTB$[122];
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

int64 ASTINCOFFSET( struct $7ASTNODE* N$1, int64 OFS$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$152:;
	{
		uint64 TMP$96$2;
		TMP$96$2 = *(uint64*)N$1;
		goto label$155;
		label$156:;
		{
			*(int64*)((uint8*)N$1 + 40ll) = *(int64*)((uint8*)N$1 + 40ll) + OFS$1;
			fb$result$1 = -1ll;
		}
		goto label$154;
		label$157:;
		{
			*(int64*)((uint8*)N$1 + 40ll) = *(int64*)((uint8*)N$1 + 40ll) + OFS$1;
			fb$result$1 = -1ll;
		}
		goto label$154;
		label$158:;
		{
			*(int64*)((uint8*)N$1 + 40ll) = *(int64*)((uint8*)N$1 + 40ll) + OFS$1;
			fb$result$1 = -1ll;
		}
		goto label$154;
		label$159:;
		{
			{
				int64 TMP$97$4;
				TMP$97$4 = *(int64*)((uint8*)N$1 + 40ll);
				if( TMP$97$4 != 1ll) goto label$161;
				label$162:;
				{
					int64 vr$13 = ASTINCOFFSET( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), OFS$1 );
					fb$result$1 = vr$13;
				}
				goto label$160;
				label$161:;
				if( TMP$97$4 != 2ll) goto label$163;
				label$164:;
				{
					int64 vr$15 = ASTINCOFFSET( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll), OFS$1 );
					fb$result$1 = vr$15;
				}
				label$163:;
				label$160:;
			}
		}
		goto label$154;
		label$165:;
		{
			int64 vr$17 = ASTINCOFFSET( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), OFS$1 );
			fb$result$1 = vr$17;
		}
		goto label$154;
		label$166:;
		{
			if( *(int64*)((uint8*)N$1 + 40ll) != 0ll) goto label$168;
			{
				int64 vr$20 = ASTINCOFFSET( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), OFS$1 );
				fb$result$1 = vr$20;
			}
			goto label$167;
			label$168:;
			{
				fb$result$1 = 0ll;
			}
			label$167:;
		}
		goto label$154;
		label$169:;
		{
			fb$result$1 = 0ll;
		}
		goto label$154;
		label$155:;
		static const void* tmp$98[22ll] = {
			&&label$166,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$159,
			&&label$169,
			&&label$156,
			&&label$157,
			&&label$165,
			&&label$158,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$169,
			&&label$165,
		};
		if( (TMP$96$2 - 5ull) > 21ull ) goto label$169;
		goto *tmp$98[TMP$96$2 - 5ull];
		label$154:;
	}
	label$153:;
	return fb$result$1;
}

struct $7ASTNODE* ASTOPTASSIGNMENT( struct $7ASTNODE* N$1 )
{
	int64 TMP$165$1;
	int64 TMP$167$1;
	int64 TMP$170$1;
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$662:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 DTYPE$1;
	int64 DCLASS$1;
	fb$result$1 = N$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$665;
	{
		goto label$663;
	}
	label$665:;
	label$664:;
	{
		$13AST_NODECLASS TMP$162$2;
		TMP$162$2 = *($13AST_NODECLASS*)N$1;
		if( TMP$162$2 != 2ll) goto label$667;
		label$668:;
		{
		}
		goto label$666;
		label$667:;
		if( TMP$162$2 == 15ll) goto label$670;
		label$671:;
		if( TMP$162$2 != 13ll) goto label$669;
		label$670:;
		{
			struct $7ASTNODE* vr$3 = ASTOPTASSIGNMENT( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
			*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$3;
			struct $7ASTNODE* vr$6 = ASTOPTASSIGNMENT( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
			*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$6;
			goto label$663;
		}
		goto label$666;
		label$669:;
		{
			goto label$663;
		}
		label$672:;
		label$666:;
	}
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	DTYPE$1 = *(int64*)((uint8*)N$1 + 8ll);
	{
		int64 TMP$163$2;
		int64 TMP$164$2;
		if( (DTYPE$1 & 480ll) == 0ll) goto label$673;
		TMP$163$2 = 24ll;
		goto label$718;
		label$673:;
		TMP$163$2 = DTYPE$1 & 31ll;
		label$718:;
		TMP$164$2 = TMP$163$2;
		if( TMP$164$2 == 17ll) goto label$676;
		label$677:;
		if( TMP$164$2 == 18ll) goto label$676;
		label$678:;
		if( TMP$164$2 != 7ll) goto label$675;
		label$676:;
		{
			struct $7ASTNODE* vr$13 = HOPTSTRASSIGNMENT( N$1, L$1, R$1 );
			fb$result$1 = vr$13;
			goto label$663;
		}
		label$675:;
		label$674:;
	}
	if( (DTYPE$1 & 480ll) == 0ll) goto label$679;
	TMP$165$1 = 24ll;
	goto label$719;
	label$679:;
	TMP$165$1 = DTYPE$1 & 31ll;
	label$719:;
	DCLASS$1 = *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$165$1 * 56ll));
	if( DCLASS$1 != 0ll) goto label$681;
	{
		if( (*(int64*)((uint8*)&IR$ + 544ll) & 512ll) != 0ll) goto label$683;
		{
			goto label$663;
		}
		label$683:;
		label$682:;
	}
	goto label$680;
	label$681:;
	{
		if( (*(int64*)((uint8*)&IR$ + 544ll) & 8ll) != 0ll) goto label$685;
		{
			if( *(int64*)R$1 != 16ll) goto label$687;
			{
				if( DCLASS$1 != 1ll) goto label$689;
				{
					int64 TMP$166$5;
					if( ((*(int64*)((uint8*)R$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$690;
					TMP$166$5 = 24ll;
					goto label$720;
					label$690:;
					TMP$166$5 = (*(int64*)((uint8*)R$1 + 8ll) & 511ll) & 31ll;
					label$720:;
					if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$166$5 * 56ll)) == 1ll) goto label$692;
					{
						struct $7ASTNODE* vr$27 = ASTNEWCONV( DTYPE$1, (struct $8FBSYMBOL*)0ull, R$1, 0ll, (int64*)0ull );
						*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$27;
					}
					label$692:;
					label$691:;
				}
				label$689:;
				label$688:;
			}
			label$687:;
			label$686:;
			goto label$663;
		}
		label$685:;
		label$684:;
	}
	label$680:;
	if( (DTYPE$1 & 480ll) == 0ll) goto label$693;
	TMP$167$1 = 24ll;
	goto label$721;
	label$693:;
	TMP$167$1 = DTYPE$1 & 31ll;
	label$721:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$167$1 * 56ll)) + 8ll) != 1ll) goto label$695;
	{
		goto label$663;
	}
	label$695:;
	label$694:;
	struct $7ASTNODE* LNOCAST$1;
	struct $7ASTNODE* vr$32 = ASTSKIPNOCONVCAST( L$1 );
	LNOCAST$1 = vr$32;
	struct $7ASTNODE* RNOCAST$1;
	struct $7ASTNODE* vr$33 = ASTSKIPNOCONVCAST( R$1 );
	RNOCAST$1 = vr$33;
	{
		uint64 TMP$168$2;
		TMP$168$2 = *(uint64*)LNOCAST$1;
		goto label$697;
		label$698:;
		{
		}
		goto label$696;
		label$699:;
		{
			if( *(int64*)((uint8*)*(struct $8FBSYMBOL**)((uint8*)LNOCAST$1 + 24ll) + 192ll) <= 0ll) goto label$701;
			{
				goto label$663;
			}
			label$701:;
			label$700:;
		}
		goto label$696;
		label$702:;
		{
			goto label$663;
		}
		goto label$696;
		label$697:;
		static const void* tmp$171[10ll] = {
			&&label$698,
			&&label$698,
			&&label$699,
			&&label$698,
			&&label$702,
			&&label$702,
			&&label$702,
			&&label$702,
			&&label$702,
			&&label$698,
		};
		if( (TMP$168$2 - 17ull) > 9ull ) goto label$702;
		goto *tmp$171[TMP$168$2 - 17ull];
		label$696:;
	}
	{
		$13AST_NODECLASS TMP$169$2;
		TMP$169$2 = *($13AST_NODECLASS*)RNOCAST$1;
		if( TMP$169$2 != 4ll) goto label$704;
		label$705:;
		{
		}
		goto label$703;
		label$704:;
		if( TMP$169$2 != 3ll) goto label$706;
		label$707:;
		{
			if( (*(int64*)(((int64)(struct $10AST_OPINFO*)AST_OPTB$ + (*(int64*)((uint8*)RNOCAST$1 + 40ll) << (5ll & 63ll))) + 8ll) & 8ll) == 0ll) goto label$709;
			{
				goto label$663;
			}
			label$709:;
			label$708:;
		}
		goto label$703;
		label$706:;
		{
			goto label$663;
		}
		label$710:;
		label$703:;
	}
	if( ((*(int64*)((uint8*)RNOCAST$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$711;
	TMP$170$1 = 24ll;
	goto label$722;
	label$711:;
	TMP$170$1 = (*(int64*)((uint8*)RNOCAST$1 + 8ll) & 511ll) & 31ll;
	label$722:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$170$1 * 56ll)) == 0ll) goto label$713;
	{
		goto label$663;
	}
	label$713:;
	label$712:;
	struct $7ASTNODE* vr$49 = ASTSKIPNOCONVCAST( *(struct $7ASTNODE**)((uint8*)RNOCAST$1 + 112ll) );
	int64 vr$50 = ASTISTREEEQUAL( LNOCAST$1, vr$49 );
	if( vr$50 != 0ll) goto label$715;
	{
		goto label$663;
	}
	label$715:;
	label$714:;
	*($9AST_OPOPT*)((uint8*)RNOCAST$1 + 48ll) = *(int64*)((uint8*)RNOCAST$1 + 48ll) & -2ll;
	ASTDELNODE( N$1 );
	ASTDELTREE( L$1 );
	if( R$1 == RNOCAST$1) goto label$717;
	{
		ASTDELNODE( R$1 );
		R$1 = RNOCAST$1;
	}
	label$717:;
	label$716:;
	fb$result$1 = R$1;
	label$663:;
	return fb$result$1;
}

struct $7ASTNODE* HOPTSELFASSIGN( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$723:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 DTYPE$1;
	int64 DCLASS$1;
	fb$result$1 = N$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$726;
	{
		goto label$724;
	}
	label$726:;
	label$725:;
	if( *(int64*)N$1 == 2ll) goto label$728;
	{
		goto label$724;
	}
	label$728:;
	label$727:;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	int64 vr$4 = ASTISTREEEQUAL( L$1, R$1 );
	if( vr$4 != 0ll) goto label$730;
	{
		goto label$724;
	}
	label$730:;
	label$729:;
	ASTDELNODE( N$1 );
	ASTDELTREE( L$1 );
	ASTDELTREE( R$1 );
	struct $7ASTNODE* vr$5 = ASTNEWNOP(  );
	fb$result$1 = vr$5;
	label$724:;
	return fb$result$1;
}

struct $7ASTNODE* HOPTSELFCOMPARE( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$731:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 DTYPE$1;
	int64 DCLASS$1;
	fb$result$1 = N$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$734;
	{
		goto label$732;
	}
	label$734:;
	label$733:;
	if( *(int64*)N$1 == 3ll) goto label$736;
	{
		goto label$732;
	}
	label$736:;
	label$735:;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	int64 vr$4 = ASTISTREEEQUAL( L$1, R$1 );
	if( vr$4 != 0ll) goto label$738;
	{
		goto label$732;
	}
	label$738:;
	label$737:;
	if( *(int64*)((uint8*)&ENV$ + 240ll) != 0ll) goto label$740;
	{
		int64 TMP$172$2;
		int64 TMP$173$2;
		if( ((*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$741;
		TMP$172$2 = 24ll;
		goto label$754;
		label$741:;
		TMP$172$2 = (*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 31ll;
		label$754:;
		if( ((*(int64*)((uint8*)R$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$742;
		TMP$173$2 = 24ll;
		goto label$755;
		label$742:;
		TMP$173$2 = (*(int64*)((uint8*)R$1 + 8ll) & 511ll) & 31ll;
		label$755:;
		if( ((int64)-(*(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$172$2 * 56ll)) == 1ll) | (int64)-(*(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$173$2 * 56ll)) == 1ll)) == 0ll) goto label$744;
		{
			goto label$732;
		}
		label$744:;
		label$743:;
	}
	label$740:;
	label$739:;
	int64 C$1;
	__builtin_memset( &C$1, 0, 8ll );
	{
		uint64 TMP$174$2;
		TMP$174$2 = *(uint64*)((uint8*)N$1 + 40ll);
		goto label$746;
		label$747:;
		{
			if( (*(int64*)((uint8*)N$1 + 48ll) & 128ll) == 0ll) goto label$749;
			{
				C$1 = 0ll;
			}
			goto label$748;
			label$749:;
			{
				C$1 = -1ll;
			}
			label$748:;
		}
		goto label$745;
		label$750:;
		{
			if( (*(int64*)((uint8*)N$1 + 48ll) & 128ll) == 0ll) goto label$752;
			{
				C$1 = -1ll;
			}
			goto label$751;
			label$752:;
			{
				C$1 = 0ll;
			}
			label$751:;
		}
		goto label$745;
		label$753:;
		{
			goto label$732;
		}
		goto label$745;
		label$746:;
		static const void* tmp$175[6ll] = {
			&&label$747,
			&&label$750,
			&&label$750,
			&&label$750,
			&&label$747,
			&&label$747,
		};
		if( (TMP$174$2 - 45ull) > 5ull ) goto label$753;
		goto *tmp$175[TMP$174$2 - 45ull];
		label$745:;
	}
	ASTDELNODE( N$1 );
	ASTDELTREE( L$1 );
	ASTDELTREE( R$1 );
	struct $7ASTNODE* vr$28 = ASTNEWCONSTI( C$1, 8ll, (struct $8FBSYMBOL*)0ull );
	fb$result$1 = vr$28;
	label$732:;
	return fb$result$1;
}

struct $7ASTNODE* ASTOPTIMIZETREE( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$780:;
	*(int64*)((uint8*)&AST$ + 464ll) = *(int64*)((uint8*)&AST$ + 464ll) + 1ll;
	struct $7ASTNODE* vr$2 = HOPTASSOCADD( N$1 );
	N$1 = vr$2;
	struct $7ASTNODE* vr$3 = HOPTASSOCMUL( N$1 );
	N$1 = vr$3;
	struct $7ASTNODE* vr$4 = HOPTCONSTDISTMUL( N$1 );
	N$1 = vr$4;
	struct $7ASTNODE* vr$5 = HOPTCONSTACCUM1( N$1 );
	N$1 = vr$5;
	struct $7ASTNODE* vr$6 = HOPTCONSTACCUM2( N$1 );
	N$1 = vr$6;
	HOPTCONSTREMNEG( N$1 );
	struct $7ASTNODE* vr$7 = HOPTCONSTIDX( N$1 );
	N$1 = vr$7;
	HOPTTOSHIFT( N$1 );
	struct $7ASTNODE* vr$8 = HOPTBOP32( N$1 );
	N$1 = vr$8;
	struct $7ASTNODE* vr$9 = HOPTLOGIC( N$1 );
	N$1 = vr$9;
	struct $7ASTNODE* vr$10 = HOPTNULLOP( N$1 );
	N$1 = vr$10;
	struct $7ASTNODE* vr$11 = HOPTSELFASSIGN( N$1 );
	N$1 = vr$11;
	struct $7ASTNODE* vr$12 = HOPTSELFCOMPARE( N$1 );
	N$1 = vr$12;
	if( ((int64)-((*(int64*)((uint8*)&IR$ + 544ll) & 1ll) == 0ll) & (int64)-(*(int64*)((uint8*)&ENV$ + 208ll) == 0ll)) == 0ll) goto label$783;
	{
		struct $7ASTNODE* vr$17 = HDOOPTREMCONV( N$1 );
		N$1 = vr$17;
	}
	label$783:;
	label$782:;
	if( (*(int64*)((uint8*)&IR$ + 544ll) & 131072ll) != 0ll) goto label$785;
	{
		if( *(int64*)((uint8*)&ENV$ + 240ll) != 1ll) goto label$787;
		{
			struct $7ASTNODE* vr$19 = HOPTRECIPROCAL( N$1 );
			N$1 = vr$19;
		}
		label$787:;
		label$786:;
	}
	label$785:;
	label$784:;
	*(int64*)((uint8*)&AST$ + 464ll) = *(int64*)((uint8*)&AST$ + 464ll) + -1ll;
	fb$result$1 = N$1;
	label$781:;
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

static void HOPTCONSTREMNEG( struct $7ASTNODE* N$1 )
{
	int64 TMP$86$1;
	label$10:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	struct $7ASTNODE* LL$1;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 == (struct $7ASTNODE*)0ull) goto label$13;
	{
		HOPTCONSTREMNEG( L$1 );
	}
	label$13:;
	label$12:;
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	if( R$1 == (struct $7ASTNODE*)0ull) goto label$15;
	{
		HOPTCONSTREMNEG( R$1 );
	}
	label$15:;
	label$14:;
	if( *(int64*)N$1 != 3ll) goto label$16;
	TMP$86$1 = (int64)-(*(int64*)((uint8*)N$1 + 40ll) == 28ll);
	goto label$788;
	label$16:;
	TMP$86$1 = 0ll;
	label$788:;
	if( TMP$86$1 == 0ll) goto label$18;
	{
		int64 TMP$87$2;
		L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
		R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
		if( *(int64*)L$1 != 4ll) goto label$19;
		TMP$87$2 = (int64)-(*(int64*)((uint8*)L$1 + 40ll) == 55ll);
		goto label$789;
		label$19:;
		TMP$87$2 = 0ll;
		label$789:;
		if( (TMP$87$2 & (int64)-(*(int64*)R$1 == 16ll)) == 0ll) goto label$21;
		{
			LL$1 = *(struct $7ASTNODE**)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 112ll);
			if( *(int64*)LL$1 != 17ll) goto label$23;
			{
				*(int64*)((uint8*)N$1 + 40ll) = 29ll;
				*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = R$1;
				*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = LL$1;
				ASTDELNODE( L$1 );
			}
			label$23:;
			label$22:;
		}
		label$21:;
		label$20:;
	}
	label$18:;
	label$17:;
	label$11:;
}

static struct $7ASTNODE* HCONSTACCUMADDSUB( struct $7ASTNODE* N$1, struct $7ASTNODE** ACCUMVAL$1, int64 SIGN$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$24:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 DTYPE$1;
	int64 O$1;
	int64 RSIGN$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$27;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$25;
	}
	label$27:;
	label$26:;
	if( *(int64*)N$1 == 3ll) goto label$29;
	{
		fb$result$1 = N$1;
		goto label$25;
	}
	label$29:;
	label$28:;
	O$1 = *(int64*)((uint8*)N$1 + 40ll);
	{
		if( O$1 == 28ll) goto label$32;
		label$33:;
		if( O$1 != 29ll) goto label$31;
		label$32:;
		{
			L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
			R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
			if( O$1 != 29ll) goto label$35;
			{
				RSIGN$1 = -SIGN$1;
			}
			goto label$34;
			label$35:;
			{
				RSIGN$1 = SIGN$1;
			}
			label$34:;
			if( *(int64*)R$1 != 16ll) goto label$37;
			{
				if( *ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$39;
				{
					if( RSIGN$1 >= 0ll) goto label$41;
					{
						if( O$1 != 28ll) goto label$43;
						{
							O$1 = 29ll;
						}
						goto label$42;
						label$43:;
						{
							O$1 = 28ll;
						}
						label$42:;
					}
					label$41:;
					label$40:;
					struct $7ASTNODE* vr$9 = ASTNEWBOP( O$1, *ACCUMVAL$1, R$1, (struct $8FBSYMBOL*)0ull, 1ll );
					*ACCUMVAL$1 = vr$9;
				}
				goto label$38;
				label$39:;
				{
					*ACCUMVAL$1 = R$1;
					if( RSIGN$1 >= 0ll) goto label$45;
					{
						struct $7ASTNODE* vr$13 = ASTNEWUOP( 55ll, *ACCUMVAL$1 );
						*ACCUMVAL$1 = vr$13;
					}
					label$45:;
					label$44:;
				}
				label$38:;
				ASTDELNODE( N$1 );
				struct $7ASTNODE* vr$15 = HCONSTACCUMADDSUB( L$1, ACCUMVAL$1, SIGN$1 );
				N$1 = vr$15;
			}
			goto label$36;
			label$37:;
			{
				struct $7ASTNODE* vr$16 = HCONSTACCUMADDSUB( L$1, ACCUMVAL$1, SIGN$1 );
				*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$16;
				struct $7ASTNODE* vr$18 = HCONSTACCUMADDSUB( R$1, ACCUMVAL$1, RSIGN$1 );
				*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$18;
			}
			label$36:;
		}
		label$31:;
		label$30:;
	}
	fb$result$1 = N$1;
	label$25:;
	return fb$result$1;
}

static struct $7ASTNODE* HCONSTACCUMMUL( struct $7ASTNODE* N$1, struct $7ASTNODE** ACCUMVAL$1 )
{
	int64 TMP$88$1;
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$46:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 DTYPE$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$49;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$47;
	}
	label$49:;
	label$48:;
	if( *(int64*)N$1 != 3ll) goto label$50;
	TMP$88$1 = (int64)-(*(int64*)((uint8*)N$1 + 40ll) == 30ll);
	goto label$790;
	label$50:;
	TMP$88$1 = 0ll;
	label$790:;
	if( TMP$88$1 == 0ll) goto label$52;
	{
		L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
		R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
		if( *(int64*)R$1 != 16ll) goto label$54;
		{
			if( *ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$56;
			{
				struct $7ASTNODE* vr$9 = ASTNEWBOP( 30ll, *ACCUMVAL$1, R$1, (struct $8FBSYMBOL*)0ull, 1ll );
				*ACCUMVAL$1 = vr$9;
			}
			goto label$55;
			label$56:;
			{
				*ACCUMVAL$1 = R$1;
			}
			label$55:;
			ASTDELNODE( N$1 );
			struct $7ASTNODE* vr$12 = HCONSTACCUMMUL( L$1, ACCUMVAL$1 );
			N$1 = vr$12;
		}
		goto label$53;
		label$54:;
		{
			struct $7ASTNODE* vr$13 = HCONSTACCUMMUL( L$1, ACCUMVAL$1 );
			*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$13;
			struct $7ASTNODE* vr$15 = HCONSTACCUMMUL( R$1, ACCUMVAL$1 );
			*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$15;
		}
		label$53:;
	}
	label$52:;
	label$51:;
	fb$result$1 = N$1;
	label$47:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTCONSTACCUM1( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$57:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	struct $7ASTNODE* ACCUMVAL$1;
	int64 O$1;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$60;
	{
		struct $7ASTNODE* vr$3 = HOPTCONSTACCUM1( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$3;
	}
	label$60:;
	label$59:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) == (struct $7ASTNODE*)0ull) goto label$62;
	{
		struct $7ASTNODE* vr$7 = HOPTCONSTACCUM1( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$7;
	}
	label$62:;
	label$61:;
	if( *(int64*)N$1 != 3ll) goto label$64;
	{
		R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
		if( *(int64*)R$1 != 16ll) goto label$66;
		{
			ACCUMVAL$1 = (struct $7ASTNODE*)0ull;
			O$1 = *(int64*)((uint8*)N$1 + 40ll);
			{
				if( O$1 == 28ll) goto label$69;
				label$70:;
				if( O$1 != 29ll) goto label$68;
				label$69:;
				{
					struct $7ASTNODE* vr$14 = HCONSTACCUMADDSUB( N$1, &ACCUMVAL$1, 1ll );
					N$1 = vr$14;
					if( ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$72;
					{
						struct $7ASTNODE* vr$15 = ASTNEWBOP( O$1, N$1, ACCUMVAL$1, (struct $8FBSYMBOL*)0ull, 1ll );
						N$1 = vr$15;
					}
					label$72:;
					label$71:;
				}
				goto label$67;
				label$68:;
				if( O$1 != 30ll) goto label$73;
				label$74:;
				{
					struct $7ASTNODE* vr$17 = HCONSTACCUMMUL( N$1, &ACCUMVAL$1 );
					N$1 = vr$17;
					if( ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$76;
					{
						struct $7ASTNODE* vr$18 = ASTNEWBOP( 30ll, N$1, ACCUMVAL$1, (struct $8FBSYMBOL*)0ull, 1ll );
						N$1 = vr$18;
					}
					label$76:;
					label$75:;
				}
				label$73:;
				label$67:;
			}
		}
		label$66:;
		label$65:;
	}
	label$64:;
	label$63:;
	fb$result$1 = N$1;
	label$58:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTCONSTACCUM2( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$77:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	struct $7ASTNODE* ACCUMVAL$1;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$80;
	{
		struct $7ASTNODE* vr$3 = HOPTCONSTACCUM2( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$3;
	}
	label$80:;
	label$79:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) == (struct $7ASTNODE*)0ull) goto label$82;
	{
		struct $7ASTNODE* vr$7 = HOPTCONSTACCUM2( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$7;
	}
	label$82:;
	label$81:;
	if( *(int64*)N$1 != 3ll) goto label$84;
	{
		ACCUMVAL$1 = (struct $7ASTNODE*)0ull;
		{
			int64 TMP$89$3;
			TMP$89$3 = *(int64*)((uint8*)N$1 + 40ll);
			if( TMP$89$3 != 28ll) goto label$86;
			label$87:;
			{
				{
					uint64 TMP$90$5;
					TMP$90$5 = (uint64)(*(int64*)((uint8*)N$1 + 8ll) & 511ll);
					goto label$89;
					label$90:;
					{
					}
					goto label$88;
					label$91:;
					{
						struct $7ASTNODE* vr$14 = HCONSTACCUMADDSUB( N$1, &ACCUMVAL$1, 1ll );
						N$1 = vr$14;
						if( ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$93;
						{
							struct $7ASTNODE* vr$15 = ASTNEWBOP( 28ll, N$1, ACCUMVAL$1, (struct $8FBSYMBOL*)0ull, 1ll );
							N$1 = vr$15;
						}
						label$93:;
						label$92:;
					}
					goto label$88;
					label$89:;
					static const void* tmp$181[12ll] = {
						&&label$90,
						&&label$91,
						&&label$91,
						&&label$91,
						&&label$91,
						&&label$91,
						&&label$91,
						&&label$91,
						&&label$91,
						&&label$91,
						&&label$90,
						&&label$90,
					};
					if( (TMP$90$5 - 7ull) > 11ull ) goto label$91;
					goto *tmp$181[TMP$90$5 - 7ull];
					label$88:;
				}
			}
			goto label$85;
			label$86:;
			if( TMP$89$3 != 30ll) goto label$94;
			label$95:;
			{
				struct $7ASTNODE* vr$17 = HCONSTACCUMMUL( N$1, &ACCUMVAL$1 );
				N$1 = vr$17;
				if( ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$97;
				{
					struct $7ASTNODE* vr$18 = ASTNEWBOP( 30ll, N$1, ACCUMVAL$1, (struct $8FBSYMBOL*)0ull, 1ll );
					N$1 = vr$18;
				}
				label$97:;
				label$96:;
			}
			label$94:;
			label$85:;
		}
	}
	label$84:;
	label$83:;
	fb$result$1 = N$1;
	label$78:;
	return fb$result$1;
}

static struct $7ASTNODE* HCONSTDISTMUL( struct $7ASTNODE* N$1, struct $7ASTNODE** ACCUMVAL$1 )
{
	int64 TMP$91$1;
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$98:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 DTYPE$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$101;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$99;
	}
	label$101:;
	label$100:;
	if( *(int64*)N$1 != 3ll) goto label$102;
	TMP$91$1 = (int64)-(*(int64*)((uint8*)N$1 + 40ll) == 28ll);
	goto label$791;
	label$102:;
	TMP$91$1 = 0ll;
	label$791:;
	if( TMP$91$1 == 0ll) goto label$104;
	{
		L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
		R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
		if( *(int64*)R$1 != 16ll) goto label$106;
		{
			if( *ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$108;
			{
				struct $7ASTNODE* vr$9 = ASTNEWBOP( 28ll, *ACCUMVAL$1, R$1, (struct $8FBSYMBOL*)0ull, 1ll );
				*ACCUMVAL$1 = vr$9;
			}
			goto label$107;
			label$108:;
			{
				*ACCUMVAL$1 = R$1;
			}
			label$107:;
			ASTDELNODE( N$1 );
			struct $7ASTNODE* vr$12 = HCONSTDISTMUL( L$1, ACCUMVAL$1 );
			N$1 = vr$12;
		}
		goto label$105;
		label$106:;
		{
			struct $7ASTNODE* vr$13 = HCONSTDISTMUL( L$1, ACCUMVAL$1 );
			*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$13;
			struct $7ASTNODE* vr$15 = HCONSTDISTMUL( R$1, ACCUMVAL$1 );
			*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$15;
		}
		label$105:;
	}
	label$104:;
	label$103:;
	fb$result$1 = N$1;
	label$99:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTCONSTDISTMUL( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$109:;
	struct $7ASTNODE* ACCUMVAL$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$112;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$110;
	}
	label$112:;
	label$111:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$114;
	{
		struct $7ASTNODE* vr$3 = HOPTCONSTDISTMUL( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$3;
	}
	label$114:;
	label$113:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) == (struct $7ASTNODE*)0ull) goto label$116;
	{
		struct $7ASTNODE* vr$7 = HOPTCONSTDISTMUL( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$7;
	}
	label$116:;
	label$115:;
	if( *(int64*)N$1 != 3ll) goto label$118;
	{
		if( *(int64*)*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) != 16ll) goto label$120;
		{
			if( *(int64*)((uint8*)N$1 + 40ll) != 30ll) goto label$122;
			{
				ACCUMVAL$1 = (struct $7ASTNODE*)0ull;
				struct $7ASTNODE* vr$15 = HCONSTDISTMUL( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), &ACCUMVAL$1 );
				*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$15;
				if( ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$124;
				{
					struct $7ASTNODE* vr$18 = ASTCLONETREE( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
					struct $7ASTNODE* vr$19 = ASTNEWBOP( 30ll, ACCUMVAL$1, vr$18, (struct $8FBSYMBOL*)0ull, 1ll );
					ACCUMVAL$1 = vr$19;
					struct $7ASTNODE* vr$20 = ASTNEWBOP( 28ll, N$1, ACCUMVAL$1, (struct $8FBSYMBOL*)0ull, 1ll );
					N$1 = vr$20;
				}
				label$124:;
				label$123:;
			}
			label$122:;
			label$121:;
		}
		label$120:;
		label$119:;
	}
	label$118:;
	label$117:;
	fb$result$1 = N$1;
	label$110:;
	return fb$result$1;
}

static void HOPTCONSTIDXMULT( struct $7ASTNODE* N$1 )
{
	int64 TMP$92$1;
	int64 TMP$94$1;
	int64 TMP$95$1;
	label$125:;
	int64 OPTIMIZE$1;
	int64 C$1;
	struct $7ASTNODE* L$1;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( *(int64*)L$1 != 3ll) goto label$127;
	TMP$92$1 = (int64)-(*(int64*)((uint8*)L$1 + 40ll) == 30ll);
	goto label$792;
	label$127:;
	TMP$92$1 = 0ll;
	label$792:;
	if( TMP$92$1 == 0ll) goto label$129;
	{
		struct $7ASTNODE* LR$2;
		LR$2 = *(struct $7ASTNODE**)((uint8*)L$1 + 120ll);
		if( *(int64*)LR$2 != 16ll) goto label$131;
		{
			if( (*(int64*)((uint8*)&IR$ + 544ll) & 65536ll) == 0ll) goto label$133;
			{
				C$1 = *(int64*)((uint8*)LR$2 + 40ll);
				if( ((int64)-(C$1 >= 1ll) & (int64)-(C$1 <= 9ll)) == 0ll) goto label$135;
				{
					{
						uint64 TMP$93$6;
						TMP$93$6 = (uint64)C$1;
						goto label$137;
						label$138:;
						{
							OPTIMIZE$1 = -1ll;
						}
						goto label$136;
						label$139:;
						{
							OPTIMIZE$1 = -1ll;
							struct $8FBSYMBOL* S$7;
							S$7 = *(struct $8FBSYMBOL**)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) + 24ll);
							if( (*(int64*)((uint8*)S$7 + 8ll) & 114688ll) == 0ll) goto label$141;
							{
								OPTIMIZE$1 = 0ll;
							}
							goto label$140;
							label$141:;
							if( (*(int64*)((uint8*)S$7 + 8ll) & 128ll) == 0ll) goto label$142;
							{
								if( (*(int64*)((uint8*)S$7 + 8ll) & 2ll) != 0ll) goto label$144;
								{
									OPTIMIZE$1 = 0ll;
								}
								label$144:;
								label$143:;
							}
							label$142:;
							label$140:;
						}
						goto label$136;
						label$145:;
						{
							OPTIMIZE$1 = 0ll;
						}
						goto label$136;
						label$137:;
						static const void* tmp$182[9ll] = {
							&&label$138,
							&&label$138,
							&&label$139,
							&&label$138,
							&&label$139,
							&&label$145,
							&&label$145,
							&&label$138,
							&&label$139,
						};
						if( (TMP$93$6 - 1ull) > 8ull ) goto label$145;
						goto *tmp$182[TMP$93$6 - 1ull];
						label$136:;
					}
					if( OPTIMIZE$1 == 0ll) goto label$147;
					{
						*(int64*)((uint8*)N$1 + 48ll) = C$1;
						*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = *(struct $7ASTNODE**)((uint8*)L$1 + 112ll);
						ASTDELNODE( LR$2 );
						ASTDELNODE( L$1 );
						L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
					}
					label$147:;
					label$146:;
				}
				label$135:;
				label$134:;
			}
			label$133:;
			label$132:;
		}
		label$131:;
		label$130:;
	}
	label$129:;
	label$128:;
	if( ((*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$148;
	TMP$94$1 = 24ll;
	goto label$793;
	label$148:;
	TMP$94$1 = (*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 31ll;
	label$793:;
	if( ((*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$149;
	TMP$95$1 = 24ll;
	goto label$794;
	label$149:;
	TMP$95$1 = (*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 31ll;
	label$794:;
	if( ((int64)-(*(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$94$1 * 56ll)) != 0ll) | (int64)-(*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$95$1 * 56ll)) + 8ll) != *(int64*)((uint8*)&ENV$ + 624ll))) == 0ll) goto label$151;
	{
		struct $7ASTNODE* vr$40 = ASTNEWCONV( 8ll, (struct $8FBSYMBOL*)0ull, L$1, 0ll, (int64*)0ull );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$40;
	}
	label$151:;
	label$150:;
	label$126:;
}

static struct $7ASTNODE* HOPTDEREFADDR( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$170:;
	struct $7ASTNODE* L$1;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	int64 OFS$1;
	OFS$1 = 0ll;
	{
		$13AST_NODECLASS TMP$99$2;
		TMP$99$2 = *($13AST_NODECLASS*)L$1;
		if( TMP$99$2 != 23ll) goto label$173;
		label$174:;
		{
			int64 vr$5 = ASTGETOFFSETCHILDOFS( *(struct $7ASTNODE**)((uint8*)L$1 + 112ll) );
			OFS$1 = *(int64*)((uint8*)L$1 + 40ll) - vr$5;
		}
		goto label$172;
		label$173:;
		if( TMP$99$2 != 6ll) goto label$175;
		label$176:;
		{
		}
		goto label$172;
		label$175:;
		{
			fb$result$1 = N$1;
			goto label$171;
		}
		label$177:;
		label$172:;
	}
	OFS$1 = OFS$1 + *(int64*)((uint8*)N$1 + 40ll);
	int64 vr$10 = ASTINCOFFSET( *(struct $7ASTNODE**)((uint8*)L$1 + 112ll), OFS$1 );
	if( vr$10 != 0ll) goto label$179;
	{
		fb$result$1 = N$1;
		goto label$171;
	}
	label$179:;
	label$178:;
	int64 DTYPE$1;
	DTYPE$1 = *(int64*)((uint8*)N$1 + 8ll);
	struct $8FBSYMBOL* SUBTYPE$1;
	SUBTYPE$1 = *(struct $8FBSYMBOL**)((uint8*)N$1 + 16ll);
	ASTDELNODE( N$1 );
	N$1 = *(struct $7ASTNODE**)((uint8*)L$1 + 112ll);
	ASTDELNODE( L$1 );
	ASTSETTYPE( N$1, DTYPE$1, SUBTYPE$1 );
	fb$result$1 = N$1;
	label$171:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTCONSTIDX( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$180:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	struct $7ASTNODE* ACCUMVAL$1;
	int64 C$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$183;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$181;
	}
	label$183:;
	label$182:;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 == (struct $7ASTNODE*)0ull) goto label$185;
	{
		struct $7ASTNODE* vr$2 = HOPTCONSTIDX( L$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$2;
	}
	label$185:;
	label$184:;
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	if( R$1 == (struct $7ASTNODE*)0ull) goto label$187;
	{
		struct $7ASTNODE* vr$5 = HOPTCONSTIDX( R$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$5;
	}
	label$187:;
	label$186:;
	{
		$13AST_NODECLASS TMP$100$2;
		TMP$100$2 = *($13AST_NODECLASS*)N$1;
		if( TMP$100$2 == 18ll) goto label$190;
		label$191:;
		if( TMP$100$2 != 20ll) goto label$189;
		label$190:;
		{
			if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$193;
			{
				ACCUMVAL$1 = (struct $7ASTNODE*)0ull;
				struct $7ASTNODE* vr$11 = HCONSTACCUMADDSUB( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), &ACCUMVAL$1, 1ll );
				*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$11;
				if( ACCUMVAL$1 == (struct $7ASTNODE*)0ull) goto label$195;
				{
					int64 vr$13 = ASTCONSTFLUSHTOINT( ACCUMVAL$1, 8ll );
					C$1 = vr$13;
					if( *(int64*)N$1 != 18ll) goto label$197;
					{
						*(int64*)((uint8*)N$1 + 40ll) = *(int64*)((uint8*)N$1 + 40ll) + (C$1 * *(int64*)((uint8*)N$1 + 48ll));
					}
					goto label$196;
					label$197:;
					{
						*(int64*)((uint8*)N$1 + 40ll) = *(int64*)((uint8*)N$1 + 40ll) + C$1;
					}
					label$196:;
				}
				label$195:;
				label$194:;
				if( *(int64*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) != 16ll) goto label$199;
				{
					int64 vr$26 = ASTCONSTFLUSHTOINT( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), 8ll );
					C$1 = vr$26;
					*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = (struct $7ASTNODE*)0ull;
					if( *(int64*)N$1 != 18ll) goto label$201;
					{
						*(int64*)((uint8*)N$1 + 40ll) = *(int64*)((uint8*)N$1 + 40ll) + (C$1 * *(int64*)((uint8*)N$1 + 48ll));
					}
					goto label$200;
					label$201:;
					{
						*(int64*)((uint8*)N$1 + 40ll) = *(int64*)((uint8*)N$1 + 40ll) + C$1;
					}
					label$200:;
				}
				goto label$198;
				label$199:;
				{
					if( *(int64*)N$1 != 18ll) goto label$203;
					{
						HOPTCONSTIDXMULT( N$1 );
					}
					goto label$202;
					label$203:;
					{
						struct $7ASTNODE* vr$38 = HOPTDEREFADDR( N$1 );
						N$1 = vr$38;
					}
					label$202:;
				}
				label$198:;
			}
			label$193:;
			label$192:;
		}
		label$189:;
		label$188:;
	}
	fb$result$1 = N$1;
	label$181:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTASSOCADD( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$204:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	struct $7ASTNODE* N_OLD$1;
	int64 OP$1;
	int64 ROP$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$207;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$205;
	}
	label$207:;
	label$206:;
	if( *(int64*)N$1 != 3ll) goto label$209;
	{
		OP$1 = *(int64*)((uint8*)N$1 + 40ll);
		{
			if( OP$1 == 28ll) goto label$212;
			label$213:;
			if( OP$1 != 29ll) goto label$211;
			label$212:;
			{
				{
					int64 TMP$101$5;
					TMP$101$5 = *(int64*)((uint8*)N$1 + 8ll) & 511ll;
					if( TMP$101$5 == 17ll) goto label$216;
					label$217:;
					if( TMP$101$5 == 18ll) goto label$216;
					label$218:;
					if( TMP$101$5 != 7ll) goto label$215;
					label$216:;
					{
					}
					goto label$214;
					label$215:;
					{
						R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
						if( *(int64*)R$1 != 3ll) goto label$221;
						{
							ROP$1 = *(int64*)((uint8*)R$1 + 40ll);
							{
								if( ROP$1 == 28ll) goto label$224;
								label$225:;
								if( ROP$1 != 29ll) goto label$223;
								label$224:;
								{
									if( OP$1 != 29ll) goto label$227;
									{
										if( ROP$1 != 29ll) goto label$229;
										{
											OP$1 = 28ll;
										}
										goto label$228;
										label$229:;
										{
											ROP$1 = 29ll;
										}
										label$228:;
									}
									goto label$226;
									label$227:;
									{
										if( ROP$1 != 29ll) goto label$231;
										{
											OP$1 = 29ll;
											ROP$1 = 28ll;
										}
										label$231:;
										label$230:;
									}
									label$226:;
									N_OLD$1 = N$1;
									struct $7ASTNODE* vr$11 = ASTNEWBOP( ROP$1, *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), *(struct $7ASTNODE**)((uint8*)R$1 + 112ll), (struct $8FBSYMBOL*)0ull, 1ll );
									struct $7ASTNODE* vr$12 = ASTNEWBOP( OP$1, vr$11, *(struct $7ASTNODE**)((uint8*)R$1 + 120ll), (struct $8FBSYMBOL*)0ull, 1ll );
									N$1 = vr$12;
									ASTDELNODE( R$1 );
									ASTDELNODE( N_OLD$1 );
								}
								label$223:;
								label$222:;
							}
						}
						label$221:;
						label$220:;
					}
					label$219:;
					label$214:;
				}
			}
			label$211:;
			label$210:;
		}
	}
	label$209:;
	label$208:;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 == (struct $7ASTNODE*)0ull) goto label$233;
	{
		struct $7ASTNODE* vr$14 = HOPTASSOCADD( L$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$14;
	}
	label$233:;
	label$232:;
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	if( R$1 == (struct $7ASTNODE*)0ull) goto label$235;
	{
		struct $7ASTNODE* vr$17 = HOPTASSOCADD( R$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$17;
	}
	label$235:;
	label$234:;
	fb$result$1 = N$1;
	label$205:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTASSOCMUL( struct $7ASTNODE* N$1 )
{
	int64 TMP$102$1;
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$236:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	struct $7ASTNODE* N_OLD$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$239;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$237;
	}
	label$239:;
	label$238:;
	if( *(int64*)N$1 != 3ll) goto label$240;
	TMP$102$1 = (int64)-(*(int64*)((uint8*)N$1 + 40ll) == 30ll);
	goto label$795;
	label$240:;
	TMP$102$1 = 0ll;
	label$795:;
	if( TMP$102$1 == 0ll) goto label$242;
	{
		int64 TMP$103$2;
		R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
		if( *(int64*)R$1 != 3ll) goto label$243;
		TMP$103$2 = (int64)-(*(int64*)((uint8*)R$1 + 40ll) == 30ll);
		goto label$796;
		label$243:;
		TMP$103$2 = 0ll;
		label$796:;
		if( TMP$103$2 == 0ll) goto label$245;
		{
			N_OLD$1 = N$1;
			struct $7ASTNODE* vr$11 = ASTNEWBOP( 30ll, *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), *(struct $7ASTNODE**)((uint8*)R$1 + 112ll), (struct $8FBSYMBOL*)0ull, 1ll );
			struct $7ASTNODE* vr$12 = ASTNEWBOP( 30ll, vr$11, *(struct $7ASTNODE**)((uint8*)R$1 + 120ll), (struct $8FBSYMBOL*)0ull, 1ll );
			N$1 = vr$12;
			ASTDELNODE( R$1 );
			ASTDELNODE( N_OLD$1 );
		}
		label$245:;
		label$244:;
	}
	label$242:;
	label$241:;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 == (struct $7ASTNODE*)0ull) goto label$247;
	{
		struct $7ASTNODE* vr$14 = HOPTASSOCMUL( L$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$14;
	}
	label$247:;
	label$246:;
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	if( R$1 == (struct $7ASTNODE*)0ull) goto label$249;
	{
		struct $7ASTNODE* vr$17 = HOPTASSOCMUL( R$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$17;
	}
	label$249:;
	label$248:;
	fb$result$1 = N$1;
	label$237:;
	return fb$result$1;
}

static void HDIVTOSHIFT_SIGNED( struct $7ASTNODE* N$1, int64 CONST_VAL$1 )
{
	int64 TMP$104$1;
	label$250:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* L_CPY$1;
	int64 DTYPE$1;
	int64 BITS$1;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( *(int64*)L$1 == 17ll) goto label$253;
	{
		goto label$251;
	}
	label$253:;
	label$252:;
	DTYPE$1 = *(int64*)((uint8*)L$1 + 8ll);
	if( (DTYPE$1 & 480ll) == 0ll) goto label$254;
	TMP$104$1 = 24ll;
	goto label$797;
	label$254:;
	TMP$104$1 = DTYPE$1 & 31ll;
	label$797:;
	BITS$1 = (*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$104$1 * 56ll)) + 8ll) << (3ll & 63ll)) + -1ll;
	if( BITS$1 != 7ll) goto label$256;
	{
		BITS$1 = (*(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + 456ll) << (3ll & 63ll)) + -1ll;
	}
	label$256:;
	label$255:;
	struct $7ASTNODE* vr$10 = ASTCLONETREE( L$1 );
	L_CPY$1 = vr$10;
	if( CONST_VAL$1 != 1ll) goto label$258;
	{
		struct $7ASTNODE* vr$11 = ASTNEWCONSTI( BITS$1, 8ll, (struct $8FBSYMBOL*)0ull );
		int64 vr$12 = TYPETOUNSIGNED( DTYPE$1 );
		struct $7ASTNODE* vr$13 = ASTNEWCONV( vr$12, (struct $8FBSYMBOL*)0ull, L$1, 1ll, (int64*)0ull );
		struct $7ASTNODE* vr$14 = ASTNEWBOP( 42ll, vr$13, vr$11, (struct $8FBSYMBOL*)0ull, 1ll );
		struct $7ASTNODE* vr$15 = ASTNEWBOP( 28ll, L_CPY$1, vr$14, (struct $8FBSYMBOL*)0ull, 1ll );
		struct $7ASTNODE* vr$16 = ASTNEWCONV( DTYPE$1, (struct $8FBSYMBOL*)0ull, vr$15, 1ll, (int64*)0ull );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$16;
	}
	goto label$257;
	label$258:;
	{
		struct $7ASTNODE* vr$20 = ASTNEWCONSTI( (1ll << (CONST_VAL$1 & 63ll)) + -1ll, 8ll, (struct $8FBSYMBOL*)0ull );
		struct $7ASTNODE* vr$21 = ASTNEWCONSTI( BITS$1, 8ll, (struct $8FBSYMBOL*)0ull );
		struct $7ASTNODE* vr$22 = ASTNEWBOP( 42ll, L$1, vr$21, (struct $8FBSYMBOL*)0ull, 1ll );
		struct $7ASTNODE* vr$23 = ASTNEWBOP( 34ll, vr$22, vr$20, (struct $8FBSYMBOL*)0ull, 1ll );
		struct $7ASTNODE* vr$24 = ASTNEWBOP( 28ll, L_CPY$1, vr$23, (struct $8FBSYMBOL*)0ull, 1ll );
		struct $7ASTNODE* vr$25 = ASTNEWCONV( DTYPE$1, (struct $8FBSYMBOL*)0ull, vr$24, 1ll, (int64*)0ull );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$25;
	}
	label$257:;
	*(int64*)((uint8*)N$1 + 40ll) = 42ll;
	*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) + 40ll) = CONST_VAL$1;
	label$251:;
}

static int64 HTOPOW2( uint64 V$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$259:;
	{
		int64 I$2;
		I$2 = 1ll;
		label$264:;
		{
			if( V$1 != (1ull << (I$2 & 63ll))) goto label$266;
			{
				fb$result$1 = I$2;
				goto label$260;
			}
			label$266:;
			label$265:;
		}
		label$262:;
		I$2 = I$2 + 1ll;
		label$261:;
		if( I$2 <= 63ll) goto label$264;
		label$263:;
	}
	fb$result$1 = 0ll;
	label$260:;
	return fb$result$1;
}

static void HOPTTOSHIFT( struct $7ASTNODE* N$1 )
{
	label$267:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 OP$1;
	int64 EXPONENT$1;
	int64 VALUE$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$270;
	{
		goto label$268;
	}
	label$270:;
	label$269:;
	if( *(int64*)N$1 != 3ll) goto label$272;
	{
		OP$1 = *(int64*)((uint8*)N$1 + 40ll);
		{
			if( OP$1 == 30ll) goto label$275;
			label$276:;
			if( OP$1 == 32ll) goto label$275;
			label$277:;
			if( OP$1 != 33ll) goto label$274;
			label$275:;
			{
				int64 TMP$105$4;
				int64 TMP$106$4;
				R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
				if( (*(int64*)((uint8*)N$1 + 8ll) & 480ll) == 0ll) goto label$278;
				TMP$105$4 = 24ll;
				goto label$798;
				label$278:;
				TMP$105$4 = *(int64*)((uint8*)N$1 + 8ll) & 31ll;
				label$798:;
				if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$105$4 * 56ll)) == 0ll) goto label$280;
				{
					goto label$273;
				}
				label$280:;
				label$279:;
				if( *(int64*)R$1 == 16ll) goto label$282;
				{
					goto label$273;
				}
				label$282:;
				label$281:;
				VALUE$1 = *(int64*)((uint8*)R$1 + 40ll);
				if( (*(int64*)((uint8*)R$1 + 8ll) & 480ll) == 0ll) goto label$283;
				TMP$106$4 = 24ll;
				goto label$799;
				label$283:;
				TMP$106$4 = *(int64*)((uint8*)R$1 + 8ll) & 31ll;
				label$799:;
				if( ((int64)-(VALUE$1 == 0ll) | ((int64)-(VALUE$1 < 0ll) & *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$106$4 * 56ll)) + 16ll))) == 0ll) goto label$285;
				{
					goto label$273;
				}
				label$285:;
				label$284:;
				int64 vr$19 = HTOPOW2( (uint64)VALUE$1 );
				EXPONENT$1 = vr$19;
				if( EXPONENT$1 > 0ll) goto label$287;
				{
					goto label$273;
				}
				label$287:;
				label$286:;
				{
					if( OP$1 != 30ll) goto label$289;
					label$290:;
					{
						int64 TMP$107$6;
						if( (*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 480ll) == 0ll) goto label$291;
						TMP$107$6 = 24ll;
						goto label$800;
						label$291:;
						TMP$107$6 = *(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 31ll;
						label$800:;
						if( EXPONENT$1 <= (*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$107$6 * 56ll)) + 8ll) << (3ll & 63ll))) goto label$293;
						{
							goto label$273;
						}
						label$293:;
						label$292:;
						*(int64*)((uint8*)N$1 + 40ll) = 41ll;
						*(int64*)((uint8*)R$1 + 40ll) = EXPONENT$1;
					}
					goto label$288;
					label$289:;
					if( OP$1 != 32ll) goto label$294;
					label$295:;
					{
						int64 TMP$108$6;
						int64 TMP$109$6;
						if( (*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 480ll) == 0ll) goto label$296;
						TMP$108$6 = 24ll;
						goto label$801;
						label$296:;
						TMP$108$6 = *(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 31ll;
						label$801:;
						if( EXPONENT$1 <= (*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$108$6 * 56ll)) + 8ll) << (3ll & 63ll))) goto label$298;
						{
							goto label$273;
						}
						label$298:;
						label$297:;
						if( (*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 480ll) == 0ll) goto label$299;
						TMP$109$6 = 24ll;
						goto label$802;
						label$299:;
						TMP$109$6 = *(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 31ll;
						label$802:;
						if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$109$6 * 56ll)) + 16ll) != 0ll) goto label$301;
						{
							*(int64*)((uint8*)N$1 + 40ll) = 42ll;
							*(int64*)((uint8*)R$1 + 40ll) = EXPONENT$1;
						}
						goto label$300;
						label$301:;
						{
							HDIVTOSHIFT_SIGNED( N$1, EXPONENT$1 );
						}
						label$300:;
					}
					goto label$288;
					label$294:;
					if( OP$1 != 33ll) goto label$302;
					label$303:;
					{
						int64 TMP$110$6;
						if( ((*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 511ll) & 480ll) == 0ll) goto label$304;
						TMP$110$6 = 24ll;
						goto label$803;
						label$304:;
						TMP$110$6 = (*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 511ll) & 31ll;
						label$803:;
						if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$110$6 * 56ll)) + 16ll) == 0ll) goto label$306;
						{
							goto label$273;
						}
						label$306:;
						label$305:;
						*(int64*)((uint8*)N$1 + 40ll) = 34ll;
						*(int64*)((uint8*)R$1 + 40ll) = *(int64*)((uint8*)R$1 + 40ll) + -1ll;
					}
					label$302:;
					label$288:;
				}
			}
			label$274:;
			label$273:;
		}
	}
	label$272:;
	label$271:;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 == (struct $7ASTNODE*)0ull) goto label$308;
	{
		HOPTTOSHIFT( L$1 );
	}
	label$308:;
	label$307:;
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	if( R$1 == (struct $7ASTNODE*)0ull) goto label$310;
	{
		HOPTTOSHIFT( R$1 );
	}
	label$310:;
	label$309:;
	label$268:;
}

static struct $7ASTNODE* HTRYREMOVECAST( struct $7ASTNODE* N$1, $11FB_DATATYPE DTYPE$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$311:;
	struct $7ASTNODE* L$1;
	int64 OP$1;
	if( *(int64*)N$1 == 5ll) goto label$314;
	{
		fb$result$1 = N$1;
		goto label$312;
	}
	label$314:;
	label$313:;
	{
		int64 TMP$111$2;
		TMP$111$2 = *(int64*)((uint8*)N$1 + 8ll) & 511ll;
		if( TMP$111$2 == 8ll) goto label$317;
		label$318:;
		if( TMP$111$2 != 9ll) goto label$316;
		label$317:;
		{
		}
		goto label$315;
		label$316:;
		if( TMP$111$2 == 11ll) goto label$320;
		label$321:;
		if( TMP$111$2 != 12ll) goto label$319;
		label$320:;
		{
		}
		goto label$315;
		label$319:;
		{
			fb$result$1 = N$1;
			goto label$312;
		}
		label$322:;
		label$315:;
	}
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	{
		int64 TMP$112$2;
		TMP$112$2 = *(int64*)((uint8*)L$1 + 8ll) & 511ll;
		if( TMP$112$2 == 11ll) goto label$325;
		label$326:;
		if( TMP$112$2 != 12ll) goto label$324;
		label$325:;
		{
			ASTDELNODE( N$1 );
			fb$result$1 = L$1;
			goto label$312;
		}
		label$324:;
		label$323:;
	}
	*(int64*)((uint8*)N$1 + 8ll) = (*(int64*)((uint8*)N$1 + 8ll) & -512ll) | (DTYPE$1 & 511ll);
	fb$result$1 = N$1;
	goto label$312;
	label$312:;
	return fb$result$1;
}

static $11FB_DATATYPE HGETOPERANDAS32BIT( struct $7ASTNODE* N$1 )
{
	int64 TMP$113$1;
	int64 TMP$114$1;
	int64 TMP$117$1;
	int64 TMP$118$1;
	int64 TMP$119$1;
	$11FB_DATATYPE TMP$120$1;
	$11FB_DATATYPE fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$327:;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$330;
	{
		fb$result$1 = 0ll;
		goto label$328;
	}
	label$330:;
	label$329:;
	if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$331;
	TMP$113$1 = 24ll;
	goto label$804;
	label$331:;
	TMP$113$1 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
	label$804:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$113$1 * 56ll)) == 0ll) goto label$333;
	{
		fb$result$1 = 0ll;
		goto label$328;
	}
	label$333:;
	label$332:;
	if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$334;
	TMP$114$1 = 24ll;
	goto label$805;
	label$334:;
	TMP$114$1 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
	label$805:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$114$1 * 56ll)) + 8ll) != 4ll) goto label$336;
	{
		int64 TMP$115$2;
		$11FB_DATATYPE TMP$116$2;
		if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$337;
		TMP$115$2 = 24ll;
		goto label$806;
		label$337:;
		TMP$115$2 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
		label$806:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$115$2 * 56ll)) + 16ll) == 0ll) goto label$338;
		TMP$116$2 = 11ll;
		goto label$807;
		label$338:;
		TMP$116$2 = 12ll;
		label$807:;
		fb$result$1 = TMP$116$2;
		goto label$328;
	}
	label$336:;
	label$335:;
	if( *(int64*)N$1 == 5ll) goto label$340;
	{
		fb$result$1 = 0ll;
		goto label$328;
	}
	label$340:;
	label$339:;
	if( ((*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 511ll) & 480ll) == 0ll) goto label$341;
	TMP$117$1 = 24ll;
	goto label$808;
	label$341:;
	TMP$117$1 = (*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 511ll) & 31ll;
	label$808:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$117$1 * 56ll)) == 0ll) goto label$343;
	{
		fb$result$1 = 0ll;
		goto label$328;
	}
	label$343:;
	label$342:;
	if( ((*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 511ll) & 480ll) == 0ll) goto label$344;
	TMP$118$1 = 24ll;
	goto label$809;
	label$344:;
	TMP$118$1 = (*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 511ll) & 31ll;
	label$809:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$118$1 * 56ll)) + 8ll) <= 4ll) goto label$346;
	{
		fb$result$1 = 0ll;
		goto label$328;
	}
	label$346:;
	label$345:;
	if( ((*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 511ll) & 480ll) == 0ll) goto label$347;
	TMP$119$1 = 24ll;
	goto label$810;
	label$347:;
	TMP$119$1 = (*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) + 8ll) & 511ll) & 31ll;
	label$810:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$119$1 * 56ll)) + 16ll) == 0ll) goto label$348;
	TMP$120$1 = 11ll;
	goto label$811;
	label$348:;
	TMP$120$1 = 12ll;
	label$811:;
	fb$result$1 = TMP$120$1;
	goto label$328;
	label$328:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTBOP32( struct $7ASTNODE* N$1 )
{
	int64 TMP$121$1;
	int64 TMP$125$1;
	int64 TMP$126$1;
	int64 TMP$127$1;
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$349:;
	struct $7ASTNODE* L$1;
	$11FB_DATATYPE LDTYPE$1;
	$11FB_DATATYPE RDTYPE$1;
	int64 OP$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$352;
	{
		fb$result$1 = N$1;
		goto label$350;
	}
	label$352:;
	label$351:;
	int64 vr$1 = FBGETCPUFAMILY(  );
	if( vr$1 == 1ll) goto label$354;
	{
		fb$result$1 = N$1;
		goto label$350;
	}
	label$354:;
	label$353:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$356;
	{
		struct $7ASTNODE* vr$4 = HOPTBOP32( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$4;
	}
	label$356:;
	label$355:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) == (struct $7ASTNODE*)0ull) goto label$358;
	{
		struct $7ASTNODE* vr$8 = HOPTBOP32( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$8;
	}
	label$358:;
	label$357:;
	if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$359;
	TMP$121$1 = 24ll;
	goto label$812;
	label$359:;
	TMP$121$1 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
	label$812:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$121$1 * 56ll)) == 0ll) goto label$361;
	{
		fb$result$1 = N$1;
		goto label$350;
	}
	label$361:;
	label$360:;
	{
		$13AST_NODECLASS TMP$122$2;
		TMP$122$2 = *($13AST_NODECLASS*)N$1;
		if( TMP$122$2 != 3ll) goto label$363;
		label$364:;
		{
			L$1 = N$1;
		}
		goto label$362;
		label$363:;
		if( TMP$122$2 != 5ll) goto label$365;
		label$366:;
		{
			L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
		}
		goto label$362;
		label$365:;
		{
			fb$result$1 = N$1;
			goto label$350;
		}
		label$367:;
		label$362:;
	}
	OP$1 = *(int64*)((uint8*)L$1 + 40ll);
	{
		if( OP$1 != 41ll) goto label$369;
		label$370:;
		{
			{
				$13AST_NODECLASS TMP$123$4;
				TMP$123$4 = *($13AST_NODECLASS*)N$1;
				if( TMP$123$4 != 5ll) goto label$372;
				label$373:;
				{
					int64 TMP$124$5;
					if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$374;
					TMP$124$5 = 24ll;
					goto label$813;
					label$374:;
					TMP$124$5 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
					label$813:;
					if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$124$5 * 56ll)) + 8ll) <= 4ll) goto label$376;
					{
						fb$result$1 = N$1;
						goto label$350;
					}
					label$376:;
					label$375:;
				}
				goto label$371;
				label$372:;
				{
					fb$result$1 = N$1;
					goto label$350;
				}
				label$377:;
				label$371:;
			}
		}
		goto label$368;
		label$369:;
		if( OP$1 == 33ll) goto label$379;
		label$380:;
		if( OP$1 != 32ll) goto label$378;
		label$379:;
		{
		}
		goto label$368;
		label$378:;
		{
			fb$result$1 = N$1;
			goto label$350;
		}
		label$381:;
		label$368:;
	}
	if( ((*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$382;
	TMP$125$1 = 24ll;
	goto label$814;
	label$382:;
	TMP$125$1 = (*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 31ll;
	label$814:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$125$1 * 56ll)) + 8ll) > 4ll) goto label$384;
	{
		fb$result$1 = N$1;
		goto label$350;
	}
	label$384:;
	label$383:;
	$11FB_DATATYPE vr$36 = HGETOPERANDAS32BIT( *(struct $7ASTNODE**)((uint8*)L$1 + 112ll) );
	LDTYPE$1 = vr$36;
	if( LDTYPE$1 != 0ll) goto label$386;
	{
		fb$result$1 = N$1;
		goto label$350;
	}
	label$386:;
	label$385:;
	$11FB_DATATYPE vr$38 = HGETOPERANDAS32BIT( *(struct $7ASTNODE**)((uint8*)L$1 + 120ll) );
	RDTYPE$1 = vr$38;
	if( RDTYPE$1 != 0ll) goto label$388;
	{
		fb$result$1 = N$1;
		goto label$350;
	}
	label$388:;
	label$387:;
	if( (LDTYPE$1 & 480ll) == 0ll) goto label$389;
	TMP$126$1 = 24ll;
	goto label$815;
	label$389:;
	TMP$126$1 = LDTYPE$1 & 31ll;
	label$815:;
	if( (RDTYPE$1 & 480ll) == 0ll) goto label$390;
	TMP$127$1 = 24ll;
	goto label$816;
	label$390:;
	TMP$127$1 = RDTYPE$1 & 31ll;
	label$816:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$126$1 * 56ll)) + 16ll) == *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$127$1 * 56ll)) + 16ll)) goto label$392;
	{
		fb$result$1 = N$1;
		goto label$350;
	}
	label$392:;
	label$391:;
	{
		$13AST_NODECLASS TMP$128$2;
		TMP$128$2 = *($13AST_NODECLASS*)N$1;
		if( TMP$128$2 != 5ll) goto label$394;
		label$395:;
		{
			int64 TMP$129$3;
			if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$396;
			TMP$129$3 = 24ll;
			goto label$817;
			label$396:;
			TMP$129$3 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
			label$817:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$129$3 * 56ll)) + 8ll) > 4ll) goto label$398;
			{
				*(int64*)((uint8*)L$1 + 8ll) = (*(int64*)((uint8*)L$1 + 8ll) & -512ll) | (LDTYPE$1 & 511ll);
			}
			label$398:;
			label$397:;
			ASTDELNODE( N$1 );
			N$1 = L$1;
		}
		goto label$393;
		label$394:;
		if( TMP$128$2 != 3ll) goto label$399;
		label$400:;
		{
			int64 TMP$130$3;
			int64 TMP$131$3;
			$11FB_DATATYPE TMP$132$3;
			if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$401;
			TMP$130$3 = 24ll;
			goto label$818;
			label$401:;
			TMP$130$3 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
			label$818:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$130$3 * 56ll)) + 8ll) < 4ll) goto label$403;
			{
				*(int64*)((uint8*)L$1 + 8ll) = (*(int64*)((uint8*)L$1 + 8ll) & -512ll) | (LDTYPE$1 & 511ll);
			}
			label$403:;
			label$402:;
			if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$404;
			TMP$131$3 = 24ll;
			goto label$819;
			label$404:;
			TMP$131$3 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
			label$819:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$131$3 * 56ll)) + 16ll) == 0ll) goto label$405;
			TMP$132$3 = 8ll;
			goto label$820;
			label$405:;
			TMP$132$3 = 9ll;
			label$820:;
			struct $7ASTNODE* vr$77 = ASTNEWCONV( TMP$132$3, (struct $8FBSYMBOL*)0ull, N$1, 0ll, (int64*)0ull );
			N$1 = vr$77;
		}
		label$399:;
		label$393:;
	}
	struct $7ASTNODE* vr$79 = HTRYREMOVECAST( *(struct $7ASTNODE**)((uint8*)L$1 + 112ll), LDTYPE$1 );
	*(struct $7ASTNODE**)((uint8*)L$1 + 112ll) = vr$79;
	struct $7ASTNODE* vr$82 = HTRYREMOVECAST( *(struct $7ASTNODE**)((uint8*)L$1 + 120ll), RDTYPE$1 );
	*(struct $7ASTNODE**)((uint8*)L$1 + 120ll) = vr$82;
	fb$result$1 = N$1;
	goto label$350;
	label$350:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTCONSTCONV( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$406:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* N_OLD$1;
	int64 V$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$409;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$407;
	}
	label$409:;
	label$408:;
	if( *(int64*)N$1 != 5ll) goto label$411;
	{
		L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
		if( *(int64*)L$1 != 16ll) goto label$413;
		{
			int64 TMP$133$3;
			if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$414;
			TMP$133$3 = 24ll;
			goto label$821;
			label$414:;
			TMP$133$3 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
			label$821:;
			if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$133$3 * 56ll)) != 0ll) goto label$416;
			{
				int64 TMP$134$4;
				if( ((*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$417;
				TMP$134$4 = 24ll;
				goto label$822;
				label$417:;
				TMP$134$4 = (*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 31ll;
				label$822:;
				if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$134$4 * 56ll)) != 0ll) goto label$419;
				{
					if( *(int64*)L$1 != 16ll) goto label$421;
					{
						V$1 = *(int64*)((uint8*)L$1 + 40ll);
						if( V$1 != 0ll) goto label$423;
						{
							N_OLD$1 = N$1;
							struct $7ASTNODE* vr$22 = ASTNEWCONSTI( V$1, *(int64*)((uint8*)N$1 + 8ll), *(struct $8FBSYMBOL**)((uint8*)N$1 + 16ll) );
							N$1 = vr$22;
							ASTDELNODE( L$1 );
							ASTDELNODE( N_OLD$1 );
						}
						label$423:;
						label$422:;
					}
					label$421:;
					label$420:;
				}
				label$419:;
				label$418:;
			}
			label$416:;
			label$415:;
		}
		label$413:;
		label$412:;
	}
	label$411:;
	label$410:;
	fb$result$1 = N$1;
	label$407:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTNULLOP( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$424:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 OP$1;
	int64 V$1;
	int64 KEEP_L$1;
	int64 KEEP_R$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$427;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$425;
	}
	label$427:;
	label$426:;
	struct $7ASTNODE* vr$2 = HOPTNULLOP( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$2;
	struct $7ASTNODE* vr$5 = HOPTNULLOP( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
	*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$5;
	if( *(int64*)N$1 != 3ll) goto label$429;
	{
		int64 TMP$135$2;
		OP$1 = *(int64*)((uint8*)N$1 + 40ll);
		L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
		R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
		int64 vr$11 = ASTHASSIDEFX( L$1 );
		KEEP_L$1 = vr$11;
		int64 vr$12 = ASTHASSIDEFX( R$1 );
		KEEP_R$1 = vr$12;
		if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$430;
		TMP$135$2 = 24ll;
		goto label$823;
		label$430:;
		TMP$135$2 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
		label$823:;
		if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$135$2 * 56ll)) != 0ll) goto label$432;
		{
			if( *(int64*)R$1 != 16ll) goto label$434;
			{
				V$1 = *(int64*)((uint8*)R$1 + 40ll);
				{
					uint64 TMP$136$5;
					TMP$136$5 = (uint64)OP$1;
					goto label$436;
					label$437:;
					{
						if( V$1 != 0ll) goto label$439;
						{
							if( KEEP_L$1 != 0ll) goto label$441;
							{
								ASTDELTREE( L$1 );
								ASTDELNODE( N$1 );
								fb$result$1 = R$1;
								goto label$425;
							}
							goto label$440;
							label$441:;
							{
								*(int64*)((uint8*)N$1 + 40ll) = 34ll;
							}
							label$440:;
						}
						goto label$438;
						label$439:;
						if( V$1 != 1ll) goto label$442;
						{
							ASTDELNODE( R$1 );
							ASTDELNODE( N$1 );
							fb$result$1 = L$1;
							goto label$425;
						}
						label$442:;
						label$438:;
					}
					goto label$435;
					label$443:;
					{
						int64 TMP$137$6;
						if( ((*(int64*)((uint8*)R$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$444;
						TMP$137$6 = 24ll;
						goto label$824;
						label$444:;
						TMP$137$6 = (*(int64*)((uint8*)R$1 + 8ll) & 511ll) & 31ll;
						label$824:;
						if( ((int64)-(V$1 == 1ll) | ((int64)-(V$1 == -1ll) & (int64)-(*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$137$6 * 56ll)) + 16ll) != 0ll))) == 0ll) goto label$446;
						{
							if( KEEP_L$1 != 0ll) goto label$448;
							{
								*(int64*)((uint8*)R$1 + 40ll) = 0ll;
								ASTDELTREE( L$1 );
								ASTDELNODE( N$1 );
								fb$result$1 = R$1;
								goto label$425;
							}
							label$448:;
							label$447:;
						}
						label$446:;
						label$445:;
					}
					goto label$435;
					label$449:;
					{
						if( V$1 != 1ll) goto label$451;
						{
							ASTDELNODE( R$1 );
							ASTDELNODE( N$1 );
							fb$result$1 = L$1;
							goto label$425;
						}
						label$451:;
						label$450:;
					}
					goto label$435;
					label$452:;
					{
						if( V$1 != 0ll) goto label$454;
						{
							ASTDELNODE( R$1 );
							ASTDELNODE( N$1 );
							fb$result$1 = L$1;
							goto label$425;
						}
						label$454:;
						label$453:;
					}
					goto label$435;
					label$455:;
					{
						if( V$1 != -1ll) goto label$457;
						{
							if( KEEP_L$1 != 0ll) goto label$459;
							{
								ASTDELTREE( L$1 );
								ASTDELNODE( N$1 );
								fb$result$1 = R$1;
								goto label$425;
							}
							label$459:;
							label$458:;
						}
						label$457:;
						label$456:;
					}
					goto label$435;
					label$460:;
					{
						if( V$1 != 0ll) goto label$462;
						{
							ASTDELNODE( R$1 );
							ASTDELNODE( N$1 );
							fb$result$1 = L$1;
							goto label$425;
						}
						goto label$461;
						label$462:;
						if( V$1 != -1ll) goto label$463;
						{
							if( KEEP_L$1 != 0ll) goto label$465;
							{
								ASTDELTREE( L$1 );
								ASTDELNODE( N$1 );
								fb$result$1 = R$1;
								goto label$425;
							}
							label$465:;
							label$464:;
						}
						label$463:;
						label$461:;
					}
					goto label$435;
					label$466:;
					{
						if( V$1 != -1ll) goto label$468;
						{
							ASTDELNODE( R$1 );
							ASTDELNODE( N$1 );
							fb$result$1 = L$1;
							goto label$425;
						}
						goto label$467;
						label$468:;
						if( V$1 != 0ll) goto label$469;
						{
							if( KEEP_L$1 != 0ll) goto label$471;
							{
								ASTDELTREE( L$1 );
								ASTDELNODE( N$1 );
								fb$result$1 = R$1;
								goto label$425;
							}
							label$471:;
							label$470:;
						}
						label$469:;
						label$467:;
					}
					goto label$435;
					label$436:;
					static const void* tmp$183[15ll] = {
						&&label$452,
						&&label$452,
						&&label$437,
						&&label$435,
						&&label$449,
						&&label$443,
						&&label$466,
						&&label$460,
						&&label$435,
						&&label$435,
						&&label$452,
						&&label$435,
						&&label$455,
						&&label$452,
						&&label$452,
					};
					if( (TMP$136$5 - 28ull) > 14ull ) goto label$435;
					goto *tmp$183[TMP$136$5 - 28ull];
					label$435:;
				}
			}
			goto label$433;
			label$434:;
			if( *(int64*)L$1 != 16ll) goto label$472;
			{
				V$1 = *(int64*)((uint8*)L$1 + 40ll);
				{
					uint64 TMP$138$5;
					TMP$138$5 = (uint64)OP$1;
					goto label$474;
					label$475:;
					{
						if( V$1 != 0ll) goto label$477;
						{
							if( KEEP_R$1 != 0ll) goto label$479;
							{
								ASTDELTREE( R$1 );
								ASTDELNODE( N$1 );
								fb$result$1 = L$1;
								goto label$425;
							}
							label$479:;
							label$478:;
						}
						label$477:;
						label$476:;
					}
					goto label$473;
					label$474:;
					static const void* tmp$184[13ll] = {
						&&label$475,
						&&label$473,
						&&label$475,
						&&label$475,
						&&label$473,
						&&label$473,
						&&label$473,
						&&label$473,
						&&label$473,
						&&label$473,
						&&label$473,
						&&label$475,
						&&label$475,
					};
					if( (TMP$138$5 - 30ull) > 12ull ) goto label$473;
					goto *tmp$184[TMP$138$5 - 30ull];
					label$473:;
				}
			}
			label$472:;
			label$433:;
		}
		label$432:;
		label$431:;
	}
	label$429:;
	label$428:;
	struct $7ASTNODE* vr$38 = HOPTCONSTCONV( N$1 );
	fb$result$1 = vr$38;
	label$425:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTLOGIC( struct $7ASTNODE* N$1 )
{
	int64 TMP$139$1;
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$480:;
	struct $7ASTNODE* M$1;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 OP$1;
	int64 V$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$483;
	{
		fb$result$1 = N$1;
		goto label$481;
	}
	label$483:;
	label$482:;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 == (struct $7ASTNODE*)0ull) goto label$485;
	{
		struct $7ASTNODE* vr$2 = HOPTLOGIC( L$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$2;
		L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	}
	label$485:;
	label$484:;
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	if( R$1 == (struct $7ASTNODE*)0ull) goto label$487;
	{
		struct $7ASTNODE* vr$6 = HOPTLOGIC( R$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$6;
		R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	}
	label$487:;
	label$486:;
	if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$488;
	TMP$139$1 = 24ll;
	goto label$825;
	label$488:;
	TMP$139$1 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
	label$825:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$139$1 * 56ll)) != 0ll) goto label$490;
	{
		int64 TMP$140$2;
		if( *(int64*)N$1 != 4ll) goto label$491;
		TMP$140$2 = (int64)-(*(int64*)((uint8*)N$1 + 40ll) == 52ll);
		goto label$826;
		label$491:;
		TMP$140$2 = 0ll;
		label$826:;
		if( TMP$140$2 == 0ll) goto label$493;
		{
			int64 TMP$141$3;
			int64 TMP$142$3;
			if( *(int64*)L$1 != 4ll) goto label$494;
			TMP$141$3 = (int64)-(*(int64*)((uint8*)L$1 + 40ll) == 52ll);
			goto label$827;
			label$494:;
			TMP$141$3 = 0ll;
			label$827:;
			if( TMP$141$3 == 0ll) goto label$496;
			{
				M$1 = *(struct $7ASTNODE**)((uint8*)L$1 + 112ll);
				ASTDELNODE( L$1 );
				ASTDELNODE( N$1 );
				struct $7ASTNODE* vr$23 = HOPTLOGIC( M$1 );
				N$1 = vr$23;
			}
			goto label$495;
			label$496:;
			if( *(int64*)L$1 != 3ll) goto label$498;
			TMP$142$3 = (int64)-(*(int64*)((uint8*)L$1 + 40ll) == 38ll);
			goto label$828;
			label$498:;
			TMP$142$3 = 0ll;
			label$828:;
			if( TMP$142$3 == 0ll) goto label$497;
			{
				int64 TMP$143$4;
				if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$499;
				TMP$143$4 = 24ll;
				goto label$829;
				label$499:;
				TMP$143$4 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
				label$829:;
				if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$143$4 * 56ll)) != 0ll) goto label$501;
				{
					if( *(int64*)*(struct $7ASTNODE**)((uint8*)L$1 + 112ll) != 16ll) goto label$503;
					{
						*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)L$1 + 112ll) + 40ll) = ~(*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)L$1 + 112ll) + 40ll));
						ASTDELNODE( N$1 );
						struct $7ASTNODE* vr$41 = HOPTLOGIC( L$1 );
						N$1 = vr$41;
					}
					goto label$502;
					label$503:;
					if( *(int64*)*(struct $7ASTNODE**)((uint8*)L$1 + 120ll) != 16ll) goto label$504;
					{
						*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)L$1 + 120ll) + 40ll) = ~(*(int64*)((uint8*)*(struct $7ASTNODE**)((uint8*)L$1 + 120ll) + 40ll));
						ASTDELNODE( N$1 );
						struct $7ASTNODE* vr$49 = HOPTLOGIC( L$1 );
						N$1 = vr$49;
					}
					label$504:;
					label$502:;
				}
				label$501:;
				label$500:;
			}
			label$497:;
			label$495:;
		}
		goto label$492;
		label$493:;
		if( *(int64*)N$1 != 3ll) goto label$505;
		{
			int64 TMP$144$3;
			if( ((*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$506;
			TMP$144$3 = 24ll;
			goto label$830;
			label$506:;
			TMP$144$3 = (*(int64*)((uint8*)N$1 + 8ll) & 511ll) & 31ll;
			label$830:;
			if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$144$3 * 56ll)) != 0ll) goto label$508;
			{
				OP$1 = *(int64*)((uint8*)N$1 + 40ll);
				{
					if( OP$1 == 35ll) goto label$511;
					label$512:;
					if( OP$1 == 34ll) goto label$511;
					label$513:;
					if( OP$1 != 38ll) goto label$510;
					label$511:;
					{
						int64 TMP$145$6;
						int64 TMP$146$6;
						int64 TMP$147$6;
						int64 TMP$148$6;
						int64 TMP$149$6;
						int64 TMP$150$6;
						if( OP$1 != 34ll) goto label$515;
						{
							OP$1 = 35ll;
						}
						goto label$514;
						label$515:;
						if( OP$1 != 35ll) goto label$516;
						{
							OP$1 = 34ll;
						}
						label$516:;
						label$514:;
						if( *(int64*)L$1 != 4ll) goto label$517;
						TMP$145$6 = (int64)-(*(int64*)((uint8*)L$1 + 40ll) == 52ll);
						goto label$831;
						label$517:;
						TMP$145$6 = 0ll;
						label$831:;
						if( *(int64*)R$1 != 4ll) goto label$518;
						TMP$146$6 = (int64)-(*(int64*)((uint8*)R$1 + 40ll) == 52ll);
						goto label$832;
						label$518:;
						TMP$146$6 = 0ll;
						label$832:;
						if( (TMP$145$6 & TMP$146$6) == 0ll) goto label$520;
						{
							struct $7ASTNODE* vr$67 = HOPTLOGIC( *(struct $7ASTNODE**)((uint8*)L$1 + 112ll) );
							L$1 = vr$67;
							struct $7ASTNODE* vr$69 = HOPTLOGIC( *(struct $7ASTNODE**)((uint8*)R$1 + 112ll) );
							R$1 = vr$69;
							struct $7ASTNODE* vr$70 = ASTNEWBOP( OP$1, L$1, R$1, (struct $8FBSYMBOL*)0ull, 1ll );
							M$1 = vr$70;
							if( OP$1 == 38ll) goto label$522;
							{
								struct $7ASTNODE* vr$71 = ASTNEWUOP( 52ll, M$1 );
								M$1 = vr$71;
							}
							label$522:;
							label$521:;
							ASTDELNODE( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
							ASTDELNODE( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
							ASTDELNODE( N$1 );
							N$1 = M$1;
						}
						goto label$519;
						label$520:;
						if( *(int64*)R$1 != 4ll) goto label$524;
						TMP$147$6 = (int64)-(*(int64*)((uint8*)R$1 + 40ll) == 52ll);
						goto label$833;
						label$524:;
						TMP$147$6 = 0ll;
						label$833:;
						if( ((int64)-(*(int64*)L$1 == 16ll) & TMP$147$6) == 0ll) goto label$523;
						{
							V$1 = *(int64*)((uint8*)L$1 + 40ll);
							struct $7ASTNODE* vr$82 = ASTNEWBOP( OP$1, L$1, *(struct $7ASTNODE**)((uint8*)R$1 + 112ll), (struct $8FBSYMBOL*)0ull, 1ll );
							M$1 = vr$82;
							*(int64*)((uint8*)L$1 + 40ll) = ~V$1;
							if( OP$1 == 38ll) goto label$526;
							{
								struct $7ASTNODE* vr$85 = ASTNEWUOP( 52ll, M$1 );
								M$1 = vr$85;
							}
							label$526:;
							label$525:;
							ASTDELNODE( R$1 );
							ASTDELNODE( N$1 );
							N$1 = M$1;
						}
						goto label$519;
						label$523:;
						if( *(int64*)L$1 != 4ll) goto label$528;
						TMP$148$6 = (int64)-(*(int64*)((uint8*)L$1 + 40ll) == 52ll);
						goto label$834;
						label$528:;
						TMP$148$6 = 0ll;
						label$834:;
						if( ((int64)-(*(int64*)R$1 == 16ll) & TMP$148$6) == 0ll) goto label$527;
						{
							V$1 = *(int64*)((uint8*)R$1 + 40ll);
							struct $7ASTNODE* vr$94 = ASTNEWBOP( OP$1, *(struct $7ASTNODE**)((uint8*)L$1 + 112ll), R$1, (struct $8FBSYMBOL*)0ull, 1ll );
							M$1 = vr$94;
							*(int64*)((uint8*)R$1 + 40ll) = ~V$1;
							if( OP$1 == 38ll) goto label$530;
							{
								struct $7ASTNODE* vr$97 = ASTNEWUOP( 52ll, M$1 );
								M$1 = vr$97;
							}
							label$530:;
							label$529:;
							ASTDELNODE( L$1 );
							ASTDELNODE( N$1 );
							N$1 = M$1;
						}
						goto label$519;
						label$527:;
						if( *(int64*)L$1 != 4ll) goto label$532;
						TMP$149$6 = (int64)-(*(int64*)((uint8*)L$1 + 40ll) == 52ll);
						goto label$835;
						label$532:;
						TMP$149$6 = 0ll;
						label$835:;
						if( ((int64)-(OP$1 == 38ll) & TMP$149$6) == 0ll) goto label$531;
						{
							struct $7ASTNODE* vr$103 = ASTNEWUOP( 52ll, N$1 );
							M$1 = vr$103;
							*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = *(struct $7ASTNODE**)((uint8*)L$1 + 112ll);
							ASTDELNODE( L$1 );
							N$1 = M$1;
						}
						goto label$519;
						label$531:;
						if( *(int64*)R$1 != 4ll) goto label$534;
						TMP$150$6 = (int64)-(*(int64*)((uint8*)R$1 + 40ll) == 52ll);
						goto label$836;
						label$534:;
						TMP$150$6 = 0ll;
						label$836:;
						if( ((int64)-(OP$1 == 38ll) & TMP$150$6) == 0ll) goto label$533;
						{
							struct $7ASTNODE* vr$111 = ASTNEWUOP( 52ll, N$1 );
							M$1 = vr$111;
							*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = *(struct $7ASTNODE**)((uint8*)R$1 + 112ll);
							ASTDELNODE( R$1 );
							N$1 = M$1;
						}
						label$533:;
						label$519:;
					}
					label$510:;
					label$509:;
				}
			}
			label$508:;
			label$507:;
		}
		label$505:;
		label$492:;
	}
	label$490:;
	label$489:;
	fb$result$1 = N$1;
	label$481:;
	return fb$result$1;
}

static struct $7ASTNODE* HDOOPTREMCONV( struct $7ASTNODE* N$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$535:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	int64 DOREM$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$538;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$536;
	}
	label$538:;
	label$537:;
	if( *(int64*)N$1 != 3ll) goto label$540;
	{
		{
			int64 TMP$151$3;
			TMP$151$3 = *(int64*)((uint8*)N$1 + 8ll) & 511ll;
			if( TMP$151$3 == 15ll) goto label$543;
			label$544:;
			if( TMP$151$3 != 16ll) goto label$542;
			label$543:;
			{
				R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
				if( *(int64*)R$1 != 5ll) goto label$546;
				{
					if( *(int64*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == 5ll) goto label$548;
					{
						{
							int64 TMP$152$7;
							TMP$152$7 = *(int64*)((uint8*)R$1 + 8ll) & 511ll;
							if( TMP$152$7 == 15ll) goto label$551;
							label$552:;
							if( TMP$152$7 != 16ll) goto label$550;
							label$551:;
							{
								L$1 = *(struct $7ASTNODE**)((uint8*)R$1 + 112ll);
								{
									int64 TMP$153$9;
									int64 TMP$154$9;
									if( ((*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$553;
									TMP$153$9 = 24ll;
									goto label$837;
									label$553:;
									TMP$153$9 = (*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 31ll;
									label$837:;
									TMP$154$9 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$153$9 * 56ll)) + 8ll);
									if( TMP$154$9 == 2ll) goto label$556;
									label$557:;
									if( TMP$154$9 != 4ll) goto label$555;
									label$556:;
									{
										DOREM$1 = 0ll;
										{
											$13AST_NODECLASS TMP$155$11;
											TMP$155$11 = *($13AST_NODECLASS*)L$1;
											if( TMP$155$11 == 17ll) goto label$560;
											label$561:;
											if( TMP$155$11 == 18ll) goto label$560;
											label$562:;
											if( TMP$155$11 == 19ll) goto label$560;
											label$563:;
											if( TMP$155$11 != 20ll) goto label$559;
											label$560:;
											{
												int64 TMP$156$12;
												if( ((*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$564;
												TMP$156$12 = 24ll;
												goto label$838;
												label$564:;
												TMP$156$12 = (*(int64*)((uint8*)L$1 + 8ll) & 511ll) & 31ll;
												label$838:;
												if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$156$12 * 56ll)) + 16ll) == 0ll) goto label$566;
												{
													DOREM$1 = -1ll;
												}
												label$566:;
												label$565:;
											}
											label$559:;
											label$558:;
										}
										if( DOREM$1 == 0ll) goto label$568;
										{
											ASTDELNODE( R$1 );
											*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = L$1;
										}
										label$568:;
										label$567:;
									}
									label$555:;
									label$554:;
								}
							}
							label$550:;
							label$549:;
						}
					}
					label$548:;
					label$547:;
				}
				label$546:;
				label$545:;
			}
			label$542:;
			label$541:;
		}
	}
	label$540:;
	label$539:;
	struct $7ASTNODE* vr$28 = HDOOPTREMCONV( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
	*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$28;
	struct $7ASTNODE* vr$31 = HDOOPTREMCONV( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
	*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$31;
	fb$result$1 = N$1;
	label$536:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTSTRMULTCONCAT( struct $7ASTNODE* LNK$1, struct $7ASTNODE* DST$1, struct $7ASTNODE* N$1, int64 IS_WSTR$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$569:;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$572;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$570;
	}
	label$572:;
	label$571:;
	if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$574;
	{
		if( *(int64*)*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) != 3ll) goto label$576;
		{
			struct $7ASTNODE* vr$5 = HOPTSTRMULTCONCAT( LNK$1, DST$1, *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), IS_WSTR$1 );
			LNK$1 = vr$5;
			*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = (struct $7ASTNODE*)0ull;
		}
		label$576:;
		label$575:;
	}
	label$574:;
	label$573:;
	if( *(int64*)N$1 != 3ll) goto label$578;
	{
		if( *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) == (struct $7ASTNODE*)0ull) goto label$580;
		{
			if( LNK$1 != (struct $7ASTNODE*)0ull) goto label$582;
			{
				if( IS_WSTR$1 != 0ll) goto label$584;
				{
					struct $7ASTNODE* vr$10 = ASTCLONETREE( DST$1 );
					struct $7ASTNODE* vr$11 = RTLSTRASSIGN( vr$10, *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), 0ll );
					LNK$1 = vr$11;
				}
				goto label$583;
				label$584:;
				{
					struct $7ASTNODE* vr$13 = ASTCLONETREE( DST$1 );
					struct $7ASTNODE* vr$14 = RTLWSTRASSIGN( vr$13, *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), 0ll );
					LNK$1 = vr$14;
				}
				label$583:;
			}
			goto label$581;
			label$582:;
			{
				if( IS_WSTR$1 != 0ll) goto label$586;
				{
					struct $7ASTNODE* vr$16 = ASTCLONETREE( DST$1 );
					struct $7ASTNODE* vr$17 = RTLSTRCONCATASSIGN( vr$16, *(struct $7ASTNODE**)((uint8*)N$1 + 112ll), 0ll );
					struct $7ASTNODE* vr$18 = ASTNEWLINK( LNK$1, vr$17, 0ll );
					LNK$1 = vr$18;
				}
				goto label$585;
				label$586:;
				{
					struct $7ASTNODE* vr$20 = ASTCLONETREE( DST$1 );
					struct $7ASTNODE* vr$21 = RTLWSTRCONCATASSIGN( vr$20, *(struct $7ASTNODE**)((uint8*)N$1 + 112ll) );
					struct $7ASTNODE* vr$22 = ASTNEWLINK( LNK$1, vr$21, 0ll );
					LNK$1 = vr$22;
				}
				label$585:;
			}
			label$581:;
		}
		label$580:;
		label$579:;
		if( *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) == (struct $7ASTNODE*)0ull) goto label$588;
		{
			if( IS_WSTR$1 != 0ll) goto label$590;
			{
				struct $7ASTNODE* vr$25 = ASTCLONETREE( DST$1 );
				struct $7ASTNODE* vr$26 = RTLSTRCONCATASSIGN( vr$25, *(struct $7ASTNODE**)((uint8*)N$1 + 120ll), 0ll );
				struct $7ASTNODE* vr$27 = ASTNEWLINK( LNK$1, vr$26, 0ll );
				LNK$1 = vr$27;
			}
			goto label$589;
			label$590:;
			{
				struct $7ASTNODE* vr$29 = ASTCLONETREE( DST$1 );
				struct $7ASTNODE* vr$30 = RTLWSTRCONCATASSIGN( vr$29, *(struct $7ASTNODE**)((uint8*)N$1 + 120ll) );
				struct $7ASTNODE* vr$31 = ASTNEWLINK( LNK$1, vr$30, 0ll );
				LNK$1 = vr$31;
			}
			label$589:;
		}
		label$588:;
		label$587:;
		ASTDELNODE( N$1 );
	}
	goto label$577;
	label$578:;
	{
		if( LNK$1 != (struct $7ASTNODE*)0ull) goto label$592;
		{
			if( IS_WSTR$1 != 0ll) goto label$594;
			{
				struct $7ASTNODE* vr$32 = ASTCLONETREE( DST$1 );
				struct $7ASTNODE* vr$33 = RTLSTRASSIGN( vr$32, N$1, 0ll );
				LNK$1 = vr$33;
			}
			goto label$593;
			label$594:;
			{
				struct $7ASTNODE* vr$34 = ASTCLONETREE( DST$1 );
				struct $7ASTNODE* vr$35 = RTLWSTRASSIGN( vr$34, N$1, 0ll );
				LNK$1 = vr$35;
			}
			label$593:;
		}
		goto label$591;
		label$592:;
		{
			if( IS_WSTR$1 != 0ll) goto label$596;
			{
				struct $7ASTNODE* vr$36 = ASTCLONETREE( DST$1 );
				struct $7ASTNODE* vr$37 = RTLSTRCONCATASSIGN( vr$36, N$1, 0ll );
				struct $7ASTNODE* vr$38 = ASTNEWLINK( LNK$1, vr$37, 0ll );
				LNK$1 = vr$38;
			}
			goto label$595;
			label$596:;
			{
				struct $7ASTNODE* vr$39 = ASTCLONETREE( DST$1 );
				struct $7ASTNODE* vr$40 = RTLWSTRCONCATASSIGN( vr$39, N$1 );
				struct $7ASTNODE* vr$41 = ASTNEWLINK( LNK$1, vr$40, 0ll );
				LNK$1 = vr$41;
			}
			label$595:;
		}
		label$591:;
	}
	label$577:;
	fb$result$1 = LNK$1;
	label$570:;
	return fb$result$1;
}

static int64 HISMULTSTRCONCAT( struct $7ASTNODE* L$1, struct $7ASTNODE* R$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$597:;
	struct $8FBSYMBOL* SYM$1;
	fb$result$1 = 0ll;
	if( *(int64*)R$1 != 3ll) goto label$600;
	{
		{
			$13AST_NODECLASS TMP$157$3;
			TMP$157$3 = *($13AST_NODECLASS*)L$1;
			if( TMP$157$3 == 17ll) goto label$603;
			label$604:;
			if( TMP$157$3 != 18ll) goto label$602;
			label$603:;
			{
				SYM$1 = *(struct $8FBSYMBOL**)((uint8*)L$1 + 24ll);
				if( SYM$1 == (struct $8FBSYMBOL*)0ull) goto label$606;
				{
					if( (*(int64*)((uint8*)SYM$1 + 8ll) & 81920ll) != 0ll) goto label$608;
					{
						int64 vr$6 = ASTISSYMBOLONTREE( SYM$1, R$1 );
						fb$result$1 = (int64)-(vr$6 == 0ll);
					}
					label$608:;
					label$607:;
				}
				label$606:;
				label$605:;
			}
			goto label$601;
			label$602:;
			if( TMP$157$3 == 19ll) goto label$610;
			label$611:;
			if( TMP$157$3 != 26ll) goto label$609;
			label$610:;
			{
				{
					$13AST_NODECLASS TMP$158$5;
					TMP$158$5 = *($13AST_NODECLASS*)*(struct $7ASTNODE**)((uint8*)L$1 + 112ll);
					if( TMP$158$5 == 17ll) goto label$614;
					label$615:;
					if( TMP$158$5 != 18ll) goto label$613;
					label$614:;
					{
						SYM$1 = *(struct $8FBSYMBOL**)((uint8*)L$1 + 24ll);
						if( SYM$1 == (struct $8FBSYMBOL*)0ull) goto label$617;
						{
							int64 vr$11 = ASTISSYMBOLONTREE( SYM$1, R$1 );
							fb$result$1 = (int64)-(vr$11 == 0ll);
						}
						label$617:;
						label$616:;
					}
					label$613:;
					label$612:;
				}
			}
			label$609:;
			label$601:;
		}
	}
	label$600:;
	label$599:;
	label$598:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTSTRASSIGNMENT( struct $7ASTNODE* N$1, struct $7ASTNODE* L$1, struct $7ASTNODE* R$1 )
{
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$618:;
	int64 OPTIMIZE$1;
	int64 IS_BYREF$1;
	int64 IS_WSTR$1;
	OPTIMIZE$1 = 0ll;
	IS_BYREF$1 = 0ll;
	if( *(int64*)R$1 != 3ll) goto label$621;
	{
		struct $8FBSYMBOL* SYM$2;
		__builtin_memset( &SYM$2, 0, 8ll );
		{
			uint64 TMP$159$3;
			TMP$159$3 = *(uint64*)L$1;
			goto label$623;
			label$624:;
			{
				int64 vr$5 = ASTISTREEEQUAL( L$1, *(struct $7ASTNODE**)((uint8*)R$1 + 112ll) );
				if( vr$5 == 0ll) goto label$626;
				{
					SYM$2 = *(struct $8FBSYMBOL**)((uint8*)L$1 + 24ll);
					if( SYM$2 == (struct $8FBSYMBOL*)0ull) goto label$628;
					{
						if( (*(int64*)((uint8*)SYM$2 + 8ll) & 81920ll) != 0ll) goto label$630;
						{
							int64 vr$10 = ASTISSYMBOLONTREE( SYM$2, *(struct $7ASTNODE**)((uint8*)R$1 + 120ll) );
							OPTIMIZE$1 = (int64)-(vr$10 == 0ll);
						}
						label$630:;
						label$629:;
					}
					label$628:;
					label$627:;
				}
				label$626:;
				label$625:;
			}
			goto label$622;
			label$631:;
			{
				int64 vr$13 = ASTISTREEEQUAL( L$1, *(struct $7ASTNODE**)((uint8*)R$1 + 112ll) );
				if( vr$13 == 0ll) goto label$633;
				{
					SYM$2 = *(struct $8FBSYMBOL**)((uint8*)*(struct $7ASTNODE**)((uint8*)L$1 + 112ll) + 24ll);
					if( SYM$2 == (struct $8FBSYMBOL*)0ull) goto label$635;
					{
						if( (*(int64*)((uint8*)L$1 + 8ll) & 511ll) != 17ll) goto label$637;
						{
							int64 vr$19 = ASTISSYMBOLONTREE( SYM$2, *(struct $7ASTNODE**)((uint8*)R$1 + 120ll) );
							OPTIMIZE$1 = (int64)-(vr$19 == 0ll);
							IS_BYREF$1 = -1ll;
						}
						label$637:;
						label$636:;
					}
					label$635:;
					label$634:;
				}
				label$633:;
				label$632:;
			}
			goto label$622;
			label$638:;
			{
				{
					uint64 TMP$160$5;
					TMP$160$5 = *(uint64*)*(struct $7ASTNODE**)((uint8*)L$1 + 112ll);
					goto label$640;
					label$641:;
					{
						int64 vr$24 = ASTISTREEEQUAL( L$1, *(struct $7ASTNODE**)((uint8*)R$1 + 112ll) );
						if( vr$24 == 0ll) goto label$643;
						{
							SYM$2 = *(struct $8FBSYMBOL**)((uint8*)L$1 + 24ll);
							if( SYM$2 == (struct $8FBSYMBOL*)0ull) goto label$645;
							{
								if( (*(int64*)((uint8*)L$1 + 8ll) & 511ll) != 17ll) goto label$647;
								{
									IS_BYREF$1 = -1ll;
								}
								label$647:;
								label$646:;
								int64 vr$29 = ASTISSYMBOLONTREE( SYM$2, *(struct $7ASTNODE**)((uint8*)R$1 + 120ll) );
								OPTIMIZE$1 = (int64)-(vr$29 == 0ll);
							}
							label$645:;
							label$644:;
						}
						label$643:;
						label$642:;
					}
					goto label$639;
					label$640:;
					static const void* tmp$185[2ll] = {
						&&label$641,
						&&label$641,
					};
					if( (TMP$160$5 - 17ull) > 1ull ) goto label$639;
					goto *tmp$185[TMP$160$5 - 17ull];
					label$639:;
				}
			}
			goto label$622;
			label$623:;
			static const void* tmp$186[10ll] = {
				&&label$624,
				&&label$624,
				&&label$638,
				&&label$631,
				&&label$622,
				&&label$622,
				&&label$622,
				&&label$622,
				&&label$622,
				&&label$638,
			};
			if( (TMP$159$3 - 17ull) > 9ull ) goto label$622;
			goto *tmp$186[TMP$159$3 - 17ull];
			label$622:;
		}
	}
	label$621:;
	label$620:;
	IS_WSTR$1 = (int64)-((*(int64*)((uint8*)N$1 + 8ll) & 511ll) == 7ll);
	if( OPTIMIZE$1 == 0ll) goto label$649;
	{
		ASTDELNODE( N$1 );
		N$1 = R$1;
		ASTDELTREE( L$1 );
		L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
		R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
		int64 vr$36 = HISMULTSTRCONCAT( L$1, R$1 );
		if( vr$36 == 0ll) goto label$651;
		{
			struct $7ASTNODE* vr$37 = HOPTSTRMULTCONCAT( L$1, L$1, R$1, IS_WSTR$1 );
			fb$result$1 = vr$37;
		}
		goto label$650;
		label$651:;
		{
			if( IS_WSTR$1 != 0ll) goto label$653;
			{
				struct $7ASTNODE* vr$38 = ASTUPDSTRCONCAT( R$1 );
				struct $7ASTNODE* vr$39 = RTLSTRCONCATASSIGN( L$1, vr$38, IS_BYREF$1 );
				fb$result$1 = vr$39;
			}
			goto label$652;
			label$653:;
			{
				struct $7ASTNODE* vr$40 = ASTUPDSTRCONCAT( R$1 );
				struct $7ASTNODE* vr$41 = RTLWSTRCONCATASSIGN( L$1, vr$40 );
				fb$result$1 = vr$41;
			}
			label$652:;
		}
		label$650:;
	}
	goto label$648;
	label$649:;
	{
		{
			int64 TMP$161$3;
			TMP$161$3 = *(int64*)((uint8*)L$1 + 8ll) & 511ll;
			if( TMP$161$3 != 18ll) goto label$655;
			label$656:;
			{
			}
			goto label$654;
			label$655:;
			{
				int64 vr$44 = HISMULTSTRCONCAT( L$1, R$1 );
				OPTIMIZE$1 = vr$44;
			}
			label$657:;
			label$654:;
		}
		if( OPTIMIZE$1 == 0ll) goto label$659;
		{
			struct $7ASTNODE* vr$45 = HOPTSTRMULTCONCAT( (struct $7ASTNODE*)0ull, L$1, R$1, IS_WSTR$1 );
			fb$result$1 = vr$45;
		}
		goto label$658;
		label$659:;
		{
			if( IS_WSTR$1 != 0ll) goto label$661;
			{
				struct $7ASTNODE* vr$46 = ASTUPDSTRCONCAT( R$1 );
				struct $7ASTNODE* vr$47 = RTLSTRASSIGN( L$1, vr$46, 0ll );
				fb$result$1 = vr$47;
			}
			goto label$660;
			label$661:;
			{
				struct $7ASTNODE* vr$48 = ASTUPDSTRCONCAT( R$1 );
				struct $7ASTNODE* vr$49 = RTLWSTRASSIGN( L$1, vr$48, 0ll );
				fb$result$1 = vr$49;
			}
			label$660:;
		}
		label$658:;
	}
	label$648:;
	ASTDELNODE( N$1 );
	label$619:;
	return fb$result$1;
}

static struct $7ASTNODE* HOPTRECIPROCAL( struct $7ASTNODE* N$1 )
{
	int64 TMP$176$1;
	struct $7ASTNODE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$756:;
	struct $7ASTNODE* L$1;
	struct $7ASTNODE* R$1;
	float V$1;
	if( N$1 != (struct $7ASTNODE*)0ull) goto label$759;
	{
		fb$result$1 = (struct $7ASTNODE*)0ull;
		goto label$757;
	}
	label$759:;
	label$758:;
	if( *(int64*)N$1 != 3ll) goto label$760;
	TMP$176$1 = (int64)-(*(int64*)((uint8*)N$1 + 40ll) == 31ll);
	goto label$839;
	label$760:;
	TMP$176$1 = 0ll;
	label$839:;
	if( TMP$176$1 == 0ll) goto label$762;
	{
		L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
		if( *(int64*)L$1 != 16ll) goto label$764;
		{
			int64 TMP$177$3;
			if( (*(int64*)((uint8*)L$1 + 8ll) & 511ll) != 15ll) goto label$765;
			TMP$177$3 = (int64)-(*(double*)((uint8*)L$1 + 40ll) == 0x1.p+0);
			goto label$840;
			label$765:;
			TMP$177$3 = 0ll;
			label$840:;
			if( TMP$177$3 == 0ll) goto label$767;
			{
				R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
				{
					int64 TMP$178$5;
					uint64 TMP$179$5;
					if( ((*(int64*)((uint8*)R$1 + 8ll) & 511ll) & 480ll) == 0ll) goto label$768;
					TMP$178$5 = 24ll;
					goto label$841;
					label$768:;
					TMP$178$5 = (*(int64*)((uint8*)R$1 + 8ll) & 511ll) & 31ll;
					label$841:;
					TMP$179$5 = *(uint64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$178$5 * 56ll));
					goto label$770;
					label$771:;
					{
						int64 TMP$180$6;
						if( *(int64*)R$1 != 4ll) goto label$772;
						TMP$180$6 = (int64)-(*(int64*)((uint8*)R$1 + 40ll) == 66ll);
						goto label$842;
						label$772:;
						TMP$180$6 = 0ll;
						label$842:;
						if( TMP$180$6 == 0ll) goto label$774;
						{
							__builtin_memcpy( N$1, R$1, 144 );
							*($13AST_NODECLASS*)N$1 = 4ll;
							*(int64*)((uint8*)N$1 + 40ll) = 67ll;
							ASTDELNODE( R$1 );
							ASTDELNODE( L$1 );
						}
						goto label$773;
						label$774:;
						if( (*(int64*)((uint8*)R$1 + 8ll) & 511ll) != 15ll) goto label$775;
						{
							ASTDELNODE( N$1 );
							struct $7ASTNODE* vr$25 = ASTNEWUOP( 68ll, R$1 );
							N$1 = vr$25;
							ASTDELNODE( L$1 );
						}
						label$775:;
						label$773:;
					}
					goto label$769;
					label$770:;
					static const void* tmp$187[2ll] = {
						&&label$771,
						&&label$771,
					};
					if( TMP$179$5 > 1ull ) goto label$769;
					goto *tmp$187[TMP$179$5 - 0ull];
					label$769:;
				}
			}
			label$767:;
			label$766:;
		}
		label$764:;
		label$763:;
	}
	label$762:;
	label$761:;
	L$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 112ll);
	if( L$1 == (struct $7ASTNODE*)0ull) goto label$777;
	{
		struct $7ASTNODE* vr$27 = HOPTRECIPROCAL( L$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 112ll) = vr$27;
	}
	label$777:;
	label$776:;
	R$1 = *(struct $7ASTNODE**)((uint8*)N$1 + 120ll);
	if( R$1 == (struct $7ASTNODE*)0ull) goto label$779;
	{
		struct $7ASTNODE* vr$30 = HOPTRECIPROCAL( R$1 );
		*(struct $7ASTNODE**)((uint8*)N$1 + 120ll) = vr$30;
	}
	label$779:;
	label$778:;
	fb$result$1 = N$1;
	label$757:;
	return fb$result$1;
}
