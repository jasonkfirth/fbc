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
typedef int64 $15IRVREGTYPE_ENUM;
typedef int64 $11FB_DATATYPE;
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
struct $12EMITDATATYPE {
	int64 RNAMETB;
	char MNAME[12];
};
__FB_STATIC_ASSERT( sizeof( struct $12EMITDATATYPE ) == 24 );
typedef struct $6IRVREG* (*tmp$73)( int64, struct $8FBSYMBOL*, struct $8FBSYMBOL*, int64 );
typedef int64 $7CMPF_OP;
struct $11CMPF_RECIPE {
	$7CMPF_OP OP;
	char* MNEMONIC;
	char* REV_MNEMONIC;
	char* MSK_MNEMONIC;
	char* MASK;
	int64 PARITY_FALSE;
	int64 PARITY_TRUE;
	int64 SWAPREGS;
	int64 SWAPINIT;
};
__FB_STATIC_ASSERT( sizeof( struct $11CMPF_RECIPE ) == 72 );
typedef int64 $10IR_EMITOPT;
struct $16__FB_ARRAYDIMTB$ {
	int64 ELEMENTS;
	int64 LBOUND;
	int64 UBOUND;
};
__FB_STATIC_ASSERT( sizeof( struct $16__FB_ARRAYDIMTB$ ) == 24 );
struct $8FBARRAY1I11CMPF_RECIPEE {
	struct $11CMPF_RECIPE* DATA;
	struct $11CMPF_RECIPE* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1I11CMPF_RECIPEE ) == 72 );
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
struct $8FBARRAY1I8FBSTRINGE {
	FBSTRING* DATA;
	FBSTRING* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1I8FBSTRINGE ) == 72 );
FBSTRING* fb_StrAssign( void*, int64, void*, int64, int32 );
void fb_StrDelete( FBSTRING* );
FBSTRING* fb_StrConcat( FBSTRING*, void*, int64, void*, int64 );
FBSTRING* fb_StrConcatAssign( void*, int64, void*, int64, int32 );
FBSTRING* fb_LongintToStr( int64 );
FBSTRING* fb_CHR( int32, ... );
static void fb_ctor__emit_SSE( void ) __attribute__(( constructor ));
static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM*, struct $11TSTRSETITEM* );
struct $8FBSYMBOL* SYMBALLOCINTCONST( int64, int64 );
char* SYMBUNIQUELABEL( void );
char* SYMBGETMANGLEDNAME( struct $8FBSYMBOL* );
void HPREPOPERAND( struct $6IRVREG*, FBSTRING*, $11FB_DATATYPE, int64, int64, int64 );
void HPREPOPERAND64( struct $6IRVREG*, FBSTRING*, FBSTRING* );
void HPUSH( char* );
void HPOP( char* );
int64 HFINDREGNOTINVREG( struct $6IRVREG*, int64 );
int64 HFINDFREEREG( int64 );
int64 HISREGFREE( int64, int64 );
char* HGETREGNAME( int64, int64 );
void OUTP( char* );
void HLABEL( char* );
void HCMPF_JXX( struct $11CMPF_RECIPE*, FBSTRING* );
void HCMPF_SET( struct $6IRVREG*, struct $11CMPF_RECIPE*, FBSTRING* );
static void HULONG2DBL( struct $6IRVREG* );
static void _EMITLOADB2F_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void HMOVESSEREGTOFPUSTACK( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADF2B_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORF2L_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORF2I_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORL2F_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORI2F_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void HEMITSTOREFREG2F_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORF2F_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADF2L_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADF2I_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADL2F_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADI2F_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADF2F_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMOVF_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSWZREPF_SSE( struct $6IRVREG* );
static int64 HEMITCONVERTOPERANDS_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITADDF_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSUBF_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMULF_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITDIVF_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITATN2_SSE( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITPOW_SSE( struct $6IRVREG*, struct $6IRVREG* );
static struct $11CMPF_RECIPE* HCMPF_GET_RECIPE( $7CMPF_OP, $10IR_EMITOPT, struct $8FBSYMBOL* );
static void HCMPF_SSE( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, struct $11CMPF_RECIPE* );
static void _EMITCGTF_SSE( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCLTF_SSE( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCEQF_SSE( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCNEF_SSE( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCLEF_SSE( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCGEF_SSE( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITNEGF_SSE( struct $6IRVREG* );
static void _EMITHADDF_SSE( struct $6IRVREG* );
static void _EMITABSF_SSE( struct $6IRVREG* );
static void _EMITSGNF_SSE( struct $6IRVREG* );
static void HSINCOS_FAST_SSE( struct $6IRVREG*, int64 );
static void _EMITSIN_SSE( struct $6IRVREG* );
static void _EMITASIN_SSE( struct $6IRVREG* );
static void _EMITCOS_SSE( struct $6IRVREG* );
static void _EMITACOS_SSE( struct $6IRVREG* );
static void _EMITTAN_SSE( struct $6IRVREG* );
static void _EMITATAN_SSE( struct $6IRVREG* );
static void _EMITSQRT_SSE( struct $6IRVREG* );
static void _EMITRSQRT_SSE( struct $6IRVREG* );
static void _EMITRCP_SSE( struct $6IRVREG* );
static void _EMITLOG_SSE( struct $6IRVREG* );
static void _EMITEXP_SSE( struct $6IRVREG* );
static void _EMITFLOOR_SSE( struct $6IRVREG* );
static void _EMITFIX_SSE( struct $6IRVREG* );
static void _EMITFRAC_SSE( struct $6IRVREG* );
static void _EMITPUSHF_SSE( struct $6IRVREG*, int64 );
static void _EMITPOPF_SSE( struct $6IRVREG*, int64 );
typedef void (*tmp$39)( void );
typedef int64 (*tmp$40)( void );
typedef int64 $14IR_OPTIONVALUE;
typedef int64 (*tmp$41)( $14IR_OPTIONVALUE );
typedef int64 (*tmp$42)( int64, int64 );
typedef void (*tmp$43)( struct $8FBSYMBOL* );
typedef void (*tmp$44)( struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef char* (*tmp$45)( void );
typedef void (*tmp$47)( int64 );
typedef void (*tmp$48)( struct $8FBSYMBOL*, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef void (*tmp$49)( struct $8FBSYMBOL*, struct $6IRVREG*, int64, int64, struct $6IRVREG* );
typedef void (*tmp$50)( struct $9ASTASMTOK* );
typedef void (*tmp$51)( char* );
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
static struct $8FBARRAY1IPvE tmp$107$;
extern struct $12EMITDATATYPE DTYPETB$[26];
struct $8FBARRAY1I12EMITDATATYPEE {
	struct $12EMITDATATYPE* DATA;
	struct $12EMITDATATYPE* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1I12EMITDATATYPEE ) == 72 );
static struct $8FBARRAY1I12EMITDATATYPEE tmp$109$;
static tmp$46 _EMITLOADB2F_X86$;
static tmp$46 _EMITLOADF2B_X86$;

int64 _INIT_OPFNTB_SSE( void** _OPFNTB_SSE$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$744:;
	if( _EMITLOADB2F_X86$ != (tmp$46)0ull) goto label$747;
	{
		_EMITLOADB2F_X86$ = *(tmp$46*)((uint8*)_OPFNTB_SSE$1 + 64ll);
	}
	label$747:;
	label$746:;
	if( _EMITLOADF2B_X86$ != (tmp$46)0ull) goto label$749;
	{
		_EMITLOADF2B_X86$ = *(tmp$46*)((uint8*)_OPFNTB_SSE$1 + 112ll);
	}
	label$749:;
	label$748:;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 16ll) = (void*)&_EMITLOADF2I_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 40ll) = (void*)&_EMITLOADI2F_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 80ll) = (void*)&_EMITLOADF2L_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 56ll) = (void*)&_EMITLOADL2F_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 48ll) = (void*)&_EMITLOADF2F_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 64ll) = (void*)&_EMITLOADB2F_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 112ll) = (void*)&_EMITLOADF2B_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 144ll) = (void*)&_EMITSTORF2I_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 168ll) = (void*)&_EMITSTORI2F_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 208ll) = (void*)&_EMITSTORF2L_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 184ll) = (void*)&_EMITSTORL2F_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 176ll) = (void*)&_EMITSTORF2F_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 272ll) = (void*)&_EMITMOVF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 296ll) = (void*)&_EMITADDF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 320ll) = (void*)&_EMITSUBF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 344ll) = (void*)&_EMITMULF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 368ll) = (void*)&_EMITDIVF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 520ll) = (void*)&_EMITATN2_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 528ll) = (void*)&_EMITPOW_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 560ll) = (void*)&_EMITCGTF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 584ll) = (void*)&_EMITCLTF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 608ll) = (void*)&_EMITCEQF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 632ll) = (void*)&_EMITCNEF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 656ll) = (void*)&_EMITCGEF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 680ll) = (void*)&_EMITCLEF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 704ll) = (void*)&_EMITNEGF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 736ll) = (void*)&_EMITHADDF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 752ll) = (void*)&_EMITABSF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 776ll) = (void*)&_EMITSGNF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 792ll) = (void*)&_EMITFIX_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 800ll) = (void*)&_EMITFRAC_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 824ll) = (void*)&_EMITSIN_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 832ll) = (void*)&_EMITASIN_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 840ll) = (void*)&_EMITCOS_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 848ll) = (void*)&_EMITACOS_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 856ll) = (void*)&_EMITTAN_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 864ll) = (void*)&_EMITATAN_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 872ll) = (void*)&_EMITSQRT_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 880ll) = (void*)&_EMITRSQRT_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 888ll) = (void*)&_EMITRCP_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 896ll) = (void*)&_EMITLOG_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 904ll) = (void*)&_EMITEXP_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 912ll) = (void*)&_EMITFLOOR_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 816ll) = (void*)&_EMITSWZREPF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 944ll) = (void*)&_EMITPUSHF_SSE;
	*(void**)((uint8*)_OPFNTB_SSE$1 + 968ll) = (void*)&_EMITPOPF_SSE;
	fb$result$1 = -1ll;
	label$745:;
	return fb$result$1;
}

__attribute__(( constructor )) static void fb_ctor__emit_SSE( void )
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

static void HULONG2DBL( struct $6IRVREG* SVREG$1 )
{
	label$10:;
	static FBSTRING LABEL$1;
	static FBSTRING AUX$1;
	static FBSTRING OSTR$1;
	char* vr$0 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&LABEL$1, -1ll, (void*)vr$0, 0ll, 0 );
	HPREPOPERAND( SVREG$1, &AUX$1, 8ll, 0ll, -1ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"cmp ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 0", 4ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"jns ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&LABEL$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	HPUSH( (char*)"0x403f" );
	HPUSH( (char*)"0x80000000" );
	HPUSH( (char*)"0" );
	OUTP( (char*)"fldt [esp]" );
	OUTP( (char*)"add esp, 12" );
	OUTP( (char*)"faddp" );
	HLABEL( (char*)*(char**)&LABEL$1 );
	label$11:;
}

static void _EMITLOADB2F_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$120$1;
	FBSTRING TMP$123$1;
	FBSTRING TMP$124$1;
	FBSTRING TMP$141$1;
	FBSTRING TMP$142$1;
	label$12:;
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	int64 DDSIZE$1;
	(_EMITLOADB2F_X86$)( DVREG$1, SVREG$1 );
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$14;
	TMP$120$1 = 24ll;
	goto label$750;
	label$14:;
	TMP$120$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$750:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$120$1 * 56ll)) + 8ll);
	__builtin_memset( &TMP$124$1, 0, 24ll );
	FBSTRING* vr$8 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$123$1, 0, 24ll );
	FBSTRING* vr$11 = fb_StrConcat( &TMP$123$1, (void*)"sub esp, ", 10ll, (void*)vr$8, -1ll );
	fb_StrAssign( (void*)&TMP$124$1, -1ll, (void*)vr$11, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$124$1 );
	fb_StrDelete( (FBSTRING*)&TMP$124$1 );
	if( DDSIZE$1 <= 4ll) goto label$16;
	{
		FBSTRING TMP$128$2;
		FBSTRING TMP$129$2;
		FBSTRING TMP$130$2;
		FBSTRING TMP$131$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$131$2, 0, 24ll );
		__builtin_memset( &TMP$128$2, 0, 24ll );
		FBSTRING* vr$18 = fb_StrConcat( &TMP$128$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$129$2, 0, 24ll );
		FBSTRING* vr$21 = fb_StrConcat( &TMP$129$2, (void*)vr$18, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$130$2, 0, 24ll );
		FBSTRING* vr$24 = fb_StrConcat( &TMP$130$2, (void*)vr$21, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$131$2, -1ll, (void*)vr$24, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$131$2 );
		fb_StrDelete( (FBSTRING*)&TMP$131$2 );
	}
	goto label$15;
	label$16:;
	{
		FBSTRING TMP$135$2;
		FBSTRING TMP$136$2;
		FBSTRING TMP$137$2;
		FBSTRING TMP$138$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$138$2, 0, 24ll );
		__builtin_memset( &TMP$135$2, 0, 24ll );
		FBSTRING* vr$31 = fb_StrConcat( &TMP$135$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$136$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$136$2, (void*)vr$31, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$137$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$137$2, (void*)vr$34, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$138$2, -1ll, (void*)vr$37, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$138$2 );
		fb_StrDelete( (FBSTRING*)&TMP$138$2 );
	}
	label$15:;
	__builtin_memset( &TMP$142$1, 0, 24ll );
	FBSTRING* vr$41 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$141$1, 0, 24ll );
	FBSTRING* vr$44 = fb_StrConcat( &TMP$141$1, (void*)"add esp, ", 10ll, (void*)vr$41, -1ll );
	fb_StrAssign( (void*)&TMP$142$1, -1ll, (void*)vr$44, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$142$1 );
	fb_StrDelete( (FBSTRING*)&TMP$142$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	label$13:;
}

static void HMOVESSEREGTOFPUSTACK( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$143$1;
	FBSTRING TMP$144$1;
	FBSTRING TMP$145$1;
	FBSTRING TMP$156$1;
	FBSTRING TMP$157$1;
	label$17:;
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	int64 SDSIZE$1;
	__builtin_memset( &SDSIZE$1, 0, 8ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$19;
	TMP$143$1 = 24ll;
	goto label$751;
	label$19:;
	TMP$143$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$751:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$143$1 * 56ll)) + 8ll);
	__builtin_memset( &TMP$145$1, 0, 24ll );
	FBSTRING* vr$8 = fb_LongintToStr( SDSIZE$1 );
	__builtin_memset( &TMP$144$1, 0, 24ll );
	FBSTRING* vr$11 = fb_StrConcat( &TMP$144$1, (void*)"sub esp, ", 10ll, (void*)vr$8, -1ll );
	fb_StrAssign( (void*)&TMP$145$1, -1ll, (void*)vr$11, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$145$1 );
	fb_StrDelete( (FBSTRING*)&TMP$145$1 );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( SDSIZE$1 <= 4ll) goto label$21;
	{
		FBSTRING TMP$148$2;
		FBSTRING TMP$149$2;
		__builtin_memset( &TMP$149$2, 0, 24ll );
		__builtin_memset( &TMP$148$2, 0, 24ll );
		FBSTRING* vr$19 = fb_StrConcat( &TMP$148$2, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$149$2, -1ll, (void*)vr$19, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$149$2 );
		fb_StrDelete( (FBSTRING*)&TMP$149$2 );
		OUTP( (char*)"fld qword ptr [esp]" );
	}
	goto label$20;
	label$21:;
	{
		FBSTRING TMP$153$2;
		FBSTRING TMP$154$2;
		__builtin_memset( &TMP$154$2, 0, 24ll );
		__builtin_memset( &TMP$153$2, 0, 24ll );
		FBSTRING* vr$26 = fb_StrConcat( &TMP$153$2, (void*)"movss dword ptr [esp], ", 24ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$154$2, -1ll, (void*)vr$26, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$154$2 );
		fb_StrDelete( (FBSTRING*)&TMP$154$2 );
		OUTP( (char*)"fld dword ptr [esp]" );
	}
	label$20:;
	__builtin_memset( &TMP$157$1, 0, 24ll );
	FBSTRING* vr$30 = fb_LongintToStr( SDSIZE$1 );
	__builtin_memset( &TMP$156$1, 0, 24ll );
	FBSTRING* vr$33 = fb_StrConcat( &TMP$156$1, (void*)"add esp, ", 10ll, (void*)vr$30, -1ll );
	fb_StrAssign( (void*)&TMP$157$1, -1ll, (void*)vr$33, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$157$1 );
	fb_StrDelete( (FBSTRING*)&TMP$157$1 );
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	label$18:;
}

static void _EMITLOADF2B_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$22:;
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 1ll) goto label$25;
	{
		if( *(int64*)SVREG$1 != 4ll) goto label$27;
		{
			HMOVESSEREGTOFPUSTACK( DVREG$1, SVREG$1 );
		}
		label$27:;
		label$26:;
	}
	label$25:;
	label$24:;
	(_EMITLOADF2B_X86$)( DVREG$1, SVREG$1 );
	label$23:;
}

static void _EMITSTORF2L_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$158$1;
	FBSTRING TMP$160$1;
	FBSTRING TMP$161$1;
	label$28:;
	static FBSTRING DST$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$30;
	TMP$158$1 = 24ll;
	goto label$752;
	label$30:;
	TMP$158$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$752:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$158$1 * 56ll)) + 16ll) != 0ll) goto label$32;
	{
		goto label$29;
		label$32:;
	}
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 1ll) goto label$34;
	{
		HMOVESSEREGTOFPUSTACK( DVREG$1, SVREG$1 );
	}
	label$34:;
	label$33:;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$161$1, 0, 24ll );
	__builtin_memset( &TMP$160$1, 0, 24ll );
	FBSTRING* vr$9 = fb_StrConcat( &TMP$160$1, (void*)"fistp ", 7ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$161$1, -1ll, (void*)vr$9, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$161$1 );
	fb_StrDelete( (FBSTRING*)&TMP$161$1 );
	label$29:;
}

static void _EMITSTORF2I_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$162$1;
	int64 TMP$163$1;
	int64 TMP$164$1;
	int64 TMP$175$1;
	label$35:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static FBSTRING OSTR$1;
	static FBSTRING AUX$1;
	static FBSTRING AUX8$1;
	static FBSTRING AUX16$1;
	static int64 ISFREE$1;
	static int64 REG$1;
	static int64 WASREG$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$37;
	TMP$162$1 = 24ll;
	goto label$753;
	label$37:;
	TMP$162$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$753:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$162$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$38;
	TMP$163$1 = 24ll;
	goto label$754;
	label$38:;
	TMP$163$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$754:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$163$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$39;
	TMP$164$1 = 24ll;
	goto label$755;
	label$39:;
	TMP$164$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$755:;
	if( ((int64)-(DDSIZE$1 == 4ll) & (int64)-(*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$164$1 * 56ll)) + 16ll) == 0ll)) == 0ll) goto label$41;
	{
		OUTP( (char*)"sub esp, 8" );
		if( *(int64*)SVREG$1 == 4ll) goto label$43;
		{
			FBSTRING TMP$167$3;
			FBSTRING TMP$168$3;
			__builtin_memset( &TMP$168$3, 0, 24ll );
			__builtin_memset( &TMP$167$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$167$3, (void*)"fld ", 5ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$168$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$168$3 );
			fb_StrDelete( (FBSTRING*)&TMP$168$3 );
		}
		goto label$42;
		label$43:;
		if( *(int64*)((uint8*)SVREG$1 + 32ll) != 1ll) goto label$44;
		{
			if( SDSIZE$1 <= 4ll) goto label$46;
			{
				FBSTRING TMP$169$4;
				FBSTRING TMP$170$4;
				__builtin_memset( &TMP$170$4, 0, 24ll );
				__builtin_memset( &TMP$169$4, 0, 24ll );
				FBSTRING* vr$29 = fb_StrConcat( &TMP$169$4, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$170$4, -1ll, (void*)vr$29, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$170$4 );
				fb_StrDelete( (FBSTRING*)&TMP$170$4 );
				OUTP( (char*)"fld qword ptr [esp]" );
			}
			goto label$45;
			label$46:;
			{
				FBSTRING TMP$171$4;
				FBSTRING TMP$172$4;
				__builtin_memset( &TMP$172$4, 0, 24ll );
				__builtin_memset( &TMP$171$4, 0, 24ll );
				FBSTRING* vr$35 = fb_StrConcat( &TMP$171$4, (void*)"movss dword ptr [esp], ", 24ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$172$4, -1ll, (void*)vr$35, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$172$4 );
				fb_StrDelete( (FBSTRING*)&TMP$172$4 );
				OUTP( (char*)"fld dword ptr [esp]" );
			}
			label$45:;
		}
		label$44:;
		label$42:;
		OUTP( (char*)"fistp qword ptr [esp]" );
		HPOP( (char*)*(char**)&DST$1 );
		OUTP( (char*)"add esp, 4" );
		goto label$36;
	}
	label$41:;
	label$40:;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$47;
	TMP$175$1 = 24ll;
	goto label$756;
	label$47:;
	TMP$175$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$756:;
	if( ((int64)-(DDSIZE$1 == 2ll) & *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$175$1 * 56ll)) + 16ll)) == 0ll) goto label$49;
	{
		FBSTRING TMP$182$2;
		FBSTRING TMP$183$2;
		OUTP( (char*)"sub esp, 8" );
		if( *(int64*)SVREG$1 == 4ll) goto label$51;
		{
			FBSTRING TMP$176$3;
			FBSTRING TMP$177$3;
			__builtin_memset( &TMP$177$3, 0, 24ll );
			__builtin_memset( &TMP$176$3, 0, 24ll );
			FBSTRING* vr$49 = fb_StrConcat( &TMP$176$3, (void*)"fld ", 5ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$177$3, -1ll, (void*)vr$49, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$177$3 );
			fb_StrDelete( (FBSTRING*)&TMP$177$3 );
		}
		goto label$50;
		label$51:;
		if( *(int64*)((uint8*)SVREG$1 + 32ll) != 1ll) goto label$52;
		{
			if( SDSIZE$1 <= 4ll) goto label$54;
			{
				FBSTRING TMP$178$4;
				FBSTRING TMP$179$4;
				__builtin_memset( &TMP$179$4, 0, 24ll );
				__builtin_memset( &TMP$178$4, 0, 24ll );
				FBSTRING* vr$56 = fb_StrConcat( &TMP$178$4, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$179$4, -1ll, (void*)vr$56, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$179$4 );
				fb_StrDelete( (FBSTRING*)&TMP$179$4 );
				OUTP( (char*)"fld qword ptr [esp]" );
			}
			goto label$53;
			label$54:;
			{
				FBSTRING TMP$180$4;
				FBSTRING TMP$181$4;
				__builtin_memset( &TMP$181$4, 0, 24ll );
				__builtin_memset( &TMP$180$4, 0, 24ll );
				FBSTRING* vr$62 = fb_StrConcat( &TMP$180$4, (void*)"movss dword ptr [esp], ", 24ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$181$4, -1ll, (void*)vr$62, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$181$4 );
				fb_StrDelete( (FBSTRING*)&TMP$181$4 );
				OUTP( (char*)"fld dword ptr [esp]" );
			}
			label$53:;
		}
		label$52:;
		label$50:;
		__builtin_memset( &TMP$183$2, 0, 24ll );
		__builtin_memset( &TMP$182$2, 0, 24ll );
		FBSTRING* vr$68 = fb_StrConcat( &TMP$182$2, (void*)"fistp ", 7ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$183$2, -1ll, (void*)vr$68, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$183$2 );
		fb_StrDelete( (FBSTRING*)&TMP$183$2 );
		OUTP( (char*)"add esp, 8" );
		goto label$36;
	}
	label$49:;
	label$48:;
	if( ((int64)-(*(int64*)DVREG$1 == 4ll) & (int64)-(DDSIZE$1 == 4ll)) == 0ll) goto label$56;
	{
		ISFREE$1 = -1ll;
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)&DST$1, -1ll, 0 );
		WASREG$1 = -1ll;
	}
	goto label$55;
	label$56:;
	{
		WASREG$1 = 0ll;
		int64 vr$75 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$1 = vr$75;
		char* vr$76 = HGETREGNAME( 8ll, REG$1 );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$76, 0ll, 0 );
		int64 vr$77 = HISREGFREE( 0ll, REG$1 );
		ISFREE$1 = vr$77;
		if( ISFREE$1 != 0ll) goto label$58;
		{
			HPUSH( (char*)*(char**)&AUX$1 );
		}
		label$58:;
		label$57:;
	}
	label$55:;
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$60;
	{
		FBSTRING TMP$188$2;
		FBSTRING TMP$189$2;
		FBSTRING TMP$190$2;
		FBSTRING TMP$191$2;
		OUTP( (char*)"sub esp, 4" );
		OUTP( (char*)"fistp dword ptr [esp]" );
		__builtin_memset( &TMP$191$2, 0, 24ll );
		__builtin_memset( &TMP$188$2, 0, 24ll );
		FBSTRING* vr$82 = fb_StrConcat( &TMP$188$2, (void*)"mov ", 5ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$189$2, 0, 24ll );
		FBSTRING* vr$85 = fb_StrConcat( &TMP$189$2, (void*)vr$82, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$190$2, 0, 24ll );
		FBSTRING* vr$88 = fb_StrConcat( &TMP$190$2, (void*)vr$85, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$191$2, -1ll, (void*)vr$88, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$191$2 );
		fb_StrDelete( (FBSTRING*)&TMP$191$2 );
		OUTP( (char*)"add esp, 4" );
	}
	goto label$59;
	label$60:;
	{
		if( SDSIZE$1 <= 4ll) goto label$62;
		{
			FBSTRING TMP$193$3;
			FBSTRING TMP$194$3;
			FBSTRING TMP$195$3;
			FBSTRING TMP$196$3;
			__builtin_memset( &TMP$196$3, 0, 24ll );
			__builtin_memset( &TMP$193$3, 0, 24ll );
			FBSTRING* vr$94 = fb_StrConcat( &TMP$193$3, (void*)"cvtsd2si ", 10ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$194$3, 0, 24ll );
			FBSTRING* vr$97 = fb_StrConcat( &TMP$194$3, (void*)vr$94, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$195$3, 0, 24ll );
			FBSTRING* vr$100 = fb_StrConcat( &TMP$195$3, (void*)vr$97, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$196$3, -1ll, (void*)vr$100, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$196$3 );
			fb_StrDelete( (FBSTRING*)&TMP$196$3 );
		}
		goto label$61;
		label$62:;
		{
			FBSTRING TMP$198$3;
			FBSTRING TMP$199$3;
			FBSTRING TMP$200$3;
			FBSTRING TMP$201$3;
			__builtin_memset( &TMP$201$3, 0, 24ll );
			__builtin_memset( &TMP$198$3, 0, 24ll );
			FBSTRING* vr$106 = fb_StrConcat( &TMP$198$3, (void*)"cvtss2si ", 10ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$199$3, 0, 24ll );
			FBSTRING* vr$109 = fb_StrConcat( &TMP$199$3, (void*)vr$106, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$200$3, 0, 24ll );
			FBSTRING* vr$112 = fb_StrConcat( &TMP$200$3, (void*)vr$109, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$201$3, -1ll, (void*)vr$112, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$201$3 );
			fb_StrDelete( (FBSTRING*)&TMP$201$3 );
		}
		label$61:;
	}
	label$59:;
	if( WASREG$1 != 0ll) goto label$64;
	{
		if( DDSIZE$1 != 1ll) goto label$66;
		{
			FBSTRING TMP$202$3;
			FBSTRING TMP$203$3;
			FBSTRING TMP$204$3;
			FBSTRING TMP$205$3;
			char* vr$115 = HGETREGNAME( 2ll, REG$1 );
			fb_StrAssign( (void*)&AUX8$1, -1ll, (void*)vr$115, 0ll, 0 );
			__builtin_memset( &TMP$205$3, 0, 24ll );
			__builtin_memset( &TMP$202$3, 0, 24ll );
			FBSTRING* vr$119 = fb_StrConcat( &TMP$202$3, (void*)"mov ", 5ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$203$3, 0, 24ll );
			FBSTRING* vr$122 = fb_StrConcat( &TMP$203$3, (void*)vr$119, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$204$3, 0, 24ll );
			FBSTRING* vr$125 = fb_StrConcat( &TMP$204$3, (void*)vr$122, -1ll, (void*)&AUX8$1, -1ll );
			fb_StrAssign( (void*)&TMP$205$3, -1ll, (void*)vr$125, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$205$3 );
			fb_StrDelete( (FBSTRING*)&TMP$205$3 );
		}
		goto label$65;
		label$66:;
		if( DDSIZE$1 != 2ll) goto label$67;
		{
			FBSTRING TMP$206$3;
			FBSTRING TMP$207$3;
			FBSTRING TMP$208$3;
			FBSTRING TMP$209$3;
			char* vr$128 = HGETREGNAME( 5ll, REG$1 );
			fb_StrAssign( (void*)&AUX16$1, -1ll, (void*)vr$128, 0ll, 0 );
			__builtin_memset( &TMP$209$3, 0, 24ll );
			__builtin_memset( &TMP$206$3, 0, 24ll );
			FBSTRING* vr$132 = fb_StrConcat( &TMP$206$3, (void*)"mov ", 5ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$207$3, 0, 24ll );
			FBSTRING* vr$135 = fb_StrConcat( &TMP$207$3, (void*)vr$132, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$208$3, 0, 24ll );
			FBSTRING* vr$138 = fb_StrConcat( &TMP$208$3, (void*)vr$135, -1ll, (void*)&AUX16$1, -1ll );
			fb_StrAssign( (void*)&TMP$209$3, -1ll, (void*)vr$138, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$209$3 );
			fb_StrDelete( (FBSTRING*)&TMP$209$3 );
		}
		goto label$65;
		label$67:;
		{
			FBSTRING TMP$210$3;
			FBSTRING TMP$211$3;
			FBSTRING TMP$212$3;
			FBSTRING TMP$213$3;
			__builtin_memset( &TMP$213$3, 0, 24ll );
			__builtin_memset( &TMP$210$3, 0, 24ll );
			FBSTRING* vr$144 = fb_StrConcat( &TMP$210$3, (void*)"mov ", 5ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$211$3, 0, 24ll );
			FBSTRING* vr$147 = fb_StrConcat( &TMP$211$3, (void*)vr$144, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$212$3, 0, 24ll );
			FBSTRING* vr$150 = fb_StrConcat( &TMP$212$3, (void*)vr$147, -1ll, (void*)&AUX$1, -1ll );
			fb_StrAssign( (void*)&TMP$213$3, -1ll, (void*)vr$150, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$213$3 );
			fb_StrDelete( (FBSTRING*)&TMP$213$3 );
		}
		label$65:;
		if( ISFREE$1 != 0ll) goto label$69;
		{
			HPOP( (char*)*(char**)&AUX$1 );
		}
		label$69:;
		label$68:;
	}
	label$64:;
	label$63:;
	label$36:;
}

static void _EMITSTORL2F_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$70:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING AUX$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( ((int64)-(*(int64*)SVREG$1 == 4ll) | (int64)-(*(int64*)SVREG$1 == 0ll)) == 0ll) goto label$73;
	{
		int64 TMP$214$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$74;
		TMP$214$2 = 24ll;
		goto label$757;
		label$74:;
		TMP$214$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$757:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$214$2 * 56ll)) + 16ll) == 0ll) goto label$76;
		{
			FBSTRING TMP$217$3;
			FBSTRING TMP$218$3;
			HPREPOPERAND64( SVREG$1, &SRC$1, &AUX$1 );
			HPUSH( (char*)*(char**)&AUX$1 );
			HPUSH( (char*)*(char**)&SRC$1 );
			__builtin_memset( &TMP$217$3, 0, 24ll );
			FBSTRING* vr$15 = fb_StrConcat( &TMP$217$3, (void*)"fild ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)SVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
			__builtin_memset( &TMP$218$3, 0, 24ll );
			FBSTRING* vr$18 = fb_StrConcat( &TMP$218$3, (void*)vr$15, -1ll, (void*)" [esp]", 7ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$18, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			OUTP( (char*)"add esp, 8" );
		}
		goto label$75;
		label$76:;
		{
			HPREPOPERAND64( SVREG$1, &SRC$1, &AUX$1 );
			HPUSH( (char*)*(char**)&AUX$1 );
			HPUSH( (char*)*(char**)&SRC$1 );
			OUTP( (char*)"fild qword ptr [esp]" );
			OUTP( (char*)"add esp, 8" );
			HULONG2DBL( SVREG$1 );
		}
		label$75:;
	}
	goto label$72;
	label$73:;
	{
		int64 TMP$220$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$77;
		TMP$220$2 = 24ll;
		goto label$758;
		label$77:;
		TMP$220$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$758:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$220$2 * 56ll)) + 16ll) == 0ll) goto label$79;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$78;
		label$79:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			HULONG2DBL( SVREG$1 );
		}
		label$78:;
	}
	label$72:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fstp ", 6ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$71:;
}

static void _EMITSTORI2F_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$222$1;
	int64 TMP$223$1;
	int64 TMP$224$1;
	label$80:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING AUX$1;
	static int64 DDSIZE$1;
	static int64 SDSIZE$1;
	static int64 REG$1;
	static int64 ISFREE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$82;
	TMP$222$1 = 24ll;
	goto label$759;
	label$82:;
	TMP$222$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$759:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$222$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$83;
	TMP$223$1 = 24ll;
	goto label$760;
	label$83:;
	TMP$223$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$760:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$223$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$84;
	TMP$224$1 = 24ll;
	goto label$761;
	label$84:;
	TMP$224$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$761:;
	if( ((int64)-(*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$224$1 * 56ll)) + 16ll) == 0ll) & (int64)-(SDSIZE$1 == 4ll)) == 0ll) goto label$86;
	{
		FBSTRING TMP$225$2;
		FBSTRING TMP$226$2;
		HPUSH( (char*)"0" );
		HPUSH( (char*)*(char**)&SRC$1 );
		OUTP( (char*)"fild qword ptr [esp]" );
		OUTP( (char*)"add esp, 8" );
		__builtin_memset( &TMP$226$2, 0, 24ll );
		__builtin_memset( &TMP$225$2, 0, 24ll );
		FBSTRING* vr$21 = fb_StrConcat( &TMP$225$2, (void*)"fstp ", 6ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$226$2, -1ll, (void*)vr$21, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$226$2 );
		fb_StrDelete( (FBSTRING*)&TMP$226$2 );
		goto label$81;
	}
	label$86:;
	label$85:;
	if( ((int64)-(*(int64*)SVREG$1 != 0ll) & (int64)-(SDSIZE$1 == 4ll)) == 0ll) goto label$88;
	{
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		ISFREE$1 = -1ll;
	}
	goto label$87;
	label$88:;
	{
		int64 vr$28 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$1 = vr$28;
		char* vr$29 = HGETREGNAME( 8ll, REG$1 );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$29, 0ll, 0 );
		int64 vr$30 = HISREGFREE( 0ll, REG$1 );
		ISFREE$1 = vr$30;
		if( ISFREE$1 != 0ll) goto label$90;
		{
			HPUSH( (char*)*(char**)&AUX$1 );
		}
		label$90:;
		label$89:;
		if( *(int64*)SVREG$1 != 0ll) goto label$92;
		{
			FBSTRING TMP$227$3;
			FBSTRING TMP$228$3;
			FBSTRING TMP$229$3;
			FBSTRING TMP$230$3;
			__builtin_memset( &TMP$230$3, 0, 24ll );
			__builtin_memset( &TMP$227$3, 0, 24ll );
			FBSTRING* vr$35 = fb_StrConcat( &TMP$227$3, (void*)"mov ", 5ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$228$3, 0, 24ll );
			FBSTRING* vr$38 = fb_StrConcat( &TMP$228$3, (void*)vr$35, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$229$3, 0, 24ll );
			FBSTRING* vr$41 = fb_StrConcat( &TMP$229$3, (void*)vr$38, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$230$3, -1ll, (void*)vr$41, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$230$3 );
			fb_StrDelete( (FBSTRING*)&TMP$230$3 );
		}
		goto label$91;
		label$92:;
		{
			int64 TMP$231$3;
			FBSTRING TMP$234$3;
			FBSTRING TMP$235$3;
			FBSTRING TMP$236$3;
			FBSTRING TMP$237$3;
			if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$93;
			TMP$231$3 = 24ll;
			goto label$762;
			label$93:;
			TMP$231$3 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
			label$762:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$231$3 * 56ll)) + 16ll) == 0ll) goto label$95;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movsx ", 7ll, 0 );
			}
			goto label$94;
			label$95:;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movzx ", 7ll, 0 );
			}
			label$94:;
			__builtin_memset( &TMP$237$3, 0, 24ll );
			__builtin_memset( &TMP$234$3, 0, 24ll );
			FBSTRING* vr$52 = fb_StrConcat( &TMP$234$3, (void*)&OSTR$1, -1ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$235$3, 0, 24ll );
			FBSTRING* vr$55 = fb_StrConcat( &TMP$235$3, (void*)vr$52, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$236$3, 0, 24ll );
			FBSTRING* vr$58 = fb_StrConcat( &TMP$236$3, (void*)vr$55, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$237$3, -1ll, (void*)vr$58, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$237$3 );
			fb_StrDelete( (FBSTRING*)&TMP$237$3 );
		}
		label$91:;
	}
	label$87:;
	if( DDSIZE$1 <= 4ll) goto label$97;
	{
		FBSTRING TMP$240$2;
		FBSTRING TMP$241$2;
		FBSTRING TMP$243$2;
		FBSTRING TMP$244$2;
		FBSTRING TMP$245$2;
		FBSTRING TMP$246$2;
		__builtin_memset( &TMP$241$2, 0, 24ll );
		__builtin_memset( &TMP$240$2, 0, 24ll );
		FBSTRING* vr$64 = fb_StrConcat( &TMP$240$2, (void*)"cvtsi2sd xmm7, ", 16ll, (void*)&AUX$1, -1ll );
		fb_StrAssign( (void*)&TMP$241$2, -1ll, (void*)vr$64, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$241$2 );
		fb_StrDelete( (FBSTRING*)&TMP$241$2 );
		__builtin_memset( &TMP$246$2, 0, 24ll );
		__builtin_memset( &TMP$243$2, 0, 24ll );
		FBSTRING* vr$70 = fb_StrConcat( &TMP$243$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$244$2, 0, 24ll );
		FBSTRING* vr$73 = fb_StrConcat( &TMP$244$2, (void*)vr$70, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$245$2, 0, 24ll );
		FBSTRING* vr$76 = fb_StrConcat( &TMP$245$2, (void*)vr$73, -1ll, (void*)"xmm7", 5ll );
		fb_StrAssign( (void*)&TMP$246$2, -1ll, (void*)vr$76, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$246$2 );
		fb_StrDelete( (FBSTRING*)&TMP$246$2 );
	}
	goto label$96;
	label$97:;
	{
		FBSTRING TMP$249$2;
		FBSTRING TMP$250$2;
		FBSTRING TMP$251$2;
		FBSTRING TMP$252$2;
		FBSTRING TMP$253$2;
		FBSTRING TMP$254$2;
		__builtin_memset( &TMP$250$2, 0, 24ll );
		__builtin_memset( &TMP$249$2, 0, 24ll );
		FBSTRING* vr$82 = fb_StrConcat( &TMP$249$2, (void*)"cvtsi2ss xmm7, ", 16ll, (void*)&AUX$1, -1ll );
		fb_StrAssign( (void*)&TMP$250$2, -1ll, (void*)vr$82, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$250$2 );
		fb_StrDelete( (FBSTRING*)&TMP$250$2 );
		__builtin_memset( &TMP$254$2, 0, 24ll );
		__builtin_memset( &TMP$251$2, 0, 24ll );
		FBSTRING* vr$88 = fb_StrConcat( &TMP$251$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$252$2, 0, 24ll );
		FBSTRING* vr$91 = fb_StrConcat( &TMP$252$2, (void*)vr$88, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$253$2, 0, 24ll );
		FBSTRING* vr$94 = fb_StrConcat( &TMP$253$2, (void*)vr$91, -1ll, (void*)"xmm7", 5ll );
		fb_StrAssign( (void*)&TMP$254$2, -1ll, (void*)vr$94, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$254$2 );
		fb_StrDelete( (FBSTRING*)&TMP$254$2 );
	}
	label$96:;
	if( ISFREE$1 != 0ll) goto label$99;
	{
		HPOP( (char*)*(char**)&AUX$1 );
	}
	label$99:;
	label$98:;
	label$81:;
}

static void HEMITSTOREFREG2F_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$255$1;
	label$100:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, 0ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$102;
	TMP$255$1 = 24ll;
	goto label$763;
	label$102:;
	TMP$255$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$763:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$255$1 * 56ll)) + 8ll);
	if( ((int64)-(*(int64*)((uint8*)SVREG$1 + 40ll) == 2ll) & (int64)-(DDSIZE$1 > 4ll)) == 0ll) goto label$104;
	{
		FBSTRING TMP$257$2;
		FBSTRING TMP$258$2;
		FBSTRING TMP$259$2;
		FBSTRING TMP$260$2;
		__builtin_memset( &TMP$260$2, 0, 24ll );
		__builtin_memset( &TMP$257$2, 0, 24ll );
		FBSTRING* vr$12 = fb_StrConcat( &TMP$257$2, (void*)"movupd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$258$2, 0, 24ll );
		FBSTRING* vr$15 = fb_StrConcat( &TMP$258$2, (void*)vr$12, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$259$2, 0, 24ll );
		FBSTRING* vr$18 = fb_StrConcat( &TMP$259$2, (void*)vr$15, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$260$2, -1ll, (void*)vr$18, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$260$2 );
		fb_StrDelete( (FBSTRING*)&TMP$260$2 );
		goto label$101;
	}
	label$104:;
	label$103:;
	if( *(int64*)((uint8*)SVREG$1 + 40ll) != 2ll) goto label$106;
	{
		FBSTRING TMP$262$2;
		FBSTRING TMP$263$2;
		FBSTRING TMP$264$2;
		FBSTRING TMP$265$2;
		__builtin_memset( &TMP$265$2, 0, 24ll );
		__builtin_memset( &TMP$262$2, 0, 24ll );
		FBSTRING* vr$25 = fb_StrConcat( &TMP$262$2, (void*)"movlps ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$263$2, 0, 24ll );
		FBSTRING* vr$28 = fb_StrConcat( &TMP$263$2, (void*)vr$25, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$264$2, 0, 24ll );
		FBSTRING* vr$31 = fb_StrConcat( &TMP$264$2, (void*)vr$28, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$265$2, -1ll, (void*)vr$31, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$265$2 );
		fb_StrDelete( (FBSTRING*)&TMP$265$2 );
	}
	goto label$105;
	label$106:;
	if( *(int64*)((uint8*)SVREG$1 + 40ll) != 3ll) goto label$107;
	{
		FBSTRING TMP$268$2;
		FBSTRING TMP$269$2;
		FBSTRING TMP$270$2;
		FBSTRING TMP$271$2;
		FBSTRING TMP$272$2;
		FBSTRING TMP$273$2;
		FBSTRING TMP$274$2;
		FBSTRING TMP$275$2;
		FBSTRING TMP$276$2;
		FBSTRING TMP$277$2;
		__builtin_memset( &TMP$269$2, 0, 24ll );
		__builtin_memset( &TMP$268$2, 0, 24ll );
		FBSTRING* vr$38 = fb_StrConcat( &TMP$268$2, (void*)"movhlps xmm7, ", 15ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$269$2, -1ll, (void*)vr$38, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$269$2 );
		fb_StrDelete( (FBSTRING*)&TMP$269$2 );
		__builtin_memset( &TMP$273$2, 0, 24ll );
		__builtin_memset( &TMP$270$2, 0, 24ll );
		FBSTRING* vr$44 = fb_StrConcat( &TMP$270$2, (void*)"movlps ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$271$2, 0, 24ll );
		FBSTRING* vr$47 = fb_StrConcat( &TMP$271$2, (void*)vr$44, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$272$2, 0, 24ll );
		FBSTRING* vr$50 = fb_StrConcat( &TMP$272$2, (void*)vr$47, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$273$2, -1ll, (void*)vr$50, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$273$2 );
		fb_StrDelete( (FBSTRING*)&TMP$273$2 );
		HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 8ll, 0ll, -1ll );
		__builtin_memset( &TMP$277$2, 0, 24ll );
		__builtin_memset( &TMP$274$2, 0, 24ll );
		FBSTRING* vr$56 = fb_StrConcat( &TMP$274$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$275$2, 0, 24ll );
		FBSTRING* vr$59 = fb_StrConcat( &TMP$275$2, (void*)vr$56, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$276$2, 0, 24ll );
		FBSTRING* vr$62 = fb_StrConcat( &TMP$276$2, (void*)vr$59, -1ll, (void*)"xmm7", 5ll );
		fb_StrAssign( (void*)&TMP$277$2, -1ll, (void*)vr$62, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$277$2 );
		fb_StrDelete( (FBSTRING*)&TMP$277$2 );
	}
	goto label$105;
	label$107:;
	if( *(int64*)((uint8*)SVREG$1 + 40ll) != 4ll) goto label$108;
	{
		FBSTRING TMP$279$2;
		FBSTRING TMP$280$2;
		FBSTRING TMP$281$2;
		FBSTRING TMP$282$2;
		__builtin_memset( &TMP$282$2, 0, 24ll );
		__builtin_memset( &TMP$279$2, 0, 24ll );
		FBSTRING* vr$69 = fb_StrConcat( &TMP$279$2, (void*)"movups ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$280$2, 0, 24ll );
		FBSTRING* vr$72 = fb_StrConcat( &TMP$280$2, (void*)vr$69, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$281$2, 0, 24ll );
		FBSTRING* vr$75 = fb_StrConcat( &TMP$281$2, (void*)vr$72, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$282$2, -1ll, (void*)vr$75, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$282$2 );
		fb_StrDelete( (FBSTRING*)&TMP$282$2 );
	}
	label$108:;
	label$105:;
	label$101:;
}

static void _EMITSTORF2F_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$283$1;
	int64 TMP$284$1;
	label$109:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	static int64 SDSIZE$1;
	static int64 SRC_VEC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, 0ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$111;
	TMP$283$1 = 24ll;
	goto label$764;
	label$111:;
	TMP$283$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$764:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$283$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$112;
	TMP$284$1 = 24ll;
	goto label$765;
	label$112:;
	TMP$284$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$765:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$284$1 * 56ll)) + 8ll);
	SRC_VEC$1 = (int64)-(*(int64*)((uint8*)SVREG$1 + 40ll) > 0ll);
	if( *(int64*)SVREG$1 != 4ll) goto label$114;
	{
		if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$116;
		{
			FBSTRING TMP$285$3;
			FBSTRING TMP$286$3;
			HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
			__builtin_memset( &TMP$286$3, 0, 24ll );
			__builtin_memset( &TMP$285$3, 0, 24ll );
			FBSTRING* vr$17 = fb_StrConcat( &TMP$285$3, (void*)"fstp ", 6ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$286$3, -1ll, (void*)vr$17, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$286$3 );
			fb_StrDelete( (FBSTRING*)&TMP$286$3 );
			goto label$110;
		}
		label$116:;
		label$115:;
		if( SRC_VEC$1 == 0ll) goto label$118;
		{
			HEMITSTOREFREG2F_SSE( DVREG$1, SVREG$1 );
			goto label$110;
		}
		label$118:;
		label$117:;
		if( DDSIZE$1 <= 4ll) goto label$120;
		{
			FBSTRING TMP$292$3;
			FBSTRING TMP$293$3;
			FBSTRING TMP$294$3;
			FBSTRING TMP$295$3;
			if( SDSIZE$1 > 4ll) goto label$122;
			{
				FBSTRING TMP$288$4;
				FBSTRING TMP$289$4;
				FBSTRING TMP$290$4;
				FBSTRING TMP$291$4;
				__builtin_memset( &TMP$291$4, 0, 24ll );
				__builtin_memset( &TMP$288$4, 0, 24ll );
				FBSTRING* vr$23 = fb_StrConcat( &TMP$288$4, (void*)"cvtss2sd ", 10ll, (void*)&SRC$1, -1ll );
				__builtin_memset( &TMP$289$4, 0, 24ll );
				FBSTRING* vr$26 = fb_StrConcat( &TMP$289$4, (void*)vr$23, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$290$4, 0, 24ll );
				FBSTRING* vr$29 = fb_StrConcat( &TMP$290$4, (void*)vr$26, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$291$4, -1ll, (void*)vr$29, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$291$4 );
				fb_StrDelete( (FBSTRING*)&TMP$291$4 );
			}
			label$122:;
			label$121:;
			__builtin_memset( &TMP$295$3, 0, 24ll );
			__builtin_memset( &TMP$292$3, 0, 24ll );
			FBSTRING* vr$35 = fb_StrConcat( &TMP$292$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$293$3, 0, 24ll );
			FBSTRING* vr$38 = fb_StrConcat( &TMP$293$3, (void*)vr$35, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$294$3, 0, 24ll );
			FBSTRING* vr$41 = fb_StrConcat( &TMP$294$3, (void*)vr$38, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$295$3, -1ll, (void*)vr$41, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$295$3 );
			fb_StrDelete( (FBSTRING*)&TMP$295$3 );
		}
		goto label$119;
		label$120:;
		{
			FBSTRING TMP$301$3;
			FBSTRING TMP$302$3;
			FBSTRING TMP$303$3;
			FBSTRING TMP$304$3;
			if( SDSIZE$1 <= 4ll) goto label$124;
			{
				FBSTRING TMP$297$4;
				FBSTRING TMP$298$4;
				FBSTRING TMP$299$4;
				FBSTRING TMP$300$4;
				__builtin_memset( &TMP$300$4, 0, 24ll );
				__builtin_memset( &TMP$297$4, 0, 24ll );
				FBSTRING* vr$47 = fb_StrConcat( &TMP$297$4, (void*)"cvtsd2ss ", 10ll, (void*)&SRC$1, -1ll );
				__builtin_memset( &TMP$298$4, 0, 24ll );
				FBSTRING* vr$50 = fb_StrConcat( &TMP$298$4, (void*)vr$47, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$299$4, 0, 24ll );
				FBSTRING* vr$53 = fb_StrConcat( &TMP$299$4, (void*)vr$50, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$300$4, -1ll, (void*)vr$53, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$300$4 );
				fb_StrDelete( (FBSTRING*)&TMP$300$4 );
			}
			label$124:;
			label$123:;
			__builtin_memset( &TMP$304$3, 0, 24ll );
			__builtin_memset( &TMP$301$3, 0, 24ll );
			FBSTRING* vr$59 = fb_StrConcat( &TMP$301$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$302$3, 0, 24ll );
			FBSTRING* vr$62 = fb_StrConcat( &TMP$302$3, (void*)vr$59, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$303$3, 0, 24ll );
			FBSTRING* vr$65 = fb_StrConcat( &TMP$303$3, (void*)vr$62, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$304$3, -1ll, (void*)vr$65, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$304$3 );
			fb_StrDelete( (FBSTRING*)&TMP$304$3 );
		}
		label$119:;
	}
	goto label$113;
	label$114:;
	{
		if( SDSIZE$1 != DDSIZE$1) goto label$126;
		{
			if( SRC_VEC$1 == 0ll) goto label$128;
			{
				HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, 0ll );
				HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
				if( DDSIZE$1 <= 4ll) goto label$130;
				{
					FBSTRING TMP$307$5;
					FBSTRING TMP$308$5;
					FBSTRING TMP$309$5;
					FBSTRING TMP$310$5;
					FBSTRING TMP$311$5;
					FBSTRING TMP$312$5;
					__builtin_memset( &TMP$308$5, 0, 24ll );
					__builtin_memset( &TMP$307$5, 0, 24ll );
					FBSTRING* vr$71 = fb_StrConcat( &TMP$307$5, (void*)"movupd xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$308$5, -1ll, (void*)vr$71, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$308$5 );
					fb_StrDelete( (FBSTRING*)&TMP$308$5 );
					__builtin_memset( &TMP$312$5, 0, 24ll );
					__builtin_memset( &TMP$309$5, 0, 24ll );
					FBSTRING* vr$77 = fb_StrConcat( &TMP$309$5, (void*)"movupd ", 8ll, (void*)&DST$1, -1ll );
					__builtin_memset( &TMP$310$5, 0, 24ll );
					FBSTRING* vr$80 = fb_StrConcat( &TMP$310$5, (void*)vr$77, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$311$5, 0, 24ll );
					FBSTRING* vr$83 = fb_StrConcat( &TMP$311$5, (void*)vr$80, -1ll, (void*)"xmm7", 5ll );
					fb_StrAssign( (void*)&TMP$312$5, -1ll, (void*)vr$83, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$312$5 );
					fb_StrDelete( (FBSTRING*)&TMP$312$5 );
				}
				goto label$129;
				label$130:;
				{
					if( *(int64*)((uint8*)SVREG$1 + 40ll) != 2ll) goto label$132;
					{
						FBSTRING TMP$315$6;
						FBSTRING TMP$316$6;
						FBSTRING TMP$317$6;
						FBSTRING TMP$318$6;
						FBSTRING TMP$319$6;
						FBSTRING TMP$320$6;
						__builtin_memset( &TMP$316$6, 0, 24ll );
						__builtin_memset( &TMP$315$6, 0, 24ll );
						FBSTRING* vr$90 = fb_StrConcat( &TMP$315$6, (void*)"movlps xmm7, ", 14ll, (void*)&SRC$1, -1ll );
						fb_StrAssign( (void*)&TMP$316$6, -1ll, (void*)vr$90, -1ll, 0 );
						OUTP( (char*)*(char**)&TMP$316$6 );
						fb_StrDelete( (FBSTRING*)&TMP$316$6 );
						__builtin_memset( &TMP$320$6, 0, 24ll );
						__builtin_memset( &TMP$317$6, 0, 24ll );
						FBSTRING* vr$96 = fb_StrConcat( &TMP$317$6, (void*)"movlps ", 8ll, (void*)&DST$1, -1ll );
						__builtin_memset( &TMP$318$6, 0, 24ll );
						FBSTRING* vr$99 = fb_StrConcat( &TMP$318$6, (void*)vr$96, -1ll, (void*)", ", 3ll );
						__builtin_memset( &TMP$319$6, 0, 24ll );
						FBSTRING* vr$102 = fb_StrConcat( &TMP$319$6, (void*)vr$99, -1ll, (void*)"xmm7", 5ll );
						fb_StrAssign( (void*)&TMP$320$6, -1ll, (void*)vr$102, -1ll, 0 );
						OUTP( (char*)*(char**)&TMP$320$6 );
						fb_StrDelete( (FBSTRING*)&TMP$320$6 );
					}
					goto label$131;
					label$132:;
					if( *(int64*)((uint8*)SVREG$1 + 40ll) != 3ll) goto label$133;
					{
						FBSTRING TMP$323$6;
						FBSTRING TMP$324$6;
						FBSTRING TMP$325$6;
						FBSTRING TMP$326$6;
						FBSTRING TMP$327$6;
						FBSTRING TMP$328$6;
						FBSTRING TMP$330$6;
						FBSTRING TMP$331$6;
						FBSTRING TMP$332$6;
						FBSTRING TMP$333$6;
						__builtin_memset( &TMP$324$6, 0, 24ll );
						__builtin_memset( &TMP$323$6, 0, 24ll );
						FBSTRING* vr$109 = fb_StrConcat( &TMP$323$6, (void*)"movups xmm7, ", 14ll, (void*)&SRC$1, -1ll );
						fb_StrAssign( (void*)&TMP$324$6, -1ll, (void*)vr$109, -1ll, 0 );
						OUTP( (char*)*(char**)&TMP$324$6 );
						fb_StrDelete( (FBSTRING*)&TMP$324$6 );
						__builtin_memset( &TMP$328$6, 0, 24ll );
						__builtin_memset( &TMP$325$6, 0, 24ll );
						FBSTRING* vr$115 = fb_StrConcat( &TMP$325$6, (void*)"movlps ", 8ll, (void*)&DST$1, -1ll );
						__builtin_memset( &TMP$326$6, 0, 24ll );
						FBSTRING* vr$118 = fb_StrConcat( &TMP$326$6, (void*)vr$115, -1ll, (void*)", ", 3ll );
						__builtin_memset( &TMP$327$6, 0, 24ll );
						FBSTRING* vr$121 = fb_StrConcat( &TMP$327$6, (void*)vr$118, -1ll, (void*)"xmm7", 5ll );
						fb_StrAssign( (void*)&TMP$328$6, -1ll, (void*)vr$121, -1ll, 0 );
						OUTP( (char*)*(char**)&TMP$328$6 );
						fb_StrDelete( (FBSTRING*)&TMP$328$6 );
						OUTP( (char*)"unpckhps xmm7, xmm7" );
						HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 8ll, 0ll, -1ll );
						__builtin_memset( &TMP$333$6, 0, 24ll );
						__builtin_memset( &TMP$330$6, 0, 24ll );
						FBSTRING* vr$127 = fb_StrConcat( &TMP$330$6, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
						__builtin_memset( &TMP$331$6, 0, 24ll );
						FBSTRING* vr$130 = fb_StrConcat( &TMP$331$6, (void*)vr$127, -1ll, (void*)", ", 3ll );
						__builtin_memset( &TMP$332$6, 0, 24ll );
						FBSTRING* vr$133 = fb_StrConcat( &TMP$332$6, (void*)vr$130, -1ll, (void*)"xmm7", 5ll );
						fb_StrAssign( (void*)&TMP$333$6, -1ll, (void*)vr$133, -1ll, 0 );
						OUTP( (char*)*(char**)&TMP$333$6 );
						fb_StrDelete( (FBSTRING*)&TMP$333$6 );
					}
					goto label$131;
					label$133:;
					if( *(int64*)((uint8*)SVREG$1 + 40ll) != 4ll) goto label$134;
					{
						FBSTRING TMP$334$6;
						FBSTRING TMP$335$6;
						FBSTRING TMP$336$6;
						FBSTRING TMP$337$6;
						FBSTRING TMP$338$6;
						FBSTRING TMP$339$6;
						__builtin_memset( &TMP$335$6, 0, 24ll );
						__builtin_memset( &TMP$334$6, 0, 24ll );
						FBSTRING* vr$140 = fb_StrConcat( &TMP$334$6, (void*)"movups xmm7, ", 14ll, (void*)&SRC$1, -1ll );
						fb_StrAssign( (void*)&TMP$335$6, -1ll, (void*)vr$140, -1ll, 0 );
						OUTP( (char*)*(char**)&TMP$335$6 );
						fb_StrDelete( (FBSTRING*)&TMP$335$6 );
						__builtin_memset( &TMP$339$6, 0, 24ll );
						__builtin_memset( &TMP$336$6, 0, 24ll );
						FBSTRING* vr$146 = fb_StrConcat( &TMP$336$6, (void*)"movups ", 8ll, (void*)&DST$1, -1ll );
						__builtin_memset( &TMP$337$6, 0, 24ll );
						FBSTRING* vr$149 = fb_StrConcat( &TMP$337$6, (void*)vr$146, -1ll, (void*)", ", 3ll );
						__builtin_memset( &TMP$338$6, 0, 24ll );
						FBSTRING* vr$152 = fb_StrConcat( &TMP$338$6, (void*)vr$149, -1ll, (void*)"xmm7", 5ll );
						fb_StrAssign( (void*)&TMP$339$6, -1ll, (void*)vr$152, -1ll, 0 );
						OUTP( (char*)*(char**)&TMP$339$6 );
						fb_StrDelete( (FBSTRING*)&TMP$339$6 );
					}
					label$134:;
					label$131:;
				}
				label$129:;
				goto label$110;
			}
			label$128:;
			label$127:;
			if( DDSIZE$1 <= 4ll) goto label$136;
			{
				FBSTRING TMP$342$4;
				FBSTRING TMP$343$4;
				FBSTRING TMP$344$4;
				FBSTRING TMP$345$4;
				FBSTRING TMP$346$4;
				FBSTRING TMP$347$4;
				__builtin_memset( &TMP$343$4, 0, 24ll );
				__builtin_memset( &TMP$342$4, 0, 24ll );
				FBSTRING* vr$158 = fb_StrConcat( &TMP$342$4, (void*)"movlpd xmm7, ", 14ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$343$4, -1ll, (void*)vr$158, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$343$4 );
				fb_StrDelete( (FBSTRING*)&TMP$343$4 );
				__builtin_memset( &TMP$347$4, 0, 24ll );
				__builtin_memset( &TMP$344$4, 0, 24ll );
				FBSTRING* vr$164 = fb_StrConcat( &TMP$344$4, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
				__builtin_memset( &TMP$345$4, 0, 24ll );
				FBSTRING* vr$167 = fb_StrConcat( &TMP$345$4, (void*)vr$164, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$346$4, 0, 24ll );
				FBSTRING* vr$170 = fb_StrConcat( &TMP$346$4, (void*)vr$167, -1ll, (void*)"xmm7", 5ll );
				fb_StrAssign( (void*)&TMP$347$4, -1ll, (void*)vr$170, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$347$4 );
				fb_StrDelete( (FBSTRING*)&TMP$347$4 );
			}
			goto label$135;
			label$136:;
			{
				FBSTRING TMP$350$4;
				FBSTRING TMP$351$4;
				FBSTRING TMP$352$4;
				FBSTRING TMP$353$4;
				FBSTRING TMP$354$4;
				FBSTRING TMP$355$4;
				__builtin_memset( &TMP$351$4, 0, 24ll );
				__builtin_memset( &TMP$350$4, 0, 24ll );
				FBSTRING* vr$176 = fb_StrConcat( &TMP$350$4, (void*)"movss xmm7, ", 13ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$351$4, -1ll, (void*)vr$176, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$351$4 );
				fb_StrDelete( (FBSTRING*)&TMP$351$4 );
				__builtin_memset( &TMP$355$4, 0, 24ll );
				__builtin_memset( &TMP$352$4, 0, 24ll );
				FBSTRING* vr$182 = fb_StrConcat( &TMP$352$4, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
				__builtin_memset( &TMP$353$4, 0, 24ll );
				FBSTRING* vr$185 = fb_StrConcat( &TMP$353$4, (void*)vr$182, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$354$4, 0, 24ll );
				FBSTRING* vr$188 = fb_StrConcat( &TMP$354$4, (void*)vr$185, -1ll, (void*)"xmm7", 5ll );
				fb_StrAssign( (void*)&TMP$355$4, -1ll, (void*)vr$188, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$355$4 );
				fb_StrDelete( (FBSTRING*)&TMP$355$4 );
			}
			label$135:;
		}
		goto label$125;
		label$126:;
		{
			if( SDSIZE$1 <= 4ll) goto label$138;
			{
				if( SRC_VEC$1 == 0ll) goto label$140;
				{
					FBSTRING TMP$358$5;
					FBSTRING TMP$359$5;
					FBSTRING TMP$360$5;
					FBSTRING TMP$361$5;
					FBSTRING TMP$362$5;
					FBSTRING TMP$363$5;
					__builtin_memset( &TMP$359$5, 0, 24ll );
					__builtin_memset( &TMP$358$5, 0, 24ll );
					FBSTRING* vr$194 = fb_StrConcat( &TMP$358$5, (void*)"cvtpd2ps xmm7, ", 16ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$359$5, -1ll, (void*)vr$194, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$359$5 );
					fb_StrDelete( (FBSTRING*)&TMP$359$5 );
					__builtin_memset( &TMP$363$5, 0, 24ll );
					__builtin_memset( &TMP$360$5, 0, 24ll );
					FBSTRING* vr$200 = fb_StrConcat( &TMP$360$5, (void*)"movlps ", 8ll, (void*)&DST$1, -1ll );
					__builtin_memset( &TMP$361$5, 0, 24ll );
					FBSTRING* vr$203 = fb_StrConcat( &TMP$361$5, (void*)vr$200, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$362$5, 0, 24ll );
					FBSTRING* vr$206 = fb_StrConcat( &TMP$362$5, (void*)vr$203, -1ll, (void*)"xmm7", 5ll );
					fb_StrAssign( (void*)&TMP$363$5, -1ll, (void*)vr$206, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$363$5 );
					fb_StrDelete( (FBSTRING*)&TMP$363$5 );
				}
				goto label$139;
				label$140:;
				{
					FBSTRING TMP$366$5;
					FBSTRING TMP$367$5;
					FBSTRING TMP$368$5;
					FBSTRING TMP$369$5;
					FBSTRING TMP$370$5;
					FBSTRING TMP$371$5;
					__builtin_memset( &TMP$367$5, 0, 24ll );
					__builtin_memset( &TMP$366$5, 0, 24ll );
					FBSTRING* vr$212 = fb_StrConcat( &TMP$366$5, (void*)"cvtsd2ss xmm7, ", 16ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$367$5, -1ll, (void*)vr$212, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$367$5 );
					fb_StrDelete( (FBSTRING*)&TMP$367$5 );
					__builtin_memset( &TMP$371$5, 0, 24ll );
					__builtin_memset( &TMP$368$5, 0, 24ll );
					FBSTRING* vr$218 = fb_StrConcat( &TMP$368$5, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
					__builtin_memset( &TMP$369$5, 0, 24ll );
					FBSTRING* vr$221 = fb_StrConcat( &TMP$369$5, (void*)vr$218, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$370$5, 0, 24ll );
					FBSTRING* vr$224 = fb_StrConcat( &TMP$370$5, (void*)vr$221, -1ll, (void*)"xmm7", 5ll );
					fb_StrAssign( (void*)&TMP$371$5, -1ll, (void*)vr$224, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$371$5 );
					fb_StrDelete( (FBSTRING*)&TMP$371$5 );
				}
				label$139:;
			}
			goto label$137;
			label$138:;
			{
				if( SRC_VEC$1 == 0ll) goto label$142;
				{
					FBSTRING TMP$374$5;
					FBSTRING TMP$375$5;
					FBSTRING TMP$376$5;
					FBSTRING TMP$377$5;
					FBSTRING TMP$378$5;
					FBSTRING TMP$379$5;
					__builtin_memset( &TMP$375$5, 0, 24ll );
					__builtin_memset( &TMP$374$5, 0, 24ll );
					FBSTRING* vr$230 = fb_StrConcat( &TMP$374$5, (void*)"cvtps2pd xmm7, ", 16ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$375$5, -1ll, (void*)vr$230, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$375$5 );
					fb_StrDelete( (FBSTRING*)&TMP$375$5 );
					__builtin_memset( &TMP$379$5, 0, 24ll );
					__builtin_memset( &TMP$376$5, 0, 24ll );
					FBSTRING* vr$236 = fb_StrConcat( &TMP$376$5, (void*)"movupd ", 8ll, (void*)&DST$1, -1ll );
					__builtin_memset( &TMP$377$5, 0, 24ll );
					FBSTRING* vr$239 = fb_StrConcat( &TMP$377$5, (void*)vr$236, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$378$5, 0, 24ll );
					FBSTRING* vr$242 = fb_StrConcat( &TMP$378$5, (void*)vr$239, -1ll, (void*)"xmm7", 5ll );
					fb_StrAssign( (void*)&TMP$379$5, -1ll, (void*)vr$242, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$379$5 );
					fb_StrDelete( (FBSTRING*)&TMP$379$5 );
				}
				goto label$141;
				label$142:;
				{
					FBSTRING TMP$382$5;
					FBSTRING TMP$383$5;
					FBSTRING TMP$384$5;
					FBSTRING TMP$385$5;
					FBSTRING TMP$386$5;
					FBSTRING TMP$387$5;
					__builtin_memset( &TMP$383$5, 0, 24ll );
					__builtin_memset( &TMP$382$5, 0, 24ll );
					FBSTRING* vr$248 = fb_StrConcat( &TMP$382$5, (void*)"cvtss2sd xmm7, ", 16ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$383$5, -1ll, (void*)vr$248, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$383$5 );
					fb_StrDelete( (FBSTRING*)&TMP$383$5 );
					__builtin_memset( &TMP$387$5, 0, 24ll );
					__builtin_memset( &TMP$384$5, 0, 24ll );
					FBSTRING* vr$254 = fb_StrConcat( &TMP$384$5, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
					__builtin_memset( &TMP$385$5, 0, 24ll );
					FBSTRING* vr$257 = fb_StrConcat( &TMP$385$5, (void*)vr$254, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$386$5, 0, 24ll );
					FBSTRING* vr$260 = fb_StrConcat( &TMP$386$5, (void*)vr$257, -1ll, (void*)"xmm7", 5ll );
					fb_StrAssign( (void*)&TMP$387$5, -1ll, (void*)vr$260, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$387$5 );
					fb_StrDelete( (FBSTRING*)&TMP$387$5 );
				}
				label$141:;
			}
			label$137:;
		}
		label$125:;
	}
	label$113:;
	label$110:;
}

static void _EMITLOADF2L_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$388$1;
	int64 TMP$395$1;
	int64 TMP$396$1;
	label$143:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING AUX$1;
	static FBSTRING OSTR$1;
	static int64 DDSIZE$1;
	static int64 SDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$145;
	TMP$388$1 = 24ll;
	goto label$766;
	label$145:;
	TMP$388$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$766:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$388$1 * 56ll)) + 8ll);
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 1ll) goto label$147;
	{
		if( *(int64*)SVREG$1 != 4ll) goto label$149;
		{
			OUTP( (char*)"sub esp, 8" );
			if( SDSIZE$1 <= 4ll) goto label$151;
			{
				FBSTRING TMP$389$4;
				FBSTRING TMP$390$4;
				__builtin_memset( &TMP$390$4, 0, 24ll );
				__builtin_memset( &TMP$389$4, 0, 24ll );
				FBSTRING* vr$10 = fb_StrConcat( &TMP$389$4, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$390$4, -1ll, (void*)vr$10, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$390$4 );
				fb_StrDelete( (FBSTRING*)&TMP$390$4 );
				OUTP( (char*)"fld qword ptr [esp]" );
			}
			goto label$150;
			label$151:;
			{
				FBSTRING TMP$391$4;
				FBSTRING TMP$392$4;
				__builtin_memset( &TMP$392$4, 0, 24ll );
				__builtin_memset( &TMP$391$4, 0, 24ll );
				FBSTRING* vr$16 = fb_StrConcat( &TMP$391$4, (void*)"movss dword ptr [esp], ", 24ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$392$4, -1ll, (void*)vr$16, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$392$4 );
				fb_StrDelete( (FBSTRING*)&TMP$392$4 );
				OUTP( (char*)"fld dword ptr [esp]" );
			}
			label$150:;
			OUTP( (char*)"add esp, 8" );
		}
		goto label$148;
		label$149:;
		{
			FBSTRING TMP$393$3;
			FBSTRING TMP$394$3;
			__builtin_memset( &TMP$394$3, 0, 24ll );
			__builtin_memset( &TMP$393$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$393$3, (void*)"fld ", 5ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$394$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$394$3 );
			fb_StrDelete( (FBSTRING*)&TMP$394$3 );
		}
		label$148:;
	}
	label$147:;
	label$146:;
	HPREPOPERAND64( DVREG$1, &DST$1, &AUX$1 );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$152;
	TMP$395$1 = 24ll;
	goto label$767;
	label$152:;
	TMP$395$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$767:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$395$1 * 56ll)) + 16ll) != 0ll) goto label$153;
	TMP$396$1 = (int64)-(*(int64*)((uint8*)DVREG$1 + 8ll) == 14ll);
	goto label$768;
	label$153:;
	TMP$396$1 = -1ll;
	label$768:;
	if( TMP$396$1 == 0ll) goto label$155;
	{
		FBSTRING TMP$397$2;
		FBSTRING TMP$398$2;
		OUTP( (char*)"sub esp, 8" );
		__builtin_memset( &TMP$397$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$397$2, (void*)"fistp ", 7ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
		__builtin_memset( &TMP$398$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$398$2, (void*)vr$37, -1ll, (void*)" [esp]", 7ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$40, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$154;
	label$155:;
	{
		OUTP( (char*)"fld st(0)" );
		OUTP( (char*)"push 0x4f800000" );
		OUTP( (char*)"fdiv dword ptr [esp]" );
		OUTP( (char*)"fistp dword ptr [esp]" );
		OUTP( (char*)"fild dword ptr [esp]" );
		OUTP( (char*)"push 0x4f800000" );
		OUTP( (char*)"fmul dword ptr [esp]" );
		OUTP( (char*)"fsubp" );
		OUTP( (char*)"fistp dword ptr [esp]" );
	}
	label$154:;
	HPOP( (char*)*(char**)&DST$1 );
	HPOP( (char*)*(char**)&AUX$1 );
	label$144:;
}

static void _EMITLOADF2I_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$405$1;
	int64 TMP$406$1;
	int64 TMP$407$1;
	label$156:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING SUFFIX$1;
	static FBSTRING AUX$1;
	static FBSTRING AUX8_16$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static int64 ISFREE$1;
	static int64 REG$1;
	static int64 WASREG$1;
	static struct $8FBSYMBOL* SYM$1;
	static struct $6IRVREG* TEMPVREG$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$158;
	TMP$405$1 = 24ll;
	goto label$769;
	label$158:;
	TMP$405$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$769:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$405$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$159;
	TMP$406$1 = 24ll;
	goto label$770;
	label$159:;
	TMP$406$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$770:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$406$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$160;
	TMP$407$1 = 24ll;
	goto label$771;
	label$160:;
	TMP$407$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$771:;
	if( ((int64)-(*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$407$1 * 56ll)) + 16ll) == 0ll) & (int64)-(DDSIZE$1 == 4ll)) == 0ll) goto label$162;
	{
		OUTP( (char*)"sub esp, 8" );
		if( *(int64*)SVREG$1 == 4ll) goto label$164;
		{
			FBSTRING TMP$408$3;
			FBSTRING TMP$409$3;
			__builtin_memset( &TMP$409$3, 0, 24ll );
			__builtin_memset( &TMP$408$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$408$3, (void*)"fld ", 5ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$409$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$409$3 );
			fb_StrDelete( (FBSTRING*)&TMP$409$3 );
		}
		goto label$163;
		label$164:;
		if( *(int64*)((uint8*)SVREG$1 + 32ll) != 1ll) goto label$165;
		{
			if( SDSIZE$1 <= 4ll) goto label$167;
			{
				FBSTRING TMP$410$4;
				FBSTRING TMP$411$4;
				__builtin_memset( &TMP$411$4, 0, 24ll );
				__builtin_memset( &TMP$410$4, 0, 24ll );
				FBSTRING* vr$29 = fb_StrConcat( &TMP$410$4, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$411$4, -1ll, (void*)vr$29, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$411$4 );
				fb_StrDelete( (FBSTRING*)&TMP$411$4 );
				OUTP( (char*)"fld qword ptr [esp]" );
			}
			goto label$166;
			label$167:;
			{
				FBSTRING TMP$412$4;
				FBSTRING TMP$413$4;
				__builtin_memset( &TMP$413$4, 0, 24ll );
				__builtin_memset( &TMP$412$4, 0, 24ll );
				FBSTRING* vr$35 = fb_StrConcat( &TMP$412$4, (void*)"movss dword ptr [esp], ", 24ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$413$4, -1ll, (void*)vr$35, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$413$4 );
				fb_StrDelete( (FBSTRING*)&TMP$413$4 );
				OUTP( (char*)"fld dword ptr [esp]" );
			}
			label$166:;
		}
		label$165:;
		label$163:;
		OUTP( (char*)"fistp qword ptr [esp]" );
		HPOP( (char*)*(char**)&DST$1 );
		OUTP( (char*)"add esp, 4" );
		goto label$157;
	}
	label$162:;
	label$161:;
	if( *(int64*)DVREG$1 != 4ll) goto label$169;
	{
		ISFREE$1 = -1ll;
		if( DDSIZE$1 >= 4ll) goto label$171;
		{
			char* vr$40 = HGETREGNAME( 8ll, *(int64*)((uint8*)DVREG$1 + 24ll) );
			fb_StrAssign( (void*)&DST$1, -1ll, (void*)vr$40, 0ll, 0 );
		}
		label$171:;
		label$170:;
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)&DST$1, -1ll, 0 );
		WASREG$1 = -1ll;
	}
	goto label$168;
	label$169:;
	{
		WASREG$1 = 0ll;
		int64 vr$41 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$1 = vr$41;
		char* vr$42 = HGETREGNAME( 8ll, REG$1 );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$42, 0ll, 0 );
		char* vr$44 = HGETREGNAME( *(int64*)((uint8*)DVREG$1 + 8ll), REG$1 );
		fb_StrAssign( (void*)&AUX8_16$1, -1ll, (void*)vr$44, 0ll, 0 );
		int64 vr$45 = HISREGFREE( 0ll, REG$1 );
		ISFREE$1 = vr$45;
		if( ISFREE$1 != 0ll) goto label$173;
		{
			HPUSH( (char*)*(char**)&AUX$1 );
		}
		label$173:;
		label$172:;
	}
	label$168:;
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$175;
	{
		OUTP( (char*)"sub esp, 4" );
		if( DDSIZE$1 != 2ll) goto label$177;
		{
			OUTP( (char*)"fistp word ptr [esp]" );
		}
		goto label$176;
		label$177:;
		{
			OUTP( (char*)"fistp dword ptr [esp]" );
		}
		label$176:;
		HPOP( (char*)*(char**)&AUX$1 );
	}
	goto label$174;
	label$175:;
	{
		int64 TMP$415$2;
		int64 TMP$416$2;
		if( SDSIZE$1 <= 4ll) goto label$178;
		TMP$415$2 = 100ll;
		goto label$772;
		label$178:;
		TMP$415$2 = 115ll;
		label$772:;
		FBSTRING* vr$47 = fb_CHR( 1, TMP$415$2 );
		fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)vr$47, -1ll, 0 );
		if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$179;
		TMP$416$2 = 24ll;
		goto label$773;
		label$179:;
		TMP$416$2 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
		label$773:;
		if( (*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$416$2 * 56ll)) + 16ll) & (int64)-(DDSIZE$1 == 2ll)) == 0ll) goto label$181;
		{
			FBSTRING TMP$423$3;
			FBSTRING TMP$424$3;
			FBSTRING TMP$425$3;
			FBSTRING TMP$426$3;
			FBSTRING TMP$427$3;
			FBSTRING TMP$430$3;
			FBSTRING TMP$431$3;
			FBSTRING TMP$432$3;
			FBSTRING TMP$433$3;
			if( *(int64*)SVREG$1 == 4ll) goto label$183;
			{
				if( SDSIZE$1 <= 4ll) goto label$185;
				{
					FBSTRING TMP$417$5;
					FBSTRING TMP$418$5;
					__builtin_memset( &TMP$418$5, 0, 24ll );
					__builtin_memset( &TMP$417$5, 0, 24ll );
					FBSTRING* vr$59 = fb_StrConcat( &TMP$417$5, (void*)"movlpd xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$418$5, -1ll, (void*)vr$59, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$418$5 );
					fb_StrDelete( (FBSTRING*)&TMP$418$5 );
				}
				goto label$184;
				label$185:;
				{
					FBSTRING TMP$419$5;
					FBSTRING TMP$420$5;
					__builtin_memset( &TMP$420$5, 0, 24ll );
					__builtin_memset( &TMP$419$5, 0, 24ll );
					FBSTRING* vr$65 = fb_StrConcat( &TMP$419$5, (void*)"movss xmm7, ", 13ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$420$5, -1ll, (void*)vr$65, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$420$5 );
					fb_StrDelete( (FBSTRING*)&TMP$420$5 );
				}
				label$184:;
				fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
			}
			label$183:;
			label$182:;
			__builtin_memset( &TMP$427$3, 0, 24ll );
			__builtin_memset( &TMP$423$3, 0, 24ll );
			FBSTRING* vr$71 = fb_StrConcat( &TMP$423$3, (void*)"cvtp", 5ll, (void*)&SUFFIX$1, -1ll );
			__builtin_memset( &TMP$424$3, 0, 24ll );
			FBSTRING* vr$74 = fb_StrConcat( &TMP$424$3, (void*)vr$71, -1ll, (void*)"2dq xmm7", 9ll );
			__builtin_memset( &TMP$425$3, 0, 24ll );
			FBSTRING* vr$77 = fb_StrConcat( &TMP$425$3, (void*)vr$74, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$426$3, 0, 24ll );
			FBSTRING* vr$80 = fb_StrConcat( &TMP$426$3, (void*)vr$77, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$427$3, -1ll, (void*)vr$80, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$427$3 );
			fb_StrDelete( (FBSTRING*)&TMP$427$3 );
			OUTP( (char*)"packssdw xmm7, xmm7" );
			__builtin_memset( &TMP$433$3, 0, 24ll );
			__builtin_memset( &TMP$430$3, 0, 24ll );
			FBSTRING* vr$86 = fb_StrConcat( &TMP$430$3, (void*)"movd ", 6ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$431$3, 0, 24ll );
			FBSTRING* vr$89 = fb_StrConcat( &TMP$431$3, (void*)vr$86, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$432$3, 0, 24ll );
			FBSTRING* vr$92 = fb_StrConcat( &TMP$432$3, (void*)vr$89, -1ll, (void*)"xmm7", 5ll );
			fb_StrAssign( (void*)&TMP$433$3, -1ll, (void*)vr$92, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$433$3 );
			fb_StrDelete( (FBSTRING*)&TMP$433$3 );
		}
		goto label$180;
		label$181:;
		{
			FBSTRING TMP$436$3;
			FBSTRING TMP$437$3;
			FBSTRING TMP$438$3;
			FBSTRING TMP$439$3;
			FBSTRING TMP$440$3;
			FBSTRING TMP$441$3;
			HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
			__builtin_memset( &TMP$441$3, 0, 24ll );
			__builtin_memset( &TMP$436$3, 0, 24ll );
			FBSTRING* vr$98 = fb_StrConcat( &TMP$436$3, (void*)"cvts", 5ll, (void*)&SUFFIX$1, -1ll );
			__builtin_memset( &TMP$437$3, 0, 24ll );
			FBSTRING* vr$101 = fb_StrConcat( &TMP$437$3, (void*)vr$98, -1ll, (void*)"2si ", 5ll );
			__builtin_memset( &TMP$438$3, 0, 24ll );
			FBSTRING* vr$104 = fb_StrConcat( &TMP$438$3, (void*)vr$101, -1ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$439$3, 0, 24ll );
			FBSTRING* vr$107 = fb_StrConcat( &TMP$439$3, (void*)vr$104, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$440$3, 0, 24ll );
			FBSTRING* vr$110 = fb_StrConcat( &TMP$440$3, (void*)vr$107, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$441$3, -1ll, (void*)vr$110, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$441$3 );
			fb_StrDelete( (FBSTRING*)&TMP$441$3 );
		}
		label$180:;
	}
	label$174:;
	if( WASREG$1 != 0ll) goto label$187;
	{
		if( DDSIZE$1 != 4ll) goto label$189;
		{
			FBSTRING TMP$442$3;
			FBSTRING TMP$443$3;
			FBSTRING TMP$444$3;
			FBSTRING TMP$445$3;
			__builtin_memset( &TMP$445$3, 0, 24ll );
			__builtin_memset( &TMP$442$3, 0, 24ll );
			FBSTRING* vr$116 = fb_StrConcat( &TMP$442$3, (void*)"mov ", 5ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$443$3, 0, 24ll );
			FBSTRING* vr$119 = fb_StrConcat( &TMP$443$3, (void*)vr$116, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$444$3, 0, 24ll );
			FBSTRING* vr$122 = fb_StrConcat( &TMP$444$3, (void*)vr$119, -1ll, (void*)&AUX$1, -1ll );
			fb_StrAssign( (void*)&TMP$445$3, -1ll, (void*)vr$122, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$445$3 );
			fb_StrDelete( (FBSTRING*)&TMP$445$3 );
		}
		goto label$188;
		label$189:;
		{
			FBSTRING TMP$446$3;
			FBSTRING TMP$447$3;
			FBSTRING TMP$448$3;
			FBSTRING TMP$449$3;
			__builtin_memset( &TMP$449$3, 0, 24ll );
			__builtin_memset( &TMP$446$3, 0, 24ll );
			FBSTRING* vr$128 = fb_StrConcat( &TMP$446$3, (void*)"mov ", 5ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$447$3, 0, 24ll );
			FBSTRING* vr$131 = fb_StrConcat( &TMP$447$3, (void*)vr$128, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$448$3, 0, 24ll );
			FBSTRING* vr$134 = fb_StrConcat( &TMP$448$3, (void*)vr$131, -1ll, (void*)&AUX8_16$1, -1ll );
			fb_StrAssign( (void*)&TMP$449$3, -1ll, (void*)vr$134, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$449$3 );
			fb_StrDelete( (FBSTRING*)&TMP$449$3 );
		}
		label$188:;
		if( ISFREE$1 != 0ll) goto label$191;
		{
			HPOP( (char*)*(char**)&AUX$1 );
		}
		label$191:;
		label$190:;
	}
	label$187:;
	label$186:;
	label$157:;
}

static void _EMITLOADL2F_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$454$1;
	FBSTRING TMP$455$1;
	FBSTRING TMP$456$1;
	FBSTRING TMP$465$1;
	FBSTRING TMP$466$1;
	label$192:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING AUX$1;
	static FBSTRING OSTR$1;
	static int64 DDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( ((int64)-(*(int64*)SVREG$1 == 4ll) | (int64)-(*(int64*)SVREG$1 == 0ll)) == 0ll) goto label$195;
	{
		int64 TMP$450$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$196;
		TMP$450$2 = 24ll;
		goto label$774;
		label$196:;
		TMP$450$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$774:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$450$2 * 56ll)) + 16ll) == 0ll) goto label$198;
		{
			FBSTRING TMP$451$3;
			FBSTRING TMP$452$3;
			HPREPOPERAND64( SVREG$1, &SRC$1, &AUX$1 );
			HPUSH( (char*)*(char**)&AUX$1 );
			HPUSH( (char*)*(char**)&SRC$1 );
			__builtin_memset( &TMP$451$3, 0, 24ll );
			FBSTRING* vr$15 = fb_StrConcat( &TMP$451$3, (void*)"fild ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)SVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
			__builtin_memset( &TMP$452$3, 0, 24ll );
			FBSTRING* vr$18 = fb_StrConcat( &TMP$452$3, (void*)vr$15, -1ll, (void*)" [esp]", 7ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$18, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			OUTP( (char*)"add esp, 8" );
		}
		goto label$197;
		label$198:;
		{
			HPREPOPERAND64( SVREG$1, &SRC$1, &AUX$1 );
			HPUSH( (char*)*(char**)&AUX$1 );
			HPUSH( (char*)*(char**)&SRC$1 );
			OUTP( (char*)"fild qword ptr [esp]" );
			OUTP( (char*)"add esp, 8" );
			HULONG2DBL( SVREG$1 );
		}
		label$197:;
	}
	goto label$194;
	label$195:;
	{
		int64 TMP$453$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$199;
		TMP$453$2 = 24ll;
		goto label$775;
		label$199:;
		TMP$453$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$775:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$453$2 * 56ll)) + 16ll) == 0ll) goto label$201;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$200;
		label$201:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			HULONG2DBL( SVREG$1 );
		}
		label$200:;
	}
	label$194:;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$202;
	TMP$454$1 = 24ll;
	goto label$776;
	label$202:;
	TMP$454$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$776:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$454$1 * 56ll)) + 8ll);
	__builtin_memset( &TMP$456$1, 0, 24ll );
	FBSTRING* vr$30 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$455$1, 0, 24ll );
	FBSTRING* vr$33 = fb_StrConcat( &TMP$455$1, (void*)"sub esp, ", 10ll, (void*)vr$30, -1ll );
	fb_StrAssign( (void*)&TMP$456$1, -1ll, (void*)vr$33, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$456$1 );
	fb_StrDelete( (FBSTRING*)&TMP$456$1 );
	if( DDSIZE$1 <= 4ll) goto label$204;
	{
		FBSTRING TMP$457$2;
		FBSTRING TMP$458$2;
		FBSTRING TMP$459$2;
		FBSTRING TMP$460$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$460$2, 0, 24ll );
		__builtin_memset( &TMP$457$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$457$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$458$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$458$2, (void*)vr$39, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$459$2, 0, 24ll );
		FBSTRING* vr$45 = fb_StrConcat( &TMP$459$2, (void*)vr$42, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$460$2, -1ll, (void*)vr$45, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$460$2 );
		fb_StrDelete( (FBSTRING*)&TMP$460$2 );
	}
	goto label$203;
	label$204:;
	{
		FBSTRING TMP$461$2;
		FBSTRING TMP$462$2;
		FBSTRING TMP$463$2;
		FBSTRING TMP$464$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$464$2, 0, 24ll );
		__builtin_memset( &TMP$461$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$461$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$462$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$462$2, (void*)vr$51, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$463$2, 0, 24ll );
		FBSTRING* vr$57 = fb_StrConcat( &TMP$463$2, (void*)vr$54, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$464$2, -1ll, (void*)vr$57, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$464$2 );
		fb_StrDelete( (FBSTRING*)&TMP$464$2 );
	}
	label$203:;
	__builtin_memset( &TMP$466$1, 0, 24ll );
	FBSTRING* vr$61 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$465$1, 0, 24ll );
	FBSTRING* vr$64 = fb_StrConcat( &TMP$465$1, (void*)"add esp, ", 10ll, (void*)vr$61, -1ll );
	fb_StrAssign( (void*)&TMP$466$1, -1ll, (void*)vr$64, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$466$1 );
	fb_StrDelete( (FBSTRING*)&TMP$466$1 );
	label$193:;
}

static void _EMITLOADI2F_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$467$1;
	int64 TMP$468$1;
	int64 TMP$469$1;
	FBSTRING TMP$538$1;
	FBSTRING TMP$539$1;
	FBSTRING TMP$548$1;
	FBSTRING TMP$549$1;
	label$205:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static FBSTRING SUFFIX$1;
	static FBSTRING AUX$1;
	static int64 ISFREE$1;
	static int64 REG$1;
	static struct $8FBSYMBOL* SYM$1;
	static struct $6IRVREG* TEMPVREG$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$207;
	TMP$467$1 = 24ll;
	goto label$777;
	label$207:;
	TMP$467$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$777:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$467$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$208;
	TMP$468$1 = 24ll;
	goto label$778;
	label$208:;
	TMP$468$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$778:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$468$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$209;
	TMP$469$1 = 24ll;
	goto label$779;
	label$209:;
	TMP$469$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$779:;
	if( ((int64)-(*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$469$1 * 56ll)) + 16ll) == 0ll) & (int64)-(SDSIZE$1 == 4ll)) == 0ll) goto label$211;
	{
		FBSTRING TMP$472$2;
		FBSTRING TMP$473$2;
		FBSTRING TMP$474$2;
		FBSTRING TMP$475$2;
		FBSTRING TMP$478$2;
		FBSTRING TMP$479$2;
		FBSTRING TMP$480$2;
		FBSTRING TMP$481$2;
		FBSTRING TMP$483$2;
		FBSTRING TMP$484$2;
		FBSTRING TMP$485$2;
		FBSTRING TMP$486$2;
		FBSTRING TMP$487$2;
		FBSTRING TMP$488$2;
		FBSTRING TMP$489$2;
		FBSTRING TMP$490$2;
		FBSTRING TMP$491$2;
		FBSTRING TMP$494$2;
		FBSTRING TMP$495$2;
		FBSTRING TMP$496$2;
		FBSTRING TMP$497$2;
		FBSTRING TMP$498$2;
		FBSTRING TMP$499$2;
		FBSTRING TMP$500$2;
		FBSTRING TMP$501$2;
		FBSTRING TMP$502$2;
		FBSTRING TMP$504$2;
		FBSTRING TMP$505$2;
		FBSTRING TMP$506$2;
		FBSTRING TMP$507$2;
		FBSTRING TMP$508$2;
		FBSTRING TMP$510$2;
		FBSTRING TMP$511$2;
		FBSTRING TMP$512$2;
		FBSTRING TMP$513$2;
		FBSTRING TMP$514$2;
		int64 vr$18 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$1 = vr$18;
		char* vr$19 = HGETREGNAME( 8ll, REG$1 );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$19, 0ll, 0 );
		int64 vr$20 = HISREGFREE( 0ll, REG$1 );
		ISFREE$1 = vr$20;
		if( ISFREE$1 != 0ll) goto label$213;
		{
			HPUSH( (char*)*(char**)&AUX$1 );
		}
		label$213:;
		label$212:;
		if( DDSIZE$1 <= 4ll) goto label$215;
		{
			struct $8FBSYMBOL* vr$21 = SYMBALLOCINTCONST( 4679240012837945344ll, 14ll );
			SYM$1 = vr$21;
			struct $6IRVREG* vr$23 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
			TEMPVREG$1 = vr$23;
			fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)"sd ", 4ll, 0 );
		}
		goto label$214;
		label$215:;
		{
			struct $8FBSYMBOL* vr$24 = SYMBALLOCINTCONST( 1199570944ll, 9ll );
			SYM$1 = vr$24;
			struct $6IRVREG* vr$26 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
			TEMPVREG$1 = vr$26;
			fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)"ss ", 4ll, 0 );
		}
		label$214:;
		*(int64*)((uint8*)SYM$1 + 168ll) = 16ll;
		__builtin_memset( &TMP$475$2, 0, 24ll );
		__builtin_memset( &TMP$472$2, 0, 24ll );
		FBSTRING* vr$31 = fb_StrConcat( &TMP$472$2, (void*)"mov ", 5ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$473$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$473$2, (void*)vr$31, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$474$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$474$2, (void*)vr$34, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$475$2, -1ll, (void*)vr$37, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$475$2 );
		fb_StrDelete( (FBSTRING*)&TMP$475$2 );
		__builtin_memset( &TMP$481$2, 0, 24ll );
		__builtin_memset( &TMP$478$2, 0, 24ll );
		FBSTRING* vr$43 = fb_StrConcat( &TMP$478$2, (void*)"and ", 5ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$479$2, 0, 24ll );
		FBSTRING* vr$46 = fb_StrConcat( &TMP$479$2, (void*)vr$43, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$480$2, 0, 24ll );
		FBSTRING* vr$49 = fb_StrConcat( &TMP$480$2, (void*)vr$46, -1ll, (void*)"0xFFFF", 7ll );
		fb_StrAssign( (void*)&TMP$481$2, -1ll, (void*)vr$49, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$481$2 );
		fb_StrDelete( (FBSTRING*)&TMP$481$2 );
		__builtin_memset( &TMP$487$2, 0, 24ll );
		__builtin_memset( &TMP$483$2, 0, 24ll );
		FBSTRING* vr$55 = fb_StrConcat( &TMP$483$2, (void*)"cvtsi2", 7ll, (void*)&SUFFIX$1, -1ll );
		__builtin_memset( &TMP$484$2, 0, 24ll );
		FBSTRING* vr$58 = fb_StrConcat( &TMP$484$2, (void*)vr$55, -1ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$485$2, 0, 24ll );
		FBSTRING* vr$61 = fb_StrConcat( &TMP$485$2, (void*)vr$58, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$486$2, 0, 24ll );
		FBSTRING* vr$64 = fb_StrConcat( &TMP$486$2, (void*)vr$61, -1ll, (void*)&AUX$1, -1ll );
		fb_StrAssign( (void*)&TMP$487$2, -1ll, (void*)vr$64, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$487$2 );
		fb_StrDelete( (FBSTRING*)&TMP$487$2 );
		__builtin_memset( &TMP$491$2, 0, 24ll );
		__builtin_memset( &TMP$488$2, 0, 24ll );
		FBSTRING* vr$70 = fb_StrConcat( &TMP$488$2, (void*)"mov ", 5ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$489$2, 0, 24ll );
		FBSTRING* vr$73 = fb_StrConcat( &TMP$489$2, (void*)vr$70, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$490$2, 0, 24ll );
		FBSTRING* vr$76 = fb_StrConcat( &TMP$490$2, (void*)vr$73, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$491$2, -1ll, (void*)vr$76, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$491$2 );
		fb_StrDelete( (FBSTRING*)&TMP$491$2 );
		__builtin_memset( &TMP$497$2, 0, 24ll );
		__builtin_memset( &TMP$494$2, 0, 24ll );
		FBSTRING* vr$82 = fb_StrConcat( &TMP$494$2, (void*)"shr ", 5ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$495$2, 0, 24ll );
		FBSTRING* vr$85 = fb_StrConcat( &TMP$495$2, (void*)vr$82, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$496$2, 0, 24ll );
		FBSTRING* vr$88 = fb_StrConcat( &TMP$496$2, (void*)vr$85, -1ll, (void*)"16", 3ll );
		fb_StrAssign( (void*)&TMP$497$2, -1ll, (void*)vr$88, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$497$2 );
		fb_StrDelete( (FBSTRING*)&TMP$497$2 );
		__builtin_memset( &TMP$502$2, 0, 24ll );
		__builtin_memset( &TMP$498$2, 0, 24ll );
		FBSTRING* vr$94 = fb_StrConcat( &TMP$498$2, (void*)"cvtsi2", 7ll, (void*)&SUFFIX$1, -1ll );
		__builtin_memset( &TMP$499$2, 0, 24ll );
		FBSTRING* vr$97 = fb_StrConcat( &TMP$499$2, (void*)vr$94, -1ll, (void*)"xmm7", 5ll );
		__builtin_memset( &TMP$500$2, 0, 24ll );
		FBSTRING* vr$100 = fb_StrConcat( &TMP$500$2, (void*)vr$97, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$501$2, 0, 24ll );
		FBSTRING* vr$103 = fb_StrConcat( &TMP$501$2, (void*)vr$100, -1ll, (void*)&AUX$1, -1ll );
		fb_StrAssign( (void*)&TMP$502$2, -1ll, (void*)vr$103, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$502$2 );
		fb_StrDelete( (FBSTRING*)&TMP$502$2 );
		HPREPOPERAND( TEMPVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$508$2, 0, 24ll );
		__builtin_memset( &TMP$504$2, 0, 24ll );
		FBSTRING* vr$109 = fb_StrConcat( &TMP$504$2, (void*)"mul", 4ll, (void*)&SUFFIX$1, -1ll );
		__builtin_memset( &TMP$505$2, 0, 24ll );
		FBSTRING* vr$112 = fb_StrConcat( &TMP$505$2, (void*)vr$109, -1ll, (void*)"xmm7", 5ll );
		__builtin_memset( &TMP$506$2, 0, 24ll );
		FBSTRING* vr$115 = fb_StrConcat( &TMP$506$2, (void*)vr$112, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$507$2, 0, 24ll );
		FBSTRING* vr$118 = fb_StrConcat( &TMP$507$2, (void*)vr$115, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$508$2, -1ll, (void*)vr$118, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$508$2 );
		fb_StrDelete( (FBSTRING*)&TMP$508$2 );
		__builtin_memset( &TMP$514$2, 0, 24ll );
		__builtin_memset( &TMP$510$2, 0, 24ll );
		FBSTRING* vr$124 = fb_StrConcat( &TMP$510$2, (void*)"add", 4ll, (void*)&SUFFIX$1, -1ll );
		__builtin_memset( &TMP$511$2, 0, 24ll );
		FBSTRING* vr$127 = fb_StrConcat( &TMP$511$2, (void*)vr$124, -1ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$512$2, 0, 24ll );
		FBSTRING* vr$130 = fb_StrConcat( &TMP$512$2, (void*)vr$127, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$513$2, 0, 24ll );
		FBSTRING* vr$133 = fb_StrConcat( &TMP$513$2, (void*)vr$130, -1ll, (void*)"xmm7", 5ll );
		fb_StrAssign( (void*)&TMP$514$2, -1ll, (void*)vr$133, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$514$2 );
		fb_StrDelete( (FBSTRING*)&TMP$514$2 );
		if( ISFREE$1 != 0ll) goto label$217;
		{
			HPOP( (char*)*(char**)&AUX$1 );
		}
		label$217:;
		label$216:;
		goto label$206;
	}
	label$211:;
	label$210:;
	if( ((int64)-(*(int64*)SVREG$1 != 0ll) & (int64)-(SDSIZE$1 == 4ll)) == 0ll) goto label$219;
	{
		ISFREE$1 = -1ll;
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	}
	goto label$218;
	label$219:;
	{
		int64 vr$140 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$1 = vr$140;
		char* vr$141 = HGETREGNAME( 8ll, REG$1 );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$141, 0ll, 0 );
		int64 vr$142 = HISREGFREE( 0ll, REG$1 );
		ISFREE$1 = vr$142;
		if( ISFREE$1 != 0ll) goto label$221;
		{
			HPUSH( (char*)*(char**)&AUX$1 );
		}
		label$221:;
		label$220:;
		if( ((int64)-(*(int64*)SVREG$1 == 0ll) | (int64)-(SDSIZE$1 == 4ll)) == 0ll) goto label$223;
		{
			FBSTRING TMP$515$3;
			FBSTRING TMP$516$3;
			FBSTRING TMP$517$3;
			FBSTRING TMP$518$3;
			__builtin_memset( &TMP$518$3, 0, 24ll );
			__builtin_memset( &TMP$515$3, 0, 24ll );
			FBSTRING* vr$150 = fb_StrConcat( &TMP$515$3, (void*)"mov ", 5ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$516$3, 0, 24ll );
			FBSTRING* vr$153 = fb_StrConcat( &TMP$516$3, (void*)vr$150, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$517$3, 0, 24ll );
			FBSTRING* vr$156 = fb_StrConcat( &TMP$517$3, (void*)vr$153, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$518$3, -1ll, (void*)vr$156, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$518$3 );
			fb_StrDelete( (FBSTRING*)&TMP$518$3 );
		}
		goto label$222;
		label$223:;
		{
			int64 TMP$519$3;
			if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$224;
			TMP$519$3 = 24ll;
			goto label$780;
			label$224:;
			TMP$519$3 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
			label$780:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$519$3 * 56ll)) + 16ll) == 0ll) goto label$226;
			{
				FBSTRING TMP$520$4;
				FBSTRING TMP$521$4;
				FBSTRING TMP$522$4;
				FBSTRING TMP$523$4;
				__builtin_memset( &TMP$523$4, 0, 24ll );
				__builtin_memset( &TMP$520$4, 0, 24ll );
				FBSTRING* vr$167 = fb_StrConcat( &TMP$520$4, (void*)"movsx ", 7ll, (void*)&AUX$1, -1ll );
				__builtin_memset( &TMP$521$4, 0, 24ll );
				FBSTRING* vr$170 = fb_StrConcat( &TMP$521$4, (void*)vr$167, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$522$4, 0, 24ll );
				FBSTRING* vr$173 = fb_StrConcat( &TMP$522$4, (void*)vr$170, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$523$4, -1ll, (void*)vr$173, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$523$4 );
				fb_StrDelete( (FBSTRING*)&TMP$523$4 );
			}
			goto label$225;
			label$226:;
			{
				FBSTRING TMP$524$4;
				FBSTRING TMP$525$4;
				FBSTRING TMP$526$4;
				FBSTRING TMP$527$4;
				__builtin_memset( &TMP$527$4, 0, 24ll );
				__builtin_memset( &TMP$524$4, 0, 24ll );
				FBSTRING* vr$179 = fb_StrConcat( &TMP$524$4, (void*)"movzx ", 7ll, (void*)&AUX$1, -1ll );
				__builtin_memset( &TMP$525$4, 0, 24ll );
				FBSTRING* vr$182 = fb_StrConcat( &TMP$525$4, (void*)vr$179, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$526$4, 0, 24ll );
				FBSTRING* vr$185 = fb_StrConcat( &TMP$526$4, (void*)vr$182, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$527$4, -1ll, (void*)vr$185, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$527$4 );
				fb_StrDelete( (FBSTRING*)&TMP$527$4 );
			}
			label$225:;
		}
		label$222:;
	}
	label$218:;
	if( DDSIZE$1 <= 4ll) goto label$228;
	{
		FBSTRING TMP$529$2;
		FBSTRING TMP$530$2;
		FBSTRING TMP$531$2;
		FBSTRING TMP$532$2;
		__builtin_memset( &TMP$532$2, 0, 24ll );
		__builtin_memset( &TMP$529$2, 0, 24ll );
		FBSTRING* vr$191 = fb_StrConcat( &TMP$529$2, (void*)"cvtsi2sd ", 10ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$530$2, 0, 24ll );
		FBSTRING* vr$194 = fb_StrConcat( &TMP$530$2, (void*)vr$191, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$531$2, 0, 24ll );
		FBSTRING* vr$197 = fb_StrConcat( &TMP$531$2, (void*)vr$194, -1ll, (void*)&AUX$1, -1ll );
		fb_StrAssign( (void*)&TMP$532$2, -1ll, (void*)vr$197, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$532$2 );
		fb_StrDelete( (FBSTRING*)&TMP$532$2 );
	}
	goto label$227;
	label$228:;
	{
		FBSTRING TMP$534$2;
		FBSTRING TMP$535$2;
		FBSTRING TMP$536$2;
		FBSTRING TMP$537$2;
		__builtin_memset( &TMP$537$2, 0, 24ll );
		__builtin_memset( &TMP$534$2, 0, 24ll );
		FBSTRING* vr$203 = fb_StrConcat( &TMP$534$2, (void*)"cvtsi2ss ", 10ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$535$2, 0, 24ll );
		FBSTRING* vr$206 = fb_StrConcat( &TMP$535$2, (void*)vr$203, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$536$2, 0, 24ll );
		FBSTRING* vr$209 = fb_StrConcat( &TMP$536$2, (void*)vr$206, -1ll, (void*)&AUX$1, -1ll );
		fb_StrAssign( (void*)&TMP$537$2, -1ll, (void*)vr$209, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$537$2 );
		fb_StrDelete( (FBSTRING*)&TMP$537$2 );
	}
	label$227:;
	if( ISFREE$1 != 0ll) goto label$230;
	{
		HPOP( (char*)*(char**)&AUX$1 );
	}
	label$230:;
	label$229:;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$232;
	{
		goto label$206;
		label$232:;
	}
	__builtin_memset( &TMP$539$1, 0, 24ll );
	FBSTRING* vr$214 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$538$1, 0, 24ll );
	FBSTRING* vr$217 = fb_StrConcat( &TMP$538$1, (void*)"sub esp, ", 10ll, (void*)vr$214, -1ll );
	fb_StrAssign( (void*)&TMP$539$1, -1ll, (void*)vr$217, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$539$1 );
	fb_StrDelete( (FBSTRING*)&TMP$539$1 );
	if( DDSIZE$1 <= 4ll) goto label$234;
	{
		FBSTRING TMP$542$2;
		FBSTRING TMP$543$2;
		__builtin_memset( &TMP$543$2, 0, 24ll );
		__builtin_memset( &TMP$542$2, 0, 24ll );
		FBSTRING* vr$223 = fb_StrConcat( &TMP$542$2, (void*)"movlpd [esp], ", 15ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$543$2, -1ll, (void*)vr$223, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$543$2 );
		fb_StrDelete( (FBSTRING*)&TMP$543$2 );
		OUTP( (char*)"fld qword ptr [esp]" );
	}
	goto label$233;
	label$234:;
	{
		FBSTRING TMP$546$2;
		FBSTRING TMP$547$2;
		__builtin_memset( &TMP$547$2, 0, 24ll );
		__builtin_memset( &TMP$546$2, 0, 24ll );
		FBSTRING* vr$229 = fb_StrConcat( &TMP$546$2, (void*)"movss [esp], ", 14ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$547$2, -1ll, (void*)vr$229, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$547$2 );
		fb_StrDelete( (FBSTRING*)&TMP$547$2 );
		OUTP( (char*)"fld dword ptr [esp]" );
	}
	label$233:;
	__builtin_memset( &TMP$549$1, 0, 24ll );
	FBSTRING* vr$233 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$548$1, 0, 24ll );
	FBSTRING* vr$236 = fb_StrConcat( &TMP$548$1, (void*)"add esp, ", 10ll, (void*)vr$233, -1ll );
	fb_StrAssign( (void*)&TMP$549$1, -1ll, (void*)vr$236, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$549$1 );
	fb_StrDelete( (FBSTRING*)&TMP$549$1 );
	label$206:;
}

static void _EMITLOADF2F_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$552$1;
	int64 TMP$553$1;
	label$235:;
	static FBSTRING SRC$1;
	static FBSTRING DST$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, 0ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$238;
	{
		FBSTRING TMP$550$2;
		FBSTRING TMP$551$2;
		HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$551$2, 0, 24ll );
		__builtin_memset( &TMP$550$2, 0, 24ll );
		FBSTRING* vr$4 = fb_StrConcat( &TMP$550$2, (void*)"fld ", 5ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$551$2, -1ll, (void*)vr$4, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$551$2 );
		fb_StrDelete( (FBSTRING*)&TMP$551$2 );
		goto label$236;
	}
	label$238:;
	label$237:;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$239;
	TMP$552$1 = 24ll;
	goto label$781;
	label$239:;
	TMP$552$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$781:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$552$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$240;
	TMP$553$1 = 24ll;
	goto label$782;
	label$240:;
	TMP$553$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$782:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$553$1 * 56ll)) + 8ll);
	if( SDSIZE$1 != DDSIZE$1) goto label$242;
	{
		if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$244;
		{
			HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
			if( DDSIZE$1 <= 4ll) goto label$246;
			{
				FBSTRING TMP$554$4;
				FBSTRING TMP$555$4;
				FBSTRING TMP$556$4;
				FBSTRING TMP$557$4;
				__builtin_memset( &TMP$557$4, 0, 24ll );
				__builtin_memset( &TMP$554$4, 0, 24ll );
				FBSTRING* vr$21 = fb_StrConcat( &TMP$554$4, (void*)"movupd ", 8ll, (void*)&DST$1, -1ll );
				__builtin_memset( &TMP$555$4, 0, 24ll );
				FBSTRING* vr$24 = fb_StrConcat( &TMP$555$4, (void*)vr$21, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$556$4, 0, 24ll );
				FBSTRING* vr$27 = fb_StrConcat( &TMP$556$4, (void*)vr$24, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$557$4, -1ll, (void*)vr$27, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$557$4 );
				fb_StrDelete( (FBSTRING*)&TMP$557$4 );
			}
			goto label$245;
			label$246:;
			{
				if( *(int64*)((uint8*)SVREG$1 + 40ll) != 2ll) goto label$248;
				{
					FBSTRING TMP$558$5;
					FBSTRING TMP$559$5;
					FBSTRING TMP$560$5;
					FBSTRING TMP$561$5;
					__builtin_memset( &TMP$561$5, 0, 24ll );
					__builtin_memset( &TMP$558$5, 0, 24ll );
					FBSTRING* vr$34 = fb_StrConcat( &TMP$558$5, (void*)"movlps ", 8ll, (void*)&DST$1, -1ll );
					__builtin_memset( &TMP$559$5, 0, 24ll );
					FBSTRING* vr$37 = fb_StrConcat( &TMP$559$5, (void*)vr$34, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$560$5, 0, 24ll );
					FBSTRING* vr$40 = fb_StrConcat( &TMP$560$5, (void*)vr$37, -1ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$561$5, -1ll, (void*)vr$40, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$561$5 );
					fb_StrDelete( (FBSTRING*)&TMP$561$5 );
				}
				goto label$247;
				label$248:;
				{
					FBSTRING TMP$562$5;
					FBSTRING TMP$563$5;
					FBSTRING TMP$564$5;
					FBSTRING TMP$565$5;
					__builtin_memset( &TMP$565$5, 0, 24ll );
					__builtin_memset( &TMP$562$5, 0, 24ll );
					FBSTRING* vr$46 = fb_StrConcat( &TMP$562$5, (void*)"movups ", 8ll, (void*)&DST$1, -1ll );
					__builtin_memset( &TMP$563$5, 0, 24ll );
					FBSTRING* vr$49 = fb_StrConcat( &TMP$563$5, (void*)vr$46, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$564$5, 0, 24ll );
					FBSTRING* vr$52 = fb_StrConcat( &TMP$564$5, (void*)vr$49, -1ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$565$5, -1ll, (void*)vr$52, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$565$5 );
					fb_StrDelete( (FBSTRING*)&TMP$565$5 );
				}
				label$247:;
			}
			label$245:;
			goto label$236;
		}
		label$244:;
		label$243:;
		if( DDSIZE$1 <= 4ll) goto label$250;
		{
			FBSTRING TMP$566$3;
			FBSTRING TMP$567$3;
			FBSTRING TMP$568$3;
			FBSTRING TMP$569$3;
			__builtin_memset( &TMP$569$3, 0, 24ll );
			__builtin_memset( &TMP$566$3, 0, 24ll );
			FBSTRING* vr$58 = fb_StrConcat( &TMP$566$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$567$3, 0, 24ll );
			FBSTRING* vr$61 = fb_StrConcat( &TMP$567$3, (void*)vr$58, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$568$3, 0, 24ll );
			FBSTRING* vr$64 = fb_StrConcat( &TMP$568$3, (void*)vr$61, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$569$3, -1ll, (void*)vr$64, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$569$3 );
			fb_StrDelete( (FBSTRING*)&TMP$569$3 );
		}
		goto label$249;
		label$250:;
		{
			FBSTRING TMP$570$3;
			FBSTRING TMP$571$3;
			FBSTRING TMP$572$3;
			FBSTRING TMP$573$3;
			__builtin_memset( &TMP$573$3, 0, 24ll );
			__builtin_memset( &TMP$570$3, 0, 24ll );
			FBSTRING* vr$70 = fb_StrConcat( &TMP$570$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$571$3, 0, 24ll );
			FBSTRING* vr$73 = fb_StrConcat( &TMP$571$3, (void*)vr$70, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$572$3, 0, 24ll );
			FBSTRING* vr$76 = fb_StrConcat( &TMP$572$3, (void*)vr$73, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$573$3, -1ll, (void*)vr$76, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$573$3 );
			fb_StrDelete( (FBSTRING*)&TMP$573$3 );
		}
		label$249:;
	}
	goto label$241;
	label$242:;
	if( SDSIZE$1 <= 4ll) goto label$251;
	{
		if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$253;
		{
			FBSTRING TMP$575$3;
			FBSTRING TMP$576$3;
			FBSTRING TMP$577$3;
			FBSTRING TMP$578$3;
			__builtin_memset( &TMP$578$3, 0, 24ll );
			__builtin_memset( &TMP$575$3, 0, 24ll );
			FBSTRING* vr$83 = fb_StrConcat( &TMP$575$3, (void*)"cvtpd2ps ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$576$3, 0, 24ll );
			FBSTRING* vr$86 = fb_StrConcat( &TMP$576$3, (void*)vr$83, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$577$3, 0, 24ll );
			FBSTRING* vr$89 = fb_StrConcat( &TMP$577$3, (void*)vr$86, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$578$3, -1ll, (void*)vr$89, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$578$3 );
			fb_StrDelete( (FBSTRING*)&TMP$578$3 );
		}
		goto label$252;
		label$253:;
		{
			FBSTRING TMP$579$3;
			FBSTRING TMP$580$3;
			FBSTRING TMP$581$3;
			FBSTRING TMP$582$3;
			__builtin_memset( &TMP$582$3, 0, 24ll );
			__builtin_memset( &TMP$579$3, 0, 24ll );
			FBSTRING* vr$95 = fb_StrConcat( &TMP$579$3, (void*)"cvtsd2ss ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$580$3, 0, 24ll );
			FBSTRING* vr$98 = fb_StrConcat( &TMP$580$3, (void*)vr$95, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$581$3, 0, 24ll );
			FBSTRING* vr$101 = fb_StrConcat( &TMP$581$3, (void*)vr$98, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$582$3, -1ll, (void*)vr$101, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$582$3 );
			fb_StrDelete( (FBSTRING*)&TMP$582$3 );
		}
		label$252:;
	}
	goto label$241;
	label$251:;
	{
		if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$255;
		{
			FBSTRING TMP$584$3;
			FBSTRING TMP$585$3;
			FBSTRING TMP$586$3;
			FBSTRING TMP$587$3;
			__builtin_memset( &TMP$587$3, 0, 24ll );
			__builtin_memset( &TMP$584$3, 0, 24ll );
			FBSTRING* vr$108 = fb_StrConcat( &TMP$584$3, (void*)"cvtps2pd ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$585$3, 0, 24ll );
			FBSTRING* vr$111 = fb_StrConcat( &TMP$585$3, (void*)vr$108, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$586$3, 0, 24ll );
			FBSTRING* vr$114 = fb_StrConcat( &TMP$586$3, (void*)vr$111, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$587$3, -1ll, (void*)vr$114, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$587$3 );
			fb_StrDelete( (FBSTRING*)&TMP$587$3 );
		}
		goto label$254;
		label$255:;
		{
			FBSTRING TMP$588$3;
			FBSTRING TMP$589$3;
			FBSTRING TMP$590$3;
			FBSTRING TMP$591$3;
			__builtin_memset( &TMP$591$3, 0, 24ll );
			__builtin_memset( &TMP$588$3, 0, 24ll );
			FBSTRING* vr$120 = fb_StrConcat( &TMP$588$3, (void*)"cvtss2sd ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$589$3, 0, 24ll );
			FBSTRING* vr$123 = fb_StrConcat( &TMP$589$3, (void*)vr$120, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$590$3, 0, 24ll );
			FBSTRING* vr$126 = fb_StrConcat( &TMP$590$3, (void*)vr$123, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$591$3, -1ll, (void*)vr$126, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$591$3 );
			fb_StrDelete( (FBSTRING*)&TMP$591$3 );
		}
		label$254:;
	}
	label$241:;
	label$236:;
}

static void _EMITMOVF_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$592$1;
	int64 TMP$593$1;
	label$256:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, 0ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$258;
	TMP$592$1 = 24ll;
	goto label$783;
	label$258:;
	TMP$592$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$783:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$592$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$259;
	TMP$593$1 = 24ll;
	goto label$784;
	label$259:;
	TMP$593$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$784:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$593$1 * 56ll)) + 8ll);
	if( ((int64)-(SDSIZE$1 > 4ll) & (int64)-(DDSIZE$1 <= 4ll)) == 0ll) goto label$261;
	{
		if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$263;
		{
			FBSTRING TMP$594$3;
			FBSTRING TMP$595$3;
			FBSTRING TMP$596$3;
			FBSTRING TMP$597$3;
			__builtin_memset( &TMP$597$3, 0, 24ll );
			__builtin_memset( &TMP$594$3, 0, 24ll );
			FBSTRING* vr$17 = fb_StrConcat( &TMP$594$3, (void*)"cvtpd2ps ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$595$3, 0, 24ll );
			FBSTRING* vr$20 = fb_StrConcat( &TMP$595$3, (void*)vr$17, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$596$3, 0, 24ll );
			FBSTRING* vr$23 = fb_StrConcat( &TMP$596$3, (void*)vr$20, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$597$3, -1ll, (void*)vr$23, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$597$3 );
			fb_StrDelete( (FBSTRING*)&TMP$597$3 );
		}
		goto label$262;
		label$263:;
		{
			FBSTRING TMP$598$3;
			FBSTRING TMP$599$3;
			FBSTRING TMP$600$3;
			FBSTRING TMP$601$3;
			__builtin_memset( &TMP$601$3, 0, 24ll );
			__builtin_memset( &TMP$598$3, 0, 24ll );
			FBSTRING* vr$29 = fb_StrConcat( &TMP$598$3, (void*)"cvtsd2ss ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$599$3, 0, 24ll );
			FBSTRING* vr$32 = fb_StrConcat( &TMP$599$3, (void*)vr$29, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$600$3, 0, 24ll );
			FBSTRING* vr$35 = fb_StrConcat( &TMP$600$3, (void*)vr$32, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$601$3, -1ll, (void*)vr$35, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$601$3 );
			fb_StrDelete( (FBSTRING*)&TMP$601$3 );
		}
		label$262:;
	}
	goto label$260;
	label$261:;
	if( ((int64)-(DDSIZE$1 > 4ll) & (int64)-(SDSIZE$1 <= 4ll)) == 0ll) goto label$264;
	{
		if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$266;
		{
			FBSTRING TMP$602$3;
			FBSTRING TMP$603$3;
			FBSTRING TMP$604$3;
			FBSTRING TMP$605$3;
			__builtin_memset( &TMP$605$3, 0, 24ll );
			__builtin_memset( &TMP$602$3, 0, 24ll );
			FBSTRING* vr$45 = fb_StrConcat( &TMP$602$3, (void*)"cvtps2pd ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$603$3, 0, 24ll );
			FBSTRING* vr$48 = fb_StrConcat( &TMP$603$3, (void*)vr$45, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$604$3, 0, 24ll );
			FBSTRING* vr$51 = fb_StrConcat( &TMP$604$3, (void*)vr$48, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$605$3, -1ll, (void*)vr$51, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$605$3 );
			fb_StrDelete( (FBSTRING*)&TMP$605$3 );
		}
		goto label$265;
		label$266:;
		{
			FBSTRING TMP$606$3;
			FBSTRING TMP$607$3;
			FBSTRING TMP$608$3;
			FBSTRING TMP$609$3;
			__builtin_memset( &TMP$609$3, 0, 24ll );
			__builtin_memset( &TMP$606$3, 0, 24ll );
			FBSTRING* vr$57 = fb_StrConcat( &TMP$606$3, (void*)"cvtss2sd ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$607$3, 0, 24ll );
			FBSTRING* vr$60 = fb_StrConcat( &TMP$607$3, (void*)vr$57, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$608$3, 0, 24ll );
			FBSTRING* vr$63 = fb_StrConcat( &TMP$608$3, (void*)vr$60, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$609$3, -1ll, (void*)vr$63, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$609$3 );
			fb_StrDelete( (FBSTRING*)&TMP$609$3 );
		}
		label$265:;
	}
	goto label$260;
	label$264:;
	{
		FBSTRING TMP$611$2;
		FBSTRING TMP$612$2;
		FBSTRING TMP$613$2;
		FBSTRING TMP$614$2;
		__builtin_memset( &TMP$614$2, 0, 24ll );
		__builtin_memset( &TMP$611$2, 0, 24ll );
		FBSTRING* vr$69 = fb_StrConcat( &TMP$611$2, (void*)"movaps ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$612$2, 0, 24ll );
		FBSTRING* vr$72 = fb_StrConcat( &TMP$612$2, (void*)vr$69, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$613$2, 0, 24ll );
		FBSTRING* vr$75 = fb_StrConcat( &TMP$613$2, (void*)vr$72, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$614$2, -1ll, (void*)vr$75, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$614$2 );
		fb_StrDelete( (FBSTRING*)&TMP$614$2 );
	}
	label$260:;
	label$257:;
}

static void _EMITSWZREPF_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$615$1;
	label$267:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$269;
	TMP$615$1 = 24ll;
	goto label$785;
	label$269:;
	TMP$615$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$785:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$615$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( DDSIZE$1 <= 4ll) goto label$271;
	{
		FBSTRING TMP$617$2;
		FBSTRING TMP$618$2;
		FBSTRING TMP$619$2;
		FBSTRING TMP$620$2;
		__builtin_memset( &TMP$620$2, 0, 24ll );
		__builtin_memset( &TMP$617$2, 0, 24ll );
		FBSTRING* vr$8 = fb_StrConcat( &TMP$617$2, (void*)"unpcklpd ", 10ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$618$2, 0, 24ll );
		FBSTRING* vr$11 = fb_StrConcat( &TMP$618$2, (void*)vr$8, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$619$2, 0, 24ll );
		FBSTRING* vr$14 = fb_StrConcat( &TMP$619$2, (void*)vr$11, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$620$2, -1ll, (void*)vr$14, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$620$2 );
		fb_StrDelete( (FBSTRING*)&TMP$620$2 );
	}
	goto label$270;
	label$271:;
	{
		if( *(int64*)((uint8*)DVREG$1 + 40ll) != 2ll) goto label$273;
		{
			FBSTRING TMP$622$3;
			FBSTRING TMP$623$3;
			FBSTRING TMP$624$3;
			FBSTRING TMP$625$3;
			__builtin_memset( &TMP$625$3, 0, 24ll );
			__builtin_memset( &TMP$622$3, 0, 24ll );
			FBSTRING* vr$21 = fb_StrConcat( &TMP$622$3, (void*)"unpcklps ", 10ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$623$3, 0, 24ll );
			FBSTRING* vr$24 = fb_StrConcat( &TMP$623$3, (void*)vr$21, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$624$3, 0, 24ll );
			FBSTRING* vr$27 = fb_StrConcat( &TMP$624$3, (void*)vr$24, -1ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$625$3, -1ll, (void*)vr$27, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$625$3 );
			fb_StrDelete( (FBSTRING*)&TMP$625$3 );
		}
		goto label$272;
		label$273:;
		{
			FBSTRING TMP$628$3;
			FBSTRING TMP$629$3;
			FBSTRING TMP$630$3;
			FBSTRING TMP$631$3;
			FBSTRING TMP$632$3;
			FBSTRING TMP$633$3;
			__builtin_memset( &TMP$633$3, 0, 24ll );
			__builtin_memset( &TMP$628$3, 0, 24ll );
			FBSTRING* vr$33 = fb_StrConcat( &TMP$628$3, (void*)"shufps ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$629$3, 0, 24ll );
			FBSTRING* vr$36 = fb_StrConcat( &TMP$629$3, (void*)vr$33, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$630$3, 0, 24ll );
			FBSTRING* vr$39 = fb_StrConcat( &TMP$630$3, (void*)vr$36, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$631$3, 0, 24ll );
			FBSTRING* vr$42 = fb_StrConcat( &TMP$631$3, (void*)vr$39, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$632$3, 0, 24ll );
			FBSTRING* vr$45 = fb_StrConcat( &TMP$632$3, (void*)vr$42, -1ll, (void*)"0x0", 4ll );
			fb_StrAssign( (void*)&TMP$633$3, -1ll, (void*)vr$45, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$633$3 );
			fb_StrDelete( (FBSTRING*)&TMP$633$3 );
		}
		label$272:;
	}
	label$270:;
	label$268:;
}

static int64 HEMITCONVERTOPERANDS_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$634$1;
	int64 TMP$635$1;
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$274:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$276;
	TMP$634$1 = 24ll;
	goto label$786;
	label$276:;
	TMP$634$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$786:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$634$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$277;
	TMP$635$1 = 24ll;
	goto label$787;
	label$277:;
	TMP$635$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$787:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$635$1 * 56ll)) + 8ll);
	fb$result$1 = 0ll;
	if( DDSIZE$1 <= 4ll) goto label$279;
	{
		if( SDSIZE$1 != 4ll) goto label$281;
		{
			if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$283;
			{
				FBSTRING TMP$636$4;
				FBSTRING TMP$637$4;
				__builtin_memset( &TMP$637$4, 0, 24ll );
				__builtin_memset( &TMP$636$4, 0, 24ll );
				FBSTRING* vr$15 = fb_StrConcat( &TMP$636$4, (void*)"cvtps2pd xmm7, ", 16ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$637$4, -1ll, (void*)vr$15, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$637$4 );
				fb_StrDelete( (FBSTRING*)&TMP$637$4 );
			}
			goto label$282;
			label$283:;
			{
				FBSTRING TMP$638$4;
				FBSTRING TMP$639$4;
				__builtin_memset( &TMP$639$4, 0, 24ll );
				__builtin_memset( &TMP$638$4, 0, 24ll );
				FBSTRING* vr$21 = fb_StrConcat( &TMP$638$4, (void*)"cvtss2sd xmm7, ", 16ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$639$4, -1ll, (void*)vr$21, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$639$4 );
				fb_StrDelete( (FBSTRING*)&TMP$639$4 );
			}
			label$282:;
			fb$result$1 = -1ll;
		}
		label$281:;
		label$280:;
	}
	goto label$278;
	label$279:;
	{
		if( SDSIZE$1 <= 4ll) goto label$285;
		{
			if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$287;
			{
				FBSTRING TMP$640$4;
				FBSTRING TMP$641$4;
				__builtin_memset( &TMP$641$4, 0, 24ll );
				__builtin_memset( &TMP$640$4, 0, 24ll );
				FBSTRING* vr$28 = fb_StrConcat( &TMP$640$4, (void*)"cvtpd2ps xmm7, ", 16ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$641$4, -1ll, (void*)vr$28, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$641$4 );
				fb_StrDelete( (FBSTRING*)&TMP$641$4 );
			}
			goto label$286;
			label$287:;
			{
				FBSTRING TMP$642$4;
				FBSTRING TMP$643$4;
				__builtin_memset( &TMP$643$4, 0, 24ll );
				__builtin_memset( &TMP$642$4, 0, 24ll );
				FBSTRING* vr$34 = fb_StrConcat( &TMP$642$4, (void*)"cvtsd2ss xmm7, ", 16ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$643$4, -1ll, (void*)vr$34, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$643$4 );
				fb_StrDelete( (FBSTRING*)&TMP$643$4 );
			}
			label$286:;
			fb$result$1 = -1ll;
		}
		label$285:;
		label$284:;
	}
	label$278:;
	label$275:;
	return fb$result$1;
}

static void _EMITADDF_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$644$1;
	int64 TMP$645$1;
	int64 TMP$666$1;
	label$288:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static int64 RETURNSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$290;
	TMP$644$1 = 24ll;
	goto label$788;
	label$290:;
	TMP$644$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$788:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$644$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$291;
	TMP$645$1 = 24ll;
	goto label$789;
	label$291:;
	TMP$645$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$789:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$645$1 * 56ll)) + 8ll);
	RETURNSIZE$1 = 0ll;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$293;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		RETURNSIZE$1 = DDSIZE$1;
	}
	goto label$292;
	label$293:;
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$294;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		RETURNSIZE$1 = SDSIZE$1;
	}
	label$294:;
	label$292:;
	if( RETURNSIZE$1 == 0ll) goto label$296;
	{
		FBSTRING TMP$646$2;
		FBSTRING TMP$647$2;
		__builtin_memset( &TMP$647$2, 0, 24ll );
		FBSTRING* vr$13 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$646$2, 0, 24ll );
		FBSTRING* vr$16 = fb_StrConcat( &TMP$646$2, (void*)"sub esp, ", 10ll, (void*)vr$13, -1ll );
		fb_StrAssign( (void*)&TMP$647$2, -1ll, (void*)vr$16, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$647$2 );
		fb_StrDelete( (FBSTRING*)&TMP$647$2 );
	}
	label$296:;
	label$295:;
	if( RETURNSIZE$1 != 8ll) goto label$298;
	{
		FBSTRING TMP$648$2;
		FBSTRING TMP$649$2;
		FBSTRING TMP$650$2;
		FBSTRING TMP$651$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$651$2, 0, 24ll );
		__builtin_memset( &TMP$648$2, 0, 24ll );
		FBSTRING* vr$22 = fb_StrConcat( &TMP$648$2, (void*)"movlpd ", 8ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$649$2, 0, 24ll );
		FBSTRING* vr$25 = fb_StrConcat( &TMP$649$2, (void*)vr$22, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$650$2, 0, 24ll );
		FBSTRING* vr$28 = fb_StrConcat( &TMP$650$2, (void*)vr$25, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$651$2, -1ll, (void*)vr$28, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$651$2 );
		fb_StrDelete( (FBSTRING*)&TMP$651$2 );
	}
	goto label$297;
	label$298:;
	if( RETURNSIZE$1 != 4ll) goto label$299;
	{
		FBSTRING TMP$652$2;
		FBSTRING TMP$653$2;
		FBSTRING TMP$654$2;
		FBSTRING TMP$655$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$655$2, 0, 24ll );
		__builtin_memset( &TMP$652$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$652$2, (void*)"movss ", 7ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$653$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$653$2, (void*)vr$34, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$654$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$654$2, (void*)vr$37, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$655$2, -1ll, (void*)vr$40, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$655$2 );
		fb_StrDelete( (FBSTRING*)&TMP$655$2 );
	}
	label$299:;
	label$297:;
	if( RETURNSIZE$1 == 0ll) goto label$301;
	{
		FBSTRING TMP$656$2;
		FBSTRING TMP$657$2;
		__builtin_memset( &TMP$657$2, 0, 24ll );
		FBSTRING* vr$44 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$656$2, 0, 24ll );
		FBSTRING* vr$47 = fb_StrConcat( &TMP$656$2, (void*)"add esp, ", 10ll, (void*)vr$44, -1ll );
		fb_StrAssign( (void*)&TMP$657$2, -1ll, (void*)vr$47, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$657$2 );
		fb_StrDelete( (FBSTRING*)&TMP$657$2 );
	}
	label$301:;
	label$300:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"adds", 5ll, 0 );
	if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$303;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"addp", 5ll, 0 );
		if( *(int64*)SVREG$1 == 4ll) goto label$305;
		{
			HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
			if( SDSIZE$1 <= 4ll) goto label$307;
			{
				FBSTRING TMP$660$4;
				FBSTRING TMP$661$4;
				__builtin_memset( &TMP$661$4, 0, 24ll );
				__builtin_memset( &TMP$660$4, 0, 24ll );
				FBSTRING* vr$55 = fb_StrConcat( &TMP$660$4, (void*)"movupd xmm7, ", 14ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$661$4, -1ll, (void*)vr$55, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$661$4 );
				fb_StrDelete( (FBSTRING*)&TMP$661$4 );
			}
			goto label$306;
			label$307:;
			{
				if( *(int64*)((uint8*)SVREG$1 + 40ll) != 2ll) goto label$309;
				{
					FBSTRING TMP$662$5;
					FBSTRING TMP$663$5;
					__builtin_memset( &TMP$663$5, 0, 24ll );
					__builtin_memset( &TMP$662$5, 0, 24ll );
					FBSTRING* vr$62 = fb_StrConcat( &TMP$662$5, (void*)"movlps xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$663$5, -1ll, (void*)vr$62, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$663$5 );
					fb_StrDelete( (FBSTRING*)&TMP$663$5 );
				}
				goto label$308;
				label$309:;
				{
					FBSTRING TMP$664$5;
					FBSTRING TMP$665$5;
					__builtin_memset( &TMP$665$5, 0, 24ll );
					__builtin_memset( &TMP$664$5, 0, 24ll );
					FBSTRING* vr$68 = fb_StrConcat( &TMP$664$5, (void*)"movups xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$665$5, -1ll, (void*)vr$68, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$665$5 );
					fb_StrDelete( (FBSTRING*)&TMP$665$5 );
				}
				label$308:;
			}
			label$306:;
			fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
		}
		label$305:;
		label$304:;
	}
	label$303:;
	label$302:;
	int64 vr$71 = HEMITCONVERTOPERANDS_SSE( DVREG$1, SVREG$1 );
	if( vr$71 == 0ll) goto label$311;
	{
		fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
	}
	label$311:;
	label$310:;
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$312;
	TMP$666$1 = 24ll;
	goto label$790;
	label$312:;
	TMP$666$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$790:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$666$1 * 56ll)) != 1ll) goto label$314;
	{
		if( DDSIZE$1 <= 4ll) goto label$316;
		{
			FBSTRING TMP$668$3;
			FBSTRING TMP$669$3;
			FBSTRING TMP$670$3;
			FBSTRING TMP$671$3;
			FBSTRING TMP$672$3;
			__builtin_memset( &TMP$672$3, 0, 24ll );
			__builtin_memset( &TMP$668$3, 0, 24ll );
			FBSTRING* vr$80 = fb_StrConcat( &TMP$668$3, (void*)&OSTR$1, -1ll, (void*)"d ", 3ll );
			__builtin_memset( &TMP$669$3, 0, 24ll );
			FBSTRING* vr$83 = fb_StrConcat( &TMP$669$3, (void*)vr$80, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$670$3, 0, 24ll );
			FBSTRING* vr$86 = fb_StrConcat( &TMP$670$3, (void*)vr$83, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$671$3, 0, 24ll );
			FBSTRING* vr$89 = fb_StrConcat( &TMP$671$3, (void*)vr$86, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$672$3, -1ll, (void*)vr$89, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$672$3 );
			fb_StrDelete( (FBSTRING*)&TMP$672$3 );
		}
		goto label$315;
		label$316:;
		{
			FBSTRING TMP$674$3;
			FBSTRING TMP$675$3;
			FBSTRING TMP$676$3;
			FBSTRING TMP$677$3;
			FBSTRING TMP$678$3;
			__builtin_memset( &TMP$678$3, 0, 24ll );
			__builtin_memset( &TMP$674$3, 0, 24ll );
			FBSTRING* vr$95 = fb_StrConcat( &TMP$674$3, (void*)&OSTR$1, -1ll, (void*)"s ", 3ll );
			__builtin_memset( &TMP$675$3, 0, 24ll );
			FBSTRING* vr$98 = fb_StrConcat( &TMP$675$3, (void*)vr$95, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$676$3, 0, 24ll );
			FBSTRING* vr$101 = fb_StrConcat( &TMP$676$3, (void*)vr$98, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$677$3, 0, 24ll );
			FBSTRING* vr$104 = fb_StrConcat( &TMP$677$3, (void*)vr$101, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$678$3, -1ll, (void*)vr$104, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$678$3 );
			fb_StrDelete( (FBSTRING*)&TMP$678$3 );
		}
		label$315:;
	}
	goto label$313;
	label$314:;
	{
		OUTP( (char*)" implement 'add integer to float'" );
	}
	label$313:;
	label$289:;
}

static void _EMITSUBF_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$680$1;
	int64 TMP$681$1;
	int64 TMP$702$1;
	label$317:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static int64 RETURNSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$319;
	TMP$680$1 = 24ll;
	goto label$791;
	label$319:;
	TMP$680$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$791:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$680$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$320;
	TMP$681$1 = 24ll;
	goto label$792;
	label$320:;
	TMP$681$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$792:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$681$1 * 56ll)) + 8ll);
	RETURNSIZE$1 = 0ll;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$322;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		RETURNSIZE$1 = DDSIZE$1;
	}
	goto label$321;
	label$322:;
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$323;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		RETURNSIZE$1 = SDSIZE$1;
	}
	label$323:;
	label$321:;
	if( RETURNSIZE$1 == 0ll) goto label$325;
	{
		FBSTRING TMP$682$2;
		FBSTRING TMP$683$2;
		__builtin_memset( &TMP$683$2, 0, 24ll );
		FBSTRING* vr$13 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$682$2, 0, 24ll );
		FBSTRING* vr$16 = fb_StrConcat( &TMP$682$2, (void*)"sub esp, ", 10ll, (void*)vr$13, -1ll );
		fb_StrAssign( (void*)&TMP$683$2, -1ll, (void*)vr$16, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$683$2 );
		fb_StrDelete( (FBSTRING*)&TMP$683$2 );
	}
	label$325:;
	label$324:;
	if( RETURNSIZE$1 != 8ll) goto label$327;
	{
		FBSTRING TMP$684$2;
		FBSTRING TMP$685$2;
		FBSTRING TMP$686$2;
		FBSTRING TMP$687$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$687$2, 0, 24ll );
		__builtin_memset( &TMP$684$2, 0, 24ll );
		FBSTRING* vr$22 = fb_StrConcat( &TMP$684$2, (void*)"movlpd ", 8ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$685$2, 0, 24ll );
		FBSTRING* vr$25 = fb_StrConcat( &TMP$685$2, (void*)vr$22, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$686$2, 0, 24ll );
		FBSTRING* vr$28 = fb_StrConcat( &TMP$686$2, (void*)vr$25, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$687$2, -1ll, (void*)vr$28, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$687$2 );
		fb_StrDelete( (FBSTRING*)&TMP$687$2 );
	}
	goto label$326;
	label$327:;
	if( RETURNSIZE$1 != 4ll) goto label$328;
	{
		FBSTRING TMP$688$2;
		FBSTRING TMP$689$2;
		FBSTRING TMP$690$2;
		FBSTRING TMP$691$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$691$2, 0, 24ll );
		__builtin_memset( &TMP$688$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$688$2, (void*)"movss ", 7ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$689$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$689$2, (void*)vr$34, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$690$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$690$2, (void*)vr$37, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$691$2, -1ll, (void*)vr$40, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$691$2 );
		fb_StrDelete( (FBSTRING*)&TMP$691$2 );
	}
	label$328:;
	label$326:;
	if( RETURNSIZE$1 == 0ll) goto label$330;
	{
		FBSTRING TMP$692$2;
		FBSTRING TMP$693$2;
		__builtin_memset( &TMP$693$2, 0, 24ll );
		FBSTRING* vr$44 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$692$2, 0, 24ll );
		FBSTRING* vr$47 = fb_StrConcat( &TMP$692$2, (void*)"add esp, ", 10ll, (void*)vr$44, -1ll );
		fb_StrAssign( (void*)&TMP$693$2, -1ll, (void*)vr$47, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$693$2 );
		fb_StrDelete( (FBSTRING*)&TMP$693$2 );
	}
	label$330:;
	label$329:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"subs", 5ll, 0 );
	if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$332;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"subp", 5ll, 0 );
		if( *(int64*)SVREG$1 == 4ll) goto label$334;
		{
			HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
			if( SDSIZE$1 <= 4ll) goto label$336;
			{
				FBSTRING TMP$696$4;
				FBSTRING TMP$697$4;
				__builtin_memset( &TMP$697$4, 0, 24ll );
				__builtin_memset( &TMP$696$4, 0, 24ll );
				FBSTRING* vr$55 = fb_StrConcat( &TMP$696$4, (void*)"movupd xmm7, ", 14ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$697$4, -1ll, (void*)vr$55, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$697$4 );
				fb_StrDelete( (FBSTRING*)&TMP$697$4 );
			}
			goto label$335;
			label$336:;
			{
				if( *(int64*)((uint8*)SVREG$1 + 40ll) != 2ll) goto label$338;
				{
					FBSTRING TMP$698$5;
					FBSTRING TMP$699$5;
					__builtin_memset( &TMP$699$5, 0, 24ll );
					__builtin_memset( &TMP$698$5, 0, 24ll );
					FBSTRING* vr$62 = fb_StrConcat( &TMP$698$5, (void*)"movlps xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$699$5, -1ll, (void*)vr$62, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$699$5 );
					fb_StrDelete( (FBSTRING*)&TMP$699$5 );
				}
				goto label$337;
				label$338:;
				{
					FBSTRING TMP$700$5;
					FBSTRING TMP$701$5;
					__builtin_memset( &TMP$701$5, 0, 24ll );
					__builtin_memset( &TMP$700$5, 0, 24ll );
					FBSTRING* vr$68 = fb_StrConcat( &TMP$700$5, (void*)"movups xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$701$5, -1ll, (void*)vr$68, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$701$5 );
					fb_StrDelete( (FBSTRING*)&TMP$701$5 );
				}
				label$337:;
			}
			label$335:;
			fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
		}
		label$334:;
		label$333:;
	}
	label$332:;
	label$331:;
	int64 vr$71 = HEMITCONVERTOPERANDS_SSE( DVREG$1, SVREG$1 );
	if( vr$71 == 0ll) goto label$340;
	{
		fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
	}
	label$340:;
	label$339:;
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$341;
	TMP$702$1 = 24ll;
	goto label$793;
	label$341:;
	TMP$702$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$793:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$702$1 * 56ll)) != 1ll) goto label$343;
	{
		if( DDSIZE$1 <= 4ll) goto label$345;
		{
			FBSTRING TMP$703$3;
			FBSTRING TMP$704$3;
			FBSTRING TMP$705$3;
			FBSTRING TMP$706$3;
			FBSTRING TMP$707$3;
			__builtin_memset( &TMP$707$3, 0, 24ll );
			__builtin_memset( &TMP$703$3, 0, 24ll );
			FBSTRING* vr$80 = fb_StrConcat( &TMP$703$3, (void*)&OSTR$1, -1ll, (void*)"d ", 3ll );
			__builtin_memset( &TMP$704$3, 0, 24ll );
			FBSTRING* vr$83 = fb_StrConcat( &TMP$704$3, (void*)vr$80, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$705$3, 0, 24ll );
			FBSTRING* vr$86 = fb_StrConcat( &TMP$705$3, (void*)vr$83, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$706$3, 0, 24ll );
			FBSTRING* vr$89 = fb_StrConcat( &TMP$706$3, (void*)vr$86, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$707$3, -1ll, (void*)vr$89, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$707$3 );
			fb_StrDelete( (FBSTRING*)&TMP$707$3 );
		}
		goto label$344;
		label$345:;
		{
			FBSTRING TMP$708$3;
			FBSTRING TMP$709$3;
			FBSTRING TMP$710$3;
			FBSTRING TMP$711$3;
			FBSTRING TMP$712$3;
			__builtin_memset( &TMP$712$3, 0, 24ll );
			__builtin_memset( &TMP$708$3, 0, 24ll );
			FBSTRING* vr$95 = fb_StrConcat( &TMP$708$3, (void*)&OSTR$1, -1ll, (void*)"s ", 3ll );
			__builtin_memset( &TMP$709$3, 0, 24ll );
			FBSTRING* vr$98 = fb_StrConcat( &TMP$709$3, (void*)vr$95, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$710$3, 0, 24ll );
			FBSTRING* vr$101 = fb_StrConcat( &TMP$710$3, (void*)vr$98, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$711$3, 0, 24ll );
			FBSTRING* vr$104 = fb_StrConcat( &TMP$711$3, (void*)vr$101, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$712$3, -1ll, (void*)vr$104, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$712$3 );
			fb_StrDelete( (FBSTRING*)&TMP$712$3 );
		}
		label$344:;
	}
	goto label$342;
	label$343:;
	{
		OUTP( (char*)" implement 'subtract integer from float'" );
	}
	label$342:;
	label$318:;
}

static void _EMITMULF_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$714$1;
	int64 TMP$715$1;
	int64 TMP$736$1;
	label$346:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static int64 RETURNSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$348;
	TMP$714$1 = 24ll;
	goto label$794;
	label$348:;
	TMP$714$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$794:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$714$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$349;
	TMP$715$1 = 24ll;
	goto label$795;
	label$349:;
	TMP$715$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$795:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$715$1 * 56ll)) + 8ll);
	RETURNSIZE$1 = 0ll;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$351;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		RETURNSIZE$1 = DDSIZE$1;
	}
	goto label$350;
	label$351:;
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$352;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		RETURNSIZE$1 = SDSIZE$1;
	}
	label$352:;
	label$350:;
	if( RETURNSIZE$1 == 0ll) goto label$354;
	{
		FBSTRING TMP$716$2;
		FBSTRING TMP$717$2;
		__builtin_memset( &TMP$717$2, 0, 24ll );
		FBSTRING* vr$13 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$716$2, 0, 24ll );
		FBSTRING* vr$16 = fb_StrConcat( &TMP$716$2, (void*)"sub esp, ", 10ll, (void*)vr$13, -1ll );
		fb_StrAssign( (void*)&TMP$717$2, -1ll, (void*)vr$16, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$717$2 );
		fb_StrDelete( (FBSTRING*)&TMP$717$2 );
	}
	label$354:;
	label$353:;
	if( RETURNSIZE$1 != 8ll) goto label$356;
	{
		FBSTRING TMP$718$2;
		FBSTRING TMP$719$2;
		FBSTRING TMP$720$2;
		FBSTRING TMP$721$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$721$2, 0, 24ll );
		__builtin_memset( &TMP$718$2, 0, 24ll );
		FBSTRING* vr$22 = fb_StrConcat( &TMP$718$2, (void*)"movlpd ", 8ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$719$2, 0, 24ll );
		FBSTRING* vr$25 = fb_StrConcat( &TMP$719$2, (void*)vr$22, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$720$2, 0, 24ll );
		FBSTRING* vr$28 = fb_StrConcat( &TMP$720$2, (void*)vr$25, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$721$2, -1ll, (void*)vr$28, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$721$2 );
		fb_StrDelete( (FBSTRING*)&TMP$721$2 );
	}
	goto label$355;
	label$356:;
	if( RETURNSIZE$1 != 4ll) goto label$357;
	{
		FBSTRING TMP$722$2;
		FBSTRING TMP$723$2;
		FBSTRING TMP$724$2;
		FBSTRING TMP$725$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$725$2, 0, 24ll );
		__builtin_memset( &TMP$722$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$722$2, (void*)"movss ", 7ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$723$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$723$2, (void*)vr$34, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$724$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$724$2, (void*)vr$37, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$725$2, -1ll, (void*)vr$40, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$725$2 );
		fb_StrDelete( (FBSTRING*)&TMP$725$2 );
	}
	label$357:;
	label$355:;
	if( RETURNSIZE$1 == 0ll) goto label$359;
	{
		FBSTRING TMP$726$2;
		FBSTRING TMP$727$2;
		__builtin_memset( &TMP$727$2, 0, 24ll );
		FBSTRING* vr$44 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$726$2, 0, 24ll );
		FBSTRING* vr$47 = fb_StrConcat( &TMP$726$2, (void*)"add esp, ", 10ll, (void*)vr$44, -1ll );
		fb_StrAssign( (void*)&TMP$727$2, -1ll, (void*)vr$47, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$727$2 );
		fb_StrDelete( (FBSTRING*)&TMP$727$2 );
	}
	label$359:;
	label$358:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"muls", 5ll, 0 );
	if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$361;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mulp", 5ll, 0 );
		if( *(int64*)SVREG$1 == 4ll) goto label$363;
		{
			HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
			if( SDSIZE$1 <= 4ll) goto label$365;
			{
				FBSTRING TMP$730$4;
				FBSTRING TMP$731$4;
				__builtin_memset( &TMP$731$4, 0, 24ll );
				__builtin_memset( &TMP$730$4, 0, 24ll );
				FBSTRING* vr$55 = fb_StrConcat( &TMP$730$4, (void*)"movupd xmm7, ", 14ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$731$4, -1ll, (void*)vr$55, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$731$4 );
				fb_StrDelete( (FBSTRING*)&TMP$731$4 );
			}
			goto label$364;
			label$365:;
			{
				if( *(int64*)((uint8*)SVREG$1 + 40ll) != 2ll) goto label$367;
				{
					FBSTRING TMP$732$5;
					FBSTRING TMP$733$5;
					__builtin_memset( &TMP$733$5, 0, 24ll );
					__builtin_memset( &TMP$732$5, 0, 24ll );
					FBSTRING* vr$62 = fb_StrConcat( &TMP$732$5, (void*)"movlps xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$733$5, -1ll, (void*)vr$62, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$733$5 );
					fb_StrDelete( (FBSTRING*)&TMP$733$5 );
				}
				goto label$366;
				label$367:;
				{
					FBSTRING TMP$734$5;
					FBSTRING TMP$735$5;
					__builtin_memset( &TMP$735$5, 0, 24ll );
					__builtin_memset( &TMP$734$5, 0, 24ll );
					FBSTRING* vr$68 = fb_StrConcat( &TMP$734$5, (void*)"movups xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$735$5, -1ll, (void*)vr$68, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$735$5 );
					fb_StrDelete( (FBSTRING*)&TMP$735$5 );
				}
				label$366:;
			}
			label$364:;
			fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
		}
		label$363:;
		label$362:;
	}
	label$361:;
	label$360:;
	int64 vr$71 = HEMITCONVERTOPERANDS_SSE( DVREG$1, SVREG$1 );
	if( vr$71 == 0ll) goto label$369;
	{
		fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
	}
	label$369:;
	label$368:;
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$370;
	TMP$736$1 = 24ll;
	goto label$796;
	label$370:;
	TMP$736$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$796:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$736$1 * 56ll)) != 1ll) goto label$372;
	{
		if( DDSIZE$1 <= 4ll) goto label$374;
		{
			FBSTRING TMP$737$3;
			FBSTRING TMP$738$3;
			FBSTRING TMP$739$3;
			FBSTRING TMP$740$3;
			FBSTRING TMP$741$3;
			__builtin_memset( &TMP$741$3, 0, 24ll );
			__builtin_memset( &TMP$737$3, 0, 24ll );
			FBSTRING* vr$80 = fb_StrConcat( &TMP$737$3, (void*)&OSTR$1, -1ll, (void*)"d ", 3ll );
			__builtin_memset( &TMP$738$3, 0, 24ll );
			FBSTRING* vr$83 = fb_StrConcat( &TMP$738$3, (void*)vr$80, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$739$3, 0, 24ll );
			FBSTRING* vr$86 = fb_StrConcat( &TMP$739$3, (void*)vr$83, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$740$3, 0, 24ll );
			FBSTRING* vr$89 = fb_StrConcat( &TMP$740$3, (void*)vr$86, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$741$3, -1ll, (void*)vr$89, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$741$3 );
			fb_StrDelete( (FBSTRING*)&TMP$741$3 );
		}
		goto label$373;
		label$374:;
		{
			FBSTRING TMP$742$3;
			FBSTRING TMP$743$3;
			FBSTRING TMP$744$3;
			FBSTRING TMP$745$3;
			FBSTRING TMP$746$3;
			__builtin_memset( &TMP$746$3, 0, 24ll );
			__builtin_memset( &TMP$742$3, 0, 24ll );
			FBSTRING* vr$95 = fb_StrConcat( &TMP$742$3, (void*)&OSTR$1, -1ll, (void*)"s ", 3ll );
			__builtin_memset( &TMP$743$3, 0, 24ll );
			FBSTRING* vr$98 = fb_StrConcat( &TMP$743$3, (void*)vr$95, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$744$3, 0, 24ll );
			FBSTRING* vr$101 = fb_StrConcat( &TMP$744$3, (void*)vr$98, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$745$3, 0, 24ll );
			FBSTRING* vr$104 = fb_StrConcat( &TMP$745$3, (void*)vr$101, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$746$3, -1ll, (void*)vr$104, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$746$3 );
			fb_StrDelete( (FBSTRING*)&TMP$746$3 );
		}
		label$373:;
	}
	goto label$371;
	label$372:;
	{
		OUTP( (char*)" implement 'multiply float by integer'" );
	}
	label$371:;
	label$347:;
}

static void _EMITDIVF_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$748$1;
	int64 TMP$749$1;
	int64 TMP$770$1;
	label$375:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static int64 RETURNSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$377;
	TMP$748$1 = 24ll;
	goto label$797;
	label$377:;
	TMP$748$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$797:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$748$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$378;
	TMP$749$1 = 24ll;
	goto label$798;
	label$378:;
	TMP$749$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$798:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$749$1 * 56ll)) + 8ll);
	RETURNSIZE$1 = 0ll;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$380;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		RETURNSIZE$1 = DDSIZE$1;
	}
	goto label$379;
	label$380:;
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$381;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		RETURNSIZE$1 = SDSIZE$1;
	}
	label$381:;
	label$379:;
	if( RETURNSIZE$1 == 0ll) goto label$383;
	{
		FBSTRING TMP$750$2;
		FBSTRING TMP$751$2;
		__builtin_memset( &TMP$751$2, 0, 24ll );
		FBSTRING* vr$13 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$750$2, 0, 24ll );
		FBSTRING* vr$16 = fb_StrConcat( &TMP$750$2, (void*)"sub esp, ", 10ll, (void*)vr$13, -1ll );
		fb_StrAssign( (void*)&TMP$751$2, -1ll, (void*)vr$16, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$751$2 );
		fb_StrDelete( (FBSTRING*)&TMP$751$2 );
	}
	label$383:;
	label$382:;
	if( RETURNSIZE$1 != 8ll) goto label$385;
	{
		FBSTRING TMP$752$2;
		FBSTRING TMP$753$2;
		FBSTRING TMP$754$2;
		FBSTRING TMP$755$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$755$2, 0, 24ll );
		__builtin_memset( &TMP$752$2, 0, 24ll );
		FBSTRING* vr$22 = fb_StrConcat( &TMP$752$2, (void*)"movlpd ", 8ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$753$2, 0, 24ll );
		FBSTRING* vr$25 = fb_StrConcat( &TMP$753$2, (void*)vr$22, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$754$2, 0, 24ll );
		FBSTRING* vr$28 = fb_StrConcat( &TMP$754$2, (void*)vr$25, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$755$2, -1ll, (void*)vr$28, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$755$2 );
		fb_StrDelete( (FBSTRING*)&TMP$755$2 );
	}
	goto label$384;
	label$385:;
	if( RETURNSIZE$1 != 4ll) goto label$386;
	{
		FBSTRING TMP$756$2;
		FBSTRING TMP$757$2;
		FBSTRING TMP$758$2;
		FBSTRING TMP$759$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$759$2, 0, 24ll );
		__builtin_memset( &TMP$756$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$756$2, (void*)"movss ", 7ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$757$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$757$2, (void*)vr$34, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$758$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$758$2, (void*)vr$37, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$759$2, -1ll, (void*)vr$40, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$759$2 );
		fb_StrDelete( (FBSTRING*)&TMP$759$2 );
	}
	label$386:;
	label$384:;
	if( RETURNSIZE$1 == 0ll) goto label$388;
	{
		FBSTRING TMP$760$2;
		FBSTRING TMP$761$2;
		__builtin_memset( &TMP$761$2, 0, 24ll );
		FBSTRING* vr$44 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$760$2, 0, 24ll );
		FBSTRING* vr$47 = fb_StrConcat( &TMP$760$2, (void*)"add esp, ", 10ll, (void*)vr$44, -1ll );
		fb_StrAssign( (void*)&TMP$761$2, -1ll, (void*)vr$47, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$761$2 );
		fb_StrDelete( (FBSTRING*)&TMP$761$2 );
	}
	label$388:;
	label$387:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"divs", 5ll, 0 );
	if( *(int64*)((uint8*)SVREG$1 + 40ll) == 0ll) goto label$390;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"divp", 5ll, 0 );
		if( *(int64*)SVREG$1 == 4ll) goto label$392;
		{
			HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
			if( SDSIZE$1 <= 4ll) goto label$394;
			{
				FBSTRING TMP$764$4;
				FBSTRING TMP$765$4;
				__builtin_memset( &TMP$765$4, 0, 24ll );
				__builtin_memset( &TMP$764$4, 0, 24ll );
				FBSTRING* vr$55 = fb_StrConcat( &TMP$764$4, (void*)"movupd xmm7, ", 14ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$765$4, -1ll, (void*)vr$55, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$765$4 );
				fb_StrDelete( (FBSTRING*)&TMP$765$4 );
			}
			goto label$393;
			label$394:;
			{
				if( *(int64*)((uint8*)SVREG$1 + 40ll) != 2ll) goto label$396;
				{
					FBSTRING TMP$766$5;
					FBSTRING TMP$767$5;
					__builtin_memset( &TMP$767$5, 0, 24ll );
					__builtin_memset( &TMP$766$5, 0, 24ll );
					FBSTRING* vr$62 = fb_StrConcat( &TMP$766$5, (void*)"movlps xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$767$5, -1ll, (void*)vr$62, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$767$5 );
					fb_StrDelete( (FBSTRING*)&TMP$767$5 );
				}
				goto label$395;
				label$396:;
				{
					FBSTRING TMP$768$5;
					FBSTRING TMP$769$5;
					__builtin_memset( &TMP$769$5, 0, 24ll );
					__builtin_memset( &TMP$768$5, 0, 24ll );
					FBSTRING* vr$68 = fb_StrConcat( &TMP$768$5, (void*)"movups xmm7, ", 14ll, (void*)&SRC$1, -1ll );
					fb_StrAssign( (void*)&TMP$769$5, -1ll, (void*)vr$68, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$769$5 );
					fb_StrDelete( (FBSTRING*)&TMP$769$5 );
				}
				label$395:;
			}
			label$393:;
			fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
		}
		label$392:;
		label$391:;
	}
	label$390:;
	label$389:;
	int64 vr$71 = HEMITCONVERTOPERANDS_SSE( DVREG$1, SVREG$1 );
	if( vr$71 == 0ll) goto label$398;
	{
		fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"xmm7", 5ll, 0 );
	}
	label$398:;
	label$397:;
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$399;
	TMP$770$1 = 24ll;
	goto label$799;
	label$399:;
	TMP$770$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$799:;
	if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$770$1 * 56ll)) != 1ll) goto label$401;
	{
		if( DDSIZE$1 <= 4ll) goto label$403;
		{
			FBSTRING TMP$771$3;
			FBSTRING TMP$772$3;
			FBSTRING TMP$773$3;
			FBSTRING TMP$774$3;
			FBSTRING TMP$775$3;
			__builtin_memset( &TMP$775$3, 0, 24ll );
			__builtin_memset( &TMP$771$3, 0, 24ll );
			FBSTRING* vr$80 = fb_StrConcat( &TMP$771$3, (void*)&OSTR$1, -1ll, (void*)"d ", 3ll );
			__builtin_memset( &TMP$772$3, 0, 24ll );
			FBSTRING* vr$83 = fb_StrConcat( &TMP$772$3, (void*)vr$80, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$773$3, 0, 24ll );
			FBSTRING* vr$86 = fb_StrConcat( &TMP$773$3, (void*)vr$83, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$774$3, 0, 24ll );
			FBSTRING* vr$89 = fb_StrConcat( &TMP$774$3, (void*)vr$86, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$775$3, -1ll, (void*)vr$89, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$775$3 );
			fb_StrDelete( (FBSTRING*)&TMP$775$3 );
		}
		goto label$402;
		label$403:;
		{
			FBSTRING TMP$776$3;
			FBSTRING TMP$777$3;
			FBSTRING TMP$778$3;
			FBSTRING TMP$779$3;
			FBSTRING TMP$780$3;
			__builtin_memset( &TMP$780$3, 0, 24ll );
			__builtin_memset( &TMP$776$3, 0, 24ll );
			FBSTRING* vr$95 = fb_StrConcat( &TMP$776$3, (void*)&OSTR$1, -1ll, (void*)"s ", 3ll );
			__builtin_memset( &TMP$777$3, 0, 24ll );
			FBSTRING* vr$98 = fb_StrConcat( &TMP$777$3, (void*)vr$95, -1ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$778$3, 0, 24ll );
			FBSTRING* vr$101 = fb_StrConcat( &TMP$778$3, (void*)vr$98, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$779$3, 0, 24ll );
			FBSTRING* vr$104 = fb_StrConcat( &TMP$779$3, (void*)vr$101, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$780$3, -1ll, (void*)vr$104, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$780$3 );
			fb_StrDelete( (FBSTRING*)&TMP$780$3 );
		}
		label$402:;
	}
	goto label$400;
	label$401:;
	{
		OUTP( (char*)" implement 'divide float by integer'" );
	}
	label$400:;
	label$376:;
}

static void _EMITATN2_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$782$1;
	int64 TMP$783$1;
	label$404:;
	static FBSTRING SRC$1;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static int64 ADJUSTSTACK$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$406;
	TMP$782$1 = 24ll;
	goto label$800;
	label$406:;
	TMP$782$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$800:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$782$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$407;
	TMP$783$1 = 24ll;
	goto label$801;
	label$407:;
	TMP$783$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$801:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$783$1 * 56ll)) + 8ll);
	ADJUSTSTACK$1 = 0ll;
	if( *(int64*)DVREG$1 != 4ll) goto label$409;
	{
		OUTP( (char*)"sub esp, 8" );
		ADJUSTSTACK$1 = -1ll;
		if( DDSIZE$1 <= 4ll) goto label$411;
		{
			FBSTRING TMP$784$3;
			FBSTRING TMP$785$3;
			__builtin_memset( &TMP$785$3, 0, 24ll );
			__builtin_memset( &TMP$784$3, 0, 24ll );
			FBSTRING* vr$14 = fb_StrConcat( &TMP$784$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$785$3, -1ll, (void*)vr$14, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$785$3 );
			fb_StrDelete( (FBSTRING*)&TMP$785$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$410;
		label$411:;
		{
			FBSTRING TMP$786$3;
			FBSTRING TMP$787$3;
			__builtin_memset( &TMP$787$3, 0, 24ll );
			__builtin_memset( &TMP$786$3, 0, 24ll );
			FBSTRING* vr$20 = fb_StrConcat( &TMP$786$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$787$3, -1ll, (void*)vr$20, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$787$3 );
			fb_StrDelete( (FBSTRING*)&TMP$787$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$410:;
	}
	goto label$408;
	label$409:;
	{
		FBSTRING TMP$788$2;
		FBSTRING TMP$789$2;
		__builtin_memset( &TMP$789$2, 0, 24ll );
		__builtin_memset( &TMP$788$2, 0, 24ll );
		FBSTRING* vr$26 = fb_StrConcat( &TMP$788$2, (void*)"fld ", 5ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$789$2, -1ll, (void*)vr$26, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$789$2 );
		fb_StrDelete( (FBSTRING*)&TMP$789$2 );
	}
	label$408:;
	if( *(int64*)SVREG$1 != 4ll) goto label$413;
	{
		if( ADJUSTSTACK$1 != 0ll) goto label$415;
		{
			OUTP( (char*)"sub esp, 8" );
			ADJUSTSTACK$1 = -1ll;
		}
		label$415:;
		label$414:;
		if( SDSIZE$1 <= 4ll) goto label$417;
		{
			FBSTRING TMP$790$3;
			FBSTRING TMP$791$3;
			__builtin_memset( &TMP$791$3, 0, 24ll );
			__builtin_memset( &TMP$790$3, 0, 24ll );
			FBSTRING* vr$33 = fb_StrConcat( &TMP$790$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$791$3, -1ll, (void*)vr$33, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$791$3 );
			fb_StrDelete( (FBSTRING*)&TMP$791$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$416;
		label$417:;
		{
			FBSTRING TMP$792$3;
			FBSTRING TMP$793$3;
			__builtin_memset( &TMP$793$3, 0, 24ll );
			__builtin_memset( &TMP$792$3, 0, 24ll );
			FBSTRING* vr$39 = fb_StrConcat( &TMP$792$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$793$3, -1ll, (void*)vr$39, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$793$3 );
			fb_StrDelete( (FBSTRING*)&TMP$793$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$416:;
	}
	goto label$412;
	label$413:;
	{
		FBSTRING TMP$794$2;
		FBSTRING TMP$795$2;
		__builtin_memset( &TMP$795$2, 0, 24ll );
		__builtin_memset( &TMP$794$2, 0, 24ll );
		FBSTRING* vr$45 = fb_StrConcat( &TMP$794$2, (void*)"fld ", 5ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$795$2, -1ll, (void*)vr$45, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$795$2 );
		fb_StrDelete( (FBSTRING*)&TMP$795$2 );
	}
	label$412:;
	OUTP( (char*)"fpatan" );
	if( DDSIZE$1 <= 4ll) goto label$419;
	{
		FBSTRING TMP$797$2;
		FBSTRING TMP$798$2;
		FBSTRING TMP$799$2;
		FBSTRING TMP$800$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$800$2, 0, 24ll );
		__builtin_memset( &TMP$797$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$797$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$798$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$798$2, (void*)vr$51, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$799$2, 0, 24ll );
		FBSTRING* vr$57 = fb_StrConcat( &TMP$799$2, (void*)vr$54, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$800$2, -1ll, (void*)vr$57, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$800$2 );
		fb_StrDelete( (FBSTRING*)&TMP$800$2 );
	}
	goto label$418;
	label$419:;
	{
		FBSTRING TMP$801$2;
		FBSTRING TMP$802$2;
		FBSTRING TMP$803$2;
		FBSTRING TMP$804$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$804$2, 0, 24ll );
		__builtin_memset( &TMP$801$2, 0, 24ll );
		FBSTRING* vr$63 = fb_StrConcat( &TMP$801$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$802$2, 0, 24ll );
		FBSTRING* vr$66 = fb_StrConcat( &TMP$802$2, (void*)vr$63, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$803$2, 0, 24ll );
		FBSTRING* vr$69 = fb_StrConcat( &TMP$803$2, (void*)vr$66, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$804$2, -1ll, (void*)vr$69, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$804$2 );
		fb_StrDelete( (FBSTRING*)&TMP$804$2 );
	}
	label$418:;
	if( ADJUSTSTACK$1 == 0ll) goto label$421;
	{
		OUTP( (char*)"add esp, 8" );
	}
	label$421:;
	label$420:;
	label$405:;
}

static void _EMITPOW_SSE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$422:;
	static FBSTRING SRC$1;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static int64 ADJUSTSTACK$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	ADJUSTSTACK$1 = 0ll;
	if( *(int64*)DVREG$1 != 4ll) goto label$425;
	{
		OUTP( (char*)"sub esp, 8" );
		ADJUSTSTACK$1 = -1ll;
		if( DDSIZE$1 <= 4ll) goto label$427;
		{
			FBSTRING TMP$805$3;
			FBSTRING TMP$806$3;
			__builtin_memset( &TMP$806$3, 0, 24ll );
			__builtin_memset( &TMP$805$3, 0, 24ll );
			FBSTRING* vr$4 = fb_StrConcat( &TMP$805$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$806$3, -1ll, (void*)vr$4, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$806$3 );
			fb_StrDelete( (FBSTRING*)&TMP$806$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$426;
		label$427:;
		{
			FBSTRING TMP$807$3;
			FBSTRING TMP$808$3;
			__builtin_memset( &TMP$808$3, 0, 24ll );
			__builtin_memset( &TMP$807$3, 0, 24ll );
			FBSTRING* vr$10 = fb_StrConcat( &TMP$807$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$808$3, -1ll, (void*)vr$10, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$808$3 );
			fb_StrDelete( (FBSTRING*)&TMP$808$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$426:;
	}
	goto label$424;
	label$425:;
	{
		FBSTRING TMP$809$2;
		FBSTRING TMP$810$2;
		__builtin_memset( &TMP$810$2, 0, 24ll );
		__builtin_memset( &TMP$809$2, 0, 24ll );
		FBSTRING* vr$16 = fb_StrConcat( &TMP$809$2, (void*)"fld ", 5ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$810$2, -1ll, (void*)vr$16, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$810$2 );
		fb_StrDelete( (FBSTRING*)&TMP$810$2 );
	}
	label$424:;
	if( *(int64*)SVREG$1 != 4ll) goto label$429;
	{
		if( ADJUSTSTACK$1 != 0ll) goto label$431;
		{
			OUTP( (char*)"sub esp, 8" );
			ADJUSTSTACK$1 = -1ll;
		}
		label$431:;
		label$430:;
		if( SDSIZE$1 <= 4ll) goto label$433;
		{
			FBSTRING TMP$811$3;
			FBSTRING TMP$812$3;
			__builtin_memset( &TMP$812$3, 0, 24ll );
			__builtin_memset( &TMP$811$3, 0, 24ll );
			FBSTRING* vr$23 = fb_StrConcat( &TMP$811$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$812$3, -1ll, (void*)vr$23, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$812$3 );
			fb_StrDelete( (FBSTRING*)&TMP$812$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$432;
		label$433:;
		{
			FBSTRING TMP$813$3;
			FBSTRING TMP$814$3;
			__builtin_memset( &TMP$814$3, 0, 24ll );
			__builtin_memset( &TMP$813$3, 0, 24ll );
			FBSTRING* vr$29 = fb_StrConcat( &TMP$813$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$814$3, -1ll, (void*)vr$29, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$814$3 );
			fb_StrDelete( (FBSTRING*)&TMP$814$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$432:;
	}
	goto label$428;
	label$429:;
	{
		FBSTRING TMP$815$2;
		FBSTRING TMP$816$2;
		__builtin_memset( &TMP$816$2, 0, 24ll );
		__builtin_memset( &TMP$815$2, 0, 24ll );
		FBSTRING* vr$35 = fb_StrConcat( &TMP$815$2, (void*)"fld ", 5ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$816$2, -1ll, (void*)vr$35, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$816$2 );
		fb_StrDelete( (FBSTRING*)&TMP$816$2 );
	}
	label$428:;
	OUTP( (char*)"fabs" );
	OUTP( (char*)"fyl2x" );
	OUTP( (char*)"fld st(0)" );
	OUTP( (char*)"frndint" );
	OUTP( (char*)"fsub st(1), st(0)" );
	OUTP( (char*)"fxch" );
	OUTP( (char*)"f2xm1" );
	OUTP( (char*)"fld1" );
	OUTP( (char*)"faddp" );
	OUTP( (char*)"fscale" );
	OUTP( (char*)"fstp st(1)" );
	if( DDSIZE$1 <= 4ll) goto label$435;
	{
		FBSTRING TMP$826$2;
		FBSTRING TMP$827$2;
		FBSTRING TMP$828$2;
		FBSTRING TMP$829$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$829$2, 0, 24ll );
		__builtin_memset( &TMP$826$2, 0, 24ll );
		FBSTRING* vr$41 = fb_StrConcat( &TMP$826$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$827$2, 0, 24ll );
		FBSTRING* vr$44 = fb_StrConcat( &TMP$827$2, (void*)vr$41, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$828$2, 0, 24ll );
		FBSTRING* vr$47 = fb_StrConcat( &TMP$828$2, (void*)vr$44, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$829$2, -1ll, (void*)vr$47, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$829$2 );
		fb_StrDelete( (FBSTRING*)&TMP$829$2 );
	}
	goto label$434;
	label$435:;
	{
		FBSTRING TMP$830$2;
		FBSTRING TMP$831$2;
		FBSTRING TMP$832$2;
		FBSTRING TMP$833$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$833$2, 0, 24ll );
		__builtin_memset( &TMP$830$2, 0, 24ll );
		FBSTRING* vr$53 = fb_StrConcat( &TMP$830$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$831$2, 0, 24ll );
		FBSTRING* vr$56 = fb_StrConcat( &TMP$831$2, (void*)vr$53, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$832$2, 0, 24ll );
		FBSTRING* vr$59 = fb_StrConcat( &TMP$832$2, (void*)vr$56, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$833$2, -1ll, (void*)vr$59, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$833$2 );
		fb_StrDelete( (FBSTRING*)&TMP$833$2 );
	}
	label$434:;
	if( ADJUSTSTACK$1 == 0ll) goto label$437;
	{
		OUTP( (char*)"add esp, 8" );
	}
	label$437:;
	label$436:;
	label$423:;
}

static struct $11CMPF_RECIPE* HCMPF_GET_RECIPE( $7CMPF_OP OP$1, $10IR_EMITOPT OPTIONS$1, struct $8FBSYMBOL* LABEL$1 )
{
	struct $11CMPF_RECIPE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$438:;
	static struct $11CMPF_RECIPE RECIPE$1[24] = { { 0ll, (char*)"e", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 1ll, (char*)"ne", (char*)"", (char*)"", (char*)"", -1ll, 0ll, 0ll, 0ll }, { 2ll, (char*)"a", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 3ll, (char*)"b", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 4ll, (char*)"ae", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 5ll, (char*)"be", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 0ll, (char*)"ne", (char*)"", (char*)"", (char*)"", -1ll, 0ll, 0ll, 0ll }, { 1ll, (char*)"e", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 2ll, (char*)"be", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 3ll, (char*)"ae", (char*)"", (char*)"", (char*)"", -1ll, 0ll, 0ll, 0ll }, { 4ll, (char*)"b", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 5ll, (char*)"a", (char*)"", (char*)"", (char*)"", -1ll, 0ll, 0ll, 0ll }, { 0ll, (char*)"e", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 1ll, (char*)"ne", (char*)"", (char*)"", (char*)"", -1ll, 0ll, 0ll, 0ll }, { 2ll, (char*)"a", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 3ll, (char*)"b", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 4ll, (char*)"ae", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 5ll, (char*)"be", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 0ll, (char*)"ne", (char*)"", (char*)"nz", (char*)"0b01000000", -1ll, 0ll, 0ll, 0ll }, { 1ll, (char*)"e", (char*)"", (char*)"z", (char*)"0b01000000", 0ll, -1ll, 0ll, 0ll }, { 2ll, (char*)"be", (char*)"", (char*)"z", (char*)"0b01000001", 0ll, 0ll, 0ll, 0ll }, { 3ll, (char*)"ae", (char*)"", (char*)"nz", (char*)"0b00000001", -1ll, 0ll, 0ll, 0ll }, { 4ll, (char*)"b", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 5ll, (char*)"a", (char*)"", (char*)"nz", (char*)"0b01000001", -1ll, 0ll, 0ll, 0ll } };
	static struct $8FBARRAY1I11CMPF_RECIPEE tmp$834$1 = { (struct $11CMPF_RECIPE*)RECIPE$1, (struct $11CMPF_RECIPE*)RECIPE$1, 1728ll, 72ll, 1ll, 49ll, { { 24ll, 0ll, 23ll } } };
	int64 INDEX$1;
	INDEX$1 = OP$1;
	if( LABEL$1 == (struct $8FBSYMBOL*)0ull) goto label$441;
	{
		INDEX$1 = INDEX$1 + 12ll;
	}
	label$441:;
	label$440:;
	if( (OPTIONS$1 & 1ll) == 0ll) goto label$443;
	{
		INDEX$1 = INDEX$1 + 6ll;
	}
	label$443:;
	label$442:;
	fb$result$1 = (struct $11CMPF_RECIPE*)((int64)(struct $11CMPF_RECIPE*)RECIPE$1 + (INDEX$1 * 72ll));
	goto label$439;
	label$439:;
	return fb$result$1;
}

static void HCMPF_SSE( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, struct $11CMPF_RECIPE* RECIPE$1 )
{
	int64 TMP$846$1;
	int64 TMP$847$1;
	label$444:;
	static FBSTRING RNAME$1;
	static FBSTRING RNAME8$1;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static FBSTRING LNAME$1;
	static FBSTRING ISNANLABEL$1;
	static int64 ISEAXFREE$1;
	static int64 ISEDXFREE$1;
	static int64 SDSIZE$1;
	static int64 DDSIZE$1;
	static int64 RETURNSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$446;
	TMP$846$1 = 24ll;
	goto label$802;
	label$446:;
	TMP$846$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$802:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$846$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$447;
	TMP$847$1 = 24ll;
	goto label$803;
	label$447:;
	TMP$847$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$803:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$847$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( LABEL$1 != (struct $8FBSYMBOL*)0ull) goto label$449;
	{
		char* vr$10 = SYMBUNIQUELABEL(  );
		fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$10, 0ll, 0 );
	}
	goto label$448;
	label$449:;
	{
		char* vr$11 = SYMBGETMANGLEDNAME( LABEL$1 );
		fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$11, 0ll, 0 );
	}
	label$448:;
	RETURNSIZE$1 = 0ll;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$451;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		RETURNSIZE$1 = DDSIZE$1;
	}
	goto label$450;
	label$451:;
	if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$452;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		RETURNSIZE$1 = SDSIZE$1;
	}
	label$452:;
	label$450:;
	if( RETURNSIZE$1 == 0ll) goto label$454;
	{
		FBSTRING TMP$848$2;
		FBSTRING TMP$849$2;
		__builtin_memset( &TMP$849$2, 0, 24ll );
		FBSTRING* vr$15 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$848$2, 0, 24ll );
		FBSTRING* vr$18 = fb_StrConcat( &TMP$848$2, (void*)"sub esp, ", 10ll, (void*)vr$15, -1ll );
		fb_StrAssign( (void*)&TMP$849$2, -1ll, (void*)vr$18, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$849$2 );
		fb_StrDelete( (FBSTRING*)&TMP$849$2 );
	}
	label$454:;
	label$453:;
	if( RETURNSIZE$1 != 8ll) goto label$456;
	{
		FBSTRING TMP$850$2;
		FBSTRING TMP$851$2;
		FBSTRING TMP$852$2;
		FBSTRING TMP$853$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$853$2, 0, 24ll );
		__builtin_memset( &TMP$850$2, 0, 24ll );
		FBSTRING* vr$24 = fb_StrConcat( &TMP$850$2, (void*)"movlpd ", 8ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$851$2, 0, 24ll );
		FBSTRING* vr$27 = fb_StrConcat( &TMP$851$2, (void*)vr$24, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$852$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$852$2, (void*)vr$27, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$853$2, -1ll, (void*)vr$30, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$853$2 );
		fb_StrDelete( (FBSTRING*)&TMP$853$2 );
	}
	goto label$455;
	label$456:;
	if( RETURNSIZE$1 != 4ll) goto label$457;
	{
		FBSTRING TMP$854$2;
		FBSTRING TMP$855$2;
		FBSTRING TMP$856$2;
		FBSTRING TMP$857$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$857$2, 0, 24ll );
		__builtin_memset( &TMP$854$2, 0, 24ll );
		FBSTRING* vr$36 = fb_StrConcat( &TMP$854$2, (void*)"movss ", 7ll, (void*)&OSTR$1, -1ll );
		__builtin_memset( &TMP$855$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$855$2, (void*)vr$36, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$856$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$856$2, (void*)vr$39, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$857$2, -1ll, (void*)vr$42, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$857$2 );
		fb_StrDelete( (FBSTRING*)&TMP$857$2 );
	}
	label$457:;
	label$455:;
	if( RETURNSIZE$1 == 0ll) goto label$459;
	{
		FBSTRING TMP$858$2;
		FBSTRING TMP$859$2;
		__builtin_memset( &TMP$859$2, 0, 24ll );
		FBSTRING* vr$46 = fb_LongintToStr( RETURNSIZE$1 );
		__builtin_memset( &TMP$858$2, 0, 24ll );
		FBSTRING* vr$49 = fb_StrConcat( &TMP$858$2, (void*)"add esp, ", 10ll, (void*)vr$46, -1ll );
		fb_StrAssign( (void*)&TMP$859$2, -1ll, (void*)vr$49, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$859$2 );
		fb_StrDelete( (FBSTRING*)&TMP$859$2 );
	}
	label$459:;
	label$458:;
	if( DDSIZE$1 <= 4ll) goto label$461;
	{
		if( SDSIZE$1 <= 4ll) goto label$463;
		{
			FBSTRING TMP$861$3;
			FBSTRING TMP$862$3;
			FBSTRING TMP$863$3;
			FBSTRING TMP$864$3;
			__builtin_memset( &TMP$864$3, 0, 24ll );
			__builtin_memset( &TMP$861$3, 0, 24ll );
			FBSTRING* vr$55 = fb_StrConcat( &TMP$861$3, (void*)"comisd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$862$3, 0, 24ll );
			FBSTRING* vr$58 = fb_StrConcat( &TMP$862$3, (void*)vr$55, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$863$3, 0, 24ll );
			FBSTRING* vr$61 = fb_StrConcat( &TMP$863$3, (void*)vr$58, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$864$3, -1ll, (void*)vr$61, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$864$3 );
			fb_StrDelete( (FBSTRING*)&TMP$864$3 );
		}
		goto label$462;
		label$463:;
		{
			FBSTRING TMP$865$3;
			FBSTRING TMP$866$3;
			FBSTRING TMP$867$3;
			FBSTRING TMP$868$3;
			FBSTRING TMP$869$3;
			FBSTRING TMP$870$3;
			__builtin_memset( &TMP$866$3, 0, 24ll );
			__builtin_memset( &TMP$865$3, 0, 24ll );
			FBSTRING* vr$67 = fb_StrConcat( &TMP$865$3, (void*)"cvtss2sd xmm7, ", 16ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$866$3, -1ll, (void*)vr$67, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$866$3 );
			fb_StrDelete( (FBSTRING*)&TMP$866$3 );
			__builtin_memset( &TMP$870$3, 0, 24ll );
			__builtin_memset( &TMP$867$3, 0, 24ll );
			FBSTRING* vr$73 = fb_StrConcat( &TMP$867$3, (void*)"comisd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$868$3, 0, 24ll );
			FBSTRING* vr$76 = fb_StrConcat( &TMP$868$3, (void*)vr$73, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$869$3, 0, 24ll );
			FBSTRING* vr$79 = fb_StrConcat( &TMP$869$3, (void*)vr$76, -1ll, (void*)"xmm7", 5ll );
			fb_StrAssign( (void*)&TMP$870$3, -1ll, (void*)vr$79, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$870$3 );
			fb_StrDelete( (FBSTRING*)&TMP$870$3 );
		}
		label$462:;
	}
	goto label$460;
	label$461:;
	{
		if( SDSIZE$1 <= 4ll) goto label$465;
		{
			FBSTRING TMP$871$3;
			FBSTRING TMP$872$3;
			FBSTRING TMP$875$3;
			FBSTRING TMP$876$3;
			__builtin_memset( &TMP$872$3, 0, 24ll );
			__builtin_memset( &TMP$871$3, 0, 24ll );
			FBSTRING* vr$85 = fb_StrConcat( &TMP$871$3, (void*)"cvtss2sd xmm7, ", 16ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$872$3, -1ll, (void*)vr$85, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$872$3 );
			fb_StrDelete( (FBSTRING*)&TMP$872$3 );
			__builtin_memset( &TMP$876$3, 0, 24ll );
			__builtin_memset( &TMP$875$3, 0, 24ll );
			FBSTRING* vr$91 = fb_StrConcat( &TMP$875$3, (void*)"comisd xmm7, ", 14ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$876$3, -1ll, (void*)vr$91, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$876$3 );
			fb_StrDelete( (FBSTRING*)&TMP$876$3 );
		}
		goto label$464;
		label$465:;
		{
			FBSTRING TMP$878$3;
			FBSTRING TMP$879$3;
			FBSTRING TMP$880$3;
			FBSTRING TMP$881$3;
			__builtin_memset( &TMP$881$3, 0, 24ll );
			__builtin_memset( &TMP$878$3, 0, 24ll );
			FBSTRING* vr$97 = fb_StrConcat( &TMP$878$3, (void*)"comiss ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$879$3, 0, 24ll );
			FBSTRING* vr$100 = fb_StrConcat( &TMP$879$3, (void*)vr$97, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$880$3, 0, 24ll );
			FBSTRING* vr$103 = fb_StrConcat( &TMP$880$3, (void*)vr$100, -1ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$881$3, -1ll, (void*)vr$103, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$881$3 );
			fb_StrDelete( (FBSTRING*)&TMP$881$3 );
		}
		label$464:;
	}
	label$460:;
	if( RVREG$1 != (struct $6IRVREG*)0ull) goto label$467;
	{
		HCMPF_JXX( RECIPE$1, &LNAME$1 );
		goto label$445;
	}
	label$467:;
	label$466:;
	HCMPF_SET( RVREG$1, RECIPE$1, &LNAME$1 );
	label$445:;
}

static void _EMITCGTF_SSE( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$468:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 2ll, OPTIONS$1, LABEL$1 );
	HCMPF_SSE( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$469:;
}

static void _EMITCLTF_SSE( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$470:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 3ll, OPTIONS$1, LABEL$1 );
	HCMPF_SSE( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$471:;
}

static void _EMITCEQF_SSE( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$472:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 0ll, OPTIONS$1, LABEL$1 );
	HCMPF_SSE( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$473:;
}

static void _EMITCNEF_SSE( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$474:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 1ll, OPTIONS$1, LABEL$1 );
	HCMPF_SSE( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$475:;
}

static void _EMITCLEF_SSE( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$476:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 5ll, OPTIONS$1, LABEL$1 );
	HCMPF_SSE( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$477:;
}

static void _EMITCGEF_SSE( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$478:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 4ll, OPTIONS$1, LABEL$1 );
	HCMPF_SSE( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$479:;
}

static void _EMITNEGF_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$883$1;
	label$480:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	static struct $8FBSYMBOL* SYM$1;
	static struct $6IRVREG* TEMPVREG$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$482;
	TMP$883$1 = 24ll;
	goto label$804;
	label$482:;
	TMP$883$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$804:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$883$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$484;
	{
		FBSTRING TMP$884$2;
		FBSTRING TMP$885$2;
		FBSTRING TMP$894$2;
		FBSTRING TMP$895$2;
		__builtin_memset( &TMP$885$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$884$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$884$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$885$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$885$2 );
		fb_StrDelete( (FBSTRING*)&TMP$885$2 );
		if( DDSIZE$1 <= 4ll) goto label$486;
		{
			FBSTRING TMP$886$3;
			FBSTRING TMP$887$3;
			FBSTRING TMP$888$3;
			FBSTRING TMP$889$3;
			OUTP( (char*)"fstp qword ptr [esp]" );
			__builtin_memset( &TMP$889$3, 0, 24ll );
			__builtin_memset( &TMP$886$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$886$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$887$3, 0, 24ll );
			FBSTRING* vr$19 = fb_StrConcat( &TMP$887$3, (void*)vr$16, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$888$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$888$3, (void*)vr$19, -1ll, (void*)"qword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$889$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$889$3 );
			fb_StrDelete( (FBSTRING*)&TMP$889$3 );
		}
		goto label$485;
		label$486:;
		{
			FBSTRING TMP$890$3;
			FBSTRING TMP$891$3;
			FBSTRING TMP$892$3;
			FBSTRING TMP$893$3;
			OUTP( (char*)"fstp dword ptr [esp]" );
			__builtin_memset( &TMP$893$3, 0, 24ll );
			__builtin_memset( &TMP$890$3, 0, 24ll );
			FBSTRING* vr$28 = fb_StrConcat( &TMP$890$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$891$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$891$3, (void*)vr$28, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$892$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$892$3, (void*)vr$31, -1ll, (void*)"dword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$893$3, -1ll, (void*)vr$34, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$893$3 );
			fb_StrDelete( (FBSTRING*)&TMP$893$3 );
		}
		label$485:;
		__builtin_memset( &TMP$895$2, 0, 24ll );
		FBSTRING* vr$38 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$894$2, 0, 24ll );
		FBSTRING* vr$41 = fb_StrConcat( &TMP$894$2, (void*)"add esp, ", 10ll, (void*)vr$38, -1ll );
		fb_StrAssign( (void*)&TMP$895$2, -1ll, (void*)vr$41, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$895$2 );
		fb_StrDelete( (FBSTRING*)&TMP$895$2 );
	}
	label$484:;
	label$483:;
	if( DDSIZE$1 <= 4ll) goto label$488;
	{
		struct $8FBSYMBOL* vr$44 = SYMBALLOCINTCONST( (int64)-9223372036854775808ull, 14ll );
		SYM$1 = vr$44;
		struct $6IRVREG* vr$46 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		TEMPVREG$1 = vr$46;
	}
	goto label$487;
	label$488:;
	{
		struct $8FBSYMBOL* vr$47 = SYMBALLOCINTCONST( 2147483648ll, 9ll );
		SYM$1 = vr$47;
		struct $6IRVREG* vr$49 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		TEMPVREG$1 = vr$49;
	}
	label$487:;
	*(int64*)((uint8*)SYM$1 + 168ll) = 16ll;
	HPREPOPERAND( TEMPVREG$1, &SRC$1, 25ll, 0ll, 0ll, -1ll );
	if( DDSIZE$1 <= 4ll) goto label$490;
	{
		FBSTRING TMP$897$2;
		FBSTRING TMP$898$2;
		FBSTRING TMP$899$2;
		FBSTRING TMP$900$2;
		__builtin_memset( &TMP$900$2, 0, 24ll );
		__builtin_memset( &TMP$897$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$897$2, (void*)"xorpd ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$898$2, 0, 24ll );
		FBSTRING* vr$57 = fb_StrConcat( &TMP$898$2, (void*)vr$54, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$899$2, 0, 24ll );
		FBSTRING* vr$60 = fb_StrConcat( &TMP$899$2, (void*)vr$57, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$900$2, -1ll, (void*)vr$60, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$900$2 );
		fb_StrDelete( (FBSTRING*)&TMP$900$2 );
	}
	goto label$489;
	label$490:;
	{
		FBSTRING TMP$902$2;
		FBSTRING TMP$903$2;
		FBSTRING TMP$904$2;
		FBSTRING TMP$905$2;
		__builtin_memset( &TMP$905$2, 0, 24ll );
		__builtin_memset( &TMP$902$2, 0, 24ll );
		FBSTRING* vr$66 = fb_StrConcat( &TMP$902$2, (void*)"xorps ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$903$2, 0, 24ll );
		FBSTRING* vr$69 = fb_StrConcat( &TMP$903$2, (void*)vr$66, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$904$2, 0, 24ll );
		FBSTRING* vr$72 = fb_StrConcat( &TMP$904$2, (void*)vr$69, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$905$2, -1ll, (void*)vr$72, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$905$2 );
		fb_StrDelete( (FBSTRING*)&TMP$905$2 );
	}
	label$489:;
	label$481:;
}

static void _EMITHADDF_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$906$1;
	label$491:;
	static FBSTRING DST$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$493;
	TMP$906$1 = 24ll;
	goto label$805;
	label$493:;
	TMP$906$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$805:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$906$1 * 56ll)) + 8ll) <= 4ll) goto label$495;
	{
		FBSTRING TMP$907$2;
		FBSTRING TMP$908$2;
		FBSTRING TMP$910$2;
		FBSTRING TMP$911$2;
		FBSTRING TMP$912$2;
		FBSTRING TMP$913$2;
		__builtin_memset( &TMP$908$2, 0, 24ll );
		__builtin_memset( &TMP$907$2, 0, 24ll );
		FBSTRING* vr$8 = fb_StrConcat( &TMP$907$2, (void*)"movhlps xmm7, ", 15ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$908$2, -1ll, (void*)vr$8, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$908$2 );
		fb_StrDelete( (FBSTRING*)&TMP$908$2 );
		__builtin_memset( &TMP$913$2, 0, 24ll );
		__builtin_memset( &TMP$910$2, 0, 24ll );
		FBSTRING* vr$14 = fb_StrConcat( &TMP$910$2, (void*)"addsd ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$911$2, 0, 24ll );
		FBSTRING* vr$17 = fb_StrConcat( &TMP$911$2, (void*)vr$14, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$912$2, 0, 24ll );
		FBSTRING* vr$20 = fb_StrConcat( &TMP$912$2, (void*)vr$17, -1ll, (void*)"xmm7", 5ll );
		fb_StrAssign( (void*)&TMP$913$2, -1ll, (void*)vr$20, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$913$2 );
		fb_StrDelete( (FBSTRING*)&TMP$913$2 );
	}
	goto label$494;
	label$495:;
	{
		if( *(int64*)((uint8*)DVREG$1 + 40ll) != 2ll) goto label$497;
		{
			FBSTRING TMP$917$3;
			FBSTRING TMP$918$3;
			FBSTRING TMP$919$3;
			FBSTRING TMP$920$3;
			FBSTRING TMP$922$3;
			FBSTRING TMP$923$3;
			FBSTRING TMP$924$3;
			FBSTRING TMP$925$3;
			__builtin_memset( &TMP$920$3, 0, 24ll );
			__builtin_memset( &TMP$917$3, 0, 24ll );
			FBSTRING* vr$27 = fb_StrConcat( &TMP$917$3, (void*)"pshufd xmm7, ", 14ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$918$3, 0, 24ll );
			FBSTRING* vr$30 = fb_StrConcat( &TMP$918$3, (void*)vr$27, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$919$3, 0, 24ll );
			FBSTRING* vr$33 = fb_StrConcat( &TMP$919$3, (void*)vr$30, -1ll, (void*)"0x01", 5ll );
			fb_StrAssign( (void*)&TMP$920$3, -1ll, (void*)vr$33, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$920$3 );
			fb_StrDelete( (FBSTRING*)&TMP$920$3 );
			__builtin_memset( &TMP$925$3, 0, 24ll );
			__builtin_memset( &TMP$922$3, 0, 24ll );
			FBSTRING* vr$39 = fb_StrConcat( &TMP$922$3, (void*)"addss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$923$3, 0, 24ll );
			FBSTRING* vr$42 = fb_StrConcat( &TMP$923$3, (void*)vr$39, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$924$3, 0, 24ll );
			FBSTRING* vr$45 = fb_StrConcat( &TMP$924$3, (void*)vr$42, -1ll, (void*)"xmm7", 5ll );
			fb_StrAssign( (void*)&TMP$925$3, -1ll, (void*)vr$45, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$925$3 );
			fb_StrDelete( (FBSTRING*)&TMP$925$3 );
		}
		goto label$496;
		label$497:;
		if( *(int64*)((uint8*)DVREG$1 + 40ll) != 3ll) goto label$498;
		{
			FBSTRING TMP$926$3;
			FBSTRING TMP$927$3;
			FBSTRING TMP$928$3;
			FBSTRING TMP$929$3;
			FBSTRING TMP$930$3;
			FBSTRING TMP$931$3;
			FBSTRING TMP$932$3;
			FBSTRING TMP$933$3;
			FBSTRING TMP$934$3;
			FBSTRING TMP$935$3;
			FBSTRING TMP$936$3;
			FBSTRING TMP$937$3;
			FBSTRING TMP$938$3;
			FBSTRING TMP$939$3;
			__builtin_memset( &TMP$929$3, 0, 24ll );
			__builtin_memset( &TMP$926$3, 0, 24ll );
			FBSTRING* vr$52 = fb_StrConcat( &TMP$926$3, (void*)"pshufd xmm7, ", 14ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$927$3, 0, 24ll );
			FBSTRING* vr$55 = fb_StrConcat( &TMP$927$3, (void*)vr$52, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$928$3, 0, 24ll );
			FBSTRING* vr$58 = fb_StrConcat( &TMP$928$3, (void*)vr$55, -1ll, (void*)"0x01", 5ll );
			fb_StrAssign( (void*)&TMP$929$3, -1ll, (void*)vr$58, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$929$3 );
			fb_StrDelete( (FBSTRING*)&TMP$929$3 );
			__builtin_memset( &TMP$933$3, 0, 24ll );
			__builtin_memset( &TMP$930$3, 0, 24ll );
			FBSTRING* vr$64 = fb_StrConcat( &TMP$930$3, (void*)"addss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$931$3, 0, 24ll );
			FBSTRING* vr$67 = fb_StrConcat( &TMP$931$3, (void*)vr$64, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$932$3, 0, 24ll );
			FBSTRING* vr$70 = fb_StrConcat( &TMP$932$3, (void*)vr$67, -1ll, (void*)"xmm7", 5ll );
			fb_StrAssign( (void*)&TMP$933$3, -1ll, (void*)vr$70, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$933$3 );
			fb_StrDelete( (FBSTRING*)&TMP$933$3 );
			__builtin_memset( &TMP$935$3, 0, 24ll );
			__builtin_memset( &TMP$934$3, 0, 24ll );
			FBSTRING* vr$76 = fb_StrConcat( &TMP$934$3, (void*)"movhlps xmm7, ", 15ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$935$3, -1ll, (void*)vr$76, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$935$3 );
			fb_StrDelete( (FBSTRING*)&TMP$935$3 );
			__builtin_memset( &TMP$939$3, 0, 24ll );
			__builtin_memset( &TMP$936$3, 0, 24ll );
			FBSTRING* vr$82 = fb_StrConcat( &TMP$936$3, (void*)"addss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$937$3, 0, 24ll );
			FBSTRING* vr$85 = fb_StrConcat( &TMP$937$3, (void*)vr$82, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$938$3, 0, 24ll );
			FBSTRING* vr$88 = fb_StrConcat( &TMP$938$3, (void*)vr$85, -1ll, (void*)"xmm7", 5ll );
			fb_StrAssign( (void*)&TMP$939$3, -1ll, (void*)vr$88, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$939$3 );
			fb_StrDelete( (FBSTRING*)&TMP$939$3 );
		}
		goto label$496;
		label$498:;
		if( *(int64*)((uint8*)DVREG$1 + 40ll) != 4ll) goto label$499;
		{
			FBSTRING TMP$940$3;
			FBSTRING TMP$941$3;
			FBSTRING TMP$943$3;
			FBSTRING TMP$944$3;
			FBSTRING TMP$945$3;
			FBSTRING TMP$946$3;
			FBSTRING TMP$947$3;
			FBSTRING TMP$948$3;
			FBSTRING TMP$949$3;
			FBSTRING TMP$950$3;
			FBSTRING TMP$951$3;
			FBSTRING TMP$952$3;
			FBSTRING TMP$953$3;
			FBSTRING TMP$954$3;
			__builtin_memset( &TMP$941$3, 0, 24ll );
			__builtin_memset( &TMP$940$3, 0, 24ll );
			FBSTRING* vr$95 = fb_StrConcat( &TMP$940$3, (void*)"movhlps xmm7, ", 15ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$941$3, -1ll, (void*)vr$95, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$941$3 );
			fb_StrDelete( (FBSTRING*)&TMP$941$3 );
			__builtin_memset( &TMP$946$3, 0, 24ll );
			__builtin_memset( &TMP$943$3, 0, 24ll );
			FBSTRING* vr$101 = fb_StrConcat( &TMP$943$3, (void*)"addps ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$944$3, 0, 24ll );
			FBSTRING* vr$104 = fb_StrConcat( &TMP$944$3, (void*)vr$101, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$945$3, 0, 24ll );
			FBSTRING* vr$107 = fb_StrConcat( &TMP$945$3, (void*)vr$104, -1ll, (void*)"xmm7", 5ll );
			fb_StrAssign( (void*)&TMP$946$3, -1ll, (void*)vr$107, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$946$3 );
			fb_StrDelete( (FBSTRING*)&TMP$946$3 );
			__builtin_memset( &TMP$950$3, 0, 24ll );
			__builtin_memset( &TMP$947$3, 0, 24ll );
			FBSTRING* vr$113 = fb_StrConcat( &TMP$947$3, (void*)"pshufd xmm7, ", 14ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$948$3, 0, 24ll );
			FBSTRING* vr$116 = fb_StrConcat( &TMP$948$3, (void*)vr$113, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$949$3, 0, 24ll );
			FBSTRING* vr$119 = fb_StrConcat( &TMP$949$3, (void*)vr$116, -1ll, (void*)"0x01", 5ll );
			fb_StrAssign( (void*)&TMP$950$3, -1ll, (void*)vr$119, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$950$3 );
			fb_StrDelete( (FBSTRING*)&TMP$950$3 );
			__builtin_memset( &TMP$954$3, 0, 24ll );
			__builtin_memset( &TMP$951$3, 0, 24ll );
			FBSTRING* vr$125 = fb_StrConcat( &TMP$951$3, (void*)"addss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$952$3, 0, 24ll );
			FBSTRING* vr$128 = fb_StrConcat( &TMP$952$3, (void*)vr$125, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$953$3, 0, 24ll );
			FBSTRING* vr$131 = fb_StrConcat( &TMP$953$3, (void*)vr$128, -1ll, (void*)"xmm7", 5ll );
			fb_StrAssign( (void*)&TMP$954$3, -1ll, (void*)vr$131, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$954$3 );
			fb_StrDelete( (FBSTRING*)&TMP$954$3 );
		}
		goto label$496;
		label$499:;
		{
		}
		label$496:;
	}
	label$494:;
	label$492:;
}

static void _EMITABSF_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$955$1;
	label$500:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	static struct $8FBSYMBOL* SYM$1;
	static struct $6IRVREG* TEMPVREG$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$502;
	TMP$955$1 = 24ll;
	goto label$806;
	label$502:;
	TMP$955$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$806:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$955$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$504;
	{
		FBSTRING TMP$956$2;
		FBSTRING TMP$957$2;
		FBSTRING TMP$966$2;
		FBSTRING TMP$967$2;
		__builtin_memset( &TMP$957$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$956$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$956$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$957$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$957$2 );
		fb_StrDelete( (FBSTRING*)&TMP$957$2 );
		if( DDSIZE$1 <= 4ll) goto label$506;
		{
			FBSTRING TMP$958$3;
			FBSTRING TMP$959$3;
			FBSTRING TMP$960$3;
			FBSTRING TMP$961$3;
			OUTP( (char*)"fstp qword ptr [esp]" );
			__builtin_memset( &TMP$961$3, 0, 24ll );
			__builtin_memset( &TMP$958$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$958$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$959$3, 0, 24ll );
			FBSTRING* vr$19 = fb_StrConcat( &TMP$959$3, (void*)vr$16, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$960$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$960$3, (void*)vr$19, -1ll, (void*)"qword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$961$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$961$3 );
			fb_StrDelete( (FBSTRING*)&TMP$961$3 );
		}
		goto label$505;
		label$506:;
		{
			FBSTRING TMP$962$3;
			FBSTRING TMP$963$3;
			FBSTRING TMP$964$3;
			FBSTRING TMP$965$3;
			OUTP( (char*)"fstp dword ptr [esp]" );
			__builtin_memset( &TMP$965$3, 0, 24ll );
			__builtin_memset( &TMP$962$3, 0, 24ll );
			FBSTRING* vr$28 = fb_StrConcat( &TMP$962$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$963$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$963$3, (void*)vr$28, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$964$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$964$3, (void*)vr$31, -1ll, (void*)"dword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$965$3, -1ll, (void*)vr$34, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$965$3 );
			fb_StrDelete( (FBSTRING*)&TMP$965$3 );
		}
		label$505:;
		__builtin_memset( &TMP$967$2, 0, 24ll );
		FBSTRING* vr$38 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$966$2, 0, 24ll );
		FBSTRING* vr$41 = fb_StrConcat( &TMP$966$2, (void*)"add esp, ", 10ll, (void*)vr$38, -1ll );
		fb_StrAssign( (void*)&TMP$967$2, -1ll, (void*)vr$41, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$967$2 );
		fb_StrDelete( (FBSTRING*)&TMP$967$2 );
	}
	label$504:;
	label$503:;
	if( DDSIZE$1 <= 4ll) goto label$508;
	{
		struct $8FBSYMBOL* vr$44 = SYMBALLOCINTCONST( 9223372036854775807ll, 14ll );
		SYM$1 = vr$44;
		struct $6IRVREG* vr$46 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		TEMPVREG$1 = vr$46;
	}
	goto label$507;
	label$508:;
	{
		struct $8FBSYMBOL* vr$47 = SYMBALLOCINTCONST( 2147483647ll, 9ll );
		SYM$1 = vr$47;
		struct $6IRVREG* vr$49 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		TEMPVREG$1 = vr$49;
	}
	label$507:;
	*(int64*)((uint8*)SYM$1 + 168ll) = 16ll;
	HPREPOPERAND( TEMPVREG$1, &SRC$1, 25ll, 0ll, 0ll, -1ll );
	if( DDSIZE$1 <= 4ll) goto label$510;
	{
		FBSTRING TMP$969$2;
		FBSTRING TMP$970$2;
		FBSTRING TMP$971$2;
		FBSTRING TMP$972$2;
		__builtin_memset( &TMP$972$2, 0, 24ll );
		__builtin_memset( &TMP$969$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$969$2, (void*)"andpd ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$970$2, 0, 24ll );
		FBSTRING* vr$57 = fb_StrConcat( &TMP$970$2, (void*)vr$54, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$971$2, 0, 24ll );
		FBSTRING* vr$60 = fb_StrConcat( &TMP$971$2, (void*)vr$57, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$972$2, -1ll, (void*)vr$60, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$972$2 );
		fb_StrDelete( (FBSTRING*)&TMP$972$2 );
	}
	goto label$509;
	label$510:;
	{
		FBSTRING TMP$974$2;
		FBSTRING TMP$975$2;
		FBSTRING TMP$976$2;
		FBSTRING TMP$977$2;
		__builtin_memset( &TMP$977$2, 0, 24ll );
		__builtin_memset( &TMP$974$2, 0, 24ll );
		FBSTRING* vr$66 = fb_StrConcat( &TMP$974$2, (void*)"andps ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$975$2, 0, 24ll );
		FBSTRING* vr$69 = fb_StrConcat( &TMP$975$2, (void*)vr$66, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$976$2, 0, 24ll );
		FBSTRING* vr$72 = fb_StrConcat( &TMP$976$2, (void*)vr$69, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$977$2, -1ll, (void*)vr$72, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$977$2 );
		fb_StrDelete( (FBSTRING*)&TMP$977$2 );
	}
	label$509:;
	label$501:;
}

static void _EMITSGNF_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$978$1;
	label$511:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static struct $8FBSYMBOL* SYM$1;
	static struct $6IRVREG* TEMPVREG$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$513;
	TMP$978$1 = 24ll;
	goto label$807;
	label$513:;
	TMP$978$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$807:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$978$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$515;
	{
		FBSTRING TMP$979$2;
		FBSTRING TMP$980$2;
		FBSTRING TMP$989$2;
		FBSTRING TMP$990$2;
		__builtin_memset( &TMP$980$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$979$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$979$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$980$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$980$2 );
		fb_StrDelete( (FBSTRING*)&TMP$980$2 );
		if( DDSIZE$1 <= 4ll) goto label$517;
		{
			FBSTRING TMP$981$3;
			FBSTRING TMP$982$3;
			FBSTRING TMP$983$3;
			FBSTRING TMP$984$3;
			OUTP( (char*)"fstp qword ptr [esp]" );
			__builtin_memset( &TMP$984$3, 0, 24ll );
			__builtin_memset( &TMP$981$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$981$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$982$3, 0, 24ll );
			FBSTRING* vr$19 = fb_StrConcat( &TMP$982$3, (void*)vr$16, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$983$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$983$3, (void*)vr$19, -1ll, (void*)"qword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$984$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$984$3 );
			fb_StrDelete( (FBSTRING*)&TMP$984$3 );
		}
		goto label$516;
		label$517:;
		{
			FBSTRING TMP$985$3;
			FBSTRING TMP$986$3;
			FBSTRING TMP$987$3;
			FBSTRING TMP$988$3;
			OUTP( (char*)"fstp dword ptr [esp]" );
			__builtin_memset( &TMP$988$3, 0, 24ll );
			__builtin_memset( &TMP$985$3, 0, 24ll );
			FBSTRING* vr$28 = fb_StrConcat( &TMP$985$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$986$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$986$3, (void*)vr$28, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$987$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$987$3, (void*)vr$31, -1ll, (void*)"dword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$988$3, -1ll, (void*)vr$34, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$988$3 );
			fb_StrDelete( (FBSTRING*)&TMP$988$3 );
		}
		label$516:;
		__builtin_memset( &TMP$990$2, 0, 24ll );
		FBSTRING* vr$38 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$989$2, 0, 24ll );
		FBSTRING* vr$41 = fb_StrConcat( &TMP$989$2, (void*)"add esp, ", 10ll, (void*)vr$38, -1ll );
		fb_StrAssign( (void*)&TMP$990$2, -1ll, (void*)vr$41, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$990$2 );
		fb_StrDelete( (FBSTRING*)&TMP$990$2 );
	}
	label$515:;
	label$514:;
	if( DDSIZE$1 <= 4ll) goto label$519;
	{
		FBSTRING TMP$994$2;
		FBSTRING TMP$995$2;
		FBSTRING TMP$997$2;
		FBSTRING TMP$998$2;
		FBSTRING TMP$999$2;
		FBSTRING TMP$1000$2;
		FBSTRING TMP$1003$2;
		FBSTRING TMP$1004$2;
		FBSTRING TMP$1005$2;
		FBSTRING TMP$1006$2;
		FBSTRING TMP$1007$2;
		FBSTRING TMP$1008$2;
		OUTP( (char*)"xorpd xmm7, xmm7" );
		__builtin_memset( &TMP$995$2, 0, 24ll );
		__builtin_memset( &TMP$994$2, 0, 24ll );
		FBSTRING* vr$47 = fb_StrConcat( &TMP$994$2, (void*)"cmpneqsd xmm7, ", 16ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$995$2, -1ll, (void*)vr$47, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$995$2 );
		fb_StrDelete( (FBSTRING*)&TMP$995$2 );
		struct $8FBSYMBOL* vr$50 = SYMBALLOCINTCONST( 9223372036854775807ll, 14ll );
		SYM$1 = vr$50;
		*(int64*)((uint8*)SYM$1 + 168ll) = 16ll;
		struct $6IRVREG* vr$53 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		TEMPVREG$1 = vr$53;
		HPREPOPERAND( TEMPVREG$1, &SRC$1, 25ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$1000$2, 0, 24ll );
		__builtin_memset( &TMP$997$2, 0, 24ll );
		FBSTRING* vr$57 = fb_StrConcat( &TMP$997$2, (void*)"orpd ", 6ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$998$2, 0, 24ll );
		FBSTRING* vr$60 = fb_StrConcat( &TMP$998$2, (void*)vr$57, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$999$2, 0, 24ll );
		FBSTRING* vr$63 = fb_StrConcat( &TMP$999$2, (void*)vr$60, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$1000$2, -1ll, (void*)vr$63, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1000$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1000$2 );
		struct $8FBSYMBOL* vr$66 = SYMBALLOCINTCONST( -4616189618054758400ll, 14ll );
		SYM$1 = vr$66;
		*(int64*)((uint8*)SYM$1 + 168ll) = 16ll;
		struct $6IRVREG* vr$69 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		TEMPVREG$1 = vr$69;
		HPREPOPERAND( TEMPVREG$1, &SRC$1, 25ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$1004$2, 0, 24ll );
		__builtin_memset( &TMP$1003$2, 0, 24ll );
		FBSTRING* vr$73 = fb_StrConcat( &TMP$1003$2, (void*)"andpd xmm7, ", 13ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$1004$2, -1ll, (void*)vr$73, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1004$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1004$2 );
		__builtin_memset( &TMP$1008$2, 0, 24ll );
		__builtin_memset( &TMP$1005$2, 0, 24ll );
		FBSTRING* vr$79 = fb_StrConcat( &TMP$1005$2, (void*)"andpd ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1006$2, 0, 24ll );
		FBSTRING* vr$82 = fb_StrConcat( &TMP$1006$2, (void*)vr$79, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1007$2, 0, 24ll );
		FBSTRING* vr$85 = fb_StrConcat( &TMP$1007$2, (void*)vr$82, -1ll, (void*)"xmm7", 5ll );
		fb_StrAssign( (void*)&TMP$1008$2, -1ll, (void*)vr$85, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1008$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1008$2 );
	}
	goto label$518;
	label$519:;
	{
		FBSTRING TMP$1012$2;
		FBSTRING TMP$1013$2;
		FBSTRING TMP$1015$2;
		FBSTRING TMP$1016$2;
		FBSTRING TMP$1017$2;
		FBSTRING TMP$1018$2;
		FBSTRING TMP$1021$2;
		FBSTRING TMP$1022$2;
		FBSTRING TMP$1023$2;
		FBSTRING TMP$1024$2;
		FBSTRING TMP$1025$2;
		FBSTRING TMP$1026$2;
		OUTP( (char*)"xorps xmm7, xmm7" );
		__builtin_memset( &TMP$1013$2, 0, 24ll );
		__builtin_memset( &TMP$1012$2, 0, 24ll );
		FBSTRING* vr$91 = fb_StrConcat( &TMP$1012$2, (void*)"cmpneqss xmm7, ", 16ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1013$2, -1ll, (void*)vr$91, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1013$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1013$2 );
		struct $8FBSYMBOL* vr$94 = SYMBALLOCINTCONST( 2147483647ll, 9ll );
		SYM$1 = vr$94;
		*(int64*)((uint8*)SYM$1 + 168ll) = 16ll;
		struct $6IRVREG* vr$97 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		TEMPVREG$1 = vr$97;
		HPREPOPERAND( TEMPVREG$1, &SRC$1, 25ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$1018$2, 0, 24ll );
		__builtin_memset( &TMP$1015$2, 0, 24ll );
		FBSTRING* vr$101 = fb_StrConcat( &TMP$1015$2, (void*)"orps ", 6ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1016$2, 0, 24ll );
		FBSTRING* vr$104 = fb_StrConcat( &TMP$1016$2, (void*)vr$101, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1017$2, 0, 24ll );
		FBSTRING* vr$107 = fb_StrConcat( &TMP$1017$2, (void*)vr$104, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$1018$2, -1ll, (void*)vr$107, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1018$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1018$2 );
		struct $8FBSYMBOL* vr$110 = SYMBALLOCINTCONST( 3212836864ll, 9ll );
		SYM$1 = vr$110;
		*(int64*)((uint8*)SYM$1 + 168ll) = 16ll;
		struct $6IRVREG* vr$113 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		TEMPVREG$1 = vr$113;
		HPREPOPERAND( TEMPVREG$1, &SRC$1, 25ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$1022$2, 0, 24ll );
		__builtin_memset( &TMP$1021$2, 0, 24ll );
		FBSTRING* vr$117 = fb_StrConcat( &TMP$1021$2, (void*)"andps xmm7, ", 13ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$1022$2, -1ll, (void*)vr$117, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1022$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1022$2 );
		__builtin_memset( &TMP$1026$2, 0, 24ll );
		__builtin_memset( &TMP$1023$2, 0, 24ll );
		FBSTRING* vr$123 = fb_StrConcat( &TMP$1023$2, (void*)"andps ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1024$2, 0, 24ll );
		FBSTRING* vr$126 = fb_StrConcat( &TMP$1024$2, (void*)vr$123, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1025$2, 0, 24ll );
		FBSTRING* vr$129 = fb_StrConcat( &TMP$1025$2, (void*)vr$126, -1ll, (void*)"xmm7", 5ll );
		fb_StrAssign( (void*)&TMP$1026$2, -1ll, (void*)vr$129, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1026$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1026$2 );
	}
	label$518:;
	label$512:;
}

static void HSINCOS_FAST_SSE( struct $6IRVREG* DVREG$1, int64 ISCOS$1 )
{
	FBSTRING TMP$1030$1;
	FBSTRING TMP$1031$1;
	FBSTRING TMP$1057$1;
	FBSTRING TMP$1058$1;
	FBSTRING TMP$1059$1;
	FBSTRING TMP$1060$1;
	FBSTRING TMP$1070$1;
	FBSTRING TMP$1071$1;
	FBSTRING TMP$1072$1;
	FBSTRING TMP$1073$1;
	FBSTRING TMP$1074$1;
	FBSTRING TMP$1075$1;
	FBSTRING TMP$1077$1;
	FBSTRING TMP$1078$1;
	FBSTRING TMP$1079$1;
	FBSTRING TMP$1080$1;
	FBSTRING TMP$1082$1;
	FBSTRING TMP$1083$1;
	FBSTRING TMP$1084$1;
	FBSTRING TMP$1085$1;
	FBSTRING TMP$1088$1;
	FBSTRING TMP$1089$1;
	FBSTRING TMP$1090$1;
	FBSTRING TMP$1091$1;
	FBSTRING TMP$1093$1;
	FBSTRING TMP$1094$1;
	FBSTRING TMP$1096$1;
	FBSTRING TMP$1097$1;
	FBSTRING TMP$1098$1;
	FBSTRING TMP$1099$1;
	FBSTRING TMP$1101$1;
	FBSTRING TMP$1102$1;
	FBSTRING TMP$1103$1;
	FBSTRING TMP$1104$1;
	FBSTRING TMP$1106$1;
	FBSTRING TMP$1107$1;
	FBSTRING TMP$1108$1;
	FBSTRING TMP$1109$1;
	FBSTRING TMP$1111$1;
	FBSTRING TMP$1112$1;
	FBSTRING TMP$1114$1;
	FBSTRING TMP$1115$1;
	FBSTRING TMP$1116$1;
	FBSTRING TMP$1117$1;
	FBSTRING TMP$1119$1;
	FBSTRING TMP$1120$1;
	FBSTRING TMP$1121$1;
	FBSTRING TMP$1122$1;
	FBSTRING TMP$1125$1;
	FBSTRING TMP$1126$1;
	FBSTRING TMP$1127$1;
	FBSTRING TMP$1128$1;
	FBSTRING TMP$1130$1;
	FBSTRING TMP$1131$1;
	FBSTRING TMP$1132$1;
	FBSTRING TMP$1133$1;
	FBSTRING TMP$1136$1;
	FBSTRING TMP$1137$1;
	FBSTRING TMP$1144$1;
	FBSTRING TMP$1145$1;
	FBSTRING TMP$1146$1;
	FBSTRING TMP$1147$1;
	FBSTRING TMP$1150$1;
	FBSTRING TMP$1151$1;
	FBSTRING TMP$1152$1;
	FBSTRING TMP$1153$1;
	FBSTRING TMP$1154$1;
	FBSTRING TMP$1155$1;
	FBSTRING TMP$1156$1;
	FBSTRING TMP$1157$1;
	FBSTRING TMP$1160$1;
	FBSTRING TMP$1161$1;
	FBSTRING TMP$1164$1;
	FBSTRING TMP$1165$1;
	FBSTRING TMP$1166$1;
	FBSTRING TMP$1167$1;
	FBSTRING TMP$1168$1;
	FBSTRING TMP$1169$1;
	FBSTRING TMP$1170$1;
	FBSTRING TMP$1171$1;
	FBSTRING TMP$1172$1;
	FBSTRING TMP$1173$1;
	FBSTRING TMP$1174$1;
	FBSTRING TMP$1175$1;
	FBSTRING TMP$1176$1;
	FBSTRING TMP$1177$1;
	FBSTRING TMP$1178$1;
	FBSTRING TMP$1179$1;
	FBSTRING TMP$1180$1;
	FBSTRING TMP$1181$1;
	FBSTRING TMP$1195$1;
	FBSTRING TMP$1196$1;
	label$520:;
	static int64 REG$1[3];
	static struct $8FBARRAY1IlE tmp$1027$1 = { (int64*)REG$1, (int64*)REG$1, 24ll, 8ll, 1ll, 49ll, { { 3ll, 0ll, 2ll } } };
	static int64 ISFREE$1[3];
	static struct $8FBARRAY1IlE tmp$1028$1 = { (int64*)ISFREE$1, (int64*)ISFREE$1, 24ll, 8ll, 1ll, 49ll, { { 3ll, 0ll, 2ll } } };
	static int64 STACKSIZE$1;
	static int64 I$1;
	static int64 STACKPOINTER$1;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING REGNAME$1[3];
	static struct $8FBARRAY1I8FBSTRINGE tmp$1029$1 = { (FBSTRING*)REGNAME$1, (FBSTRING*)REGNAME$1, 72ll, 24ll, 1ll, 49ll, { { 3ll, 0ll, 2ll } } };
	static struct $8FBSYMBOL* SYM_INVSIGNBITMASK$1;
	static struct $8FBSYMBOL* SYM_ONE$1;
	static struct $8FBSYMBOL* SYM_PIOVERTWO$1;
	static struct $8FBSYMBOL* SYM_TWOOVERPI$1;
	static struct $8FBSYMBOL* SYM_SIN_C0$1;
	static struct $8FBSYMBOL* SYM_SIN_C1$1;
	static struct $8FBSYMBOL* SYM_SIN_C2$1;
	static struct $8FBSYMBOL* SYM_SIN_C3$1;
	static struct $6IRVREG* VREG_INVSIGNBITMASK$1;
	static struct $6IRVREG* VREG_ONE$1;
	static struct $6IRVREG* VREG_PIOVERTWO$1;
	static struct $6IRVREG* VREG_TWOOVERPI$1;
	static struct $6IRVREG* VREG_SIN_C0$1;
	static struct $6IRVREG* VREG_SIN_C1$1;
	static struct $6IRVREG* VREG_SIN_C2$1;
	static struct $6IRVREG* VREG_SIN_C3$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	STACKSIZE$1 = 4ll;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$523;
	{
		STACKSIZE$1 = STACKSIZE$1 + 4ll;
	}
	label$523:;
	label$522:;
	*(int64*)REG$1 = 3ll;
	*(int64*)ISFREE$1 = 0ll;
	*(int64*)((int64)(int64*)REG$1 + 8ll) = 5ll;
	*(int64*)((int64)(int64*)ISFREE$1 + 8ll) = 0ll;
	int64 vr$2 = HISREGFREE( 0ll, 3ll );
	if( vr$2 == 0ll) goto label$525;
	{
		*(int64*)REG$1 = 3ll;
		*(int64*)ISFREE$1 = -1ll;
		int64 vr$3 = HISREGFREE( 0ll, 0ll );
		if( vr$3 == 0ll) goto label$527;
		{
			*(int64*)((int64)(int64*)REG$1 + 8ll) = 0ll;
			*(int64*)((int64)(int64*)ISFREE$1 + 8ll) = -1ll;
		}
		goto label$526;
		label$527:;
		int64 vr$4 = HISREGFREE( 0ll, 5ll );
		if( vr$4 == 0ll) goto label$528;
		{
			*(int64*)((int64)(int64*)REG$1 + 8ll) = 5ll;
			*(int64*)((int64)(int64*)ISFREE$1 + 8ll) = -1ll;
		}
		label$528:;
		label$526:;
	}
	goto label$524;
	label$525:;
	int64 vr$5 = HISREGFREE( 0ll, 0ll );
	if( vr$5 == 0ll) goto label$529;
	{
		*(int64*)REG$1 = 0ll;
		*(int64*)ISFREE$1 = -1ll;
		int64 vr$6 = HISREGFREE( 0ll, 5ll );
		if( vr$6 == 0ll) goto label$531;
		{
			*(int64*)((int64)(int64*)REG$1 + 8ll) = 5ll;
			*(int64*)((int64)(int64*)ISFREE$1 + 8ll) = -1ll;
		}
		label$531:;
		label$530:;
	}
	goto label$524;
	label$529:;
	{
		int64 vr$7 = HISREGFREE( 0ll, 5ll );
		if( vr$7 == 0ll) goto label$533;
		{
			*(int64*)((int64)(int64*)REG$1 + 8ll) = 5ll;
			*(int64*)((int64)(int64*)ISFREE$1 + 8ll) = -1ll;
		}
		label$533:;
		label$532:;
	}
	label$524:;
	int64 vr$8 = HFINDFREEREG( 1ll );
	*(int64*)((int64)(int64*)REG$1 + 16ll) = vr$8;
	if( *(int64*)((int64)(int64*)REG$1 + 16ll) != -1ll) goto label$535;
	{
		*(int64*)((int64)(int64*)REG$1 + 16ll) = 0ll;
		*(int64*)((int64)(int64*)ISFREE$1 + 16ll) = 0ll;
	}
	goto label$534;
	label$535:;
	{
		*(int64*)((int64)(int64*)ISFREE$1 + 16ll) = -1ll;
	}
	label$534:;
	STACKSIZE$1 = STACKSIZE$1 + ((*(int64*)ISFREE$1 & 1ll) << (2ll & 63ll));
	STACKSIZE$1 = STACKSIZE$1 + ((*(int64*)((int64)(int64*)ISFREE$1 + 8ll) & 1ll) << (2ll & 63ll));
	STACKSIZE$1 = STACKSIZE$1 + ((*(int64*)((int64)(int64*)ISFREE$1 + 16ll) & 1ll) << (2ll & 63ll));
	char* vr$18 = HGETREGNAME( 8ll, *(int64*)REG$1 );
	fb_StrAssign( (void*)REGNAME$1, -1ll, (void*)vr$18, 0ll, 0 );
	char* vr$19 = HGETREGNAME( 8ll, *(int64*)((int64)(int64*)REG$1 + 8ll) );
	fb_StrAssign( (void*)((uint8*)REGNAME$1 + 24ll), -1ll, (void*)vr$19, 0ll, 0 );
	char* vr$20 = HGETREGNAME( 15ll, *(int64*)((int64)(int64*)REG$1 + 16ll) );
	fb_StrAssign( (void*)((uint8*)REGNAME$1 + 48ll), -1ll, (void*)vr$20, 0ll, 0 );
	struct $8FBSYMBOL* vr$21 = SYMBALLOCINTCONST( 2147483647ll, 9ll );
	SYM_INVSIGNBITMASK$1 = vr$21;
	*(int64*)((uint8*)SYM_INVSIGNBITMASK$1 + 168ll) = 16ll;
	struct $6IRVREG* vr$24 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM_INVSIGNBITMASK$1, *(int64*)((uint8*)SYM_INVSIGNBITMASK$1 + 88ll) );
	VREG_INVSIGNBITMASK$1 = vr$24;
	struct $8FBSYMBOL* vr$25 = SYMBALLOCINTCONST( 1070141403ll, 9ll );
	SYM_PIOVERTWO$1 = vr$25;
	*(int64*)((uint8*)SYM_PIOVERTWO$1 + 168ll) = 16ll;
	struct $6IRVREG* vr$28 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM_PIOVERTWO$1, *(int64*)((uint8*)SYM_PIOVERTWO$1 + 88ll) );
	VREG_PIOVERTWO$1 = vr$28;
	struct $8FBSYMBOL* vr$29 = SYMBALLOCINTCONST( 1059256707ll, 9ll );
	SYM_TWOOVERPI$1 = vr$29;
	*(int64*)((uint8*)SYM_TWOOVERPI$1 + 168ll) = 16ll;
	struct $6IRVREG* vr$32 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM_TWOOVERPI$1, *(int64*)((uint8*)SYM_TWOOVERPI$1 + 88ll) );
	VREG_TWOOVERPI$1 = vr$32;
	struct $8FBSYMBOL* vr$33 = SYMBALLOCINTCONST( 1065353216ll, 9ll );
	SYM_ONE$1 = vr$33;
	*(int64*)((uint8*)SYM_ONE$1 + 168ll) = 16ll;
	struct $6IRVREG* vr$36 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM_ONE$1, *(int64*)((uint8*)SYM_ONE$1 + 88ll) );
	VREG_ONE$1 = vr$36;
	struct $8FBSYMBOL* vr$37 = SYMBALLOCINTCONST( 1070141403ll, 9ll );
	SYM_SIN_C0$1 = vr$37;
	*(int64*)((uint8*)SYM_SIN_C0$1 + 168ll) = 16ll;
	struct $6IRVREG* vr$40 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM_SIN_C0$1, *(int64*)((uint8*)SYM_SIN_C0$1 + 88ll) );
	VREG_SIN_C0$1 = vr$40;
	struct $8FBSYMBOL* vr$41 = SYMBALLOCINTCONST( 3206897127ll, 9ll );
	SYM_SIN_C1$1 = vr$41;
	*(int64*)((uint8*)SYM_SIN_C1$1 + 168ll) = 16ll;
	struct $6IRVREG* vr$44 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM_SIN_C1$1, *(int64*)((uint8*)SYM_SIN_C1$1 + 88ll) );
	VREG_SIN_C1$1 = vr$44;
	struct $8FBSYMBOL* vr$45 = SYMBALLOCINTCONST( 1034106339ll, 9ll );
	SYM_SIN_C2$1 = vr$45;
	*(int64*)((uint8*)SYM_SIN_C2$1 + 168ll) = 16ll;
	struct $6IRVREG* vr$48 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM_SIN_C2$1, *(int64*)((uint8*)SYM_SIN_C2$1 + 88ll) );
	VREG_SIN_C2$1 = vr$48;
	struct $8FBSYMBOL* vr$49 = SYMBALLOCINTCONST( 3147393382ll, 9ll );
	SYM_SIN_C3$1 = vr$49;
	*(int64*)((uint8*)SYM_SIN_C3$1 + 168ll) = 16ll;
	struct $6IRVREG* vr$52 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM_SIN_C3$1, *(int64*)((uint8*)SYM_SIN_C3$1 + 88ll) );
	VREG_SIN_C3$1 = vr$52;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1031$1, 0, 24ll );
	FBSTRING* vr$54 = fb_LongintToStr( STACKSIZE$1 );
	__builtin_memset( &TMP$1030$1, 0, 24ll );
	FBSTRING* vr$57 = fb_StrConcat( &TMP$1030$1, (void*)"sub esp, ", 10ll, (void*)vr$54, -1ll );
	fb_StrAssign( (void*)&TMP$1031$1, -1ll, (void*)vr$57, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1031$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1031$1 );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$537;
	{
		FBSTRING TMP$1032$2;
		FBSTRING TMP$1033$2;
		FBSTRING TMP$1034$2;
		FBSTRING TMP$1035$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1035$2, 0, 24ll );
		__builtin_memset( &TMP$1032$2, 0, 24ll );
		FBSTRING* vr$64 = fb_StrConcat( &TMP$1032$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1033$2, 0, 24ll );
		FBSTRING* vr$67 = fb_StrConcat( &TMP$1033$2, (void*)vr$64, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1034$2, 0, 24ll );
		FBSTRING* vr$70 = fb_StrConcat( &TMP$1034$2, (void*)vr$67, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1035$2, -1ll, (void*)vr$70, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1035$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1035$2 );
	}
	label$537:;
	label$536:;
	STACKPOINTER$1 = 4ll;
	{
		I$1 = 0ll;
		label$541:;
		{
			if( *(int64*)((int64)(int64*)ISFREE$1 + (I$1 << (3ll & 63ll))) != 0ll) goto label$543;
			{
				if( I$1 >= 2ll) goto label$545;
				{
					FBSTRING TMP$1038$5;
					FBSTRING TMP$1039$5;
					FBSTRING TMP$1040$5;
					FBSTRING TMP$1041$5;
					FBSTRING TMP$1042$5;
					__builtin_memset( &TMP$1042$5, 0, 24ll );
					FBSTRING* vr$77 = fb_LongintToStr( STACKPOINTER$1 );
					__builtin_memset( &TMP$1038$5, 0, 24ll );
					FBSTRING* vr$80 = fb_StrConcat( &TMP$1038$5, (void*)"mov [esp+", 10ll, (void*)vr$77, -1ll );
					__builtin_memset( &TMP$1039$5, 0, 24ll );
					FBSTRING* vr$83 = fb_StrConcat( &TMP$1039$5, (void*)vr$80, -1ll, (void*)"]", 2ll );
					__builtin_memset( &TMP$1040$5, 0, 24ll );
					FBSTRING* vr$86 = fb_StrConcat( &TMP$1040$5, (void*)vr$83, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$1041$5, 0, 24ll );
					FBSTRING* vr$89 = fb_StrConcat( &TMP$1041$5, (void*)vr$86, -1ll, (void*)((int64)(FBSTRING*)REGNAME$1 + (I$1 * 24ll)), -1ll );
					fb_StrAssign( (void*)&TMP$1042$5, -1ll, (void*)vr$89, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$1042$5 );
					fb_StrDelete( (FBSTRING*)&TMP$1042$5 );
				}
				goto label$544;
				label$545:;
				{
					FBSTRING TMP$1044$5;
					FBSTRING TMP$1045$5;
					FBSTRING TMP$1046$5;
					FBSTRING TMP$1047$5;
					FBSTRING TMP$1048$5;
					__builtin_memset( &TMP$1048$5, 0, 24ll );
					FBSTRING* vr$95 = fb_LongintToStr( STACKPOINTER$1 );
					__builtin_memset( &TMP$1044$5, 0, 24ll );
					FBSTRING* vr$98 = fb_StrConcat( &TMP$1044$5, (void*)"movss [esp+", 12ll, (void*)vr$95, -1ll );
					__builtin_memset( &TMP$1045$5, 0, 24ll );
					FBSTRING* vr$101 = fb_StrConcat( &TMP$1045$5, (void*)vr$98, -1ll, (void*)"]", 2ll );
					__builtin_memset( &TMP$1046$5, 0, 24ll );
					FBSTRING* vr$104 = fb_StrConcat( &TMP$1046$5, (void*)vr$101, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$1047$5, 0, 24ll );
					FBSTRING* vr$107 = fb_StrConcat( &TMP$1047$5, (void*)vr$104, -1ll, (void*)((int64)(FBSTRING*)REGNAME$1 + (I$1 * 24ll)), -1ll );
					fb_StrAssign( (void*)&TMP$1048$5, -1ll, (void*)vr$107, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$1048$5 );
					fb_StrDelete( (FBSTRING*)&TMP$1048$5 );
				}
				label$544:;
				STACKPOINTER$1 = STACKPOINTER$1 + 4ll;
			}
			label$543:;
			label$542:;
		}
		label$539:;
		I$1 = I$1 + 1ll;
		label$538:;
		if( I$1 <= 2ll) goto label$541;
		label$540:;
	}
	if( ISCOS$1 != 0ll) goto label$547;
	{
		FBSTRING TMP$1049$2;
		FBSTRING TMP$1050$2;
		FBSTRING TMP$1052$2;
		FBSTRING TMP$1053$2;
		FBSTRING TMP$1054$2;
		FBSTRING TMP$1055$2;
		__builtin_memset( &TMP$1050$2, 0, 24ll );
		__builtin_memset( &TMP$1049$2, 0, 24ll );
		FBSTRING* vr$115 = fb_StrConcat( &TMP$1049$2, (void*)"movss [esp], ", 14ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1050$2, -1ll, (void*)vr$115, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1050$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1050$2 );
		HPREPOPERAND( VREG_TWOOVERPI$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$1055$2, 0, 24ll );
		__builtin_memset( &TMP$1052$2, 0, 24ll );
		FBSTRING* vr$121 = fb_StrConcat( &TMP$1052$2, (void*)"mulss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1053$2, 0, 24ll );
		FBSTRING* vr$124 = fb_StrConcat( &TMP$1053$2, (void*)vr$121, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1054$2, 0, 24ll );
		FBSTRING* vr$127 = fb_StrConcat( &TMP$1054$2, (void*)vr$124, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$1055$2, -1ll, (void*)vr$127, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1055$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1055$2 );
		OUTP( (char*)"and       dword ptr [esp], 0x80000000" );
	}
	label$547:;
	label$546:;
	HPREPOPERAND( VREG_INVSIGNBITMASK$1, &SRC$1, 25ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1060$1, 0, 24ll );
	__builtin_memset( &TMP$1057$1, 0, 24ll );
	FBSTRING* vr$133 = fb_StrConcat( &TMP$1057$1, (void*)"andps ", 7ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1058$1, 0, 24ll );
	FBSTRING* vr$136 = fb_StrConcat( &TMP$1058$1, (void*)vr$133, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1059$1, 0, 24ll );
	FBSTRING* vr$139 = fb_StrConcat( &TMP$1059$1, (void*)vr$136, -1ll, (void*)&SRC$1, -1ll );
	fb_StrAssign( (void*)&TMP$1060$1, -1ll, (void*)vr$139, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1060$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1060$1 );
	if( ISCOS$1 != -1ll) goto label$549;
	{
		FBSTRING TMP$1061$2;
		FBSTRING TMP$1062$2;
		FBSTRING TMP$1063$2;
		FBSTRING TMP$1064$2;
		FBSTRING TMP$1065$2;
		FBSTRING TMP$1066$2;
		FBSTRING TMP$1067$2;
		FBSTRING TMP$1068$2;
		HPREPOPERAND( VREG_PIOVERTWO$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$1064$2, 0, 24ll );
		__builtin_memset( &TMP$1061$2, 0, 24ll );
		FBSTRING* vr$145 = fb_StrConcat( &TMP$1061$2, (void*)"addss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1062$2, 0, 24ll );
		FBSTRING* vr$148 = fb_StrConcat( &TMP$1062$2, (void*)vr$145, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1063$2, 0, 24ll );
		FBSTRING* vr$151 = fb_StrConcat( &TMP$1063$2, (void*)vr$148, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$1064$2, -1ll, (void*)vr$151, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1064$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1064$2 );
		HPREPOPERAND( VREG_TWOOVERPI$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$1068$2, 0, 24ll );
		__builtin_memset( &TMP$1065$2, 0, 24ll );
		FBSTRING* vr$157 = fb_StrConcat( &TMP$1065$2, (void*)"mulss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1066$2, 0, 24ll );
		FBSTRING* vr$160 = fb_StrConcat( &TMP$1066$2, (void*)vr$157, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1067$2, 0, 24ll );
		FBSTRING* vr$163 = fb_StrConcat( &TMP$1067$2, (void*)vr$160, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$1068$2, -1ll, (void*)vr$163, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1068$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1068$2 );
	}
	label$549:;
	label$548:;
	__builtin_memset( &TMP$1073$1, 0, 24ll );
	__builtin_memset( &TMP$1070$1, 0, 24ll );
	FBSTRING* vr$169 = fb_StrConcat( &TMP$1070$1, (void*)"cvttss2si ", 11ll, (void*)REGNAME$1, -1ll );
	__builtin_memset( &TMP$1071$1, 0, 24ll );
	FBSTRING* vr$172 = fb_StrConcat( &TMP$1071$1, (void*)vr$169, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1072$1, 0, 24ll );
	FBSTRING* vr$175 = fb_StrConcat( &TMP$1072$1, (void*)vr$172, -1ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$1073$1, -1ll, (void*)vr$175, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1073$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1073$1 );
	HPREPOPERAND( VREG_ONE$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1075$1, 0, 24ll );
	__builtin_memset( &TMP$1074$1, 0, 24ll );
	FBSTRING* vr$181 = fb_StrConcat( &TMP$1074$1, (void*)"movss xmm7, ", 13ll, (void*)&SRC$1, -1ll );
	fb_StrAssign( (void*)&TMP$1075$1, -1ll, (void*)vr$181, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1075$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1075$1 );
	__builtin_memset( &TMP$1080$1, 0, 24ll );
	__builtin_memset( &TMP$1077$1, 0, 24ll );
	FBSTRING* vr$187 = fb_StrConcat( &TMP$1077$1, (void*)"mov       ", 11ll, (void*)((uint8*)REGNAME$1 + 24ll), -1ll );
	__builtin_memset( &TMP$1078$1, 0, 24ll );
	FBSTRING* vr$190 = fb_StrConcat( &TMP$1078$1, (void*)vr$187, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1079$1, 0, 24ll );
	FBSTRING* vr$193 = fb_StrConcat( &TMP$1079$1, (void*)vr$190, -1ll, (void*)REGNAME$1, -1ll );
	fb_StrAssign( (void*)&TMP$1080$1, -1ll, (void*)vr$193, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1080$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1080$1 );
	__builtin_memset( &TMP$1085$1, 0, 24ll );
	__builtin_memset( &TMP$1082$1, 0, 24ll );
	FBSTRING* vr$199 = fb_StrConcat( &TMP$1082$1, (void*)"cvtsi2ss  ", 11ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	__builtin_memset( &TMP$1083$1, 0, 24ll );
	FBSTRING* vr$202 = fb_StrConcat( &TMP$1083$1, (void*)vr$199, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1084$1, 0, 24ll );
	FBSTRING* vr$205 = fb_StrConcat( &TMP$1084$1, (void*)vr$202, -1ll, (void*)REGNAME$1, -1ll );
	fb_StrAssign( (void*)&TMP$1085$1, -1ll, (void*)vr$205, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1085$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1085$1 );
	__builtin_memset( &TMP$1091$1, 0, 24ll );
	__builtin_memset( &TMP$1088$1, 0, 24ll );
	FBSTRING* vr$211 = fb_StrConcat( &TMP$1088$1, (void*)"shl       ", 11ll, (void*)((uint8*)REGNAME$1 + 24ll), -1ll );
	__builtin_memset( &TMP$1089$1, 0, 24ll );
	FBSTRING* vr$214 = fb_StrConcat( &TMP$1089$1, (void*)vr$211, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1090$1, 0, 24ll );
	FBSTRING* vr$217 = fb_StrConcat( &TMP$1090$1, (void*)vr$214, -1ll, (void*)"30", 3ll );
	fb_StrAssign( (void*)&TMP$1091$1, -1ll, (void*)vr$217, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1091$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1091$1 );
	__builtin_memset( &TMP$1094$1, 0, 24ll );
	__builtin_memset( &TMP$1093$1, 0, 24ll );
	FBSTRING* vr$223 = fb_StrConcat( &TMP$1093$1, (void*)"not       ", 11ll, (void*)REGNAME$1, -1ll );
	fb_StrAssign( (void*)&TMP$1094$1, -1ll, (void*)vr$223, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1094$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1094$1 );
	__builtin_memset( &TMP$1099$1, 0, 24ll );
	__builtin_memset( &TMP$1096$1, 0, 24ll );
	FBSTRING* vr$229 = fb_StrConcat( &TMP$1096$1, (void*)"and       ", 11ll, (void*)((uint8*)REGNAME$1 + 24ll), -1ll );
	__builtin_memset( &TMP$1097$1, 0, 24ll );
	FBSTRING* vr$232 = fb_StrConcat( &TMP$1097$1, (void*)vr$229, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1098$1, 0, 24ll );
	FBSTRING* vr$235 = fb_StrConcat( &TMP$1098$1, (void*)vr$232, -1ll, (void*)"0x80000000", 11ll );
	fb_StrAssign( (void*)&TMP$1099$1, -1ll, (void*)vr$235, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1099$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1099$1 );
	__builtin_memset( &TMP$1104$1, 0, 24ll );
	__builtin_memset( &TMP$1101$1, 0, 24ll );
	FBSTRING* vr$241 = fb_StrConcat( &TMP$1101$1, (void*)"and       ", 11ll, (void*)REGNAME$1, -1ll );
	__builtin_memset( &TMP$1102$1, 0, 24ll );
	FBSTRING* vr$244 = fb_StrConcat( &TMP$1102$1, (void*)vr$241, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1103$1, 0, 24ll );
	FBSTRING* vr$247 = fb_StrConcat( &TMP$1103$1, (void*)vr$244, -1ll, (void*)"0x1", 4ll );
	fb_StrAssign( (void*)&TMP$1104$1, -1ll, (void*)vr$247, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1104$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1104$1 );
	__builtin_memset( &TMP$1109$1, 0, 24ll );
	__builtin_memset( &TMP$1106$1, 0, 24ll );
	FBSTRING* vr$253 = fb_StrConcat( &TMP$1106$1, (void*)"subss ", 7ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1107$1, 0, 24ll );
	FBSTRING* vr$256 = fb_StrConcat( &TMP$1107$1, (void*)vr$253, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1108$1, 0, 24ll );
	FBSTRING* vr$259 = fb_StrConcat( &TMP$1108$1, (void*)vr$256, -1ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	fb_StrAssign( (void*)&TMP$1109$1, -1ll, (void*)vr$259, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1109$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1109$1 );
	__builtin_memset( &TMP$1112$1, 0, 24ll );
	__builtin_memset( &TMP$1111$1, 0, 24ll );
	FBSTRING* vr$265 = fb_StrConcat( &TMP$1111$1, (void*)"dec       ", 11ll, (void*)REGNAME$1, -1ll );
	fb_StrAssign( (void*)&TMP$1112$1, -1ll, (void*)vr$265, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1112$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1112$1 );
	__builtin_memset( &TMP$1117$1, 0, 24ll );
	__builtin_memset( &TMP$1114$1, 0, 24ll );
	FBSTRING* vr$271 = fb_StrConcat( &TMP$1114$1, (void*)"minss ", 7ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1115$1, 0, 24ll );
	FBSTRING* vr$274 = fb_StrConcat( &TMP$1115$1, (void*)vr$271, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1116$1, 0, 24ll );
	FBSTRING* vr$277 = fb_StrConcat( &TMP$1116$1, (void*)vr$274, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1117$1, -1ll, (void*)vr$277, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1117$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1117$1 );
	__builtin_memset( &TMP$1122$1, 0, 24ll );
	__builtin_memset( &TMP$1119$1, 0, 24ll );
	FBSTRING* vr$283 = fb_StrConcat( &TMP$1119$1, (void*)"movd      ", 11ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	__builtin_memset( &TMP$1120$1, 0, 24ll );
	FBSTRING* vr$286 = fb_StrConcat( &TMP$1120$1, (void*)vr$283, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1121$1, 0, 24ll );
	FBSTRING* vr$289 = fb_StrConcat( &TMP$1121$1, (void*)vr$286, -1ll, (void*)REGNAME$1, -1ll );
	fb_StrAssign( (void*)&TMP$1122$1, -1ll, (void*)vr$289, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1122$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1122$1 );
	__builtin_memset( &TMP$1126$1, 0, 24ll );
	__builtin_memset( &TMP$1125$1, 0, 24ll );
	FBSTRING* vr$295 = fb_StrConcat( &TMP$1125$1, (void*)"subss xmm7, ", 13ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$1126$1, -1ll, (void*)vr$295, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1126$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1126$1 );
	__builtin_memset( &TMP$1128$1, 0, 24ll );
	__builtin_memset( &TMP$1127$1, 0, 24ll );
	FBSTRING* vr$301 = fb_StrConcat( &TMP$1127$1, (void*)"andps xmm7, ", 13ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	fb_StrAssign( (void*)&TMP$1128$1, -1ll, (void*)vr$301, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1128$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1128$1 );
	__builtin_memset( &TMP$1133$1, 0, 24ll );
	__builtin_memset( &TMP$1130$1, 0, 24ll );
	FBSTRING* vr$307 = fb_StrConcat( &TMP$1130$1, (void*)"andnps    ", 11ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	__builtin_memset( &TMP$1131$1, 0, 24ll );
	FBSTRING* vr$310 = fb_StrConcat( &TMP$1131$1, (void*)vr$307, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1132$1, 0, 24ll );
	FBSTRING* vr$313 = fb_StrConcat( &TMP$1132$1, (void*)vr$310, -1ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$1133$1, -1ll, (void*)vr$313, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1133$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1133$1 );
	__builtin_memset( &TMP$1137$1, 0, 24ll );
	__builtin_memset( &TMP$1136$1, 0, 24ll );
	FBSTRING* vr$319 = fb_StrConcat( &TMP$1136$1, (void*)"orps      xmm7, ", 17ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	fb_StrAssign( (void*)&TMP$1137$1, -1ll, (void*)vr$319, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1137$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1137$1 );
	if( ISCOS$1 != 0ll) goto label$551;
	{
		FBSTRING TMP$1140$2;
		FBSTRING TMP$1141$2;
		FBSTRING TMP$1142$2;
		FBSTRING TMP$1143$2;
		__builtin_memset( &TMP$1143$2, 0, 24ll );
		__builtin_memset( &TMP$1140$2, 0, 24ll );
		FBSTRING* vr$325 = fb_StrConcat( &TMP$1140$2, (void*)"xor       ", 11ll, (void*)((uint8*)REGNAME$1 + 24ll), -1ll );
		__builtin_memset( &TMP$1141$2, 0, 24ll );
		FBSTRING* vr$328 = fb_StrConcat( &TMP$1141$2, (void*)vr$325, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1142$2, 0, 24ll );
		FBSTRING* vr$331 = fb_StrConcat( &TMP$1142$2, (void*)vr$328, -1ll, (void*)"[esp]", 6ll );
		fb_StrAssign( (void*)&TMP$1143$2, -1ll, (void*)vr$331, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1143$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1143$2 );
	}
	label$551:;
	label$550:;
	__builtin_memset( &TMP$1147$1, 0, 24ll );
	__builtin_memset( &TMP$1144$1, 0, 24ll );
	FBSTRING* vr$337 = fb_StrConcat( &TMP$1144$1, (void*)"movd      ", 11ll, (void*)REGNAME$1, -1ll );
	__builtin_memset( &TMP$1145$1, 0, 24ll );
	FBSTRING* vr$340 = fb_StrConcat( &TMP$1145$1, (void*)vr$337, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1146$1, 0, 24ll );
	FBSTRING* vr$343 = fb_StrConcat( &TMP$1146$1, (void*)vr$340, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1147$1, -1ll, (void*)vr$343, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1147$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1147$1 );
	OUTP( (char*)"mulss xmm7, xmm7" );
	__builtin_memset( &TMP$1153$1, 0, 24ll );
	__builtin_memset( &TMP$1150$1, 0, 24ll );
	FBSTRING* vr$349 = fb_StrConcat( &TMP$1150$1, (void*)"or        ", 11ll, (void*)((uint8*)REGNAME$1 + 24ll), -1ll );
	__builtin_memset( &TMP$1151$1, 0, 24ll );
	FBSTRING* vr$352 = fb_StrConcat( &TMP$1151$1, (void*)vr$349, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1152$1, 0, 24ll );
	FBSTRING* vr$355 = fb_StrConcat( &TMP$1152$1, (void*)vr$352, -1ll, (void*)REGNAME$1, -1ll );
	fb_StrAssign( (void*)&TMP$1153$1, -1ll, (void*)vr$355, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1153$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1153$1 );
	__builtin_memset( &TMP$1157$1, 0, 24ll );
	__builtin_memset( &TMP$1154$1, 0, 24ll );
	FBSTRING* vr$361 = fb_StrConcat( &TMP$1154$1, (void*)"movss ", 7ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	__builtin_memset( &TMP$1155$1, 0, 24ll );
	FBSTRING* vr$364 = fb_StrConcat( &TMP$1155$1, (void*)vr$361, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1156$1, 0, 24ll );
	FBSTRING* vr$367 = fb_StrConcat( &TMP$1156$1, (void*)vr$364, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1157$1, -1ll, (void*)vr$367, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1157$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1157$1 );
	HPREPOPERAND( VREG_SIN_C3$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1161$1, 0, 24ll );
	__builtin_memset( &TMP$1160$1, 0, 24ll );
	FBSTRING* vr$373 = fb_StrConcat( &TMP$1160$1, (void*)"mulss xmm7, ", 13ll, (void*)&SRC$1, -1ll );
	fb_StrAssign( (void*)&TMP$1161$1, -1ll, (void*)vr$373, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1161$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1161$1 );
	HPREPOPERAND( VREG_SIN_C2$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1165$1, 0, 24ll );
	__builtin_memset( &TMP$1164$1, 0, 24ll );
	FBSTRING* vr$379 = fb_StrConcat( &TMP$1164$1, (void*)"addss xmm7, ", 13ll, (void*)&SRC$1, -1ll );
	fb_StrAssign( (void*)&TMP$1165$1, -1ll, (void*)vr$379, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1165$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1165$1 );
	__builtin_memset( &TMP$1167$1, 0, 24ll );
	__builtin_memset( &TMP$1166$1, 0, 24ll );
	FBSTRING* vr$385 = fb_StrConcat( &TMP$1166$1, (void*)"mulss xmm7, ", 13ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	fb_StrAssign( (void*)&TMP$1167$1, -1ll, (void*)vr$385, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1167$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1167$1 );
	__builtin_memset( &TMP$1171$1, 0, 24ll );
	__builtin_memset( &TMP$1168$1, 0, 24ll );
	FBSTRING* vr$391 = fb_StrConcat( &TMP$1168$1, (void*)"movd      ", 11ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1169$1, 0, 24ll );
	FBSTRING* vr$394 = fb_StrConcat( &TMP$1169$1, (void*)vr$391, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1170$1, 0, 24ll );
	FBSTRING* vr$397 = fb_StrConcat( &TMP$1170$1, (void*)vr$394, -1ll, (void*)((uint8*)REGNAME$1 + 24ll), -1ll );
	fb_StrAssign( (void*)&TMP$1171$1, -1ll, (void*)vr$397, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1171$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1171$1 );
	HPREPOPERAND( VREG_SIN_C1$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1173$1, 0, 24ll );
	__builtin_memset( &TMP$1172$1, 0, 24ll );
	FBSTRING* vr$403 = fb_StrConcat( &TMP$1172$1, (void*)"addss xmm7, ", 13ll, (void*)&SRC$1, -1ll );
	fb_StrAssign( (void*)&TMP$1173$1, -1ll, (void*)vr$403, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1173$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1173$1 );
	__builtin_memset( &TMP$1175$1, 0, 24ll );
	__builtin_memset( &TMP$1174$1, 0, 24ll );
	FBSTRING* vr$409 = fb_StrConcat( &TMP$1174$1, (void*)"mulss xmm7, ", 13ll, (void*)((uint8*)REGNAME$1 + 48ll), -1ll );
	fb_StrAssign( (void*)&TMP$1175$1, -1ll, (void*)vr$409, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1175$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1175$1 );
	HPREPOPERAND( VREG_SIN_C0$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1177$1, 0, 24ll );
	__builtin_memset( &TMP$1176$1, 0, 24ll );
	FBSTRING* vr$415 = fb_StrConcat( &TMP$1176$1, (void*)"addss xmm7, ", 13ll, (void*)&SRC$1, -1ll );
	fb_StrAssign( (void*)&TMP$1177$1, -1ll, (void*)vr$415, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1177$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1177$1 );
	__builtin_memset( &TMP$1181$1, 0, 24ll );
	__builtin_memset( &TMP$1178$1, 0, 24ll );
	FBSTRING* vr$421 = fb_StrConcat( &TMP$1178$1, (void*)"mulss ", 7ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1179$1, 0, 24ll );
	FBSTRING* vr$424 = fb_StrConcat( &TMP$1179$1, (void*)vr$421, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1180$1, 0, 24ll );
	FBSTRING* vr$427 = fb_StrConcat( &TMP$1180$1, (void*)vr$424, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1181$1, -1ll, (void*)vr$427, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1181$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1181$1 );
	STACKPOINTER$1 = 4ll;
	{
		I$1 = 0ll;
		label$555:;
		{
			if( *(int64*)((int64)(int64*)ISFREE$1 + (I$1 << (3ll & 63ll))) != 0ll) goto label$557;
			{
				if( I$1 >= 2ll) goto label$559;
				{
					FBSTRING TMP$1183$5;
					FBSTRING TMP$1184$5;
					FBSTRING TMP$1185$5;
					FBSTRING TMP$1186$5;
					FBSTRING TMP$1187$5;
					FBSTRING TMP$1188$5;
					__builtin_memset( &TMP$1188$5, 0, 24ll );
					FBSTRING* vr$432 = fb_LongintToStr( STACKPOINTER$1 );
					__builtin_memset( &TMP$1183$5, 0, 24ll );
					FBSTRING* vr$437 = fb_StrConcat( &TMP$1183$5, (void*)"mov ", 5ll, (void*)((int64)(FBSTRING*)REGNAME$1 + (I$1 * 24ll)), -1ll );
					__builtin_memset( &TMP$1184$5, 0, 24ll );
					FBSTRING* vr$440 = fb_StrConcat( &TMP$1184$5, (void*)vr$437, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$1185$5, 0, 24ll );
					FBSTRING* vr$443 = fb_StrConcat( &TMP$1185$5, (void*)vr$440, -1ll, (void*)"[esp+", 6ll );
					__builtin_memset( &TMP$1186$5, 0, 24ll );
					FBSTRING* vr$446 = fb_StrConcat( &TMP$1186$5, (void*)vr$443, -1ll, (void*)vr$432, -1ll );
					__builtin_memset( &TMP$1187$5, 0, 24ll );
					FBSTRING* vr$449 = fb_StrConcat( &TMP$1187$5, (void*)vr$446, -1ll, (void*)"]", 2ll );
					fb_StrAssign( (void*)&TMP$1188$5, -1ll, (void*)vr$449, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$1188$5 );
					fb_StrDelete( (FBSTRING*)&TMP$1188$5 );
				}
				goto label$558;
				label$559:;
				{
					FBSTRING TMP$1189$5;
					FBSTRING TMP$1190$5;
					FBSTRING TMP$1191$5;
					FBSTRING TMP$1192$5;
					FBSTRING TMP$1193$5;
					FBSTRING TMP$1194$5;
					__builtin_memset( &TMP$1194$5, 0, 24ll );
					FBSTRING* vr$453 = fb_LongintToStr( STACKPOINTER$1 );
					__builtin_memset( &TMP$1189$5, 0, 24ll );
					FBSTRING* vr$458 = fb_StrConcat( &TMP$1189$5, (void*)"movss ", 7ll, (void*)((int64)(FBSTRING*)REGNAME$1 + (I$1 * 24ll)), -1ll );
					__builtin_memset( &TMP$1190$5, 0, 24ll );
					FBSTRING* vr$461 = fb_StrConcat( &TMP$1190$5, (void*)vr$458, -1ll, (void*)", ", 3ll );
					__builtin_memset( &TMP$1191$5, 0, 24ll );
					FBSTRING* vr$464 = fb_StrConcat( &TMP$1191$5, (void*)vr$461, -1ll, (void*)"[esp+", 6ll );
					__builtin_memset( &TMP$1192$5, 0, 24ll );
					FBSTRING* vr$467 = fb_StrConcat( &TMP$1192$5, (void*)vr$464, -1ll, (void*)vr$453, -1ll );
					__builtin_memset( &TMP$1193$5, 0, 24ll );
					FBSTRING* vr$470 = fb_StrConcat( &TMP$1193$5, (void*)vr$467, -1ll, (void*)"]", 2ll );
					fb_StrAssign( (void*)&TMP$1194$5, -1ll, (void*)vr$470, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$1194$5 );
					fb_StrDelete( (FBSTRING*)&TMP$1194$5 );
				}
				label$558:;
				STACKPOINTER$1 = STACKPOINTER$1 + 4ll;
			}
			label$557:;
			label$556:;
		}
		label$553:;
		I$1 = I$1 + 1ll;
		label$552:;
		if( I$1 <= 2ll) goto label$555;
		label$554:;
	}
	__builtin_memset( &TMP$1196$1, 0, 24ll );
	FBSTRING* vr$476 = fb_LongintToStr( STACKSIZE$1 );
	__builtin_memset( &TMP$1195$1, 0, 24ll );
	FBSTRING* vr$479 = fb_StrConcat( &TMP$1195$1, (void*)"add esp, ", 10ll, (void*)vr$476, -1ll );
	fb_StrAssign( (void*)&TMP$1196$1, -1ll, (void*)vr$479, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1196$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1196$1 );
	label$521:;
}

static void _EMITSIN_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1197$1;
	FBSTRING TMP$1215$1;
	FBSTRING TMP$1216$1;
	label$560:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$562;
	TMP$1197$1 = 24ll;
	goto label$808;
	label$562:;
	TMP$1197$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$808:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1197$1 * 56ll)) + 8ll);
	if( ((int64)-(DDSIZE$1 == 4ll) & (int64)-(*(int64*)((uint8*)&ENV$ + 240ll) == 1ll)) == 0ll) goto label$564;
	{
		HSINCOS_FAST_SSE( DVREG$1, 0ll );
		goto label$561;
	}
	label$564:;
	label$563:;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$566;
	{
		FBSTRING TMP$1198$2;
		FBSTRING TMP$1199$2;
		__builtin_memset( &TMP$1199$2, 0, 24ll );
		FBSTRING* vr$10 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1198$2, 0, 24ll );
		FBSTRING* vr$13 = fb_StrConcat( &TMP$1198$2, (void*)"sub esp, ", 10ll, (void*)vr$10, -1ll );
		fb_StrAssign( (void*)&TMP$1199$2, -1ll, (void*)vr$13, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1199$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1199$2 );
		if( DDSIZE$1 <= 4ll) goto label$568;
		{
			FBSTRING TMP$1200$3;
			FBSTRING TMP$1201$3;
			__builtin_memset( &TMP$1201$3, 0, 24ll );
			__builtin_memset( &TMP$1200$3, 0, 24ll );
			FBSTRING* vr$19 = fb_StrConcat( &TMP$1200$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1201$3, -1ll, (void*)vr$19, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1201$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1201$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$567;
		label$568:;
		{
			FBSTRING TMP$1202$3;
			FBSTRING TMP$1203$3;
			__builtin_memset( &TMP$1203$3, 0, 24ll );
			__builtin_memset( &TMP$1202$3, 0, 24ll );
			FBSTRING* vr$25 = fb_StrConcat( &TMP$1202$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1203$3, -1ll, (void*)vr$25, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1203$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1203$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$567:;
	}
	label$566:;
	label$565:;
	OUTP( (char*)"fsin" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$570;
	{
		FBSTRING TMP$1205$2;
		FBSTRING TMP$1206$2;
		__builtin_memset( &TMP$1206$2, 0, 24ll );
		FBSTRING* vr$30 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1205$2, 0, 24ll );
		FBSTRING* vr$33 = fb_StrConcat( &TMP$1205$2, (void*)"sub esp, ", 10ll, (void*)vr$30, -1ll );
		fb_StrAssign( (void*)&TMP$1206$2, -1ll, (void*)vr$33, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1206$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1206$2 );
	}
	label$570:;
	label$569:;
	if( DDSIZE$1 <= 4ll) goto label$572;
	{
		FBSTRING TMP$1207$2;
		FBSTRING TMP$1208$2;
		FBSTRING TMP$1209$2;
		FBSTRING TMP$1210$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$1210$2, 0, 24ll );
		__builtin_memset( &TMP$1207$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$1207$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1208$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$1208$2, (void*)vr$39, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1209$2, 0, 24ll );
		FBSTRING* vr$45 = fb_StrConcat( &TMP$1209$2, (void*)vr$42, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1210$2, -1ll, (void*)vr$45, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1210$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1210$2 );
	}
	goto label$571;
	label$572:;
	{
		FBSTRING TMP$1211$2;
		FBSTRING TMP$1212$2;
		FBSTRING TMP$1213$2;
		FBSTRING TMP$1214$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1214$2, 0, 24ll );
		__builtin_memset( &TMP$1211$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$1211$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1212$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$1212$2, (void*)vr$51, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1213$2, 0, 24ll );
		FBSTRING* vr$57 = fb_StrConcat( &TMP$1213$2, (void*)vr$54, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1214$2, -1ll, (void*)vr$57, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1214$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1214$2 );
	}
	label$571:;
	__builtin_memset( &TMP$1216$1, 0, 24ll );
	FBSTRING* vr$61 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$1215$1, 0, 24ll );
	FBSTRING* vr$64 = fb_StrConcat( &TMP$1215$1, (void*)"add esp, ", 10ll, (void*)vr$61, -1ll );
	fb_StrAssign( (void*)&TMP$1216$1, -1ll, (void*)vr$64, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1216$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1216$1 );
	label$561:;
}

static void _EMITASIN_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1217$1;
	FBSTRING TMP$1237$1;
	FBSTRING TMP$1238$1;
	label$573:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$575;
	TMP$1217$1 = 24ll;
	goto label$809;
	label$575:;
	TMP$1217$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$809:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1217$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$577;
	{
		FBSTRING TMP$1218$2;
		FBSTRING TMP$1219$2;
		__builtin_memset( &TMP$1219$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1218$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1218$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1219$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1219$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1219$2 );
		if( DDSIZE$1 <= 4ll) goto label$579;
		{
			FBSTRING TMP$1220$3;
			FBSTRING TMP$1221$3;
			__builtin_memset( &TMP$1221$3, 0, 24ll );
			__builtin_memset( &TMP$1220$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1220$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1221$3, -1ll, (void*)vr$16, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1221$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1221$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$578;
		label$579:;
		{
			FBSTRING TMP$1222$3;
			FBSTRING TMP$1223$3;
			__builtin_memset( &TMP$1223$3, 0, 24ll );
			__builtin_memset( &TMP$1222$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1222$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1223$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1223$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1223$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$578:;
	}
	label$577:;
	label$576:;
	OUTP( (char*)"fld st(0)" );
	OUTP( (char*)"fmul st(0), st(0)" );
	OUTP( (char*)"fld1" );
	OUTP( (char*)"fsubrp" );
	OUTP( (char*)"fsqrt" );
	OUTP( (char*)"fpatan" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$581;
	{
		FBSTRING TMP$1227$2;
		FBSTRING TMP$1228$2;
		__builtin_memset( &TMP$1228$2, 0, 24ll );
		FBSTRING* vr$27 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1227$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$1227$2, (void*)"sub esp, ", 10ll, (void*)vr$27, -1ll );
		fb_StrAssign( (void*)&TMP$1228$2, -1ll, (void*)vr$30, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1228$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1228$2 );
	}
	label$581:;
	label$580:;
	if( DDSIZE$1 <= 4ll) goto label$583;
	{
		FBSTRING TMP$1229$2;
		FBSTRING TMP$1230$2;
		FBSTRING TMP$1231$2;
		FBSTRING TMP$1232$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$1232$2, 0, 24ll );
		__builtin_memset( &TMP$1229$2, 0, 24ll );
		FBSTRING* vr$36 = fb_StrConcat( &TMP$1229$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1230$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$1230$2, (void*)vr$36, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1231$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$1231$2, (void*)vr$39, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1232$2, -1ll, (void*)vr$42, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1232$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1232$2 );
	}
	goto label$582;
	label$583:;
	{
		FBSTRING TMP$1233$2;
		FBSTRING TMP$1234$2;
		FBSTRING TMP$1235$2;
		FBSTRING TMP$1236$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1236$2, 0, 24ll );
		__builtin_memset( &TMP$1233$2, 0, 24ll );
		FBSTRING* vr$48 = fb_StrConcat( &TMP$1233$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1234$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$1234$2, (void*)vr$48, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1235$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$1235$2, (void*)vr$51, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1236$2, -1ll, (void*)vr$54, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1236$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1236$2 );
	}
	label$582:;
	__builtin_memset( &TMP$1238$1, 0, 24ll );
	FBSTRING* vr$58 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$1237$1, 0, 24ll );
	FBSTRING* vr$61 = fb_StrConcat( &TMP$1237$1, (void*)"add esp, ", 10ll, (void*)vr$58, -1ll );
	fb_StrAssign( (void*)&TMP$1238$1, -1ll, (void*)vr$61, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1238$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1238$1 );
	label$574:;
}

static void _EMITCOS_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1239$1;
	FBSTRING TMP$1257$1;
	FBSTRING TMP$1258$1;
	label$584:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$586;
	TMP$1239$1 = 24ll;
	goto label$810;
	label$586:;
	TMP$1239$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$810:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1239$1 * 56ll)) + 8ll);
	if( ((int64)-(DDSIZE$1 == 4ll) & (int64)-(*(int64*)((uint8*)&ENV$ + 240ll) == 1ll)) == 0ll) goto label$588;
	{
		HSINCOS_FAST_SSE( DVREG$1, -1ll );
		goto label$585;
	}
	label$588:;
	label$587:;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$590;
	{
		FBSTRING TMP$1240$2;
		FBSTRING TMP$1241$2;
		__builtin_memset( &TMP$1241$2, 0, 24ll );
		FBSTRING* vr$10 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1240$2, 0, 24ll );
		FBSTRING* vr$13 = fb_StrConcat( &TMP$1240$2, (void*)"sub esp, ", 10ll, (void*)vr$10, -1ll );
		fb_StrAssign( (void*)&TMP$1241$2, -1ll, (void*)vr$13, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1241$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1241$2 );
		if( DDSIZE$1 <= 4ll) goto label$592;
		{
			FBSTRING TMP$1242$3;
			FBSTRING TMP$1243$3;
			__builtin_memset( &TMP$1243$3, 0, 24ll );
			__builtin_memset( &TMP$1242$3, 0, 24ll );
			FBSTRING* vr$19 = fb_StrConcat( &TMP$1242$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1243$3, -1ll, (void*)vr$19, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1243$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1243$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$591;
		label$592:;
		{
			FBSTRING TMP$1244$3;
			FBSTRING TMP$1245$3;
			__builtin_memset( &TMP$1245$3, 0, 24ll );
			__builtin_memset( &TMP$1244$3, 0, 24ll );
			FBSTRING* vr$25 = fb_StrConcat( &TMP$1244$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1245$3, -1ll, (void*)vr$25, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1245$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1245$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$591:;
	}
	label$590:;
	label$589:;
	OUTP( (char*)"fcos" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$594;
	{
		FBSTRING TMP$1247$2;
		FBSTRING TMP$1248$2;
		__builtin_memset( &TMP$1248$2, 0, 24ll );
		FBSTRING* vr$30 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1247$2, 0, 24ll );
		FBSTRING* vr$33 = fb_StrConcat( &TMP$1247$2, (void*)"sub esp, ", 10ll, (void*)vr$30, -1ll );
		fb_StrAssign( (void*)&TMP$1248$2, -1ll, (void*)vr$33, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1248$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1248$2 );
	}
	label$594:;
	label$593:;
	if( DDSIZE$1 <= 4ll) goto label$596;
	{
		FBSTRING TMP$1249$2;
		FBSTRING TMP$1250$2;
		FBSTRING TMP$1251$2;
		FBSTRING TMP$1252$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$1252$2, 0, 24ll );
		__builtin_memset( &TMP$1249$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$1249$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1250$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$1250$2, (void*)vr$39, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1251$2, 0, 24ll );
		FBSTRING* vr$45 = fb_StrConcat( &TMP$1251$2, (void*)vr$42, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1252$2, -1ll, (void*)vr$45, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1252$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1252$2 );
	}
	goto label$595;
	label$596:;
	{
		FBSTRING TMP$1253$2;
		FBSTRING TMP$1254$2;
		FBSTRING TMP$1255$2;
		FBSTRING TMP$1256$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1256$2, 0, 24ll );
		__builtin_memset( &TMP$1253$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$1253$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1254$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$1254$2, (void*)vr$51, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1255$2, 0, 24ll );
		FBSTRING* vr$57 = fb_StrConcat( &TMP$1255$2, (void*)vr$54, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1256$2, -1ll, (void*)vr$57, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1256$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1256$2 );
	}
	label$595:;
	__builtin_memset( &TMP$1258$1, 0, 24ll );
	FBSTRING* vr$61 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$1257$1, 0, 24ll );
	FBSTRING* vr$64 = fb_StrConcat( &TMP$1257$1, (void*)"add esp, ", 10ll, (void*)vr$61, -1ll );
	fb_StrAssign( (void*)&TMP$1258$1, -1ll, (void*)vr$64, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1258$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1258$1 );
	label$585:;
}

static void _EMITACOS_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1259$1;
	FBSTRING TMP$1276$1;
	FBSTRING TMP$1277$1;
	label$597:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$599;
	TMP$1259$1 = 24ll;
	goto label$811;
	label$599:;
	TMP$1259$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$811:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1259$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$601;
	{
		FBSTRING TMP$1260$2;
		FBSTRING TMP$1261$2;
		__builtin_memset( &TMP$1261$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1260$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1260$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1261$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1261$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1261$2 );
		if( DDSIZE$1 <= 4ll) goto label$603;
		{
			FBSTRING TMP$1262$3;
			FBSTRING TMP$1263$3;
			__builtin_memset( &TMP$1263$3, 0, 24ll );
			__builtin_memset( &TMP$1262$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1262$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1263$3, -1ll, (void*)vr$16, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1263$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1263$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$602;
		label$603:;
		{
			FBSTRING TMP$1264$3;
			FBSTRING TMP$1265$3;
			__builtin_memset( &TMP$1265$3, 0, 24ll );
			__builtin_memset( &TMP$1264$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1264$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1265$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1265$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1265$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$602:;
	}
	label$601:;
	label$600:;
	OUTP( (char*)"fld st(0)" );
	OUTP( (char*)"fmul st(0), st(0)" );
	OUTP( (char*)"fld1" );
	OUTP( (char*)"fsubrp" );
	OUTP( (char*)"fsqrt" );
	OUTP( (char*)"fxch" );
	OUTP( (char*)"fpatan" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$605;
	{
		FBSTRING TMP$1266$2;
		FBSTRING TMP$1267$2;
		__builtin_memset( &TMP$1267$2, 0, 24ll );
		FBSTRING* vr$27 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1266$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$1266$2, (void*)"sub esp, ", 10ll, (void*)vr$27, -1ll );
		fb_StrAssign( (void*)&TMP$1267$2, -1ll, (void*)vr$30, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1267$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1267$2 );
	}
	label$605:;
	label$604:;
	if( DDSIZE$1 <= 4ll) goto label$607;
	{
		FBSTRING TMP$1268$2;
		FBSTRING TMP$1269$2;
		FBSTRING TMP$1270$2;
		FBSTRING TMP$1271$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$1271$2, 0, 24ll );
		__builtin_memset( &TMP$1268$2, 0, 24ll );
		FBSTRING* vr$36 = fb_StrConcat( &TMP$1268$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1269$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$1269$2, (void*)vr$36, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1270$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$1270$2, (void*)vr$39, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1271$2, -1ll, (void*)vr$42, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1271$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1271$2 );
	}
	goto label$606;
	label$607:;
	{
		FBSTRING TMP$1272$2;
		FBSTRING TMP$1273$2;
		FBSTRING TMP$1274$2;
		FBSTRING TMP$1275$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1275$2, 0, 24ll );
		__builtin_memset( &TMP$1272$2, 0, 24ll );
		FBSTRING* vr$48 = fb_StrConcat( &TMP$1272$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1273$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$1273$2, (void*)vr$48, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1274$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$1274$2, (void*)vr$51, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1275$2, -1ll, (void*)vr$54, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1275$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1275$2 );
	}
	label$606:;
	__builtin_memset( &TMP$1277$1, 0, 24ll );
	FBSTRING* vr$58 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$1276$1, 0, 24ll );
	FBSTRING* vr$61 = fb_StrConcat( &TMP$1276$1, (void*)"add esp, ", 10ll, (void*)vr$58, -1ll );
	fb_StrAssign( (void*)&TMP$1277$1, -1ll, (void*)vr$61, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1277$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1277$1 );
	label$598:;
}

static void _EMITTAN_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1278$1;
	FBSTRING TMP$1297$1;
	FBSTRING TMP$1298$1;
	label$608:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$610;
	TMP$1278$1 = 24ll;
	goto label$812;
	label$610:;
	TMP$1278$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$812:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1278$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$612;
	{
		FBSTRING TMP$1279$2;
		FBSTRING TMP$1280$2;
		__builtin_memset( &TMP$1280$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1279$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1279$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1280$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1280$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1280$2 );
		if( DDSIZE$1 <= 4ll) goto label$614;
		{
			FBSTRING TMP$1281$3;
			FBSTRING TMP$1282$3;
			__builtin_memset( &TMP$1282$3, 0, 24ll );
			__builtin_memset( &TMP$1281$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1281$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1282$3, -1ll, (void*)vr$16, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1282$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1282$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$613;
		label$614:;
		{
			FBSTRING TMP$1283$3;
			FBSTRING TMP$1284$3;
			__builtin_memset( &TMP$1284$3, 0, 24ll );
			__builtin_memset( &TMP$1283$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1283$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1284$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1284$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1284$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$613:;
	}
	label$612:;
	label$611:;
	OUTP( (char*)"fptan" );
	OUTP( (char*)"fstp st(0)" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$616;
	{
		FBSTRING TMP$1287$2;
		FBSTRING TMP$1288$2;
		__builtin_memset( &TMP$1288$2, 0, 24ll );
		FBSTRING* vr$27 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1287$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$1287$2, (void*)"sub esp, ", 10ll, (void*)vr$27, -1ll );
		fb_StrAssign( (void*)&TMP$1288$2, -1ll, (void*)vr$30, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1288$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1288$2 );
	}
	label$616:;
	label$615:;
	if( DDSIZE$1 <= 4ll) goto label$618;
	{
		FBSTRING TMP$1289$2;
		FBSTRING TMP$1290$2;
		FBSTRING TMP$1291$2;
		FBSTRING TMP$1292$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$1292$2, 0, 24ll );
		__builtin_memset( &TMP$1289$2, 0, 24ll );
		FBSTRING* vr$36 = fb_StrConcat( &TMP$1289$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1290$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$1290$2, (void*)vr$36, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1291$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$1291$2, (void*)vr$39, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1292$2, -1ll, (void*)vr$42, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1292$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1292$2 );
	}
	goto label$617;
	label$618:;
	{
		FBSTRING TMP$1293$2;
		FBSTRING TMP$1294$2;
		FBSTRING TMP$1295$2;
		FBSTRING TMP$1296$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1296$2, 0, 24ll );
		__builtin_memset( &TMP$1293$2, 0, 24ll );
		FBSTRING* vr$48 = fb_StrConcat( &TMP$1293$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1294$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$1294$2, (void*)vr$48, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1295$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$1295$2, (void*)vr$51, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1296$2, -1ll, (void*)vr$54, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1296$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1296$2 );
	}
	label$617:;
	__builtin_memset( &TMP$1298$1, 0, 24ll );
	FBSTRING* vr$58 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$1297$1, 0, 24ll );
	FBSTRING* vr$61 = fb_StrConcat( &TMP$1297$1, (void*)"add esp, ", 10ll, (void*)vr$58, -1ll );
	fb_StrAssign( (void*)&TMP$1298$1, -1ll, (void*)vr$61, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1298$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1298$1 );
	label$609:;
}

static void _EMITATAN_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1299$1;
	FBSTRING TMP$1316$1;
	FBSTRING TMP$1317$1;
	label$619:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$621;
	TMP$1299$1 = 24ll;
	goto label$813;
	label$621:;
	TMP$1299$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$813:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1299$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$623;
	{
		FBSTRING TMP$1300$2;
		FBSTRING TMP$1301$2;
		__builtin_memset( &TMP$1301$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1300$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1300$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1301$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1301$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1301$2 );
		if( DDSIZE$1 <= 4ll) goto label$625;
		{
			FBSTRING TMP$1302$3;
			FBSTRING TMP$1303$3;
			__builtin_memset( &TMP$1303$3, 0, 24ll );
			__builtin_memset( &TMP$1302$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1302$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1303$3, -1ll, (void*)vr$16, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1303$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1303$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$624;
		label$625:;
		{
			FBSTRING TMP$1304$3;
			FBSTRING TMP$1305$3;
			__builtin_memset( &TMP$1305$3, 0, 24ll );
			__builtin_memset( &TMP$1304$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1304$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1305$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1305$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1305$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$624:;
	}
	label$623:;
	label$622:;
	OUTP( (char*)"fld1" );
	OUTP( (char*)"fpatan" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$627;
	{
		FBSTRING TMP$1306$2;
		FBSTRING TMP$1307$2;
		__builtin_memset( &TMP$1307$2, 0, 24ll );
		FBSTRING* vr$27 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1306$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$1306$2, (void*)"sub esp, ", 10ll, (void*)vr$27, -1ll );
		fb_StrAssign( (void*)&TMP$1307$2, -1ll, (void*)vr$30, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1307$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1307$2 );
	}
	label$627:;
	label$626:;
	if( DDSIZE$1 <= 4ll) goto label$629;
	{
		FBSTRING TMP$1308$2;
		FBSTRING TMP$1309$2;
		FBSTRING TMP$1310$2;
		FBSTRING TMP$1311$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$1311$2, 0, 24ll );
		__builtin_memset( &TMP$1308$2, 0, 24ll );
		FBSTRING* vr$36 = fb_StrConcat( &TMP$1308$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1309$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$1309$2, (void*)vr$36, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1310$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$1310$2, (void*)vr$39, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1311$2, -1ll, (void*)vr$42, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1311$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1311$2 );
	}
	goto label$628;
	label$629:;
	{
		FBSTRING TMP$1312$2;
		FBSTRING TMP$1313$2;
		FBSTRING TMP$1314$2;
		FBSTRING TMP$1315$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1315$2, 0, 24ll );
		__builtin_memset( &TMP$1312$2, 0, 24ll );
		FBSTRING* vr$48 = fb_StrConcat( &TMP$1312$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1313$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$1313$2, (void*)vr$48, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1314$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$1314$2, (void*)vr$51, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1315$2, -1ll, (void*)vr$54, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1315$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1315$2 );
	}
	label$628:;
	__builtin_memset( &TMP$1317$1, 0, 24ll );
	FBSTRING* vr$58 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$1316$1, 0, 24ll );
	FBSTRING* vr$61 = fb_StrConcat( &TMP$1316$1, (void*)"add esp, ", 10ll, (void*)vr$58, -1ll );
	fb_StrAssign( (void*)&TMP$1317$1, -1ll, (void*)vr$61, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1317$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1317$1 );
	label$620:;
}

static void _EMITSQRT_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1318$1;
	label$630:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$632;
	TMP$1318$1 = 24ll;
	goto label$814;
	label$632:;
	TMP$1318$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$814:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1318$1 * 56ll)) + 8ll);
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$634;
	{
		FBSTRING TMP$1319$2;
		FBSTRING TMP$1320$2;
		__builtin_memset( &TMP$1320$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1319$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1319$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1320$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1320$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1320$2 );
		if( DDSIZE$1 <= 4ll) goto label$636;
		{
			FBSTRING TMP$1321$3;
			FBSTRING TMP$1322$3;
			FBSTRING TMP$1323$3;
			FBSTRING TMP$1324$3;
			OUTP( (char*)"fstp qword ptr [esp]" );
			__builtin_memset( &TMP$1324$3, 0, 24ll );
			__builtin_memset( &TMP$1321$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1321$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1322$3, 0, 24ll );
			FBSTRING* vr$19 = fb_StrConcat( &TMP$1322$3, (void*)vr$16, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1323$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1323$3, (void*)vr$19, -1ll, (void*)"qword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1324$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1324$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1324$3 );
		}
		goto label$635;
		label$636:;
		{
			FBSTRING TMP$1325$3;
			FBSTRING TMP$1326$3;
			FBSTRING TMP$1327$3;
			FBSTRING TMP$1328$3;
			OUTP( (char*)"fstp dword ptr [esp]" );
			__builtin_memset( &TMP$1328$3, 0, 24ll );
			__builtin_memset( &TMP$1325$3, 0, 24ll );
			FBSTRING* vr$28 = fb_StrConcat( &TMP$1325$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1326$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$1326$3, (void*)vr$28, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1327$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$1327$3, (void*)vr$31, -1ll, (void*)"dword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1328$3, -1ll, (void*)vr$34, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1328$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1328$3 );
		}
		label$635:;
	}
	label$634:;
	label$633:;
	if( DDSIZE$1 <= 4ll) goto label$638;
	{
		FBSTRING TMP$1330$2;
		FBSTRING TMP$1331$2;
		FBSTRING TMP$1332$2;
		FBSTRING TMP$1333$2;
		__builtin_memset( &TMP$1333$2, 0, 24ll );
		__builtin_memset( &TMP$1330$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$1330$2, (void*)"sqrtsd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1331$2, 0, 24ll );
		FBSTRING* vr$43 = fb_StrConcat( &TMP$1331$2, (void*)vr$40, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1332$2, 0, 24ll );
		FBSTRING* vr$46 = fb_StrConcat( &TMP$1332$2, (void*)vr$43, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1333$2, -1ll, (void*)vr$46, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1333$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1333$2 );
	}
	goto label$637;
	label$638:;
	{
		FBSTRING TMP$1335$2;
		FBSTRING TMP$1336$2;
		FBSTRING TMP$1337$2;
		FBSTRING TMP$1338$2;
		__builtin_memset( &TMP$1338$2, 0, 24ll );
		__builtin_memset( &TMP$1335$2, 0, 24ll );
		FBSTRING* vr$52 = fb_StrConcat( &TMP$1335$2, (void*)"sqrtss ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1336$2, 0, 24ll );
		FBSTRING* vr$55 = fb_StrConcat( &TMP$1336$2, (void*)vr$52, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1337$2, 0, 24ll );
		FBSTRING* vr$58 = fb_StrConcat( &TMP$1337$2, (void*)vr$55, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1338$2, -1ll, (void*)vr$58, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1338$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1338$2 );
	}
	label$637:;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$640;
	{
		FBSTRING TMP$1339$2;
		FBSTRING TMP$1340$2;
		__builtin_memset( &TMP$1340$2, 0, 24ll );
		FBSTRING* vr$63 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1339$2, 0, 24ll );
		FBSTRING* vr$66 = fb_StrConcat( &TMP$1339$2, (void*)"add esp, ", 10ll, (void*)vr$63, -1ll );
		fb_StrAssign( (void*)&TMP$1340$2, -1ll, (void*)vr$66, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1340$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1340$2 );
	}
	label$640:;
	label$639:;
	label$631:;
}

static void _EMITRSQRT_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1341$1;
	label$641:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$643;
	TMP$1341$1 = 24ll;
	goto label$815;
	label$643:;
	TMP$1341$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$815:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1341$1 * 56ll)) + 8ll);
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$645;
	{
		FBSTRING TMP$1342$2;
		FBSTRING TMP$1343$2;
		__builtin_memset( &TMP$1343$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1342$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1342$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1343$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1343$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1343$2 );
		if( DDSIZE$1 <= 4ll) goto label$647;
		{
			FBSTRING TMP$1344$3;
			FBSTRING TMP$1345$3;
			FBSTRING TMP$1346$3;
			FBSTRING TMP$1347$3;
			OUTP( (char*)"fstp qword ptr [esp]" );
			__builtin_memset( &TMP$1347$3, 0, 24ll );
			__builtin_memset( &TMP$1344$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1344$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1345$3, 0, 24ll );
			FBSTRING* vr$19 = fb_StrConcat( &TMP$1345$3, (void*)vr$16, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1346$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1346$3, (void*)vr$19, -1ll, (void*)"qword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1347$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1347$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1347$3 );
		}
		goto label$646;
		label$647:;
		{
			FBSTRING TMP$1348$3;
			FBSTRING TMP$1349$3;
			FBSTRING TMP$1350$3;
			FBSTRING TMP$1351$3;
			OUTP( (char*)"fstp dword ptr [esp]" );
			__builtin_memset( &TMP$1351$3, 0, 24ll );
			__builtin_memset( &TMP$1348$3, 0, 24ll );
			FBSTRING* vr$28 = fb_StrConcat( &TMP$1348$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1349$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$1349$3, (void*)vr$28, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1350$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$1350$3, (void*)vr$31, -1ll, (void*)"dword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1351$3, -1ll, (void*)vr$34, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1351$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1351$3 );
		}
		label$646:;
	}
	label$645:;
	label$644:;
	if( DDSIZE$1 <= 4ll) goto label$649;
	{
		FBSTRING TMP$1353$2;
		FBSTRING TMP$1354$2;
		FBSTRING TMP$1355$2;
		FBSTRING TMP$1356$2;
		__builtin_memset( &TMP$1356$2, 0, 24ll );
		__builtin_memset( &TMP$1353$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$1353$2, (void*)"rsqrtsd ", 9ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1354$2, 0, 24ll );
		FBSTRING* vr$43 = fb_StrConcat( &TMP$1354$2, (void*)vr$40, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1355$2, 0, 24ll );
		FBSTRING* vr$46 = fb_StrConcat( &TMP$1355$2, (void*)vr$43, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1356$2, -1ll, (void*)vr$46, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1356$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1356$2 );
	}
	goto label$648;
	label$649:;
	{
		FBSTRING TMP$1358$2;
		FBSTRING TMP$1359$2;
		FBSTRING TMP$1360$2;
		FBSTRING TMP$1361$2;
		__builtin_memset( &TMP$1361$2, 0, 24ll );
		__builtin_memset( &TMP$1358$2, 0, 24ll );
		FBSTRING* vr$52 = fb_StrConcat( &TMP$1358$2, (void*)"rsqrtss ", 9ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1359$2, 0, 24ll );
		FBSTRING* vr$55 = fb_StrConcat( &TMP$1359$2, (void*)vr$52, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1360$2, 0, 24ll );
		FBSTRING* vr$58 = fb_StrConcat( &TMP$1360$2, (void*)vr$55, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1361$2, -1ll, (void*)vr$58, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1361$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1361$2 );
	}
	label$648:;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$651;
	{
		FBSTRING TMP$1362$2;
		FBSTRING TMP$1363$2;
		__builtin_memset( &TMP$1363$2, 0, 24ll );
		FBSTRING* vr$63 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1362$2, 0, 24ll );
		FBSTRING* vr$66 = fb_StrConcat( &TMP$1362$2, (void*)"add esp, ", 10ll, (void*)vr$63, -1ll );
		fb_StrAssign( (void*)&TMP$1363$2, -1ll, (void*)vr$66, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1363$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1363$2 );
	}
	label$651:;
	label$650:;
	label$642:;
}

static void _EMITRCP_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1364$1;
	label$652:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$654;
	TMP$1364$1 = 24ll;
	goto label$816;
	label$654:;
	TMP$1364$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$816:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1364$1 * 56ll)) + 8ll);
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$656;
	{
		FBSTRING TMP$1365$2;
		FBSTRING TMP$1366$2;
		__builtin_memset( &TMP$1366$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1365$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1365$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1366$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1366$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1366$2 );
		if( DDSIZE$1 <= 4ll) goto label$658;
		{
			FBSTRING TMP$1367$3;
			FBSTRING TMP$1368$3;
			FBSTRING TMP$1369$3;
			FBSTRING TMP$1370$3;
			OUTP( (char*)"fstp qword ptr [esp]" );
			__builtin_memset( &TMP$1370$3, 0, 24ll );
			__builtin_memset( &TMP$1367$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1367$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1368$3, 0, 24ll );
			FBSTRING* vr$19 = fb_StrConcat( &TMP$1368$3, (void*)vr$16, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1369$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1369$3, (void*)vr$19, -1ll, (void*)"qword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1370$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1370$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1370$3 );
		}
		goto label$657;
		label$658:;
		{
			FBSTRING TMP$1371$3;
			FBSTRING TMP$1372$3;
			FBSTRING TMP$1373$3;
			FBSTRING TMP$1374$3;
			OUTP( (char*)"fstp dword ptr [esp]" );
			__builtin_memset( &TMP$1374$3, 0, 24ll );
			__builtin_memset( &TMP$1371$3, 0, 24ll );
			FBSTRING* vr$28 = fb_StrConcat( &TMP$1371$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1372$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$1372$3, (void*)vr$28, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1373$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$1373$3, (void*)vr$31, -1ll, (void*)"dword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1374$3, -1ll, (void*)vr$34, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1374$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1374$3 );
		}
		label$657:;
	}
	label$656:;
	label$655:;
	if( DDSIZE$1 <= 4ll) goto label$660;
	{
		FBSTRING TMP$1376$2;
		FBSTRING TMP$1377$2;
		FBSTRING TMP$1378$2;
		FBSTRING TMP$1379$2;
		__builtin_memset( &TMP$1379$2, 0, 24ll );
		__builtin_memset( &TMP$1376$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$1376$2, (void*)"rcpsd ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1377$2, 0, 24ll );
		FBSTRING* vr$43 = fb_StrConcat( &TMP$1377$2, (void*)vr$40, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1378$2, 0, 24ll );
		FBSTRING* vr$46 = fb_StrConcat( &TMP$1378$2, (void*)vr$43, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1379$2, -1ll, (void*)vr$46, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1379$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1379$2 );
	}
	goto label$659;
	label$660:;
	{
		FBSTRING TMP$1381$2;
		FBSTRING TMP$1382$2;
		FBSTRING TMP$1383$2;
		FBSTRING TMP$1384$2;
		__builtin_memset( &TMP$1384$2, 0, 24ll );
		__builtin_memset( &TMP$1381$2, 0, 24ll );
		FBSTRING* vr$52 = fb_StrConcat( &TMP$1381$2, (void*)"rcpss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1382$2, 0, 24ll );
		FBSTRING* vr$55 = fb_StrConcat( &TMP$1382$2, (void*)vr$52, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1383$2, 0, 24ll );
		FBSTRING* vr$58 = fb_StrConcat( &TMP$1383$2, (void*)vr$55, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1384$2, -1ll, (void*)vr$58, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1384$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1384$2 );
	}
	label$659:;
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$662;
	{
		FBSTRING TMP$1385$2;
		FBSTRING TMP$1386$2;
		__builtin_memset( &TMP$1386$2, 0, 24ll );
		FBSTRING* vr$63 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1385$2, 0, 24ll );
		FBSTRING* vr$66 = fb_StrConcat( &TMP$1385$2, (void*)"add esp, ", 10ll, (void*)vr$63, -1ll );
		fb_StrAssign( (void*)&TMP$1386$2, -1ll, (void*)vr$66, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1386$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1386$2 );
	}
	label$662:;
	label$661:;
	label$653:;
}

static void _EMITLOG_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1387$1;
	FBSTRING TMP$1405$1;
	FBSTRING TMP$1406$1;
	label$663:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$665;
	TMP$1387$1 = 24ll;
	goto label$817;
	label$665:;
	TMP$1387$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$817:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1387$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$667;
	{
		FBSTRING TMP$1388$2;
		FBSTRING TMP$1389$2;
		__builtin_memset( &TMP$1389$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1388$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1388$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1389$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1389$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1389$2 );
		if( DDSIZE$1 <= 4ll) goto label$669;
		{
			FBSTRING TMP$1390$3;
			FBSTRING TMP$1391$3;
			__builtin_memset( &TMP$1391$3, 0, 24ll );
			__builtin_memset( &TMP$1390$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1390$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1391$3, -1ll, (void*)vr$16, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1391$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1391$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$668;
		label$669:;
		{
			FBSTRING TMP$1392$3;
			FBSTRING TMP$1393$3;
			__builtin_memset( &TMP$1393$3, 0, 24ll );
			__builtin_memset( &TMP$1392$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1392$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1393$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1393$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1393$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$668:;
	}
	label$667:;
	label$666:;
	OUTP( (char*)"fldln2" );
	OUTP( (char*)"fxch" );
	OUTP( (char*)"fyl2x" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$671;
	{
		FBSTRING TMP$1395$2;
		FBSTRING TMP$1396$2;
		__builtin_memset( &TMP$1396$2, 0, 24ll );
		FBSTRING* vr$27 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1395$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$1395$2, (void*)"sub esp, ", 10ll, (void*)vr$27, -1ll );
		fb_StrAssign( (void*)&TMP$1396$2, -1ll, (void*)vr$30, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1396$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1396$2 );
	}
	label$671:;
	label$670:;
	if( DDSIZE$1 <= 4ll) goto label$673;
	{
		FBSTRING TMP$1397$2;
		FBSTRING TMP$1398$2;
		FBSTRING TMP$1399$2;
		FBSTRING TMP$1400$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$1400$2, 0, 24ll );
		__builtin_memset( &TMP$1397$2, 0, 24ll );
		FBSTRING* vr$36 = fb_StrConcat( &TMP$1397$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1398$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$1398$2, (void*)vr$36, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1399$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$1399$2, (void*)vr$39, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1400$2, -1ll, (void*)vr$42, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1400$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1400$2 );
	}
	goto label$672;
	label$673:;
	{
		FBSTRING TMP$1401$2;
		FBSTRING TMP$1402$2;
		FBSTRING TMP$1403$2;
		FBSTRING TMP$1404$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1404$2, 0, 24ll );
		__builtin_memset( &TMP$1401$2, 0, 24ll );
		FBSTRING* vr$48 = fb_StrConcat( &TMP$1401$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1402$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$1402$2, (void*)vr$48, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1403$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$1403$2, (void*)vr$51, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1404$2, -1ll, (void*)vr$54, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1404$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1404$2 );
	}
	label$672:;
	__builtin_memset( &TMP$1406$1, 0, 24ll );
	FBSTRING* vr$58 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$1405$1, 0, 24ll );
	FBSTRING* vr$61 = fb_StrConcat( &TMP$1405$1, (void*)"add esp, ", 10ll, (void*)vr$58, -1ll );
	fb_StrAssign( (void*)&TMP$1406$1, -1ll, (void*)vr$61, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1406$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1406$1 );
	label$664:;
}

static void _EMITEXP_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1407$1;
	FBSTRING TMP$1430$1;
	FBSTRING TMP$1431$1;
	label$674:;
	static FBSTRING DST$1;
	static int64 DDSIZE$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$676;
	TMP$1407$1 = 24ll;
	goto label$818;
	label$676:;
	TMP$1407$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$818:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1407$1 * 56ll)) + 8ll);
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$678;
	{
		FBSTRING TMP$1408$2;
		FBSTRING TMP$1409$2;
		__builtin_memset( &TMP$1409$2, 0, 24ll );
		FBSTRING* vr$7 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1408$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$1408$2, (void*)"sub esp, ", 10ll, (void*)vr$7, -1ll );
		fb_StrAssign( (void*)&TMP$1409$2, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1409$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1409$2 );
		if( DDSIZE$1 <= 4ll) goto label$680;
		{
			FBSTRING TMP$1410$3;
			FBSTRING TMP$1411$3;
			__builtin_memset( &TMP$1411$3, 0, 24ll );
			__builtin_memset( &TMP$1410$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1410$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1411$3, -1ll, (void*)vr$16, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1411$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1411$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$679;
		label$680:;
		{
			FBSTRING TMP$1412$3;
			FBSTRING TMP$1413$3;
			__builtin_memset( &TMP$1413$3, 0, 24ll );
			__builtin_memset( &TMP$1412$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1412$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1413$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1413$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1413$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$679:;
	}
	label$678:;
	label$677:;
	OUTP( (char*)"fldl2e" );
	OUTP( (char*)"fmulp st(1), st" );
	OUTP( (char*)"fld st" );
	OUTP( (char*)"frndint" );
	OUTP( (char*)"fsub st(1), st" );
	OUTP( (char*)"fxch" );
	OUTP( (char*)"f2xm1" );
	HPUSH( (char*)"0x3f800000" );
	OUTP( (char*)"fadd dword ptr [esp]" );
	OUTP( (char*)"add esp, 4" );
	OUTP( (char*)"fscale" );
	OUTP( (char*)"fstp st(1)" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 0ll) goto label$682;
	{
		FBSTRING TMP$1420$2;
		FBSTRING TMP$1421$2;
		__builtin_memset( &TMP$1421$2, 0, 24ll );
		FBSTRING* vr$27 = fb_LongintToStr( DDSIZE$1 );
		__builtin_memset( &TMP$1420$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$1420$2, (void*)"sub esp, ", 10ll, (void*)vr$27, -1ll );
		fb_StrAssign( (void*)&TMP$1421$2, -1ll, (void*)vr$30, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1421$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1421$2 );
	}
	label$682:;
	label$681:;
	if( DDSIZE$1 <= 4ll) goto label$684;
	{
		FBSTRING TMP$1422$2;
		FBSTRING TMP$1423$2;
		FBSTRING TMP$1424$2;
		FBSTRING TMP$1425$2;
		OUTP( (char*)"fstp qword ptr [esp]" );
		__builtin_memset( &TMP$1425$2, 0, 24ll );
		__builtin_memset( &TMP$1422$2, 0, 24ll );
		FBSTRING* vr$36 = fb_StrConcat( &TMP$1422$2, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1423$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$1423$2, (void*)vr$36, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1424$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$1424$2, (void*)vr$39, -1ll, (void*)"qword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1425$2, -1ll, (void*)vr$42, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1425$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1425$2 );
	}
	goto label$683;
	label$684:;
	{
		FBSTRING TMP$1426$2;
		FBSTRING TMP$1427$2;
		FBSTRING TMP$1428$2;
		FBSTRING TMP$1429$2;
		OUTP( (char*)"fstp dword ptr [esp]" );
		__builtin_memset( &TMP$1429$2, 0, 24ll );
		__builtin_memset( &TMP$1426$2, 0, 24ll );
		FBSTRING* vr$48 = fb_StrConcat( &TMP$1426$2, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1427$2, 0, 24ll );
		FBSTRING* vr$51 = fb_StrConcat( &TMP$1427$2, (void*)vr$48, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1428$2, 0, 24ll );
		FBSTRING* vr$54 = fb_StrConcat( &TMP$1428$2, (void*)vr$51, -1ll, (void*)"dword ptr [esp]", 16ll );
		fb_StrAssign( (void*)&TMP$1429$2, -1ll, (void*)vr$54, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1429$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1429$2 );
	}
	label$683:;
	__builtin_memset( &TMP$1431$1, 0, 24ll );
	FBSTRING* vr$58 = fb_LongintToStr( DDSIZE$1 );
	__builtin_memset( &TMP$1430$1, 0, 24ll );
	FBSTRING* vr$61 = fb_StrConcat( &TMP$1430$1, (void*)"add esp, ", 10ll, (void*)vr$58, -1ll );
	fb_StrAssign( (void*)&TMP$1431$1, -1ll, (void*)vr$61, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1431$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1431$1 );
	label$675:;
}

static void _EMITFLOOR_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1432$1;
	FBSTRING TMP$1447$1;
	FBSTRING TMP$1448$1;
	FBSTRING TMP$1449$1;
	FBSTRING TMP$1451$1;
	FBSTRING TMP$1452$1;
	FBSTRING TMP$1453$1;
	FBSTRING TMP$1454$1;
	FBSTRING TMP$1455$1;
	FBSTRING TMP$1456$1;
	FBSTRING TMP$1457$1;
	FBSTRING TMP$1458$1;
	FBSTRING TMP$1459$1;
	FBSTRING TMP$1460$1;
	FBSTRING TMP$1462$1;
	FBSTRING TMP$1463$1;
	FBSTRING TMP$1464$1;
	FBSTRING TMP$1465$1;
	FBSTRING TMP$1466$1;
	FBSTRING TMP$1468$1;
	FBSTRING TMP$1469$1;
	FBSTRING TMP$1470$1;
	FBSTRING TMP$1471$1;
	FBSTRING TMP$1472$1;
	FBSTRING TMP$1473$1;
	FBSTRING TMP$1474$1;
	FBSTRING TMP$1475$1;
	FBSTRING TMP$1476$1;
	FBSTRING TMP$1477$1;
	label$685:;
	static FBSTRING DST$1;
	static FBSTRING NEG1$1;
	static FBSTRING SUFFIX$1;
	static int64 DDSIZE$1;
	static struct $8FBSYMBOL* SYM$1;
	static struct $6IRVREG* VREG$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$687;
	TMP$1432$1 = 24ll;
	goto label$819;
	label$687:;
	TMP$1432$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$819:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1432$1 * 56ll)) + 8ll);
	if( DDSIZE$1 <= 4ll) goto label$689;
	{
		struct $8FBSYMBOL* vr$5 = SYMBALLOCINTCONST( -4616189618054758400ll, 14ll );
		SYM$1 = vr$5;
		struct $6IRVREG* vr$7 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		VREG$1 = vr$7;
		fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)"d ", 3ll, 0 );
	}
	goto label$688;
	label$689:;
	{
		struct $8FBSYMBOL* vr$8 = SYMBALLOCINTCONST( 3212836864ll, 9ll );
		SYM$1 = vr$8;
		struct $6IRVREG* vr$10 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, SYM$1, *(int64*)((uint8*)SYM$1 + 88ll) );
		VREG$1 = vr$10;
		fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)"s ", 3ll, 0 );
	}
	label$688:;
	*(int64*)((uint8*)SYM$1 + 168ll) = 16ll;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( VREG$1, &NEG1$1, 25ll, 0ll, 0ll, -1ll );
	OUTP( (char*)"sub esp, 8" );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$691;
	{
		FBSTRING TMP$1438$2;
		FBSTRING TMP$1439$2;
		FBSTRING TMP$1440$2;
		FBSTRING TMP$1441$2;
		FBSTRING TMP$1442$2;
		if( DDSIZE$1 <= 4ll) goto label$693;
		{
			FBSTRING TMP$1433$3;
			FBSTRING TMP$1434$3;
			__builtin_memset( &TMP$1434$3, 0, 24ll );
			__builtin_memset( &TMP$1433$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$1433$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1434$3, -1ll, (void*)vr$16, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1434$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1434$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$692;
		label$693:;
		{
			FBSTRING TMP$1435$3;
			FBSTRING TMP$1436$3;
			__builtin_memset( &TMP$1436$3, 0, 24ll );
			__builtin_memset( &TMP$1435$3, 0, 24ll );
			FBSTRING* vr$22 = fb_StrConcat( &TMP$1435$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1436$3, -1ll, (void*)vr$22, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1436$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1436$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$692:;
		__builtin_memset( &TMP$1442$2, 0, 24ll );
		__builtin_memset( &TMP$1438$2, 0, 24ll );
		FBSTRING* vr$28 = fb_StrConcat( &TMP$1438$2, (void*)"movap", 6ll, (void*)&SUFFIX$1, -1ll );
		__builtin_memset( &TMP$1439$2, 0, 24ll );
		FBSTRING* vr$31 = fb_StrConcat( &TMP$1439$2, (void*)vr$28, -1ll, (void*)"xmm7", 5ll );
		__builtin_memset( &TMP$1440$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$1440$2, (void*)vr$31, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1441$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$1441$2, (void*)vr$34, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1442$2, -1ll, (void*)vr$37, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1442$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1442$2 );
	}
	goto label$690;
	label$691:;
	{
		if( DDSIZE$1 <= 4ll) goto label$695;
		{
			OUTP( (char*)"fst qword ptr [esp]" );
			OUTP( (char*)"movlpd xmm7, qword ptr [esp]" );
		}
		goto label$694;
		label$695:;
		{
			OUTP( (char*)"fst dword ptr [esp]" );
			OUTP( (char*)"movss xmm7, dword ptr [esp]" );
		}
		label$694:;
	}
	label$690:;
	OUTP( (char*)"fistp qword ptr [esp]" );
	OUTP( (char*)"fild qword ptr [esp]" );
	__builtin_memset( &TMP$1449$1, 0, 24ll );
	__builtin_memset( &TMP$1447$1, 0, 24ll );
	FBSTRING* vr$46 = fb_StrConcat( &TMP$1447$1, (void*)"fstp ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
	__builtin_memset( &TMP$1448$1, 0, 24ll );
	FBSTRING* vr$49 = fb_StrConcat( &TMP$1448$1, (void*)vr$46, -1ll, (void*)" [esp]", 7ll );
	fb_StrAssign( (void*)&TMP$1449$1, -1ll, (void*)vr$49, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1449$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1449$1 );
	__builtin_memset( &TMP$1455$1, 0, 24ll );
	__builtin_memset( &TMP$1451$1, 0, 24ll );
	FBSTRING* vr$55 = fb_StrConcat( &TMP$1451$1, (void*)"xorp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1452$1, 0, 24ll );
	FBSTRING* vr$58 = fb_StrConcat( &TMP$1452$1, (void*)vr$55, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1453$1, 0, 24ll );
	FBSTRING* vr$61 = fb_StrConcat( &TMP$1453$1, (void*)vr$58, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1454$1, 0, 24ll );
	FBSTRING* vr$64 = fb_StrConcat( &TMP$1454$1, (void*)vr$61, -1ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$1455$1, -1ll, (void*)vr$64, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1455$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1455$1 );
	__builtin_memset( &TMP$1460$1, 0, 24ll );
	__builtin_memset( &TMP$1456$1, 0, 24ll );
	FBSTRING* vr$70 = fb_StrConcat( &TMP$1456$1, (void*)"subs", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1457$1, 0, 24ll );
	FBSTRING* vr$73 = fb_StrConcat( &TMP$1457$1, (void*)vr$70, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1458$1, 0, 24ll );
	FBSTRING* vr$76 = fb_StrConcat( &TMP$1458$1, (void*)vr$73, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1459$1, 0, 24ll );
	FBSTRING* vr$79 = fb_StrConcat( &TMP$1459$1, (void*)vr$76, -1ll, (void*)"[esp]", 6ll );
	fb_StrAssign( (void*)&TMP$1460$1, -1ll, (void*)vr$79, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1460$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1460$1 );
	__builtin_memset( &TMP$1466$1, 0, 24ll );
	__builtin_memset( &TMP$1462$1, 0, 24ll );
	FBSTRING* vr$85 = fb_StrConcat( &TMP$1462$1, (void*)"cmpnles", 8ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1463$1, 0, 24ll );
	FBSTRING* vr$88 = fb_StrConcat( &TMP$1463$1, (void*)vr$85, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1464$1, 0, 24ll );
	FBSTRING* vr$91 = fb_StrConcat( &TMP$1464$1, (void*)vr$88, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1465$1, 0, 24ll );
	FBSTRING* vr$94 = fb_StrConcat( &TMP$1465$1, (void*)vr$91, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1466$1, -1ll, (void*)vr$94, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1466$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1466$1 );
	__builtin_memset( &TMP$1472$1, 0, 24ll );
	__builtin_memset( &TMP$1468$1, 0, 24ll );
	FBSTRING* vr$100 = fb_StrConcat( &TMP$1468$1, (void*)"andp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1469$1, 0, 24ll );
	FBSTRING* vr$103 = fb_StrConcat( &TMP$1469$1, (void*)vr$100, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1470$1, 0, 24ll );
	FBSTRING* vr$106 = fb_StrConcat( &TMP$1470$1, (void*)vr$103, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1471$1, 0, 24ll );
	FBSTRING* vr$109 = fb_StrConcat( &TMP$1471$1, (void*)vr$106, -1ll, (void*)&NEG1$1, -1ll );
	fb_StrAssign( (void*)&TMP$1472$1, -1ll, (void*)vr$109, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1472$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1472$1 );
	__builtin_memset( &TMP$1477$1, 0, 24ll );
	__builtin_memset( &TMP$1473$1, 0, 24ll );
	FBSTRING* vr$115 = fb_StrConcat( &TMP$1473$1, (void*)"adds", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1474$1, 0, 24ll );
	FBSTRING* vr$118 = fb_StrConcat( &TMP$1474$1, (void*)vr$115, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1475$1, 0, 24ll );
	FBSTRING* vr$121 = fb_StrConcat( &TMP$1475$1, (void*)vr$118, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1476$1, 0, 24ll );
	FBSTRING* vr$124 = fb_StrConcat( &TMP$1476$1, (void*)vr$121, -1ll, (void*)"[esp]", 6ll );
	fb_StrAssign( (void*)&TMP$1477$1, -1ll, (void*)vr$124, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1477$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1477$1 );
	OUTP( (char*)"add esp, 8" );
	label$686:;
}

static void _EMITFIX_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1478$1;
	FBSTRING TMP$1479$1;
	FBSTRING TMP$1480$1;
	FBSTRING TMP$1494$1;
	FBSTRING TMP$1495$1;
	FBSTRING TMP$1496$1;
	FBSTRING TMP$1499$1;
	FBSTRING TMP$1500$1;
	FBSTRING TMP$1501$1;
	FBSTRING TMP$1502$1;
	FBSTRING TMP$1503$1;
	FBSTRING TMP$1504$1;
	FBSTRING TMP$1505$1;
	FBSTRING TMP$1506$1;
	FBSTRING TMP$1507$1;
	FBSTRING TMP$1508$1;
	FBSTRING TMP$1509$1;
	FBSTRING TMP$1510$1;
	FBSTRING TMP$1511$1;
	FBSTRING TMP$1512$1;
	FBSTRING TMP$1513$1;
	FBSTRING TMP$1514$1;
	FBSTRING TMP$1515$1;
	FBSTRING TMP$1516$1;
	FBSTRING TMP$1517$1;
	FBSTRING TMP$1518$1;
	FBSTRING TMP$1519$1;
	FBSTRING TMP$1520$1;
	FBSTRING TMP$1521$1;
	FBSTRING TMP$1522$1;
	FBSTRING TMP$1523$1;
	FBSTRING TMP$1524$1;
	FBSTRING TMP$1525$1;
	FBSTRING TMP$1526$1;
	FBSTRING TMP$1529$1;
	FBSTRING TMP$1530$1;
	FBSTRING TMP$1531$1;
	FBSTRING TMP$1532$1;
	FBSTRING TMP$1533$1;
	FBSTRING TMP$1534$1;
	FBSTRING TMP$1535$1;
	FBSTRING TMP$1536$1;
	FBSTRING TMP$1537$1;
	FBSTRING TMP$1538$1;
	FBSTRING TMP$1539$1;
	FBSTRING TMP$1540$1;
	FBSTRING TMP$1541$1;
	FBSTRING TMP$1542$1;
	FBSTRING TMP$1543$1;
	FBSTRING TMP$1544$1;
	FBSTRING TMP$1545$1;
	label$696:;
	static FBSTRING DST$1;
	static FBSTRING SUFFIX$1;
	static FBSTRING ABSVAL$1;
	static FBSTRING NEG1$1;
	static int64 DDSIZE$1;
	static struct $8FBSYMBOL* NEG1_SYM$1;
	static struct $8FBSYMBOL* ABSVAL_SYM$1;
	static struct $6IRVREG* NEG1_VREG$1;
	static struct $6IRVREG* ABSVAL_VREG$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$698;
	TMP$1478$1 = 24ll;
	goto label$820;
	label$698:;
	TMP$1478$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$820:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1478$1 * 56ll)) + 8ll);
	if( DDSIZE$1 <= 4ll) goto label$700;
	{
		struct $8FBSYMBOL* vr$5 = SYMBALLOCINTCONST( -4616189618054758400ll, 14ll );
		NEG1_SYM$1 = vr$5;
		struct $6IRVREG* vr$7 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, NEG1_SYM$1, *(int64*)((uint8*)NEG1_SYM$1 + 88ll) );
		NEG1_VREG$1 = vr$7;
		struct $8FBSYMBOL* vr$8 = SYMBALLOCINTCONST( (int64)-9223372036854775808ull, 14ll );
		ABSVAL_SYM$1 = vr$8;
		struct $6IRVREG* vr$10 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, ABSVAL_SYM$1, *(int64*)((uint8*)ABSVAL_SYM$1 + 88ll) );
		ABSVAL_VREG$1 = vr$10;
		fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)"d ", 3ll, 0 );
	}
	goto label$699;
	label$700:;
	{
		struct $8FBSYMBOL* vr$11 = SYMBALLOCINTCONST( 3212836864ll, 9ll );
		NEG1_SYM$1 = vr$11;
		struct $6IRVREG* vr$13 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, NEG1_SYM$1, *(int64*)((uint8*)NEG1_SYM$1 + 88ll) );
		NEG1_VREG$1 = vr$13;
		struct $8FBSYMBOL* vr$14 = SYMBALLOCINTCONST( 2147483648ll, 9ll );
		ABSVAL_SYM$1 = vr$14;
		struct $6IRVREG* vr$16 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, ABSVAL_SYM$1, *(int64*)((uint8*)ABSVAL_SYM$1 + 88ll) );
		ABSVAL_VREG$1 = vr$16;
		fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)"s ", 3ll, 0 );
	}
	label$699:;
	*(int64*)((uint8*)NEG1_SYM$1 + 168ll) = 16ll;
	*(int64*)((uint8*)ABSVAL_SYM$1 + 168ll) = 16ll;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( NEG1_VREG$1, &NEG1$1, 25ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( ABSVAL_VREG$1, &ABSVAL$1, 25ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1480$1, 0, 24ll );
	FBSTRING* vr$21 = fb_LongintToStr( DDSIZE$1 + 8ll );
	__builtin_memset( &TMP$1479$1, 0, 24ll );
	FBSTRING* vr$24 = fb_StrConcat( &TMP$1479$1, (void*)"sub esp, ", 10ll, (void*)vr$21, -1ll );
	fb_StrAssign( (void*)&TMP$1480$1, -1ll, (void*)vr$24, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1480$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1480$1 );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$702;
	{
		if( DDSIZE$1 <= 4ll) goto label$704;
		{
			FBSTRING TMP$1481$3;
			FBSTRING TMP$1482$3;
			__builtin_memset( &TMP$1482$3, 0, 24ll );
			__builtin_memset( &TMP$1481$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$1481$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1482$3, -1ll, (void*)vr$31, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1482$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1482$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$703;
		label$704:;
		{
			FBSTRING TMP$1483$3;
			FBSTRING TMP$1484$3;
			__builtin_memset( &TMP$1484$3, 0, 24ll );
			__builtin_memset( &TMP$1483$3, 0, 24ll );
			FBSTRING* vr$37 = fb_StrConcat( &TMP$1483$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1484$3, -1ll, (void*)vr$37, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1484$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1484$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$703:;
	}
	goto label$701;
	label$702:;
	{
		if( DDSIZE$1 <= 4ll) goto label$706;
		{
			FBSTRING TMP$1485$3;
			FBSTRING TMP$1486$3;
			FBSTRING TMP$1487$3;
			FBSTRING TMP$1488$3;
			OUTP( (char*)"fst qword ptr [esp]" );
			__builtin_memset( &TMP$1488$3, 0, 24ll );
			__builtin_memset( &TMP$1485$3, 0, 24ll );
			FBSTRING* vr$43 = fb_StrConcat( &TMP$1485$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1486$3, 0, 24ll );
			FBSTRING* vr$46 = fb_StrConcat( &TMP$1486$3, (void*)vr$43, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1487$3, 0, 24ll );
			FBSTRING* vr$49 = fb_StrConcat( &TMP$1487$3, (void*)vr$46, -1ll, (void*)"qword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1488$3, -1ll, (void*)vr$49, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1488$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1488$3 );
		}
		goto label$705;
		label$706:;
		{
			FBSTRING TMP$1489$3;
			FBSTRING TMP$1490$3;
			FBSTRING TMP$1491$3;
			FBSTRING TMP$1492$3;
			OUTP( (char*)"fst dword ptr [esp]" );
			__builtin_memset( &TMP$1492$3, 0, 24ll );
			__builtin_memset( &TMP$1489$3, 0, 24ll );
			FBSTRING* vr$55 = fb_StrConcat( &TMP$1489$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1490$3, 0, 24ll );
			FBSTRING* vr$58 = fb_StrConcat( &TMP$1490$3, (void*)vr$55, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1491$3, 0, 24ll );
			FBSTRING* vr$61 = fb_StrConcat( &TMP$1491$3, (void*)vr$58, -1ll, (void*)"dword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1492$3, -1ll, (void*)vr$61, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1492$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1492$3 );
		}
		label$705:;
	}
	label$701:;
	__builtin_memset( &TMP$1496$1, 0, 24ll );
	__builtin_memset( &TMP$1494$1, 0, 24ll );
	FBSTRING* vr$67 = fb_StrConcat( &TMP$1494$1, (void*)"xorp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1495$1, 0, 24ll );
	FBSTRING* vr$70 = fb_StrConcat( &TMP$1495$1, (void*)vr$67, -1ll, (void*)"xmm7, xmm7", 11ll );
	fb_StrAssign( (void*)&TMP$1496$1, -1ll, (void*)vr$70, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1496$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1496$1 );
	if( DDSIZE$1 <= 4ll) goto label$708;
	{
		OUTP( (char*)"movlpd [esp+8], xmm7" );
	}
	goto label$707;
	label$708:;
	{
		OUTP( (char*)"movss [esp+8], xmm7" );
	}
	label$707:;
	OUTP( (char*)"fistp qword ptr [esp]" );
	__builtin_memset( &TMP$1503$1, 0, 24ll );
	__builtin_memset( &TMP$1499$1, 0, 24ll );
	FBSTRING* vr$76 = fb_StrConcat( &TMP$1499$1, (void*)"cmpnles", 8ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1500$1, 0, 24ll );
	FBSTRING* vr$79 = fb_StrConcat( &TMP$1500$1, (void*)vr$76, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1501$1, 0, 24ll );
	FBSTRING* vr$82 = fb_StrConcat( &TMP$1501$1, (void*)vr$79, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1502$1, 0, 24ll );
	FBSTRING* vr$85 = fb_StrConcat( &TMP$1502$1, (void*)vr$82, -1ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$1503$1, -1ll, (void*)vr$85, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1503$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1503$1 );
	OUTP( (char*)"fild qword ptr [esp]" );
	__builtin_memset( &TMP$1508$1, 0, 24ll );
	__builtin_memset( &TMP$1504$1, 0, 24ll );
	FBSTRING* vr$91 = fb_StrConcat( &TMP$1504$1, (void*)"andp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1505$1, 0, 24ll );
	FBSTRING* vr$94 = fb_StrConcat( &TMP$1505$1, (void*)vr$91, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1506$1, 0, 24ll );
	FBSTRING* vr$97 = fb_StrConcat( &TMP$1506$1, (void*)vr$94, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1507$1, 0, 24ll );
	FBSTRING* vr$100 = fb_StrConcat( &TMP$1507$1, (void*)vr$97, -1ll, (void*)&ABSVAL$1, -1ll );
	fb_StrAssign( (void*)&TMP$1508$1, -1ll, (void*)vr$100, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1508$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1508$1 );
	__builtin_memset( &TMP$1511$1, 0, 24ll );
	__builtin_memset( &TMP$1509$1, 0, 24ll );
	FBSTRING* vr$109 = fb_StrConcat( &TMP$1509$1, (void*)"fstp ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
	__builtin_memset( &TMP$1510$1, 0, 24ll );
	FBSTRING* vr$112 = fb_StrConcat( &TMP$1510$1, (void*)vr$109, -1ll, (void*)" [esp]", 7ll );
	fb_StrAssign( (void*)&TMP$1511$1, -1ll, (void*)vr$112, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1511$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1511$1 );
	__builtin_memset( &TMP$1516$1, 0, 24ll );
	__builtin_memset( &TMP$1512$1, 0, 24ll );
	FBSTRING* vr$118 = fb_StrConcat( &TMP$1512$1, (void*)"subs", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1513$1, 0, 24ll );
	FBSTRING* vr$121 = fb_StrConcat( &TMP$1513$1, (void*)vr$118, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1514$1, 0, 24ll );
	FBSTRING* vr$124 = fb_StrConcat( &TMP$1514$1, (void*)vr$121, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1515$1, 0, 24ll );
	FBSTRING* vr$127 = fb_StrConcat( &TMP$1515$1, (void*)vr$124, -1ll, (void*)"[esp]", 6ll );
	fb_StrAssign( (void*)&TMP$1516$1, -1ll, (void*)vr$127, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1516$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1516$1 );
	__builtin_memset( &TMP$1521$1, 0, 24ll );
	__builtin_memset( &TMP$1517$1, 0, 24ll );
	FBSTRING* vr$133 = fb_StrConcat( &TMP$1517$1, (void*)"xorp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1518$1, 0, 24ll );
	FBSTRING* vr$136 = fb_StrConcat( &TMP$1518$1, (void*)vr$133, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1519$1, 0, 24ll );
	FBSTRING* vr$139 = fb_StrConcat( &TMP$1519$1, (void*)vr$136, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1520$1, 0, 24ll );
	FBSTRING* vr$142 = fb_StrConcat( &TMP$1520$1, (void*)vr$139, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1521$1, -1ll, (void*)vr$142, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1521$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1521$1 );
	__builtin_memset( &TMP$1526$1, 0, 24ll );
	__builtin_memset( &TMP$1522$1, 0, 24ll );
	FBSTRING* vr$148 = fb_StrConcat( &TMP$1522$1, (void*)"xorp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1523$1, 0, 24ll );
	FBSTRING* vr$151 = fb_StrConcat( &TMP$1523$1, (void*)vr$148, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1524$1, 0, 24ll );
	FBSTRING* vr$154 = fb_StrConcat( &TMP$1524$1, (void*)vr$151, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1525$1, 0, 24ll );
	FBSTRING* vr$157 = fb_StrConcat( &TMP$1525$1, (void*)vr$154, -1ll, (void*)&NEG1$1, -1ll );
	fb_StrAssign( (void*)&TMP$1526$1, -1ll, (void*)vr$157, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1526$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1526$1 );
	__builtin_memset( &TMP$1533$1, 0, 24ll );
	__builtin_memset( &TMP$1529$1, 0, 24ll );
	FBSTRING* vr$163 = fb_StrConcat( &TMP$1529$1, (void*)"cmplts", 7ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1530$1, 0, 24ll );
	FBSTRING* vr$166 = fb_StrConcat( &TMP$1530$1, (void*)vr$163, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1531$1, 0, 24ll );
	FBSTRING* vr$169 = fb_StrConcat( &TMP$1531$1, (void*)vr$166, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1532$1, 0, 24ll );
	FBSTRING* vr$172 = fb_StrConcat( &TMP$1532$1, (void*)vr$169, -1ll, (void*)"[esp+8]", 8ll );
	fb_StrAssign( (void*)&TMP$1533$1, -1ll, (void*)vr$172, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1533$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1533$1 );
	__builtin_memset( &TMP$1538$1, 0, 24ll );
	__builtin_memset( &TMP$1534$1, 0, 24ll );
	FBSTRING* vr$178 = fb_StrConcat( &TMP$1534$1, (void*)"andp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1535$1, 0, 24ll );
	FBSTRING* vr$181 = fb_StrConcat( &TMP$1535$1, (void*)vr$178, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1536$1, 0, 24ll );
	FBSTRING* vr$184 = fb_StrConcat( &TMP$1536$1, (void*)vr$181, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1537$1, 0, 24ll );
	FBSTRING* vr$187 = fb_StrConcat( &TMP$1537$1, (void*)vr$184, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1538$1, -1ll, (void*)vr$187, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1538$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1538$1 );
	__builtin_memset( &TMP$1543$1, 0, 24ll );
	__builtin_memset( &TMP$1539$1, 0, 24ll );
	FBSTRING* vr$193 = fb_StrConcat( &TMP$1539$1, (void*)"adds", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1540$1, 0, 24ll );
	FBSTRING* vr$196 = fb_StrConcat( &TMP$1540$1, (void*)vr$193, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1541$1, 0, 24ll );
	FBSTRING* vr$199 = fb_StrConcat( &TMP$1541$1, (void*)vr$196, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1542$1, 0, 24ll );
	FBSTRING* vr$202 = fb_StrConcat( &TMP$1542$1, (void*)vr$199, -1ll, (void*)"[esp]", 6ll );
	fb_StrAssign( (void*)&TMP$1543$1, -1ll, (void*)vr$202, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1543$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1543$1 );
	__builtin_memset( &TMP$1545$1, 0, 24ll );
	FBSTRING* vr$207 = fb_LongintToStr( DDSIZE$1 + 8ll );
	__builtin_memset( &TMP$1544$1, 0, 24ll );
	FBSTRING* vr$210 = fb_StrConcat( &TMP$1544$1, (void*)"add esp, ", 10ll, (void*)vr$207, -1ll );
	fb_StrAssign( (void*)&TMP$1545$1, -1ll, (void*)vr$210, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1545$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1545$1 );
	label$697:;
}

static void _EMITFRAC_SSE( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$1546$1;
	FBSTRING TMP$1547$1;
	FBSTRING TMP$1548$1;
	FBSTRING TMP$1561$1;
	FBSTRING TMP$1562$1;
	FBSTRING TMP$1563$1;
	FBSTRING TMP$1576$1;
	FBSTRING TMP$1577$1;
	FBSTRING TMP$1578$1;
	FBSTRING TMP$1579$1;
	FBSTRING TMP$1580$1;
	FBSTRING TMP$1581$1;
	FBSTRING TMP$1582$1;
	FBSTRING TMP$1583$1;
	FBSTRING TMP$1584$1;
	FBSTRING TMP$1585$1;
	FBSTRING TMP$1586$1;
	FBSTRING TMP$1587$1;
	FBSTRING TMP$1588$1;
	FBSTRING TMP$1589$1;
	FBSTRING TMP$1590$1;
	FBSTRING TMP$1591$1;
	FBSTRING TMP$1592$1;
	FBSTRING TMP$1593$1;
	FBSTRING TMP$1594$1;
	FBSTRING TMP$1595$1;
	FBSTRING TMP$1596$1;
	FBSTRING TMP$1597$1;
	FBSTRING TMP$1598$1;
	FBSTRING TMP$1599$1;
	FBSTRING TMP$1600$1;
	FBSTRING TMP$1601$1;
	FBSTRING TMP$1602$1;
	FBSTRING TMP$1603$1;
	FBSTRING TMP$1604$1;
	FBSTRING TMP$1605$1;
	FBSTRING TMP$1606$1;
	FBSTRING TMP$1607$1;
	FBSTRING TMP$1608$1;
	FBSTRING TMP$1609$1;
	FBSTRING TMP$1610$1;
	FBSTRING TMP$1611$1;
	FBSTRING TMP$1612$1;
	FBSTRING TMP$1613$1;
	FBSTRING TMP$1625$1;
	FBSTRING TMP$1626$1;
	FBSTRING TMP$1627$1;
	FBSTRING TMP$1628$1;
	FBSTRING TMP$1629$1;
	FBSTRING TMP$1630$1;
	FBSTRING TMP$1631$1;
	FBSTRING TMP$1632$1;
	FBSTRING TMP$1633$1;
	FBSTRING TMP$1634$1;
	FBSTRING TMP$1635$1;
	FBSTRING TMP$1636$1;
	label$709:;
	static FBSTRING DST$1;
	static FBSTRING SUFFIX$1;
	static FBSTRING ABSVAL$1;
	static FBSTRING NEG1$1;
	static int64 DDSIZE$1;
	static struct $8FBSYMBOL* NEG1_SYM$1;
	static struct $8FBSYMBOL* ABSVAL_SYM$1;
	static struct $6IRVREG* NEG1_VREG$1;
	static struct $6IRVREG* ABSVAL_VREG$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$711;
	TMP$1546$1 = 24ll;
	goto label$821;
	label$711:;
	TMP$1546$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$821:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1546$1 * 56ll)) + 8ll);
	if( DDSIZE$1 <= 4ll) goto label$713;
	{
		struct $8FBSYMBOL* vr$5 = SYMBALLOCINTCONST( -4616189618054758400ll, 14ll );
		NEG1_SYM$1 = vr$5;
		struct $6IRVREG* vr$7 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, NEG1_SYM$1, *(int64*)((uint8*)NEG1_SYM$1 + 88ll) );
		NEG1_VREG$1 = vr$7;
		struct $8FBSYMBOL* vr$8 = SYMBALLOCINTCONST( (int64)-9223372036854775808ull, 14ll );
		ABSVAL_SYM$1 = vr$8;
		struct $6IRVREG* vr$10 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 14ll, (struct $8FBSYMBOL*)0ull, ABSVAL_SYM$1, *(int64*)((uint8*)ABSVAL_SYM$1 + 88ll) );
		ABSVAL_VREG$1 = vr$10;
		fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)"d ", 3ll, 0 );
	}
	goto label$712;
	label$713:;
	{
		struct $8FBSYMBOL* vr$11 = SYMBALLOCINTCONST( 3212836864ll, 9ll );
		NEG1_SYM$1 = vr$11;
		struct $6IRVREG* vr$13 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, NEG1_SYM$1, *(int64*)((uint8*)NEG1_SYM$1 + 88ll) );
		NEG1_VREG$1 = vr$13;
		struct $8FBSYMBOL* vr$14 = SYMBALLOCINTCONST( 2147483648ll, 9ll );
		ABSVAL_SYM$1 = vr$14;
		struct $6IRVREG* vr$16 = (*(tmp$73*)((uint8*)&IR$ + 472ll))( 9ll, (struct $8FBSYMBOL*)0ull, ABSVAL_SYM$1, *(int64*)((uint8*)ABSVAL_SYM$1 + 88ll) );
		ABSVAL_VREG$1 = vr$16;
		fb_StrAssign( (void*)&SUFFIX$1, -1ll, (void*)"s ", 3ll, 0 );
	}
	label$712:;
	*(int64*)((uint8*)NEG1_SYM$1 + 168ll) = 16ll;
	*(int64*)((uint8*)ABSVAL_SYM$1 + 168ll) = 16ll;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( NEG1_VREG$1, &NEG1$1, 25ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( ABSVAL_VREG$1, &ABSVAL$1, 25ll, 0ll, 0ll, -1ll );
	__builtin_memset( &TMP$1548$1, 0, 24ll );
	FBSTRING* vr$21 = fb_LongintToStr( DDSIZE$1 + 8ll );
	__builtin_memset( &TMP$1547$1, 0, 24ll );
	FBSTRING* vr$24 = fb_StrConcat( &TMP$1547$1, (void*)"sub esp, ", 10ll, (void*)vr$21, -1ll );
	fb_StrAssign( (void*)&TMP$1548$1, -1ll, (void*)vr$24, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1548$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1548$1 );
	if( *(int64*)((uint8*)DVREG$1 + 32ll) != 1ll) goto label$715;
	{
		if( DDSIZE$1 <= 4ll) goto label$717;
		{
			FBSTRING TMP$1549$3;
			FBSTRING TMP$1550$3;
			__builtin_memset( &TMP$1550$3, 0, 24ll );
			__builtin_memset( &TMP$1549$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$1549$3, (void*)"movlpd qword ptr [esp], ", 25ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1550$3, -1ll, (void*)vr$31, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1550$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1550$3 );
			OUTP( (char*)"fld qword ptr [esp]" );
		}
		goto label$716;
		label$717:;
		{
			FBSTRING TMP$1551$3;
			FBSTRING TMP$1552$3;
			__builtin_memset( &TMP$1552$3, 0, 24ll );
			__builtin_memset( &TMP$1551$3, 0, 24ll );
			FBSTRING* vr$37 = fb_StrConcat( &TMP$1551$3, (void*)"movss dword ptr [esp], ", 24ll, (void*)&DST$1, -1ll );
			fb_StrAssign( (void*)&TMP$1552$3, -1ll, (void*)vr$37, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1552$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1552$3 );
			OUTP( (char*)"fld dword ptr [esp]" );
		}
		label$716:;
	}
	goto label$714;
	label$715:;
	{
		if( DDSIZE$1 <= 4ll) goto label$719;
		{
			FBSTRING TMP$1553$3;
			FBSTRING TMP$1554$3;
			FBSTRING TMP$1555$3;
			FBSTRING TMP$1556$3;
			OUTP( (char*)"fst qword ptr [esp]" );
			__builtin_memset( &TMP$1556$3, 0, 24ll );
			__builtin_memset( &TMP$1553$3, 0, 24ll );
			FBSTRING* vr$43 = fb_StrConcat( &TMP$1553$3, (void*)"movlpd ", 8ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1554$3, 0, 24ll );
			FBSTRING* vr$46 = fb_StrConcat( &TMP$1554$3, (void*)vr$43, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1555$3, 0, 24ll );
			FBSTRING* vr$49 = fb_StrConcat( &TMP$1555$3, (void*)vr$46, -1ll, (void*)"qword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1556$3, -1ll, (void*)vr$49, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1556$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1556$3 );
		}
		goto label$718;
		label$719:;
		{
			FBSTRING TMP$1557$3;
			FBSTRING TMP$1558$3;
			FBSTRING TMP$1559$3;
			FBSTRING TMP$1560$3;
			OUTP( (char*)"fst dword ptr [esp]" );
			__builtin_memset( &TMP$1560$3, 0, 24ll );
			__builtin_memset( &TMP$1557$3, 0, 24ll );
			FBSTRING* vr$55 = fb_StrConcat( &TMP$1557$3, (void*)"movss ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$1558$3, 0, 24ll );
			FBSTRING* vr$58 = fb_StrConcat( &TMP$1558$3, (void*)vr$55, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$1559$3, 0, 24ll );
			FBSTRING* vr$61 = fb_StrConcat( &TMP$1559$3, (void*)vr$58, -1ll, (void*)"dword ptr [esp]", 16ll );
			fb_StrAssign( (void*)&TMP$1560$3, -1ll, (void*)vr$61, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$1560$3 );
			fb_StrDelete( (FBSTRING*)&TMP$1560$3 );
		}
		label$718:;
	}
	label$714:;
	__builtin_memset( &TMP$1563$1, 0, 24ll );
	__builtin_memset( &TMP$1561$1, 0, 24ll );
	FBSTRING* vr$67 = fb_StrConcat( &TMP$1561$1, (void*)"xorp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1562$1, 0, 24ll );
	FBSTRING* vr$70 = fb_StrConcat( &TMP$1562$1, (void*)vr$67, -1ll, (void*)"xmm7, xmm7", 11ll );
	fb_StrAssign( (void*)&TMP$1563$1, -1ll, (void*)vr$70, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1563$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1563$1 );
	if( DDSIZE$1 <= 4ll) goto label$721;
	{
		FBSTRING TMP$1565$2;
		FBSTRING TMP$1566$2;
		FBSTRING TMP$1567$2;
		FBSTRING TMP$1568$2;
		FBSTRING TMP$1569$2;
		FBSTRING TMP$1570$2;
		__builtin_memset( &TMP$1570$2, 0, 24ll );
		__builtin_memset( &TMP$1565$2, 0, 24ll );
		FBSTRING* vr$76 = fb_StrConcat( &TMP$1565$2, (void*)"shufpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1566$2, 0, 24ll );
		FBSTRING* vr$79 = fb_StrConcat( &TMP$1566$2, (void*)vr$76, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1567$2, 0, 24ll );
		FBSTRING* vr$82 = fb_StrConcat( &TMP$1567$2, (void*)vr$79, -1ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1568$2, 0, 24ll );
		FBSTRING* vr$85 = fb_StrConcat( &TMP$1568$2, (void*)vr$82, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1569$2, 0, 24ll );
		FBSTRING* vr$88 = fb_StrConcat( &TMP$1569$2, (void*)vr$85, -1ll, (void*)"0", 2ll );
		fb_StrAssign( (void*)&TMP$1570$2, -1ll, (void*)vr$88, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1570$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1570$2 );
		OUTP( (char*)"movlpd [esp+8], xmm7" );
	}
	goto label$720;
	label$721:;
	{
		FBSTRING TMP$1572$2;
		FBSTRING TMP$1573$2;
		FBSTRING TMP$1574$2;
		FBSTRING TMP$1575$2;
		__builtin_memset( &TMP$1575$2, 0, 24ll );
		__builtin_memset( &TMP$1572$2, 0, 24ll );
		FBSTRING* vr$94 = fb_StrConcat( &TMP$1572$2, (void*)"movlhps ", 9ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1573$2, 0, 24ll );
		FBSTRING* vr$97 = fb_StrConcat( &TMP$1573$2, (void*)vr$94, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1574$2, 0, 24ll );
		FBSTRING* vr$100 = fb_StrConcat( &TMP$1574$2, (void*)vr$97, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1575$2, -1ll, (void*)vr$100, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1575$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1575$2 );
		OUTP( (char*)"movss [esp+8], xmm7" );
	}
	label$720:;
	OUTP( (char*)"fistp qword ptr [esp]" );
	__builtin_memset( &TMP$1580$1, 0, 24ll );
	__builtin_memset( &TMP$1576$1, 0, 24ll );
	FBSTRING* vr$106 = fb_StrConcat( &TMP$1576$1, (void*)"cmpnles", 8ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1577$1, 0, 24ll );
	FBSTRING* vr$109 = fb_StrConcat( &TMP$1577$1, (void*)vr$106, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1578$1, 0, 24ll );
	FBSTRING* vr$112 = fb_StrConcat( &TMP$1578$1, (void*)vr$109, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1579$1, 0, 24ll );
	FBSTRING* vr$115 = fb_StrConcat( &TMP$1579$1, (void*)vr$112, -1ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$1580$1, -1ll, (void*)vr$115, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1580$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1580$1 );
	OUTP( (char*)"fild qword ptr [esp]" );
	__builtin_memset( &TMP$1585$1, 0, 24ll );
	__builtin_memset( &TMP$1581$1, 0, 24ll );
	FBSTRING* vr$121 = fb_StrConcat( &TMP$1581$1, (void*)"andp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1582$1, 0, 24ll );
	FBSTRING* vr$124 = fb_StrConcat( &TMP$1582$1, (void*)vr$121, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1583$1, 0, 24ll );
	FBSTRING* vr$127 = fb_StrConcat( &TMP$1583$1, (void*)vr$124, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1584$1, 0, 24ll );
	FBSTRING* vr$130 = fb_StrConcat( &TMP$1584$1, (void*)vr$127, -1ll, (void*)&ABSVAL$1, -1ll );
	fb_StrAssign( (void*)&TMP$1585$1, -1ll, (void*)vr$130, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1585$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1585$1 );
	__builtin_memset( &TMP$1588$1, 0, 24ll );
	__builtin_memset( &TMP$1586$1, 0, 24ll );
	FBSTRING* vr$139 = fb_StrConcat( &TMP$1586$1, (void*)"fstp ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
	__builtin_memset( &TMP$1587$1, 0, 24ll );
	FBSTRING* vr$142 = fb_StrConcat( &TMP$1587$1, (void*)vr$139, -1ll, (void*)" [esp]", 7ll );
	fb_StrAssign( (void*)&TMP$1588$1, -1ll, (void*)vr$142, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1588$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1588$1 );
	__builtin_memset( &TMP$1593$1, 0, 24ll );
	__builtin_memset( &TMP$1589$1, 0, 24ll );
	FBSTRING* vr$148 = fb_StrConcat( &TMP$1589$1, (void*)"subs", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1590$1, 0, 24ll );
	FBSTRING* vr$151 = fb_StrConcat( &TMP$1590$1, (void*)vr$148, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1591$1, 0, 24ll );
	FBSTRING* vr$154 = fb_StrConcat( &TMP$1591$1, (void*)vr$151, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1592$1, 0, 24ll );
	FBSTRING* vr$157 = fb_StrConcat( &TMP$1592$1, (void*)vr$154, -1ll, (void*)"[esp]", 6ll );
	fb_StrAssign( (void*)&TMP$1593$1, -1ll, (void*)vr$157, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1593$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1593$1 );
	__builtin_memset( &TMP$1598$1, 0, 24ll );
	__builtin_memset( &TMP$1594$1, 0, 24ll );
	FBSTRING* vr$163 = fb_StrConcat( &TMP$1594$1, (void*)"xorp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1595$1, 0, 24ll );
	FBSTRING* vr$166 = fb_StrConcat( &TMP$1595$1, (void*)vr$163, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1596$1, 0, 24ll );
	FBSTRING* vr$169 = fb_StrConcat( &TMP$1596$1, (void*)vr$166, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1597$1, 0, 24ll );
	FBSTRING* vr$172 = fb_StrConcat( &TMP$1597$1, (void*)vr$169, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1598$1, -1ll, (void*)vr$172, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1598$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1598$1 );
	__builtin_memset( &TMP$1603$1, 0, 24ll );
	__builtin_memset( &TMP$1599$1, 0, 24ll );
	FBSTRING* vr$178 = fb_StrConcat( &TMP$1599$1, (void*)"xorp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1600$1, 0, 24ll );
	FBSTRING* vr$181 = fb_StrConcat( &TMP$1600$1, (void*)vr$178, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1601$1, 0, 24ll );
	FBSTRING* vr$184 = fb_StrConcat( &TMP$1601$1, (void*)vr$181, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1602$1, 0, 24ll );
	FBSTRING* vr$187 = fb_StrConcat( &TMP$1602$1, (void*)vr$184, -1ll, (void*)&NEG1$1, -1ll );
	fb_StrAssign( (void*)&TMP$1603$1, -1ll, (void*)vr$187, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1603$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1603$1 );
	__builtin_memset( &TMP$1608$1, 0, 24ll );
	__builtin_memset( &TMP$1604$1, 0, 24ll );
	FBSTRING* vr$193 = fb_StrConcat( &TMP$1604$1, (void*)"cmplts", 7ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1605$1, 0, 24ll );
	FBSTRING* vr$196 = fb_StrConcat( &TMP$1605$1, (void*)vr$193, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1606$1, 0, 24ll );
	FBSTRING* vr$199 = fb_StrConcat( &TMP$1606$1, (void*)vr$196, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1607$1, 0, 24ll );
	FBSTRING* vr$202 = fb_StrConcat( &TMP$1607$1, (void*)vr$199, -1ll, (void*)"[esp+8]", 8ll );
	fb_StrAssign( (void*)&TMP$1608$1, -1ll, (void*)vr$202, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1608$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1608$1 );
	__builtin_memset( &TMP$1613$1, 0, 24ll );
	__builtin_memset( &TMP$1609$1, 0, 24ll );
	FBSTRING* vr$208 = fb_StrConcat( &TMP$1609$1, (void*)"andp", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1610$1, 0, 24ll );
	FBSTRING* vr$211 = fb_StrConcat( &TMP$1610$1, (void*)vr$208, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1611$1, 0, 24ll );
	FBSTRING* vr$214 = fb_StrConcat( &TMP$1611$1, (void*)vr$211, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1612$1, 0, 24ll );
	FBSTRING* vr$217 = fb_StrConcat( &TMP$1612$1, (void*)vr$214, -1ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$1613$1, -1ll, (void*)vr$217, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1613$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1613$1 );
	if( DDSIZE$1 <= 4ll) goto label$723;
	{
		FBSTRING TMP$1614$2;
		FBSTRING TMP$1615$2;
		FBSTRING TMP$1616$2;
		FBSTRING TMP$1617$2;
		FBSTRING TMP$1618$2;
		FBSTRING TMP$1619$2;
		__builtin_memset( &TMP$1619$2, 0, 24ll );
		__builtin_memset( &TMP$1614$2, 0, 24ll );
		FBSTRING* vr$223 = fb_StrConcat( &TMP$1614$2, (void*)"shufpd ", 8ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1615$2, 0, 24ll );
		FBSTRING* vr$226 = fb_StrConcat( &TMP$1615$2, (void*)vr$223, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1616$2, 0, 24ll );
		FBSTRING* vr$229 = fb_StrConcat( &TMP$1616$2, (void*)vr$226, -1ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1617$2, 0, 24ll );
		FBSTRING* vr$232 = fb_StrConcat( &TMP$1617$2, (void*)vr$229, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1618$2, 0, 24ll );
		FBSTRING* vr$235 = fb_StrConcat( &TMP$1618$2, (void*)vr$232, -1ll, (void*)"1", 2ll );
		fb_StrAssign( (void*)&TMP$1619$2, -1ll, (void*)vr$235, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1619$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1619$2 );
	}
	goto label$722;
	label$723:;
	{
		FBSTRING TMP$1621$2;
		FBSTRING TMP$1622$2;
		FBSTRING TMP$1623$2;
		FBSTRING TMP$1624$2;
		__builtin_memset( &TMP$1624$2, 0, 24ll );
		__builtin_memset( &TMP$1621$2, 0, 24ll );
		FBSTRING* vr$241 = fb_StrConcat( &TMP$1621$2, (void*)"movhlps ", 9ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1622$2, 0, 24ll );
		FBSTRING* vr$244 = fb_StrConcat( &TMP$1622$2, (void*)vr$241, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1623$2, 0, 24ll );
		FBSTRING* vr$247 = fb_StrConcat( &TMP$1623$2, (void*)vr$244, -1ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$1624$2, -1ll, (void*)vr$247, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1624$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1624$2 );
	}
	label$722:;
	__builtin_memset( &TMP$1629$1, 0, 24ll );
	__builtin_memset( &TMP$1625$1, 0, 24ll );
	FBSTRING* vr$253 = fb_StrConcat( &TMP$1625$1, (void*)"adds", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1626$1, 0, 24ll );
	FBSTRING* vr$256 = fb_StrConcat( &TMP$1626$1, (void*)vr$253, -1ll, (void*)"xmm7", 5ll );
	__builtin_memset( &TMP$1627$1, 0, 24ll );
	FBSTRING* vr$259 = fb_StrConcat( &TMP$1627$1, (void*)vr$256, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1628$1, 0, 24ll );
	FBSTRING* vr$262 = fb_StrConcat( &TMP$1628$1, (void*)vr$259, -1ll, (void*)"[esp]", 6ll );
	fb_StrAssign( (void*)&TMP$1629$1, -1ll, (void*)vr$262, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1629$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1629$1 );
	__builtin_memset( &TMP$1634$1, 0, 24ll );
	__builtin_memset( &TMP$1630$1, 0, 24ll );
	FBSTRING* vr$268 = fb_StrConcat( &TMP$1630$1, (void*)"subs", 5ll, (void*)&SUFFIX$1, -1ll );
	__builtin_memset( &TMP$1631$1, 0, 24ll );
	FBSTRING* vr$271 = fb_StrConcat( &TMP$1631$1, (void*)vr$268, -1ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$1632$1, 0, 24ll );
	FBSTRING* vr$274 = fb_StrConcat( &TMP$1632$1, (void*)vr$271, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$1633$1, 0, 24ll );
	FBSTRING* vr$277 = fb_StrConcat( &TMP$1633$1, (void*)vr$274, -1ll, (void*)"xmm7", 5ll );
	fb_StrAssign( (void*)&TMP$1634$1, -1ll, (void*)vr$277, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1634$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1634$1 );
	__builtin_memset( &TMP$1636$1, 0, 24ll );
	FBSTRING* vr$282 = fb_LongintToStr( DDSIZE$1 + 8ll );
	__builtin_memset( &TMP$1635$1, 0, 24ll );
	FBSTRING* vr$285 = fb_StrConcat( &TMP$1635$1, (void*)"add esp, ", 10ll, (void*)vr$282, -1ll );
	fb_StrAssign( (void*)&TMP$1636$1, -1ll, (void*)vr$285, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$1636$1 );
	fb_StrDelete( (FBSTRING*)&TMP$1636$1 );
	label$710:;
}

static void _EMITPUSHF_SSE( struct $6IRVREG* SVREG$1, int64 UNUSED$1 )
{
	int64 TMP$1638$1;
	label$724:;
	static FBSTRING SRC$1;
	static int64 SDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$726;
	TMP$1638$1 = 24ll;
	goto label$822;
	label$726:;
	TMP$1638$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$822:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1638$1 * 56ll)) + 8ll);
	if( *(int64*)SVREG$1 == 4ll) goto label$728;
	{
		if( *(int64*)((uint8*)SVREG$1 + 8ll) != 15ll) goto label$730;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$729;
		label$730:;
		{
			HPREPOPERAND( SVREG$1, &SRC$1, 8ll, 4ll, 0ll, -1ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			HPREPOPERAND( SVREG$1, &SRC$1, 8ll, 0ll, 0ll, -1ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$729:;
	}
	goto label$727;
	label$728:;
	{
		FBSTRING TMP$1642$2;
		FBSTRING TMP$1643$2;
		FBSTRING TMP$1644$2;
		FBSTRING TMP$1645$2;
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sub esp, ", 10ll, 0 );
		FBSTRING* vr$7 = fb_LongintToStr( SDSIZE$1 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$7, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		if( *(int64*)((uint8*)SVREG$1 + 32ll) != 0ll) goto label$732;
		{
			FBSTRING TMP$1640$3;
			FBSTRING TMP$1641$3;
			__builtin_memset( &TMP$1640$3, 0, 24ll );
			FBSTRING* vr$14 = fb_StrConcat( &TMP$1640$3, (void*)"fstp ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)SVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
			__builtin_memset( &TMP$1641$3, 0, 24ll );
			FBSTRING* vr$17 = fb_StrConcat( &TMP$1641$3, (void*)vr$14, -1ll, (void*)" [esp]", 7ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$17, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			goto label$725;
		}
		label$732:;
		label$731:;
		if( SDSIZE$1 <= 4ll) goto label$734;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movlpd ", 8ll, 0 );
		}
		goto label$733;
		label$734:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movss ", 7ll, 0 );
		}
		label$733:;
		__builtin_memset( &TMP$1645$2, 0, 24ll );
		__builtin_memset( &TMP$1642$2, 0, 24ll );
		FBSTRING* vr$21 = fb_StrConcat( &TMP$1642$2, (void*)&OSTR$1, -1ll, (void*)"[esp]", 6ll );
		__builtin_memset( &TMP$1643$2, 0, 24ll );
		FBSTRING* vr$24 = fb_StrConcat( &TMP$1643$2, (void*)vr$21, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1644$2, 0, 24ll );
		FBSTRING* vr$27 = fb_StrConcat( &TMP$1644$2, (void*)vr$24, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$1645$2, -1ll, (void*)vr$27, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1645$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1645$2 );
	}
	label$727:;
	label$725:;
}

static void _EMITPOPF_SSE( struct $6IRVREG* DVREG$1, int64 UNUSED$1 )
{
	int64 TMP$1646$1;
	label$735:;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	static int64 DSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$737;
	TMP$1646$1 = 24ll;
	goto label$823;
	label$737:;
	TMP$1646$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$823:;
	DSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1646$1 * 56ll)) + 8ll);
	if( *(int64*)DVREG$1 == 4ll) goto label$739;
	{
		if( *(int64*)((uint8*)DVREG$1 + 8ll) != 15ll) goto label$741;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$740;
		label$741:;
		{
			HPREPOPERAND( DVREG$1, &DST$1, 8ll, 0ll, 0ll, -1ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			HPREPOPERAND( DVREG$1, &DST$1, 8ll, 4ll, 0ll, -1ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$740:;
	}
	goto label$738;
	label$739:;
	{
		FBSTRING TMP$1648$2;
		FBSTRING TMP$1649$2;
		FBSTRING TMP$1650$2;
		FBSTRING TMP$1651$2;
		FBSTRING TMP$1652$2;
		FBSTRING TMP$1653$2;
		FBSTRING TMP$1654$2;
		if( DSIZE$1 <= 4ll) goto label$743;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movlpd ", 8ll, 0 );
		}
		goto label$742;
		label$743:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movss ", 7ll, 0 );
		}
		label$742:;
		__builtin_memset( &TMP$1652$2, 0, 24ll );
		__builtin_memset( &TMP$1648$2, 0, 24ll );
		FBSTRING* vr$13 = fb_StrConcat( &TMP$1648$2, (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$1649$2, 0, 24ll );
		FBSTRING* vr$16 = fb_StrConcat( &TMP$1649$2, (void*)vr$13, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$1650$2, 0, 24ll );
		FBSTRING* vr$19 = fb_StrConcat( &TMP$1650$2, (void*)vr$16, -1ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
		__builtin_memset( &TMP$1651$2, 0, 24ll );
		FBSTRING* vr$22 = fb_StrConcat( &TMP$1651$2, (void*)vr$19, -1ll, (void*)" [esp]", 7ll );
		fb_StrAssign( (void*)&TMP$1652$2, -1ll, (void*)vr$22, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1652$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1652$2 );
		__builtin_memset( &TMP$1654$2, 0, 24ll );
		FBSTRING* vr$26 = fb_LongintToStr( DSIZE$1 );
		__builtin_memset( &TMP$1653$2, 0, 24ll );
		FBSTRING* vr$29 = fb_StrConcat( &TMP$1653$2, (void*)"add esp, ", 10ll, (void*)vr$26, -1ll );
		fb_StrAssign( (void*)&TMP$1654$2, -1ll, (void*)vr$29, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$1654$2 );
		fb_StrDelete( (FBSTRING*)&TMP$1654$2 );
	}
	label$738:;
	label$736:;
}
