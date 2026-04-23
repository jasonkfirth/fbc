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
typedef int64 $19EMIT_NODECLASS_ENUM;
typedef int64 $10IR_EMITOPT;
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
struct $8REGCLASS;
typedef int64 (*tmp$85)( struct $8REGCLASS*, struct $6IRVREG*, struct $6IRVREG*, uint64 );
typedef int64 (*tmp$86)( struct $8REGCLASS*, int64, struct $6IRVREG*, struct $6IRVREG* );
typedef void (*tmp$87)( struct $8REGCLASS*, int64 );
typedef int64 (*tmp$88)( struct $8REGCLASS*, int64 );
typedef void (*tmp$89)( struct $8REGCLASS*, int64, struct $6IRVREG*, struct $6IRVREG* );
typedef int64 (*tmp$90)( struct $8REGCLASS* );
typedef struct $6IRVREG* (*tmp$91)( struct $8REGCLASS*, int64, struct $6IRVREG** );
typedef void (*tmp$92)( struct $8REGCLASS* );
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
	tmp$85 ENSURE;
	tmp$85 _ALLOCATE;
	tmp$86 ALLOCATEREG;
	tmp$87 FREE;
	tmp$88 ISFREE;
	tmp$89 SETOWNER;
	tmp$90 GETMAXREGS;
	tmp$90 GETFIRST;
	tmp$88 GETNEXT;
	tmp$91 GETVREG;
	tmp$88 GETREALREG;
	tmp$92 CLEAR;
	tmp$92 DUMP;
	int64 CLASS;
	int64 ISSTACK;
	int64 REGS;
	struct $6IRVREG* VREGTB[8];
	struct $6IRVREG* VAUXPARENT[8];
	struct $10REG_REGCTX REGCTX;
	struct $10REG_STKCTX STKCTX;
};
__FB_STATIC_ASSERT( sizeof( struct $8REGCLASS ) == 608 );
struct $12EMITDATATYPE {
	int64 RNAMETB;
	char MNAME[12];
};
__FB_STATIC_ASSERT( sizeof( struct $12EMITDATATYPE ) == 24 );
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
typedef char* (*tmp$105)( int64 );
typedef int64 (*tmp$40)( void );
typedef void (*tmp$39)( void );
typedef int64 $14IR_OPTIONVALUE;
typedef int64 (*tmp$41)( $14IR_OPTIONVALUE );
typedef int64 (*tmp$42)( int64, int64 );
typedef void (*tmp$101)( int64, int64, int64, int64* );
typedef void (*tmp$102)( int64, int64, int64*, int64* );
typedef char* (*tmp$45)( void );
typedef void (*tmp$43)( struct $8FBSYMBOL* );
typedef void (*tmp$44)( struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef void (*tmp$103)( struct $8FBSYMBOL*, int64, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
typedef void (*tmp$104)( int64, int64 );
typedef char* (*tmp$106)( int64, int64 );
struct $9EMIT_VTBL {
	tmp$40 INIT;
	tmp$39 END;
	tmp$41 GETOPTIONVALUE;
	tmp$40 OPEN;
	tmp$39 CLOSE;
	tmp$42 ISREGPRESERVED;
	tmp$42 GETFREEPRESERVEDREG;
	tmp$101 GETARGREG;
	tmp$102 GETRESULTREG;
	tmp$45 PROCGETFRAMEREGNAME;
	tmp$43 PROCBEGIN;
	tmp$43 PROCEND;
	tmp$44 PROCHEADER;
	tmp$103 PROCFOOTER;
	tmp$44 PROCALLOCARG;
	tmp$44 PROCALLOCLOCAL;
	tmp$43 PROCALLOCSTATICVARS;
	tmp$43 SCOPEBEGIN;
	tmp$43 SCOPEEND;
	tmp$104 SETSECTION;
	tmp$105 GETTYPESTRING;
	tmp$106 GETSECTIONSTRING;
};
__FB_STATIC_ASSERT( sizeof( struct $9EMIT_VTBL ) == 176 );
struct $11TSTRSETITEM {
	FBSTRING S;
	int64 USERDATA;
	struct $8HASHITEM* HASHITEM;
};
__FB_STATIC_ASSERT( sizeof( struct $11TSTRSETITEM ) == 40 );
struct $16__FB_ARRAYDIMTB$ {
	int64 ELEMENTS;
	int64 LBOUND;
	int64 UBOUND;
};
__FB_STATIC_ASSERT( sizeof( struct $16__FB_ARRAYDIMTB$ ) == 24 );
struct $8FBARRAY1I12REG_SIZEMASKE {
	$12REG_SIZEMASK* DATA;
	$12REG_SIZEMASK* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[1];
};
__FB_STATIC_ASSERT( sizeof( struct $8FBARRAY1I12REG_SIZEMASKE ) == 72 );
struct $7FBARRAYI12REG_SIZEMASKE {
	$12REG_SIZEMASK* DATA;
	$12REG_SIZEMASK* PTR;
	int64 SIZE;
	int64 ELEMENT_LEN;
	int64 DIMENSIONS;
	int64 FLAGS;
	struct $16__FB_ARRAYDIMTB$ DIMTB[8];
};
__FB_STATIC_ASSERT( sizeof( struct $7FBARRAYI12REG_SIZEMASKE ) == 240 );
struct $20FB_GLOBCTORLIST_ITEM;
struct $20FB_GLOBCTORLIST_ITEM {
	struct $8FBSYMBOL* SYM;
	struct $20FB_GLOBCTORLIST_ITEM* NEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $20FB_GLOBCTORLIST_ITEM ) == 16 );
typedef int64 $10IR_SECTION;
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
typedef int64 $6IR_OPT;
int32 fb_FileOpen( FBSTRING*, uint32, uint32, uint32, int32, int32 );
int32 fb_FileClose( int32 );
int32 fb_FilePutStrLarge( int32, int64, void*, int64 );
int32 fb_FileFree( void );
int32 fb_FileKill( FBSTRING* );
FBSTRING* fb_StrInit( void*, int64, void*, int64, int32 );
FBSTRING* fb_StrAssign( void*, int64, void*, int64, int32 );
void fb_StrDelete( FBSTRING* );
FBSTRING* fb_StrConcat( FBSTRING*, void*, int64, void*, int64 );
FBSTRING* fb_StrConcatByref( void*, int64, void*, int64, int32 );
int32 fb_StrCompare( void*, int64, void*, int64 );
FBSTRING* fb_StrConcatAssign( void*, int64, void*, int64, int32 );
FBSTRING* fb_StrAllocTempDescZ( char* );
FBSTRING* fb_IntToStr( int32 );
FBSTRING* fb_LongintToStr( int64 );
FBSTRING* fb_ULongintToStr( uint64 );
int64 fb_StrLen( void*, int64 );
FBSTRING* fb_HEX_l( uint64 );
FBSTRING* fb_RIGHT( FBSTRING*, int64 );
static void fb_ctor__emit_x86( void ) __attribute__(( constructor ));
static void _ZN11TSTRSETITEMaSERKS_( struct $11TSTRSETITEM*, struct $11TSTRSETITEM* );
typedef int64 $12FB_ERRMSGOPT;
void ERRREPORTEX( int64, char*, int64, $12FB_ERRMSGOPT, char* );
int64 FBGETOPTION( int64 );
int64 SYMBGETREALSIZE( struct $8FBSYMBOL* );
char* SYMBUNIQUELABEL( void );
char* SYMBMAKEPROFILELABELNAME( void );
char* SYMBGETMANGLEDNAME( struct $8FBSYMBOL* );
void SYMBFOREACHGLOBAL( int64, tmp$43 );
int64 HFILEEXISTS( char* );
FBSTRING* HFLOATTOHEX( double, int64 );
char* HESCAPE( char*, int64 );
char* HESCAPEW( uint32*, int64 );
struct $8REGCLASS* REGNEWCLASS( int64, int64, struct $7FBARRAYI12REG_SIZEMASKE*, int64 );
int64 REGDELCLASS( struct $8REGCLASS* );
void EMITVARINIOFS( char*, int64 );
void EMITWRITESTR( char*, int64 );
void EMITRESET( void );
void EMITFLUSH( void );
void EDBGINIT( void );
void EDBGEMITHEADER( char* );
void EDBGLINEBEGIN( struct $8FBSYMBOL*, int64, int64, char* );
void EDBGLINEEND( struct $8FBSYMBOL*, int64, int64 );
void EDBGEMITLINEFLUSH( struct $8FBSYMBOL*, int64, struct $8FBSYMBOL* );
void EDBGSCOPEBEGIN( struct $8FBSYMBOL* );
void EDBGSCOPEEND( struct $8FBSYMBOL* );
void EDBGEMITSCOPEINI( struct $8FBSYMBOL* );
void EDBGEMITSCOPEEND( struct $8FBSYMBOL* );
void EDBGPROCBEGIN( struct $8FBSYMBOL* );
void EDBGPROCEND( struct $8FBSYMBOL* );
void EDBGPROCEMITBEGIN( struct $8FBSYMBOL* );
void EDBGEMITPROCHEADER( struct $8FBSYMBOL* );
void EDBGEMITPROCFOOTER( struct $8FBSYMBOL*, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
void EDBGEMITFOOTER( void );
void EDBGEMITGLOBALVAR( struct $8FBSYMBOL*, int64 );
void EDBGINCLUDE( char* );
void HPREPOPERAND( struct $6IRVREG*, FBSTRING*, $11FB_DATATYPE, int64, int64, int64 );
void HPREPOPERAND64( struct $6IRVREG*, FBSTRING*, FBSTRING* );
void HPUSH( char* );
void HPOP( char* );
int64 HFINDREGNOTINVREG( struct $6IRVREG*, int64 );
int64 HFINDFREEREG( int64 );
int64 HISREGFREE( int64, int64 );
int64 HISREGINVREG( struct $6IRVREG*, int64 );
char* HGETREGNAME( int64, int64 );
void OUTP( char* );
void HLABEL( char* );
void HBRANCH( char*, char* );
void HMOV( char*, char* );
int64 _INIT_OPFNTB_SSE( void** );
void HCMPF_JXX( struct $11CMPF_RECIPE*, FBSTRING* );
void HCMPF_SET( struct $6IRVREG*, struct $11CMPF_RECIPE*, FBSTRING* );
void IRHLFLUSHSTATICINITIALIZER( struct $8FBSYMBOL* );
static void HDECLVARIABLE( struct $8FBSYMBOL* );
static char* _GETSECTIONSTRING( int64, int64 );
static void _SETSECTION( int64, int64 );
static char* _GETTYPESTRING( int64 );
static char* HGETIDXNAME( struct $6IRVREG* );
static void OUTEX( char*, int64 );
static void HXCHG( char*, char* );
static void HPUBLIC( char*, int64 );
static void HALIGN( int64 );
static void HINITREGTB( void );
static void HENDREGTB( void );
static int64 HGETGLOBALTYPEALIGN( int64 );
static int64 HGETGLOBALVARALIGN( struct $8FBSYMBOL* );
static void HEMITVARBSS( struct $8FBSYMBOL* );
static void HWRITEHEADER( void );
static void HEMITVARCONST( struct $8FBSYMBOL* );
static void HWRITECTOR( struct $20FB_GLOBCTORLIST_ITEM*, int64 );
static void HEMITEXPORT( struct $8FBSYMBOL* );
static void HCLEARLOCALS( int64, int64 );
static int64 HFRAMEBYTESTOALLOC( struct $8FBSYMBOL* );
static void HSTOREREGISTERARGUMENT( struct $8FBSYMBOL*, FBSTRING* );
static void HSTOREREGISTERARGUMENTS( struct $8FBSYMBOL* );
static void HCREATEFRAME( struct $8FBSYMBOL* );
static void HDESTROYFRAME( struct $8FBSYMBOL*, int64 );
static void _EMITLIT( char* );
static void _EMITJMPTB( struct $8FBSYMBOL*, uint64*, struct $8FBSYMBOL**, int64, struct $8FBSYMBOL*, uint64, uint64 );
static void _EMITCALL( struct $6IRVREG*, struct $8FBSYMBOL*, int64 );
static void _EMITCALLPTR( struct $6IRVREG*, struct $8FBSYMBOL*, int64 );
static void _EMITBRANCH( struct $6IRVREG*, struct $8FBSYMBOL*, int64 );
static void _EMITJUMP( struct $6IRVREG*, struct $8FBSYMBOL*, int64 );
static void _EMITJUMPPTR( struct $6IRVREG*, struct $8FBSYMBOL*, int64 );
static void _EMITRET( struct $6IRVREG* );
static void _EMITPUBLIC( struct $8FBSYMBOL* );
static void _EMITLABEL( struct $8FBSYMBOL* );
static void _EMITNOP( void );
static void HULONG2DBL( struct $6IRVREG* );
static void _EMITSTORL2L( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORI2L( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORF2L( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORI2I( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORL2I( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORF2I( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORL2F( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORI2F( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORF2F( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADL2L( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADI2L( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADF2L( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADI2I( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADL2I( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADF2I( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADL2F( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADI2F( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADF2F( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMOVL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMOVI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMOVF( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITADDL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITADDI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITADDF( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSUBL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSUBI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSUBF( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMULL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMULI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMULF( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITDIVF( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITDIVI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITMODI( struct $6IRVREG*, struct $6IRVREG* );
static void HSHIFTL( int64, struct $6IRVREG*, struct $6IRVREG* );
static void HSHIFTI( int64, struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSHLL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSHLI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSHRL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSHRI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITANDL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITANDI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITORL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITORI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITXORL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITXORI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITEQVL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITEQVI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITIMPL( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITIMPI( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITATN2( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITPOW( struct $6IRVREG*, struct $6IRVREG* );
static void HCMPL( struct $6IRVREG*, struct $8FBSYMBOL*, char*, char*, char*, struct $6IRVREG*, struct $6IRVREG*, int64 );
static void HCMPI( struct $6IRVREG*, struct $8FBSYMBOL*, char*, struct $6IRVREG*, struct $6IRVREG* );
static struct $11CMPF_RECIPE* HCMPF_GET_RECIPE( $7CMPF_OP, $10IR_EMITOPT, struct $8FBSYMBOL* );
static void HCMPF( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, struct $11CMPF_RECIPE* );
static void _EMITCGTL( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCGTI( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCGTF( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCLTL( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCLTI( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCLTF( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCEQL( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCEQI( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCEQF( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCNEL( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCNEI( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCNEF( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCLEL( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCLEI( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCLEF( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCGEL( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCGEI( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITCGEF( struct $6IRVREG*, struct $8FBSYMBOL*, struct $6IRVREG*, struct $6IRVREG*, $10IR_EMITOPT );
static void _EMITNEGL( struct $6IRVREG* );
static void _EMITNEGI( struct $6IRVREG* );
static void _EMITNEGF( struct $6IRVREG* );
static void _EMITNOTL( struct $6IRVREG* );
static void _EMITNOTI( struct $6IRVREG* );
static void _EMITABSL( struct $6IRVREG* );
static void _EMITABSI( struct $6IRVREG* );
static void _EMITABSF( struct $6IRVREG* );
static void _EMITSGNL( struct $6IRVREG* );
static void _EMITSGNI( struct $6IRVREG* );
static void _EMITSGNF( struct $6IRVREG* );
static void _EMITSIN( struct $6IRVREG* );
static void _EMITASIN( struct $6IRVREG* );
static void _EMITCOS( struct $6IRVREG* );
static void _EMITACOS( struct $6IRVREG* );
static void _EMITTAN( struct $6IRVREG* );
static void _EMITATAN( struct $6IRVREG* );
static void _EMITSQRT( struct $6IRVREG* );
static void _EMITLOG( struct $6IRVREG* );
static void _EMITEXP( struct $6IRVREG* );
static void HFPUCHANGERC( FBSTRING*, char* );
static void HEMITFLOATFUNC( int64 );
static void HEMITFLOAT_INT_686( struct $6IRVREG* );
static void HEMITFLOAT_FIX_686( struct $6IRVREG* );
static void _EMITFLOOR( struct $6IRVREG* );
static void _EMITFIX( struct $6IRVREG* );
static void _EMITFRAC( struct $6IRVREG* );
static void _EMITCONVFD2FS( struct $6IRVREG* );
static void _EMITXCHGTOS( struct $6IRVREG* );
static void _EMITSTACKALIGN( struct $6IRVREG*, int64 );
static void _EMITPUSHL( struct $6IRVREG*, int64 );
static void _EMITPUSHI( struct $6IRVREG*, int64 );
static void _EMITPUSHF( struct $6IRVREG*, int64 );
static void _EMITPUSHUDT( struct $6IRVREG*, int64 );
static void _EMITPOPL( struct $6IRVREG*, int64 );
static void _EMITPOPI( struct $6IRVREG*, int64 );
static void _EMITPOPF( struct $6IRVREG*, int64 );
static void _EMITPOPST0( struct $6IRVREG*, int64 );
static void _EMITADDROF( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITDEREF( struct $6IRVREG*, struct $6IRVREG* );
static void HMEMMOVEREP( struct $6IRVREG*, struct $6IRVREG*, int64 );
static void HMEMMOVEBLK( struct $6IRVREG*, struct $6IRVREG*, int64 );
static void _EMITMEMMOVE( struct $6IRVREG*, struct $6IRVREG*, int64, int64 );
static void _EMITMEMSWAP( struct $6IRVREG*, struct $6IRVREG*, int64, int64 );
static void HMEMFILLREPIMM( struct $6IRVREG*, uint32, int32 );
static void HMEMFILLBLKIMM( struct $6IRVREG*, uint32, int32 );
static void HMEMFILL( struct $6IRVREG*, struct $6IRVREG*, int64 );
static void _EMITMEMFILL( struct $6IRVREG*, struct $6IRVREG*, int64, int64 );
static void _EMITSTKCLEAR( struct $6IRVREG*, struct $6IRVREG*, int64, int64 );
static void _EMITLINEINI( struct $8FBSYMBOL*, int64, int64, char* );
static void _EMITLINEEND( struct $8FBSYMBOL*, int64, int64, char* );
static void _EMITSCOPEINI( struct $8FBSYMBOL*, int64, int64, char* );
static void _EMITSCOPEEND( struct $8FBSYMBOL*, int64, int64, char* );
static void _EMITLOADB2B( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORB2B( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADB2I( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORB2I( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADI2B( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORI2B( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADF2B( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORF2B( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADB2F( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORB2F( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADB2L( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORB2L( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITLOADL2B( struct $6IRVREG*, struct $6IRVREG* );
static void _EMITSTORL2B( struct $6IRVREG*, struct $6IRVREG* );
static int64 _INIT( void );
static void _END( void );
static int64 _GETOPTIONVALUE( $14IR_OPTIONVALUE );
static int64 _OPEN( void );
static void _CLOSE( void );
static char* _PROCGETFRAMEREGNAME( void );
static int64 _ISREGPRESERVED( int64, int64 );
static void _GETARGREG( int64, int64, int64, int64* );
static void _GETRESULTREG( int64, int64, int64*, int64* );
static int64 _GETFREEPRESERVEDREG( int64, int64 );
static void _PROCBEGIN( struct $8FBSYMBOL* );
static void _PROCEND( struct $8FBSYMBOL* );
static void _PROCALLOCSTATICVARS( struct $8FBSYMBOL* );
static void _PROCALLOCLOCAL( struct $8FBSYMBOL*, struct $8FBSYMBOL* );
static void _PROCALLOCARG( struct $8FBSYMBOL*, struct $8FBSYMBOL* );
static void _PROCHEADER( struct $8FBSYMBOL*, struct $8FBSYMBOL* );
static void _PROCFOOTER( struct $8FBSYMBOL*, int64, struct $8FBSYMBOL*, struct $8FBSYMBOL* );
static void _SCOPEBEGIN( struct $8FBSYMBOL* );
static void _SCOPEEND( struct $8FBSYMBOL* );
typedef void (*tmp$46)( struct $6IRVREG*, struct $6IRVREG* );
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
static struct $8FBARRAY1IPvE tmp$107$;
extern struct $12EMITDATATYPE DTYPETB$[26];
static char RNAMETB$[4][8][8] = { { "dl", "di", "si", "cl", "bl", "al", "bp", "sp" }, { "dx", "di", "si", "cx", "bx", "ax", "bp", "sp" }, { "edx", "edi", "esi", "ecx", "ebx", "eax", "ebp", "esp" }, { "st(0)", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)" } };
static void* _OPFNTB$[143] = { (void*)&_EMITNOP, (void*)&_EMITLOADI2I, (void*)&_EMITLOADF2I, (void*)&_EMITLOADL2I, (void*)&_EMITLOADB2I, (void*)&_EMITLOADI2F, (void*)&_EMITLOADF2F, (void*)&_EMITLOADL2F, (void*)&_EMITLOADB2F, (void*)&_EMITLOADI2L, (void*)&_EMITLOADF2L, (void*)&_EMITLOADL2L, (void*)&_EMITLOADB2L, (void*)&_EMITLOADI2B, (void*)&_EMITLOADF2B, (void*)&_EMITLOADL2B, (void*)&_EMITLOADB2B, (void*)&_EMITSTORI2I, (void*)&_EMITSTORF2I, (void*)&_EMITSTORL2I, (void*)&_EMITSTORB2I, (void*)&_EMITSTORI2F, (void*)&_EMITSTORF2F, (void*)&_EMITSTORL2F, (void*)&_EMITSTORB2F, (void*)&_EMITSTORI2L, (void*)&_EMITSTORF2L, (void*)&_EMITSTORL2L, (void*)&_EMITSTORB2L, (void*)&_EMITSTORI2B, (void*)&_EMITSTORF2B, (void*)&_EMITSTORL2B, (void*)&_EMITSTORB2B, (void*)&_EMITMOVI, (void*)&_EMITMOVF, (void*)&_EMITMOVL, (void*)&_EMITADDI, (void*)&_EMITADDF, (void*)&_EMITADDL, (void*)&_EMITSUBI, (void*)&_EMITSUBF, (void*)&_EMITSUBL, (void*)&_EMITMULI, (void*)&_EMITMULF, (void*)&_EMITMULL, (void*)&_EMITDIVI, (void*)&_EMITDIVF, (void*)0ull, (void*)&_EMITMODI, (void*)0ull, (void*)0ull, (void*)&_EMITSHLI, (void*)&_EMITSHLL, (void*)&_EMITSHRI, (void*)&_EMITSHRL, (void*)&_EMITANDI, (void*)&_EMITANDL, (void*)&_EMITORI, (void*)&_EMITORL, (void*)&_EMITXORI, (void*)&_EMITXORL, (void*)&_EMITEQVI, (void*)&_EMITEQVL, (void*)&_EMITIMPI, (void*)&_EMITIMPL, (void*)&_EMITATN2, (void*)&_EMITPOW, (void*)&_EMITADDROF, (void*)&_EMITDEREF, (void*)&_EMITCGTI, (void*)&_EMITCGTF, (void*)&_EMITCGTL, (void*)&_EMITCLTI, (void*)&_EMITCLTF, (void*)&_EMITCLTL, (void*)&_EMITCEQI, (void*)&_EMITCEQF, (void*)&_EMITCEQL, (void*)&_EMITCNEI, (void*)&_EMITCNEF, (void*)&_EMITCNEL, (void*)&_EMITCGEI, (void*)&_EMITCGEF, (void*)&_EMITCGEL, (void*)&_EMITCLEI, (void*)&_EMITCLEF, (void*)&_EMITCLEL, (void*)&_EMITNEGI, (void*)&_EMITNEGF, (void*)&_EMITNEGL, (void*)&_EMITNOTI, (void*)&_EMITNOTL, (void*)0ull, (void*)&_EMITABSI, (void*)&_EMITABSF, (void*)&_EMITABSL, (void*)&_EMITSGNI, (void*)&_EMITSGNF, (void*)&_EMITSGNL, (void*)&_EMITFIX, (void*)&_EMITFRAC, (void*)&_EMITCONVFD2FS, (void*)0ull, (void*)&_EMITSIN, (void*)&_EMITASIN, (void*)&_EMITCOS, (void*)&_EMITACOS, (void*)&_EMITTAN, (void*)&_EMITATAN, (void*)&_EMITSQRT, (void*)0ull, (void*)0ull, (void*)&_EMITLOG, (void*)&_EMITEXP, (void*)&_EMITFLOOR, (void*)&_EMITXCHGTOS, (void*)&_EMITSTACKALIGN, (void*)&_EMITPUSHI, (void*)&_EMITPUSHF, (void*)&_EMITPUSHL, (void*)&_EMITPOPI, (void*)&_EMITPOPF, (void*)&_EMITPOPL, (void*)&_EMITPUSHUDT, (void*)&_EMITPOPST0, (void*)&_EMITCALL, (void*)&_EMITCALLPTR, (void*)&_EMITBRANCH, (void*)&_EMITJUMP, (void*)&_EMITJUMPPTR, (void*)&_EMITRET, (void*)&_EMITLABEL, (void*)&_EMITPUBLIC, (void*)&_EMITLIT, (void*)&_EMITJMPTB, (void*)&_EMITMEMMOVE, (void*)&_EMITMEMSWAP, (void*)&_EMITMEMFILL, (void*)&_EMITSTKCLEAR, (void*)&_EMITLINEINI, (void*)&_EMITLINEEND, (void*)&_EMITSCOPEINI, (void*)&_EMITSCOPEEND };
struct $12EMITDATATYPE DTYPETB$[26] = { { 0ll, "void ptr" }, { 0ll, "byte ptr" }, { 0ll, "byte ptr" }, { 0ll, "byte ptr" }, { 0ll, "byte ptr" }, { 1ll, "word ptr" }, { 1ll, "word ptr" }, { 1ll, "word ptr" }, { 2ll, "dword ptr" }, { 2ll, "dword ptr" }, { 2ll, "dword ptr" }, { 2ll, "dword ptr" }, { 2ll, "dword ptr" }, { 2ll, "qword ptr" }, { 2ll, "qword ptr" }, { 3ll, "dword ptr" }, { 3ll, "qword ptr" }, { 0ll, "" }, { 0ll, "byte ptr" }, { 0ll, "dword ptr" }, { 2ll, "dword ptr" }, { 0ll, "" }, { 2ll, "dword ptr" }, { 0ll, "byte ptr" }, { 2ll, "dword ptr" }, { 3ll, "xmmword ptr" } };

int64 HISREGFREE( int64 DCLASS$1, int64 REG$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$10:;
	if( DCLASS$1 != 0ll) goto label$13;
	{
		{
			if( REG$1 == 4ll) goto label$16;
			label$17:;
			if( REG$1 == 2ll) goto label$16;
			label$18:;
			if( REG$1 != 1ll) goto label$15;
			label$16:;
			{
				if( (*(int64*)((uint8*)&EMIT$ + 264ll) & (1ll << (REG$1 & 63ll))) != 0ll) goto label$20;
				{
					fb$result$1 = 0ll;
					goto label$11;
				}
				label$20:;
				label$19:;
			}
			label$15:;
			label$14:;
		}
	}
	label$13:;
	label$12:;
	*(int64*)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 264ll) = *(int64*)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 264ll) | (1ll << (REG$1 & 63ll));
	fb$result$1 = (int64)-((*(int64*)((uint8*)((uint8*)*(struct $9EMIT_NODE**)((uint8*)&EMIT$ + 256ll) + (DCLASS$1 << (3ll & 63ll))) + 72ll) & (1ll << (REG$1 & 63ll))) != 0ll);
	label$11:;
	return fb$result$1;
}

int64 HFINDREGNOTINVREG( struct $6IRVREG* VREG$1, int64 NOSIDI$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$21:;
	int64 R$1;
	int64 REG$1;
	int64 REG2$1;
	int64 REGS$1;
	fb$result$1 = -1ll;
	REG$1 = -1ll;
	{
		$15IRVREGTYPE_ENUM TMP$145$2;
		TMP$145$2 = *($15IRVREGTYPE_ENUM*)VREG$1;
		if( TMP$145$2 != 4ll) goto label$24;
		label$25:;
		{
			REG$1 = *(int64*)((uint8*)VREG$1 + 24ll);
		}
		goto label$23;
		label$24:;
		if( TMP$145$2 == 2ll) goto label$27;
		label$28:;
		if( TMP$145$2 != 3ll) goto label$26;
		label$27:;
		{
			if( *(struct $6IRVREG**)((uint8*)VREG$1 + 80ll) == (struct $6IRVREG*)0ull) goto label$30;
			{
				if( *(int64*)*(struct $6IRVREG**)((uint8*)VREG$1 + 80ll) != 4ll) goto label$32;
				{
					REG$1 = *(int64*)((uint8*)*(struct $6IRVREG**)((uint8*)VREG$1 + 80ll) + 24ll);
				}
				label$32:;
				label$31:;
			}
			label$30:;
			label$29:;
		}
		label$26:;
		label$23:;
	}
	REG2$1 = -1ll;
	if( *(struct $6IRVREG**)((uint8*)VREG$1 + 88ll) == (struct $6IRVREG*)0ull) goto label$34;
	{
		if( *(int64*)*(struct $6IRVREG**)((uint8*)VREG$1 + 88ll) != 4ll) goto label$36;
		{
			REG2$1 = *(int64*)((uint8*)*(struct $6IRVREG**)((uint8*)VREG$1 + 88ll) + 24ll);
		}
		label$36:;
		label$35:;
	}
	label$34:;
	label$33:;
	int64 vr$13 = (*(tmp$90*)((uint8*)*(struct $8REGCLASS**)((uint8*)&EMIT$ + 16ll) + 48ll))( *(struct $8REGCLASS**)((uint8*)&EMIT$ + 16ll) );
	REGS$1 = vr$13;
	if( REG2$1 != -1ll) goto label$38;
	{
		if( NOSIDI$1 != 0ll) goto label$40;
		{
			{
				int64 R$4;
				R$4 = REGS$1 + -1ll;
				goto label$41;
				label$44:;
				{
					if( R$4 == REG$1) goto label$46;
					{
						fb$result$1 = R$4;
						int64 vr$16 = HISREGFREE( 0ll, R$4 );
						if( vr$16 == 0ll) goto label$48;
						{
							goto label$43;
						}
						label$48:;
						label$47:;
					}
					label$46:;
					label$45:;
				}
				label$42:;
				R$4 = R$4 + -1ll;
				label$41:;
				if( R$4 >= 0ll) goto label$44;
				label$43:;
			}
		}
		goto label$39;
		label$40:;
		{
			{
				int64 R$4;
				R$4 = REGS$1 + -1ll;
				goto label$49;
				label$52:;
				{
					if( R$4 == REG$1) goto label$54;
					{
						if( R$4 == 2ll) goto label$56;
						{
							if( R$4 == 1ll) goto label$58;
							{
								fb$result$1 = R$4;
								int64 vr$19 = HISREGFREE( 0ll, R$4 );
								if( vr$19 == 0ll) goto label$60;
								{
									goto label$51;
								}
								label$60:;
								label$59:;
							}
							label$58:;
							label$57:;
						}
						label$56:;
						label$55:;
					}
					label$54:;
					label$53:;
				}
				label$50:;
				R$4 = R$4 + -1ll;
				label$49:;
				if( R$4 >= 0ll) goto label$52;
				label$51:;
			}
		}
		label$39:;
	}
	goto label$37;
	label$38:;
	{
		if( NOSIDI$1 != 0ll) goto label$62;
		{
			{
				int64 R$4;
				R$4 = REGS$1 + -1ll;
				goto label$63;
				label$66:;
				{
					if( ((int64)-(R$4 != REG$1) & (int64)-(R$4 != REG2$1)) == 0ll) goto label$68;
					{
						fb$result$1 = R$4;
						int64 vr$25 = HISREGFREE( 0ll, R$4 );
						if( vr$25 == 0ll) goto label$70;
						{
							goto label$65;
						}
						label$70:;
						label$69:;
					}
					label$68:;
					label$67:;
				}
				label$64:;
				R$4 = R$4 + -1ll;
				label$63:;
				if( R$4 >= 0ll) goto label$66;
				label$65:;
			}
		}
		goto label$61;
		label$62:;
		{
			{
				R$1 = REGS$1 + -1ll;
				goto label$71;
				label$74:;
				{
					if( ((int64)-(R$1 != REG$1) & (int64)-(R$1 != REG2$1)) == 0ll) goto label$76;
					{
						if( R$1 == 2ll) goto label$78;
						{
							if( R$1 == 1ll) goto label$80;
							{
								fb$result$1 = R$1;
								int64 vr$31 = HISREGFREE( 0ll, R$1 );
								if( vr$31 == 0ll) goto label$82;
								{
									goto label$73;
								}
								label$82:;
								label$81:;
							}
							label$80:;
							label$79:;
						}
						label$78:;
						label$77:;
					}
					label$76:;
					label$75:;
				}
				label$72:;
				R$1 = R$1 + -1ll;
				label$71:;
				if( R$1 >= 0ll) goto label$74;
				label$73:;
			}
		}
		label$61:;
	}
	label$37:;
	label$22:;
	return fb$result$1;
}

int64 HFINDFREEREG( int64 DCLASS$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$83:;
	fb$result$1 = -1ll;
	{
		int64 R$2;
		int64 vr$3 = (*(tmp$90*)((uint8*)*(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 16ll) + 48ll))( *(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 16ll) );
		R$2 = vr$3 + -1ll;
		goto label$85;
		label$88:;
		{
			int64 vr$8 = HISREGFREE( DCLASS$1, R$2 );
			if( vr$8 == 0ll) goto label$90;
			{
				fb$result$1 = R$2;
				goto label$84;
			}
			label$90:;
			label$89:;
		}
		label$86:;
		R$2 = R$2 + -1ll;
		label$85:;
		if( R$2 >= 0ll) goto label$88;
		label$87:;
	}
	label$84:;
	return fb$result$1;
}

int64 HISREGINVREG( struct $6IRVREG* VREG$1, int64 REG$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$91:;
	fb$result$1 = 0ll;
	{
		$15IRVREGTYPE_ENUM TMP$146$2;
		TMP$146$2 = *($15IRVREGTYPE_ENUM*)VREG$1;
		if( TMP$146$2 != 4ll) goto label$94;
		label$95:;
		{
			if( *(int64*)((uint8*)VREG$1 + 24ll) != REG$1) goto label$97;
			{
				fb$result$1 = -1ll;
				goto label$92;
			}
			label$97:;
			label$96:;
		}
		goto label$93;
		label$94:;
		if( TMP$146$2 == 2ll) goto label$99;
		label$100:;
		if( TMP$146$2 != 3ll) goto label$98;
		label$99:;
		{
			if( *(struct $6IRVREG**)((uint8*)VREG$1 + 80ll) == (struct $6IRVREG*)0ull) goto label$102;
			{
				if( *(int64*)*(struct $6IRVREG**)((uint8*)VREG$1 + 80ll) != 4ll) goto label$104;
				{
					if( *(int64*)((uint8*)*(struct $6IRVREG**)((uint8*)VREG$1 + 80ll) + 24ll) != REG$1) goto label$106;
					{
						fb$result$1 = -1ll;
						goto label$92;
					}
					label$106:;
					label$105:;
				}
				label$104:;
				label$103:;
			}
			label$102:;
			label$101:;
		}
		label$98:;
		label$93:;
	}
	if( *(struct $6IRVREG**)((uint8*)VREG$1 + 88ll) == (struct $6IRVREG*)0ull) goto label$108;
	{
		if( *(int64*)*(struct $6IRVREG**)((uint8*)VREG$1 + 88ll) != 4ll) goto label$110;
		{
			if( *(int64*)((uint8*)*(struct $6IRVREG**)((uint8*)VREG$1 + 88ll) + 24ll) != REG$1) goto label$112;
			{
				fb$result$1 = -1ll;
				goto label$92;
			}
			label$112:;
			label$111:;
		}
		label$110:;
		label$109:;
	}
	label$108:;
	label$107:;
	label$92:;
	return fb$result$1;
}

char* HGETREGNAME( int64 DTYPE$1, int64 REG$1 )
{
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$113:;
	if( REG$1 != -1ll) goto label$116;
	{
		fb$result$1 = (char*)0ull;
	}
	goto label$115;
	label$116:;
	{
		int64 TMP$147$2;
		int64 TB$2;
		if( (DTYPE$1 & 480ll) == 0ll) goto label$117;
		TMP$147$2 = 24ll;
		goto label$118;
		label$117:;
		TMP$147$2 = DTYPE$1 & 31ll;
		label$118:;
		TB$2 = *(int64*)((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (TMP$147$2 * 24ll));
		fb$result$1 = (char*)((int64)(char*)RNAMETB$ + (((TB$2 << (3ll & 63ll)) + REG$1) << (3ll & 63ll)));
	}
	label$115:;
	label$114:;
	return fb$result$1;
}

void HPREPOPERAND( struct $6IRVREG* VREG$1, FBSTRING* OPERAND$1, $11FB_DATATYPE DTYPE$1, int64 OFS$1, int64 ISAUX$1, int64 ADDPREFIX$1 )
{
	label$138:;
	if( VREG$1 != (struct $6IRVREG*)0ull) goto label$141;
	{
		fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)"", 1ll, 0 );
		goto label$139;
	}
	label$141:;
	label$140:;
	if( DTYPE$1 != 2147483648ll) goto label$143;
	{
		DTYPE$1 = *($11FB_DATATYPE*)((uint8*)VREG$1 + 8ll);
	}
	label$143:;
	label$142:;
	{
		uint64 TMP$156$2;
		TMP$156$2 = *(uint64*)VREG$1;
		goto label$145;
		label$146:;
		{
			if( ADDPREFIX$1 == 0ll) goto label$148;
			{
				fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (DTYPE$1 * 24ll)) + 8ll), 12ll, 0 );
				fb_StrConcatByref( (void*)OPERAND$1, -1ll, (void*)" [", 3ll, 0 );
			}
			goto label$147;
			label$148:;
			{
				fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)"[", 2ll, 0 );
			}
			label$147:;
			static char* IDX_OP$3;
			if( *(int64*)VREG$1 != 1ll) goto label$150;
			{
				char* vr$6 = SYMBGETMANGLEDNAME( *(struct $8FBSYMBOL**)((uint8*)VREG$1 + 56ll) );
				IDX_OP$3 = vr$6;
			}
			goto label$149;
			label$150:;
			{
				char* vr$7 = HGETIDXNAME( VREG$1 );
				IDX_OP$3 = vr$7;
			}
			label$149:;
			if( IDX_OP$3 == (char*)0ull) goto label$152;
			{
				FBSTRING TMP$159$4;
				__builtin_memset( &TMP$159$4, 0, 24ll );
				FBSTRING* vr$10 = fb_StrConcat( &TMP$159$4, (void*)OPERAND$1, -1ll, (void*)IDX_OP$3, 0ll );
				fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)vr$10, -1ll, 0 );
			}
			label$152:;
			label$151:;
			OFS$1 = OFS$1 + *(int64*)((uint8*)VREG$1 + 64ll);
			if( ISAUX$1 == 0ll) goto label$154;
			{
				OFS$1 = OFS$1 + 4ll;
			}
			label$154:;
			label$153:;
			if( OFS$1 <= 0ll) goto label$156;
			{
				if( IDX_OP$3 == (char*)0ull) goto label$158;
				{
					fb_StrConcatByref( (void*)OPERAND$1, -1ll, (void*)"+", 2ll, 0 );
				}
				label$158:;
				label$157:;
				FBSTRING* vr$14 = fb_LongintToStr( OFS$1 );
				fb_StrConcatByref( (void*)OPERAND$1, -1ll, (void*)vr$14, -1ll, 0 );
			}
			goto label$155;
			label$156:;
			if( OFS$1 >= 0ll) goto label$159;
			{
				FBSTRING* vr$15 = fb_LongintToStr( OFS$1 );
				fb_StrConcatByref( (void*)OPERAND$1, -1ll, (void*)vr$15, -1ll, 0 );
			}
			goto label$155;
			label$159:;
			{
				if( IDX_OP$3 != (char*)0ull) goto label$161;
				{
					fb_StrConcatByref( (void*)OPERAND$1, -1ll, (void*)"0", 2ll, 0 );
				}
				label$161:;
				label$160:;
			}
			label$155:;
			fb_StrConcatByref( (void*)OPERAND$1, -1ll, (void*)"]", 2ll, 0 );
		}
		goto label$144;
		label$162:;
		{
			FBSTRING TMP$162$3;
			fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)"offset ", 8ll, 0 );
			char* vr$17 = SYMBGETMANGLEDNAME( *(struct $8FBSYMBOL**)((uint8*)VREG$1 + 56ll) );
			__builtin_memset( &TMP$162$3, 0, 24ll );
			FBSTRING* vr$20 = fb_StrConcat( &TMP$162$3, (void*)OPERAND$1, -1ll, (void*)vr$17, 0ll );
			fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)vr$20, -1ll, 0 );
			if( *(int64*)((uint8*)VREG$1 + 64ll) == 0ll) goto label$164;
			{
				FBSTRING TMP$164$4;
				fb_StrConcatByref( (void*)OPERAND$1, -1ll, (void*)" + ", 4ll, 0 );
				FBSTRING* vr$23 = fb_LongintToStr( *(int64*)((uint8*)VREG$1 + 64ll) );
				__builtin_memset( &TMP$164$4, 0, 24ll );
				FBSTRING* vr$26 = fb_StrConcat( &TMP$164$4, (void*)OPERAND$1, -1ll, (void*)vr$23, -1ll );
				fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)vr$26, -1ll, 0 );
			}
			label$164:;
			label$163:;
		}
		goto label$144;
		label$165:;
		{
			if( ISAUX$1 != 0ll) goto label$167;
			{
				char* vr$28 = HGETREGNAME( DTYPE$1, *(int64*)((uint8*)VREG$1 + 24ll) );
				fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)vr$28, 0ll, 0 );
			}
			goto label$166;
			label$167:;
			{
				char* vr$31 = HGETREGNAME( DTYPE$1, *(int64*)((uint8*)*(struct $6IRVREG**)((uint8*)VREG$1 + 88ll) + 24ll) );
				fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)vr$31, 0ll, 0 );
			}
			label$166:;
		}
		goto label$144;
		label$168:;
		{
			static int64 I$3;
			if( ISAUX$1 != 0ll) goto label$170;
			{
				I$3 = *(int64*)((uint8*)VREG$1 + 48ll);
			}
			goto label$169;
			label$170:;
			{
				I$3 = *(int64*)((uint8*)*(struct $6IRVREG**)((uint8*)VREG$1 + 88ll) + 48ll);
			}
			label$169:;
			if( DTYPE$1 != 1ll) goto label$172;
			{
				if( I$3 == 0ll) goto label$174;
				{
					fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)"1", 2ll, 0 );
				}
				goto label$173;
				label$174:;
				{
					fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)"0", 2ll, 0 );
				}
				label$173:;
			}
			goto label$171;
			label$172:;
			{
				FBSTRING* vr$35 = fb_LongintToStr( I$3 );
				fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)vr$35, -1ll, 0 );
			}
			label$171:;
		}
		goto label$144;
		label$175:;
		{
			fb_StrAssign( (void*)OPERAND$1, -1ll, (void*)"", 1ll, 0 );
		}
		goto label$144;
		label$145:;
		static const void* tmp$165[6ll] = {
			&&label$168,
			&&label$146,
			&&label$146,
			&&label$146,
			&&label$165,
			&&label$162,
		};
		if( TMP$156$2 > 5ull ) goto label$175;
		goto *tmp$165[TMP$156$2 - 0ull];
		label$144:;
	}
	label$139:;
}

void HPREPOPERAND64( struct $6IRVREG* VREG$1, FBSTRING* OPERAND1$1, FBSTRING* OPERAND2$1 )
{
	label$176:;
	HPREPOPERAND( VREG$1, OPERAND1$1, 9ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( VREG$1, OPERAND2$1, 8ll, 0ll, -1ll, -1ll );
	label$177:;
}

void OUTP( char* S$1 )
{
	label$184:;
	static FBSTRING OSTR$1;
	if( *(int64*)((uint8*)&ENV$ + 296ll) == 0ll) goto label$187;
	{
		FBSTRING TMP$167$2;
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"\x09", 2ll, 0 );
		__builtin_memset( &TMP$167$2, 0, 24ll );
		FBSTRING* vr$2 = fb_StrConcat( &TMP$167$2, (void*)&OSTR$1, -1ll, (void*)S$1, 0ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	}
	goto label$186;
	label$187:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)S$1, 0ll, 0 );
	}
	label$186:;
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A", 2ll, 0 );
	int64 vr$3 = fb_StrLen( (void*)&OSTR$1, -1ll );
	OUTEX( (char*)*(char**)&OSTR$1, vr$3 );
	label$185:;
}

void HBRANCH( char* MNEMONIC$1, char* LABEL$1 )
{
	FBSTRING TMP$170$1;
	label$188:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)MNEMONIC$1, 0ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)" ", 2ll, 0 );
	__builtin_memset( &TMP$170$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$170$1, (void*)&OSTR$1, -1ll, (void*)LABEL$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$189:;
}

void HPUSH( char* RNAME$1 )
{
	FBSTRING TMP$172$1;
	label$190:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
	__builtin_memset( &TMP$172$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$172$1, (void*)&OSTR$1, -1ll, (void*)RNAME$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$191:;
}

void HPOP( char* RNAME$1 )
{
	FBSTRING TMP$174$1;
	label$192:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
	__builtin_memset( &TMP$174$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$174$1, (void*)&OSTR$1, -1ll, (void*)RNAME$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$193:;
}

void HMOV( char* DNAME$1, char* SNAME$1 )
{
	FBSTRING TMP$176$1;
	FBSTRING TMP$177$1;
	label$194:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	__builtin_memset( &TMP$176$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$176$1, (void*)&OSTR$1, -1ll, (void*)DNAME$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	__builtin_memset( &TMP$177$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$177$1, (void*)&OSTR$1, -1ll, (void*)SNAME$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$5, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$195:;
}

void HLABEL( char* LABEL$1 )
{
	label$202:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)LABEL$1, 0ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)":\x0A", 3ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$203:;
}

void HCMPF_JXX( struct $11CMPF_RECIPE* RECIPE$1, FBSTRING* LNAME$1 )
{
	label$1072:;
	static FBSTRING OSTR$1;
	static FBSTRING ISNANLABEL$1;
	if( *(int64*)((uint8*)RECIPE$1 + 40ll) == 0ll) goto label$1075;
	{
		HBRANCH( (char*)"jp", (char*)*(char**)LNAME$1 );
	}
	goto label$1074;
	label$1075:;
	if( *(int64*)((uint8*)RECIPE$1 + 48ll) == 0ll) goto label$1076;
	{
		char* vr$3 = SYMBUNIQUELABEL(  );
		fb_StrAssign( (void*)&ISNANLABEL$1, -1ll, (void*)vr$3, 0ll, 0 );
		HBRANCH( (char*)"jp", (char*)*(char**)&ISNANLABEL$1 );
	}
	label$1076:;
	label$1074:;
	int64 vr$5 = fb_StrLen( *(void**)((uint8*)RECIPE$1 + 24ll), 0ll );
	if( vr$5 <= 0ll) goto label$1078;
	{
		FBSTRING TMP$662$2;
		__builtin_memset( &TMP$662$2, 0, 24ll );
		FBSTRING* vr$9 = fb_StrConcat( &TMP$662$2, (void*)"j", 2ll, *(void**)((uint8*)RECIPE$1 + 8ll), 0ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$9, -1ll, 0 );
	}
	goto label$1077;
	label$1078:;
	{
		FBSTRING TMP$663$2;
		__builtin_memset( &TMP$663$2, 0, 24ll );
		FBSTRING* vr$13 = fb_StrConcat( &TMP$663$2, (void*)"j", 2ll, *(void**)((uint8*)RECIPE$1 + 8ll), 0ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$13, -1ll, 0 );
	}
	label$1077:;
	HBRANCH( (char*)*(char**)&OSTR$1, (char*)*(char**)LNAME$1 );
	if( *(int64*)((uint8*)RECIPE$1 + 48ll) == 0ll) goto label$1080;
	{
		HLABEL( (char*)*(char**)&ISNANLABEL$1 );
	}
	label$1080:;
	label$1079:;
	label$1073:;
}

void HCMPF_SET( struct $6IRVREG* RVREG$1, struct $11CMPF_RECIPE* RECIPE$1, FBSTRING* LNAME$1 )
{
	label$1081:;
	static FBSTRING RNAME$1;
	static FBSTRING RNAME8$1;
	static FBSTRING OSTR$1;
	static FBSTRING ISNANLABEL$1;
	static int64 ISEAXFREE$1;
	static int64 ISEDXFREE$1;
	HPREPOPERAND( RVREG$1, &RNAME$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)&ENV$ + 224ll) < 1ll) goto label$1084;
	{
		char* vr$1 = HGETREGNAME( 2ll, *(int64*)((uint8*)RVREG$1 + 24ll) );
		fb_StrAssign( (void*)&RNAME8$1, -1ll, (void*)vr$1, 0ll, 0 );
		if( (((int64)-(*(int64*)((uint8*)RVREG$1 + 24ll) == 2ll) | (int64)-(*(int64*)((uint8*)RVREG$1 + 24ll) == 1ll)) | (*(int64*)((uint8*)RECIPE$1 + 48ll) | *(int64*)((uint8*)RECIPE$1 + 40ll))) == 0ll) goto label$1086;
		{
			FBSTRING TMP$671$3;
			FBSTRING TMP$672$3;
			int64 vr$11 = HISREGFREE( 0ll, 0ll );
			ISEDXFREE$1 = vr$11;
			if( *(int64*)((uint8*)RVREG$1 + 24ll) == 0ll) goto label$1088;
			{
				if( ISEDXFREE$1 != 0ll) goto label$1090;
				{
					fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xchg edx, ", 11ll, 0 );
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
					OUTP( (char*)*(char**)&OSTR$1 );
				}
				label$1090:;
				label$1089:;
			}
			label$1088:;
			label$1087:;
			if( *(int64*)((uint8*)&ENV$ + 240ll) == 1ll) goto label$1092;
			{
				if( *(int64*)((uint8*)RECIPE$1 + 40ll) == 0ll) goto label$1094;
				{
					fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"setp\x09" "dh", 8ll, 0 );
					OUTP( (char*)*(char**)&OSTR$1 );
				}
				goto label$1093;
				label$1094:;
				if( *(int64*)((uint8*)RECIPE$1 + 48ll) == 0ll) goto label$1095;
				{
					fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"setnp\x09" "dh", 9ll, 0 );
					OUTP( (char*)*(char**)&OSTR$1 );
				}
				label$1095:;
				label$1093:;
			}
			label$1092:;
			label$1091:;
			__builtin_memset( &TMP$671$3, 0, 24ll );
			FBSTRING* vr$18 = fb_StrConcat( &TMP$671$3, (void*)"set", 4ll, *(void**)((uint8*)RECIPE$1 + 8ll), 0ll );
			__builtin_memset( &TMP$672$3, 0, 24ll );
			FBSTRING* vr$21 = fb_StrConcat( &TMP$672$3, (void*)vr$18, -1ll, (void*)"\x09" "dl", 4ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$21, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			if( *(int64*)((uint8*)&ENV$ + 240ll) == 1ll) goto label$1097;
			{
				if( *(int64*)((uint8*)RECIPE$1 + 40ll) == 0ll) goto label$1099;
				{
					if( *(int64*)((uint8*)RECIPE$1 + 64ll) == 0ll) goto label$1101;
					{
						fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"and dl, dh", 11ll, 0 );
					}
					goto label$1100;
					label$1101:;
					{
						fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"or dl, dh", 10ll, 0 );
					}
					label$1100:;
					OUTP( (char*)*(char**)&OSTR$1 );
				}
				goto label$1098;
				label$1099:;
				if( *(int64*)((uint8*)RECIPE$1 + 48ll) == 0ll) goto label$1102;
				{
					if( *(int64*)((uint8*)RECIPE$1 + 64ll) == 0ll) goto label$1104;
					{
						fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"or dl, dh", 10ll, 0 );
					}
					goto label$1103;
					label$1104:;
					{
						fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"and dl, dh", 11ll, 0 );
					}
					label$1103:;
					OUTP( (char*)*(char**)&OSTR$1 );
				}
				label$1102:;
				label$1098:;
			}
			label$1097:;
			label$1096:;
			if( *(int64*)((uint8*)RVREG$1 + 24ll) == 0ll) goto label$1106;
			{
				if( ISEDXFREE$1 != 0ll) goto label$1108;
				{
					fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xchg edx, ", 11ll, 0 );
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
					OUTP( (char*)*(char**)&OSTR$1 );
				}
				goto label$1107;
				label$1108:;
				{
					HMOV( (char*)*(char**)&RNAME$1, (char*)"edx" );
				}
				label$1107:;
			}
			label$1106:;
			label$1105:;
		}
		goto label$1085;
		label$1086:;
		{
			if( *(int64*)((uint8*)RECIPE$1 + 64ll) == 0ll) goto label$1110;
			{
				FBSTRING TMP$679$4;
				FBSTRING TMP$680$4;
				FBSTRING TMP$681$4;
				__builtin_memset( &TMP$680$4, 0, 24ll );
				FBSTRING* vr$30 = fb_StrConcat( &TMP$680$4, (void*)"\x09", 2ll, (void*)&RNAME8$1, -1ll );
				__builtin_memset( &TMP$679$4, 0, 24ll );
				FBSTRING* vr$34 = fb_StrConcat( &TMP$679$4, (void*)"set", 4ll, *(void**)((uint8*)RECIPE$1 + 16ll), 0ll );
				__builtin_memset( &TMP$681$4, 0, 24ll );
				FBSTRING* vr$37 = fb_StrConcat( &TMP$681$4, (void*)vr$34, -1ll, (void*)vr$30, -1ll );
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$37, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
			}
			goto label$1109;
			label$1110:;
			{
				FBSTRING TMP$682$4;
				FBSTRING TMP$683$4;
				FBSTRING TMP$684$4;
				__builtin_memset( &TMP$682$4, 0, 24ll );
				FBSTRING* vr$41 = fb_StrConcat( &TMP$682$4, (void*)"set", 4ll, *(void**)((uint8*)RECIPE$1 + 8ll), 0ll );
				__builtin_memset( &TMP$683$4, 0, 24ll );
				FBSTRING* vr$44 = fb_StrConcat( &TMP$683$4, (void*)vr$41, -1ll, (void*)" ", 2ll );
				__builtin_memset( &TMP$684$4, 0, 24ll );
				FBSTRING* vr$47 = fb_StrConcat( &TMP$684$4, (void*)vr$44, -1ll, (void*)&RNAME8$1, -1ll );
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$47, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
			}
			label$1109:;
		}
		label$1085:;
		if( *(int64*)((uint8*)RVREG$1 + 8ll) == 1ll) goto label$1112;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"shr ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 1", 4ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sbb ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$1112:;
		label$1111:;
	}
	goto label$1083;
	label$1084:;
	{
		FBSTRING TMP$685$2;
		if( *(int64*)((uint8*)RECIPE$1 + 64ll) == 0ll) goto label$1114;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 0", 4ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$1113;
		label$1114:;
		{
			if( *(int64*)((uint8*)RVREG$1 + 8ll) != 1ll) goto label$1116;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 1", 4ll, 0 );
			}
			goto label$1115;
			label$1116:;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", -1", 5ll, 0 );
			}
			label$1115:;
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$1113:;
		if( *(int64*)((uint8*)&ENV$ + 240ll) != 0ll) goto label$1118;
		{
			if( *(int64*)((uint8*)RECIPE$1 + 40ll) == 0ll) goto label$1120;
			{
				HBRANCH( (char*)"jp", (char*)*(char**)LNAME$1 );
			}
			goto label$1119;
			label$1120:;
			if( *(int64*)((uint8*)RECIPE$1 + 48ll) == 0ll) goto label$1121;
			{
				char* vr$54 = SYMBUNIQUELABEL(  );
				fb_StrAssign( (void*)&ISNANLABEL$1, -1ll, (void*)vr$54, 0ll, 0 );
				HBRANCH( (char*)"jp", (char*)*(char**)&ISNANLABEL$1 );
			}
			label$1121:;
			label$1119:;
		}
		label$1118:;
		label$1117:;
		__builtin_memset( &TMP$685$2, 0, 24ll );
		FBSTRING* vr$58 = fb_StrConcat( &TMP$685$2, (void*)"j", 2ll, *(void**)((uint8*)RECIPE$1 + 8ll), 0ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$58, -1ll, 0 );
		HBRANCH( (char*)*(char**)&OSTR$1, (char*)*(char**)LNAME$1 );
		if( *(int64*)((uint8*)&ENV$ + 240ll) != 0ll) goto label$1123;
		{
			if( *(int64*)((uint8*)RECIPE$1 + 48ll) == 0ll) goto label$1125;
			{
				HLABEL( (char*)*(char**)&ISNANLABEL$1 );
			}
			label$1125:;
			label$1124:;
		}
		label$1123:;
		label$1122:;
		if( *(int64*)((uint8*)RECIPE$1 + 64ll) == 0ll) goto label$1127;
		{
			if( *(int64*)((uint8*)RVREG$1 + 8ll) != 1ll) goto label$1129;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 1", 4ll, 0 );
			}
			goto label$1128;
			label$1129:;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", -1", 5ll, 0 );
			}
			label$1128:;
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$1126;
		label$1127:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$1126:;
		HLABEL( (char*)*(char**)LNAME$1 );
	}
	label$1083:;
	label$1082:;
}

void EMITVARINIBEGIN( struct $8FBSYMBOL* SYM$1 )
{
	label$1666:;
	_SETSECTION( 1ll, 0ll );
	EDBGEMITGLOBALVAR( SYM$1, 1ll );
	int64 vr$0 = HGETGLOBALVARALIGN( SYM$1 );
	HALIGN( vr$0 );
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 32ll) == 0ll) goto label$1669;
	{
		char* vr$6 = SYMBGETMANGLEDNAME( SYM$1 );
		HPUBLIC( vr$6, (int64)-((*(int64*)((uint8*)SYM$1 + 8ll) & 256ll) != 0ll) );
	}
	label$1669:;
	label$1668:;
	char* vr$7 = SYMBGETMANGLEDNAME( SYM$1 );
	HLABEL( vr$7 );
	label$1667:;
}

void EMITVARINII( int64 DTYPE$1, int64 VALUE$1 )
{
	FBSTRING TMP$980$1;
	label$1670:;
	FBSTRING S$1;
	__builtin_memset( &S$1, 0, 24ll );
	char* vr$1 = _GETTYPESTRING( DTYPE$1 );
	__builtin_memset( &TMP$980$1, 0, 24ll );
	FBSTRING* vr$4 = fb_StrConcat( &TMP$980$1, (void*)vr$1, 0ll, (void*)" ", 2ll );
	fb_StrAssign( (void*)&S$1, -1ll, (void*)vr$4, -1ll, 0 );
	if( ((int64)-(DTYPE$1 == 1ll) & (int64)-(VALUE$1 != 0ll)) == 0ll) goto label$1673;
	{
		VALUE$1 = 1ll;
	}
	label$1673:;
	label$1672:;
	if( ((int64)-(DTYPE$1 == 13ll) | (int64)-(DTYPE$1 == 14ll)) == 0ll) goto label$1675;
	{
		fb_StrConcatAssign( (void*)&S$1, -1ll, (void*)"0x", 3ll, 0 );
		FBSTRING* vr$13 = fb_HEX_l( (uint64)VALUE$1 );
		fb_StrConcatAssign( (void*)&S$1, -1ll, (void*)vr$13, -1ll, 0 );
	}
	goto label$1674;
	label$1675:;
	{
		FBSTRING* vr$15 = fb_LongintToStr( VALUE$1 );
		fb_StrConcatAssign( (void*)&S$1, -1ll, (void*)vr$15, -1ll, 0 );
	}
	label$1674:;
	fb_StrConcatAssign( (void*)&S$1, -1ll, (void*)"\x0A", 2ll, 0 );
	OUTEX( (char*)*(char**)&S$1, 0ll );
	fb_StrDelete( (FBSTRING*)&S$1 );
	label$1671:;
}

void EMITVARINIF( int64 DTYPE$1, double VALUE$1 )
{
	FBSTRING TMP$982$1;
	FBSTRING TMP$983$1;
	FBSTRING TMP$984$1;
	FBSTRING TMP$985$1;
	label$1676:;
	__builtin_memset( &TMP$985$1, 0, 24ll );
	FBSTRING* vr$1 = HFLOATTOHEX( VALUE$1, DTYPE$1 );
	char* vr$2 = _GETTYPESTRING( DTYPE$1 );
	__builtin_memset( &TMP$982$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$982$1, (void*)vr$2, 0ll, (void*)" ", 2ll );
	__builtin_memset( &TMP$983$1, 0, 24ll );
	FBSTRING* vr$8 = fb_StrConcat( &TMP$983$1, (void*)vr$5, -1ll, (void*)vr$1, -1ll );
	__builtin_memset( &TMP$984$1, 0, 24ll );
	FBSTRING* vr$11 = fb_StrConcat( &TMP$984$1, (void*)vr$8, -1ll, (void*)"\x0A", 2ll );
	fb_StrAssign( (void*)&TMP$985$1, -1ll, (void*)vr$11, -1ll, 0 );
	OUTEX( (char*)*(char**)&TMP$985$1, 0ll );
	fb_StrDelete( (FBSTRING*)&TMP$985$1 );
	label$1677:;
}

void EMITVARINIOFS( char* SNAME$1, int64 OFS$1 )
{
	FBSTRING TMP$987$1;
	label$1678:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)".int ", 6ll, 0 );
	__builtin_memset( &TMP$987$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$987$1, (void*)&OSTR$1, -1ll, (void*)SNAME$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	if( OFS$1 == 0ll) goto label$1681;
	{
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)" + ", 4ll, 0 );
		FBSTRING* vr$3 = fb_LongintToStr( OFS$1 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$3, -1ll, 0 );
	}
	label$1681:;
	label$1680:;
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A", 2ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$1679:;
}

void EMITVARINISTR( char* S$1, int64 NOTERM$1 )
{
	FBSTRING TMP$988$1;
	label$1682:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)".ascii \x22", 9ll, 0 );
	__builtin_memset( &TMP$988$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$988$1, (void*)&OSTR$1, -1ll, (void*)S$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	if( NOTERM$1 != 0ll) goto label$1685;
	{
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x5C" "0", 3ll, 0 );
	}
	label$1685:;
	label$1684:;
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x22\x0A", 3ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$1683:;
}

void EMITVARINIWSTR( char* S$1 )
{
	FBSTRING TMP$989$1;
	label$1686:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)".ascii \x22", 9ll, 0 );
	__builtin_memset( &TMP$989$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$989$1, (void*)&OSTR$1, -1ll, (void*)S$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	{
		int64 I$2;
		I$2 = 1ll;
		int64 TMP$990$2;
		TMP$990$2 = *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + 400ll);
		goto label$1688;
		label$1691:;
		{
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x5C" "0", 3ll, 0 );
		}
		label$1689:;
		I$2 = I$2 + 1ll;
		label$1688:;
		if( I$2 <= TMP$990$2) goto label$1691;
		label$1690:;
	}
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x22\x0A", 3ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$1687:;
}

void EMITVARINIPAD( int64 BYTES$1, int64 FILLCHAR$1 )
{
	FBSTRING TMP$992$1;
	FBSTRING TMP$993$1;
	FBSTRING TMP$994$1;
	FBSTRING TMP$995$1;
	FBSTRING TMP$996$1;
	label$1692:;
	__builtin_memset( &TMP$996$1, 0, 24ll );
	FBSTRING* vr$1 = fb_LongintToStr( FILLCHAR$1 );
	FBSTRING* vr$2 = fb_LongintToStr( BYTES$1 );
	__builtin_memset( &TMP$992$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$992$1, (void*)".skip ", 7ll, (void*)vr$2, -1ll );
	__builtin_memset( &TMP$993$1, 0, 24ll );
	FBSTRING* vr$8 = fb_StrConcat( &TMP$993$1, (void*)vr$5, -1ll, (void*)",", 2ll );
	__builtin_memset( &TMP$994$1, 0, 24ll );
	FBSTRING* vr$11 = fb_StrConcat( &TMP$994$1, (void*)vr$8, -1ll, (void*)vr$1, -1ll );
	__builtin_memset( &TMP$995$1, 0, 24ll );
	FBSTRING* vr$14 = fb_StrConcat( &TMP$995$1, (void*)vr$11, -1ll, (void*)"\x0A", 2ll );
	fb_StrAssign( (void*)&TMP$996$1, -1ll, (void*)vr$14, -1ll, 0 );
	OUTEX( (char*)*(char**)&TMP$996$1, 0ll );
	fb_StrDelete( (FBSTRING*)&TMP$996$1 );
	label$1693:;
}

void EMITFBCTINFBEGIN( void )
{
	label$1694:;
	_SETSECTION( 7ll, 0ll );
	label$1695:;
}

void EMITFBCTINFSTRING( char* S$1 )
{
	FBSTRING TMP$999$1;
	FBSTRING TMP$1000$1;
	FBSTRING TMP$1001$1;
	label$1696:;
	static FBSTRING LN$1;
	char* vr$0 = (*(tmp$105*)((uint8*)&EMIT$ + 456ll))( 4ll );
	fb_StrAssign( (void*)&LN$1, -1ll, (void*)vr$0, 0ll, 0 );
	__builtin_memset( &TMP$999$1, 0, 24ll );
	FBSTRING* vr$3 = fb_StrConcat( &TMP$999$1, (void*)" \x22", 3ll, (void*)S$1, 0ll );
	__builtin_memset( &TMP$1000$1, 0, 24ll );
	FBSTRING* vr$6 = fb_StrConcat( &TMP$1000$1, (void*)vr$3, -1ll, (void*)"\x5C" "0\x22", 4ll );
	__builtin_memset( &TMP$1001$1, 0, 24ll );
	FBSTRING* vr$9 = fb_StrConcat( &TMP$1001$1, (void*)&LN$1, -1ll, (void*)vr$6, -1ll );
	fb_StrAssign( (void*)&LN$1, -1ll, (void*)vr$9, -1ll, 0 );
	EMITWRITESTR( (char*)*(char**)&LN$1, 0ll );
	label$1697:;
}

void EMITFBCTINFEND( void )
{
	label$1698:;
	EMITWRITESTR( (char*)"", 0ll );
	label$1699:;
}

int64 EMITGASX86_CTOR( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1870:;
	static struct $9EMIT_VTBL _VTBL$1 = { (tmp$40)&_INIT, (tmp$39)&_END, (tmp$41)&_GETOPTIONVALUE, (tmp$40)&_OPEN, (tmp$39)&_CLOSE, (tmp$42)&_ISREGPRESERVED, (tmp$42)&_GETFREEPRESERVEDREG, (tmp$101)&_GETARGREG, (tmp$102)&_GETRESULTREG, (tmp$45)&_PROCGETFRAMEREGNAME, (tmp$43)&_PROCBEGIN, (tmp$43)&_PROCEND, (tmp$44)&_PROCHEADER, (tmp$103)&_PROCFOOTER, (tmp$44)&_PROCALLOCARG, (tmp$44)&_PROCALLOCLOCAL, (tmp$43)&_PROCALLOCSTATICVARS, (tmp$43)&_SCOPEBEGIN, (tmp$43)&_SCOPEEND, (tmp$104)&_SETSECTION, (tmp$105)&_GETTYPESTRING, (tmp$106)&_GETSECTIONSTRING };
	__builtin_memcpy( (struct $9EMIT_VTBL*)((uint8*)&EMIT$ + 296ll), &_VTBL$1, 176 );
	*(void***)((uint8*)&EMIT$ + 472ll) = (void**)_OPFNTB$;
	if( *(int64*)((uint8*)&ENV$ + 232ll) < 1ll) goto label$1873;
	{
		_INIT_OPFNTB_SSE( *(void***)((uint8*)&EMIT$ + 472ll) );
	}
	label$1873:;
	label$1872:;
	fb$result$1 = -1ll;
	label$1871:;
	return fb$result$1;
}

__attribute__(( constructor )) static void fb_ctor__emit_x86( void )
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

static char* HGETIDXNAME( struct $6IRVREG* VREG$1 )
{
	FBSTRING TMP$150$1;
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$119:;
	static char INAME$1[154];
	static struct $8FBSYMBOL* SYM$1;
	static struct $6IRVREG* VI$1;
	static int64 ADDONE$1;
	static int64 MULT$1;
	static char* RNAME$1;
	SYM$1 = *(struct $8FBSYMBOL**)((uint8*)VREG$1 + 56ll);
	VI$1 = *(struct $6IRVREG**)((uint8*)VREG$1 + 80ll);
	if( SYM$1 != (struct $8FBSYMBOL*)0ull) goto label$122;
	{
		if( VI$1 != (struct $6IRVREG*)0ull) goto label$124;
		{
			fb$result$1 = (char*)0ull;
			goto label$120;
		}
		label$124:;
		label$123:;
		fb_StrAssign( (void*)INAME$1, 154ll, (void*)"", 1ll, 0 );
	}
	goto label$121;
	label$122:;
	{
		char* vr$3 = SYMBGETMANGLEDNAME( SYM$1 );
		fb_StrAssign( (void*)INAME$1, 154ll, (void*)vr$3, 0ll, 0 );
		if( VI$1 == (struct $6IRVREG*)0ull) goto label$126;
		{
			FBSTRING TMP$149$3;
			__builtin_memset( &TMP$149$3, 0, 24ll );
			FBSTRING* vr$6 = fb_StrConcat( &TMP$149$3, (void*)INAME$1, 154ll, (void*)"+", 2ll );
			fb_StrAssign( (void*)INAME$1, 154ll, (void*)vr$6, -1ll, 0 );
		}
		label$126:;
		label$125:;
	}
	label$121:;
	char* vr$9 = HGETREGNAME( *(int64*)((uint8*)VI$1 + 8ll), *(int64*)((uint8*)VI$1 + 24ll) );
	RNAME$1 = vr$9;
	__builtin_memset( &TMP$150$1, 0, 24ll );
	FBSTRING* vr$12 = fb_StrConcat( &TMP$150$1, (void*)INAME$1, 154ll, (void*)RNAME$1, 0ll );
	fb_StrAssign( (void*)INAME$1, 154ll, (void*)vr$12, -1ll, 0 );
	if( VI$1 == (struct $6IRVREG*)0ull) goto label$128;
	{
		MULT$1 = *(int64*)((uint8*)VREG$1 + 72ll);
		if( MULT$1 <= 1ll) goto label$130;
		{
			FBSTRING TMP$152$3;
			FBSTRING TMP$153$3;
			ADDONE$1 = 0ll;
			{
				if( MULT$1 == 3ll) goto label$133;
				label$134:;
				if( MULT$1 == 5ll) goto label$133;
				label$135:;
				if( MULT$1 != 9ll) goto label$132;
				label$133:;
				{
					MULT$1 = MULT$1 + -1ll;
					ADDONE$1 = -1ll;
				}
				label$132:;
				label$131:;
			}
			__builtin_memset( &TMP$152$3, 0, 24ll );
			FBSTRING* vr$17 = fb_StrConcat( &TMP$152$3, (void*)INAME$1, 154ll, (void*)"*", 2ll );
			fb_StrAssign( (void*)INAME$1, 154ll, (void*)vr$17, -1ll, 0 );
			FBSTRING* vr$18 = fb_LongintToStr( MULT$1 );
			__builtin_memset( &TMP$153$3, 0, 24ll );
			FBSTRING* vr$21 = fb_StrConcat( &TMP$153$3, (void*)INAME$1, 154ll, (void*)vr$18, -1ll );
			fb_StrAssign( (void*)INAME$1, 154ll, (void*)vr$21, -1ll, 0 );
			if( ADDONE$1 == 0ll) goto label$137;
			{
				FBSTRING TMP$154$4;
				FBSTRING TMP$155$4;
				__builtin_memset( &TMP$154$4, 0, 24ll );
				FBSTRING* vr$24 = fb_StrConcat( &TMP$154$4, (void*)INAME$1, 154ll, (void*)"+", 2ll );
				fb_StrAssign( (void*)INAME$1, 154ll, (void*)vr$24, -1ll, 0 );
				__builtin_memset( &TMP$155$4, 0, 24ll );
				FBSTRING* vr$27 = fb_StrConcat( &TMP$155$4, (void*)INAME$1, 154ll, (void*)RNAME$1, 0ll );
				fb_StrAssign( (void*)INAME$1, 154ll, (void*)vr$27, -1ll, 0 );
			}
			label$137:;
			label$136:;
		}
		label$130:;
		label$129:;
	}
	label$128:;
	label$127:;
	fb$result$1 = (char*)INAME$1;
	label$120:;
	return fb$result$1;
}

static void OUTEX( char* S$1, int64 BYTES$1 )
{
	label$178:;
	if( BYTES$1 != 0ll) goto label$181;
	{
		int64 vr$0 = fb_StrLen( (void*)S$1, 0ll );
		BYTES$1 = vr$0;
	}
	label$181:;
	label$180:;
	int32 vr$2 = fb_FilePutStrLarge( (int32)*(int64*)((uint8*)&ENV$ + 928ll), 0ll, (void*)S$1, 0ll );
	if( (int64)vr$2 != 0ll) goto label$183;
	{
	}
	label$183:;
	label$182:;
	label$179:;
}

static void HXCHG( char* DNAME$1, char* SNAME$1 )
{
	FBSTRING TMP$179$1;
	FBSTRING TMP$180$1;
	label$196:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xchg ", 6ll, 0 );
	__builtin_memset( &TMP$179$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$179$1, (void*)&OSTR$1, -1ll, (void*)DNAME$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	__builtin_memset( &TMP$180$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$180$1, (void*)&OSTR$1, -1ll, (void*)SNAME$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$5, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$197:;
}

static void HPUBLIC( char* LABEL$1, int64 ISEXPORT$1 )
{
	FBSTRING TMP$186$1;
	label$200:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A.globl ", 9ll, 0 );
	__builtin_memset( &TMP$186$1, 0, 24ll );
	FBSTRING* vr$2 = fb_StrConcat( &TMP$186$1, (void*)&OSTR$1, -1ll, (void*)LABEL$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$2, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A", 2ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$201:;
}

static void HALIGN( int64 BYTES$1 )
{
	label$204:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)".balign ", 9ll, 0 );
	FBSTRING* vr$0 = fb_LongintToStr( BYTES$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$0, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A", 2ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$205:;
}

static void HINITREGTB( void )
{
	label$206:;
	int64 LASTCLASS$1;
	__builtin_memset( &LASTCLASS$1, 0, 8ll );
	int64 REGS$1;
	__builtin_memset( &REGS$1, 0, 8ll );
	int64 I$1;
	__builtin_memset( &I$1, 0, 8ll );
	static $12REG_SIZEMASK INT_BITSMASK$1[6] = { 7ll, 6ll, 6ll, 7ll, 7ll, 7ll };
	static struct $8FBARRAY1I12REG_SIZEMASKE tmp$190$1 = { ($12REG_SIZEMASK*)INT_BITSMASK$1, ($12REG_SIZEMASK*)INT_BITSMASK$1, 48ll, 8ll, 1ll, 49ll, { { 6ll, 0ll, 5ll } } };
	struct $8REGCLASS* vr$3 = REGNEWCLASS( 0ll, 6ll, (struct $7FBARRAYI12REG_SIZEMASKE*)&tmp$190$1, 0ll );
	*(struct $8REGCLASS**)((uint8*)&EMIT$ + 16ll) = vr$3;
	static $12REG_SIZEMASK FLT_BITSMASK$1[7] = { 12ll, 12ll, 12ll, 12ll, 12ll, 12ll, 12ll };
	static struct $8FBARRAY1I12REG_SIZEMASKE tmp$191$1 = { ($12REG_SIZEMASK*)FLT_BITSMASK$1, ($12REG_SIZEMASK*)FLT_BITSMASK$1, 56ll, 8ll, 1ll, 49ll, { { 7ll, 0ll, 6ll } } };
	if( *(int64*)((uint8*)&ENV$ + 232ll) != 1ll) goto label$209;
	{
		struct $8REGCLASS* vr$4 = REGNEWCLASS( 1ll, 7ll, (struct $7FBARRAYI12REG_SIZEMASKE*)&tmp$191$1, 0ll );
		*(struct $8REGCLASS**)((uint8*)&EMIT$ + 24ll) = vr$4;
		{
			I$1 = 0ll;
			label$213:;
			{
				FBSTRING TMP$193$4;
				FBSTRING* vr$5 = fb_LongintToStr( I$1 );
				__builtin_memset( &TMP$193$4, 0, 24ll );
				FBSTRING* vr$8 = fb_StrConcat( &TMP$193$4, (void*)"xmm", 4ll, (void*)vr$5, -1ll );
				fb_StrAssign( (void*)(((int64)(char*)RNAMETB$ + (I$1 << (3ll & 63ll))) + 192ll), 8ll, (void*)vr$8, -1ll, 0 );
			}
			label$211:;
			I$1 = I$1 + 1ll;
			label$210:;
			if( I$1 <= 7ll) goto label$213;
			label$212:;
		}
	}
	goto label$208;
	label$209:;
	{
		struct $8REGCLASS* vr$12 = REGNEWCLASS( 1ll, 7ll, (struct $7FBARRAYI12REG_SIZEMASKE*)&tmp$191$1, -1ll );
		*(struct $8REGCLASS**)((uint8*)&EMIT$ + 24ll) = vr$12;
	}
	label$208:;
	label$207:;
}

static void HENDREGTB( void )
{
	label$214:;
	int64 I$1;
	__builtin_memset( &I$1, 0, 8ll );
	{
		I$1 = 0ll;
		label$219:;
		{
			REGDELCLASS( *(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (I$1 << (3ll & 63ll))) + 16ll) );
		}
		label$217:;
		I$1 = I$1 + 1ll;
		label$216:;
		if( I$1 <= 1ll) goto label$219;
		label$218:;
	}
	label$215:;
}

static int64 HGETGLOBALTYPEALIGN( int64 DTYPE$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$220:;
	if( DTYPE$1 != 16ll) goto label$223;
	{
		fb$result$1 = 8ll;
	}
	goto label$222;
	label$223:;
	{
		fb$result$1 = 4ll;
	}
	label$222:;
	label$221:;
	return fb$result$1;
}

static int64 HGETGLOBALVARALIGN( struct $8FBSYMBOL* SYM$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$224:;
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 262144ll) == 0ll) goto label$227;
	{
		fb$result$1 = 4ll;
	}
	goto label$226;
	label$227:;
	{
		int64 vr$5 = HGETGLOBALTYPEALIGN( *(int64*)((uint8*)SYM$1 + 56ll) & 511ll );
		fb$result$1 = vr$5;
	}
	label$226:;
	label$225:;
	return fb$result$1;
}

static void HEMITVARBSS( struct $8FBSYMBOL* S$1 )
{
	FBSTRING TMP$196$1;
	label$228:;
	static FBSTRING ALLOC$1;
	static FBSTRING OSTR$1;
	static int64 ATTRIB$1;
	ATTRIB$1 = *(int64*)((uint8*)S$1 + 8ll);
	_SETSECTION( 2ll, 0ll );
	if( (ATTRIB$1 & 8ll) != 0ll) goto label$231;
	{
		if( (ATTRIB$1 & 32ll) <= 0ll) goto label$233;
		{
			char* vr$3 = SYMBGETMANGLEDNAME( S$1 );
			HPUBLIC( vr$3, -1ll );
		}
		label$233:;
		label$232:;
		fb_StrAssign( (void*)&ALLOC$1, -1ll, (void*)".lcomm", 7ll, 0 );
	}
	goto label$230;
	label$231:;
	{
		char* vr$4 = SYMBGETMANGLEDNAME( S$1 );
		HPUBLIC( vr$4, 0ll );
		fb_StrAssign( (void*)&ALLOC$1, -1ll, (void*)".comm", 6ll, 0 );
	}
	label$230:;
	int64 vr$5 = HGETGLOBALVARALIGN( S$1 );
	HALIGN( vr$5 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&ALLOC$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x09", 2ll, 0 );
	char* vr$6 = SYMBGETMANGLEDNAME( S$1 );
	__builtin_memset( &TMP$196$1, 0, 24ll );
	FBSTRING* vr$9 = fb_StrConcat( &TMP$196$1, (void*)&OSTR$1, -1ll, (void*)vr$6, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$9, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)",", 2ll, 0 );
	int64 vr$10 = SYMBGETREALSIZE( S$1 );
	FBSTRING* vr$11 = fb_LongintToStr( vr$10 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$11, -1ll, 0 );
	EMITWRITESTR( (char*)*(char**)&OSTR$1, -1ll );
	EDBGEMITGLOBALVAR( S$1, 2ll );
	label$229:;
}

static void HWRITEHEADER( void )
{
	label$234:;
	EDBGEMITHEADER( (char*)((uint8*)&ENV$ + 640ll) );
	EMITWRITESTR( (char*)".intel_syntax noprefix", -1ll );
	label$235:;
}

static void HEMITVARCONST( struct $8FBSYMBOL* S$1 )
{
	label$236:;
	static FBSTRING STEXT$1;
	static FBSTRING STYPE$1;
	static FBSTRING OSTR$1;
	static int64 DTYPE$1;
	DTYPE$1 = *(int64*)((uint8*)S$1 + 56ll) & 511ll;
	{
		uint64 TMP$199$2;
		TMP$199$2 = (uint64)DTYPE$1;
		goto label$239;
		label$240:;
		{
			FBSTRING TMP$201$3;
			fb_StrAssign( (void*)&STEXT$1, -1ll, (void*)"\x22", 2ll, 0 );
			char* vr$3 = HESCAPE( (char*)*(char**)((uint8*)S$1 + 96ll), 0ll );
			__builtin_memset( &TMP$201$3, 0, 24ll );
			FBSTRING* vr$6 = fb_StrConcat( &TMP$201$3, (void*)&STEXT$1, -1ll, (void*)vr$3, 0ll );
			fb_StrAssign( (void*)&STEXT$1, -1ll, (void*)vr$6, -1ll, 0 );
			fb_StrConcatAssign( (void*)&STEXT$1, -1ll, (void*)"\x5C" "0\x22", 4ll, 0 );
		}
		goto label$238;
		label$241:;
		{
			FBSTRING TMP$205$3;
			fb_StrAssign( (void*)&STEXT$1, -1ll, (void*)"\x22", 2ll, 0 );
			char* vr$8 = HESCAPEW( (uint32*)*(uint32**)((uint8*)S$1 + 96ll), 0ll );
			__builtin_memset( &TMP$205$3, 0, 24ll );
			FBSTRING* vr$11 = fb_StrConcat( &TMP$205$3, (void*)&STEXT$1, -1ll, (void*)vr$8, 0ll );
			fb_StrAssign( (void*)&STEXT$1, -1ll, (void*)vr$11, -1ll, 0 );
			{
				int64 I$4;
				I$4 = 1ll;
				int64 TMP$206$4;
				TMP$206$4 = *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + 400ll);
				goto label$242;
				label$245:;
				{
					fb_StrConcatAssign( (void*)&STEXT$1, -1ll, (void*)"\x5C" "0", 3ll, 0 );
				}
				label$243:;
				I$4 = I$4 + 1ll;
				label$242:;
				if( I$4 <= TMP$206$4) goto label$245;
				label$244:;
			}
			fb_StrConcatAssign( (void*)&STEXT$1, -1ll, (void*)"\x22", 2ll, 0 );
		}
		goto label$238;
		label$246:;
		{
			fb_StrAssign( (void*)&STEXT$1, -1ll, *(void**)((uint8*)S$1 + 96ll), 0ll, 0 );
		}
		goto label$238;
		label$239:;
		static const void* tmp$1057[4ll] = {
			&&label$240,
			&&label$246,
			&&label$246,
			&&label$241,
		};
		if( (TMP$199$2 - 4ull) > 3ull ) goto label$246;
		goto *tmp$1057[TMP$199$2 - 4ull];
		label$238:;
	}
	if( *(int64*)((uint8*)&ENV$ + 216ll) != 2ll) goto label$248;
	{
		_SETSECTION( 0ll, 0ll );
	}
	goto label$247;
	label$248:;
	{
		_SETSECTION( 1ll, 0ll );
	}
	label$247:;
	if( *(int64*)((uint8*)S$1 + 168ll) == 0ll) goto label$250;
	{
		HALIGN( *(int64*)((uint8*)S$1 + 168ll) );
	}
	goto label$249;
	label$250:;
	{
		int64 vr$16 = HGETGLOBALTYPEALIGN( DTYPE$1 );
		HALIGN( vr$16 );
	}
	label$249:;
	char* vr$17 = _GETTYPESTRING( DTYPE$1 );
	fb_StrAssign( (void*)&STYPE$1, -1ll, (void*)vr$17, 0ll, 0 );
	char* vr$18 = SYMBGETMANGLEDNAME( S$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$18, 0ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)":\x09", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&STYPE$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x09", 2ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&STEXT$1, -1ll, 0 );
	EMITWRITESTR( (char*)*(char**)&OSTR$1, 0ll );
	label$237:;
}

static void HWRITECTOR( struct $20FB_GLOBCTORLIST_ITEM* PROC_HEAD$1, int64 IS_CTOR$1 )
{
	label$251:;
	if( PROC_HEAD$1 != (struct $20FB_GLOBCTORLIST_ITEM*)0ull) goto label$254;
	{
		goto label$252;
	}
	label$254:;
	label$253:;
	label$255:;
	{
		if( (*(int64*)((uint8*)*(struct $8FBSYMBOL**)PROC_HEAD$1 + 24ll) & 16777216ll) == 0ll) goto label$259;
		{
			$10IR_SECTION TMP$208$3;
			if( IS_CTOR$1 == 0ll) goto label$260;
			TMP$208$3 = 5ll;
			goto label$1874;
			label$260:;
			TMP$208$3 = 6ll;
			label$1874:;
			_SETSECTION( TMP$208$3, *(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)*(struct $8FBSYMBOL**)PROC_HEAD$1 + 208ll) + 112ll) );
			char* vr$7 = SYMBGETMANGLEDNAME( *(struct $8FBSYMBOL**)PROC_HEAD$1 );
			EMITVARINIOFS( vr$7, 0ll );
		}
		label$259:;
		label$258:;
		PROC_HEAD$1 = *(struct $20FB_GLOBCTORLIST_ITEM**)((uint8*)PROC_HEAD$1 + 8ll);
	}
	label$257:;
	if( PROC_HEAD$1 != (struct $20FB_GLOBCTORLIST_ITEM*)0ull) goto label$255;
	label$256:;
	label$252:;
}

static void HEMITEXPORT( struct $8FBSYMBOL* S$1 )
{
	label$261:;
	if( (*(int64*)((uint8*)S$1 + 8ll) & 256ll) == 0ll) goto label$264;
	{
		FBSTRING TMP$214$2;
		FBSTRING TMP$215$2;
		FBSTRING TMP$216$2;
		_SETSECTION( 4ll, 0ll );
		char* SNAME$2;
		char* vr$2 = SYMBGETMANGLEDNAME( S$1 );
		SNAME$2 = vr$2;
		if( *(int64*)((uint8*)&ENV$ + 616ll) == 0ll) goto label$266;
		{
			SNAME$2 = (char*)((uint8*)SNAME$2 + 1ll);
		}
		label$266:;
		label$265:;
		__builtin_memset( &TMP$216$2, 0, 24ll );
		__builtin_memset( &TMP$214$2, 0, 24ll );
		FBSTRING* vr$7 = fb_StrConcat( &TMP$214$2, (void*)".ascii \x22 -export:", 18ll, (void*)SNAME$2, 0ll );
		__builtin_memset( &TMP$215$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$215$2, (void*)vr$7, -1ll, (void*)"\x22\x0A", 3ll );
		fb_StrAssign( (void*)&TMP$216$2, -1ll, (void*)vr$10, -1ll, 0 );
		EMITWRITESTR( (char*)*(char**)&TMP$216$2, -1ll );
		fb_StrDelete( (FBSTRING*)&TMP$216$2 );
	}
	label$264:;
	label$263:;
	label$262:;
}

static void HDECLVARIABLE( struct $8FBSYMBOL* S$1 )
{
	label$267:;
	if( (*(int64*)((uint8*)S$1 + 8ll) & 1024ll) == 0ll) goto label$270;
	{
		{
			int64 TMP$217$3;
			TMP$217$3 = *(int64*)((uint8*)S$1 + 56ll) & 511ll;
			if( TMP$217$3 == 4ll) goto label$273;
			label$274:;
			if( TMP$217$3 != 7ll) goto label$272;
			label$273:;
			{
				if( (*(int64*)((uint8*)S$1 + 24ll) & 2ll) != 0ll) goto label$276;
				{
					goto label$268;
				}
				label$276:;
				label$275:;
			}
			label$272:;
			label$271:;
		}
		HEMITVARCONST( S$1 );
		goto label$268;
	}
	label$270:;
	label$269:;
	if( (*(int64*)((uint8*)S$1 + 24ll) & 32768ll) == 0ll) goto label$278;
	{
		goto label$268;
	}
	label$278:;
	label$277:;
	if( ((int64)-((*(int64*)((uint8*)S$1 + 8ll) & 16ll) != 0ll) | (int64)-((*(int64*)((uint8*)S$1 + 8ll) & 4ll) != 0ll)) == 0ll) goto label$280;
	{
		goto label$268;
	}
	label$280:;
	label$279:;
	if( *(struct $7ASTNODE**)((uint8*)S$1 + 96ll) == (struct $7ASTNODE*)0ull) goto label$282;
	{
		if( (*(int64*)((uint8*)S$1 + 24ll) & 2ll) != 0ll) goto label$284;
		{
			if( (*(int64*)((uint8*)S$1 + 8ll) & 32ll) != 0ll) goto label$286;
			{
				goto label$268;
			}
			label$286:;
			label$285:;
		}
		label$284:;
		label$283:;
		_SETSECTION( 1ll, 0ll );
		IRHLFLUSHSTATICINITIALIZER( S$1 );
		goto label$268;
	}
	label$282:;
	label$281:;
	HEMITVARBSS( S$1 );
	label$268:;
}

static void HCLEARLOCALS( int64 BYTESTOCLEAR$1, int64 BASEOFFSET$1 )
{
	label$287:;
	static int64 I$1;
	static FBSTRING LNAME$1;
	if( BYTESTOCLEAR$1 != 0ll) goto label$290;
	{
		goto label$288;
	}
	label$290:;
	label$289:;
	if( *(int64*)((uint8*)&ENV$ + 224ll) < 3ll) goto label$292;
	{
		if( ((uint64)BYTESTOCLEAR$1 >> (3ull & 63ll)) <= 7ull) goto label$294;
		{
			FBSTRING TMP$219$3;
			FBSTRING TMP$220$3;
			FBSTRING TMP$221$3;
			FBSTRING TMP$223$3;
			FBSTRING TMP$224$3;
			FBSTRING TMP$230$3;
			FBSTRING TMP$231$3;
			if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 2ll) != 0ll) goto label$296;
			{
				HPUSH( (char*)"edi" );
			}
			label$296:;
			label$295:;
			__builtin_memset( &TMP$221$3, 0, 24ll );
			FBSTRING* vr$4 = fb_LongintToStr( BASEOFFSET$1 + BYTESTOCLEAR$1 );
			__builtin_memset( &TMP$219$3, 0, 24ll );
			FBSTRING* vr$7 = fb_StrConcat( &TMP$219$3, (void*)"lea edi, [ebp-", 15ll, (void*)vr$4, -1ll );
			__builtin_memset( &TMP$220$3, 0, 24ll );
			FBSTRING* vr$10 = fb_StrConcat( &TMP$220$3, (void*)vr$7, -1ll, (void*)"]", 2ll );
			fb_StrAssign( (void*)&TMP$221$3, -1ll, (void*)vr$10, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$221$3 );
			fb_StrDelete( (FBSTRING*)&TMP$221$3 );
			__builtin_memset( &TMP$224$3, 0, 24ll );
			FBSTRING* vr$15 = fb_ULongintToStr( (uint64)BYTESTOCLEAR$1 >> (3ull & 63ll) );
			__builtin_memset( &TMP$223$3, 0, 24ll );
			FBSTRING* vr$18 = fb_StrConcat( &TMP$223$3, (void*)"mov ecx,", 9ll, (void*)vr$15, -1ll );
			fb_StrAssign( (void*)&TMP$224$3, -1ll, (void*)vr$18, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$224$3 );
			fb_StrDelete( (FBSTRING*)&TMP$224$3 );
			OUTP( (char*)"pxor mm0, mm0" );
			char* vr$21 = SYMBUNIQUELABEL(  );
			fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$21, 0ll, 0 );
			HLABEL( (char*)*(char**)&LNAME$1 );
			OUTP( (char*)"movq [edi], mm0" );
			OUTP( (char*)"add edi, 8" );
			OUTP( (char*)"dec ecx" );
			__builtin_memset( &TMP$231$3, 0, 24ll );
			__builtin_memset( &TMP$230$3, 0, 24ll );
			FBSTRING* vr$25 = fb_StrConcat( &TMP$230$3, (void*)"jnz ", 5ll, (void*)&LNAME$1, -1ll );
			fb_StrAssign( (void*)&TMP$231$3, -1ll, (void*)vr$25, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$231$3 );
			fb_StrDelete( (FBSTRING*)&TMP$231$3 );
			OUTP( (char*)"emms" );
			if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 2ll) != 0ll) goto label$298;
			{
				HPOP( (char*)"edi" );
			}
			label$298:;
			label$297:;
		}
		goto label$293;
		label$294:;
		if( ((uint64)BYTESTOCLEAR$1 >> (3ull & 63ll)) <= 0ull) goto label$299;
		{
			OUTP( (char*)"pxor mm0, mm0" );
			{
				I$1 = (int64)((uint64)BYTESTOCLEAR$1 >> (3ull & 63ll));
				goto label$300;
				label$303:;
				{
					FBSTRING TMP$235$5;
					FBSTRING TMP$236$5;
					FBSTRING TMP$237$5;
					__builtin_memset( &TMP$237$5, 0, 24ll );
					FBSTRING* vr$33 = fb_LongintToStr( I$1 << (3ll & 63ll) );
					__builtin_memset( &TMP$235$5, 0, 24ll );
					FBSTRING* vr$36 = fb_StrConcat( &TMP$235$5, (void*)"movq [ebp-", 11ll, (void*)vr$33, -1ll );
					__builtin_memset( &TMP$236$5, 0, 24ll );
					FBSTRING* vr$39 = fb_StrConcat( &TMP$236$5, (void*)vr$36, -1ll, (void*)"], mm0", 7ll );
					fb_StrAssign( (void*)&TMP$237$5, -1ll, (void*)vr$39, -1ll, 0 );
					OUTP( (char*)*(char**)&TMP$237$5 );
					fb_StrDelete( (FBSTRING*)&TMP$237$5 );
				}
				label$301:;
				I$1 = I$1 + -1ll;
				label$300:;
				if( I$1 >= 1ll) goto label$303;
				label$302:;
			}
			OUTP( (char*)"emms" );
		}
		label$299:;
		label$293:;
		if( (BYTESTOCLEAR$1 & 4ll) == 0ll) goto label$305;
		{
			FBSTRING TMP$240$3;
			FBSTRING TMP$241$3;
			FBSTRING TMP$242$3;
			__builtin_memset( &TMP$242$3, 0, 24ll );
			FBSTRING* vr$46 = fb_LongintToStr( BASEOFFSET$1 + BYTESTOCLEAR$1 );
			__builtin_memset( &TMP$240$3, 0, 24ll );
			FBSTRING* vr$49 = fb_StrConcat( &TMP$240$3, (void*)"mov dword ptr [ebp-", 20ll, (void*)vr$46, -1ll );
			__builtin_memset( &TMP$241$3, 0, 24ll );
			FBSTRING* vr$52 = fb_StrConcat( &TMP$241$3, (void*)vr$49, -1ll, (void*)"], 0", 5ll );
			fb_StrAssign( (void*)&TMP$242$3, -1ll, (void*)vr$52, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$242$3 );
			fb_StrDelete( (FBSTRING*)&TMP$242$3 );
		}
		label$305:;
		label$304:;
		goto label$288;
	}
	label$292:;
	label$291:;
	if( ((uint64)BYTESTOCLEAR$1 >> (2ull & 63ll)) <= 6ull) goto label$307;
	{
		FBSTRING TMP$243$2;
		FBSTRING TMP$244$2;
		FBSTRING TMP$245$2;
		FBSTRING TMP$246$2;
		FBSTRING TMP$247$2;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 2ll) != 0ll) goto label$309;
		{
			HPUSH( (char*)"edi" );
		}
		label$309:;
		label$308:;
		__builtin_memset( &TMP$245$2, 0, 24ll );
		FBSTRING* vr$59 = fb_LongintToStr( BASEOFFSET$1 + BYTESTOCLEAR$1 );
		__builtin_memset( &TMP$243$2, 0, 24ll );
		FBSTRING* vr$62 = fb_StrConcat( &TMP$243$2, (void*)"lea edi, [ebp-", 15ll, (void*)vr$59, -1ll );
		__builtin_memset( &TMP$244$2, 0, 24ll );
		FBSTRING* vr$65 = fb_StrConcat( &TMP$244$2, (void*)vr$62, -1ll, (void*)"]", 2ll );
		fb_StrAssign( (void*)&TMP$245$2, -1ll, (void*)vr$65, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$245$2 );
		fb_StrDelete( (FBSTRING*)&TMP$245$2 );
		__builtin_memset( &TMP$247$2, 0, 24ll );
		FBSTRING* vr$70 = fb_ULongintToStr( (uint64)BYTESTOCLEAR$1 >> (2ull & 63ll) );
		__builtin_memset( &TMP$246$2, 0, 24ll );
		FBSTRING* vr$73 = fb_StrConcat( &TMP$246$2, (void*)"mov ecx,", 9ll, (void*)vr$70, -1ll );
		fb_StrAssign( (void*)&TMP$247$2, -1ll, (void*)vr$73, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$247$2 );
		fb_StrDelete( (FBSTRING*)&TMP$247$2 );
		OUTP( (char*)"xor eax, eax" );
		OUTP( (char*)"rep stosd" );
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 2ll) != 0ll) goto label$311;
		{
			HPOP( (char*)"edi" );
		}
		label$311:;
		label$310:;
	}
	goto label$306;
	label$307:;
	{
		{
			I$1 = (int64)((uint64)BYTESTOCLEAR$1 >> (2ull & 63ll));
			goto label$312;
			label$315:;
			{
				FBSTRING TMP$250$4;
				FBSTRING TMP$251$4;
				FBSTRING TMP$252$4;
				__builtin_memset( &TMP$252$4, 0, 24ll );
				FBSTRING* vr$81 = fb_LongintToStr( BASEOFFSET$1 + (I$1 << (2ll & 63ll)) );
				__builtin_memset( &TMP$250$4, 0, 24ll );
				FBSTRING* vr$84 = fb_StrConcat( &TMP$250$4, (void*)"mov dword ptr [ebp-", 20ll, (void*)vr$81, -1ll );
				__builtin_memset( &TMP$251$4, 0, 24ll );
				FBSTRING* vr$87 = fb_StrConcat( &TMP$251$4, (void*)vr$84, -1ll, (void*)"], 0", 5ll );
				fb_StrAssign( (void*)&TMP$252$4, -1ll, (void*)vr$87, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$252$4 );
				fb_StrDelete( (FBSTRING*)&TMP$252$4 );
			}
			label$313:;
			I$1 = I$1 + -1ll;
			label$312:;
			if( I$1 >= 1ll) goto label$315;
			label$314:;
		}
	}
	label$306:;
	label$288:;
}

static int64 HFRAMEBYTESTOALLOC( struct $8FBSYMBOL* PROC$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$316:;
	static int64 BYTESTOALLOC$1;
	static int64 BYTESPUSHED$1;
	BYTESTOALLOC$1 = (*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 24ll) + 3ll) & -4ll;
	if( (*(int64*)((uint8*)&ENV$ + 600ll) & 64ll) == 0ll) goto label$319;
	{
		BYTESPUSHED$1 = 8ll;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 2ll) == 0ll) goto label$321;
		{
			BYTESPUSHED$1 = BYTESPUSHED$1 + 4ll;
		}
		label$321:;
		label$320:;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 4ll) == 0ll) goto label$323;
		{
			BYTESPUSHED$1 = BYTESPUSHED$1 + 4ll;
		}
		label$323:;
		label$322:;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 16ll) == 0ll) goto label$325;
		{
			BYTESPUSHED$1 = BYTESPUSHED$1 + 4ll;
		}
		label$325:;
		label$324:;
		BYTESTOALLOC$1 = BYTESTOALLOC$1 + BYTESPUSHED$1;
		BYTESTOALLOC$1 = (BYTESTOALLOC$1 + 15ll) & -16ll;
		BYTESTOALLOC$1 = BYTESTOALLOC$1 - BYTESPUSHED$1;
	}
	label$319:;
	label$318:;
	fb$result$1 = BYTESTOALLOC$1;
	goto label$317;
	label$317:;
	return fb$result$1;
}

static void HSTOREREGISTERARGUMENT( struct $8FBSYMBOL* PARAM$1, FBSTRING* SRC$1 )
{
	FBSTRING TMP$254$1;
	FBSTRING TMP$255$1;
	FBSTRING TMP$256$1;
	FBSTRING TMP$257$1;
	label$326:;
	FBSTRING OPERAND$1;
	fb_StrInit( (void*)&OPERAND$1, -1ll, (void*)"", 1ll, 0 );
	int64 OFS$1;
	OFS$1 = *(int64*)((uint8*)*(struct $8FBSYMBOL**)((uint8*)PARAM$1 + 104ll) + 88ll);
	fb_StrAssign( (void*)&OPERAND$1, -1ll, (void*)"dword ptr [ebp", 15ll, 0 );
	if( OFS$1 <= 0ll) goto label$329;
	{
		fb_StrConcatAssign( (void*)&OPERAND$1, -1ll, (void*)"+", 2ll, 0 );
	}
	label$329:;
	label$328:;
	if( OFS$1 == 0ll) goto label$331;
	{
		FBSTRING* vr$5 = fb_LongintToStr( OFS$1 );
		fb_StrConcatAssign( (void*)&OPERAND$1, -1ll, (void*)vr$5, -1ll, 0 );
	}
	label$331:;
	label$330:;
	fb_StrConcatAssign( (void*)&OPERAND$1, -1ll, (void*)"]", 2ll, 0 );
	__builtin_memset( &TMP$257$1, 0, 24ll );
	__builtin_memset( &TMP$254$1, 0, 24ll );
	FBSTRING* vr$12 = fb_StrConcat( &TMP$254$1, (void*)"mov ", 5ll, (void*)&OPERAND$1, -1ll );
	__builtin_memset( &TMP$255$1, 0, 24ll );
	FBSTRING* vr$15 = fb_StrConcat( &TMP$255$1, (void*)vr$12, -1ll, (void*)", ", 3ll );
	__builtin_memset( &TMP$256$1, 0, 24ll );
	FBSTRING* vr$18 = fb_StrConcat( &TMP$256$1, (void*)vr$15, -1ll, (void*)SRC$1, -1ll );
	fb_StrAssign( (void*)&TMP$257$1, -1ll, (void*)vr$18, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$257$1 );
	fb_StrDelete( (FBSTRING*)&TMP$257$1 );
	fb_StrDelete( (FBSTRING*)&OPERAND$1 );
	label$327:;
}

static void HSTOREREGISTERARGUMENTS( struct $8FBSYMBOL* PROC$1 )
{
	label$332:;
	struct $8FBSYMBOL* PARAM$1;
	PARAM$1 = *(struct $8FBSYMBOL**)((uint8*)PROC$1 + 136ll);
	label$334:;
	if( PARAM$1 == (struct $8FBSYMBOL*)0ull) goto label$335;
	{
		{
			int64 TMP$258$3;
			TMP$258$3 = *(int64*)((uint8*)PARAM$1 + 136ll);
			if( TMP$258$3 != 1ll) goto label$337;
			label$338:;
			{
				FBSTRING TMP$259$4;
				__builtin_memset( &TMP$259$4, 0, 24ll );
				fb_StrAssign( (void*)&TMP$259$4, -1ll, (void*)"ecx", 4ll, 0 );
				HSTOREREGISTERARGUMENT( PARAM$1, &TMP$259$4 );
				fb_StrDelete( (FBSTRING*)&TMP$259$4 );
			}
			goto label$336;
			label$337:;
			if( TMP$258$3 != 2ll) goto label$339;
			label$340:;
			{
				FBSTRING TMP$260$4;
				__builtin_memset( &TMP$260$4, 0, 24ll );
				fb_StrAssign( (void*)&TMP$260$4, -1ll, (void*)"edx", 4ll, 0 );
				HSTOREREGISTERARGUMENT( PARAM$1, &TMP$260$4 );
				fb_StrDelete( (FBSTRING*)&TMP$260$4 );
			}
			label$339:;
			label$336:;
		}
		PARAM$1 = *(struct $8FBSYMBOL**)((uint8*)PARAM$1 + 312ll);
	}
	goto label$334;
	label$335:;
	label$333:;
}

static void HCREATEFRAME( struct $8FBSYMBOL* PROC$1 )
{
	int64 TMP$261$1;
	label$341:;
	static int64 BYTESTOALLOC$1;
	static int64 BYTESTOCLEAR$1;
	static int64 BYTESPUSHED$1;
	static char* LPROF$1;
	static int64 HASUNWIND$1;
	if( *(int64*)((uint8*)&ENV$ + 216ll) != 2ll) goto label$343;
	int64 vr$0 = FBGETOPTION( 21ll );
	TMP$261$1 = (int64)-(vr$0 != 0ll);
	goto label$1875;
	label$343:;
	TMP$261$1 = 0ll;
	label$1875:;
	HASUNWIND$1 = TMP$261$1;
	if( (*(int64*)((uint8*)PROC$1 + 16ll) & 128ll) != 0ll) goto label$345;
	{
		int64 vr$4 = HFRAMEBYTESTOALLOC( PROC$1 );
		BYTESTOALLOC$1 = vr$4;
		int64 vr$15 = FBGETOPTION( 24ll );
		if( (((((int64)-(BYTESTOALLOC$1 != 0ll) | (int64)-(*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 8ll) != 8ll)) | (int64)-((*(int64*)((uint8*)PROC$1 + 24ll) & 4096ll) != 0ll)) | *(int64*)((uint8*)&ENV$ + 296ll)) | (int64)-(vr$15 != 0ll)) == 0ll) goto label$347;
		{
			HPUSH( (char*)"ebp" );
			if( HASUNWIND$1 == 0ll) goto label$349;
			{
				OUTP( (char*)".cfi_def_cfa_offset 8" );
				OUTP( (char*)".cfi_offset 5, -8" );
			}
			label$349:;
			label$348:;
			OUTP( (char*)"mov ebp, esp" );
			if( HASUNWIND$1 == 0ll) goto label$351;
			{
				OUTP( (char*)".cfi_def_cfa_register 5" );
				label$351:;
			}
			if( (*(int64*)((uint8*)PROC$1 + 24ll) & 4096ll) == 0ll) goto label$353;
			{
				OUTP( (char*)"and esp, 0xFFFFFFF0" );
				BYTESTOALLOC$1 = BYTESTOALLOC$1 + 8ll;
			}
			label$353:;
			label$352:;
			if( BYTESTOALLOC$1 <= 0ll) goto label$355;
			{
				FBSTRING TMP$268$4;
				FBSTRING TMP$269$4;
				__builtin_memset( &TMP$269$4, 0, 24ll );
				FBSTRING* vr$22 = fb_LongintToStr( BYTESTOALLOC$1 );
				__builtin_memset( &TMP$268$4, 0, 24ll );
				FBSTRING* vr$25 = fb_StrConcat( &TMP$268$4, (void*)"sub esp, ", 10ll, (void*)vr$22, -1ll );
				fb_StrAssign( (void*)&TMP$269$4, -1ll, (void*)vr$25, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$269$4 );
				fb_StrDelete( (FBSTRING*)&TMP$269$4 );
			}
			label$355:;
			label$354:;
		}
		label$347:;
		label$346:;
		if( *(int64*)((uint8*)&ENV$ + 216ll) != 5ll) goto label$357;
		{
			int64 vr$28 = FBGETOPTION( 24ll );
			if( vr$28 != 1ll) goto label$359;
			{
				FBSTRING TMP$274$4;
				FBSTRING TMP$275$4;
				FBSTRING TMP$276$4;
				FBSTRING TMP$277$4;
				FBSTRING TMP$282$4;
				FBSTRING TMP$283$4;
				char* vr$29 = SYMBMAKEPROFILELABELNAME(  );
				LPROF$1 = vr$29;
				OUTEX( (char*)".section .data\x0A", 0ll );
				OUTEX( (char*)".balign 4\x0A", 0ll );
				__builtin_memset( &TMP$277$4, 0, 24ll );
				__builtin_memset( &TMP$274$4, 0, 24ll );
				FBSTRING* vr$33 = fb_StrConcat( &TMP$274$4, (void*)".", 2ll, (void*)LPROF$1, 0ll );
				__builtin_memset( &TMP$275$4, 0, 24ll );
				FBSTRING* vr$36 = fb_StrConcat( &TMP$275$4, (void*)vr$33, -1ll, (void*)":", 2ll );
				__builtin_memset( &TMP$276$4, 0, 24ll );
				FBSTRING* vr$39 = fb_StrConcat( &TMP$276$4, (void*)vr$36, -1ll, (void*)"\x0A", 2ll );
				fb_StrAssign( (void*)&TMP$277$4, -1ll, (void*)vr$39, -1ll, 0 );
				OUTEX( (char*)*(char**)&TMP$277$4, 0ll );
				fb_StrDelete( (FBSTRING*)&TMP$277$4 );
				OUTP( (char*)".long 0" );
				OUTEX( (char*)".section .text\x0A", 0ll );
				__builtin_memset( &TMP$283$4, 0, 24ll );
				__builtin_memset( &TMP$282$4, 0, 24ll );
				FBSTRING* vr$45 = fb_StrConcat( &TMP$282$4, (void*)"mov edx, offset .", 18ll, (void*)LPROF$1, 0ll );
				fb_StrAssign( (void*)&TMP$283$4, -1ll, (void*)vr$45, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$283$4 );
				fb_StrDelete( (FBSTRING*)&TMP$283$4 );
				OUTP( (char*)"call _mcount" );
			}
			label$359:;
			label$358:;
		}
		label$357:;
		label$356:;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 16ll) == 0ll) goto label$361;
		{
			HPUSH( (char*)"ebx" );
		}
		label$361:;
		label$360:;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 4ll) == 0ll) goto label$363;
		{
			HPUSH( (char*)"esi" );
		}
		label$363:;
		label$362:;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 2ll) == 0ll) goto label$365;
		{
			HPUSH( (char*)"edi" );
		}
		label$365:;
		label$364:;
		HSTOREREGISTERARGUMENTS( PROC$1 );
	}
	label$345:;
	label$344:;
	label$342:;
}

static void HDESTROYFRAME( struct $8FBSYMBOL* PROC$1, int64 BYTESTOPOP$1 )
{
	label$366:;
	static int64 ISLINUX$1;
	ISLINUX$1 = (int64)-(*(int64*)((uint8*)&ENV$ + 216ll) == 2ll);
	if( (*(int64*)((uint8*)PROC$1 + 16ll) & 128ll) != 0ll) goto label$369;
	{
		int64 TMP$285$2;
		static int64 BYTESTOALLOC$2;
		static int64 HASUNWIND$2;
		if( ISLINUX$1 == 0ll) goto label$370;
		int64 vr$3 = FBGETOPTION( 21ll );
		TMP$285$2 = (int64)-(vr$3 != 0ll);
		goto label$1876;
		label$370:;
		TMP$285$2 = 0ll;
		label$1876:;
		HASUNWIND$2 = TMP$285$2;
		int64 vr$5 = HFRAMEBYTESTOALLOC( PROC$1 );
		BYTESTOALLOC$2 = vr$5;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 2ll) == 0ll) goto label$372;
		{
			HPOP( (char*)"edi" );
		}
		label$372:;
		label$371:;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 4ll) == 0ll) goto label$374;
		{
			HPOP( (char*)"esi" );
		}
		label$374:;
		label$373:;
		if( (*(int64*)((uint8*)&EMIT$ + 264ll) & 16ll) == 0ll) goto label$376;
		{
			HPOP( (char*)"ebx" );
		}
		label$376:;
		label$375:;
		int64 vr$19 = FBGETOPTION( 24ll );
		if( (((((int64)-(BYTESTOALLOC$2 != 0ll) | (int64)-(*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 8ll) != 8ll)) | (int64)-((*(int64*)((uint8*)PROC$1 + 24ll) & 4096ll) != 0ll)) | *(int64*)((uint8*)&ENV$ + 296ll)) | (int64)-(vr$19 != 0ll)) == 0ll) goto label$378;
		{
			OUTP( (char*)"mov esp, ebp" );
			HPOP( (char*)"ebp" );
			if( HASUNWIND$2 == 0ll) goto label$380;
			{
				OUTP( (char*)".cfi_restore 5" );
				OUTP( (char*)".cfi_def_cfa 4, 4" );
			}
			label$380:;
			label$379:;
		}
		label$378:;
		label$377:;
		if( BYTESTOPOP$1 <= 0ll) goto label$382;
		{
			FBSTRING TMP$290$3;
			FBSTRING TMP$291$3;
			__builtin_memset( &TMP$291$3, 0, 24ll );
			FBSTRING* vr$23 = fb_LongintToStr( BYTESTOPOP$1 );
			__builtin_memset( &TMP$290$3, 0, 24ll );
			FBSTRING* vr$26 = fb_StrConcat( &TMP$290$3, (void*)"ret ", 5ll, (void*)vr$23, -1ll );
			fb_StrAssign( (void*)&TMP$291$3, -1ll, (void*)vr$26, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$291$3 );
			fb_StrDelete( (FBSTRING*)&TMP$291$3 );
		}
		goto label$381;
		label$382:;
		{
			OUTP( (char*)"ret" );
		}
		label$381:;
	}
	label$369:;
	label$368:;
	if( ISLINUX$1 == 0ll) goto label$384;
	{
		FBSTRING TMP$295$2;
		FBSTRING TMP$296$2;
		FBSTRING TMP$297$2;
		FBSTRING TMP$298$2;
		FBSTRING TMP$299$2;
		__builtin_memset( &TMP$299$2, 0, 24ll );
		char* vr$30 = SYMBGETMANGLEDNAME( PROC$1 );
		char* vr$31 = SYMBGETMANGLEDNAME( PROC$1 );
		__builtin_memset( &TMP$295$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$295$2, (void*)".size ", 7ll, (void*)vr$31, 0ll );
		__builtin_memset( &TMP$296$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$296$2, (void*)vr$34, -1ll, (void*)", .-", 5ll );
		__builtin_memset( &TMP$297$2, 0, 24ll );
		FBSTRING* vr$40 = fb_StrConcat( &TMP$297$2, (void*)vr$37, -1ll, (void*)vr$30, 0ll );
		__builtin_memset( &TMP$298$2, 0, 24ll );
		FBSTRING* vr$43 = fb_StrConcat( &TMP$298$2, (void*)vr$40, -1ll, (void*)"\x0A", 2ll );
		fb_StrAssign( (void*)&TMP$299$2, -1ll, (void*)vr$43, -1ll, 0 );
		OUTEX( (char*)*(char**)&TMP$299$2, 0ll );
		fb_StrDelete( (FBSTRING*)&TMP$299$2 );
	}
	label$384:;
	label$383:;
	label$367:;
}

static void _EMITLIT( char* S$1 )
{
	FBSTRING TMP$301$1;
	label$385:;
	FBSTRING OSTR$1;
	__builtin_memset( &OSTR$1, 0, 24ll );
	__builtin_memset( &TMP$301$1, 0, 24ll );
	FBSTRING* vr$3 = fb_StrConcat( &TMP$301$1, (void*)S$1, 0ll, (void*)"\x0A", 2ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$3, -1ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	fb_StrDelete( (FBSTRING*)&OSTR$1 );
	label$386:;
}

static void _EMITJMPTB( struct $8FBSYMBOL* TBSYM$1, uint64* VALUES1$1, struct $8FBSYMBOL** LABELS1$1, int64 LABELCOUNT$1, struct $8FBSYMBOL* DEFLABEL$1, uint64 BIAS$1, uint64 SPAN$1 )
{
	FBSTRING TMP$303$1;
	FBSTRING TMP$304$1;
	FBSTRING TMP$305$1;
	label$387:;
	FBSTRING DEFLABELNAME$1;
	__builtin_memset( &DEFLABELNAME$1, 0, 24ll );
	FBSTRING TB$1;
	__builtin_memset( &TB$1, 0, 24ll );
	char* vr$2 = SYMBGETMANGLEDNAME( DEFLABEL$1 );
	fb_StrAssign( (void*)&DEFLABELNAME$1, -1ll, (void*)vr$2, 0ll, 0 );
	char* vr$4 = SYMBGETMANGLEDNAME( TBSYM$1 );
	fb_StrAssign( (void*)&TB$1, -1ll, (void*)vr$4, 0ll, 0 );
	__builtin_memset( &TMP$305$1, 0, 24ll );
	__builtin_memset( &TMP$303$1, 0, 24ll );
	FBSTRING* vr$10 = fb_StrConcat( &TMP$303$1, (void*)&TB$1, -1ll, (void*)":", 2ll );
	__builtin_memset( &TMP$304$1, 0, 24ll );
	FBSTRING* vr$13 = fb_StrConcat( &TMP$304$1, (void*)vr$10, -1ll, (void*)"\x0A", 2ll );
	fb_StrAssign( (void*)&TMP$305$1, -1ll, (void*)vr$13, -1ll, 0 );
	OUTEX( (char*)*(char**)&TMP$305$1, 0ll );
	fb_StrDelete( (FBSTRING*)&TMP$305$1 );
	int64 I$1;
	I$1 = 0ll;
	int64 VALUE$1;
	VALUE$1 = 0ll;
	label$389:;
	{
		FBSTRING TMP$306$2;
		FBSTRING TMP$307$2;
		FBSTRING TMP$308$2;
		struct $8FBSYMBOL* LABEL$2;
		__builtin_memset( &LABEL$2, 0, 8ll );
		if( (uint64)VALUE$1 != *(uint64*)((uint8*)VALUES1$1 + (I$1 << (3ll & 63ll)))) goto label$393;
		{
			LABEL$2 = *(struct $8FBSYMBOL**)((uint8*)LABELS1$1 + (I$1 << (3ll & 63ll)));
			I$1 = I$1 + 1ll;
		}
		goto label$392;
		label$393:;
		{
			LABEL$2 = DEFLABEL$1;
		}
		label$392:;
		__builtin_memset( &TMP$308$2, 0, 24ll );
		char* vr$23 = SYMBGETMANGLEDNAME( LABEL$2 );
		char* vr$24 = _GETTYPESTRING( 9ll );
		__builtin_memset( &TMP$306$2, 0, 24ll );
		FBSTRING* vr$27 = fb_StrConcat( &TMP$306$2, (void*)vr$24, 0ll, (void*)" ", 2ll );
		__builtin_memset( &TMP$307$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$307$2, (void*)vr$27, -1ll, (void*)vr$23, 0ll );
		fb_StrAssign( (void*)&TMP$308$2, -1ll, (void*)vr$30, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$308$2 );
		fb_StrDelete( (FBSTRING*)&TMP$308$2 );
		if( (uint64)VALUE$1 != SPAN$1) goto label$395;
		{
			goto label$390;
		}
		label$395:;
		label$394:;
		VALUE$1 = VALUE$1 + 1ll;
	}
	label$391:;
	goto label$389;
	label$390:;
	fb_StrDelete( (FBSTRING*)&TB$1 );
	fb_StrDelete( (FBSTRING*)&DEFLABELNAME$1 );
	label$388:;
}

static void _EMITCALL( struct $6IRVREG* UNUSED$1, struct $8FBSYMBOL* LABEL$1, int64 BYTESTOPOP$1 )
{
	FBSTRING TMP$311$1;
	label$396:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"call ", 6ll, 0 );
	char* vr$0 = SYMBGETMANGLEDNAME( LABEL$1 );
	__builtin_memset( &TMP$311$1, 0, 24ll );
	FBSTRING* vr$3 = fb_StrConcat( &TMP$311$1, (void*)&OSTR$1, -1ll, (void*)vr$0, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$3, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	if( BYTESTOPOP$1 == 0ll) goto label$399;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"add esp, ", 10ll, 0 );
		FBSTRING* vr$4 = fb_LongintToStr( BYTESTOPOP$1 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$4, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$399:;
	label$398:;
	label$397:;
}

static void _EMITCALLPTR( struct $6IRVREG* SVREG$1, struct $8FBSYMBOL* UNUSED$1, int64 BYTESTOPOP$1 )
{
	label$400:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"call ", 6ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	if( BYTESTOPOP$1 == 0ll) goto label$403;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"add esp, ", 10ll, 0 );
		FBSTRING* vr$0 = fb_LongintToStr( BYTESTOPOP$1 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$0, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$403:;
	label$402:;
	label$401:;
}

static void _EMITBRANCH( struct $6IRVREG* UNUSED$1, struct $8FBSYMBOL* LABEL$1, int64 OP$1 )
{
	FBSTRING TMP$320$1;
	label$404:;
	static FBSTRING OSTR$1;
	{
		uint64 TMP$313$2;
		TMP$313$2 = (uint64)OP$1;
		goto label$407;
		label$408:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"jle ", 5ll, 0 );
		}
		goto label$406;
		label$409:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"jge ", 5ll, 0 );
		}
		goto label$406;
		label$410:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"jl ", 4ll, 0 );
		}
		goto label$406;
		label$411:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"jg ", 4ll, 0 );
		}
		goto label$406;
		label$412:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"je ", 4ll, 0 );
		}
		goto label$406;
		label$413:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"jne ", 5ll, 0 );
		}
		goto label$406;
		label$407:;
		static const void* tmp$1058[6ll] = {
			&&label$412,
			&&label$411,
			&&label$410,
			&&label$413,
			&&label$409,
			&&label$408,
		};
		if( (TMP$313$2 - 93ull) > 5ull ) goto label$406;
		goto *tmp$1058[TMP$313$2 - 93ull];
		label$406:;
	}
	char* vr$0 = SYMBGETMANGLEDNAME( LABEL$1 );
	__builtin_memset( &TMP$320$1, 0, 24ll );
	FBSTRING* vr$3 = fb_StrConcat( &TMP$320$1, (void*)&OSTR$1, -1ll, (void*)vr$0, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$3, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$405:;
}

static void _EMITJUMP( struct $6IRVREG* UNUSED1$1, struct $8FBSYMBOL* LABEL$1, int64 UNUSED2$1 )
{
	FBSTRING TMP$322$1;
	label$414:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"jmp ", 5ll, 0 );
	char* vr$0 = SYMBGETMANGLEDNAME( LABEL$1 );
	__builtin_memset( &TMP$322$1, 0, 24ll );
	FBSTRING* vr$3 = fb_StrConcat( &TMP$322$1, (void*)&OSTR$1, -1ll, (void*)vr$0, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$3, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$415:;
}

static void _EMITJUMPPTR( struct $6IRVREG* SVREG$1, struct $8FBSYMBOL* UNUSED1$1, int64 UNUSED2$1 )
{
	label$416:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"jmp ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$417:;
}

static void _EMITRET( struct $6IRVREG* VREG$1 )
{
	FBSTRING TMP$324$1;
	label$418:;
	static FBSTRING OSTR$1;
	FBSTRING* vr$1 = fb_LongintToStr( *(int64*)((uint8*)VREG$1 + 48ll) );
	__builtin_memset( &TMP$324$1, 0, 24ll );
	FBSTRING* vr$4 = fb_StrConcat( &TMP$324$1, (void*)"ret ", 5ll, (void*)vr$1, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$4, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$419:;
}

static void _EMITPUBLIC( struct $8FBSYMBOL* LABEL$1 )
{
	FBSTRING TMP$326$1;
	label$420:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A.globl ", 9ll, 0 );
	char* vr$0 = SYMBGETMANGLEDNAME( LABEL$1 );
	__builtin_memset( &TMP$326$1, 0, 24ll );
	FBSTRING* vr$3 = fb_StrConcat( &TMP$326$1, (void*)&OSTR$1, -1ll, (void*)vr$0, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$3, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A", 2ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$421:;
}

static void _EMITLABEL( struct $8FBSYMBOL* LABEL$1 )
{
	label$422:;
	static FBSTRING OSTR$1;
	char* vr$0 = SYMBGETMANGLEDNAME( LABEL$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$0, 0ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)":\x0A", 3ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$423:;
}

static void _EMITNOP( void )
{
	label$424:;
	label$425:;
}

static void HULONG2DBL( struct $6IRVREG* SVREG$1 )
{
	label$426:;
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
	label$427:;
}

static void _EMITSTORL2L( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$428:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$429:;
}

static void _EMITSTORI2L( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$337$1;
	int64 TMP$343$1;
	label$430:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING EXT$1;
	static FBSTRING OSTR$1;
	static int64 SDSIZE$1;
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$432;
	TMP$337$1 = 24ll;
	goto label$1877;
	label$432:;
	TMP$337$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1877:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$337$1 * 56ll)) + 8ll);
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND( SVREG$1, &SRC1$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 != 0ll) goto label$434;
	{
		int64 TMP$338$2;
		HMOV( (char*)*(char**)&DST1$1, (char*)*(char**)&SRC1$1 );
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$435;
		TMP$338$2 = 24ll;
		goto label$1878;
		label$435:;
		TMP$338$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1878:;
		if( (*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$338$2 * 56ll)) + 16ll) & (int64)-(*(int64*)((uint8*)SVREG$1 + 48ll) < 0ll)) == 0ll) goto label$437;
		{
			HMOV( (char*)*(char**)&DST2$1, (char*)"-1" );
		}
		goto label$436;
		label$437:;
		{
			HMOV( (char*)*(char**)&DST2$1, (char*)"0" );
		}
		label$436:;
		goto label$431;
	}
	label$434:;
	label$433:;
	if( SDSIZE$1 >= 4ll) goto label$439;
	{
		int64 TMP$340$2;
		char* vr$15 = HGETREGNAME( 8ll, *(int64*)((uint8*)SVREG$1 + 24ll) );
		fb_StrAssign( (void*)&EXT$1, -1ll, (void*)vr$15, 0ll, 0 );
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$440;
		TMP$340$2 = 24ll;
		goto label$1879;
		label$440:;
		TMP$340$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1879:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$340$2 * 56ll)) + 16ll) == 0ll) goto label$442;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movsx ", 7ll, 0 );
		}
		goto label$441;
		label$442:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movzx ", 7ll, 0 );
		}
		label$441:;
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&EXT$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$438;
	label$439:;
	{
		fb_StrAssign( (void*)&EXT$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	}
	label$438:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&EXT$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$443;
	TMP$343$1 = 24ll;
	goto label$1880;
	label$443:;
	TMP$343$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1880:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$343$1 * 56ll)) + 16ll) == 0ll) goto label$445;
	{
		HPUSH( (char*)*(char**)&EXT$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sar ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&EXT$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 31", 5ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&EXT$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		HPOP( (char*)*(char**)&EXT$1 );
	}
	goto label$444;
	label$445:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 0", 4ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$444:;
	label$431:;
}

static void _EMITSTORF2L( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$346$1;
	label$446:;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$448;
	TMP$346$1 = 24ll;
	goto label$1881;
	label$448:;
	TMP$346$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1881:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$346$1 * 56ll)) + 16ll) == 0ll) goto label$450;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fistp ", 7ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$450:;
	label$449:;
	label$447:;
}

static void _EMITSTORI2I( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$348$1;
	int64 TMP$349$1;
	label$451:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	static int64 SDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$453;
	TMP$348$1 = 24ll;
	goto label$1882;
	label$453:;
	TMP$348$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1882:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$348$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$454;
	TMP$349$1 = 24ll;
	goto label$1883;
	label$454:;
	TMP$349$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1883:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$349$1 * 56ll)) + 8ll);
	if( DDSIZE$1 != 1ll) goto label$456;
	{
		if( *(int64*)SVREG$1 != 0ll) goto label$458;
		{
			DDSIZE$1 = 4ll;
		}
		label$458:;
		label$457:;
	}
	label$456:;
	label$455:;
	if( ((int64)-(*(int64*)SVREG$1 == 0ll) | (int64)-(DDSIZE$1 == SDSIZE$1)) == 0ll) goto label$460;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$459;
	label$460:;
	{
		static FBSTRING AUX$2;
		char* vr$17 = HGETREGNAME( *(int64*)((uint8*)DVREG$1 + 8ll), *(int64*)((uint8*)SVREG$1 + 24ll) );
		fb_StrAssign( (void*)&AUX$2, -1ll, (void*)vr$17, 0ll, 0 );
		if( DDSIZE$1 <= SDSIZE$1) goto label$462;
		{
			int64 TMP$350$3;
			if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$463;
			TMP$350$3 = 24ll;
			goto label$1884;
			label$463:;
			TMP$350$3 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
			label$1884:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$350$3 * 56ll)) + 16ll) == 0ll) goto label$465;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movsx ", 7ll, 0 );
			}
			goto label$464;
			label$465:;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movzx ", 7ll, 0 );
			}
			label$464:;
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$2, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$2, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$461;
		label$462:;
		{
			static int64 IS_DISI$3;
			IS_DISI$3 = 0ll;
			if( DDSIZE$1 != 1ll) goto label$467;
			{
				if( *(int64*)SVREG$1 != 4ll) goto label$469;
				{
					IS_DISI$3 = (int64)-(*(int64*)((uint8*)SVREG$1 + 24ll) == 2ll) | (int64)-(*(int64*)((uint8*)SVREG$1 + 24ll) == 1ll);
				}
				label$469:;
				label$468:;
			}
			label$467:;
			label$466:;
			if( IS_DISI$3 == 0ll) goto label$471;
			{
				static FBSTRING AUX8$4;
				static int64 REG$4;
				static int64 ISFREE$4;
				int64 vr$29 = HFINDREGNOTINVREG( DVREG$1, -1ll );
				REG$4 = vr$29;
				char* vr$30 = HGETREGNAME( 2ll, REG$4 );
				fb_StrAssign( (void*)&AUX8$4, -1ll, (void*)vr$30, 0ll, 0 );
				char* vr$32 = HGETREGNAME( *(int64*)((uint8*)SVREG$1 + 8ll), REG$4 );
				fb_StrAssign( (void*)&AUX$2, -1ll, (void*)vr$32, 0ll, 0 );
				int64 vr$33 = HISREGFREE( 0ll, REG$4 );
				ISFREE$4 = vr$33;
				if( ISFREE$4 != 0ll) goto label$473;
				{
					HPUSH( (char*)*(char**)&AUX$2 );
				}
				label$473:;
				label$472:;
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$2, -1ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX8$4, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
				if( ISFREE$4 != 0ll) goto label$475;
				{
					HPOP( (char*)*(char**)&AUX$2 );
				}
				label$475:;
				label$474:;
			}
			goto label$470;
			label$471:;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$2, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
			}
			label$470:;
		}
		label$461:;
	}
	label$459:;
	label$452:;
}

static void _EMITSTORL2I( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$476:;
	_EMITSTORI2I( DVREG$1, SVREG$1 );
	label$477:;
}

static void _EMITSTORF2I( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$351$1;
	label$478:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$480;
	TMP$351$1 = 24ll;
	goto label$1885;
	label$480:;
	TMP$351$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1885:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$351$1 * 56ll)) + 8ll);
	if( DDSIZE$1 != 1ll) goto label$482;
	{
		OUTP( (char*)"sub esp, 4" );
		OUTP( (char*)"fistp dword ptr [esp]" );
		if( *(int64*)DVREG$1 != 4ll) goto label$484;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)"byte ptr [esp]" );
			OUTP( (char*)"add esp, 4" );
		}
		goto label$483;
		label$484:;
		{
			static int64 REG$3;
			static int64 ISFREE$3;
			static FBSTRING AUX$3;
			static FBSTRING AUX8$3;
			int64 vr$6 = HFINDREGNOTINVREG( DVREG$1, -1ll );
			REG$3 = vr$6;
			char* vr$7 = HGETREGNAME( 2ll, REG$3 );
			fb_StrAssign( (void*)&AUX8$3, -1ll, (void*)vr$7, 0ll, 0 );
			char* vr$8 = HGETREGNAME( 8ll, REG$3 );
			fb_StrAssign( (void*)&AUX$3, -1ll, (void*)vr$8, 0ll, 0 );
			int64 vr$9 = HISREGFREE( 0ll, REG$3 );
			ISFREE$3 = vr$9;
			if( ISFREE$3 != 0ll) goto label$486;
			{
				HXCHG( (char*)*(char**)&AUX$3, (char*)"dword ptr [esp]" );
			}
			goto label$485;
			label$486:;
			{
				HMOV( (char*)*(char**)&AUX8$3, (char*)"byte ptr [esp]" );
			}
			label$485:;
			HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX8$3 );
			if( ISFREE$3 != 0ll) goto label$488;
			{
				HPOP( (char*)*(char**)&AUX$3 );
			}
			goto label$487;
			label$488:;
			{
				OUTP( (char*)"add esp, 4" );
			}
			label$487:;
		}
		label$483:;
	}
	goto label$481;
	label$482:;
	{
		int64 TMP$357$2;
		if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$489;
		TMP$357$2 = 24ll;
		goto label$1886;
		label$489:;
		TMP$357$2 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
		label$1886:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$357$2 * 56ll)) + 16ll) == 0ll) goto label$491;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fistp ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$490;
		label$491:;
		{
			if( DDSIZE$1 != 4ll) goto label$493;
			{
				OUTP( (char*)"sub esp, 8" );
				OUTP( (char*)"fistp qword ptr [esp]" );
				HPOP( (char*)*(char**)&DST$1 );
				OUTP( (char*)"add esp, 4" );
			}
			goto label$492;
			label$493:;
			{
				OUTP( (char*)"sub esp, 4" );
				OUTP( (char*)"fistp dword ptr [esp]" );
				HPOP( (char*)*(char**)&DST$1 );
				OUTP( (char*)"add esp, 2" );
			}
			label$492:;
		}
		label$490:;
	}
	label$481:;
	label$479:;
}

static void _EMITSTORL2F( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$494:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING AUX$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( ((int64)-(*(int64*)SVREG$1 == 4ll) | (int64)-(*(int64*)SVREG$1 == 0ll)) == 0ll) goto label$497;
	{
		int64 TMP$361$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$498;
		TMP$361$2 = 24ll;
		goto label$1887;
		label$498:;
		TMP$361$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1887:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$361$2 * 56ll)) + 16ll) == 0ll) goto label$500;
		{
			FBSTRING TMP$364$3;
			FBSTRING TMP$365$3;
			HPREPOPERAND64( SVREG$1, &SRC$1, &AUX$1 );
			HPUSH( (char*)*(char**)&AUX$1 );
			HPUSH( (char*)*(char**)&SRC$1 );
			__builtin_memset( &TMP$364$3, 0, 24ll );
			FBSTRING* vr$15 = fb_StrConcat( &TMP$364$3, (void*)"fild ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)SVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
			__builtin_memset( &TMP$365$3, 0, 24ll );
			FBSTRING* vr$18 = fb_StrConcat( &TMP$365$3, (void*)vr$15, -1ll, (void*)" [esp]", 7ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$18, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			OUTP( (char*)"add esp, 8" );
		}
		goto label$499;
		label$500:;
		{
			HPREPOPERAND64( SVREG$1, &SRC$1, &AUX$1 );
			HPUSH( (char*)*(char**)&AUX$1 );
			HPUSH( (char*)*(char**)&SRC$1 );
			OUTP( (char*)"fild qword ptr [esp]" );
			OUTP( (char*)"add esp, 8" );
			HULONG2DBL( SVREG$1 );
		}
		label$499:;
	}
	goto label$496;
	label$497:;
	{
		int64 TMP$368$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$501;
		TMP$368$2 = 24ll;
		goto label$1888;
		label$501:;
		TMP$368$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1888:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$368$2 * 56ll)) + 16ll) == 0ll) goto label$503;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$502;
		label$503:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			HULONG2DBL( SVREG$1 );
		}
		label$502:;
	}
	label$496:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fstp ", 6ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$495:;
}

static void _EMITSTORI2F( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$370$1;
	label$504:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 SDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$506;
	TMP$370$1 = 24ll;
	goto label$1889;
	label$506:;
	TMP$370$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1889:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$370$1 * 56ll)) + 8ll);
	if( SDSIZE$1 != 1ll) goto label$508;
	{
		int64 TMP$371$2;
		static FBSTRING AUX$2;
		static int64 REG$2;
		static int64 ISFREE$2;
		int64 vr$5 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$2 = vr$5;
		char* vr$6 = HGETREGNAME( 8ll, REG$2 );
		fb_StrAssign( (void*)&AUX$2, -1ll, (void*)vr$6, 0ll, 0 );
		int64 vr$7 = HISREGFREE( 0ll, REG$2 );
		ISFREE$2 = vr$7;
		if( ISFREE$2 != 0ll) goto label$510;
		{
			HPUSH( (char*)*(char**)&AUX$2 );
		}
		label$510:;
		label$509:;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$511;
		TMP$371$2 = 24ll;
		goto label$1890;
		label$511:;
		TMP$371$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1890:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$371$2 * 56ll)) + 16ll) == 0ll) goto label$513;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movsx ", 7ll, 0 );
		}
		goto label$512;
		label$513:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movzx ", 7ll, 0 );
		}
		label$512:;
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$2, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		HPUSH( (char*)*(char**)&AUX$2 );
		OUTP( (char*)"fild dword ptr [esp]" );
		OUTP( (char*)"add esp, 4" );
		if( ISFREE$2 != 0ll) goto label$515;
		{
			HPOP( (char*)*(char**)&AUX$2 );
		}
		label$515:;
		label$514:;
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fstp ", 6ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		goto label$505;
	}
	label$508:;
	label$507:;
	if( ((int64)-(*(int64*)SVREG$1 == 4ll) | (int64)-(*(int64*)SVREG$1 == 0ll)) == 0ll) goto label$517;
	{
		int64 TMP$373$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$518;
		TMP$373$2 = 24ll;
		goto label$1891;
		label$518:;
		TMP$373$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1891:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$373$2 * 56ll)) + 16ll) == 0ll) goto label$520;
		{
			FBSTRING TMP$374$3;
			FBSTRING TMP$375$3;
			if( ((int64)-(*(int64*)SVREG$1 == 4ll) & (int64)-(SDSIZE$1 < 4ll)) == 0ll) goto label$522;
			{
				char* vr$28 = HGETREGNAME( 8ll, *(int64*)((uint8*)SVREG$1 + 24ll) );
				fb_StrAssign( (void*)&SRC$1, -1ll, (void*)vr$28, 0ll, 0 );
			}
			label$522:;
			label$521:;
			HPUSH( (char*)*(char**)&SRC$1 );
			__builtin_memset( &TMP$374$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$374$3, (void*)"fild ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)SVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
			__builtin_memset( &TMP$375$3, 0, 24ll );
			FBSTRING* vr$37 = fb_StrConcat( &TMP$375$3, (void*)vr$34, -1ll, (void*)" [esp]", 7ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$37, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			OUTP( (char*)"add esp, 4" );
		}
		goto label$519;
		label$520:;
		{
			if( SDSIZE$1 != 4ll) goto label$524;
			{
				HPUSH( (char*)"0" );
				HPUSH( (char*)*(char**)&SRC$1 );
				OUTP( (char*)"fild qword ptr [esp]" );
				OUTP( (char*)"add esp, 8" );
			}
			goto label$523;
			label$524:;
			{
				if( *(int64*)SVREG$1 == 0ll) goto label$526;
				{
					HPUSH( (char*)"0" );
				}
				label$526:;
				label$525:;
				HPUSH( (char*)*(char**)&SRC$1 );
				OUTP( (char*)"fild dword ptr [esp]" );
				if( *(int64*)SVREG$1 == 0ll) goto label$528;
				{
					OUTP( (char*)"add esp, 6" );
				}
				goto label$527;
				label$528:;
				{
					OUTP( (char*)"add esp, 4" );
				}
				label$527:;
			}
			label$523:;
		}
		label$519:;
	}
	goto label$516;
	label$517:;
	{
		int64 TMP$377$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$529;
		TMP$377$2 = 24ll;
		goto label$1892;
		label$529:;
		TMP$377$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1892:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$377$2 * 56ll)) + 16ll) == 0ll) goto label$531;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$530;
		label$531:;
		{
			if( SDSIZE$1 != 4ll) goto label$533;
			{
				HPUSH( (char*)"0" );
				HPUSH( (char*)*(char**)&SRC$1 );
				OUTP( (char*)"fild qword ptr [esp]" );
				OUTP( (char*)"add esp, 8" );
			}
			goto label$532;
			label$533:;
			{
				HPUSH( (char*)"0" );
				HPUSH( (char*)*(char**)&SRC$1 );
				OUTP( (char*)"fild dword ptr [esp]" );
				OUTP( (char*)"add esp, 6" );
			}
			label$532:;
		}
		label$530:;
	}
	label$516:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fstp ", 6ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$505:;
}

static void _EMITSTORF2F( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$378$1;
	int64 TMP$379$1;
	label$534:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	static int64 SDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$536;
	TMP$378$1 = 24ll;
	goto label$1893;
	label$536:;
	TMP$378$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1893:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$378$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$537;
	TMP$379$1 = 24ll;
	goto label$1894;
	label$537:;
	TMP$379$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1894:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$379$1 * 56ll)) + 8ll);
	if( *(int64*)SVREG$1 != 4ll) goto label$539;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fstp ", 6ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$538;
	label$539:;
	{
		if( SDSIZE$1 != DDSIZE$1) goto label$541;
		{
			HPREPOPERAND( SVREG$1, &SRC$1, 8ll, 0ll, 0ll, -1ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			if( SDSIZE$1 <= 4ll) goto label$543;
			{
				HPREPOPERAND( SVREG$1, &SRC$1, 8ll, 4ll, 0ll, -1ll );
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
				HPREPOPERAND( DVREG$1, &DST$1, 8ll, 4ll, 0ll, -1ll );
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
			}
			label$543:;
			label$542:;
			HPREPOPERAND( DVREG$1, &DST$1, 8ll, 0ll, 0ll, -1ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$540;
		label$541:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fld ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fstp ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$540:;
	}
	label$538:;
	label$535:;
}

static void _EMITLOADL2L( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$544:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$545:;
}

static void _EMITLOADI2L( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$381$1;
	int64 TMP$383$1;
	label$546:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static int64 SDSIZE$1;
	static FBSTRING OSTR$1;
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$548;
	TMP$381$1 = 24ll;
	goto label$1895;
	label$548:;
	TMP$381$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1895:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$381$1 * 56ll)) + 8ll);
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND( SVREG$1, &SRC1$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 != 0ll) goto label$550;
	{
		int64 TMP$382$2;
		HMOV( (char*)*(char**)&DST1$1, (char*)*(char**)&SRC1$1 );
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$551;
		TMP$382$2 = 24ll;
		goto label$1896;
		label$551:;
		TMP$382$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1896:;
		if( (*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$382$2 * 56ll)) + 16ll) & (int64)-(*(int64*)((uint8*)SVREG$1 + 48ll) < 0ll)) == 0ll) goto label$553;
		{
			HMOV( (char*)*(char**)&DST2$1, (char*)"-1" );
		}
		goto label$552;
		label$553:;
		{
			HMOV( (char*)*(char**)&DST2$1, (char*)"0" );
		}
		label$552:;
		goto label$547;
	}
	label$550:;
	label$549:;
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$554;
	TMP$383$1 = 24ll;
	goto label$1897;
	label$554:;
	TMP$383$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1897:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$383$1 * 56ll)) + 16ll) == 0ll) goto label$556;
	{
		if( SDSIZE$1 >= 4ll) goto label$558;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movsx ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$557;
		label$558:;
		{
			HMOV( (char*)*(char**)&DST1$1, (char*)*(char**)&SRC1$1 );
		}
		label$557:;
		HMOV( (char*)*(char**)&DST2$1, (char*)*(char**)&DST1$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sar ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 31", 5ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$555;
	label$556:;
	{
		if( SDSIZE$1 >= 4ll) goto label$560;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movzx ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$559;
		label$560:;
		{
			HMOV( (char*)*(char**)&DST1$1, (char*)*(char**)&SRC1$1 );
		}
		label$559:;
		HMOV( (char*)*(char**)&DST2$1, (char*)"0" );
	}
	label$555:;
	label$547:;
}

static void _EMITLOADF2L( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$384$1;
	label$561:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING AUX$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 == 4ll) goto label$564;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fld ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$564:;
	label$563:;
	HPREPOPERAND64( DVREG$1, &DST$1, &AUX$1 );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$565;
	TMP$384$1 = 24ll;
	goto label$1898;
	label$565:;
	TMP$384$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1898:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$384$1 * 56ll)) + 16ll) == 0ll) goto label$567;
	{
		FBSTRING TMP$385$2;
		FBSTRING TMP$386$2;
		OUTP( (char*)"sub esp, 8" );
		__builtin_memset( &TMP$385$2, 0, 24ll );
		FBSTRING* vr$11 = fb_StrConcat( &TMP$385$2, (void*)"fistp ", 7ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
		__builtin_memset( &TMP$386$2, 0, 24ll );
		FBSTRING* vr$14 = fb_StrConcat( &TMP$386$2, (void*)vr$11, -1ll, (void*)" [esp]", 7ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$14, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		HPOP( (char*)*(char**)&DST$1 );
		HPOP( (char*)*(char**)&AUX$1 );
	}
	goto label$566;
	label$567:;
	{
		int64 TMP$387$2;
		FBSTRING TMP$400$2;
		FBSTRING TMP$401$2;
		FBSTRING TMP$402$2;
		static FBSTRING LABEL_GEQ$2;
		static FBSTRING LABEL_DONE$2;
		static int64 ISEAXFREE$2;
		char* vr$15 = SYMBUNIQUELABEL(  );
		fb_StrAssign( (void*)&LABEL_GEQ$2, -1ll, (void*)vr$15, 0ll, 0 );
		char* vr$16 = SYMBUNIQUELABEL(  );
		fb_StrAssign( (void*)&LABEL_DONE$2, -1ll, (void*)vr$16, 0ll, 0 );
		int64 vr$17 = HISREGFREE( 0ll, 5ll );
		ISEAXFREE$2 = vr$17;
		if( ISEAXFREE$2 != 0ll) goto label$568;
		int64 vr$18 = HISREGINVREG( DVREG$1, 5ll );
		TMP$387$2 = (int64)-(vr$18 != 0ll);
		goto label$1899;
		label$568:;
		TMP$387$2 = -1ll;
		label$1899:;
		ISEAXFREE$2 = TMP$387$2;
		OUTP( (char*)"sub esp, 8" );
		OUTP( (char*)"mov dword ptr [esp], 0x5F000000" );
		if( *(int64*)((uint8*)&ENV$ + 224ll) < 3ll) goto label$570;
		{
			OUTP( (char*)"fld dword ptr [esp]" );
			OUTP( (char*)"fcomip" );
			HBRANCH( (char*)"jbe", (char*)*(char**)&LABEL_GEQ$2 );
		}
		goto label$569;
		label$570:;
		{
			OUTP( (char*)"fcom dword ptr [esp]" );
			if( ISEAXFREE$2 == 0ll) goto label$572;
			{
				OUTP( (char*)"fnstsw ax" );
				OUTP( (char*)"test ah, 1" );
			}
			goto label$571;
			label$572:;
			{
				HPUSH( (char*)"eax" );
				OUTP( (char*)"fnstsw ax" );
				OUTP( (char*)"test ah, 1" );
				HPOP( (char*)"eax" );
			}
			label$571:;
			HBRANCH( (char*)"jz", (char*)*(char**)&LABEL_GEQ$2 );
		}
		label$569:;
		OUTP( (char*)"fistp qword ptr [esp]" );
		HPOP( (char*)*(char**)&DST$1 );
		HPOP( (char*)*(char**)&AUX$1 );
		HBRANCH( (char*)"jmp", (char*)*(char**)&LABEL_DONE$2 );
		HLABEL( (char*)*(char**)&LABEL_GEQ$2 );
		OUTP( (char*)"fsub dword ptr [esp]" );
		OUTP( (char*)"fistp qword ptr [esp]" );
		HPOP( (char*)*(char**)&DST$1 );
		HPOP( (char*)*(char**)&AUX$1 );
		__builtin_memset( &TMP$402$2, 0, 24ll );
		__builtin_memset( &TMP$400$2, 0, 24ll );
		FBSTRING* vr$23 = fb_StrConcat( &TMP$400$2, (void*)"xor ", 5ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$401$2, 0, 24ll );
		FBSTRING* vr$26 = fb_StrConcat( &TMP$401$2, (void*)vr$23, -1ll, (void*)", 0x80000000", 13ll );
		fb_StrAssign( (void*)&TMP$402$2, -1ll, (void*)vr$26, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$402$2 );
		fb_StrDelete( (FBSTRING*)&TMP$402$2 );
		HLABEL( (char*)*(char**)&LABEL_DONE$2 );
	}
	label$566:;
	label$562:;
}

static void _EMITLOADI2I( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$403$1;
	int64 TMP$404$1;
	label$573:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	static int64 SDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$575;
	TMP$403$1 = 24ll;
	goto label$1900;
	label$575:;
	TMP$403$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1900:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$403$1 * 56ll)) + 8ll);
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$576;
	TMP$404$1 = 24ll;
	goto label$1901;
	label$576:;
	TMP$404$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1901:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$404$1 * 56ll)) + 8ll);
	if( DDSIZE$1 != 1ll) goto label$578;
	{
		if( *(int64*)SVREG$1 != 0ll) goto label$580;
		{
			DDSIZE$1 = 4ll;
		}
		label$580:;
		label$579:;
	}
	label$578:;
	label$577:;
	if( ((int64)-(*(int64*)SVREG$1 == 0ll) | (int64)-(DDSIZE$1 == SDSIZE$1)) == 0ll) goto label$582;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$581;
	label$582:;
	{
		if( DDSIZE$1 <= SDSIZE$1) goto label$584;
		{
			int64 TMP$405$3;
			if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$585;
			TMP$405$3 = 24ll;
			goto label$1902;
			label$585:;
			TMP$405$3 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
			label$1902:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$405$3 * 56ll)) + 16ll) == 0ll) goto label$587;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movsx ", 7ll, 0 );
			}
			goto label$586;
			label$587:;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movzx ", 7ll, 0 );
			}
			label$586:;
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$583;
		label$584:;
		{
			if( *(int64*)SVREG$1 != 4ll) goto label$589;
			{
				if( *(int64*)((uint8*)SVREG$1 + 24ll) == *(int64*)((uint8*)DVREG$1 + 24ll)) goto label$591;
				{
					static FBSTRING AUX$5;
					static int64 DTYPE$5;
					DTYPE$5 = *(int64*)((uint8*)DVREG$1 + 8ll);
					if( DDSIZE$1 != 1ll) goto label$593;
					{
						if( ((int64)-(*(int64*)((uint8*)SVREG$1 + 24ll) == 2ll) | (int64)-(*(int64*)((uint8*)SVREG$1 + 24ll) == 1ll)) == 0ll) goto label$595;
						{
							DTYPE$5 = 8ll;
							char* vr$30 = HGETREGNAME( DTYPE$5, *(int64*)((uint8*)DVREG$1 + 24ll) );
							fb_StrAssign( (void*)&DST$1, -1ll, (void*)vr$30, 0ll, 0 );
						}
						label$595:;
						label$594:;
					}
					label$593:;
					label$592:;
					char* vr$32 = HGETREGNAME( DTYPE$5, *(int64*)((uint8*)SVREG$1 + 24ll) );
					fb_StrAssign( (void*)&AUX$5, -1ll, (void*)vr$32, 0ll, 0 );
					fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$5, -1ll, 0 );
					OUTP( (char*)*(char**)&OSTR$1 );
				}
				label$591:;
				label$590:;
			}
			goto label$588;
			label$589:;
			{
				HPREPOPERAND( SVREG$1, &SRC$1, *($11FB_DATATYPE*)((uint8*)DVREG$1 + 8ll), 0ll, 0ll, -1ll );
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
			}
			label$588:;
		}
		label$583:;
	}
	label$581:;
	label$574:;
}

static void _EMITLOADL2I( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$596:;
	_EMITLOADI2I( DVREG$1, SVREG$1 );
	label$597:;
}

static void _EMITLOADF2I( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$406$1;
	label$598:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$600;
	TMP$406$1 = 24ll;
	goto label$1903;
	label$600:;
	TMP$406$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1903:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$406$1 * 56ll)) + 8ll);
	if( *(int64*)SVREG$1 == 4ll) goto label$602;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fld ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$602:;
	label$601:;
	if( DDSIZE$1 != 1ll) goto label$604;
	{
		OUTP( (char*)"sub esp, 4" );
		OUTP( (char*)"fistp dword ptr [esp]" );
		if( *(int64*)DVREG$1 != 4ll) goto label$606;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)"byte ptr [esp]" );
			OUTP( (char*)"add esp, 4" );
		}
		goto label$605;
		label$606:;
		{
			static FBSTRING AUX$3;
			static FBSTRING AUX8$3;
			static int64 REG$3;
			static int64 ISFREE$3;
			int64 vr$7 = HFINDREGNOTINVREG( DVREG$1, -1ll );
			REG$3 = vr$7;
			char* vr$8 = HGETREGNAME( 2ll, REG$3 );
			fb_StrAssign( (void*)&AUX8$3, -1ll, (void*)vr$8, 0ll, 0 );
			char* vr$9 = HGETREGNAME( 8ll, REG$3 );
			fb_StrAssign( (void*)&AUX$3, -1ll, (void*)vr$9, 0ll, 0 );
			int64 vr$10 = HISREGFREE( 0ll, REG$3 );
			ISFREE$3 = vr$10;
			if( ISFREE$3 != 0ll) goto label$608;
			{
				HXCHG( (char*)*(char**)&AUX$3, (char*)"dword ptr [esp]" );
			}
			goto label$607;
			label$608:;
			{
				HMOV( (char*)*(char**)&AUX8$3, (char*)"byte ptr [esp]" );
			}
			label$607:;
			HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX8$3 );
			if( ISFREE$3 != 0ll) goto label$610;
			{
				HPOP( (char*)*(char**)&AUX$3 );
			}
			goto label$609;
			label$610:;
			{
				OUTP( (char*)"add esp, 4" );
			}
			label$609:;
		}
		label$605:;
	}
	goto label$603;
	label$604:;
	{
		int64 TMP$407$2;
		if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$611;
		TMP$407$2 = 24ll;
		goto label$1904;
		label$611:;
		TMP$407$2 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
		label$1904:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$407$2 * 56ll)) + 16ll) == 0ll) goto label$613;
		{
			FBSTRING TMP$408$3;
			FBSTRING TMP$409$3;
			OUTP( (char*)"sub esp, 4" );
			__builtin_memset( &TMP$408$3, 0, 24ll );
			FBSTRING* vr$21 = fb_StrConcat( &TMP$408$3, (void*)"fistp ", 7ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
			__builtin_memset( &TMP$409$3, 0, 24ll );
			FBSTRING* vr$24 = fb_StrConcat( &TMP$409$3, (void*)vr$21, -1ll, (void*)" [esp]", 7ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$24, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			if( DDSIZE$1 >= 4ll) goto label$615;
			{
				char* vr$26 = HGETREGNAME( 8ll, *(int64*)((uint8*)DVREG$1 + 24ll) );
				fb_StrAssign( (void*)&DST$1, -1ll, (void*)vr$26, 0ll, 0 );
			}
			label$615:;
			label$614:;
			HPOP( (char*)*(char**)&DST$1 );
		}
		goto label$612;
		label$613:;
		{
			if( DDSIZE$1 != 4ll) goto label$617;
			{
				OUTP( (char*)"sub esp, 8" );
				OUTP( (char*)"fistp qword ptr [esp]" );
				HPOP( (char*)*(char**)&DST$1 );
				OUTP( (char*)"add esp, 4" );
			}
			goto label$616;
			label$617:;
			{
				OUTP( (char*)"sub esp, 4" );
				OUTP( (char*)"fistp dword ptr [esp]" );
				HPOP( (char*)*(char**)&DST$1 );
				OUTP( (char*)"add esp, 2" );
			}
			label$616:;
		}
		label$612:;
	}
	label$603:;
	label$599:;
}

static void _EMITLOADL2F( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$618:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING AUX$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( ((int64)-(*(int64*)SVREG$1 == 4ll) | (int64)-(*(int64*)SVREG$1 == 0ll)) == 0ll) goto label$621;
	{
		int64 TMP$410$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$622;
		TMP$410$2 = 24ll;
		goto label$1905;
		label$622:;
		TMP$410$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1905:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$410$2 * 56ll)) + 16ll) == 0ll) goto label$624;
		{
			FBSTRING TMP$411$3;
			FBSTRING TMP$412$3;
			HPREPOPERAND64( SVREG$1, &SRC$1, &AUX$1 );
			HPUSH( (char*)*(char**)&AUX$1 );
			HPUSH( (char*)*(char**)&SRC$1 );
			__builtin_memset( &TMP$411$3, 0, 24ll );
			FBSTRING* vr$15 = fb_StrConcat( &TMP$411$3, (void*)"fild ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)SVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
			__builtin_memset( &TMP$412$3, 0, 24ll );
			FBSTRING* vr$18 = fb_StrConcat( &TMP$412$3, (void*)vr$15, -1ll, (void*)" [esp]", 7ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$18, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			OUTP( (char*)"add esp, 8" );
		}
		goto label$623;
		label$624:;
		{
			HPREPOPERAND64( SVREG$1, &SRC$1, &AUX$1 );
			HPUSH( (char*)*(char**)&AUX$1 );
			HPUSH( (char*)*(char**)&SRC$1 );
			OUTP( (char*)"fild qword ptr [esp]" );
			OUTP( (char*)"add esp, 8" );
			HULONG2DBL( SVREG$1 );
		}
		label$623:;
	}
	goto label$620;
	label$621:;
	{
		int64 TMP$413$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$625;
		TMP$413$2 = 24ll;
		goto label$1906;
		label$625:;
		TMP$413$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1906:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$413$2 * 56ll)) + 16ll) == 0ll) goto label$627;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$626;
		label$627:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			HULONG2DBL( SVREG$1 );
		}
		label$626:;
	}
	label$620:;
	label$619:;
}

static void _EMITLOADI2F( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$414$1;
	label$628:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 SDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$630;
	TMP$414$1 = 24ll;
	goto label$1907;
	label$630:;
	TMP$414$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1907:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$414$1 * 56ll)) + 8ll);
	if( SDSIZE$1 != 1ll) goto label$632;
	{
		int64 TMP$415$2;
		static FBSTRING AUX$2;
		static int64 ISFREE$2;
		static int64 REG$2;
		int64 vr$5 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$2 = vr$5;
		char* vr$6 = HGETREGNAME( 8ll, REG$2 );
		fb_StrAssign( (void*)&AUX$2, -1ll, (void*)vr$6, 0ll, 0 );
		int64 vr$7 = HISREGFREE( 0ll, REG$2 );
		ISFREE$2 = vr$7;
		if( ISFREE$2 != 0ll) goto label$634;
		{
			HPUSH( (char*)*(char**)&AUX$2 );
		}
		label$634:;
		label$633:;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$635;
		TMP$415$2 = 24ll;
		goto label$1908;
		label$635:;
		TMP$415$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1908:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$415$2 * 56ll)) + 16ll) == 0ll) goto label$637;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movsx ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$2, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$636;
		label$637:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"movzx ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&AUX$2, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$636:;
		HPUSH( (char*)*(char**)&AUX$2 );
		OUTP( (char*)"fild dword ptr [esp]" );
		OUTP( (char*)"add esp, 4" );
		if( ISFREE$2 != 0ll) goto label$639;
		{
			HPOP( (char*)*(char**)&AUX$2 );
		}
		label$639:;
		label$638:;
		goto label$629;
	}
	label$632:;
	label$631:;
	if( ((int64)-(*(int64*)SVREG$1 == 4ll) | (int64)-(*(int64*)SVREG$1 == 0ll)) == 0ll) goto label$641;
	{
		int64 TMP$416$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$642;
		TMP$416$2 = 24ll;
		goto label$1909;
		label$642:;
		TMP$416$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1909:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$416$2 * 56ll)) + 16ll) == 0ll) goto label$644;
		{
			FBSTRING TMP$417$3;
			FBSTRING TMP$418$3;
			if( ((int64)-(*(int64*)SVREG$1 == 4ll) & (int64)-(SDSIZE$1 < 4ll)) == 0ll) goto label$646;
			{
				char* vr$28 = HGETREGNAME( 8ll, *(int64*)((uint8*)SVREG$1 + 24ll) );
				fb_StrAssign( (void*)&SRC$1, -1ll, (void*)vr$28, 0ll, 0 );
			}
			label$646:;
			label$645:;
			HPUSH( (char*)*(char**)&SRC$1 );
			__builtin_memset( &TMP$417$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$417$3, (void*)"fild ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)SVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
			__builtin_memset( &TMP$418$3, 0, 24ll );
			FBSTRING* vr$37 = fb_StrConcat( &TMP$418$3, (void*)vr$34, -1ll, (void*)" [esp]", 7ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$37, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			OUTP( (char*)"add esp, 4" );
		}
		goto label$643;
		label$644:;
		{
			if( SDSIZE$1 != 4ll) goto label$648;
			{
				HPUSH( (char*)"0" );
				HPUSH( (char*)*(char**)&SRC$1 );
				OUTP( (char*)"fild qword ptr [esp]" );
				OUTP( (char*)"add esp, 8" );
			}
			goto label$647;
			label$648:;
			{
				if( *(int64*)SVREG$1 == 0ll) goto label$650;
				{
					HPUSH( (char*)"0" );
				}
				label$650:;
				label$649:;
				HPUSH( (char*)*(char**)&SRC$1 );
				OUTP( (char*)"fild dword ptr [esp]" );
				if( *(int64*)SVREG$1 == 0ll) goto label$652;
				{
					OUTP( (char*)"add esp, 6" );
				}
				goto label$651;
				label$652:;
				{
					OUTP( (char*)"add esp, 4" );
				}
				label$651:;
			}
			label$647:;
		}
		label$643:;
	}
	goto label$640;
	label$641:;
	{
		int64 TMP$419$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$653;
		TMP$419$2 = 24ll;
		goto label$1910;
		label$653:;
		TMP$419$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1910:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$419$2 * 56ll)) + 16ll) == 0ll) goto label$655;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fild ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$654;
		label$655:;
		{
			if( SDSIZE$1 != 4ll) goto label$657;
			{
				HPUSH( (char*)"0" );
				HPUSH( (char*)*(char**)&SRC$1 );
				OUTP( (char*)"fild qword ptr [esp]" );
				OUTP( (char*)"add esp, 8" );
			}
			goto label$656;
			label$657:;
			{
				HPUSH( (char*)"0" );
				HPUSH( (char*)*(char**)&SRC$1 );
				OUTP( (char*)"fild dword ptr [esp]" );
				OUTP( (char*)"add esp, 6" );
			}
			label$656:;
		}
		label$654:;
	}
	label$640:;
	label$629:;
}

static void _EMITLOADF2F( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$658:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fld ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$659:;
}

static void _EMITMOVL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$660:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$661:;
}

static void _EMITMOVI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$420$1;
	label$662:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$664;
	TMP$420$1 = 24ll;
	goto label$1911;
	label$664:;
	TMP$420$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1911:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$420$1 * 56ll)) + 8ll) != 1ll) goto label$666;
	{
		char* vr$6 = HGETREGNAME( 8ll, *(int64*)((uint8*)DVREG$1 + 24ll) );
		fb_StrAssign( (void*)&DST$1, -1ll, (void*)vr$6, 0ll, 0 );
		char* vr$8 = HGETREGNAME( 8ll, *(int64*)((uint8*)SVREG$1 + 24ll) );
		fb_StrAssign( (void*)&SRC$1, -1ll, (void*)vr$8, 0ll, 0 );
	}
	goto label$665;
	label$666:;
	{
		HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
		HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	}
	label$665:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$663:;
}

static void _EMITMOVF( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$667:;
	label$668:;
}

static void _EMITADDL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$669:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"add ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"adc ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$670:;
}

static void _EMITADDI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$671:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DOINC$1;
	static int64 DODEC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	DOINC$1 = 0ll;
	DODEC$1 = 0ll;
	if( *(int64*)SVREG$1 != 0ll) goto label$674;
	{
		{
			int64 TMP$423$3;
			TMP$423$3 = *(int64*)((uint8*)SVREG$1 + 48ll);
			if( TMP$423$3 != 1ll) goto label$676;
			label$677:;
			{
				DOINC$1 = -1ll;
			}
			goto label$675;
			label$676:;
			if( TMP$423$3 != -1ll) goto label$678;
			label$679:;
			{
				DODEC$1 = -1ll;
			}
			label$678:;
			label$675:;
		}
	}
	label$674:;
	label$673:;
	if( DOINC$1 == 0ll) goto label$681;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"inc ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$680;
	label$681:;
	if( DODEC$1 == 0ll) goto label$682;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"dec ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$680;
	label$682:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"add ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$680:;
	label$672:;
}

static void _EMITADDF( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$683:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 != 4ll) goto label$686;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"faddp", 6ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$685;
	label$686:;
	{
		int64 TMP$426$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$687;
		TMP$426$2 = 24ll;
		goto label$1912;
		label$687:;
		TMP$426$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1912:;
		if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$426$2 * 56ll)) != 1ll) goto label$689;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fadd ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$688;
		label$689:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fiadd ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$688:;
	}
	label$685:;
	label$684:;
}

static void _EMITSUBL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$690:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sub ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sbb ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$691:;
}

static void _EMITSUBI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$692:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 DOINC$1;
	static int64 DODEC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	DOINC$1 = 0ll;
	DODEC$1 = 0ll;
	if( *(int64*)SVREG$1 != 0ll) goto label$695;
	{
		{
			int64 TMP$431$3;
			TMP$431$3 = *(int64*)((uint8*)SVREG$1 + 48ll);
			if( TMP$431$3 != 1ll) goto label$697;
			label$698:;
			{
				DODEC$1 = -1ll;
			}
			goto label$696;
			label$697:;
			if( TMP$431$3 != -1ll) goto label$699;
			label$700:;
			{
				DOINC$1 = -1ll;
			}
			label$699:;
			label$696:;
		}
	}
	label$695:;
	label$694:;
	if( DODEC$1 == 0ll) goto label$702;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"dec ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$701;
	label$702:;
	if( DOINC$1 == 0ll) goto label$703;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"inc ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$701;
	label$703:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sub ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$701:;
	label$693:;
}

static void _EMITSUBF( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$704:;
	static FBSTRING SRC$1;
	static int64 DOINC$1;
	static int64 DODEC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 != 4ll) goto label$707;
	{
		OUTP( (char*)"fsubrp" );
	}
	goto label$706;
	label$707:;
	{
		int64 TMP$433$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$708;
		TMP$433$2 = 24ll;
		goto label$1913;
		label$708:;
		TMP$433$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1913:;
		if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$433$2 * 56ll)) != 1ll) goto label$710;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fsub ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$709;
		label$710:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fisub ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$709:;
	}
	label$706:;
	label$705:;
}

static void _EMITMULL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	FBSTRING TMP$437$1;
	FBSTRING TMP$438$1;
	FBSTRING TMP$439$1;
	FBSTRING TMP$441$1;
	FBSTRING TMP$442$1;
	FBSTRING TMP$443$1;
	FBSTRING TMP$445$1;
	FBSTRING TMP$446$1;
	FBSTRING TMP$447$1;
	FBSTRING TMP$449$1;
	FBSTRING TMP$450$1;
	FBSTRING TMP$451$1;
	FBSTRING TMP$454$1;
	FBSTRING TMP$455$1;
	FBSTRING TMP$456$1;
	FBSTRING TMP$458$1;
	FBSTRING TMP$459$1;
	FBSTRING TMP$460$1;
	FBSTRING TMP$464$1;
	FBSTRING TMP$465$1;
	FBSTRING TMP$466$1;
	label$711:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static int64 ISEAXFREE$1;
	static int64 ISEDXFREE$1;
	static int64 EAXINDEST$1;
	static int64 EDXINDEST$1;
	static int64 OFS$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	int64 vr$0 = HISREGFREE( 0ll, 5ll );
	ISEAXFREE$1 = vr$0;
	int64 vr$1 = HISREGFREE( 0ll, 0ll );
	ISEDXFREE$1 = vr$1;
	int64 vr$2 = HISREGINVREG( DVREG$1, 5ll );
	EAXINDEST$1 = vr$2;
	int64 vr$3 = HISREGINVREG( DVREG$1, 0ll );
	EDXINDEST$1 = vr$3;
	HPUSH( (char*)*(char**)&SRC2$1 );
	HPUSH( (char*)*(char**)&SRC1$1 );
	HPUSH( (char*)*(char**)&DST2$1 );
	HPUSH( (char*)*(char**)&DST1$1 );
	OFS$1 = 0ll;
	if( EDXINDEST$1 == 0ll) goto label$714;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$716;
		{
			OFS$1 = OFS$1 + 4ll;
			HPUSH( (char*)"edx" );
		}
		label$716:;
		label$715:;
	}
	goto label$713;
	label$714:;
	{
		if( ISEDXFREE$1 != 0ll) goto label$718;
		{
			OFS$1 = OFS$1 + 4ll;
			HPUSH( (char*)"edx" );
		}
		label$718:;
		label$717:;
	}
	label$713:;
	if( EAXINDEST$1 == 0ll) goto label$720;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$722;
		{
			OFS$1 = OFS$1 + 4ll;
			HPUSH( (char*)"eax" );
		}
		label$722:;
		label$721:;
	}
	goto label$719;
	label$720:;
	{
		if( ISEAXFREE$1 != 0ll) goto label$724;
		{
			OFS$1 = OFS$1 + 4ll;
			HPUSH( (char*)"eax" );
		}
		label$724:;
		label$723:;
	}
	label$719:;
	__builtin_memset( &TMP$439$1, 0, 24ll );
	FBSTRING* vr$11 = fb_LongintToStr( OFS$1 );
	__builtin_memset( &TMP$437$1, 0, 24ll );
	FBSTRING* vr$14 = fb_StrConcat( &TMP$437$1, (void*)"mov eax, [esp+", 15ll, (void*)vr$11, -1ll );
	__builtin_memset( &TMP$438$1, 0, 24ll );
	FBSTRING* vr$17 = fb_StrConcat( &TMP$438$1, (void*)vr$14, -1ll, (void*)"]", 2ll );
	fb_StrAssign( (void*)&TMP$439$1, -1ll, (void*)vr$17, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$439$1 );
	fb_StrDelete( (FBSTRING*)&TMP$439$1 );
	__builtin_memset( &TMP$443$1, 0, 24ll );
	FBSTRING* vr$22 = fb_LongintToStr( OFS$1 + 8ll );
	__builtin_memset( &TMP$441$1, 0, 24ll );
	FBSTRING* vr$25 = fb_StrConcat( &TMP$441$1, (void*)"mul dword ptr [esp+", 20ll, (void*)vr$22, -1ll );
	__builtin_memset( &TMP$442$1, 0, 24ll );
	FBSTRING* vr$28 = fb_StrConcat( &TMP$442$1, (void*)vr$25, -1ll, (void*)"]", 2ll );
	fb_StrAssign( (void*)&TMP$443$1, -1ll, (void*)vr$28, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$443$1 );
	fb_StrDelete( (FBSTRING*)&TMP$443$1 );
	__builtin_memset( &TMP$447$1, 0, 24ll );
	FBSTRING* vr$32 = fb_LongintToStr( OFS$1 );
	__builtin_memset( &TMP$445$1, 0, 24ll );
	FBSTRING* vr$35 = fb_StrConcat( &TMP$445$1, (void*)"xchg eax, [esp+", 16ll, (void*)vr$32, -1ll );
	__builtin_memset( &TMP$446$1, 0, 24ll );
	FBSTRING* vr$38 = fb_StrConcat( &TMP$446$1, (void*)vr$35, -1ll, (void*)"]", 2ll );
	fb_StrAssign( (void*)&TMP$447$1, -1ll, (void*)vr$38, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$447$1 );
	fb_StrDelete( (FBSTRING*)&TMP$447$1 );
	__builtin_memset( &TMP$451$1, 0, 24ll );
	FBSTRING* vr$43 = fb_LongintToStr( OFS$1 + 12ll );
	__builtin_memset( &TMP$449$1, 0, 24ll );
	FBSTRING* vr$46 = fb_StrConcat( &TMP$449$1, (void*)"imul eax, [esp+", 16ll, (void*)vr$43, -1ll );
	__builtin_memset( &TMP$450$1, 0, 24ll );
	FBSTRING* vr$49 = fb_StrConcat( &TMP$450$1, (void*)vr$46, -1ll, (void*)"]", 2ll );
	fb_StrAssign( (void*)&TMP$451$1, -1ll, (void*)vr$49, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$451$1 );
	fb_StrDelete( (FBSTRING*)&TMP$451$1 );
	OUTP( (char*)"add eax, edx" );
	__builtin_memset( &TMP$456$1, 0, 24ll );
	FBSTRING* vr$54 = fb_LongintToStr( OFS$1 + 4ll );
	__builtin_memset( &TMP$454$1, 0, 24ll );
	FBSTRING* vr$57 = fb_StrConcat( &TMP$454$1, (void*)"mov edx, [esp+", 15ll, (void*)vr$54, -1ll );
	__builtin_memset( &TMP$455$1, 0, 24ll );
	FBSTRING* vr$60 = fb_StrConcat( &TMP$455$1, (void*)vr$57, -1ll, (void*)"]", 2ll );
	fb_StrAssign( (void*)&TMP$456$1, -1ll, (void*)vr$60, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$456$1 );
	fb_StrDelete( (FBSTRING*)&TMP$456$1 );
	__builtin_memset( &TMP$460$1, 0, 24ll );
	FBSTRING* vr$65 = fb_LongintToStr( OFS$1 + 8ll );
	__builtin_memset( &TMP$458$1, 0, 24ll );
	FBSTRING* vr$68 = fb_StrConcat( &TMP$458$1, (void*)"imul edx, [esp+", 16ll, (void*)vr$65, -1ll );
	__builtin_memset( &TMP$459$1, 0, 24ll );
	FBSTRING* vr$71 = fb_StrConcat( &TMP$459$1, (void*)vr$68, -1ll, (void*)"]", 2ll );
	fb_StrAssign( (void*)&TMP$460$1, -1ll, (void*)vr$71, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$460$1 );
	fb_StrDelete( (FBSTRING*)&TMP$460$1 );
	OUTP( (char*)"add edx, eax" );
	__builtin_memset( &TMP$466$1, 0, 24ll );
	FBSTRING* vr$76 = fb_LongintToStr( OFS$1 + 4ll );
	__builtin_memset( &TMP$464$1, 0, 24ll );
	FBSTRING* vr$79 = fb_StrConcat( &TMP$464$1, (void*)"mov [esp+", 10ll, (void*)vr$76, -1ll );
	__builtin_memset( &TMP$465$1, 0, 24ll );
	FBSTRING* vr$82 = fb_StrConcat( &TMP$465$1, (void*)vr$79, -1ll, (void*)"], edx", 7ll );
	fb_StrAssign( (void*)&TMP$466$1, -1ll, (void*)vr$82, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$466$1 );
	fb_StrDelete( (FBSTRING*)&TMP$466$1 );
	if( EAXINDEST$1 == 0ll) goto label$726;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$728;
		{
			HPOP( (char*)"eax" );
		}
		label$728:;
		label$727:;
	}
	goto label$725;
	label$726:;
	{
		if( ISEAXFREE$1 != 0ll) goto label$730;
		{
			HPOP( (char*)"eax" );
		}
		label$730:;
		label$729:;
	}
	label$725:;
	if( EDXINDEST$1 == 0ll) goto label$732;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$734;
		{
			HPOP( (char*)"edx" );
		}
		label$734:;
		label$733:;
	}
	goto label$731;
	label$732:;
	{
		if( ISEDXFREE$1 != 0ll) goto label$736;
		{
			HPOP( (char*)"edx" );
		}
		label$736:;
		label$735:;
	}
	label$731:;
	HPOP( (char*)*(char**)&DST1$1 );
	HPOP( (char*)*(char**)&DST2$1 );
	OUTP( (char*)"add esp, 8" );
	label$712:;
}

static void _EMITMULI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$737:;
	static int64 REG$1;
	static int64 ISFREE$1;
	static FBSTRING RNAME$1;
	static FBSTRING OSTR$1;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)DVREG$1 == 4ll) goto label$740;
	{
		int64 vr$1 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$1 = vr$1;
		char* vr$3 = HGETREGNAME( *(int64*)((uint8*)SVREG$1 + 8ll), REG$1 );
		fb_StrAssign( (void*)&RNAME$1, -1ll, (void*)vr$3, 0ll, 0 );
		int64 vr$4 = HISREGFREE( 0ll, REG$1 );
		ISFREE$1 = vr$4;
		if( ISFREE$1 != 0ll) goto label$742;
		{
			HPUSH( (char*)*(char**)&RNAME$1 );
		}
		label$742:;
		label$741:;
		HMOV( (char*)*(char**)&RNAME$1, (char*)*(char**)&DST$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"imul ", 6ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&RNAME$1 );
		if( ISFREE$1 != 0ll) goto label$744;
		{
			HPOP( (char*)*(char**)&RNAME$1 );
		}
		label$744:;
		label$743:;
	}
	goto label$739;
	label$740:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"imul ", 6ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$739:;
	label$738:;
}

static void _EMITMULF( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$745:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 != 4ll) goto label$748;
	{
		OUTP( (char*)"fmulp" );
	}
	goto label$747;
	label$748:;
	{
		int64 TMP$469$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$749;
		TMP$469$2 = 24ll;
		goto label$1914;
		label$749:;
		TMP$469$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1914:;
		if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$469$2 * 56ll)) != 1ll) goto label$751;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fmul ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$750;
		label$751:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fimul ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$750:;
	}
	label$747:;
	label$746:;
}

static void _EMITDIVF( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$752:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 != 4ll) goto label$755;
	{
		OUTP( (char*)"fdivrp" );
	}
	goto label$754;
	label$755:;
	{
		int64 TMP$473$2;
		if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$756;
		TMP$473$2 = 24ll;
		goto label$1915;
		label$756:;
		TMP$473$2 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
		label$1915:;
		if( *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$473$2 * 56ll)) != 1ll) goto label$758;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fdiv ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$757;
		label$758:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fidiv ", 7ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$757:;
	}
	label$754:;
	label$753:;
}

static void _EMITDIVI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$476$1;
	int64 TMP$478$1;
	label$759:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 ECXTRASHED$1;
	static int64 EAXFREE$1;
	static int64 ECXFREE$1;
	static int64 EDXFREE$1;
	static int64 EAXINDEST$1;
	static int64 ECXINDEST$1;
	static int64 EDXINDEST$1;
	static int64 EAXINSOURCE$1;
	static int64 EDXINSOURCE$1;
	static FBSTRING EAX$1;
	static FBSTRING ECX$1;
	static FBSTRING EDX$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$761;
	TMP$476$1 = 24ll;
	goto label$1916;
	label$761:;
	TMP$476$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1916:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$476$1 * 56ll)) + 8ll) != 4ll) goto label$763;
	{
		fb_StrAssign( (void*)&EAX$1, -1ll, (void*)"eax", 4ll, 0 );
		fb_StrAssign( (void*)&ECX$1, -1ll, (void*)"ecx", 4ll, 0 );
		fb_StrAssign( (void*)&EDX$1, -1ll, (void*)"edx", 4ll, 0 );
	}
	goto label$762;
	label$763:;
	{
		fb_StrAssign( (void*)&EAX$1, -1ll, (void*)"ax", 3ll, 0 );
		fb_StrAssign( (void*)&ECX$1, -1ll, (void*)"cx", 3ll, 0 );
		fb_StrAssign( (void*)&EDX$1, -1ll, (void*)"dx", 3ll, 0 );
	}
	label$762:;
	ECXTRASHED$1 = 0ll;
	int64 vr$5 = HISREGFREE( 0ll, 5ll );
	EAXFREE$1 = vr$5;
	int64 vr$6 = HISREGFREE( 0ll, 3ll );
	ECXFREE$1 = vr$6;
	int64 vr$7 = HISREGFREE( 0ll, 0ll );
	EDXFREE$1 = vr$7;
	int64 vr$8 = HISREGINVREG( SVREG$1, 5ll );
	EAXINSOURCE$1 = vr$8;
	int64 vr$9 = HISREGINVREG( SVREG$1, 0ll );
	EDXINSOURCE$1 = vr$9;
	int64 vr$10 = HISREGINVREG( DVREG$1, 5ll );
	EAXINDEST$1 = vr$10;
	int64 vr$11 = HISREGINVREG( DVREG$1, 0ll );
	EDXINDEST$1 = vr$11;
	int64 vr$12 = HISREGINVREG( DVREG$1, 3ll );
	ECXINDEST$1 = vr$12;
	if( ((EAXINSOURCE$1 | EDXINSOURCE$1) | (int64)-(*(int64*)SVREG$1 == 0ll)) == 0ll) goto label$765;
	{
		ECXTRASHED$1 = -1ll;
		if( ECXINDEST$1 == 0ll) goto label$767;
		{
			HPUSH( (char*)"ecx" );
			if( *(int64*)DVREG$1 == 4ll) goto label$769;
			{
				HPREPOPERAND( DVREG$1, &OSTR$1, 8ll, 0ll, 0ll, -1ll );
				HPUSH( (char*)*(char**)&OSTR$1 );
			}
			label$769:;
			label$768:;
		}
		goto label$766;
		label$767:;
		if( ECXFREE$1 != 0ll) goto label$770;
		{
			HPUSH( (char*)"ecx" );
		}
		label$770:;
		label$766:;
		HMOV( (char*)*(char**)&ECX$1, (char*)*(char**)&SRC$1 );
		fb_StrAssign( (void*)&SRC$1, -1ll, (void*)&ECX$1, -1ll, 0 );
	}
	label$765:;
	label$764:;
	if( EAXINDEST$1 != 0ll) goto label$772;
	{
		if( (ECXINDEST$1 & ECXTRASHED$1) == 0ll) goto label$774;
		{
			if( EAXFREE$1 != 0ll) goto label$776;
			{
				OUTP( (char*)"xchg eax, [esp]" );
			}
			goto label$775;
			label$776:;
			{
				HPOP( (char*)"eax" );
			}
			label$775:;
		}
		goto label$773;
		label$774:;
		{
			if( EAXFREE$1 != 0ll) goto label$778;
			{
				HPUSH( (char*)"eax" );
			}
			label$778:;
			label$777:;
			HMOV( (char*)*(char**)&EAX$1, (char*)*(char**)&DST$1 );
		}
		label$773:;
	}
	goto label$771;
	label$772:;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$780;
		{
			HPUSH( (char*)"eax" );
			HMOV( (char*)*(char**)&EAX$1, (char*)*(char**)&DST$1 );
		}
		label$780:;
		label$779:;
	}
	label$771:;
	if( EDXINDEST$1 == 0ll) goto label$782;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$784;
		{
			HPUSH( (char*)"edx" );
		}
		label$784:;
		label$783:;
	}
	goto label$781;
	label$782:;
	if( EDXFREE$1 != 0ll) goto label$785;
	{
		HPUSH( (char*)"edx" );
	}
	label$785:;
	label$781:;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$786;
	TMP$478$1 = 24ll;
	goto label$1917;
	label$786:;
	TMP$478$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1917:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$478$1 * 56ll)) + 16ll) == 0ll) goto label$788;
	{
		int64 TMP$479$2;
		if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$789;
		TMP$479$2 = 24ll;
		goto label$1918;
		label$789:;
		TMP$479$2 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
		label$1918:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$479$2 * 56ll)) + 8ll) != 4ll) goto label$791;
		{
			OUTP( (char*)"cdq" );
		}
		goto label$790;
		label$791:;
		{
			OUTP( (char*)"cwd" );
		}
		label$790:;
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"idiv ", 6ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$787;
	label$788:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&EDX$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&EDX$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"div ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$787:;
	if( EDXINDEST$1 == 0ll) goto label$793;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$795;
		{
			HPOP( (char*)"edx" );
		}
		label$795:;
		label$794:;
	}
	goto label$792;
	label$793:;
	if( EDXFREE$1 != 0ll) goto label$796;
	{
		HPOP( (char*)"edx" );
	}
	label$796:;
	label$792:;
	if( EAXINDEST$1 != 0ll) goto label$798;
	{
		if( (ECXINDEST$1 & ECXTRASHED$1) == 0ll) goto label$800;
		{
			if( *(int64*)DVREG$1 == 4ll) goto label$802;
			{
				if( EAXFREE$1 != 0ll) goto label$804;
				{
					HPOP( (char*)"ecx" );
					OUTP( (char*)"xchg ecx, [esp]" );
				}
				goto label$803;
				label$804:;
				{
					HPOP( (char*)"ecx" );
				}
				label$803:;
			}
			label$802:;
			label$801:;
		}
		label$800:;
		label$799:;
		HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&EAX$1 );
		if( EAXFREE$1 != 0ll) goto label$806;
		{
			HPOP( (char*)"eax" );
		}
		label$806:;
		label$805:;
	}
	goto label$797;
	label$798:;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$808;
		{
			if( ((int64)-(ECXFREE$1 == 0ll) & (int64)-(ECXTRASHED$1 == 0ll)) == 0ll) goto label$810;
			{
				OUTP( (char*)"xchg ecx, [esp]" );
				OUTP( (char*)"xchg ecx, eax" );
			}
			goto label$809;
			label$810:;
			{
				HMOV( (char*)"ecx", (char*)"eax" );
				HPOP( (char*)"eax" );
			}
			label$809:;
			HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&ECX$1 );
			if( ((int64)-(ECXFREE$1 == 0ll) & (int64)-(ECXTRASHED$1 == 0ll)) == 0ll) goto label$812;
			{
				HPOP( (char*)"ecx" );
			}
			label$812:;
			label$811:;
		}
		label$808:;
		label$807:;
	}
	label$797:;
	if( ECXTRASHED$1 == 0ll) goto label$814;
	{
		if( ((int64)-(ECXFREE$1 == 0ll) & (int64)-(ECXINDEST$1 == 0ll)) == 0ll) goto label$816;
		{
			HPOP( (char*)"ecx" );
		}
		label$816:;
		label$815:;
	}
	label$814:;
	label$813:;
	label$760:;
}

static void _EMITMODI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$486$1;
	int64 TMP$487$1;
	label$817:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static int64 ECXTRASHED$1;
	static int64 EAXFREE$1;
	static int64 ECXFREE$1;
	static int64 EDXFREE$1;
	static int64 EAXINDEST$1;
	static int64 ECXINDEST$1;
	static int64 EDXINDEST$1;
	static int64 EAXINSOURCE$1;
	static int64 EDXINSOURCE$1;
	static FBSTRING EAX$1;
	static FBSTRING ECX$1;
	static FBSTRING EDX$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$819;
	TMP$486$1 = 24ll;
	goto label$1919;
	label$819:;
	TMP$486$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1919:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$486$1 * 56ll)) + 8ll) != 4ll) goto label$821;
	{
		fb_StrAssign( (void*)&EAX$1, -1ll, (void*)"eax", 4ll, 0 );
		fb_StrAssign( (void*)&ECX$1, -1ll, (void*)"ecx", 4ll, 0 );
		fb_StrAssign( (void*)&EDX$1, -1ll, (void*)"edx", 4ll, 0 );
	}
	goto label$820;
	label$821:;
	{
		fb_StrAssign( (void*)&EAX$1, -1ll, (void*)"ax", 3ll, 0 );
		fb_StrAssign( (void*)&ECX$1, -1ll, (void*)"cx", 3ll, 0 );
		fb_StrAssign( (void*)&EDX$1, -1ll, (void*)"dx", 3ll, 0 );
	}
	label$820:;
	ECXTRASHED$1 = 0ll;
	int64 vr$5 = HISREGFREE( 0ll, 5ll );
	EAXFREE$1 = vr$5;
	int64 vr$6 = HISREGFREE( 0ll, 3ll );
	ECXFREE$1 = vr$6;
	int64 vr$7 = HISREGFREE( 0ll, 0ll );
	EDXFREE$1 = vr$7;
	int64 vr$8 = HISREGINVREG( SVREG$1, 5ll );
	EAXINSOURCE$1 = vr$8;
	int64 vr$9 = HISREGINVREG( SVREG$1, 0ll );
	EDXINSOURCE$1 = vr$9;
	int64 vr$10 = HISREGINVREG( DVREG$1, 5ll );
	EAXINDEST$1 = vr$10;
	int64 vr$11 = HISREGINVREG( DVREG$1, 0ll );
	EDXINDEST$1 = vr$11;
	int64 vr$12 = HISREGINVREG( DVREG$1, 3ll );
	ECXINDEST$1 = vr$12;
	if( ((EAXINSOURCE$1 | EDXINSOURCE$1) | (int64)-(*(int64*)SVREG$1 == 0ll)) == 0ll) goto label$823;
	{
		ECXTRASHED$1 = -1ll;
		if( ECXINDEST$1 == 0ll) goto label$825;
		{
			HPUSH( (char*)"ecx" );
			if( *(int64*)DVREG$1 == 4ll) goto label$827;
			{
				HPREPOPERAND( DVREG$1, &OSTR$1, 8ll, 0ll, 0ll, -1ll );
				HPUSH( (char*)*(char**)&OSTR$1 );
			}
			label$827:;
			label$826:;
		}
		goto label$824;
		label$825:;
		if( ECXFREE$1 != 0ll) goto label$828;
		{
			HPUSH( (char*)"ecx" );
		}
		label$828:;
		label$824:;
		HMOV( (char*)*(char**)&ECX$1, (char*)*(char**)&SRC$1 );
		fb_StrAssign( (void*)&SRC$1, -1ll, (void*)&ECX$1, -1ll, 0 );
	}
	label$823:;
	label$822:;
	if( EAXINDEST$1 != 0ll) goto label$830;
	{
		if( (ECXINDEST$1 & ECXTRASHED$1) == 0ll) goto label$832;
		{
			if( EAXFREE$1 != 0ll) goto label$834;
			{
				OUTP( (char*)"xchg eax, [esp]" );
			}
			goto label$833;
			label$834:;
			{
				HPOP( (char*)"eax" );
			}
			label$833:;
		}
		goto label$831;
		label$832:;
		{
			if( EAXFREE$1 != 0ll) goto label$836;
			{
				HPUSH( (char*)"eax" );
			}
			label$836:;
			label$835:;
			HMOV( (char*)*(char**)&EAX$1, (char*)*(char**)&DST$1 );
		}
		label$831:;
	}
	goto label$829;
	label$830:;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$838;
		{
			HPUSH( (char*)"eax" );
			HMOV( (char*)*(char**)&EAX$1, (char*)*(char**)&DST$1 );
		}
		label$838:;
		label$837:;
	}
	label$829:;
	if( EDXINDEST$1 == 0ll) goto label$840;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$842;
		{
			HPUSH( (char*)"edx" );
		}
		label$842:;
		label$841:;
	}
	goto label$839;
	label$840:;
	if( EDXFREE$1 != 0ll) goto label$843;
	{
		HPUSH( (char*)"edx" );
	}
	label$843:;
	label$839:;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$844;
	TMP$487$1 = 24ll;
	goto label$1920;
	label$844:;
	TMP$487$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1920:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$487$1 * 56ll)) + 16ll) == 0ll) goto label$846;
	{
		int64 TMP$488$2;
		if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$847;
		TMP$488$2 = 24ll;
		goto label$1921;
		label$847:;
		TMP$488$2 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
		label$1921:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$488$2 * 56ll)) + 8ll) != 4ll) goto label$849;
		{
			OUTP( (char*)"cdq" );
		}
		goto label$848;
		label$849:;
		{
			OUTP( (char*)"cwd" );
		}
		label$848:;
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"idiv ", 6ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$845;
	label$846:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&EDX$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&EDX$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"div ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$845:;
	HMOV( (char*)*(char**)&EAX$1, (char*)*(char**)&EDX$1 );
	if( EDXINDEST$1 == 0ll) goto label$851;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$853;
		{
			HPOP( (char*)"edx" );
		}
		label$853:;
		label$852:;
	}
	goto label$850;
	label$851:;
	if( EDXFREE$1 != 0ll) goto label$854;
	{
		HPOP( (char*)"edx" );
	}
	label$854:;
	label$850:;
	if( EAXINDEST$1 != 0ll) goto label$856;
	{
		if( (ECXINDEST$1 & ECXTRASHED$1) == 0ll) goto label$858;
		{
			if( *(int64*)DVREG$1 == 4ll) goto label$860;
			{
				if( EAXFREE$1 != 0ll) goto label$862;
				{
					HPOP( (char*)"ecx" );
					OUTP( (char*)"xchg ecx, [esp]" );
				}
				goto label$861;
				label$862:;
				{
					HPOP( (char*)"ecx" );
				}
				label$861:;
			}
			label$860:;
			label$859:;
		}
		label$858:;
		label$857:;
		HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&EAX$1 );
		if( EAXFREE$1 != 0ll) goto label$864;
		{
			HPOP( (char*)"eax" );
		}
		label$864:;
		label$863:;
	}
	goto label$855;
	label$856:;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$866;
		{
			if( ((int64)-(ECXFREE$1 == 0ll) & (int64)-(ECXTRASHED$1 == 0ll)) == 0ll) goto label$868;
			{
				OUTP( (char*)"xchg ecx, [esp]" );
				OUTP( (char*)"xchg ecx, eax" );
			}
			goto label$867;
			label$868:;
			{
				HMOV( (char*)"ecx", (char*)"eax" );
				HPOP( (char*)"eax" );
			}
			label$867:;
			HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&ECX$1 );
			if( ((int64)-(ECXFREE$1 == 0ll) & (int64)-(ECXTRASHED$1 == 0ll)) == 0ll) goto label$870;
			{
				HPOP( (char*)"ecx" );
			}
			label$870:;
			label$869:;
		}
		label$866:;
		label$865:;
	}
	label$855:;
	if( ECXTRASHED$1 == 0ll) goto label$872;
	{
		if( ((int64)-(ECXFREE$1 == 0ll) & (int64)-(ECXINDEST$1 == 0ll)) == 0ll) goto label$874;
		{
			HPOP( (char*)"ecx" );
		}
		label$874:;
		label$873:;
	}
	label$872:;
	label$871:;
	label$818:;
}

static void HSHIFTL( int64 OP$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$875:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC$1;
	static FBSTRING LABEL$1;
	static FBSTRING MNEMONIC32$1;
	static FBSTRING MNEMONIC64$1;
	static int64 TMPREG$1;
	static int64 TMPISFREE$1;
	static FBSTRING TMPREGNAME$1;
	static FBSTRING A$1;
	static FBSTRING B$1;
	static struct $6IRVREG* AV$1;
	static struct $6IRVREG* BV$1;
	if( OP$1 != 41ll) goto label$878;
	{
		fb_StrAssign( (void*)&MNEMONIC32$1, -1ll, (void*)"shl ", 5ll, 0 );
		fb_StrAssign( (void*)&MNEMONIC64$1, -1ll, (void*)"shld ", 6ll, 0 );
	}
	goto label$877;
	label$878:;
	{
		int64 TMP$491$2;
		if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$879;
		TMP$491$2 = 24ll;
		goto label$1922;
		label$879:;
		TMP$491$2 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
		label$1922:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$491$2 * 56ll)) + 16ll) == 0ll) goto label$881;
		{
			fb_StrAssign( (void*)&MNEMONIC32$1, -1ll, (void*)"sar ", 5ll, 0 );
		}
		goto label$880;
		label$881:;
		{
			fb_StrAssign( (void*)&MNEMONIC32$1, -1ll, (void*)"shr ", 5ll, 0 );
		}
		label$880:;
		fb_StrAssign( (void*)&MNEMONIC64$1, -1ll, (void*)"shrd ", 6ll, 0 );
	}
	label$877:;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND( SVREG$1, &SRC$1, 8ll, 0ll, 0ll, -1ll );
	if( OP$1 != 41ll) goto label$883;
	{
		fb_StrAssign( (void*)&A$1, -1ll, (void*)&DST2$1, -1ll, 0 );
		AV$1 = *(struct $6IRVREG**)((uint8*)DVREG$1 + 88ll);
		fb_StrAssign( (void*)&B$1, -1ll, (void*)&DST1$1, -1ll, 0 );
		BV$1 = DVREG$1;
	}
	goto label$882;
	label$883:;
	{
		fb_StrAssign( (void*)&A$1, -1ll, (void*)&DST1$1, -1ll, 0 );
		AV$1 = DVREG$1;
		fb_StrAssign( (void*)&B$1, -1ll, (void*)&DST2$1, -1ll, 0 );
		BV$1 = *(struct $6IRVREG**)((uint8*)DVREG$1 + 88ll);
	}
	label$882:;
	if( *(int64*)SVREG$1 != 0ll) goto label$885;
	{
		if( *(int64*)((uint8*)SVREG$1 + 48ll) < 64ll) goto label$887;
		{
			if( *(int64*)BV$1 != 4ll) goto label$889;
			{
				FBSTRING TMP$494$4;
				FBSTRING TMP$495$4;
				FBSTRING TMP$496$4;
				FBSTRING TMP$497$4;
				__builtin_memset( &TMP$497$4, 0, 24ll );
				__builtin_memset( &TMP$494$4, 0, 24ll );
				FBSTRING* vr$13 = fb_StrConcat( &TMP$494$4, (void*)"xor ", 5ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$495$4, 0, 24ll );
				FBSTRING* vr$16 = fb_StrConcat( &TMP$495$4, (void*)vr$13, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$496$4, 0, 24ll );
				FBSTRING* vr$19 = fb_StrConcat( &TMP$496$4, (void*)vr$16, -1ll, (void*)&B$1, -1ll );
				fb_StrAssign( (void*)&TMP$497$4, -1ll, (void*)vr$19, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$497$4 );
				fb_StrDelete( (FBSTRING*)&TMP$497$4 );
			}
			goto label$888;
			label$889:;
			{
				FBSTRING TMP$498$4;
				FBSTRING TMP$499$4;
				FBSTRING TMP$500$4;
				__builtin_memset( &TMP$500$4, 0, 24ll );
				__builtin_memset( &TMP$498$4, 0, 24ll );
				FBSTRING* vr$25 = fb_StrConcat( &TMP$498$4, (void*)"mov ", 5ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$499$4, 0, 24ll );
				FBSTRING* vr$28 = fb_StrConcat( &TMP$499$4, (void*)vr$25, -1ll, (void*)", 0", 4ll );
				fb_StrAssign( (void*)&TMP$500$4, -1ll, (void*)vr$28, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$500$4 );
				fb_StrDelete( (FBSTRING*)&TMP$500$4 );
			}
			label$888:;
			if( *(int64*)AV$1 != 4ll) goto label$891;
			{
				FBSTRING TMP$501$4;
				FBSTRING TMP$502$4;
				FBSTRING TMP$503$4;
				FBSTRING TMP$504$4;
				__builtin_memset( &TMP$504$4, 0, 24ll );
				__builtin_memset( &TMP$501$4, 0, 24ll );
				FBSTRING* vr$35 = fb_StrConcat( &TMP$501$4, (void*)"xor ", 5ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$502$4, 0, 24ll );
				FBSTRING* vr$38 = fb_StrConcat( &TMP$502$4, (void*)vr$35, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$503$4, 0, 24ll );
				FBSTRING* vr$41 = fb_StrConcat( &TMP$503$4, (void*)vr$38, -1ll, (void*)&A$1, -1ll );
				fb_StrAssign( (void*)&TMP$504$4, -1ll, (void*)vr$41, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$504$4 );
				fb_StrDelete( (FBSTRING*)&TMP$504$4 );
			}
			goto label$890;
			label$891:;
			{
				FBSTRING TMP$505$4;
				FBSTRING TMP$506$4;
				FBSTRING TMP$507$4;
				__builtin_memset( &TMP$507$4, 0, 24ll );
				__builtin_memset( &TMP$505$4, 0, 24ll );
				FBSTRING* vr$47 = fb_StrConcat( &TMP$505$4, (void*)"mov ", 5ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$506$4, 0, 24ll );
				FBSTRING* vr$50 = fb_StrConcat( &TMP$506$4, (void*)vr$47, -1ll, (void*)", 0", 4ll );
				fb_StrAssign( (void*)&TMP$507$4, -1ll, (void*)vr$50, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$507$4 );
				fb_StrDelete( (FBSTRING*)&TMP$507$4 );
			}
			label$890:;
		}
		goto label$886;
		label$887:;
		if( *(int64*)((uint8*)SVREG$1 + 48ll) < 32ll) goto label$892;
		{
			int64 TMP$520$3;
			TMPISFREE$1 = -1ll;
			if( ((int64)-(*(int64*)BV$1 == 4ll) | (int64)-(*(int64*)AV$1 == 4ll)) == 0ll) goto label$894;
			{
				FBSTRING TMP$508$4;
				FBSTRING TMP$509$4;
				FBSTRING TMP$510$4;
				FBSTRING TMP$511$4;
				__builtin_memset( &TMP$511$4, 0, 24ll );
				__builtin_memset( &TMP$508$4, 0, 24ll );
				FBSTRING* vr$62 = fb_StrConcat( &TMP$508$4, (void*)"mov ", 5ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$509$4, 0, 24ll );
				FBSTRING* vr$65 = fb_StrConcat( &TMP$509$4, (void*)vr$62, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$510$4, 0, 24ll );
				FBSTRING* vr$68 = fb_StrConcat( &TMP$510$4, (void*)vr$65, -1ll, (void*)&B$1, -1ll );
				fb_StrAssign( (void*)&TMP$511$4, -1ll, (void*)vr$68, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$511$4 );
				fb_StrDelete( (FBSTRING*)&TMP$511$4 );
			}
			goto label$893;
			label$894:;
			{
				FBSTRING TMP$512$4;
				FBSTRING TMP$513$4;
				FBSTRING TMP$514$4;
				FBSTRING TMP$515$4;
				FBSTRING TMP$516$4;
				FBSTRING TMP$517$4;
				FBSTRING TMP$518$4;
				FBSTRING TMP$519$4;
				int64 vr$71 = HFINDFREEREG( 0ll );
				TMPREG$1 = vr$71;
				if( TMPREG$1 != -1ll) goto label$896;
				{
					int64 vr$72 = HFINDREGNOTINVREG( DVREG$1, 0ll );
					TMPREG$1 = vr$72;
					TMPISFREE$1 = 0ll;
				}
				label$896:;
				label$895:;
				char* vr$73 = HGETREGNAME( 8ll, TMPREG$1 );
				fb_StrAssign( (void*)&TMPREGNAME$1, -1ll, (void*)vr$73, 0ll, 0 );
				if( TMPISFREE$1 != 0ll) goto label$898;
				{
					HPUSH( (char*)*(char**)&TMPREGNAME$1 );
				}
				label$898:;
				label$897:;
				__builtin_memset( &TMP$515$4, 0, 24ll );
				__builtin_memset( &TMP$512$4, 0, 24ll );
				FBSTRING* vr$77 = fb_StrConcat( &TMP$512$4, (void*)"mov ", 5ll, (void*)&TMPREGNAME$1, -1ll );
				__builtin_memset( &TMP$513$4, 0, 24ll );
				FBSTRING* vr$80 = fb_StrConcat( &TMP$513$4, (void*)vr$77, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$514$4, 0, 24ll );
				FBSTRING* vr$83 = fb_StrConcat( &TMP$514$4, (void*)vr$80, -1ll, (void*)&B$1, -1ll );
				fb_StrAssign( (void*)&TMP$515$4, -1ll, (void*)vr$83, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$515$4 );
				fb_StrDelete( (FBSTRING*)&TMP$515$4 );
				__builtin_memset( &TMP$519$4, 0, 24ll );
				__builtin_memset( &TMP$516$4, 0, 24ll );
				FBSTRING* vr$89 = fb_StrConcat( &TMP$516$4, (void*)"mov ", 5ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$517$4, 0, 24ll );
				FBSTRING* vr$92 = fb_StrConcat( &TMP$517$4, (void*)vr$89, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$518$4, 0, 24ll );
				FBSTRING* vr$95 = fb_StrConcat( &TMP$518$4, (void*)vr$92, -1ll, (void*)&TMPREGNAME$1, -1ll );
				fb_StrAssign( (void*)&TMP$519$4, -1ll, (void*)vr$95, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$519$4 );
				fb_StrDelete( (FBSTRING*)&TMP$519$4 );
			}
			label$893:;
			if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$899;
			TMP$520$3 = 24ll;
			goto label$1923;
			label$899:;
			TMP$520$3 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
			label$1923:;
			if( ((int64)-(OP$1 == 42ll) & *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$520$3 * 56ll)) + 16ll)) == 0ll) goto label$901;
			{
				FBSTRING TMP$521$4;
				FBSTRING TMP$522$4;
				FBSTRING TMP$523$4;
				__builtin_memset( &TMP$523$4, 0, 24ll );
				__builtin_memset( &TMP$521$4, 0, 24ll );
				FBSTRING* vr$108 = fb_StrConcat( &TMP$521$4, (void*)"sar ", 5ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$522$4, 0, 24ll );
				FBSTRING* vr$111 = fb_StrConcat( &TMP$522$4, (void*)vr$108, -1ll, (void*)", 31", 5ll );
				fb_StrAssign( (void*)&TMP$523$4, -1ll, (void*)vr$111, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$523$4 );
				fb_StrDelete( (FBSTRING*)&TMP$523$4 );
			}
			goto label$900;
			label$901:;
			if( *(int64*)BV$1 != 4ll) goto label$902;
			{
				FBSTRING TMP$524$4;
				FBSTRING TMP$525$4;
				FBSTRING TMP$526$4;
				FBSTRING TMP$527$4;
				__builtin_memset( &TMP$527$4, 0, 24ll );
				__builtin_memset( &TMP$524$4, 0, 24ll );
				FBSTRING* vr$118 = fb_StrConcat( &TMP$524$4, (void*)"xor ", 5ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$525$4, 0, 24ll );
				FBSTRING* vr$121 = fb_StrConcat( &TMP$525$4, (void*)vr$118, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$526$4, 0, 24ll );
				FBSTRING* vr$124 = fb_StrConcat( &TMP$526$4, (void*)vr$121, -1ll, (void*)&B$1, -1ll );
				fb_StrAssign( (void*)&TMP$527$4, -1ll, (void*)vr$124, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$527$4 );
				fb_StrDelete( (FBSTRING*)&TMP$527$4 );
			}
			goto label$900;
			label$902:;
			{
				FBSTRING TMP$528$4;
				FBSTRING TMP$529$4;
				FBSTRING TMP$530$4;
				__builtin_memset( &TMP$530$4, 0, 24ll );
				__builtin_memset( &TMP$528$4, 0, 24ll );
				FBSTRING* vr$130 = fb_StrConcat( &TMP$528$4, (void*)"mov ", 5ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$529$4, 0, 24ll );
				FBSTRING* vr$133 = fb_StrConcat( &TMP$529$4, (void*)vr$130, -1ll, (void*)", 0", 4ll );
				fb_StrAssign( (void*)&TMP$530$4, -1ll, (void*)vr$133, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$530$4 );
				fb_StrDelete( (FBSTRING*)&TMP$530$4 );
			}
			label$900:;
			if( *(int64*)((uint8*)SVREG$1 + 48ll) <= 32ll) goto label$904;
			{
				FBSTRING TMP$531$4;
				FBSTRING TMP$532$4;
				FBSTRING TMP$533$4;
				FBSTRING TMP$534$4;
				FBSTRING* vr$139 = fb_LongintToStr( *(int64*)((uint8*)SVREG$1 + 48ll) + -32ll );
				fb_StrAssign( (void*)&SRC$1, -1ll, (void*)vr$139, -1ll, 0 );
				__builtin_memset( &TMP$534$4, 0, 24ll );
				__builtin_memset( &TMP$531$4, 0, 24ll );
				FBSTRING* vr$143 = fb_StrConcat( &TMP$531$4, (void*)&MNEMONIC32$1, -1ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$532$4, 0, 24ll );
				FBSTRING* vr$146 = fb_StrConcat( &TMP$532$4, (void*)vr$143, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$533$4, 0, 24ll );
				FBSTRING* vr$149 = fb_StrConcat( &TMP$533$4, (void*)vr$146, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$534$4, -1ll, (void*)vr$149, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$534$4 );
				fb_StrDelete( (FBSTRING*)&TMP$534$4 );
			}
			label$904:;
			label$903:;
			if( TMPISFREE$1 != 0ll) goto label$906;
			{
				HPOP( (char*)*(char**)&TMPREGNAME$1 );
			}
			label$906:;
			label$905:;
		}
		goto label$886;
		label$892:;
		{
			if( *(int64*)BV$1 != 4ll) goto label$908;
			{
				FBSTRING TMP$535$4;
				FBSTRING TMP$536$4;
				FBSTRING TMP$537$4;
				FBSTRING TMP$538$4;
				FBSTRING TMP$539$4;
				FBSTRING TMP$540$4;
				FBSTRING TMP$541$4;
				FBSTRING TMP$542$4;
				FBSTRING TMP$543$4;
				FBSTRING TMP$544$4;
				__builtin_memset( &TMP$540$4, 0, 24ll );
				__builtin_memset( &TMP$535$4, 0, 24ll );
				FBSTRING* vr$156 = fb_StrConcat( &TMP$535$4, (void*)&MNEMONIC64$1, -1ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$536$4, 0, 24ll );
				FBSTRING* vr$159 = fb_StrConcat( &TMP$536$4, (void*)vr$156, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$537$4, 0, 24ll );
				FBSTRING* vr$162 = fb_StrConcat( &TMP$537$4, (void*)vr$159, -1ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$538$4, 0, 24ll );
				FBSTRING* vr$165 = fb_StrConcat( &TMP$538$4, (void*)vr$162, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$539$4, 0, 24ll );
				FBSTRING* vr$168 = fb_StrConcat( &TMP$539$4, (void*)vr$165, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$540$4, -1ll, (void*)vr$168, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$540$4 );
				fb_StrDelete( (FBSTRING*)&TMP$540$4 );
				__builtin_memset( &TMP$544$4, 0, 24ll );
				__builtin_memset( &TMP$541$4, 0, 24ll );
				FBSTRING* vr$174 = fb_StrConcat( &TMP$541$4, (void*)&MNEMONIC32$1, -1ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$542$4, 0, 24ll );
				FBSTRING* vr$177 = fb_StrConcat( &TMP$542$4, (void*)vr$174, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$543$4, 0, 24ll );
				FBSTRING* vr$180 = fb_StrConcat( &TMP$543$4, (void*)vr$177, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$544$4, -1ll, (void*)vr$180, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$544$4 );
				fb_StrDelete( (FBSTRING*)&TMP$544$4 );
			}
			goto label$907;
			label$908:;
			if( *(int64*)AV$1 != 4ll) goto label$909;
			{
				FBSTRING TMP$545$4;
				FBSTRING TMP$546$4;
				FBSTRING TMP$547$4;
				FBSTRING TMP$548$4;
				FBSTRING TMP$549$4;
				FBSTRING TMP$550$4;
				FBSTRING TMP$551$4;
				FBSTRING TMP$552$4;
				FBSTRING TMP$553$4;
				FBSTRING TMP$554$4;
				FBSTRING TMP$555$4;
				FBSTRING TMP$556$4;
				FBSTRING TMP$557$4;
				FBSTRING TMP$558$4;
				FBSTRING TMP$559$4;
				FBSTRING TMP$560$4;
				FBSTRING TMP$561$4;
				FBSTRING TMP$562$4;
				__builtin_memset( &TMP$548$4, 0, 24ll );
				__builtin_memset( &TMP$545$4, 0, 24ll );
				FBSTRING* vr$187 = fb_StrConcat( &TMP$545$4, (void*)"xchg ", 6ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$546$4, 0, 24ll );
				FBSTRING* vr$190 = fb_StrConcat( &TMP$546$4, (void*)vr$187, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$547$4, 0, 24ll );
				FBSTRING* vr$193 = fb_StrConcat( &TMP$547$4, (void*)vr$190, -1ll, (void*)&B$1, -1ll );
				fb_StrAssign( (void*)&TMP$548$4, -1ll, (void*)vr$193, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$548$4 );
				fb_StrDelete( (FBSTRING*)&TMP$548$4 );
				__builtin_memset( &TMP$554$4, 0, 24ll );
				__builtin_memset( &TMP$549$4, 0, 24ll );
				FBSTRING* vr$199 = fb_StrConcat( &TMP$549$4, (void*)&MNEMONIC64$1, -1ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$550$4, 0, 24ll );
				FBSTRING* vr$202 = fb_StrConcat( &TMP$550$4, (void*)vr$199, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$551$4, 0, 24ll );
				FBSTRING* vr$205 = fb_StrConcat( &TMP$551$4, (void*)vr$202, -1ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$552$4, 0, 24ll );
				FBSTRING* vr$208 = fb_StrConcat( &TMP$552$4, (void*)vr$205, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$553$4, 0, 24ll );
				FBSTRING* vr$211 = fb_StrConcat( &TMP$553$4, (void*)vr$208, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$554$4, -1ll, (void*)vr$211, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$554$4 );
				fb_StrDelete( (FBSTRING*)&TMP$554$4 );
				__builtin_memset( &TMP$558$4, 0, 24ll );
				__builtin_memset( &TMP$555$4, 0, 24ll );
				FBSTRING* vr$217 = fb_StrConcat( &TMP$555$4, (void*)&MNEMONIC32$1, -1ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$556$4, 0, 24ll );
				FBSTRING* vr$220 = fb_StrConcat( &TMP$556$4, (void*)vr$217, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$557$4, 0, 24ll );
				FBSTRING* vr$223 = fb_StrConcat( &TMP$557$4, (void*)vr$220, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$558$4, -1ll, (void*)vr$223, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$558$4 );
				fb_StrDelete( (FBSTRING*)&TMP$558$4 );
				__builtin_memset( &TMP$562$4, 0, 24ll );
				__builtin_memset( &TMP$559$4, 0, 24ll );
				FBSTRING* vr$229 = fb_StrConcat( &TMP$559$4, (void*)"xchg ", 6ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$560$4, 0, 24ll );
				FBSTRING* vr$232 = fb_StrConcat( &TMP$560$4, (void*)vr$229, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$561$4, 0, 24ll );
				FBSTRING* vr$235 = fb_StrConcat( &TMP$561$4, (void*)vr$232, -1ll, (void*)&B$1, -1ll );
				fb_StrAssign( (void*)&TMP$562$4, -1ll, (void*)vr$235, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$562$4 );
				fb_StrDelete( (FBSTRING*)&TMP$562$4 );
			}
			goto label$907;
			label$909:;
			{
				FBSTRING TMP$563$4;
				FBSTRING TMP$564$4;
				FBSTRING TMP$565$4;
				FBSTRING TMP$566$4;
				FBSTRING TMP$567$4;
				FBSTRING TMP$568$4;
				FBSTRING TMP$569$4;
				FBSTRING TMP$570$4;
				FBSTRING TMP$571$4;
				FBSTRING TMP$572$4;
				FBSTRING TMP$573$4;
				FBSTRING TMP$574$4;
				FBSTRING TMP$575$4;
				FBSTRING TMP$576$4;
				FBSTRING TMP$577$4;
				FBSTRING TMP$578$4;
				FBSTRING TMP$579$4;
				FBSTRING TMP$580$4;
				int64 vr$238 = HFINDFREEREG( 0ll );
				TMPREG$1 = vr$238;
				if( TMPREG$1 != -1ll) goto label$911;
				{
					int64 vr$239 = HFINDREGNOTINVREG( DVREG$1, 0ll );
					TMPREG$1 = vr$239;
					TMPISFREE$1 = 0ll;
				}
				goto label$910;
				label$911:;
				{
					TMPISFREE$1 = -1ll;
				}
				label$910:;
				char* vr$240 = HGETREGNAME( 8ll, TMPREG$1 );
				fb_StrAssign( (void*)&TMPREGNAME$1, -1ll, (void*)vr$240, 0ll, 0 );
				if( TMPISFREE$1 != 0ll) goto label$913;
				{
					HPUSH( (char*)*(char**)&TMPREGNAME$1 );
				}
				label$913:;
				label$912:;
				__builtin_memset( &TMP$566$4, 0, 24ll );
				__builtin_memset( &TMP$563$4, 0, 24ll );
				FBSTRING* vr$244 = fb_StrConcat( &TMP$563$4, (void*)"mov ", 5ll, (void*)&TMPREGNAME$1, -1ll );
				__builtin_memset( &TMP$564$4, 0, 24ll );
				FBSTRING* vr$247 = fb_StrConcat( &TMP$564$4, (void*)vr$244, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$565$4, 0, 24ll );
				FBSTRING* vr$250 = fb_StrConcat( &TMP$565$4, (void*)vr$247, -1ll, (void*)&B$1, -1ll );
				fb_StrAssign( (void*)&TMP$566$4, -1ll, (void*)vr$250, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$566$4 );
				fb_StrDelete( (FBSTRING*)&TMP$566$4 );
				__builtin_memset( &TMP$572$4, 0, 24ll );
				__builtin_memset( &TMP$567$4, 0, 24ll );
				FBSTRING* vr$256 = fb_StrConcat( &TMP$567$4, (void*)&MNEMONIC64$1, -1ll, (void*)&A$1, -1ll );
				__builtin_memset( &TMP$568$4, 0, 24ll );
				FBSTRING* vr$259 = fb_StrConcat( &TMP$568$4, (void*)vr$256, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$569$4, 0, 24ll );
				FBSTRING* vr$262 = fb_StrConcat( &TMP$569$4, (void*)vr$259, -1ll, (void*)&TMPREGNAME$1, -1ll );
				__builtin_memset( &TMP$570$4, 0, 24ll );
				FBSTRING* vr$265 = fb_StrConcat( &TMP$570$4, (void*)vr$262, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$571$4, 0, 24ll );
				FBSTRING* vr$268 = fb_StrConcat( &TMP$571$4, (void*)vr$265, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$572$4, -1ll, (void*)vr$268, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$572$4 );
				fb_StrDelete( (FBSTRING*)&TMP$572$4 );
				__builtin_memset( &TMP$576$4, 0, 24ll );
				__builtin_memset( &TMP$573$4, 0, 24ll );
				FBSTRING* vr$274 = fb_StrConcat( &TMP$573$4, (void*)&MNEMONIC32$1, -1ll, (void*)&TMPREGNAME$1, -1ll );
				__builtin_memset( &TMP$574$4, 0, 24ll );
				FBSTRING* vr$277 = fb_StrConcat( &TMP$574$4, (void*)vr$274, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$575$4, 0, 24ll );
				FBSTRING* vr$280 = fb_StrConcat( &TMP$575$4, (void*)vr$277, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$576$4, -1ll, (void*)vr$280, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$576$4 );
				fb_StrDelete( (FBSTRING*)&TMP$576$4 );
				__builtin_memset( &TMP$580$4, 0, 24ll );
				__builtin_memset( &TMP$577$4, 0, 24ll );
				FBSTRING* vr$286 = fb_StrConcat( &TMP$577$4, (void*)"mov ", 5ll, (void*)&B$1, -1ll );
				__builtin_memset( &TMP$578$4, 0, 24ll );
				FBSTRING* vr$289 = fb_StrConcat( &TMP$578$4, (void*)vr$286, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$579$4, 0, 24ll );
				FBSTRING* vr$292 = fb_StrConcat( &TMP$579$4, (void*)vr$289, -1ll, (void*)&TMPREGNAME$1, -1ll );
				fb_StrAssign( (void*)&TMP$580$4, -1ll, (void*)vr$292, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$580$4 );
				fb_StrDelete( (FBSTRING*)&TMP$580$4 );
				if( TMPISFREE$1 != 0ll) goto label$915;
				{
					HPOP( (char*)"eax" );
				}
				label$915:;
				label$914:;
			}
			label$907:;
		}
		label$886:;
	}
	goto label$884;
	label$885:;
	{
		static int64 ISEAXFREE$2;
		static int64 ISEDXFREE$2;
		static int64 ISECXFREE$2;
		static int64 EAXINDEST$2;
		static int64 EDXINDEST$2;
		static int64 ECXINDEST$2;
		static int64 OFS$2;
		char* vr$295 = SYMBUNIQUELABEL(  );
		fb_StrAssign( (void*)&LABEL$1, -1ll, (void*)vr$295, 0ll, 0 );
		HPUSH( (char*)*(char**)&DST2$1 );
		HPUSH( (char*)*(char**)&DST1$1 );
		OFS$2 = 0ll;
		int64 vr$296 = HISREGFREE( 0ll, 5ll );
		ISEAXFREE$2 = vr$296;
		int64 vr$297 = HISREGFREE( 0ll, 0ll );
		ISEDXFREE$2 = vr$297;
		int64 vr$298 = HISREGFREE( 0ll, 3ll );
		ISECXFREE$2 = vr$298;
		int64 vr$299 = HISREGINVREG( DVREG$1, 5ll );
		EAXINDEST$2 = vr$299;
		int64 vr$300 = HISREGINVREG( DVREG$1, 0ll );
		EDXINDEST$2 = vr$300;
		int64 vr$301 = HISREGINVREG( DVREG$1, 3ll );
		ECXINDEST$2 = vr$301;
		if( ((int64)-(*(int64*)SVREG$1 != 4ll) | (int64)-(*(int64*)((uint8*)SVREG$1 + 24ll) != 3ll)) == 0ll) goto label$917;
		{
			int64 TMP$581$3;
			if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$918;
			TMP$581$3 = 24ll;
			goto label$1924;
			label$918:;
			TMP$581$3 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
			label$1924:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$581$3 * 56ll)) + 8ll) == 4ll) goto label$920;
			{
				if( *(int64*)SVREG$1 != 4ll) goto label$922;
				{
					char* vr$314 = HGETREGNAME( 8ll, *(int64*)((uint8*)SVREG$1 + 24ll) );
					fb_StrAssign( (void*)&SRC$1, -1ll, (void*)vr$314, 0ll, 0 );
				}
				label$922:;
				label$921:;
			}
			label$920:;
			label$919:;
			if( ISECXFREE$2 != 0ll) goto label$924;
			{
				if( (ECXINDEST$2 & (int64)-(*(int64*)DVREG$1 == 4ll)) == 0ll) goto label$926;
				{
					HMOV( (char*)"ecx", (char*)*(char**)&SRC$1 );
					ISECXFREE$2 = -1ll;
				}
				goto label$925;
				label$926:;
				{
					HPUSH( (char*)*(char**)&SRC$1 );
					OUTP( (char*)"xchg ecx, [esp]" );
					OFS$2 = OFS$2 + 4ll;
				}
				label$925:;
			}
			goto label$923;
			label$924:;
			{
				HMOV( (char*)"ecx", (char*)*(char**)&SRC$1 );
			}
			label$923:;
		}
		goto label$916;
		label$917:;
		{
			ISECXFREE$2 = -1ll;
		}
		label$916:;
		if( EAXINDEST$2 == 0ll) goto label$928;
		{
			if( *(int64*)DVREG$1 == 4ll) goto label$930;
			{
				FBSTRING TMP$582$4;
				FBSTRING TMP$583$4;
				FBSTRING TMP$584$4;
				__builtin_memset( &TMP$584$4, 0, 24ll );
				FBSTRING* vr$321 = fb_LongintToStr( OFS$2 );
				__builtin_memset( &TMP$582$4, 0, 24ll );
				FBSTRING* vr$324 = fb_StrConcat( &TMP$582$4, (void*)"xchg eax, [esp+", 16ll, (void*)vr$321, -1ll );
				__builtin_memset( &TMP$583$4, 0, 24ll );
				FBSTRING* vr$327 = fb_StrConcat( &TMP$583$4, (void*)vr$324, -1ll, (void*)"]", 2ll );
				fb_StrAssign( (void*)&TMP$584$4, -1ll, (void*)vr$327, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$584$4 );
				fb_StrDelete( (FBSTRING*)&TMP$584$4 );
			}
			goto label$929;
			label$930:;
			{
				FBSTRING TMP$585$4;
				FBSTRING TMP$586$4;
				FBSTRING TMP$587$4;
				__builtin_memset( &TMP$587$4, 0, 24ll );
				FBSTRING* vr$331 = fb_LongintToStr( OFS$2 );
				__builtin_memset( &TMP$585$4, 0, 24ll );
				FBSTRING* vr$334 = fb_StrConcat( &TMP$585$4, (void*)"mov eax, [esp+", 15ll, (void*)vr$331, -1ll );
				__builtin_memset( &TMP$586$4, 0, 24ll );
				FBSTRING* vr$337 = fb_StrConcat( &TMP$586$4, (void*)vr$334, -1ll, (void*)"]", 2ll );
				fb_StrAssign( (void*)&TMP$587$4, -1ll, (void*)vr$337, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$587$4 );
				fb_StrDelete( (FBSTRING*)&TMP$587$4 );
			}
			label$929:;
		}
		goto label$927;
		label$928:;
		{
			if( ISEAXFREE$2 != 0ll) goto label$932;
			{
				FBSTRING TMP$588$4;
				FBSTRING TMP$589$4;
				FBSTRING TMP$590$4;
				__builtin_memset( &TMP$590$4, 0, 24ll );
				FBSTRING* vr$341 = fb_LongintToStr( OFS$2 );
				__builtin_memset( &TMP$588$4, 0, 24ll );
				FBSTRING* vr$344 = fb_StrConcat( &TMP$588$4, (void*)"xchg eax, [esp+", 16ll, (void*)vr$341, -1ll );
				__builtin_memset( &TMP$589$4, 0, 24ll );
				FBSTRING* vr$347 = fb_StrConcat( &TMP$589$4, (void*)vr$344, -1ll, (void*)"]", 2ll );
				fb_StrAssign( (void*)&TMP$590$4, -1ll, (void*)vr$347, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$590$4 );
				fb_StrDelete( (FBSTRING*)&TMP$590$4 );
			}
			goto label$931;
			label$932:;
			{
				FBSTRING TMP$591$4;
				FBSTRING TMP$592$4;
				FBSTRING TMP$593$4;
				__builtin_memset( &TMP$593$4, 0, 24ll );
				FBSTRING* vr$351 = fb_LongintToStr( OFS$2 );
				__builtin_memset( &TMP$591$4, 0, 24ll );
				FBSTRING* vr$354 = fb_StrConcat( &TMP$591$4, (void*)"mov eax, [esp+", 15ll, (void*)vr$351, -1ll );
				__builtin_memset( &TMP$592$4, 0, 24ll );
				FBSTRING* vr$357 = fb_StrConcat( &TMP$592$4, (void*)vr$354, -1ll, (void*)"]", 2ll );
				fb_StrAssign( (void*)&TMP$593$4, -1ll, (void*)vr$357, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$593$4 );
				fb_StrDelete( (FBSTRING*)&TMP$593$4 );
			}
			label$931:;
		}
		label$927:;
		if( EDXINDEST$2 == 0ll) goto label$934;
		{
			if( *(int64*)DVREG$1 == 4ll) goto label$936;
			{
				FBSTRING TMP$595$4;
				FBSTRING TMP$596$4;
				FBSTRING TMP$597$4;
				__builtin_memset( &TMP$597$4, 0, 24ll );
				FBSTRING* vr$363 = fb_LongintToStr( OFS$2 + 4ll );
				__builtin_memset( &TMP$595$4, 0, 24ll );
				FBSTRING* vr$366 = fb_StrConcat( &TMP$595$4, (void*)"xchg edx, [esp+", 16ll, (void*)vr$363, -1ll );
				__builtin_memset( &TMP$596$4, 0, 24ll );
				FBSTRING* vr$369 = fb_StrConcat( &TMP$596$4, (void*)vr$366, -1ll, (void*)"]", 2ll );
				fb_StrAssign( (void*)&TMP$597$4, -1ll, (void*)vr$369, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$597$4 );
				fb_StrDelete( (FBSTRING*)&TMP$597$4 );
			}
			goto label$935;
			label$936:;
			{
				FBSTRING TMP$598$4;
				FBSTRING TMP$599$4;
				FBSTRING TMP$600$4;
				__builtin_memset( &TMP$600$4, 0, 24ll );
				FBSTRING* vr$374 = fb_LongintToStr( OFS$2 + 4ll );
				__builtin_memset( &TMP$598$4, 0, 24ll );
				FBSTRING* vr$377 = fb_StrConcat( &TMP$598$4, (void*)"mov edx, [esp+", 15ll, (void*)vr$374, -1ll );
				__builtin_memset( &TMP$599$4, 0, 24ll );
				FBSTRING* vr$380 = fb_StrConcat( &TMP$599$4, (void*)vr$377, -1ll, (void*)"]", 2ll );
				fb_StrAssign( (void*)&TMP$600$4, -1ll, (void*)vr$380, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$600$4 );
				fb_StrDelete( (FBSTRING*)&TMP$600$4 );
			}
			label$935:;
		}
		goto label$933;
		label$934:;
		{
			if( ISEDXFREE$2 != 0ll) goto label$938;
			{
				FBSTRING TMP$601$4;
				FBSTRING TMP$602$4;
				FBSTRING TMP$603$4;
				__builtin_memset( &TMP$603$4, 0, 24ll );
				FBSTRING* vr$385 = fb_LongintToStr( OFS$2 + 4ll );
				__builtin_memset( &TMP$601$4, 0, 24ll );
				FBSTRING* vr$388 = fb_StrConcat( &TMP$601$4, (void*)"xchg edx, [esp+", 16ll, (void*)vr$385, -1ll );
				__builtin_memset( &TMP$602$4, 0, 24ll );
				FBSTRING* vr$391 = fb_StrConcat( &TMP$602$4, (void*)vr$388, -1ll, (void*)"]", 2ll );
				fb_StrAssign( (void*)&TMP$603$4, -1ll, (void*)vr$391, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$603$4 );
				fb_StrDelete( (FBSTRING*)&TMP$603$4 );
			}
			goto label$937;
			label$938:;
			{
				FBSTRING TMP$604$4;
				FBSTRING TMP$605$4;
				FBSTRING TMP$606$4;
				__builtin_memset( &TMP$606$4, 0, 24ll );
				FBSTRING* vr$396 = fb_LongintToStr( OFS$2 + 4ll );
				__builtin_memset( &TMP$604$4, 0, 24ll );
				FBSTRING* vr$399 = fb_StrConcat( &TMP$604$4, (void*)"mov edx, [esp+", 15ll, (void*)vr$396, -1ll );
				__builtin_memset( &TMP$605$4, 0, 24ll );
				FBSTRING* vr$402 = fb_StrConcat( &TMP$605$4, (void*)vr$399, -1ll, (void*)"]", 2ll );
				fb_StrAssign( (void*)&TMP$606$4, -1ll, (void*)vr$402, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$606$4 );
				fb_StrDelete( (FBSTRING*)&TMP$606$4 );
			}
			label$937:;
		}
		label$933:;
		if( OP$1 != 41ll) goto label$940;
		{
			FBSTRING TMP$609$3;
			FBSTRING TMP$610$3;
			OUTP( (char*)"shld edx, eax, cl" );
			__builtin_memset( &TMP$610$3, 0, 24ll );
			__builtin_memset( &TMP$609$3, 0, 24ll );
			FBSTRING* vr$408 = fb_StrConcat( &TMP$609$3, (void*)&MNEMONIC32$1, -1ll, (void*)" eax, cl", 9ll );
			fb_StrAssign( (void*)&TMP$610$3, -1ll, (void*)vr$408, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$610$3 );
			fb_StrDelete( (FBSTRING*)&TMP$610$3 );
		}
		goto label$939;
		label$940:;
		{
			FBSTRING TMP$613$3;
			FBSTRING TMP$614$3;
			OUTP( (char*)"shrd eax, edx, cl" );
			__builtin_memset( &TMP$614$3, 0, 24ll );
			__builtin_memset( &TMP$613$3, 0, 24ll );
			FBSTRING* vr$414 = fb_StrConcat( &TMP$613$3, (void*)&MNEMONIC32$1, -1ll, (void*)" edx, cl", 9ll );
			fb_StrAssign( (void*)&TMP$614$3, -1ll, (void*)vr$414, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$614$3 );
			fb_StrDelete( (FBSTRING*)&TMP$614$3 );
		}
		label$939:;
		OUTP( (char*)"test cl, 32" );
		HBRANCH( (char*)"jz", (char*)*(char**)&LABEL$1 );
		if( OP$1 != 41ll) goto label$942;
		{
			OUTP( (char*)"mov edx, eax" );
			OUTP( (char*)"xor eax, eax" );
		}
		goto label$941;
		label$942:;
		{
			int64 TMP$618$3;
			OUTP( (char*)"mov eax, edx" );
			if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$943;
			TMP$618$3 = 24ll;
			goto label$1925;
			label$943:;
			TMP$618$3 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
			label$1925:;
			if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$618$3 * 56ll)) + 16ll) == 0ll) goto label$945;
			{
				OUTP( (char*)"sar edx, 31" );
			}
			goto label$944;
			label$945:;
			{
				OUTP( (char*)"xor edx, edx" );
			}
			label$944:;
		}
		label$941:;
		HLABEL( (char*)*(char**)&LABEL$1 );
		if( ISECXFREE$2 != 0ll) goto label$947;
		{
			HPOP( (char*)"ecx" );
		}
		label$947:;
		label$946:;
		if( EDXINDEST$2 == 0ll) goto label$949;
		{
			if( *(int64*)DVREG$1 == 4ll) goto label$951;
			{
				OUTP( (char*)"xchg edx, [esp+4]" );
			}
			goto label$950;
			label$951:;
			{
				OUTP( (char*)"mov [esp+4], edx" );
			}
			label$950:;
		}
		goto label$948;
		label$949:;
		{
			if( ISEDXFREE$2 != 0ll) goto label$953;
			{
				OUTP( (char*)"xchg edx, [esp+4]" );
			}
			goto label$952;
			label$953:;
			{
				OUTP( (char*)"mov [esp+4], edx" );
			}
			label$952:;
		}
		label$948:;
		if( EAXINDEST$2 == 0ll) goto label$955;
		{
			if( *(int64*)DVREG$1 == 4ll) goto label$957;
			{
				OUTP( (char*)"xchg eax, [esp+0]" );
			}
			goto label$956;
			label$957:;
			{
				OUTP( (char*)"mov [esp+0], eax" );
			}
			label$956:;
		}
		goto label$954;
		label$955:;
		{
			if( ISEAXFREE$2 != 0ll) goto label$959;
			{
				OUTP( (char*)"xchg eax, [esp+0]" );
			}
			goto label$958;
			label$959:;
			{
				OUTP( (char*)"mov [esp+0], eax" );
			}
			label$958:;
		}
		label$954:;
		HPOP( (char*)*(char**)&DST1$1 );
		HPOP( (char*)*(char**)&DST2$1 );
	}
	label$884:;
	label$876:;
}

static void HSHIFTI( int64 OP$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$625$1;
	label$960:;
	static int64 EAXPRESERVED$1;
	static int64 ECXPRESERVED$1;
	static int64 EAXFREE$1;
	static int64 ECXFREE$1;
	static int64 REG$1;
	static int64 ECXINDEST$1;
	static FBSTRING OSTR$1;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING TMP$1;
	static FBSTRING MNEMONIC$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$962;
	TMP$625$1 = 24ll;
	goto label$1926;
	label$962:;
	TMP$625$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1926:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$625$1 * 56ll)) + 16ll) == 0ll) goto label$964;
	{
		if( OP$1 != 41ll) goto label$966;
		{
			fb_StrAssign( (void*)&MNEMONIC$1, -1ll, (void*)"sal", 4ll, 0 );
		}
		goto label$965;
		label$966:;
		{
			fb_StrAssign( (void*)&MNEMONIC$1, -1ll, (void*)"sar", 4ll, 0 );
		}
		label$965:;
	}
	goto label$963;
	label$964:;
	{
		if( OP$1 != 41ll) goto label$968;
		{
			fb_StrAssign( (void*)&MNEMONIC$1, -1ll, (void*)"shl", 4ll, 0 );
		}
		goto label$967;
		label$968:;
		{
			fb_StrAssign( (void*)&MNEMONIC$1, -1ll, (void*)"shr", 4ll, 0 );
		}
		label$967:;
	}
	label$963:;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	ECXINDEST$1 = 0ll;
	EAXPRESERVED$1 = 0ll;
	ECXPRESERVED$1 = 0ll;
	if( *(int64*)SVREG$1 != 0ll) goto label$970;
	{
		HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
		fb_StrAssign( (void*)&TMP$1, -1ll, (void*)&DST$1, -1ll, 0 );
	}
	goto label$969;
	label$970:;
	{
		int64 vr$6 = HISREGFREE( 0ll, 5ll );
		EAXFREE$1 = vr$6;
		int64 vr$7 = HISREGFREE( 0ll, 3ll );
		ECXFREE$1 = vr$7;
		if( *(int64*)SVREG$1 != 4ll) goto label$972;
		{
			REG$1 = *(int64*)((uint8*)SVREG$1 + 24ll);
		}
		goto label$971;
		label$972:;
		{
			REG$1 = -1ll;
		}
		label$971:;
		int64 vr$10 = HISREGINVREG( DVREG$1, 3ll );
		ECXINDEST$1 = vr$10;
		if( ECXINDEST$1 == 0ll) goto label$974;
		{
			HPUSH( (char*)"ecx" );
			if( *(int64*)DVREG$1 == 4ll) goto label$976;
			{
				HPREPOPERAND( DVREG$1, &OSTR$1, 8ll, 0ll, 0ll, -1ll );
				HPUSH( (char*)*(char**)&OSTR$1 );
			}
			label$976:;
			label$975:;
		}
		goto label$973;
		label$974:;
		if( ((int64)-(REG$1 != 3ll) & (int64)-(ECXFREE$1 == 0ll)) == 0ll) goto label$977;
		{
			ECXPRESERVED$1 = -1ll;
			HPUSH( (char*)"ecx" );
		}
		label$977:;
		label$973:;
		if( *(int64*)SVREG$1 == 4ll) goto label$979;
		{
			HPREPOPERAND( SVREG$1, &OSTR$1, 2ll, 0ll, 0ll, -1ll );
			HMOV( (char*)"cl", (char*)*(char**)&OSTR$1 );
		}
		goto label$978;
		label$979:;
		{
			if( REG$1 == 3ll) goto label$981;
			{
				HMOV( (char*)"ecx", (char*)((int64)(char*)RNAMETB$ + (((*(int64*)((int64)(struct $12EMITDATATYPE*)DTYPETB$ + 192ll) << (3ll & 63ll)) + REG$1) << (3ll & 63ll))) );
			}
			label$981:;
			label$980:;
		}
		label$978:;
		if( ECXINDEST$1 == 0ll) goto label$983;
		{
			if( EAXFREE$1 != 0ll) goto label$985;
			{
				EAXPRESERVED$1 = -1ll;
				OUTP( (char*)"xchg eax, [esp]" );
			}
			goto label$984;
			label$985:;
			{
				HPOP( (char*)"eax" );
			}
			label$984:;
			fb_StrAssign( (void*)&TMP$1, -1ll, (void*)(((int64)(char*)RNAMETB$ + (*(int64*)((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) << (6ll & 63ll))) + 40ll), 8ll, 0 );
		}
		goto label$982;
		label$983:;
		{
			fb_StrAssign( (void*)&TMP$1, -1ll, (void*)&DST$1, -1ll, 0 );
		}
		label$982:;
		fb_StrAssign( (void*)&SRC$1, -1ll, (void*)"cl", 3ll, 0 );
	}
	label$969:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)&MNEMONIC$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)" ", 2ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&TMP$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	if( ECXINDEST$1 == 0ll) goto label$987;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$989;
		{
			HPOP( (char*)"ecx" );
			if( EAXPRESERVED$1 == 0ll) goto label$991;
			{
				OUTP( (char*)"xchg ecx, [esp]" );
			}
			label$991:;
			label$990:;
		}
		label$989:;
		label$988:;
		HMOV( (char*)*(char**)&DST$1, (char*)(((int64)(char*)RNAMETB$ + (*(int64*)((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) << (6ll & 63ll))) + 40ll) );
	}
	label$987:;
	label$986:;
	if( EAXPRESERVED$1 == 0ll) goto label$993;
	{
		HPOP( (char*)"eax" );
	}
	label$993:;
	label$992:;
	if( ECXPRESERVED$1 == 0ll) goto label$995;
	{
		HPOP( (char*)"ecx" );
	}
	label$995:;
	label$994:;
	label$961:;
}

static void _EMITSHLL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$996:;
	HSHIFTL( 41ll, DVREG$1, SVREG$1 );
	label$997:;
}

static void _EMITSHLI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$998:;
	HSHIFTI( 41ll, DVREG$1, SVREG$1 );
	label$999:;
}

static void _EMITSHRL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1000:;
	HSHIFTL( 42ll, DVREG$1, SVREG$1 );
	label$1001:;
}

static void _EMITSHRI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1002:;
	HSHIFTI( 42ll, DVREG$1, SVREG$1 );
	label$1003:;
}

static void _EMITANDL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1004:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"and ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"and ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1005:;
}

static void _EMITANDI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1006:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"and ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1007:;
}

static void _EMITORL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1008:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"or ", 4ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"or ", 4ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1009:;
}

static void _EMITORI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1010:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"or ", 4ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1011:;
}

static void _EMITXORL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1012:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1013:;
}

static void _EMITXORI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1014:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1015:;
}

static void _EMITEQVL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1016:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1017:;
}

static void _EMITEQVI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1018:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	if( *(int64*)((uint8*)DVREG$1 + 8ll) != 1ll) goto label$1021;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"1", 2ll, 0 );
	}
	goto label$1020;
	label$1021:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	}
	label$1020:;
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1019:;
}

static void _EMITIMPL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1022:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"or ", 4ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"or ", 4ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1023:;
}

static void _EMITIMPI( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1024:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 8ll) != 1ll) goto label$1027;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"1", 2ll, 0 );
	}
	goto label$1026;
	label$1027:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	}
	label$1026:;
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"or ", 4ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1025:;
}

static void _EMITATN2( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1028:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 == 4ll) goto label$1031;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fld ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$1030;
	label$1031:;
	{
		OUTP( (char*)"fxch" );
	}
	label$1030:;
	OUTP( (char*)"fpatan" );
	label$1029:;
}

static void _EMITPOW( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1032:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 == 4ll) goto label$1035;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fld ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		OUTP( (char*)"fxch" );
	}
	label$1035:;
	label$1034:;
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
	label$1033:;
}

static void HCMPL( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, char* MNEMONIC$1, char* REV_MNEMONIC$1, char* USG_MNEMONIC$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, int64 ISINVERSE$1 )
{
	FBSTRING TMP$645$1;
	FBSTRING TMP$647$1;
	label$1036:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING RNAME$1;
	static FBSTRING OSTR$1;
	static FBSTRING LNAME$1;
	static FBSTRING FALSELABEL$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	if( LABEL$1 != (struct $8FBSYMBOL*)0ull) goto label$1039;
	{
		char* vr$0 = SYMBUNIQUELABEL(  );
		fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$0, 0ll, 0 );
	}
	goto label$1038;
	label$1039:;
	{
		char* vr$1 = SYMBGETMANGLEDNAME( LABEL$1 );
		fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$1, 0ll, 0 );
	}
	label$1038:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"cmp ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	char* vr$2 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&FALSELABEL$1, -1ll, (void*)vr$2, 0ll, 0 );
	if( RVREG$1 == (struct $6IRVREG*)0ull) goto label$1041;
	{
		HPREPOPERAND( RVREG$1, &RNAME$1, 2147483648ll, 0ll, 0ll, -1ll );
		HMOV( (char*)*(char**)&RNAME$1, (char*)"-1" );
	}
	label$1041:;
	label$1040:;
	__builtin_memset( &TMP$645$1, 0, 24ll );
	FBSTRING* vr$5 = fb_StrConcat( &TMP$645$1, (void*)"j", 2ll, (void*)MNEMONIC$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$5, -1ll, 0 );
	if( ISINVERSE$1 != 0ll) goto label$1043;
	{
		HBRANCH( (char*)*(char**)&OSTR$1, (char*)*(char**)&LNAME$1 );
	}
	goto label$1042;
	label$1043:;
	{
		HBRANCH( (char*)*(char**)&OSTR$1, (char*)*(char**)&FALSELABEL$1 );
	}
	label$1042:;
	int64 vr$6 = fb_StrLen( (void*)REV_MNEMONIC$1, 0ll );
	if( vr$6 <= 0ll) goto label$1045;
	{
		FBSTRING TMP$646$2;
		__builtin_memset( &TMP$646$2, 0, 24ll );
		FBSTRING* vr$9 = fb_StrConcat( &TMP$646$2, (void*)"j", 2ll, (void*)REV_MNEMONIC$1, 0ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$9, -1ll, 0 );
		HBRANCH( (char*)*(char**)&OSTR$1, (char*)*(char**)&FALSELABEL$1 );
	}
	label$1045:;
	label$1044:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"cmp ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	__builtin_memset( &TMP$647$1, 0, 24ll );
	FBSTRING* vr$12 = fb_StrConcat( &TMP$647$1, (void*)"j", 2ll, (void*)USG_MNEMONIC$1, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$12, -1ll, 0 );
	HBRANCH( (char*)*(char**)&OSTR$1, (char*)*(char**)&LNAME$1 );
	HLABEL( (char*)*(char**)&FALSELABEL$1 );
	if( RVREG$1 == (struct $6IRVREG*)0ull) goto label$1047;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		HLABEL( (char*)*(char**)&LNAME$1 );
	}
	label$1047:;
	label$1046:;
	label$1037:;
}

static void HCMPI( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, char* MNEMONIC$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1048:;
	static FBSTRING RNAME$1;
	static FBSTRING RNAME8$1;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static FBSTRING LNAME$1;
	static int64 ISEDXFREE$1;
	static int64 DOTEST$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( LABEL$1 != (struct $8FBSYMBOL*)0ull) goto label$1051;
	{
		char* vr$0 = SYMBUNIQUELABEL(  );
		fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$0, 0ll, 0 );
	}
	goto label$1050;
	label$1051:;
	{
		char* vr$1 = SYMBGETMANGLEDNAME( LABEL$1 );
		fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$1, 0ll, 0 );
	}
	label$1050:;
	DOTEST$1 = 0ll;
	if( ((int64)-(*(int64*)SVREG$1 == 0ll) & (int64)-(*(int64*)DVREG$1 == 4ll)) == 0ll) goto label$1053;
	{
		if( *(int64*)((uint8*)SVREG$1 + 48ll) != 0ll) goto label$1055;
		{
			DOTEST$1 = -1ll;
		}
		label$1055:;
		label$1054:;
	}
	label$1053:;
	label$1052:;
	if( DOTEST$1 == 0ll) goto label$1057;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"test ", 6ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$1056;
	label$1057:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"cmp ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$1056:;
	if( RVREG$1 != (struct $6IRVREG*)0ull) goto label$1059;
	{
		FBSTRING TMP$649$2;
		__builtin_memset( &TMP$649$2, 0, 24ll );
		FBSTRING* vr$10 = fb_StrConcat( &TMP$649$2, (void*)"j", 2ll, (void*)MNEMONIC$1, 0ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$10, -1ll, 0 );
		HBRANCH( (char*)*(char**)&OSTR$1, (char*)*(char**)&LNAME$1 );
		goto label$1049;
	}
	label$1059:;
	label$1058:;
	HPREPOPERAND( RVREG$1, &RNAME$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( ((int64)-(*(int64*)((uint8*)&ENV$ + 224ll) >= 1ll) & (int64)-(*(int64*)RVREG$1 == 4ll)) == 0ll) goto label$1061;
	{
		char* vr$16 = HGETREGNAME( 2ll, *(int64*)((uint8*)RVREG$1 + 24ll) );
		fb_StrAssign( (void*)&RNAME8$1, -1ll, (void*)vr$16, 0ll, 0 );
		if( ((int64)-(*(int64*)((uint8*)RVREG$1 + 24ll) == 2ll) | (int64)-(*(int64*)((uint8*)RVREG$1 + 24ll) == 1ll)) == 0ll) goto label$1063;
		{
			FBSTRING TMP$653$3;
			FBSTRING TMP$654$3;
			int64 vr$22 = HISREGFREE( 0ll, 0ll );
			ISEDXFREE$1 = vr$22;
			if( ISEDXFREE$1 != 0ll) goto label$1065;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xchg edx, ", 11ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
			}
			label$1065:;
			label$1064:;
			__builtin_memset( &TMP$653$3, 0, 24ll );
			FBSTRING* vr$25 = fb_StrConcat( &TMP$653$3, (void*)"set", 4ll, (void*)MNEMONIC$1, 0ll );
			__builtin_memset( &TMP$654$3, 0, 24ll );
			FBSTRING* vr$28 = fb_StrConcat( &TMP$654$3, (void*)vr$25, -1ll, (void*)" dl", 4ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$28, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			if( ISEDXFREE$1 != 0ll) goto label$1067;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xchg edx, ", 11ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
			}
			goto label$1066;
			label$1067:;
			{
				HMOV( (char*)*(char**)&RNAME$1, (char*)"edx" );
			}
			label$1066:;
		}
		goto label$1062;
		label$1063:;
		{
			FBSTRING TMP$655$3;
			FBSTRING TMP$656$3;
			FBSTRING TMP$657$3;
			__builtin_memset( &TMP$655$3, 0, 24ll );
			FBSTRING* vr$31 = fb_StrConcat( &TMP$655$3, (void*)"set", 4ll, (void*)MNEMONIC$1, 0ll );
			__builtin_memset( &TMP$656$3, 0, 24ll );
			FBSTRING* vr$34 = fb_StrConcat( &TMP$656$3, (void*)vr$31, -1ll, (void*)" ", 2ll );
			__builtin_memset( &TMP$657$3, 0, 24ll );
			FBSTRING* vr$37 = fb_StrConcat( &TMP$657$3, (void*)vr$34, -1ll, (void*)&RNAME8$1, -1ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$37, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$1062:;
		if( *(int64*)((uint8*)RVREG$1 + 8ll) == 1ll) goto label$1069;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"shr ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 1", 4ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sbb ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$1069:;
		label$1068:;
	}
	goto label$1060;
	label$1061:;
	{
		FBSTRING TMP$660$2;
		if( *(int64*)((uint8*)RVREG$1 + 8ll) != 1ll) goto label$1071;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 1", 4ll, 0 );
		}
		goto label$1070;
		label$1071:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", -1", 5ll, 0 );
		}
		label$1070:;
		OUTP( (char*)*(char**)&OSTR$1 );
		__builtin_memset( &TMP$660$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$660$2, (void*)"j", 2ll, (void*)MNEMONIC$1, 0ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$42, -1ll, 0 );
		HBRANCH( (char*)*(char**)&OSTR$1, (char*)*(char**)&LNAME$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		HLABEL( (char*)*(char**)&LNAME$1 );
	}
	label$1060:;
	label$1049:;
}

static struct $11CMPF_RECIPE* HCMPF_GET_RECIPE( $7CMPF_OP OP$1, $10IR_EMITOPT OPTIONS$1, struct $8FBSYMBOL* LABEL$1 )
{
	struct $11CMPF_RECIPE* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1130:;
	static struct $11CMPF_RECIPE RECIPE$1[24] = { { 0ll, (char*)"e", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 1ll, (char*)"ne", (char*)"", (char*)"", (char*)"", -1ll, 0ll, 0ll, 0ll }, { 2ll, (char*)"a", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 3ll, (char*)"a", (char*)"", (char*)"", (char*)"", 0ll, 0ll, -1ll, 0ll }, { 4ll, (char*)"ae", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 5ll, (char*)"ae", (char*)"", (char*)"", (char*)"", 0ll, 0ll, -1ll, 0ll }, { 0ll, (char*)"ne", (char*)"", (char*)"", (char*)"", -1ll, 0ll, 0ll, 0ll }, { 1ll, (char*)"e", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 2ll, (char*)"a", (char*)"be", (char*)"", (char*)"", 0ll, 0ll, 0ll, -1ll }, { 3ll, (char*)"a", (char*)"be", (char*)"", (char*)"", 0ll, 0ll, -1ll, -1ll }, { 4ll, (char*)"ae", (char*)"b", (char*)"", (char*)"", 0ll, 0ll, 0ll, -1ll }, { 5ll, (char*)"ae", (char*)"b", (char*)"", (char*)"", 0ll, 0ll, -1ll, -1ll }, { 0ll, (char*)"e", (char*)"", (char*)"", (char*)"", 0ll, -1ll, 0ll, 0ll }, { 1ll, (char*)"ne", (char*)"", (char*)"", (char*)"", -1ll, 0ll, 0ll, 0ll }, { 2ll, (char*)"a", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 3ll, (char*)"a", (char*)"", (char*)"", (char*)"", 0ll, 0ll, -1ll, 0ll }, { 4ll, (char*)"ae", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 5ll, (char*)"ae", (char*)"", (char*)"", (char*)"", 0ll, 0ll, -1ll, 0ll }, { 0ll, (char*)"ne", (char*)"", (char*)"nz", (char*)"0b01000000", -1ll, 0ll, 0ll, 0ll }, { 1ll, (char*)"e", (char*)"", (char*)"z", (char*)"0b01000000", 0ll, -1ll, 0ll, 0ll }, { 2ll, (char*)"be", (char*)"", (char*)"z", (char*)"0b01000001", 0ll, 0ll, 0ll, 0ll }, { 3ll, (char*)"be", (char*)"", (char*)"nz", (char*)"0b00000001", 0ll, 0ll, -1ll, 0ll }, { 4ll, (char*)"b", (char*)"", (char*)"", (char*)"", 0ll, 0ll, 0ll, 0ll }, { 5ll, (char*)"b", (char*)"", (char*)"nz", (char*)"0b01000001", 0ll, 0ll, -1ll, 0ll } };
	static struct $8FBARRAY1I11CMPF_RECIPEE tmp$686$1 = { (struct $11CMPF_RECIPE*)RECIPE$1, (struct $11CMPF_RECIPE*)RECIPE$1, 1728ll, 72ll, 1ll, 49ll, { { 24ll, 0ll, 23ll } } };
	int64 INDEX$1;
	INDEX$1 = OP$1;
	if( LABEL$1 == (struct $8FBSYMBOL*)0ull) goto label$1133;
	{
		INDEX$1 = INDEX$1 + 12ll;
	}
	label$1133:;
	label$1132:;
	if( (OPTIONS$1 & 1ll) == 0ll) goto label$1135;
	{
		INDEX$1 = INDEX$1 + 6ll;
	}
	label$1135:;
	label$1134:;
	fb$result$1 = (struct $11CMPF_RECIPE*)((int64)(struct $11CMPF_RECIPE*)RECIPE$1 + (INDEX$1 * 72ll));
	goto label$1131;
	label$1131:;
	return fb$result$1;
}

static void HCMPF( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, struct $11CMPF_RECIPE* RECIPE$1 )
{
	label$1136:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static FBSTRING LNAME$1;
	static int64 ISEAXFREE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( LABEL$1 != (struct $8FBSYMBOL*)0ull) goto label$1139;
	{
		char* vr$0 = SYMBUNIQUELABEL(  );
		fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$0, 0ll, 0 );
	}
	goto label$1138;
	label$1139:;
	{
		char* vr$1 = SYMBGETMANGLEDNAME( LABEL$1 );
		fb_StrAssign( (void*)&LNAME$1, -1ll, (void*)vr$1, 0ll, 0 );
	}
	label$1138:;
	if( *(int64*)((uint8*)&ENV$ + 224ll) < 3ll) goto label$1141;
	{
		if( *(int64*)SVREG$1 != 4ll) goto label$1143;
		{
			if( *(int64*)((uint8*)RECIPE$1 + 56ll) == 0ll) goto label$1145;
			{
				OUTP( (char*)"fxch" );
			}
			label$1145:;
			label$1144:;
			OUTP( (char*)"fcomip" );
		}
		goto label$1142;
		label$1143:;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fld ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
			if( *(int64*)((uint8*)RECIPE$1 + 56ll) != 0ll) goto label$1147;
			{
				OUTP( (char*)"fxch" );
			}
			label$1147:;
			label$1146:;
			OUTP( (char*)"fcomip" );
		}
		label$1142:;
		OUTP( (char*)"fstp st(0)" );
	}
	goto label$1140;
	label$1141:;
	{
		if( *(int64*)SVREG$1 != 4ll) goto label$1149;
		{
			if( *(int64*)((uint8*)RECIPE$1 + 56ll) == 0ll) goto label$1151;
			{
				OUTP( (char*)"fxch" );
			}
			label$1151:;
			label$1150:;
			OUTP( (char*)"fcompp" );
		}
		goto label$1148;
		label$1149:;
		{
			if( *(int64*)((uint8*)RECIPE$1 + 56ll) == 0ll) goto label$1153;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fld ", 5ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
				OUTP( (char*)"fcompp" );
			}
			goto label$1152;
			label$1153:;
			{
				fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fcomp ", 7ll, 0 );
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
				OUTP( (char*)*(char**)&OSTR$1 );
			}
			label$1152:;
		}
		label$1148:;
		int64 vr$8 = HISREGFREE( 0ll, 5ll );
		ISEAXFREE$1 = vr$8;
		if( RVREG$1 == (struct $6IRVREG*)0ull) goto label$1155;
		{
			ISEAXFREE$1 = (int64)-(*(int64*)((uint8*)RVREG$1 + 24ll) == 5ll);
		}
		label$1155:;
		label$1154:;
		if( ISEAXFREE$1 != 0ll) goto label$1157;
		{
			HPUSH( (char*)"eax" );
		}
		label$1157:;
		label$1156:;
		OUTP( (char*)"fnstsw ax" );
		OUTP( (char*)"sahf" );
		if( ISEAXFREE$1 != 0ll) goto label$1159;
		{
			HPOP( (char*)"eax" );
		}
		label$1159:;
		label$1158:;
	}
	label$1140:;
	if( RVREG$1 != (struct $6IRVREG*)0ull) goto label$1161;
	{
		HCMPF_JXX( RECIPE$1, &LNAME$1 );
		goto label$1137;
	}
	label$1161:;
	label$1160:;
	HCMPF_SET( RVREG$1, RECIPE$1, &LNAME$1 );
	label$1137:;
}

static void _EMITCGTL( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	int64 TMP$703$1;
	label$1162:;
	static FBSTRING JMP$1;
	static FBSTRING RJMP$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1164;
	TMP$703$1 = 24ll;
	goto label$1927;
	label$1164:;
	TMP$703$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1927:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$703$1 * 56ll)) + 16ll) == 0ll) goto label$1166;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"g", 2ll, 0 );
		fb_StrAssign( (void*)&RJMP$1, -1ll, (void*)"l", 2ll, 0 );
	}
	goto label$1165;
	label$1166:;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"a", 2ll, 0 );
		fb_StrAssign( (void*)&RJMP$1, -1ll, (void*)"b", 2ll, 0 );
	}
	label$1165:;
	HCMPL( RVREG$1, LABEL$1, (char*)*(char**)&JMP$1, (char*)*(char**)&RJMP$1, (char*)"a", DVREG$1, SVREG$1, 0ll );
	label$1163:;
}

static void _EMITCGTI( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	int64 TMP$706$1;
	label$1167:;
	static FBSTRING JMP$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1169;
	TMP$706$1 = 24ll;
	goto label$1928;
	label$1169:;
	TMP$706$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1928:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$706$1 * 56ll)) + 16ll) == 0ll) goto label$1171;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"g", 2ll, 0 );
	}
	goto label$1170;
	label$1171:;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"a", 2ll, 0 );
	}
	label$1170:;
	HCMPI( RVREG$1, LABEL$1, (char*)*(char**)&JMP$1, DVREG$1, SVREG$1 );
	label$1168:;
}

static void _EMITCGTF( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1172:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 2ll, OPTIONS$1, LABEL$1 );
	HCMPF( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$1173:;
}

static void _EMITCLTL( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	int64 TMP$707$1;
	label$1174:;
	static FBSTRING JMP$1;
	static FBSTRING RJMP$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1176;
	TMP$707$1 = 24ll;
	goto label$1929;
	label$1176:;
	TMP$707$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1929:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$707$1 * 56ll)) + 16ll) == 0ll) goto label$1178;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"l", 2ll, 0 );
		fb_StrAssign( (void*)&RJMP$1, -1ll, (void*)"g", 2ll, 0 );
	}
	goto label$1177;
	label$1178:;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"b", 2ll, 0 );
		fb_StrAssign( (void*)&RJMP$1, -1ll, (void*)"a", 2ll, 0 );
	}
	label$1177:;
	HCMPL( RVREG$1, LABEL$1, (char*)*(char**)&JMP$1, (char*)*(char**)&RJMP$1, (char*)"b", DVREG$1, SVREG$1, 0ll );
	label$1175:;
}

static void _EMITCLTI( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	int64 TMP$708$1;
	label$1179:;
	static FBSTRING JMP$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1181;
	TMP$708$1 = 24ll;
	goto label$1930;
	label$1181:;
	TMP$708$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1930:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$708$1 * 56ll)) + 16ll) == 0ll) goto label$1183;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"l", 2ll, 0 );
	}
	goto label$1182;
	label$1183:;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"b", 2ll, 0 );
	}
	label$1182:;
	HCMPI( RVREG$1, LABEL$1, (char*)*(char**)&JMP$1, DVREG$1, SVREG$1 );
	label$1180:;
}

static void _EMITCLTF( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1184:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 3ll, OPTIONS$1, LABEL$1 );
	HCMPF( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$1185:;
}

static void _EMITCEQL( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1186:;
	HCMPL( RVREG$1, LABEL$1, (char*)"ne", (char*)"", (char*)"e", DVREG$1, SVREG$1, -1ll );
	label$1187:;
}

static void _EMITCEQI( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1188:;
	HCMPI( RVREG$1, LABEL$1, (char*)"e", DVREG$1, SVREG$1 );
	label$1189:;
}

static void _EMITCEQF( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1190:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 0ll, OPTIONS$1, LABEL$1 );
	HCMPF( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$1191:;
}

static void _EMITCNEL( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1192:;
	HCMPL( RVREG$1, LABEL$1, (char*)"ne", (char*)"", (char*)"ne", DVREG$1, SVREG$1, 0ll );
	label$1193:;
}

static void _EMITCNEI( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1194:;
	HCMPI( RVREG$1, LABEL$1, (char*)"ne", DVREG$1, SVREG$1 );
	label$1195:;
}

static void _EMITCNEF( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1196:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 1ll, OPTIONS$1, LABEL$1 );
	HCMPF( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$1197:;
}

static void _EMITCLEL( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	int64 TMP$709$1;
	label$1198:;
	static FBSTRING JMP$1;
	static FBSTRING RJMP$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1200;
	TMP$709$1 = 24ll;
	goto label$1931;
	label$1200:;
	TMP$709$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1931:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$709$1 * 56ll)) + 16ll) == 0ll) goto label$1202;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"l", 2ll, 0 );
		fb_StrAssign( (void*)&RJMP$1, -1ll, (void*)"g", 2ll, 0 );
	}
	goto label$1201;
	label$1202:;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"b", 2ll, 0 );
		fb_StrAssign( (void*)&RJMP$1, -1ll, (void*)"a", 2ll, 0 );
	}
	label$1201:;
	HCMPL( RVREG$1, LABEL$1, (char*)*(char**)&JMP$1, (char*)*(char**)&RJMP$1, (char*)"be", DVREG$1, SVREG$1, 0ll );
	label$1199:;
}

static void _EMITCLEI( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	int64 TMP$710$1;
	label$1203:;
	static FBSTRING JMP$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1205;
	TMP$710$1 = 24ll;
	goto label$1932;
	label$1205:;
	TMP$710$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1932:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$710$1 * 56ll)) + 16ll) == 0ll) goto label$1207;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"le", 3ll, 0 );
	}
	goto label$1206;
	label$1207:;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"be", 3ll, 0 );
	}
	label$1206:;
	HCMPI( RVREG$1, LABEL$1, (char*)*(char**)&JMP$1, DVREG$1, SVREG$1 );
	label$1204:;
}

static void _EMITCLEF( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1208:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 5ll, OPTIONS$1, LABEL$1 );
	HCMPF( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$1209:;
}

static void _EMITCGEL( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	int64 TMP$712$1;
	label$1210:;
	static FBSTRING JMP$1;
	static FBSTRING RJMP$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1212;
	TMP$712$1 = 24ll;
	goto label$1933;
	label$1212:;
	TMP$712$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1933:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$712$1 * 56ll)) + 16ll) == 0ll) goto label$1214;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"g", 2ll, 0 );
		fb_StrAssign( (void*)&RJMP$1, -1ll, (void*)"l", 2ll, 0 );
	}
	goto label$1213;
	label$1214:;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"a", 2ll, 0 );
		fb_StrAssign( (void*)&RJMP$1, -1ll, (void*)"b", 2ll, 0 );
	}
	label$1213:;
	HCMPL( RVREG$1, LABEL$1, (char*)*(char**)&JMP$1, (char*)*(char**)&RJMP$1, (char*)"ae", DVREG$1, SVREG$1, 0ll );
	label$1211:;
}

static void _EMITCGEI( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	int64 TMP$713$1;
	label$1215:;
	static FBSTRING JMP$1;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1217;
	TMP$713$1 = 24ll;
	goto label$1934;
	label$1217:;
	TMP$713$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1934:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$713$1 * 56ll)) + 16ll) == 0ll) goto label$1219;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"ge", 3ll, 0 );
	}
	goto label$1218;
	label$1219:;
	{
		fb_StrAssign( (void*)&JMP$1, -1ll, (void*)"ae", 3ll, 0 );
	}
	label$1218:;
	HCMPI( RVREG$1, LABEL$1, (char*)*(char**)&JMP$1, DVREG$1, SVREG$1 );
	label$1216:;
}

static void _EMITCGEF( struct $6IRVREG* RVREG$1, struct $8FBSYMBOL* LABEL$1, struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, $10IR_EMITOPT OPTIONS$1 )
{
	label$1220:;
	struct $11CMPF_RECIPE* vr$0 = HCMPF_GET_RECIPE( 4ll, OPTIONS$1, LABEL$1 );
	HCMPF( RVREG$1, LABEL$1, DVREG$1, SVREG$1, vr$0 );
	label$1221:;
}

static void _EMITNEGL( struct $6IRVREG* DVREG$1 )
{
	label$1222:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"neg ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"adc ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 0", 4ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"neg ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1223:;
}

static void _EMITNEGI( struct $6IRVREG* DVREG$1 )
{
	label$1224:;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"neg ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1225:;
}

static void _EMITNEGF( struct $6IRVREG* DVREG$1 )
{
	label$1226:;
	OUTP( (char*)"fchs" );
	label$1227:;
}

static void _EMITNOTL( struct $6IRVREG* DVREG$1 )
{
	label$1228:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1229:;
}

static void _EMITNOTI( struct $6IRVREG* DVREG$1 )
{
	label$1230:;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)((uint8*)DVREG$1 + 8ll) != 1ll) goto label$1233;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"1", 2ll, 0 );
	}
	goto label$1232;
	label$1233:;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"not ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	}
	label$1232:;
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1231:;
}

static void _EMITABSL( struct $6IRVREG* DVREG$1 )
{
	label$1234:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static int64 REG$1;
	static int64 ISFREE$1;
	static FBSTRING RNAME$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	int64 vr$0 = HFINDREGNOTINVREG( DVREG$1, 0ll );
	REG$1 = vr$0;
	char* vr$1 = HGETREGNAME( 8ll, REG$1 );
	fb_StrAssign( (void*)&RNAME$1, -1ll, (void*)vr$1, 0ll, 0 );
	int64 vr$2 = HISREGFREE( 0ll, REG$1 );
	ISFREE$1 = vr$2;
	if( ISFREE$1 != 0ll) goto label$1237;
	{
		HPUSH( (char*)*(char**)&RNAME$1 );
	}
	label$1237:;
	label$1236:;
	HMOV( (char*)*(char**)&RNAME$1, (char*)*(char**)&DST2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sar ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", 31", 5ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sub ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sbb ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	if( ISFREE$1 != 0ll) goto label$1239;
	{
		HPOP( (char*)*(char**)&RNAME$1 );
	}
	label$1239:;
	label$1238:;
	label$1235:;
}

static void _EMITABSI( struct $6IRVREG* DVREG$1 )
{
	int64 TMP$717$1;
	label$1240:;
	static FBSTRING DST$1;
	static int64 REG$1;
	static int64 ISFREE$1;
	static FBSTRING RNAME$1;
	static int64 BITS$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	int64 vr$0 = HFINDREGNOTINVREG( DVREG$1, 0ll );
	REG$1 = vr$0;
	char* vr$2 = HGETREGNAME( *(int64*)((uint8*)DVREG$1 + 8ll), REG$1 );
	fb_StrAssign( (void*)&RNAME$1, -1ll, (void*)vr$2, 0ll, 0 );
	int64 vr$3 = HISREGFREE( 0ll, REG$1 );
	ISFREE$1 = vr$3;
	if( ISFREE$1 != 0ll) goto label$1243;
	{
		HPUSH( (char*)*(char**)&RNAME$1 );
	}
	label$1243:;
	label$1242:;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1244;
	TMP$717$1 = 24ll;
	goto label$1935;
	label$1244:;
	TMP$717$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1935:;
	BITS$1 = (*(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$717$1 * 56ll)) + 8ll) << (3ll & 63ll)) + -1ll;
	HMOV( (char*)*(char**)&RNAME$1, (char*)*(char**)&DST$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sar ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	FBSTRING* vr$11 = fb_LongintToStr( BITS$1 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$11, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"xor ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sub ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&RNAME$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	if( ISFREE$1 != 0ll) goto label$1246;
	{
		HPOP( (char*)*(char**)&RNAME$1 );
	}
	label$1246:;
	label$1245:;
	label$1241:;
}

static void _EMITABSF( struct $6IRVREG* DVREG$1 )
{
	label$1247:;
	OUTP( (char*)"fabs" );
	label$1248:;
}

static void _EMITSGNL( struct $6IRVREG* DVREG$1 )
{
	FBSTRING TMP$718$1;
	FBSTRING TMP$719$1;
	FBSTRING TMP$720$1;
	FBSTRING TMP$723$1;
	FBSTRING TMP$724$1;
	FBSTRING TMP$725$1;
	label$1249:;
	FBSTRING LOW$1;
	__builtin_memset( &LOW$1, 0, 24ll );
	FBSTRING HIGH$1;
	__builtin_memset( &HIGH$1, 0, 24ll );
	FBSTRING EXITLABEL$1;
	__builtin_memset( &EXITLABEL$1, 0, 24ll );
	FBSTRING TMP32$1;
	__builtin_memset( &TMP32$1, 0, 24ll );
	int64 TMPREG$1;
	int64 ISTMPFREE$1;
	HPREPOPERAND64( DVREG$1, &LOW$1, &HIGH$1 );
	char* vr$6 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&EXITLABEL$1, -1ll, (void*)vr$6, 0ll, 0 );
	int64 vr$8 = HFINDREGNOTINVREG( DVREG$1, 0ll );
	TMPREG$1 = vr$8;
	int64 vr$9 = HISREGFREE( 0ll, TMPREG$1 );
	ISTMPFREE$1 = vr$9;
	char* vr$10 = HGETREGNAME( 8ll, TMPREG$1 );
	fb_StrAssign( (void*)&TMP32$1, -1ll, (void*)vr$10, 0ll, 0 );
	if( ISTMPFREE$1 != 0ll) goto label$1252;
	{
		HPUSH( (char*)*(char**)&TMP32$1 );
	}
	label$1252:;
	label$1251:;
	HMOV( (char*)*(char**)&TMP32$1, (char*)*(char**)&LOW$1 );
	__builtin_memset( &TMP$720$1, 0, 24ll );
	__builtin_memset( &TMP$718$1, 0, 24ll );
	FBSTRING* vr$16 = fb_StrConcat( &TMP$718$1, (void*)"cmp ", 5ll, (void*)&HIGH$1, -1ll );
	__builtin_memset( &TMP$719$1, 0, 24ll );
	FBSTRING* vr$19 = fb_StrConcat( &TMP$719$1, (void*)vr$16, -1ll, (void*)", 0", 4ll );
	fb_StrAssign( (void*)&TMP$720$1, -1ll, (void*)vr$19, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$720$1 );
	fb_StrDelete( (FBSTRING*)&TMP$720$1 );
	HMOV( (char*)*(char**)&LOW$1, (char*)"-1" );
	HMOV( (char*)*(char**)&HIGH$1, (char*)"-1" );
	HBRANCH( (char*)"jl", (char*)*(char**)&EXITLABEL$1 );
	HMOV( (char*)*(char**)&LOW$1, (char*)"1" );
	HMOV( (char*)*(char**)&HIGH$1, (char*)"0" );
	HBRANCH( (char*)"jg", (char*)*(char**)&EXITLABEL$1 );
	__builtin_memset( &TMP$725$1, 0, 24ll );
	__builtin_memset( &TMP$723$1, 0, 24ll );
	FBSTRING* vr$26 = fb_StrConcat( &TMP$723$1, (void*)"cmp ", 5ll, (void*)&TMP32$1, -1ll );
	__builtin_memset( &TMP$724$1, 0, 24ll );
	FBSTRING* vr$29 = fb_StrConcat( &TMP$724$1, (void*)vr$26, -1ll, (void*)", 0", 4ll );
	fb_StrAssign( (void*)&TMP$725$1, -1ll, (void*)vr$29, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$725$1 );
	fb_StrDelete( (FBSTRING*)&TMP$725$1 );
	HBRANCH( (char*)"jne", (char*)*(char**)&EXITLABEL$1 );
	HMOV( (char*)*(char**)&LOW$1, (char*)"0" );
	HLABEL( (char*)*(char**)&EXITLABEL$1 );
	if( ISTMPFREE$1 != 0ll) goto label$1254;
	{
		HPOP( (char*)*(char**)&TMP32$1 );
	}
	label$1254:;
	label$1253:;
	fb_StrDelete( (FBSTRING*)&TMP32$1 );
	fb_StrDelete( (FBSTRING*)&EXITLABEL$1 );
	fb_StrDelete( (FBSTRING*)&HIGH$1 );
	fb_StrDelete( (FBSTRING*)&LOW$1 );
	label$1250:;
}

static void _EMITSGNI( struct $6IRVREG* DVREG$1 )
{
	FBSTRING TMP$727$1;
	FBSTRING TMP$728$1;
	FBSTRING TMP$729$1;
	label$1255:;
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	FBSTRING EXITLABEL$1;
	__builtin_memset( &EXITLABEL$1, 0, 24ll );
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	char* vr$3 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&EXITLABEL$1, -1ll, (void*)vr$3, 0ll, 0 );
	__builtin_memset( &TMP$729$1, 0, 24ll );
	__builtin_memset( &TMP$727$1, 0, 24ll );
	FBSTRING* vr$9 = fb_StrConcat( &TMP$727$1, (void*)"cmp ", 5ll, (void*)&DST$1, -1ll );
	__builtin_memset( &TMP$728$1, 0, 24ll );
	FBSTRING* vr$12 = fb_StrConcat( &TMP$728$1, (void*)vr$9, -1ll, (void*)", 0", 4ll );
	fb_StrAssign( (void*)&TMP$729$1, -1ll, (void*)vr$12, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$729$1 );
	fb_StrDelete( (FBSTRING*)&TMP$729$1 );
	HBRANCH( (char*)"je", (char*)*(char**)&EXITLABEL$1 );
	HMOV( (char*)*(char**)&DST$1, (char*)"1" );
	HBRANCH( (char*)"jg", (char*)*(char**)&EXITLABEL$1 );
	HMOV( (char*)*(char**)&DST$1, (char*)"-1" );
	HLABEL( (char*)*(char**)&EXITLABEL$1 );
	fb_StrDelete( (FBSTRING*)&EXITLABEL$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	label$1256:;
}

static void _EMITSGNF( struct $6IRVREG* DVREG$1 )
{
	label$1257:;
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	FBSTRING LABEL$1;
	__builtin_memset( &LABEL$1, 0, 24ll );
	int64 ISEAXFREE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	char* vr$3 = SYMBUNIQUELABEL(  );
	fb_StrAssign( (void*)&LABEL$1, -1ll, (void*)vr$3, 0ll, 0 );
	if( *(int64*)((uint8*)&ENV$ + 224ll) < 3ll) goto label$1260;
	{
		OUTP( (char*)"fldz" );
		OUTP( (char*)"fxch" );
		OUTP( (char*)"fcomip" );
	}
	goto label$1259;
	label$1260:;
	{
		int64 vr$5 = HISREGFREE( 0ll, 5ll );
		ISEAXFREE$1 = vr$5;
		if( ISEAXFREE$1 != 0ll) goto label$1262;
		{
			HPUSH( (char*)"eax" );
		}
		label$1262:;
		label$1261:;
		OUTP( (char*)"ftst" );
		OUTP( (char*)"fnstsw ax" );
		OUTP( (char*)"sahf" );
		if( ISEAXFREE$1 != 0ll) goto label$1264;
		{
			HPOP( (char*)"eax" );
		}
		label$1264:;
		label$1263:;
	}
	label$1259:;
	HBRANCH( (char*)"jz", (char*)*(char**)&LABEL$1 );
	OUTP( (char*)"fstp st(0)" );
	OUTP( (char*)"fld1" );
	HBRANCH( (char*)"ja", (char*)*(char**)&LABEL$1 );
	OUTP( (char*)"fchs" );
	HLABEL( (char*)*(char**)&LABEL$1 );
	fb_StrDelete( (FBSTRING*)&LABEL$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	label$1258:;
}

static void _EMITSIN( struct $6IRVREG* DVREG$1 )
{
	label$1265:;
	OUTP( (char*)"fsin" );
	label$1266:;
}

static void _EMITASIN( struct $6IRVREG* DVREG$1 )
{
	label$1267:;
	OUTP( (char*)"fld st(0)" );
	OUTP( (char*)"fmul st(0), st(0)" );
	OUTP( (char*)"fld1" );
	OUTP( (char*)"fsubrp" );
	OUTP( (char*)"fsqrt" );
	OUTP( (char*)"fpatan" );
	label$1268:;
}

static void _EMITCOS( struct $6IRVREG* DVREG$1 )
{
	label$1269:;
	OUTP( (char*)"fcos" );
	label$1270:;
}

static void _EMITACOS( struct $6IRVREG* DVREG$1 )
{
	label$1271:;
	OUTP( (char*)"fld st(0)" );
	OUTP( (char*)"fmul st(0), st(0)" );
	OUTP( (char*)"fld1" );
	OUTP( (char*)"fsubrp" );
	OUTP( (char*)"fsqrt" );
	OUTP( (char*)"fxch" );
	OUTP( (char*)"fpatan" );
	label$1272:;
}

static void _EMITTAN( struct $6IRVREG* DVREG$1 )
{
	label$1273:;
	OUTP( (char*)"fptan" );
	OUTP( (char*)"fstp st(0)" );
	label$1274:;
}

static void _EMITATAN( struct $6IRVREG* DVREG$1 )
{
	label$1275:;
	OUTP( (char*)"fld1" );
	OUTP( (char*)"fpatan" );
	label$1276:;
}

static void _EMITSQRT( struct $6IRVREG* DVREG$1 )
{
	label$1277:;
	OUTP( (char*)"fsqrt" );
	label$1278:;
}

static void _EMITLOG( struct $6IRVREG* DVREG$1 )
{
	label$1279:;
	OUTP( (char*)"fldln2" );
	OUTP( (char*)"fxch" );
	OUTP( (char*)"fyl2x" );
	label$1280:;
}

static void _EMITEXP( struct $6IRVREG* DVREG$1 )
{
	label$1281:;
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
	label$1282:;
}

static void HFPUCHANGERC( FBSTRING* REGNAME$1, char* MODE$1 )
{
	FBSTRING TMP$755$1;
	FBSTRING TMP$756$1;
	FBSTRING TMP$757$1;
	FBSTRING TMP$758$1;
	FBSTRING TMP$759$1;
	label$1283:;
	OUTP( (char*)"sub esp, 4" );
	OUTP( (char*)"fnstcw [esp]" );
	HMOV( (char*)*(char**)REGNAME$1, (char*)"[esp]" );
	int32 vr$1 = fb_StrCompare( (void*)MODE$1, 0ll, (void*)"11", 3ll );
	if( (int64)vr$1 == 0ll) goto label$1286;
	{
		FBSTRING TMP$750$2;
		FBSTRING TMP$751$2;
		FBSTRING TMP$752$2;
		__builtin_memset( &TMP$752$2, 0, 24ll );
		__builtin_memset( &TMP$750$2, 0, 24ll );
		FBSTRING* vr$6 = fb_StrConcat( &TMP$750$2, (void*)"and ", 5ll, (void*)REGNAME$1, -1ll );
		__builtin_memset( &TMP$751$2, 0, 24ll );
		FBSTRING* vr$9 = fb_StrConcat( &TMP$751$2, (void*)vr$6, -1ll, (void*)", 0b1111001111111111", 21ll );
		fb_StrAssign( (void*)&TMP$752$2, -1ll, (void*)vr$9, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$752$2 );
		fb_StrDelete( (FBSTRING*)&TMP$752$2 );
	}
	label$1286:;
	label$1285:;
	__builtin_memset( &TMP$759$1, 0, 24ll );
	__builtin_memset( &TMP$756$1, 0, 24ll );
	FBSTRING* vr$15 = fb_StrConcat( &TMP$756$1, (void*)", 0b0000", 9ll, (void*)MODE$1, 0ll );
	__builtin_memset( &TMP$757$1, 0, 24ll );
	FBSTRING* vr$18 = fb_StrConcat( &TMP$757$1, (void*)vr$15, -1ll, (void*)"0000000000", 11ll );
	__builtin_memset( &TMP$755$1, 0, 24ll );
	FBSTRING* vr$21 = fb_StrConcat( &TMP$755$1, (void*)"or ", 4ll, (void*)REGNAME$1, -1ll );
	__builtin_memset( &TMP$758$1, 0, 24ll );
	FBSTRING* vr$24 = fb_StrConcat( &TMP$758$1, (void*)vr$21, -1ll, (void*)vr$18, -1ll );
	fb_StrAssign( (void*)&TMP$759$1, -1ll, (void*)vr$24, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$759$1 );
	fb_StrDelete( (FBSTRING*)&TMP$759$1 );
	HPUSH( (char*)*(char**)REGNAME$1 );
	OUTP( (char*)"fldcw [esp]" );
	OUTP( (char*)"add esp, 4" );
	label$1284:;
}

static void HEMITFLOATFUNC( int64 FUNC$1 )
{
	label$1287:;
	int64 REG$1;
	int64 ISREGFREE$1;
	FBSTRING REGNAME$1;
	__builtin_memset( &REGNAME$1, 0, 24ll );
	int64 vr$1 = HFINDFREEREG( 0ll );
	REG$1 = vr$1;
	if( REG$1 != -1ll) goto label$1290;
	{
		REG$1 = 5ll;
		ISREGFREE$1 = 0ll;
	}
	goto label$1289;
	label$1290:;
	{
		ISREGFREE$1 = -1ll;
	}
	label$1289:;
	char* vr$2 = HGETREGNAME( 8ll, REG$1 );
	fb_StrAssign( (void*)&REGNAME$1, -1ll, (void*)vr$2, 0ll, 0 );
	if( ISREGFREE$1 != 0ll) goto label$1292;
	{
		HPUSH( (char*)*(char**)&REGNAME$1 );
	}
	label$1292:;
	label$1291:;
	{
		if( FUNC$1 != 1ll) goto label$1294;
		label$1295:;
		{
			HFPUCHANGERC( &REGNAME$1, (char*)"01" );
			OUTP( (char*)"frndint" );
		}
		goto label$1293;
		label$1294:;
		if( FUNC$1 != 2ll) goto label$1296;
		label$1297:;
		{
			HFPUCHANGERC( &REGNAME$1, (char*)"11" );
			OUTP( (char*)"frndint" );
		}
		goto label$1293;
		label$1296:;
		if( FUNC$1 != 3ll) goto label$1298;
		label$1299:;
		{
			HFPUCHANGERC( &REGNAME$1, (char*)"11" );
			OUTP( (char*)"fld st(0)" );
			OUTP( (char*)"frndint" );
			OUTP( (char*)"fsubp" );
		}
		label$1298:;
		label$1293:;
	}
	OUTP( (char*)"fldcw [esp]" );
	OUTP( (char*)"add esp, 4" );
	if( ISREGFREE$1 != 0ll) goto label$1301;
	{
		HPOP( (char*)*(char**)&REGNAME$1 );
	}
	label$1301:;
	label$1300:;
	fb_StrDelete( (FBSTRING*)&REGNAME$1 );
	label$1288:;
}

static void HEMITFLOAT_INT_686( struct $6IRVREG* DVREG$1 )
{
	label$1302:;
	if( *(int64*)((uint8*)DVREG$1 + 8ll) != 15ll) goto label$1305;
	{
		OUTP( (char*)"sub esp, 4" );
		OUTP( (char*)"fist dword ptr [esp]" );
		OUTP( (char*)"fild dword ptr [esp]" );
	}
	goto label$1304;
	label$1305:;
	{
		OUTP( (char*)"sub esp, 8" );
		OUTP( (char*)"fld st(0)" );
		OUTP( (char*)"fistp qword ptr [esp]" );
		OUTP( (char*)"fild  qword ptr [esp]" );
	}
	label$1304:;
	OUTP( (char*)"fld1" );
	OUTP( (char*)"fsubr st(0), st(1)" );
	OUTP( (char*)"fxch st(2)" );
	OUTP( (char*)"fcomip" );
	OUTP( (char*)"fcmovb st(0), st(1)" );
	OUTP( (char*)"fstp st(1)" );
	if( *(int64*)((uint8*)DVREG$1 + 8ll) != 15ll) goto label$1307;
	{
		OUTP( (char*)"add esp, 4" );
	}
	goto label$1306;
	label$1307:;
	{
		OUTP( (char*)"add esp, 8" );
	}
	label$1306:;
	label$1303:;
}

static void HEMITFLOAT_FIX_686( struct $6IRVREG* DVREG$1 )
{
	label$1308:;
	if( *(int64*)((uint8*)DVREG$1 + 8ll) != 15ll) goto label$1311;
	{
		OUTP( (char*)"sub esp, 4" );
		OUTP( (char*)"fld st(0)" );
		OUTP( (char*)"fabs" );
		OUTP( (char*)"fist dword ptr [esp]" );
		OUTP( (char*)"fild dword ptr [esp]" );
	}
	goto label$1310;
	label$1311:;
	{
		OUTP( (char*)"sub esp, 8" );
		OUTP( (char*)"fld st(0)" );
		OUTP( (char*)"fabs" );
		OUTP( (char*)"fld st(0)" );
		OUTP( (char*)"fistp qword ptr [esp]" );
		OUTP( (char*)"fild  qword ptr [esp]" );
	}
	label$1310:;
	OUTP( (char*)"fld1" );
	OUTP( (char*)"fsubr st(1)" );
	OUTP( (char*)"fxch st(2)" );
	OUTP( (char*)"fcomip" );
	OUTP( (char*)"fcmovb st(0), st(1)" );
	OUTP( (char*)"fstp st(1)" );
	OUTP( (char*)"fldz" );
	OUTP( (char*)"fcomip st(2)" );
	OUTP( (char*)"fst st(1)" );
	OUTP( (char*)"fchs" );
	OUTP( (char*)"fcmovb st(0), st(1)" );
	OUTP( (char*)"fstp st(1)" );
	if( *(int64*)((uint8*)DVREG$1 + 8ll) != 15ll) goto label$1313;
	{
		OUTP( (char*)"add esp, 4" );
	}
	goto label$1312;
	label$1313:;
	{
		OUTP( (char*)"add esp, 8" );
	}
	label$1312:;
	label$1309:;
}

static void _EMITFLOOR( struct $6IRVREG* DVREG$1 )
{
	label$1314:;
	if( *(int64*)((uint8*)&ENV$ + 224ll) < 3ll) goto label$1317;
	{
		HEMITFLOAT_INT_686( DVREG$1 );
	}
	goto label$1316;
	label$1317:;
	{
		HEMITFLOATFUNC( 1ll );
	}
	label$1316:;
	label$1315:;
}

static void _EMITFIX( struct $6IRVREG* DVREG$1 )
{
	label$1318:;
	if( *(int64*)((uint8*)&ENV$ + 224ll) < 3ll) goto label$1321;
	{
		HEMITFLOAT_FIX_686( DVREG$1 );
	}
	goto label$1320;
	label$1321:;
	{
		HEMITFLOATFUNC( 2ll );
	}
	label$1320:;
	label$1319:;
}

static void _EMITFRAC( struct $6IRVREG* DVREG$1 )
{
	label$1322:;
	HEMITFLOATFUNC( 3ll );
	label$1323:;
}

static void _EMITCONVFD2FS( struct $6IRVREG* DVREG$1 )
{
	label$1324:;
	OUTP( (char*)"sub esp, 4" );
	OUTP( (char*)"fstp dword ptr [esp]" );
	OUTP( (char*)"fld dword ptr [esp]" );
	OUTP( (char*)"add esp, 4" );
	label$1325:;
}

static void _EMITXCHGTOS( struct $6IRVREG* SVREG$1 )
{
	label$1326:;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"fxch ", 6ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1327:;
}

static void _EMITSTACKALIGN( struct $6IRVREG* VREG$1, int64 UNUSED$1 )
{
	label$1328:;
	if( *(int64*)((uint8*)VREG$1 + 48ll) <= 0ll) goto label$1331;
	{
		FBSTRING TMP$774$2;
		FBSTRING TMP$775$2;
		__builtin_memset( &TMP$775$2, 0, 24ll );
		FBSTRING* vr$3 = fb_LongintToStr( *(int64*)((uint8*)VREG$1 + 48ll) );
		__builtin_memset( &TMP$774$2, 0, 24ll );
		FBSTRING* vr$6 = fb_StrConcat( &TMP$774$2, (void*)"sub esp, ", 10ll, (void*)vr$3, -1ll );
		fb_StrAssign( (void*)&TMP$775$2, -1ll, (void*)vr$6, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$775$2 );
		fb_StrDelete( (FBSTRING*)&TMP$775$2 );
	}
	goto label$1330;
	label$1331:;
	{
		FBSTRING TMP$776$2;
		FBSTRING TMP$777$2;
		__builtin_memset( &TMP$777$2, 0, 24ll );
		FBSTRING* vr$12 = fb_LongintToStr( -(*(int64*)((uint8*)VREG$1 + 48ll)) );
		__builtin_memset( &TMP$776$2, 0, 24ll );
		FBSTRING* vr$15 = fb_StrConcat( &TMP$776$2, (void*)"add esp, ", 10ll, (void*)vr$12, -1ll );
		fb_StrAssign( (void*)&TMP$777$2, -1ll, (void*)vr$15, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$777$2 );
		fb_StrDelete( (FBSTRING*)&TMP$777$2 );
	}
	label$1330:;
	label$1329:;
}

static void _EMITPUSHL( struct $6IRVREG* SVREG$1, int64 UNUSED$1 )
{
	label$1332:;
	static FBSTRING SRC1$1;
	static FBSTRING SRC2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1333:;
}

static void _EMITPUSHI( struct $6IRVREG* SVREG$1, int64 UNUSED$1 )
{
	int64 TMP$778$1;
	label$1334:;
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	FBSTRING TMP32$1;
	__builtin_memset( &TMP32$1, 0, 24ll );
	int64 SDSIZE$1;
	int64 TMPREG$1;
	int64 ISTMPFREE$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$1336;
	TMP$778$1 = 24ll;
	goto label$1936;
	label$1336:;
	TMP$778$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1936:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$778$1 * 56ll)) + 8ll);
	{
		$15IRVREGTYPE_ENUM TMP$779$2;
		TMP$779$2 = *($15IRVREGTYPE_ENUM*)SVREG$1;
		if( TMP$779$2 != 4ll) goto label$1338;
		label$1339:;
		{
			FBSTRING TMP$780$3;
			FBSTRING TMP$781$3;
			if( SDSIZE$1 >= 4ll) goto label$1341;
			{
				char* vr$10 = HGETREGNAME( 8ll, *(int64*)((uint8*)SVREG$1 + 24ll) );
				fb_StrAssign( (void*)&SRC$1, -1ll, (void*)vr$10, 0ll, 0 );
			}
			label$1341:;
			label$1340:;
			__builtin_memset( &TMP$781$3, 0, 24ll );
			__builtin_memset( &TMP$780$3, 0, 24ll );
			FBSTRING* vr$16 = fb_StrConcat( &TMP$780$3, (void*)"push ", 6ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$781$3, -1ll, (void*)vr$16, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$781$3 );
			fb_StrDelete( (FBSTRING*)&TMP$781$3 );
		}
		goto label$1337;
		label$1338:;
		if( TMP$779$2 != 0ll) goto label$1342;
		label$1343:;
		{
			FBSTRING TMP$782$3;
			FBSTRING TMP$783$3;
			__builtin_memset( &TMP$783$3, 0, 24ll );
			__builtin_memset( &TMP$782$3, 0, 24ll );
			FBSTRING* vr$23 = fb_StrConcat( &TMP$782$3, (void*)"push ", 6ll, (void*)&SRC$1, -1ll );
			fb_StrAssign( (void*)&TMP$783$3, -1ll, (void*)vr$23, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$783$3 );
			fb_StrDelete( (FBSTRING*)&TMP$783$3 );
		}
		goto label$1337;
		label$1342:;
		{
			if( SDSIZE$1 >= 4ll) goto label$1346;
			{
				FBSTRING TMP$784$4;
				FBSTRING TMP$785$4;
				FBSTRING TMP$786$4;
				FBSTRING TMP$787$4;
				FBSTRING TMP$788$4;
				FBSTRING TMP$789$4;
				int64 vr$26 = HFINDREGNOTINVREG( SVREG$1, 0ll );
				TMPREG$1 = vr$26;
				int64 vr$27 = HISREGFREE( 0ll, TMPREG$1 );
				ISTMPFREE$1 = vr$27;
				char* vr$28 = HGETREGNAME( 8ll, TMPREG$1 );
				fb_StrAssign( (void*)&TMP32$1, -1ll, (void*)vr$28, 0ll, 0 );
				if( ISTMPFREE$1 != 0ll) goto label$1348;
				{
					HPUSH( (char*)*(char**)&TMP32$1 );
				}
				label$1348:;
				label$1347:;
				__builtin_memset( &TMP$787$4, 0, 24ll );
				__builtin_memset( &TMP$784$4, 0, 24ll );
				FBSTRING* vr$35 = fb_StrConcat( &TMP$784$4, (void*)"movzx ", 7ll, (void*)&TMP32$1, -1ll );
				__builtin_memset( &TMP$785$4, 0, 24ll );
				FBSTRING* vr$38 = fb_StrConcat( &TMP$785$4, (void*)vr$35, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$786$4, 0, 24ll );
				FBSTRING* vr$41 = fb_StrConcat( &TMP$786$4, (void*)vr$38, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$787$4, -1ll, (void*)vr$41, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$787$4 );
				fb_StrDelete( (FBSTRING*)&TMP$787$4 );
				__builtin_memset( &TMP$789$4, 0, 24ll );
				__builtin_memset( &TMP$788$4, 0, 24ll );
				FBSTRING* vr$48 = fb_StrConcat( &TMP$788$4, (void*)"push ", 6ll, (void*)&TMP32$1, -1ll );
				fb_StrAssign( (void*)&TMP$789$4, -1ll, (void*)vr$48, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$789$4 );
				fb_StrDelete( (FBSTRING*)&TMP$789$4 );
				if( ISTMPFREE$1 != 0ll) goto label$1350;
				{
					HPOP( (char*)*(char**)&TMP32$1 );
				}
				label$1350:;
				label$1349:;
			}
			goto label$1345;
			label$1346:;
			{
				FBSTRING TMP$790$4;
				FBSTRING TMP$791$4;
				__builtin_memset( &TMP$791$4, 0, 24ll );
				__builtin_memset( &TMP$790$4, 0, 24ll );
				FBSTRING* vr$55 = fb_StrConcat( &TMP$790$4, (void*)"push ", 6ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$791$4, -1ll, (void*)vr$55, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$791$4 );
				fb_StrDelete( (FBSTRING*)&TMP$791$4 );
			}
			label$1345:;
		}
		label$1344:;
		label$1337:;
	}
	fb_StrDelete( (FBSTRING*)&TMP32$1 );
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	label$1335:;
}

static void _EMITPUSHF( struct $6IRVREG* SVREG$1, int64 UNUSED$1 )
{
	int64 TMP$792$1;
	label$1351:;
	static FBSTRING SRC$1;
	static int64 SDSIZE$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$1353;
	TMP$792$1 = 24ll;
	goto label$1937;
	label$1353:;
	TMP$792$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1937:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$792$1 * 56ll)) + 8ll);
	if( *(int64*)SVREG$1 == 4ll) goto label$1355;
	{
		if( *(int64*)((uint8*)SVREG$1 + 8ll) != 15ll) goto label$1357;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"push ", 6ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$1356;
		label$1357:;
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
		label$1356:;
	}
	goto label$1354;
	label$1355:;
	{
		FBSTRING TMP$794$2;
		FBSTRING TMP$795$2;
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"sub esp,", 9ll, 0 );
		FBSTRING* vr$7 = fb_LongintToStr( SDSIZE$1 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$7, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		__builtin_memset( &TMP$794$2, 0, 24ll );
		FBSTRING* vr$13 = fb_StrConcat( &TMP$794$2, (void*)"fstp ", 6ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)SVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
		__builtin_memset( &TMP$795$2, 0, 24ll );
		FBSTRING* vr$16 = fb_StrConcat( &TMP$795$2, (void*)vr$13, -1ll, (void*)" [esp]", 7ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$16, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$1354:;
	label$1352:;
}

static void _EMITPUSHUDT( struct $6IRVREG* SVREG$1, int64 SDSIZE$1 )
{
	label$1358:;
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	FBSTRING TMP32$1;
	__builtin_memset( &TMP32$1, 0, 24ll );
	FBSTRING TMP16$1;
	__builtin_memset( &TMP16$1, 0, 24ll );
	int64 OFS$1;
	int64 TMPREG$1;
	int64 ISTMPFREE$1;
	int64 REMAINDER$1;
	REMAINDER$1 = SDSIZE$1 & 3ll;
	if( REMAINDER$1 <= 0ll) goto label$1361;
	{
		FBSTRING TMP$816$2;
		FBSTRING TMP$817$2;
		int64 vr$4 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		TMPREG$1 = vr$4;
		int64 vr$5 = HISREGFREE( 0ll, TMPREG$1 );
		ISTMPFREE$1 = vr$5;
		char* vr$6 = HGETREGNAME( 8ll, TMPREG$1 );
		fb_StrAssign( (void*)&TMP32$1, -1ll, (void*)vr$6, 0ll, 0 );
		if( ISTMPFREE$1 != 0ll) goto label$1363;
		{
			HPUSH( (char*)*(char**)&TMP32$1 );
		}
		label$1363:;
		label$1362:;
		{
			if( REMAINDER$1 != 3ll) goto label$1365;
			label$1366:;
			{
				FBSTRING TMP$796$4;
				FBSTRING TMP$797$4;
				FBSTRING TMP$798$4;
				FBSTRING TMP$799$4;
				FBSTRING TMP$801$4;
				FBSTRING TMP$802$4;
				FBSTRING TMP$803$4;
				FBSTRING TMP$804$4;
				FBSTRING TMP$805$4;
				FBSTRING TMP$806$4;
				FBSTRING TMP$807$4;
				HPREPOPERAND( SVREG$1, &SRC$1, 2ll, SDSIZE$1 + -1ll, 0ll, -1ll );
				__builtin_memset( &TMP$799$4, 0, 24ll );
				__builtin_memset( &TMP$796$4, 0, 24ll );
				FBSTRING* vr$15 = fb_StrConcat( &TMP$796$4, (void*)"movzx ", 7ll, (void*)&TMP32$1, -1ll );
				__builtin_memset( &TMP$797$4, 0, 24ll );
				FBSTRING* vr$18 = fb_StrConcat( &TMP$797$4, (void*)vr$15, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$798$4, 0, 24ll );
				FBSTRING* vr$21 = fb_StrConcat( &TMP$798$4, (void*)vr$18, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$799$4, -1ll, (void*)vr$21, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$799$4 );
				fb_StrDelete( (FBSTRING*)&TMP$799$4 );
				__builtin_memset( &TMP$803$4, 0, 24ll );
				__builtin_memset( &TMP$801$4, 0, 24ll );
				FBSTRING* vr$28 = fb_StrConcat( &TMP$801$4, (void*)"shl ", 5ll, (void*)&TMP32$1, -1ll );
				__builtin_memset( &TMP$802$4, 0, 24ll );
				FBSTRING* vr$31 = fb_StrConcat( &TMP$802$4, (void*)vr$28, -1ll, (void*)", 16", 5ll );
				fb_StrAssign( (void*)&TMP$803$4, -1ll, (void*)vr$31, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$803$4 );
				fb_StrDelete( (FBSTRING*)&TMP$803$4 );
				char* vr$34 = HGETREGNAME( 5ll, TMPREG$1 );
				fb_StrAssign( (void*)&TMP16$1, -1ll, (void*)vr$34, 0ll, 0 );
				HPREPOPERAND( SVREG$1, &SRC$1, 5ll, SDSIZE$1 + -3ll, 0ll, -1ll );
				__builtin_memset( &TMP$807$4, 0, 24ll );
				__builtin_memset( &TMP$804$4, 0, 24ll );
				FBSTRING* vr$43 = fb_StrConcat( &TMP$804$4, (void*)"mov ", 5ll, (void*)&TMP16$1, -1ll );
				__builtin_memset( &TMP$805$4, 0, 24ll );
				FBSTRING* vr$46 = fb_StrConcat( &TMP$805$4, (void*)vr$43, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$806$4, 0, 24ll );
				FBSTRING* vr$49 = fb_StrConcat( &TMP$806$4, (void*)vr$46, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$807$4, -1ll, (void*)vr$49, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$807$4 );
				fb_StrDelete( (FBSTRING*)&TMP$807$4 );
			}
			goto label$1364;
			label$1365:;
			if( REMAINDER$1 != 2ll) goto label$1367;
			label$1368:;
			{
				FBSTRING TMP$808$4;
				FBSTRING TMP$809$4;
				FBSTRING TMP$810$4;
				FBSTRING TMP$811$4;
				OFS$1 = SDSIZE$1 + -2ll;
				HPREPOPERAND( SVREG$1, &SRC$1, 5ll, OFS$1, 0ll, -1ll );
				__builtin_memset( &TMP$811$4, 0, 24ll );
				__builtin_memset( &TMP$808$4, 0, 24ll );
				FBSTRING* vr$59 = fb_StrConcat( &TMP$808$4, (void*)"movzx ", 7ll, (void*)&TMP32$1, -1ll );
				__builtin_memset( &TMP$809$4, 0, 24ll );
				FBSTRING* vr$62 = fb_StrConcat( &TMP$809$4, (void*)vr$59, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$810$4, 0, 24ll );
				FBSTRING* vr$65 = fb_StrConcat( &TMP$810$4, (void*)vr$62, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$811$4, -1ll, (void*)vr$65, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$811$4 );
				fb_StrDelete( (FBSTRING*)&TMP$811$4 );
			}
			goto label$1364;
			label$1367:;
			if( REMAINDER$1 != 1ll) goto label$1369;
			label$1370:;
			{
				FBSTRING TMP$812$4;
				FBSTRING TMP$813$4;
				FBSTRING TMP$814$4;
				FBSTRING TMP$815$4;
				OFS$1 = SDSIZE$1 + -1ll;
				HPREPOPERAND( SVREG$1, &SRC$1, 2ll, OFS$1, 0ll, -1ll );
				__builtin_memset( &TMP$815$4, 0, 24ll );
				__builtin_memset( &TMP$812$4, 0, 24ll );
				FBSTRING* vr$75 = fb_StrConcat( &TMP$812$4, (void*)"movzx ", 7ll, (void*)&TMP32$1, -1ll );
				__builtin_memset( &TMP$813$4, 0, 24ll );
				FBSTRING* vr$78 = fb_StrConcat( &TMP$813$4, (void*)vr$75, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$814$4, 0, 24ll );
				FBSTRING* vr$81 = fb_StrConcat( &TMP$814$4, (void*)vr$78, -1ll, (void*)&SRC$1, -1ll );
				fb_StrAssign( (void*)&TMP$815$4, -1ll, (void*)vr$81, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$815$4 );
				fb_StrDelete( (FBSTRING*)&TMP$815$4 );
			}
			label$1369:;
			label$1364:;
		}
		__builtin_memset( &TMP$817$2, 0, 24ll );
		__builtin_memset( &TMP$816$2, 0, 24ll );
		FBSTRING* vr$88 = fb_StrConcat( &TMP$816$2, (void*)"push ", 6ll, (void*)&TMP32$1, -1ll );
		fb_StrAssign( (void*)&TMP$817$2, -1ll, (void*)vr$88, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$817$2 );
		fb_StrDelete( (FBSTRING*)&TMP$817$2 );
		if( ISTMPFREE$1 != 0ll) goto label$1372;
		{
			HPOP( (char*)*(char**)&TMP32$1 );
		}
		label$1372:;
		label$1371:;
		SDSIZE$1 = SDSIZE$1 - REMAINDER$1;
	}
	label$1361:;
	label$1360:;
	OFS$1 = SDSIZE$1 + -4ll;
	label$1373:;
	if( OFS$1 < 0ll) goto label$1374;
	{
		FBSTRING TMP$818$2;
		FBSTRING TMP$819$2;
		HPREPOPERAND( SVREG$1, &SRC$1, 8ll, OFS$1, 0ll, -1ll );
		__builtin_memset( &TMP$819$2, 0, 24ll );
		__builtin_memset( &TMP$818$2, 0, 24ll );
		FBSTRING* vr$98 = fb_StrConcat( &TMP$818$2, (void*)"push ", 6ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$819$2, -1ll, (void*)vr$98, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$819$2 );
		fb_StrDelete( (FBSTRING*)&TMP$819$2 );
		OFS$1 = OFS$1 + -4ll;
	}
	goto label$1373;
	label$1374:;
	fb_StrDelete( (FBSTRING*)&TMP16$1 );
	fb_StrDelete( (FBSTRING*)&TMP32$1 );
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	label$1359:;
}

static void _EMITPOPL( struct $6IRVREG* DVREG$1, int64 UNUSED$1 )
{
	label$1375:;
	static FBSTRING DST1$1;
	static FBSTRING DST2$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST1$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST2$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1376:;
}

static void _EMITPOPI( struct $6IRVREG* DVREG$1, int64 UNUSED$1 )
{
	int64 TMP$820$1;
	label$1377:;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	static int64 DSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1379;
	TMP$820$1 = 24ll;
	goto label$1938;
	label$1379:;
	TMP$820$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1938:;
	DSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$820$1 * 56ll)) + 8ll);
	if( *(int64*)DVREG$1 != 0ll) goto label$1381;
	{
		if( *(int64*)((uint8*)DVREG$1 + 48ll) != 4ll) goto label$1383;
		{
			int64 vr$7 = HISREGFREE( 0ll, 5ll );
			if( vr$7 == 0ll) goto label$1385;
			{
				HPOP( (char*)"eax" );
			}
			goto label$1384;
			label$1385:;
			{
				OUTP( (char*)"add esp, 4" );
			}
			label$1384:;
		}
		goto label$1382;
		label$1383:;
		{
			FBSTRING TMP$821$3;
			FBSTRING* vr$9 = fb_LongintToStr( *(int64*)((uint8*)DVREG$1 + 48ll) );
			__builtin_memset( &TMP$821$3, 0, 24ll );
			FBSTRING* vr$12 = fb_StrConcat( &TMP$821$3, (void*)"add esp, ", 10ll, (void*)vr$9, -1ll );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$12, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		label$1382:;
	}
	goto label$1380;
	label$1381:;
	if( DSIZE$1 != 4ll) goto label$1386;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	goto label$1380;
	label$1386:;
	{
		if( *(int64*)DVREG$1 != 4ll) goto label$1388;
		{
			char* vr$15 = HGETREGNAME( 8ll, *(int64*)((uint8*)DVREG$1 + 24ll) );
			fb_StrAssign( (void*)&DST$1, -1ll, (void*)vr$15, 0ll, 0 );
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$1387;
		label$1388:;
		{
			static int64 REG$3;
			static int64 ISFREE$3;
			static FBSTRING AUX8$3;
			static FBSTRING AUX16$3;
			static FBSTRING AUX32$3;
			int64 vr$16 = HFINDREGNOTINVREG( DVREG$1, 0ll );
			REG$3 = vr$16;
			char* vr$17 = HGETREGNAME( 2ll, REG$3 );
			fb_StrAssign( (void*)&AUX8$3, -1ll, (void*)vr$17, 0ll, 0 );
			char* vr$18 = HGETREGNAME( 5ll, REG$3 );
			fb_StrAssign( (void*)&AUX16$3, -1ll, (void*)vr$18, 0ll, 0 );
			char* vr$19 = HGETREGNAME( 8ll, REG$3 );
			fb_StrAssign( (void*)&AUX32$3, -1ll, (void*)vr$19, 0ll, 0 );
			int64 vr$20 = HISREGFREE( 0ll, REG$3 );
			ISFREE$3 = vr$20;
			if( ISFREE$3 == 0ll) goto label$1390;
			{
				HMOV( (char*)*(char**)&AUX32$3, (char*)"dword ptr [esp]" );
			}
			goto label$1389;
			label$1390:;
			{
				HXCHG( (char*)*(char**)&AUX32$3, (char*)"dword ptr [esp]" );
			}
			label$1389:;
			if( DSIZE$1 != 1ll) goto label$1392;
			{
				HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX8$3 );
			}
			goto label$1391;
			label$1392:;
			{
				HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX16$3 );
			}
			label$1391:;
			if( ISFREE$3 == 0ll) goto label$1394;
			{
				OUTP( (char*)"add esp, 4" );
			}
			goto label$1393;
			label$1394:;
			{
				HPOP( (char*)*(char**)&AUX32$3 );
			}
			label$1393:;
		}
		label$1387:;
	}
	label$1380:;
	label$1378:;
}

static void _EMITPOPF( struct $6IRVREG* DVREG$1, int64 UNUSED$1 )
{
	int64 TMP$822$1;
	label$1395:;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	static int64 DSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1397;
	TMP$822$1 = 24ll;
	goto label$1939;
	label$1397:;
	TMP$822$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1939:;
	DSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$822$1 * 56ll)) + 8ll);
	if( *(int64*)DVREG$1 == 4ll) goto label$1399;
	{
		if( *(int64*)((uint8*)DVREG$1 + 8ll) != 15ll) goto label$1401;
		{
			fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"pop ", 5ll, 0 );
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
			OUTP( (char*)*(char**)&OSTR$1 );
		}
		goto label$1400;
		label$1401:;
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
		label$1400:;
	}
	goto label$1398;
	label$1399:;
	{
		FBSTRING TMP$823$2;
		FBSTRING TMP$824$2;
		__builtin_memset( &TMP$823$2, 0, 24ll );
		FBSTRING* vr$12 = fb_StrConcat( &TMP$823$2, (void*)"fld ", 5ll, (void*)(((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)DVREG$1 + 8ll) * 24ll)) + 8ll), 12ll );
		__builtin_memset( &TMP$824$2, 0, 24ll );
		FBSTRING* vr$15 = fb_StrConcat( &TMP$824$2, (void*)vr$12, -1ll, (void*)" [esp]", 7ll );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)vr$15, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"add esp,", 9ll, 0 );
		FBSTRING* vr$16 = fb_LongintToStr( DSIZE$1 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$16, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
	}
	label$1398:;
	label$1396:;
}

static void _EMITPOPST0( struct $6IRVREG* DVREG$1, int64 UNUSED$1 )
{
	label$1402:;
	OUTP( (char*)"fstp st(0)" );
	label$1403:;
}

static void _EMITADDROF( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1404:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, 0ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"lea ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1405:;
}

static void _EMITDEREF( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1406:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 9ll, 0ll, 0ll, -1ll );
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ", 5ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", ", 3ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&SRC$1, -1ll, 0 );
	OUTP( (char*)*(char**)&OSTR$1 );
	label$1407:;
}

static void HMEMMOVEREP( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, int64 BYTES$1 )
{
	label$1408:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING OSTR$1;
	static int64 ECXFREE$1;
	static int64 EDIFREE$1;
	static int64 ESIFREE$1;
	static int64 EDIINSRC$1;
	static int64 ECXINSRC$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	int64 vr$0 = HISREGFREE( 0ll, 3ll );
	ECXFREE$1 = vr$0;
	int64 vr$1 = HISREGFREE( 0ll, 1ll );
	EDIFREE$1 = vr$1;
	int64 vr$2 = HISREGFREE( 0ll, 2ll );
	ESIFREE$1 = vr$2;
	int64 vr$3 = HISREGINVREG( SVREG$1, 1ll );
	EDIINSRC$1 = vr$3;
	int64 vr$4 = HISREGINVREG( SVREG$1, 3ll );
	ECXINSRC$1 = vr$4;
	if( ECXFREE$1 != 0ll) goto label$1411;
	{
		HPUSH( (char*)"ecx" );
	}
	label$1411:;
	label$1410:;
	if( EDIFREE$1 != 0ll) goto label$1413;
	{
		HPUSH( (char*)"edi" );
	}
	label$1413:;
	label$1412:;
	if( ESIFREE$1 != 0ll) goto label$1415;
	{
		HPUSH( (char*)"esi" );
	}
	label$1415:;
	label$1414:;
	if( EDIINSRC$1 != 0ll) goto label$1417;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$1419;
		{
			HMOV( (char*)"edi", (char*)*(char**)&DST$1 );
		}
		goto label$1418;
		label$1419:;
		{
			if( *(int64*)((uint8*)DVREG$1 + 24ll) == 1ll) goto label$1421;
			{
				HMOV( (char*)"edi", (char*)*(char**)&DST$1 );
			}
			label$1421:;
			label$1420:;
		}
		label$1418:;
	}
	goto label$1416;
	label$1417:;
	{
		if( ECXINSRC$1 == 0ll) goto label$1423;
		{
			HPUSH( (char*)"ecx" );
		}
		label$1423:;
		label$1422:;
		HMOV( (char*)"ecx", (char*)*(char**)&DST$1 );
		if( ECXINSRC$1 == 0ll) goto label$1425;
		{
			OUTP( (char*)"xchg ecx, [esp]" );
		}
		label$1425:;
		label$1424:;
	}
	label$1416:;
	if( *(int64*)SVREG$1 == 4ll) goto label$1427;
	{
		HMOV( (char*)"esi", (char*)*(char**)&SRC$1 );
	}
	goto label$1426;
	label$1427:;
	{
		if( *(int64*)((uint8*)SVREG$1 + 24ll) == 2ll) goto label$1429;
		{
			HMOV( (char*)"esi", (char*)*(char**)&SRC$1 );
		}
		label$1429:;
		label$1428:;
	}
	label$1426:;
	if( EDIINSRC$1 == 0ll) goto label$1431;
	{
		if( ECXINSRC$1 != 0ll) goto label$1433;
		{
			HMOV( (char*)"edi", (char*)"ecx" );
		}
		goto label$1432;
		label$1433:;
		{
			HPOP( (char*)"edi" );
		}
		label$1432:;
	}
	label$1431:;
	label$1430:;
	if( BYTES$1 <= 4ll) goto label$1435;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ecx, ", 10ll, 0 );
		FBSTRING* vr$10 = fb_ULongintToStr( (uint64)BYTES$1 >> (2ull & 63ll) );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$10, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		OUTP( (char*)"rep movsd" );
	}
	goto label$1434;
	label$1435:;
	if( BYTES$1 != 4ll) goto label$1436;
	{
		OUTP( (char*)"mov ecx, [esi]" );
		OUTP( (char*)"mov [edi], ecx" );
		if( (BYTES$1 & 3ll) <= 0ll) goto label$1438;
		{
			OUTP( (char*)"add esi, 4" );
			OUTP( (char*)"add edi, 4" );
		}
		label$1438:;
		label$1437:;
	}
	label$1436:;
	label$1434:;
	BYTES$1 = BYTES$1 & 3ll;
	if( BYTES$1 <= 0ll) goto label$1440;
	{
		if( BYTES$1 < 2ll) goto label$1442;
		{
			OUTP( (char*)"mov cx, [esi]" );
			OUTP( (char*)"mov [edi], cx" );
			if( BYTES$1 != 3ll) goto label$1444;
			{
				OUTP( (char*)"add esi, 2" );
				OUTP( (char*)"add edi, 2" );
			}
			label$1444:;
			label$1443:;
		}
		label$1442:;
		label$1441:;
		if( (BYTES$1 & 1ll) == 0ll) goto label$1446;
		{
			OUTP( (char*)"mov cl, [esi]" );
			OUTP( (char*)"mov [edi], cl" );
		}
		label$1446:;
		label$1445:;
	}
	label$1440:;
	label$1439:;
	if( ESIFREE$1 != 0ll) goto label$1448;
	{
		HPOP( (char*)"esi" );
	}
	label$1448:;
	label$1447:;
	if( EDIFREE$1 != 0ll) goto label$1450;
	{
		HPOP( (char*)"edi" );
	}
	label$1450:;
	label$1449:;
	if( ECXFREE$1 != 0ll) goto label$1452;
	{
		HPOP( (char*)"ecx" );
	}
	label$1452:;
	label$1451:;
	label$1409:;
}

static void HMEMMOVEBLK( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, int64 BYTES$1 )
{
	label$1453:;
	static FBSTRING DST$1;
	static FBSTRING SRC$1;
	static FBSTRING AUX$1;
	static int64 I$1;
	static int64 OFS$1;
	static int64 REG$1;
	static int64 ISFREE$1;
	int64 vr$0 = HFINDREGNOTINVREG( DVREG$1, 0ll );
	REG$1 = vr$0;
	int64 vr$1 = HISREGINVREG( SVREG$1, REG$1 );
	if( vr$1 == 0ll) goto label$1456;
	{
		HMEMMOVEREP( DVREG$1, SVREG$1, BYTES$1 );
		goto label$1454;
	}
	label$1456:;
	label$1455:;
	char* vr$2 = HGETREGNAME( 8ll, REG$1 );
	fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$2, 0ll, 0 );
	int64 vr$3 = HISREGFREE( 0ll, REG$1 );
	ISFREE$1 = vr$3;
	if( ISFREE$1 != 0ll) goto label$1458;
	{
		HPUSH( (char*)*(char**)&AUX$1 );
	}
	label$1458:;
	label$1457:;
	OFS$1 = 0ll;
	{
		I$1 = 1ll;
		int64 TMP$839$2;
		TMP$839$2 = (int64)((uint64)BYTES$1 >> (2ull & 63ll));
		goto label$1459;
		label$1462:;
		{
			HPREPOPERAND( SVREG$1, &SRC$1, 8ll, OFS$1, 0ll, -1ll );
			HMOV( (char*)*(char**)&AUX$1, (char*)*(char**)&SRC$1 );
			HPREPOPERAND( DVREG$1, &DST$1, 8ll, OFS$1, 0ll, -1ll );
			HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX$1 );
			OFS$1 = OFS$1 + 4ll;
		}
		label$1460:;
		I$1 = I$1 + 1ll;
		label$1459:;
		if( I$1 <= TMP$839$2) goto label$1462;
		label$1461:;
	}
	if( (BYTES$1 & 2ll) == 0ll) goto label$1464;
	{
		char* vr$8 = HGETREGNAME( 5ll, REG$1 );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$8, 0ll, 0 );
		HPREPOPERAND( SVREG$1, &SRC$1, 5ll, OFS$1, 0ll, -1ll );
		HMOV( (char*)*(char**)&AUX$1, (char*)*(char**)&SRC$1 );
		HPREPOPERAND( DVREG$1, &DST$1, 5ll, OFS$1, 0ll, -1ll );
		HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX$1 );
		OFS$1 = OFS$1 + 2ll;
	}
	label$1464:;
	label$1463:;
	if( (BYTES$1 & 1ll) == 0ll) goto label$1466;
	{
		char* vr$11 = HGETREGNAME( 2ll, REG$1 );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$11, 0ll, 0 );
		HPREPOPERAND( SVREG$1, &SRC$1, 2ll, OFS$1, 0ll, -1ll );
		HMOV( (char*)*(char**)&AUX$1, (char*)*(char**)&SRC$1 );
		HPREPOPERAND( DVREG$1, &DST$1, 2ll, OFS$1, 0ll, -1ll );
		HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX$1 );
	}
	label$1466:;
	label$1465:;
	if( ISFREE$1 != 0ll) goto label$1468;
	{
		HPOP( (char*)*(char**)&AUX$1 );
	}
	label$1468:;
	label$1467:;
	label$1454:;
}

static void _EMITMEMMOVE( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, int64 BYTES$1, int64 EXTRA$1 )
{
	label$1469:;
	if( (int64)(uint32)BYTES$1 <= 16ll) goto label$1472;
	{
		HMEMMOVEREP( DVREG$1, SVREG$1, BYTES$1 );
	}
	goto label$1471;
	label$1472:;
	{
		HMEMMOVEBLK( DVREG$1, SVREG$1, BYTES$1 );
	}
	label$1471:;
	label$1470:;
}

static void _EMITMEMSWAP( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, int64 BYTES$1, int64 EXTRA$1 )
{
	label$1473:;
	label$1474:;
}

static void HMEMFILLREPIMM( struct $6IRVREG* DVREG$1, uint32 BYTES$1, int32 FILLCHAR$1 )
{
	label$1475:;
	static FBSTRING DST$1;
	static FBSTRING OSTR$1;
	static int64 EAXFREE$1;
	static int64 ECXFREE$1;
	static int64 EDIFREE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	int64 vr$0 = HISREGFREE( 0ll, 5ll );
	EAXFREE$1 = vr$0;
	int64 vr$1 = HISREGFREE( 0ll, 3ll );
	ECXFREE$1 = vr$1;
	int64 vr$2 = HISREGFREE( 0ll, 1ll );
	EDIFREE$1 = vr$2;
	if( EAXFREE$1 != 0ll) goto label$1478;
	{
		HPUSH( (char*)"eax" );
	}
	label$1478:;
	label$1477:;
	if( ECXFREE$1 != 0ll) goto label$1480;
	{
		HPUSH( (char*)"ecx" );
	}
	label$1480:;
	label$1479:;
	if( EDIFREE$1 != 0ll) goto label$1482;
	{
		HPUSH( (char*)"edi" );
	}
	label$1482:;
	label$1481:;
	if( *(int64*)DVREG$1 == 4ll) goto label$1484;
	{
		HMOV( (char*)"edi", (char*)*(char**)&DST$1 );
	}
	goto label$1483;
	label$1484:;
	{
		if( *(int64*)((uint8*)DVREG$1 + 24ll) == 1ll) goto label$1486;
		{
			HMOV( (char*)"edi", (char*)*(char**)&DST$1 );
		}
		label$1486:;
		label$1485:;
	}
	label$1483:;
	if( (int64)FILLCHAR$1 != 0ll) goto label$1488;
	{
		OUTP( (char*)"xor eax, eax" );
	}
	goto label$1487;
	label$1488:;
	{
		FBSTRING TMP$842$2;
		FBSTRING TMP$843$2;
		FILLCHAR$1 = (int32)((int64)FILLCHAR$1 & 255ll);
		FILLCHAR$1 = (int32)((int64)FILLCHAR$1 | ((int64)FILLCHAR$1 << (8ll & 63ll)));
		FILLCHAR$1 = (int32)((int64)FILLCHAR$1 | ((int64)FILLCHAR$1 << (16ll & 63ll)));
		__builtin_memset( &TMP$843$2, 0, 24ll );
		FBSTRING* vr$20 = fb_IntToStr( FILLCHAR$1 );
		__builtin_memset( &TMP$842$2, 0, 24ll );
		FBSTRING* vr$23 = fb_StrConcat( &TMP$842$2, (void*)"mov eax, ", 10ll, (void*)vr$20, -1ll );
		fb_StrAssign( (void*)&TMP$843$2, -1ll, (void*)vr$23, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$843$2 );
		fb_StrDelete( (FBSTRING*)&TMP$843$2 );
	}
	label$1487:;
	if( (int64)BYTES$1 <= 4ll) goto label$1490;
	{
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"mov ecx, ", 10ll, 0 );
		FBSTRING* vr$29 = fb_LongintToStr( (int64)BYTES$1 / 4ll );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$29, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		OUTP( (char*)"rep stosd" );
	}
	goto label$1489;
	label$1490:;
	if( (int64)BYTES$1 != 4ll) goto label$1491;
	{
		OUTP( (char*)"mov dword ptr [edi], eax" );
		if( ((int64)BYTES$1 & 3ll) <= 0ll) goto label$1493;
		{
			OUTP( (char*)"add edi, 4" );
		}
		label$1493:;
		label$1492:;
	}
	label$1491:;
	label$1489:;
	BYTES$1 = (uint32)((int64)BYTES$1 & 3ll);
	if( (int64)BYTES$1 <= 0ll) goto label$1495;
	{
		if( (int64)BYTES$1 < 2ll) goto label$1497;
		{
			OUTP( (char*)"mov word ptr [edi], ax" );
			if( (int64)BYTES$1 != 3ll) goto label$1499;
			{
				OUTP( (char*)"add edi, 2" );
			}
			label$1499:;
			label$1498:;
		}
		label$1497:;
		label$1496:;
		if( ((int64)BYTES$1 & 1ll) == 0ll) goto label$1501;
		{
			OUTP( (char*)"mov byte ptr [edi], al" );
		}
		label$1501:;
		label$1500:;
	}
	label$1495:;
	label$1494:;
	if( EDIFREE$1 != 0ll) goto label$1503;
	{
		HPOP( (char*)"edi" );
	}
	label$1503:;
	label$1502:;
	if( ECXFREE$1 != 0ll) goto label$1505;
	{
		HPOP( (char*)"ecx" );
	}
	label$1505:;
	label$1504:;
	if( EAXFREE$1 != 0ll) goto label$1507;
	{
		HPOP( (char*)"eax" );
	}
	label$1507:;
	label$1506:;
	label$1476:;
}

static void HMEMFILLBLKIMM( struct $6IRVREG* DVREG$1, uint32 BYTES$1, int32 FILLCHAR$1 )
{
	label$1508:;
	static FBSTRING DST$1;
	static int64 I$1;
	static int64 OFS$1;
	static int32 FILL1$1;
	static int32 FILL2$1;
	static int32 FILL4$1;
	FILL1$1 = (int32)((int64)FILLCHAR$1 & 255ll);
	FILL2$1 = (int32)(((int64)FILL1$1 << (8ll & 63ll)) | (int64)FILL1$1);
	FILL4$1 = (int32)(((int64)FILL2$1 << (16ll & 63ll)) | (int64)FILL2$1);
	OFS$1 = 0ll;
	{
		I$1 = 1ll;
		int64 TMP$847$2;
		TMP$847$2 = (int64)BYTES$1 / 4ll;
		goto label$1510;
		label$1513:;
		{
			FBSTRING TMP$848$3;
			HPREPOPERAND( DVREG$1, &DST$1, 8ll, OFS$1, 0ll, -1ll );
			__builtin_memset( &TMP$848$3, 0, 24ll );
			FBSTRING* vr$16 = fb_IntToStr( FILL4$1 );
			fb_StrAssign( (void*)&TMP$848$3, -1ll, (void*)vr$16, -1ll, 0 );
			HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&TMP$848$3 );
			fb_StrDelete( (FBSTRING*)&TMP$848$3 );
			OFS$1 = OFS$1 + 4ll;
		}
		label$1511:;
		I$1 = I$1 + 1ll;
		label$1510:;
		if( I$1 <= TMP$847$2) goto label$1513;
		label$1512:;
	}
	if( ((int64)BYTES$1 & 2ll) == 0ll) goto label$1515;
	{
		FBSTRING TMP$849$2;
		HPREPOPERAND( DVREG$1, &DST$1, 5ll, OFS$1, 0ll, -1ll );
		__builtin_memset( &TMP$849$2, 0, 24ll );
		FBSTRING* vr$24 = fb_IntToStr( FILL2$1 );
		fb_StrAssign( (void*)&TMP$849$2, -1ll, (void*)vr$24, -1ll, 0 );
		HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&TMP$849$2 );
		fb_StrDelete( (FBSTRING*)&TMP$849$2 );
		OFS$1 = OFS$1 + 2ll;
	}
	label$1515:;
	label$1514:;
	if( ((int64)BYTES$1 & 1ll) == 0ll) goto label$1517;
	{
		FBSTRING TMP$850$2;
		HPREPOPERAND( DVREG$1, &DST$1, 2ll, OFS$1, 0ll, -1ll );
		__builtin_memset( &TMP$850$2, 0, 24ll );
		FBSTRING* vr$31 = fb_IntToStr( FILL1$1 );
		fb_StrAssign( (void*)&TMP$850$2, -1ll, (void*)vr$31, -1ll, 0 );
		HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&TMP$850$2 );
		fb_StrDelete( (FBSTRING*)&TMP$850$2 );
	}
	label$1517:;
	label$1516:;
	label$1509:;
}

static void HMEMFILL( struct $6IRVREG* DVREG$1, struct $6IRVREG* BYTES_VREG$1, int64 FILLCHAR$1 )
{
	label$1518:;
	static FBSTRING DST$1;
	static FBSTRING BYTES$1;
	static FBSTRING OSTR$1;
	static int64 EAXFREE$1;
	static int64 ECXFREE$1;
	static int64 EDIFREE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( BYTES_VREG$1, &BYTES$1, 2147483648ll, 0ll, 0ll, -1ll );
	int64 vr$0 = HISREGFREE( 0ll, 5ll );
	EAXFREE$1 = vr$0;
	int64 vr$1 = HISREGFREE( 0ll, 3ll );
	ECXFREE$1 = vr$1;
	int64 vr$2 = HISREGFREE( 0ll, 1ll );
	EDIFREE$1 = vr$2;
	if( EAXFREE$1 != 0ll) goto label$1521;
	{
		HPUSH( (char*)"eax" );
	}
	label$1521:;
	label$1520:;
	if( ECXFREE$1 != 0ll) goto label$1523;
	{
		HPUSH( (char*)"ecx" );
	}
	label$1523:;
	label$1522:;
	if( EDIFREE$1 != 0ll) goto label$1525;
	{
		HPUSH( (char*)"edi" );
	}
	label$1525:;
	label$1524:;
	int64 vr$3 = HISREGINVREG( BYTES_VREG$1, 1ll );
	if( vr$3 != 0ll) goto label$1527;
	{
		if( *(int64*)DVREG$1 == 4ll) goto label$1529;
		{
			HMOV( (char*)"edi", (char*)*(char**)&DST$1 );
		}
		goto label$1528;
		label$1529:;
		{
			if( *(int64*)((uint8*)DVREG$1 + 24ll) == 1ll) goto label$1531;
			{
				HMOV( (char*)"edi", (char*)*(char**)&DST$1 );
			}
			label$1531:;
			label$1530:;
		}
		label$1528:;
		if( *(int64*)BYTES_VREG$1 == 4ll) goto label$1533;
		{
			HMOV( (char*)"ecx", (char*)*(char**)&BYTES$1 );
		}
		goto label$1532;
		label$1533:;
		{
			if( *(int64*)((uint8*)BYTES_VREG$1 + 24ll) == 3ll) goto label$1535;
			{
				HMOV( (char*)"ecx", (char*)*(char**)&BYTES$1 );
			}
			label$1535:;
			label$1534:;
		}
		label$1532:;
	}
	goto label$1526;
	label$1527:;
	{
		HPUSH( (char*)*(char**)&BYTES$1 );
		fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"lea edi, ", 10ll, 0 );
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)&DST$1, -1ll, 0 );
		OUTP( (char*)*(char**)&OSTR$1 );
		HPOP( (char*)"ecx" );
	}
	label$1526:;
	if( FILLCHAR$1 != 0ll) goto label$1537;
	{
		OUTP( (char*)"xor eax, eax" );
	}
	goto label$1536;
	label$1537:;
	{
		FBSTRING TMP$852$2;
		FBSTRING TMP$853$2;
		FILLCHAR$1 = FILLCHAR$1 & 255ll;
		FILLCHAR$1 = FILLCHAR$1 | (FILLCHAR$1 << (8ll & 63ll));
		FILLCHAR$1 = FILLCHAR$1 | (FILLCHAR$1 << (16ll & 63ll));
		__builtin_memset( &TMP$853$2, 0, 24ll );
		FBSTRING* vr$14 = fb_LongintToStr( FILLCHAR$1 );
		__builtin_memset( &TMP$852$2, 0, 24ll );
		FBSTRING* vr$17 = fb_StrConcat( &TMP$852$2, (void*)"mov eax, ", 10ll, (void*)vr$14, -1ll );
		fb_StrAssign( (void*)&TMP$853$2, -1ll, (void*)vr$17, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$853$2 );
		fb_StrDelete( (FBSTRING*)&TMP$853$2 );
	}
	label$1536:;
	OUTP( (char*)"push ecx" );
	OUTP( (char*)"shr ecx, 2" );
	OUTP( (char*)"rep stosd" );
	OUTP( (char*)"pop ecx" );
	OUTP( (char*)"and ecx, 3" );
	OUTP( (char*)"rep stosb" );
	if( EDIFREE$1 != 0ll) goto label$1539;
	{
		HPOP( (char*)"edi" );
	}
	label$1539:;
	label$1538:;
	if( ECXFREE$1 != 0ll) goto label$1541;
	{
		HPOP( (char*)"ecx" );
	}
	label$1541:;
	label$1540:;
	if( EAXFREE$1 != 0ll) goto label$1543;
	{
		HPOP( (char*)"eax" );
	}
	label$1543:;
	label$1542:;
	label$1519:;
}

static void _EMITMEMFILL( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, int64 BYTES$1, int64 FILLCHAR$1 )
{
	label$1544:;
	if( *(int64*)SVREG$1 != 0ll) goto label$1547;
	{
		uint32 BYTES$2;
		BYTES$2 = (uint32)*(int64*)((uint8*)SVREG$1 + 48ll);
		if( (int64)BYTES$2 <= 16ll) goto label$1549;
		{
			HMEMFILLREPIMM( DVREG$1, BYTES$2, (int32)FILLCHAR$1 );
		}
		goto label$1548;
		label$1549:;
		{
			HMEMFILLBLKIMM( DVREG$1, BYTES$2, (int32)FILLCHAR$1 );
		}
		label$1548:;
	}
	goto label$1546;
	label$1547:;
	{
		HMEMFILL( DVREG$1, SVREG$1, FILLCHAR$1 );
	}
	label$1546:;
	label$1545:;
}

static void _EMITSTKCLEAR( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1, int64 BYTES$1, int64 BASEOFS$1 )
{
	label$1550:;
	HCLEARLOCALS( BYTES$1, BASEOFS$1 );
	label$1551:;
}

static void _EMITLINEINI( struct $8FBSYMBOL* PROC$1, int64 LNUM$1, int64 POS_$1, char* FILENAME$1 )
{
	label$1552:;
	EDBGLINEBEGIN( PROC$1, LNUM$1, POS_$1, FILENAME$1 );
	label$1553:;
}

static void _EMITLINEEND( struct $8FBSYMBOL* PROC$1, int64 LNUM$1, int64 POS_$1, char* FILENAME$1 )
{
	label$1554:;
	EDBGLINEEND( PROC$1, LNUM$1, POS_$1 );
	label$1555:;
}

static void _EMITSCOPEINI( struct $8FBSYMBOL* SYM$1, int64 LNUM$1, int64 POS_$1, char* FILENAME$1 )
{
	label$1556:;
	EDBGEMITSCOPEINI( SYM$1 );
	label$1557:;
}

static void _EMITSCOPEEND( struct $8FBSYMBOL* SYM$1, int64 LNUM$1, int64 POS_$1, char* FILENAME$1 )
{
	label$1558:;
	EDBGEMITSCOPEEND( SYM$1 );
	label$1559:;
}

static void _EMITLOADB2B( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1560:;
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&SRC$1 );
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	label$1561:;
}

static void _EMITSTORB2B( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1562:;
	_EMITLOADB2B( DVREG$1, SVREG$1 );
	label$1563:;
}

static void _EMITLOADB2I( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$861$1;
	int64 TMP$862$1;
	FBSTRING TMP$867$1;
	FBSTRING TMP$868$1;
	label$1564:;
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( *(int64*)SVREG$1 != 0ll) goto label$1567;
	{
		if( *(int64*)((uint8*)SVREG$1 + 48ll) == 0ll) goto label$1569;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)"-1" );
		}
		goto label$1568;
		label$1569:;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)"0" );
		}
		label$1568:;
		fb_StrDelete( (FBSTRING*)&DST$1 );
		fb_StrDelete( (FBSTRING*)&SRC$1 );
		goto label$1565;
	}
	label$1567:;
	label$1566:;
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1570;
	TMP$861$1 = 24ll;
	goto label$1940;
	label$1570:;
	TMP$861$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1940:;
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$1571;
	TMP$862$1 = 24ll;
	goto label$1941;
	label$1571:;
	TMP$862$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1941:;
	if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$861$1 * 56ll)) + 8ll) <= *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$862$1 * 56ll)) + 8ll)) goto label$1573;
	{
		FBSTRING TMP$863$2;
		FBSTRING TMP$864$2;
		FBSTRING TMP$865$2;
		FBSTRING TMP$866$2;
		__builtin_memset( &TMP$866$2, 0, 24ll );
		__builtin_memset( &TMP$863$2, 0, 24ll );
		FBSTRING* vr$23 = fb_StrConcat( &TMP$863$2, (void*)"movzx ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$864$2, 0, 24ll );
		FBSTRING* vr$26 = fb_StrConcat( &TMP$864$2, (void*)vr$23, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$865$2, 0, 24ll );
		FBSTRING* vr$29 = fb_StrConcat( &TMP$865$2, (void*)vr$26, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$866$2, -1ll, (void*)vr$29, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$866$2 );
		fb_StrDelete( (FBSTRING*)&TMP$866$2 );
	}
	goto label$1572;
	label$1573:;
	{
		HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&SRC$1 );
	}
	label$1572:;
	__builtin_memset( &TMP$868$1, 0, 24ll );
	__builtin_memset( &TMP$867$1, 0, 24ll );
	FBSTRING* vr$36 = fb_StrConcat( &TMP$867$1, (void*)"neg ", 5ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$868$1, -1ll, (void*)vr$36, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$868$1 );
	fb_StrDelete( (FBSTRING*)&TMP$868$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	label$1565:;
}

static void _EMITSTORB2I( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1574:;
	_EMITLOADB2I( DVREG$1, SVREG$1 );
	label$1575:;
}

static void _EMITLOADI2B( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1576:;
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	FBSTRING DST8$1;
	__builtin_memset( &DST8$1, 0, 24ll );
	int64 DDSIZE$1;
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	DDSIZE$1 = *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + 64ll);
	if( *(int64*)SVREG$1 != 0ll) goto label$1579;
	{
		if( *(int64*)((uint8*)SVREG$1 + 48ll) == 0ll) goto label$1581;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)"1" );
		}
		goto label$1580;
		label$1581:;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)"0" );
		}
		label$1580:;
	}
	goto label$1578;
	label$1579:;
	if( DDSIZE$1 != 1ll) goto label$1582;
	{
		FBSTRING TMP$869$2;
		FBSTRING TMP$870$2;
		FBSTRING TMP$871$2;
		FBSTRING TMP$873$2;
		FBSTRING TMP$874$2;
		__builtin_memset( &TMP$871$2, 0, 24ll );
		__builtin_memset( &TMP$869$2, 0, 24ll );
		FBSTRING* vr$11 = fb_StrConcat( &TMP$869$2, (void*)"cmp ", 5ll, (void*)&SRC$1, -1ll );
		__builtin_memset( &TMP$870$2, 0, 24ll );
		FBSTRING* vr$14 = fb_StrConcat( &TMP$870$2, (void*)vr$11, -1ll, (void*)", 0", 4ll );
		fb_StrAssign( (void*)&TMP$871$2, -1ll, (void*)vr$14, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$871$2 );
		fb_StrDelete( (FBSTRING*)&TMP$871$2 );
		__builtin_memset( &TMP$874$2, 0, 24ll );
		__builtin_memset( &TMP$873$2, 0, 24ll );
		FBSTRING* vr$21 = fb_StrConcat( &TMP$873$2, (void*)"setne ", 7ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$874$2, -1ll, (void*)vr$21, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$874$2 );
		fb_StrDelete( (FBSTRING*)&TMP$874$2 );
	}
	goto label$1578;
	label$1582:;
	if( (((int64)-(*(int64*)DVREG$1 == 4ll) & (int64)-(*(int64*)((uint8*)DVREG$1 + 24ll) != 2ll)) & (int64)-(*(int64*)((uint8*)DVREG$1 + 24ll) != 1ll)) == 0ll) goto label$1583;
	{
		FBSTRING TMP$875$2;
		FBSTRING TMP$876$2;
		FBSTRING TMP$877$2;
		FBSTRING TMP$878$2;
		FBSTRING TMP$879$2;
		FBSTRING TMP$880$2;
		FBSTRING TMP$881$2;
		FBSTRING TMP$882$2;
		FBSTRING TMP$883$2;
		char* vr$33 = HGETREGNAME( 2ll, *(int64*)((uint8*)DVREG$1 + 24ll) );
		fb_StrAssign( (void*)&DST8$1, -1ll, (void*)vr$33, 0ll, 0 );
		__builtin_memset( &TMP$877$2, 0, 24ll );
		__builtin_memset( &TMP$875$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$875$2, (void*)"cmp ", 5ll, (void*)&SRC$1, -1ll );
		__builtin_memset( &TMP$876$2, 0, 24ll );
		FBSTRING* vr$42 = fb_StrConcat( &TMP$876$2, (void*)vr$39, -1ll, (void*)", 0", 4ll );
		fb_StrAssign( (void*)&TMP$877$2, -1ll, (void*)vr$42, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$877$2 );
		fb_StrDelete( (FBSTRING*)&TMP$877$2 );
		__builtin_memset( &TMP$879$2, 0, 24ll );
		__builtin_memset( &TMP$878$2, 0, 24ll );
		FBSTRING* vr$49 = fb_StrConcat( &TMP$878$2, (void*)"setne ", 7ll, (void*)&DST8$1, -1ll );
		fb_StrAssign( (void*)&TMP$879$2, -1ll, (void*)vr$49, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$879$2 );
		fb_StrDelete( (FBSTRING*)&TMP$879$2 );
		__builtin_memset( &TMP$883$2, 0, 24ll );
		__builtin_memset( &TMP$880$2, 0, 24ll );
		FBSTRING* vr$57 = fb_StrConcat( &TMP$880$2, (void*)"movzx ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$881$2, 0, 24ll );
		FBSTRING* vr$60 = fb_StrConcat( &TMP$881$2, (void*)vr$57, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$882$2, 0, 24ll );
		FBSTRING* vr$63 = fb_StrConcat( &TMP$882$2, (void*)vr$60, -1ll, (void*)&DST8$1, -1ll );
		fb_StrAssign( (void*)&TMP$883$2, -1ll, (void*)vr$63, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$883$2 );
		fb_StrDelete( (FBSTRING*)&TMP$883$2 );
	}
	goto label$1578;
	label$1583:;
	{
		FBSTRING TMP$884$2;
		FBSTRING TMP$885$2;
		FBSTRING TMP$886$2;
		FBSTRING TMP$887$2;
		FBSTRING TMP$888$2;
		FBSTRING TMP$889$2;
		FBSTRING AUX$2;
		__builtin_memset( &AUX$2, 0, 24ll );
		FBSTRING AUX8$2;
		__builtin_memset( &AUX8$2, 0, 24ll );
		int64 REG$2;
		__builtin_memset( &REG$2, 0, 8ll );
		int64 ISFREE$2;
		__builtin_memset( &ISFREE$2, 0, 8ll );
		int64 vr$70 = HFINDREGNOTINVREG( DVREG$1, -1ll );
		REG$2 = vr$70;
		char* vr$71 = HGETREGNAME( 2ll, REG$2 );
		fb_StrAssign( (void*)&AUX8$2, -1ll, (void*)vr$71, 0ll, 0 );
		char* vr$74 = HGETREGNAME( *(int64*)((uint8*)DVREG$1 + 8ll), REG$2 );
		fb_StrAssign( (void*)&AUX$2, -1ll, (void*)vr$74, 0ll, 0 );
		int64 vr$76 = HISREGFREE( 0ll, REG$2 );
		ISFREE$2 = vr$76;
		if( ISFREE$2 != 0ll) goto label$1585;
		{
			HPUSH( (char*)*(char**)&AUX$2 );
		}
		label$1585:;
		label$1584:;
		__builtin_memset( &TMP$887$2, 0, 24ll );
		__builtin_memset( &TMP$884$2, 0, 24ll );
		FBSTRING* vr$81 = fb_StrConcat( &TMP$884$2, (void*)"cmp ", 5ll, (void*)&SRC$1, -1ll );
		__builtin_memset( &TMP$885$2, 0, 24ll );
		FBSTRING* vr$84 = fb_StrConcat( &TMP$885$2, (void*)vr$81, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$886$2, 0, 24ll );
		FBSTRING* vr$87 = fb_StrConcat( &TMP$886$2, (void*)vr$84, -1ll, (void*)"0", 2ll );
		fb_StrAssign( (void*)&TMP$887$2, -1ll, (void*)vr$87, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$887$2 );
		fb_StrDelete( (FBSTRING*)&TMP$887$2 );
		__builtin_memset( &TMP$889$2, 0, 24ll );
		__builtin_memset( &TMP$888$2, 0, 24ll );
		FBSTRING* vr$94 = fb_StrConcat( &TMP$888$2, (void*)"setne ", 7ll, (void*)&AUX8$2, -1ll );
		fb_StrAssign( (void*)&TMP$889$2, -1ll, (void*)vr$94, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$889$2 );
		fb_StrDelete( (FBSTRING*)&TMP$889$2 );
		if( *(int64*)DVREG$1 != 4ll) goto label$1587;
		{
			FBSTRING TMP$890$3;
			FBSTRING TMP$891$3;
			FBSTRING TMP$892$3;
			FBSTRING TMP$893$3;
			__builtin_memset( &TMP$893$3, 0, 24ll );
			__builtin_memset( &TMP$890$3, 0, 24ll );
			FBSTRING* vr$103 = fb_StrConcat( &TMP$890$3, (void*)"movzx ", 7ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$891$3, 0, 24ll );
			FBSTRING* vr$106 = fb_StrConcat( &TMP$891$3, (void*)vr$103, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$892$3, 0, 24ll );
			FBSTRING* vr$109 = fb_StrConcat( &TMP$892$3, (void*)vr$106, -1ll, (void*)&AUX8$2, -1ll );
			fb_StrAssign( (void*)&TMP$893$3, -1ll, (void*)vr$109, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$893$3 );
			fb_StrDelete( (FBSTRING*)&TMP$893$3 );
		}
		goto label$1586;
		label$1587:;
		{
			FBSTRING TMP$894$3;
			FBSTRING TMP$895$3;
			FBSTRING TMP$896$3;
			FBSTRING TMP$897$3;
			FBSTRING TMP$898$3;
			FBSTRING TMP$899$3;
			FBSTRING TMP$900$3;
			FBSTRING TMP$901$3;
			__builtin_memset( &TMP$897$3, 0, 24ll );
			__builtin_memset( &TMP$894$3, 0, 24ll );
			FBSTRING* vr$117 = fb_StrConcat( &TMP$894$3, (void*)"movzx ", 7ll, (void*)&AUX$2, -1ll );
			__builtin_memset( &TMP$895$3, 0, 24ll );
			FBSTRING* vr$120 = fb_StrConcat( &TMP$895$3, (void*)vr$117, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$896$3, 0, 24ll );
			FBSTRING* vr$123 = fb_StrConcat( &TMP$896$3, (void*)vr$120, -1ll, (void*)&AUX8$2, -1ll );
			fb_StrAssign( (void*)&TMP$897$3, -1ll, (void*)vr$123, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$897$3 );
			fb_StrDelete( (FBSTRING*)&TMP$897$3 );
			__builtin_memset( &TMP$901$3, 0, 24ll );
			__builtin_memset( &TMP$898$3, 0, 24ll );
			FBSTRING* vr$131 = fb_StrConcat( &TMP$898$3, (void*)"mov ", 5ll, (void*)&DST$1, -1ll );
			__builtin_memset( &TMP$899$3, 0, 24ll );
			FBSTRING* vr$134 = fb_StrConcat( &TMP$899$3, (void*)vr$131, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$900$3, 0, 24ll );
			FBSTRING* vr$137 = fb_StrConcat( &TMP$900$3, (void*)vr$134, -1ll, (void*)&AUX$2, -1ll );
			fb_StrAssign( (void*)&TMP$901$3, -1ll, (void*)vr$137, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$901$3 );
			fb_StrDelete( (FBSTRING*)&TMP$901$3 );
		}
		label$1586:;
		if( ISFREE$2 != 0ll) goto label$1589;
		{
			HPOP( (char*)*(char**)&AUX$2 );
		}
		label$1589:;
		label$1588:;
		fb_StrDelete( (FBSTRING*)&AUX8$2 );
		fb_StrDelete( (FBSTRING*)&AUX$2 );
	}
	label$1578:;
	fb_StrDelete( (FBSTRING*)&DST8$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	label$1577:;
}

static void _EMITSTORI2B( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1590:;
	_EMITLOADI2B( DVREG$1, SVREG$1 );
	label$1591:;
}

static void _EMITLOADF2B( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$902$1;
	int64 TMP$903$1;
	label$1592:;
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	int64 DDSIZE$1;
	int64 ISFREE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)DVREG$1 + 8ll) & 480ll) == 0ll) goto label$1594;
	TMP$902$1 = 24ll;
	goto label$1942;
	label$1594:;
	TMP$902$1 = *(int64*)((uint8*)DVREG$1 + 8ll) & 31ll;
	label$1942:;
	DDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$902$1 * 56ll)) + 8ll);
	int64 vr$9 = HISREGFREE( 0ll, 5ll );
	ISFREE$1 = vr$9;
	if( ISFREE$1 != 0ll) goto label$1595;
	int64 vr$10 = HISREGINVREG( DVREG$1, 5ll );
	TMP$903$1 = (int64)-(vr$10 != 0ll);
	goto label$1943;
	label$1595:;
	TMP$903$1 = -1ll;
	label$1943:;
	ISFREE$1 = TMP$903$1;
	if( *(int64*)SVREG$1 == 4ll) goto label$1597;
	{
		FBSTRING TMP$904$2;
		FBSTRING TMP$905$2;
		__builtin_memset( &TMP$905$2, 0, 24ll );
		__builtin_memset( &TMP$904$2, 0, 24ll );
		FBSTRING* vr$17 = fb_StrConcat( &TMP$904$2, (void*)"fld ", 5ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$905$2, -1ll, (void*)vr$17, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$905$2 );
		fb_StrDelete( (FBSTRING*)&TMP$905$2 );
	}
	label$1597:;
	label$1596:;
	if( ISFREE$1 != 0ll) goto label$1599;
	{
		OUTP( (char*)"push eax" );
	}
	label$1599:;
	label$1598:;
	if( *(int64*)((uint8*)&ENV$ + 224ll) < 3ll) goto label$1601;
	{
		OUTP( (char*)"fldz" );
		OUTP( (char*)"fcomip" );
	}
	goto label$1600;
	label$1601:;
	{
		OUTP( (char*)"ftst" );
		OUTP( (char*)"fnstsw ax" );
		OUTP( (char*)"sahf" );
	}
	label$1600:;
	if( *(int64*)((uint8*)&ENV$ + 240ll) != 0ll) goto label$1603;
	{
		OUTP( (char*)"setp ah" );
		OUTP( (char*)"setnz al" );
		OUTP( (char*)"or al, ah" );
	}
	goto label$1602;
	label$1603:;
	{
		OUTP( (char*)"setnz al" );
	}
	label$1602:;
	OUTP( (char*)"fstp st(0)" );
	if( DDSIZE$1 != 1ll) goto label$1605;
	{
		FBSTRING TMP$911$2;
		FBSTRING TMP$912$2;
		FBSTRING TMP$913$2;
		__builtin_memset( &TMP$913$2, 0, 24ll );
		__builtin_memset( &TMP$911$2, 0, 24ll );
		FBSTRING* vr$24 = fb_StrConcat( &TMP$911$2, (void*)"mov ", 5ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$912$2, 0, 24ll );
		FBSTRING* vr$27 = fb_StrConcat( &TMP$912$2, (void*)vr$24, -1ll, (void*)", al", 5ll );
		fb_StrAssign( (void*)&TMP$913$2, -1ll, (void*)vr$27, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$913$2 );
		fb_StrDelete( (FBSTRING*)&TMP$913$2 );
	}
	goto label$1604;
	label$1605:;
	{
		FBSTRING TMP$914$2;
		FBSTRING TMP$915$2;
		FBSTRING TMP$916$2;
		__builtin_memset( &TMP$916$2, 0, 24ll );
		__builtin_memset( &TMP$914$2, 0, 24ll );
		FBSTRING* vr$34 = fb_StrConcat( &TMP$914$2, (void*)"movzx ", 7ll, (void*)&DST$1, -1ll );
		__builtin_memset( &TMP$915$2, 0, 24ll );
		FBSTRING* vr$37 = fb_StrConcat( &TMP$915$2, (void*)vr$34, -1ll, (void*)", al", 5ll );
		fb_StrAssign( (void*)&TMP$916$2, -1ll, (void*)vr$37, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$916$2 );
		fb_StrDelete( (FBSTRING*)&TMP$916$2 );
	}
	label$1604:;
	if( ISFREE$1 != 0ll) goto label$1607;
	{
		OUTP( (char*)"pop eax" );
	}
	label$1607:;
	label$1606:;
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	label$1593:;
}

static void _EMITSTORF2B( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1608:;
	_EMITLOADF2B( DVREG$1, SVREG$1 );
	label$1609:;
}

static void _EMITLOADB2F( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$918$1;
	label$1610:;
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	int64 SDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$1612;
	TMP$918$1 = 24ll;
	goto label$1944;
	label$1612:;
	TMP$918$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1944:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$918$1 * 56ll)) + 8ll);
	if( *(int64*)SVREG$1 != 0ll) goto label$1614;
	{
		if( *(int64*)((uint8*)SVREG$1 + 48ll) == 0ll) goto label$1616;
		{
			OUTP( (char*)"fld1" );
			OUTP( (char*)"fchs" );
		}
		goto label$1615;
		label$1616:;
		{
			OUTP( (char*)"fldz" );
		}
		label$1615:;
		fb_StrDelete( (FBSTRING*)&DST$1 );
		fb_StrDelete( (FBSTRING*)&SRC$1 );
		goto label$1611;
	}
	label$1614:;
	label$1613:;
	if( SDSIZE$1 != 1ll) goto label$1618;
	{
		FBSTRING TMP$919$2;
		FBSTRING TMP$920$2;
		FBSTRING TMP$921$2;
		FBSTRING TMP$922$2;
		FBSTRING AUX$2;
		__builtin_memset( &AUX$2, 0, 24ll );
		int64 ISFREE$2;
		__builtin_memset( &ISFREE$2, 0, 8ll );
		int64 REG$2;
		__builtin_memset( &REG$2, 0, 8ll );
		int64 vr$16 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$2 = vr$16;
		char* vr$17 = HGETREGNAME( 8ll, REG$2 );
		fb_StrAssign( (void*)&AUX$2, -1ll, (void*)vr$17, 0ll, 0 );
		int64 vr$19 = HISREGFREE( 0ll, REG$2 );
		ISFREE$2 = vr$19;
		if( ISFREE$2 != 0ll) goto label$1620;
		{
			HPUSH( (char*)*(char**)&AUX$2 );
		}
		label$1620:;
		label$1619:;
		__builtin_memset( &TMP$922$2, 0, 24ll );
		__builtin_memset( &TMP$919$2, 0, 24ll );
		FBSTRING* vr$25 = fb_StrConcat( &TMP$919$2, (void*)"movzx ", 7ll, (void*)&AUX$2, -1ll );
		__builtin_memset( &TMP$920$2, 0, 24ll );
		FBSTRING* vr$28 = fb_StrConcat( &TMP$920$2, (void*)vr$25, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$921$2, 0, 24ll );
		FBSTRING* vr$31 = fb_StrConcat( &TMP$921$2, (void*)vr$28, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$922$2, -1ll, (void*)vr$31, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$922$2 );
		fb_StrDelete( (FBSTRING*)&TMP$922$2 );
		HPUSH( (char*)*(char**)&AUX$2 );
		OUTP( (char*)"fild dword ptr [esp]" );
		OUTP( (char*)"add esp, 4" );
		if( ISFREE$2 != 0ll) goto label$1622;
		{
			HPOP( (char*)*(char**)&AUX$2 );
		}
		label$1622:;
		label$1621:;
		fb_StrDelete( (FBSTRING*)&AUX$2 );
	}
	goto label$1617;
	label$1618:;
	{
		FBSTRING TMP$923$2;
		FBSTRING TMP$924$2;
		__builtin_memset( &TMP$924$2, 0, 24ll );
		__builtin_memset( &TMP$923$2, 0, 24ll );
		FBSTRING* vr$39 = fb_StrConcat( &TMP$923$2, (void*)"fild ", 6ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$924$2, -1ll, (void*)vr$39, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$924$2 );
		fb_StrDelete( (FBSTRING*)&TMP$924$2 );
	}
	label$1617:;
	OUTP( (char*)"fchs" );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	label$1611:;
}

static void _EMITSTORB2F( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	int64 TMP$925$1;
	FBSTRING TMP$934$1;
	FBSTRING TMP$935$1;
	label$1623:;
	FBSTRING SRC$1;
	__builtin_memset( &SRC$1, 0, 24ll );
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	int64 SDSIZE$1;
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	HPREPOPERAND( SVREG$1, &SRC$1, 2147483648ll, 0ll, 0ll, -1ll );
	if( (*(int64*)((uint8*)SVREG$1 + 8ll) & 480ll) == 0ll) goto label$1625;
	TMP$925$1 = 24ll;
	goto label$1945;
	label$1625:;
	TMP$925$1 = *(int64*)((uint8*)SVREG$1 + 8ll) & 31ll;
	label$1945:;
	SDSIZE$1 = *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$925$1 * 56ll)) + 8ll);
	if( *(int64*)SVREG$1 != 0ll) goto label$1627;
	{
		FBSTRING TMP$926$2;
		FBSTRING TMP$927$2;
		if( *(int64*)((uint8*)SVREG$1 + 48ll) == 0ll) goto label$1629;
		{
			OUTP( (char*)"fld1" );
			OUTP( (char*)"fchs" );
		}
		goto label$1628;
		label$1629:;
		{
			OUTP( (char*)"fldz" );
		}
		label$1628:;
		__builtin_memset( &TMP$927$2, 0, 24ll );
		__builtin_memset( &TMP$926$2, 0, 24ll );
		FBSTRING* vr$15 = fb_StrConcat( &TMP$926$2, (void*)"fstp ", 6ll, (void*)&DST$1, -1ll );
		fb_StrAssign( (void*)&TMP$927$2, -1ll, (void*)vr$15, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$927$2 );
		fb_StrDelete( (FBSTRING*)&TMP$927$2 );
		fb_StrDelete( (FBSTRING*)&DST$1 );
		fb_StrDelete( (FBSTRING*)&SRC$1 );
		goto label$1624;
	}
	label$1627:;
	label$1626:;
	if( SDSIZE$1 != 1ll) goto label$1631;
	{
		FBSTRING TMP$928$2;
		FBSTRING TMP$929$2;
		FBSTRING TMP$930$2;
		FBSTRING TMP$931$2;
		FBSTRING AUX$2;
		__builtin_memset( &AUX$2, 0, 24ll );
		int64 ISFREE$2;
		__builtin_memset( &ISFREE$2, 0, 8ll );
		int64 REG$2;
		__builtin_memset( &REG$2, 0, 8ll );
		int64 vr$23 = HFINDREGNOTINVREG( SVREG$1, 0ll );
		REG$2 = vr$23;
		char* vr$24 = HGETREGNAME( 8ll, REG$2 );
		fb_StrAssign( (void*)&AUX$2, -1ll, (void*)vr$24, 0ll, 0 );
		int64 vr$26 = HISREGFREE( 0ll, REG$2 );
		ISFREE$2 = vr$26;
		if( ISFREE$2 != 0ll) goto label$1633;
		{
			HPUSH( (char*)*(char**)&AUX$2 );
		}
		label$1633:;
		label$1632:;
		__builtin_memset( &TMP$931$2, 0, 24ll );
		__builtin_memset( &TMP$928$2, 0, 24ll );
		FBSTRING* vr$32 = fb_StrConcat( &TMP$928$2, (void*)"movzx ", 7ll, (void*)&AUX$2, -1ll );
		__builtin_memset( &TMP$929$2, 0, 24ll );
		FBSTRING* vr$35 = fb_StrConcat( &TMP$929$2, (void*)vr$32, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$930$2, 0, 24ll );
		FBSTRING* vr$38 = fb_StrConcat( &TMP$930$2, (void*)vr$35, -1ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$931$2, -1ll, (void*)vr$38, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$931$2 );
		fb_StrDelete( (FBSTRING*)&TMP$931$2 );
		HPUSH( (char*)*(char**)&AUX$2 );
		OUTP( (char*)"fild dword ptr [esp]" );
		OUTP( (char*)"add esp, 4" );
		if( ISFREE$2 != 0ll) goto label$1635;
		{
			HPOP( (char*)*(char**)&AUX$2 );
		}
		label$1635:;
		label$1634:;
		fb_StrDelete( (FBSTRING*)&AUX$2 );
	}
	goto label$1630;
	label$1631:;
	{
		FBSTRING TMP$932$2;
		FBSTRING TMP$933$2;
		__builtin_memset( &TMP$933$2, 0, 24ll );
		__builtin_memset( &TMP$932$2, 0, 24ll );
		FBSTRING* vr$46 = fb_StrConcat( &TMP$932$2, (void*)"fild ", 6ll, (void*)&SRC$1, -1ll );
		fb_StrAssign( (void*)&TMP$933$2, -1ll, (void*)vr$46, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$933$2 );
		fb_StrDelete( (FBSTRING*)&TMP$933$2 );
	}
	label$1630:;
	OUTP( (char*)"fchs" );
	__builtin_memset( &TMP$935$1, 0, 24ll );
	__builtin_memset( &TMP$934$1, 0, 24ll );
	FBSTRING* vr$53 = fb_StrConcat( &TMP$934$1, (void*)"fstp ", 6ll, (void*)&DST$1, -1ll );
	fb_StrAssign( (void*)&TMP$935$1, -1ll, (void*)vr$53, -1ll, 0 );
	OUTP( (char*)*(char**)&TMP$935$1 );
	fb_StrDelete( (FBSTRING*)&TMP$935$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	fb_StrDelete( (FBSTRING*)&SRC$1 );
	label$1624:;
}

static void _EMITLOADB2L( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1636:;
	FBSTRING DST1$1;
	__builtin_memset( &DST1$1, 0, 24ll );
	FBSTRING DST2$1;
	__builtin_memset( &DST2$1, 0, 24ll );
	HPREPOPERAND64( DVREG$1, &DST1$1, &DST2$1 );
	if( *(int64*)SVREG$1 != 0ll) goto label$1639;
	{
		if( *(int64*)((uint8*)SVREG$1 + 48ll) == 0ll) goto label$1641;
		{
			HMOV( (char*)*(char**)&DST1$1, (char*)"-1" );
			HMOV( (char*)*(char**)&DST2$1, (char*)"-1" );
		}
		goto label$1640;
		label$1641:;
		{
			HMOV( (char*)*(char**)&DST1$1, (char*)"0" );
			HMOV( (char*)*(char**)&DST2$1, (char*)"0" );
		}
		label$1640:;
	}
	goto label$1638;
	label$1639:;
	{
		FBSTRING TMP$936$2;
		FBSTRING TMP$937$2;
		FBSTRING TMP$938$2;
		FBSTRING TMP$939$2;
		FBSTRING TMP$940$2;
		FBSTRING TMP$941$2;
		FBSTRING SRC$2;
		__builtin_memset( &SRC$2, 0, 24ll );
		HPREPOPERAND( SVREG$1, &SRC$2, 2147483648ll, 0ll, 0ll, -1ll );
		__builtin_memset( &TMP$939$2, 0, 24ll );
		__builtin_memset( &TMP$936$2, 0, 24ll );
		FBSTRING* vr$13 = fb_StrConcat( &TMP$936$2, (void*)"movzx ", 7ll, (void*)&DST1$1, -1ll );
		__builtin_memset( &TMP$937$2, 0, 24ll );
		FBSTRING* vr$16 = fb_StrConcat( &TMP$937$2, (void*)vr$13, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$938$2, 0, 24ll );
		FBSTRING* vr$19 = fb_StrConcat( &TMP$938$2, (void*)vr$16, -1ll, (void*)&SRC$2, -1ll );
		fb_StrAssign( (void*)&TMP$939$2, -1ll, (void*)vr$19, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$939$2 );
		fb_StrDelete( (FBSTRING*)&TMP$939$2 );
		__builtin_memset( &TMP$941$2, 0, 24ll );
		__builtin_memset( &TMP$940$2, 0, 24ll );
		FBSTRING* vr$26 = fb_StrConcat( &TMP$940$2, (void*)"neg ", 5ll, (void*)&DST1$1, -1ll );
		fb_StrAssign( (void*)&TMP$941$2, -1ll, (void*)vr$26, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$941$2 );
		fb_StrDelete( (FBSTRING*)&TMP$941$2 );
		HMOV( (char*)*(char**)&DST2$1, (char*)*(char**)&DST1$1 );
		fb_StrDelete( (FBSTRING*)&SRC$2 );
	}
	label$1638:;
	fb_StrDelete( (FBSTRING*)&DST2$1 );
	fb_StrDelete( (FBSTRING*)&DST1$1 );
	label$1637:;
}

static void _EMITSTORB2L( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1642:;
	_EMITLOADB2L( DVREG$1, SVREG$1 );
	label$1643:;
}

static void _EMITLOADL2B( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1644:;
	FBSTRING DST$1;
	__builtin_memset( &DST$1, 0, 24ll );
	int64 DDSIZE$1;
	__builtin_memset( &DDSIZE$1, 0, 8ll );
	FBSTRING SRC1$1;
	__builtin_memset( &SRC1$1, 0, 24ll );
	FBSTRING SRC2$1;
	__builtin_memset( &SRC2$1, 0, 24ll );
	FBSTRING AUX$1;
	__builtin_memset( &AUX$1, 0, 24ll );
	FBSTRING AUX8$1;
	__builtin_memset( &AUX8$1, 0, 24ll );
	HPREPOPERAND64( SVREG$1, &SRC1$1, &SRC2$1 );
	HPREPOPERAND( DVREG$1, &DST$1, 2147483648ll, 0ll, 0ll, -1ll );
	DDSIZE$1 = *(int64*)((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + 64ll);
	if( *(int64*)SVREG$1 != 0ll) goto label$1647;
	{
		if( *(int64*)((uint8*)SVREG$1 + 48ll) == 0ll) goto label$1649;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)"1" );
		}
		goto label$1648;
		label$1649:;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)"0" );
		}
		label$1648:;
	}
	goto label$1646;
	label$1647:;
	if( (((int64)-(*(int64*)DVREG$1 == 4ll) & (int64)-(*(int64*)((uint8*)DVREG$1 + 24ll) != 2ll)) & (int64)-(*(int64*)((uint8*)DVREG$1 + 24ll) != 1ll)) == 0ll) goto label$1650;
	{
		FBSTRING TMP$942$2;
		FBSTRING TMP$943$2;
		FBSTRING TMP$944$2;
		FBSTRING TMP$945$2;
		FBSTRING TMP$946$2;
		FBSTRING TMP$947$2;
		FBSTRING TMP$948$2;
		FBSTRING TMP$949$2;
		FBSTRING TMP$950$2;
		char* vr$20 = HGETREGNAME( 2ll, *(int64*)((uint8*)DVREG$1 + 24ll) );
		fb_StrAssign( (void*)&AUX8$1, -1ll, (void*)vr$20, 0ll, 0 );
		char* vr$23 = HGETREGNAME( 8ll, *(int64*)((uint8*)DVREG$1 + 24ll) );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$23, 0ll, 0 );
		HMOV( (char*)*(char**)&AUX$1, (char*)*(char**)&SRC1$1 );
		__builtin_memset( &TMP$945$2, 0, 24ll );
		__builtin_memset( &TMP$942$2, 0, 24ll );
		FBSTRING* vr$30 = fb_StrConcat( &TMP$942$2, (void*)"or ", 4ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$943$2, 0, 24ll );
		FBSTRING* vr$33 = fb_StrConcat( &TMP$943$2, (void*)vr$30, -1ll, (void*)", ", 3ll );
		__builtin_memset( &TMP$944$2, 0, 24ll );
		FBSTRING* vr$36 = fb_StrConcat( &TMP$944$2, (void*)vr$33, -1ll, (void*)&SRC2$1, -1ll );
		fb_StrAssign( (void*)&TMP$945$2, -1ll, (void*)vr$36, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$945$2 );
		fb_StrDelete( (FBSTRING*)&TMP$945$2 );
		__builtin_memset( &TMP$948$2, 0, 24ll );
		__builtin_memset( &TMP$946$2, 0, 24ll );
		FBSTRING* vr$43 = fb_StrConcat( &TMP$946$2, (void*)"cmp ", 5ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$947$2, 0, 24ll );
		FBSTRING* vr$46 = fb_StrConcat( &TMP$947$2, (void*)vr$43, -1ll, (void*)", 0", 4ll );
		fb_StrAssign( (void*)&TMP$948$2, -1ll, (void*)vr$46, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$948$2 );
		fb_StrDelete( (FBSTRING*)&TMP$948$2 );
		__builtin_memset( &TMP$950$2, 0, 24ll );
		__builtin_memset( &TMP$949$2, 0, 24ll );
		FBSTRING* vr$53 = fb_StrConcat( &TMP$949$2, (void*)"setne ", 7ll, (void*)&AUX8$1, -1ll );
		fb_StrAssign( (void*)&TMP$950$2, -1ll, (void*)vr$53, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$950$2 );
		fb_StrDelete( (FBSTRING*)&TMP$950$2 );
		if( DDSIZE$1 == 1ll) goto label$1652;
		{
			FBSTRING TMP$951$3;
			FBSTRING TMP$952$3;
			FBSTRING TMP$953$3;
			FBSTRING TMP$954$3;
			__builtin_memset( &TMP$954$3, 0, 24ll );
			__builtin_memset( &TMP$951$3, 0, 24ll );
			FBSTRING* vr$61 = fb_StrConcat( &TMP$951$3, (void*)"movzx ", 7ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$952$3, 0, 24ll );
			FBSTRING* vr$64 = fb_StrConcat( &TMP$952$3, (void*)vr$61, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$953$3, 0, 24ll );
			FBSTRING* vr$67 = fb_StrConcat( &TMP$953$3, (void*)vr$64, -1ll, (void*)&AUX8$1, -1ll );
			fb_StrAssign( (void*)&TMP$954$3, -1ll, (void*)vr$67, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$954$3 );
			fb_StrDelete( (FBSTRING*)&TMP$954$3 );
		}
		label$1652:;
		label$1651:;
	}
	goto label$1646;
	label$1650:;
	{
		FBSTRING TMP$971$2;
		FBSTRING TMP$972$2;
		FBSTRING TMP$973$2;
		FBSTRING TMP$974$2;
		FBSTRING TMP$975$2;
		int64 REG$2;
		__builtin_memset( &REG$2, 0, 8ll );
		int64 ISFREE$2;
		ISFREE$2 = 0ll;
		int64 vr$71 = HFINDREGNOTINVREG( DVREG$1, -1ll );
		REG$2 = vr$71;
		char* vr$72 = HGETREGNAME( 2ll, REG$2 );
		fb_StrAssign( (void*)&AUX8$1, -1ll, (void*)vr$72, 0ll, 0 );
		char* vr$74 = HGETREGNAME( 8ll, REG$2 );
		fb_StrAssign( (void*)&AUX$1, -1ll, (void*)vr$74, 0ll, 0 );
		int64 vr$76 = HISREGFREE( 0ll, REG$2 );
		ISFREE$2 = vr$76;
		if( ISFREE$2 != 0ll) goto label$1654;
		{
			HPUSH( (char*)*(char**)&AUX$1 );
		}
		label$1654:;
		label$1653:;
		if( *(int64*)SVREG$1 != 4ll) goto label$1656;
		{
			if( REG$2 != *(int64*)((uint8*)SVREG$1 + 24ll)) goto label$1658;
			{
				FBSTRING TMP$955$4;
				FBSTRING TMP$956$4;
				FBSTRING TMP$957$4;
				FBSTRING TMP$958$4;
				__builtin_memset( &TMP$958$4, 0, 24ll );
				__builtin_memset( &TMP$955$4, 0, 24ll );
				FBSTRING* vr$84 = fb_StrConcat( &TMP$955$4, (void*)"or ", 4ll, (void*)&AUX$1, -1ll );
				__builtin_memset( &TMP$956$4, 0, 24ll );
				FBSTRING* vr$87 = fb_StrConcat( &TMP$956$4, (void*)vr$84, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$957$4, 0, 24ll );
				FBSTRING* vr$90 = fb_StrConcat( &TMP$957$4, (void*)vr$87, -1ll, (void*)&SRC2$1, -1ll );
				fb_StrAssign( (void*)&TMP$958$4, -1ll, (void*)vr$90, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$958$4 );
				fb_StrDelete( (FBSTRING*)&TMP$958$4 );
			}
			goto label$1657;
			label$1658:;
			if( REG$2 != *(int64*)((uint8*)*(struct $6IRVREG**)((uint8*)SVREG$1 + 88ll) + 24ll)) goto label$1659;
			{
				FBSTRING TMP$959$4;
				FBSTRING TMP$960$4;
				FBSTRING TMP$961$4;
				FBSTRING TMP$962$4;
				__builtin_memset( &TMP$962$4, 0, 24ll );
				__builtin_memset( &TMP$959$4, 0, 24ll );
				FBSTRING* vr$100 = fb_StrConcat( &TMP$959$4, (void*)"or ", 4ll, (void*)&AUX$1, -1ll );
				__builtin_memset( &TMP$960$4, 0, 24ll );
				FBSTRING* vr$103 = fb_StrConcat( &TMP$960$4, (void*)vr$100, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$961$4, 0, 24ll );
				FBSTRING* vr$106 = fb_StrConcat( &TMP$961$4, (void*)vr$103, -1ll, (void*)&SRC1$1, -1ll );
				fb_StrAssign( (void*)&TMP$962$4, -1ll, (void*)vr$106, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$962$4 );
				fb_StrDelete( (FBSTRING*)&TMP$962$4 );
			}
			goto label$1657;
			label$1659:;
			{
				FBSTRING TMP$963$4;
				FBSTRING TMP$964$4;
				FBSTRING TMP$965$4;
				FBSTRING TMP$966$4;
				HMOV( (char*)*(char**)&AUX$1, (char*)*(char**)&SRC1$1 );
				__builtin_memset( &TMP$966$4, 0, 24ll );
				__builtin_memset( &TMP$963$4, 0, 24ll );
				FBSTRING* vr$114 = fb_StrConcat( &TMP$963$4, (void*)"or ", 4ll, (void*)&AUX$1, -1ll );
				__builtin_memset( &TMP$964$4, 0, 24ll );
				FBSTRING* vr$117 = fb_StrConcat( &TMP$964$4, (void*)vr$114, -1ll, (void*)", ", 3ll );
				__builtin_memset( &TMP$965$4, 0, 24ll );
				FBSTRING* vr$120 = fb_StrConcat( &TMP$965$4, (void*)vr$117, -1ll, (void*)&SRC2$1, -1ll );
				fb_StrAssign( (void*)&TMP$966$4, -1ll, (void*)vr$120, -1ll, 0 );
				OUTP( (char*)*(char**)&TMP$966$4 );
				fb_StrDelete( (FBSTRING*)&TMP$966$4 );
			}
			label$1657:;
		}
		goto label$1655;
		label$1656:;
		{
			FBSTRING TMP$967$3;
			FBSTRING TMP$968$3;
			FBSTRING TMP$969$3;
			FBSTRING TMP$970$3;
			HMOV( (char*)*(char**)&AUX$1, (char*)*(char**)&SRC1$1 );
			__builtin_memset( &TMP$970$3, 0, 24ll );
			__builtin_memset( &TMP$967$3, 0, 24ll );
			FBSTRING* vr$128 = fb_StrConcat( &TMP$967$3, (void*)"or ", 4ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$968$3, 0, 24ll );
			FBSTRING* vr$131 = fb_StrConcat( &TMP$968$3, (void*)vr$128, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$969$3, 0, 24ll );
			FBSTRING* vr$134 = fb_StrConcat( &TMP$969$3, (void*)vr$131, -1ll, (void*)&SRC2$1, -1ll );
			fb_StrAssign( (void*)&TMP$970$3, -1ll, (void*)vr$134, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$970$3 );
			fb_StrDelete( (FBSTRING*)&TMP$970$3 );
		}
		label$1655:;
		__builtin_memset( &TMP$973$2, 0, 24ll );
		__builtin_memset( &TMP$971$2, 0, 24ll );
		FBSTRING* vr$141 = fb_StrConcat( &TMP$971$2, (void*)"cmp ", 5ll, (void*)&AUX$1, -1ll );
		__builtin_memset( &TMP$972$2, 0, 24ll );
		FBSTRING* vr$144 = fb_StrConcat( &TMP$972$2, (void*)vr$141, -1ll, (void*)", 0", 4ll );
		fb_StrAssign( (void*)&TMP$973$2, -1ll, (void*)vr$144, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$973$2 );
		fb_StrDelete( (FBSTRING*)&TMP$973$2 );
		__builtin_memset( &TMP$975$2, 0, 24ll );
		__builtin_memset( &TMP$974$2, 0, 24ll );
		FBSTRING* vr$151 = fb_StrConcat( &TMP$974$2, (void*)"setne ", 7ll, (void*)&AUX8$1, -1ll );
		fb_StrAssign( (void*)&TMP$975$2, -1ll, (void*)vr$151, -1ll, 0 );
		OUTP( (char*)*(char**)&TMP$975$2 );
		fb_StrDelete( (FBSTRING*)&TMP$975$2 );
		if( DDSIZE$1 != 1ll) goto label$1661;
		{
			HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX8$1 );
		}
		goto label$1660;
		label$1661:;
		{
			FBSTRING TMP$976$3;
			FBSTRING TMP$977$3;
			FBSTRING TMP$978$3;
			FBSTRING TMP$979$3;
			__builtin_memset( &TMP$979$3, 0, 24ll );
			__builtin_memset( &TMP$976$3, 0, 24ll );
			FBSTRING* vr$159 = fb_StrConcat( &TMP$976$3, (void*)"movzx ", 7ll, (void*)&AUX$1, -1ll );
			__builtin_memset( &TMP$977$3, 0, 24ll );
			FBSTRING* vr$162 = fb_StrConcat( &TMP$977$3, (void*)vr$159, -1ll, (void*)", ", 3ll );
			__builtin_memset( &TMP$978$3, 0, 24ll );
			FBSTRING* vr$165 = fb_StrConcat( &TMP$978$3, (void*)vr$162, -1ll, (void*)&AUX8$1, -1ll );
			fb_StrAssign( (void*)&TMP$979$3, -1ll, (void*)vr$165, -1ll, 0 );
			OUTP( (char*)*(char**)&TMP$979$3 );
			fb_StrDelete( (FBSTRING*)&TMP$979$3 );
			HMOV( (char*)*(char**)&DST$1, (char*)*(char**)&AUX$1 );
		}
		label$1660:;
		if( ISFREE$2 != 0ll) goto label$1663;
		{
			HPOP( (char*)*(char**)&AUX$1 );
		}
		label$1663:;
		label$1662:;
	}
	label$1646:;
	fb_StrDelete( (FBSTRING*)&AUX8$1 );
	fb_StrDelete( (FBSTRING*)&AUX$1 );
	fb_StrDelete( (FBSTRING*)&SRC2$1 );
	fb_StrDelete( (FBSTRING*)&SRC1$1 );
	fb_StrDelete( (FBSTRING*)&DST$1 );
	label$1645:;
}

static void _EMITSTORL2B( struct $6IRVREG* DVREG$1, struct $6IRVREG* SVREG$1 )
{
	label$1664:;
	_EMITLOADL2B( DVREG$1, SVREG$1 );
	label$1665:;
}

static int64 _INIT( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1700:;
	HINITREGTB(  );
	__builtin_memcpy( (struct $12EMITDATATYPE*)((uint8*)DTYPETB$ + 168ll), (struct $12EMITDATATYPE*)((int64)(struct $12EMITDATATYPE*)DTYPETB$ + (*(int64*)((uint8*)&ENV$ + 576ll) * 24ll)), 24 );
	*(int64*)((uint8*)&EMIT$ + 280ll) = -1ll;
	*(int64*)((uint8*)&EMIT$ + 288ll) = -1ll;
	uint64 IROPTIONS$1;
	IROPTIONS$1 = 66304ull;
	if( *(int64*)((uint8*)&ENV$ + 232ll) != 1ll) goto label$1703;
	{
		IROPTIONS$1 = IROPTIONS$1 | 1ull;
	}
	label$1703:;
	label$1702:;
	*($6IR_OPT*)((uint8*)&IR$ + 544ll) = ($6IR_OPT)(*(uint64*)((uint8*)&IR$ + 544ll) | IROPTIONS$1);
	EDBGINIT(  );
	fb$result$1 = -1ll;
	label$1701:;
	return fb$result$1;
}

static void _END( void )
{
	label$1704:;
	*(int64*)((uint8*)&EMIT$ + 280ll) = -1ll;
	*(int64*)((uint8*)&EMIT$ + 288ll) = -1ll;
	HENDREGTB(  );
	label$1705:;
}

static int64 _GETOPTIONVALUE( $14IR_OPTIONVALUE OPT$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1706:;
	{
		if( OPT$1 != 1ll) goto label$1709;
		label$1710:;
		{
			fb$result$1 = 16ll;
			goto label$1707;
		}
		goto label$1708;
		label$1709:;
		{
			ERRREPORTEX( 21ll, (char*)"_GETOPTIONVALUE", 0ll, 1ll, (char*)0ull );
		}
		label$1711:;
		label$1708:;
	}
	label$1707:;
	return fb$result$1;
}

static int64 _OPEN( void )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1712:;
	int64 vr$2 = HFILEEXISTS( (char*)((uint8*)&ENV$ + 936ll) );
	if( vr$2 == 0ll) goto label$1715;
	{
		FBSTRING* vr$4 = fb_StrAllocTempDescZ( (char*)((uint8*)&ENV$ + 936ll) );
		fb_FileKill( (FBSTRING*)vr$4 );
	}
	label$1715:;
	label$1714:;
	int32 vr$5 = fb_FileFree(  );
	*(int64*)((uint8*)&ENV$ + 928ll) = (int64)vr$5;
	FBSTRING* vr$9 = fb_StrAllocTempDescZ( (char*)((uint8*)&ENV$ + 936ll) );
	int32 vr$10 = fb_FileOpen( (FBSTRING*)vr$9, 0u, 3u, 0u, (int32)*(int64*)((uint8*)&ENV$ + 928ll), 0 );
	if( (int64)vr$10 == 0ll) goto label$1717;
	{
		fb$result$1 = 0ll;
		goto label$1713;
	}
	label$1717:;
	label$1716:;
	HWRITEHEADER(  );
	fb$result$1 = -1ll;
	label$1713:;
	return fb$result$1;
}

static void _CLOSE( void )
{
	label$1718:;
	EDBGINCLUDE( (char*)0ull );
	SYMBFOREACHGLOBAL( 1ll, (tmp$43)&HDECLVARIABLE );
	if( (*(int64*)((uint8*)&ENV$ + 456ll) & (*(int64*)((uint8*)&ENV$ + 600ll) & 4ll)) == 0ll) goto label$1721;
	{
		SYMBFOREACHGLOBAL( 3ll, (tmp$43)&HEMITEXPORT );
	}
	label$1721:;
	label$1720:;
	HWRITECTOR( *(struct $20FB_GLOBCTORLIST_ITEM**)((uint8*)&SYMB$ + 198048ll), -1ll );
	HWRITECTOR( *(struct $20FB_GLOBCTORLIST_ITEM**)((uint8*)&SYMB$ + 198128ll), 0ll );
	EDBGEMITFOOTER(  );
	int32 vr$3 = fb_FileClose( (int32)*(int64*)((uint8*)&ENV$ + 928ll) );
	if( (int64)vr$3 == 0ll) goto label$1723;
	{
	}
	label$1723:;
	label$1722:;
	*(int64*)((uint8*)&ENV$ + 928ll) = 0ll;
	label$1719:;
}

static char* _PROCGETFRAMEREGNAME( void )
{
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1724:;
	static char SNAME$1[4] = "ebp";
	fb$result$1 = (char*)SNAME$1;
	label$1725:;
	return fb$result$1;
}

static int64 _ISREGPRESERVED( int64 DCLASS$1, int64 REG$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1726:;
	if( DCLASS$1 != 1ll) goto label$1729;
	{
		fb$result$1 = 0ll;
		goto label$1727;
	}
	label$1729:;
	label$1728:;
	{
		uint64 TMP$1004$2;
		TMP$1004$2 = (uint64)REG$1;
		goto label$1731;
		label$1732:;
		{
			fb$result$1 = 0ll;
			goto label$1727;
		}
		goto label$1730;
		label$1733:;
		{
			fb$result$1 = -1ll;
			goto label$1727;
		}
		goto label$1730;
		label$1731:;
		static const void* tmp$1059[6ll] = {
			&&label$1732,
			&&label$1733,
			&&label$1733,
			&&label$1732,
			&&label$1733,
			&&label$1732,
		};
		if( TMP$1004$2 > 5ull ) goto label$1733;
		goto *tmp$1059[TMP$1004$2 - 0ull];
		label$1730:;
	}
	label$1727:;
	return fb$result$1;
}

static void _GETARGREG( int64 DTYPE$1, int64 DCLASS$1, int64 REGNUM$1, int64* R1$1 )
{
	label$1734:;
	{
		if( REGNUM$1 != 1ll) goto label$1737;
		label$1738:;
		{
			*R1$1 = 3ll;
		}
		goto label$1736;
		label$1737:;
		if( REGNUM$1 != 2ll) goto label$1739;
		label$1740:;
		{
			*R1$1 = 0ll;
		}
		goto label$1736;
		label$1739:;
		{
			*R1$1 = -1ll;
		}
		label$1741:;
		label$1736:;
	}
	label$1735:;
}

static void _GETRESULTREG( int64 DTYPE$1, int64 DCLASS$1, int64* R1$1, int64* R2$1 )
{
	label$1742:;
	if( DCLASS$1 != 0ll) goto label$1745;
	{
		int64 TMP$1005$2;
		int64 TMP$1006$2;
		*R1$1 = 5ll;
		if( (DTYPE$1 & 480ll) == 0ll) goto label$1746;
		TMP$1005$2 = 24ll;
		goto label$1946;
		label$1746:;
		TMP$1005$2 = DTYPE$1 & 31ll;
		label$1946:;
		if( (DTYPE$1 & 480ll) == 0ll) goto label$1747;
		TMP$1006$2 = 24ll;
		goto label$1947;
		label$1747:;
		TMP$1006$2 = DTYPE$1 & 31ll;
		label$1947:;
		if( ((int64)-(TMP$1005$2 == 13ll) | (int64)-(TMP$1006$2 == 14ll)) == 0ll) goto label$1749;
		{
			*R2$1 = 0ll;
		}
		goto label$1748;
		label$1749:;
		{
			*R2$1 = -1ll;
		}
		label$1748:;
	}
	goto label$1744;
	label$1745:;
	{
		*R1$1 = 0ll;
		*R2$1 = -1ll;
	}
	label$1744:;
	label$1743:;
}

static int64 _GETFREEPRESERVEDREG( int64 DCLASS$1, int64 DTYPE$1 )
{
	int64 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1750:;
	fb$result$1 = -1ll;
	if( DCLASS$1 != 1ll) goto label$1753;
	{
		goto label$1751;
	}
	label$1753:;
	label$1752:;
	int64 vr$3 = (*(tmp$88*)((uint8*)*(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 16ll) + 32ll))( *(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 16ll), 4ll );
	if( vr$3 == 0ll) goto label$1755;
	{
		fb$result$1 = 4ll;
	}
	goto label$1754;
	label$1755:;
	int64 vr$9 = (*(tmp$88*)((uint8*)*(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 16ll) + 32ll))( *(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 16ll), 2ll );
	if( vr$9 == 0ll) goto label$1756;
	{
		int64 TMP$1007$2;
		if( (DTYPE$1 & 480ll) == 0ll) goto label$1757;
		TMP$1007$2 = 24ll;
		goto label$1948;
		label$1757:;
		TMP$1007$2 = DTYPE$1 & 31ll;
		label$1948:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1007$2 * 56ll)) + 8ll) == 1ll) goto label$1759;
		{
			fb$result$1 = 2ll;
		}
		label$1759:;
		label$1758:;
	}
	goto label$1754;
	label$1756:;
	int64 vr$18 = (*(tmp$88*)((uint8*)*(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 16ll) + 32ll))( *(struct $8REGCLASS**)((uint8*)((uint8*)&EMIT$ + (DCLASS$1 << (3ll & 63ll))) + 16ll), 1ll );
	if( vr$18 == 0ll) goto label$1760;
	{
		int64 TMP$1008$2;
		if( (DTYPE$1 & 480ll) == 0ll) goto label$1761;
		TMP$1008$2 = 24ll;
		goto label$1949;
		label$1761:;
		TMP$1008$2 = DTYPE$1 & 31ll;
		label$1949:;
		if( *(int64*)(((int64)(struct $13SYMB_DATATYPE*)SYMB_DTYPETB$ + (TMP$1008$2 * 56ll)) + 8ll) == 1ll) goto label$1763;
		{
			fb$result$1 = 1ll;
		}
		label$1763:;
		label$1762:;
	}
	label$1760:;
	label$1754:;
	label$1751:;
	return fb$result$1;
}

static void _PROCBEGIN( struct $8FBSYMBOL* PROC$1 )
{
	label$1764:;
	*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 16ll) = 0ll;
	*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 24ll) = 0ll;
	*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 8ll) = 8ll;
	EDBGPROCBEGIN( PROC$1 );
	label$1765:;
}

static void _PROCEND( struct $8FBSYMBOL* PROC$1 )
{
	label$1766:;
	EDBGPROCEND( PROC$1 );
	label$1767:;
}

static void _PROCALLOCSTATICVARS( struct $8FBSYMBOL* S$1 )
{
	label$1768:;
	label$1770:;
	if( S$1 == (struct $8FBSYMBOL*)0ull) goto label$1771;
	{
		{
			$12FB_SYMBCLASS TMP$1009$3;
			TMP$1009$3 = *($12FB_SYMBCLASS*)S$1;
			if( TMP$1009$3 != 15ll) goto label$1773;
			label$1774:;
			{
				_PROCALLOCSTATICVARS( *(struct $8FBSYMBOL**)((uint8*)S$1 + 112ll) );
			}
			goto label$1772;
			label$1773:;
			if( TMP$1009$3 != 1ll) goto label$1775;
			label$1776:;
			{
				if( (*(int64*)((uint8*)S$1 + 8ll) & 2ll) == 0ll) goto label$1778;
				{
					HDECLVARIABLE( S$1 );
				}
				label$1778:;
				label$1777:;
			}
			label$1775:;
			label$1772:;
		}
		S$1 = *(struct $8FBSYMBOL**)((uint8*)S$1 + 312ll);
	}
	goto label$1770;
	label$1771:;
	label$1769:;
}

static void _PROCALLOCLOCAL( struct $8FBSYMBOL* PROC$1, struct $8FBSYMBOL* SYM$1 )
{
	label$1779:;
	int64 OFS$1;
	int64 LGT$1;
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 4ll) == 0ll) goto label$1782;
	{
		goto label$1780;
	}
	label$1782:;
	label$1781:;
	int64 vr$2 = SYMBGETREALSIZE( SYM$1 );
	LGT$1 = vr$2;
	*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 16ll) = *(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 16ll) + ((LGT$1 + 3ll) & -4ll);
	OFS$1 = -(*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 16ll));
	if( -OFS$1 <= *(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 24ll)) goto label$1784;
	{
		*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 24ll) = -OFS$1;
	}
	label$1784:;
	label$1783:;
	*(int64*)((uint8*)SYM$1 + 88ll) = OFS$1;
	label$1780:;
}

static void _PROCALLOCARG( struct $8FBSYMBOL* PROC$1, struct $8FBSYMBOL* SYM$1 )
{
	label$1785:;
	int64 LGT$1;
	if( (*(int64*)((uint8*)SYM$1 + 8ll) & 32768ll) == 0ll) goto label$1788;
	{
		LGT$1 = *(int64*)((uint8*)SYM$1 + 80ll);
	}
	goto label$1787;
	label$1788:;
	{
		LGT$1 = *(int64*)((uint8*)&ENV$ + 624ll);
	}
	label$1787:;
	{
		$11FB_FUNCMODE TMP$1010$2;
		TMP$1010$2 = *($11FB_FUNCMODE*)((uint8*)PROC$1 + 152ll);
		if( TMP$1010$2 == 5ll) goto label$1791;
		label$1792:;
		if( TMP$1010$2 != 6ll) goto label$1790;
		label$1791:;
		{
			struct $8FBSYMBOL* PARAM$3;
			PARAM$3 = *(struct $8FBSYMBOL**)((uint8*)PROC$1 + 136ll);
			label$1793:;
			if( PARAM$3 == (struct $8FBSYMBOL*)0ull) goto label$1794;
			{
				if( *(int64*)((uint8*)PARAM$3 + 136ll) == 0ll) goto label$1796;
				{
					if( *(struct $8FBSYMBOL**)((uint8*)PARAM$3 + 104ll) != SYM$1) goto label$1798;
					{
						_PROCALLOCLOCAL( PROC$1, SYM$1 );
						goto label$1786;
					}
					label$1798:;
					label$1797:;
				}
				label$1796:;
				label$1795:;
				PARAM$3 = *(struct $8FBSYMBOL**)((uint8*)PARAM$3 + 312ll);
			}
			goto label$1793;
			label$1794:;
		}
		label$1790:;
		label$1789:;
	}
	*(int64*)((uint8*)SYM$1 + 88ll) = *(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 8ll);
	*(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 8ll) = *(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 8ll) + ((LGT$1 + 3ll) & -4ll);
	label$1786:;
}

static void _PROCHEADER( struct $8FBSYMBOL* PROC$1, struct $8FBSYMBOL* INITLABEL$1 )
{
	label$1799:;
	EMITRESET(  );
	EDBGPROCEMITBEGIN( PROC$1 );
	label$1800:;
}

static void _PROCFOOTER( struct $8FBSYMBOL* PROC$1, int64 BYTESTOPOP$1, struct $8FBSYMBOL* INITLABEL$1, struct $8FBSYMBOL* EXITLABEL$1 )
{
	int64 TMP$1011$1;
	int64 TMP$1020$1;
	label$1801:;
	int64 OLDPOS$1;
	int64 ISPUBLIC$1;
	char* MANGLEDNAME$1;
	char* vr$0 = SYMBGETMANGLEDNAME( PROC$1 );
	MANGLEDNAME$1 = vr$0;
	struct $10FB_PROCEXT* PROCEXT$1;
	PROCEXT$1 = *(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll);
	int64 ISLINUX$1;
	int64 HASUNWIND$1;
	ISLINUX$1 = (int64)-(*(int64*)((uint8*)&ENV$ + 216ll) == 2ll);
	if( ISLINUX$1 == 0ll) goto label$1803;
	int64 vr$3 = FBGETOPTION( 21ll );
	TMP$1011$1 = (int64)-(vr$3 != 0ll);
	goto label$1950;
	label$1803:;
	TMP$1011$1 = 0ll;
	label$1950:;
	HASUNWIND$1 = TMP$1011$1;
	ISPUBLIC$1 = (int64)-((*(int64*)((uint8*)PROC$1 + 8ll) & 32ll) != 0ll);
	_SETSECTION( 3ll, 0ll );
	EDBGEMITPROCHEADER( PROC$1 );
	HALIGN( 16ll );
	if( ISPUBLIC$1 == 0ll) goto label$1805;
	{
		HPUBLIC( MANGLEDNAME$1, (int64)-((*(int64*)((uint8*)PROC$1 + 8ll) & 256ll) != 0ll) );
	}
	label$1805:;
	label$1804:;
	HLABEL( MANGLEDNAME$1 );
	if( ISLINUX$1 == 0ll) goto label$1807;
	{
		FBSTRING TMP$1014$2;
		FBSTRING TMP$1015$2;
		FBSTRING TMP$1016$2;
		FBSTRING TMP$1017$2;
		__builtin_memset( &TMP$1017$2, 0, 24ll );
		__builtin_memset( &TMP$1014$2, 0, 24ll );
		FBSTRING* vr$14 = fb_StrConcat( &TMP$1014$2, (void*)".type ", 7ll, (void*)MANGLEDNAME$1, 0ll );
		__builtin_memset( &TMP$1015$2, 0, 24ll );
		FBSTRING* vr$17 = fb_StrConcat( &TMP$1015$2, (void*)vr$14, -1ll, (void*)", @function", 12ll );
		__builtin_memset( &TMP$1016$2, 0, 24ll );
		FBSTRING* vr$20 = fb_StrConcat( &TMP$1016$2, (void*)vr$17, -1ll, (void*)"\x0A", 2ll );
		fb_StrAssign( (void*)&TMP$1017$2, -1ll, (void*)vr$20, -1ll, 0 );
		OUTEX( (char*)*(char**)&TMP$1017$2, 0ll );
		fb_StrDelete( (FBSTRING*)&TMP$1017$2 );
		if( HASUNWIND$1 == 0ll) goto label$1809;
		{
			OUTEX( (char*)".cfi_startproc\x0A", 0ll );
		}
		label$1809:;
		label$1808:;
	}
	label$1807:;
	label$1806:;
	HCREATEFRAME( PROC$1 );
	EDBGEMITLINEFLUSH( PROC$1, *(int64*)((uint8*)PROCEXT$1 + 32ll), PROC$1 );
	EMITFLUSH(  );
	HDESTROYFRAME( PROC$1, BYTESTOPOP$1 );
	if( ISLINUX$1 == 0ll) goto label$1810;
	TMP$1020$1 = (int64)-(HASUNWIND$1 != 0ll);
	goto label$1951;
	label$1810:;
	TMP$1020$1 = 0ll;
	label$1951:;
	if( TMP$1020$1 == 0ll) goto label$1812;
	{
		OUTEX( (char*)".cfi_endproc\x0A", 0ll );
	}
	label$1812:;
	label$1811:;
	EDBGEMITLINEFLUSH( PROC$1, *(int64*)((uint8*)*(struct $10FB_PROCEXT**)((uint8*)PROC$1 + 208ll) + 40ll), EXITLABEL$1 );
	EDBGEMITPROCFOOTER( PROC$1, INITLABEL$1, EXITLABEL$1 );
	label$1802:;
}

static void _SCOPEBEGIN( struct $8FBSYMBOL* S$1 )
{
	label$1813:;
	EDBGSCOPEBEGIN( S$1 );
	label$1814:;
}

static void _SCOPEEND( struct $8FBSYMBOL* S$1 )
{
	label$1815:;
	EDBGSCOPEEND( S$1 );
	label$1816:;
}

static void _SETSECTION( int64 SECTION$1, int64 PRIORITY$1 )
{
	label$1817:;
	char* SEC$1;
	char* vr$0 = _GETSECTIONSTRING( SECTION$1, PRIORITY$1 );
	SEC$1 = vr$0;
	if( SEC$1 != (char*)0ull) goto label$1820;
	{
		goto label$1818;
	}
	label$1820:;
	label$1819:;
	static FBSTRING OSTR$1;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)SEC$1, 0ll, 0 );
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A", 2ll, 0 );
	OUTEX( (char*)*(char**)&OSTR$1, 0ll );
	label$1818:;
}

static char* _GETTYPESTRING( int64 DTYPE$1 )
{
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1821:;
	{
		int64 TMP$1023$2;
		uint64 TMP$1024$2;
		if( (DTYPE$1 & 480ll) == 0ll) goto label$1823;
		TMP$1023$2 = 24ll;
		goto label$1952;
		label$1823:;
		TMP$1023$2 = DTYPE$1 & 31ll;
		label$1952:;
		TMP$1024$2 = (uint64)TMP$1023$2;
		goto label$1825;
		label$1826:;
		{
			fb$result$1 = (char*)".byte";
		}
		goto label$1824;
		label$1827:;
		{
			fb$result$1 = (char*)".short";
		}
		goto label$1824;
		label$1828:;
		{
			fb$result$1 = (char*)".int";
		}
		goto label$1824;
		label$1829:;
		{
			fb$result$1 = (char*)".long";
		}
		goto label$1824;
		label$1830:;
		{
			fb$result$1 = (char*)".quad";
		}
		goto label$1824;
		label$1831:;
		{
			fb$result$1 = (char*)".ascii";
		}
		goto label$1824;
		label$1832:;
		{
			fb$result$1 = (char*)".INVALID";
		}
		goto label$1824;
		label$1833:;
		{
			fb$result$1 = (char*)".long";
		}
		goto label$1824;
		label$1834:;
		{
			fb$result$1 = (char*)".INVALID";
		}
		goto label$1824;
		label$1825:;
		static const void* tmp$1060[24ll] = {
			&&label$1826,
			&&label$1826,
			&&label$1826,
			&&label$1831,
			&&label$1827,
			&&label$1827,
			&&label$1831,
			&&label$1828,
			&&label$1828,
			&&label$1828,
			&&label$1829,
			&&label$1829,
			&&label$1830,
			&&label$1830,
			&&label$1829,
			&&label$1830,
			&&label$1832,
			&&label$1831,
			&&label$1834,
			&&label$1832,
			&&label$1834,
			&&label$1834,
			&&label$1834,
			&&label$1833,
		};
		if( (TMP$1024$2 - 1ull) > 23ull ) goto label$1834;
		goto *tmp$1060[TMP$1024$2 - 1ull];
		label$1824:;
	}
	label$1822:;
	return fb$result$1;
}

static char* _GETSECTIONSTRING( int64 SECTION$1, int64 PRIORITY$1 )
{
	char* fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$1835:;
	static FBSTRING OSTR$1;
	if( ((int64)-(SECTION$1 == *(int64*)((uint8*)&EMIT$ + 280ll)) & (int64)-(PRIORITY$1 == *(int64*)((uint8*)&EMIT$ + 288ll))) == 0ll) goto label$1838;
	{
		fb$result$1 = (char*)0ull;
		goto label$1836;
	}
	label$1838:;
	label$1837:;
	fb_StrAssign( (void*)&OSTR$1, -1ll, (void*)"\x0A", 2ll, 0 );
	int64 vr$4 = FBGETOPTION( 3ll );
	if( vr$4 == 11ll) goto label$1840;
	{
		fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)".section ", 10ll, 0 );
	}
	label$1840:;
	label$1839:;
	fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)".", 2ll, 0 );
	{
		uint64 TMP$1033$2;
		TMP$1033$2 = (uint64)SECTION$1;
		goto label$1842;
		label$1843:;
		{
			{
				uint64 TMP$1034$4;
				int64 vr$5 = FBGETOPTION( 3ll );
				TMP$1034$4 = (uint64)vr$5;
				goto label$1845;
				label$1846:;
				{
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"rdata", 6ll, 0 );
				}
				goto label$1844;
				label$1847:;
				{
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"const", 6ll, 0 );
				}
				goto label$1844;
				label$1848:;
				{
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"rodata", 7ll, 0 );
				}
				goto label$1844;
				label$1845:;
				static const void* tmp$1061[12ll] = {
					&&label$1846,
					&&label$1846,
					&&label$1848,
					&&label$1848,
					&&label$1848,
					&&label$1846,
					&&label$1846,
					&&label$1848,
					&&label$1848,
					&&label$1848,
					&&label$1848,
					&&label$1847,
				};
				if( TMP$1034$4 > 11ull ) goto label$1848;
				goto *tmp$1061[TMP$1034$4 - 0ull];
				label$1844:;
			}
		}
		goto label$1841;
		label$1849:;
		{
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"data", 5ll, 0 );
		}
		goto label$1841;
		label$1850:;
		{
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"bss", 4ll, 0 );
		}
		goto label$1841;
		label$1851:;
		{
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"text", 5ll, 0 );
		}
		goto label$1841;
		label$1852:;
		{
			fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"drectve", 8ll, 0 );
		}
		goto label$1841;
		label$1853:;
		{
			int64 vr$6 = FBGETOPTION( 3ll );
			if( vr$6 != 11ll) goto label$1855;
			{
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"section __DATA,fbctinf", 23ll, 0 );
			}
			goto label$1854;
			label$1855:;
			{
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"fbctinf", 8ll, 0 );
			}
			label$1854:;
		}
		goto label$1841;
		label$1856:;
		{
			int64 vr$7 = FBGETOPTION( 3ll );
			if( vr$7 != 11ll) goto label$1858;
			{
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"constructor", 12ll, 0 );
			}
			goto label$1857;
			label$1858:;
			{
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"ctors", 6ll, 0 );
				if( PRIORITY$1 <= 0ll) goto label$1860;
				{
					FBSTRING TMP$1047$5;
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)".", 2ll, 0 );
					FBSTRING* vr$9 = fb_LongintToStr( 65535ll - PRIORITY$1 );
					__builtin_memset( &TMP$1047$5, 0, 24ll );
					FBSTRING* vr$12 = fb_StrConcat( &TMP$1047$5, (void*)"00000", 6ll, (void*)vr$9, -1ll );
					FBSTRING* vr$13 = fb_RIGHT( (FBSTRING*)vr$12, 5ll );
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$13, -1ll, 0 );
				}
				label$1860:;
				label$1859:;
				if( *(int64*)((uint8*)&ENV$ + 216ll) != 2ll) goto label$1862;
				{
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", \x22" "aw\x22, @progbits", 18ll, 0 );
				}
				label$1862:;
				label$1861:;
			}
			label$1857:;
		}
		goto label$1841;
		label$1863:;
		{
			int64 vr$14 = FBGETOPTION( 3ll );
			if( vr$14 != 11ll) goto label$1865;
			{
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"destructor", 11ll, 0 );
			}
			goto label$1864;
			label$1865:;
			{
				fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)"dtors", 6ll, 0 );
				if( PRIORITY$1 <= 0ll) goto label$1867;
				{
					FBSTRING TMP$1056$5;
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)".", 2ll, 0 );
					FBSTRING* vr$16 = fb_LongintToStr( 65535ll - PRIORITY$1 );
					__builtin_memset( &TMP$1056$5, 0, 24ll );
					FBSTRING* vr$19 = fb_StrConcat( &TMP$1056$5, (void*)"00000", 6ll, (void*)vr$16, -1ll );
					FBSTRING* vr$20 = fb_RIGHT( (FBSTRING*)vr$19, 5ll );
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)vr$20, -1ll, 0 );
				}
				label$1867:;
				label$1866:;
				if( *(int64*)((uint8*)&ENV$ + 216ll) != 2ll) goto label$1869;
				{
					fb_StrConcatAssign( (void*)&OSTR$1, -1ll, (void*)", \x22" "aw\x22, @progbits", 18ll, 0 );
				}
				label$1869:;
				label$1868:;
			}
			label$1864:;
		}
		goto label$1841;
		label$1842:;
		static const void* tmp$1062[8ll] = {
			&&label$1843,
			&&label$1849,
			&&label$1850,
			&&label$1851,
			&&label$1852,
			&&label$1856,
			&&label$1863,
			&&label$1853,
		};
		if( TMP$1033$2 > 7ull ) goto label$1841;
		goto *tmp$1062[TMP$1033$2 - 0ull];
		label$1841:;
	}
	fb$result$1 = (char*)*(char**)&OSTR$1;
	*(int64*)((uint8*)&EMIT$ + 280ll) = SECTION$1;
	*(int64*)((uint8*)&EMIT$ + 288ll) = PRIORITY$1;
	label$1836:;
	return fb$result$1;
}
