#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "utils.h"

#ifdef _LV64
#define UINT_MINUS_ONE (0xFFFFFFFFFFFFFFFF)
#else
#define UINT_MINUS_ONE (0xFFFFFFFF)
#endif

struct LVSharedState;

enum LVMetaMethod {
	MT_ADD = 0,
	MT_SUB = 1,
	MT_MUL = 2,
	MT_DIV = 3,
	MT_UNM = 4,
	MT_MODULO = 5,
	MT_SET = 6,
	MT_GET = 7,
	MT_TYPEOF = 8,
	MT_NEXTI = 9,
	MT_CMP = 10,
	MT_CALL = 11,
	MT_CLONED = 12,
	MT_NEWSLOT = 13,
	MT_DELSLOT = 14,
	MT_TOSTRING = 15,
	MT_NEWMEMBER = 16,
	MT_INHERITED = 17,
	MT_LAST = 18
};

#define MM_ADD      _LC("_add")
#define MM_SUB      _LC("_sub")
#define MM_MUL      _LC("_mul")
#define MM_DIV      _LC("_div")
#define MM_UNM      _LC("_unm")
#define MM_MODULO   _LC("_modulo")
#define MM_SET      _LC("_set")
#define MM_GET      _LC("_get")
#define MM_TYPEOF   _LC("_typeof")
#define MM_NEXTI    _LC("_nexti")
#define MM_CMP      _LC("_cmp")
#define MM_CALL     _LC("_call")
#define MM_CLONED   _LC("_cloned")
#define MM_NEWSLOT  _LC("_newslot")
#define MM_DELSLOT  _LC("_delslot")
#define MM_TOSTRING _LC("_tostring")
#define MM_NEWMEMBER _LC("_newmember")
#define MM_INHERITED _LC("_inherited")


#define _CONSTRUCT_VECTOR(type,size,ptr) { \
    for(LVInteger n = 0; n < ((LVInteger)size); n++) { \
            new (&ptr[n]) type(); \
        } \
}

#define _DESTRUCT_VECTOR(type,size,ptr) { \
    for(LVInteger nl = 0; nl < ((LVInteger)size); nl++) { \
            ptr[nl].~type(); \
    } \
}

#define _COPY_VECTOR(dest,src,size) { \
    for(LVInteger _n_ = 0; _n_ < ((LVInteger)size); _n_++) { \
        dest[_n_] = src[_n_]; \
    } \
}

#define _NULL_OBJECT_VECTOR(vec,size) { \
    for(LVInteger _n_ = 0; _n_ < ((LVInteger)size); _n_++) { \
        vec[_n_].Null(); \
    } \
}

#define MINPOWER2 4

struct LVRefCounted {
	LVUnsignedInteger _uiRef;
	struct LVWeakRef *_weakref;

	LVRefCounted() {
		_uiRef = 0;
		_weakref = NULL;
	}

	virtual ~LVRefCounted();
	LVWeakRef *GetWeakRef(LVObjectType type);
	virtual void Release() = 0;

};

struct LVWeakRef : LVRefCounted {
	void Release();
	LVObject _obj;
};

#define _realval(o) (type((o)) != OT_WEAKREF ? (LVObject)o : _weakref(o)->_obj)

struct LVObjectPtr;

#define __AddRef(type,unval) if(ISREFCOUNTED(type)) \
        { \
            unval.pRefCounted->_uiRef++; \
        }

#define __Release(type,unval) if(ISREFCOUNTED(type) && ((--unval.pRefCounted->_uiRef)==0))  \
        {   \
            unval.pRefCounted->Release();   \
        }

#define __ObjRelease(obj) { \
    if((obj)) { \
        (obj)->_uiRef--; \
        if((obj)->_uiRef == 0) \
            (obj)->Release(); \
        (obj) = NULL;   \
    } \
}

#define __ObjAddRef(obj) { \
    (obj)->_uiRef++; \
}

#define type(obj) ((obj)._type)
#define is_delegable(t) (type(t) & OBJECT_DELEGABLE)
#define raw_type(obj) _RAW_TYPE((obj)._type)

#define _integer(obj) ((obj)._unVal.nInteger)
#define _float(obj) ((obj)._unVal.fFloat)
#define _string(obj) ((obj)._unVal.pString)
#define _table(obj) ((obj)._unVal.pTable)
#define _array(obj) ((obj)._unVal.pArray)
#define _closure(obj) ((obj)._unVal.pClosure)
#define _generator(obj) ((obj)._unVal.pGenerator)
#define _nativeclosure(obj) ((obj)._unVal.pNativeClosure)
#define _userdata(obj) ((obj)._unVal.pUserData)
#define _userpointer(obj) ((obj)._unVal.pUserPointer)
#define _thread(obj) ((obj)._unVal.pThread)
#define _funcproto(obj) ((obj)._unVal.pFunctionProto)
#define _class(obj) ((obj)._unVal.pClass)
#define _instance(obj) ((obj)._unVal.pInstance)
#define _delegable(obj) ((LVDelegable *)(obj)._unVal.pDelegable)
#define _weakref(obj) ((obj)._unVal.pWeakRef)
#define _outer(obj) ((obj)._unVal.pOuter)
#define _refcounted(obj) ((obj)._unVal.pRefCounted)
#define _rawval(obj) ((obj)._unVal.raw)

#define _stringval(obj) (obj)._unVal.pString->_val
#define _userdataval(obj) ((LVUserPointer)LV_ALIGN((obj)._unVal.pUserData + 1))

#define tofloat(num) ((type(num)==OT_INTEGER)?(LVFloat)_integer(num):_float(num))
#define tointeger(num) ((type(num)==OT_FLOAT)?(LVInteger)_float(num):_integer(num))

#if defined(USEDOUBLE) && !defined(_LV64) || !defined(USEDOUBLE) && defined(_LV64)
#define OBJECT_RAWINIT() { _unVal.raw = 0; }
#define REFOBJECT_INIT() OBJECT_RAWINIT()
#else
#define REFOBJECT_INIT()
#endif

#define _REF_TYPE_DECL(type,_class,sym) \
    LVObjectPtr(_class * x) \
    { \
        OBJECT_RAWINIT() \
        _type=type; \
        _unVal.sym = x; \
        assert(_unVal.pTable); \
        _unVal.pRefCounted->_uiRef++; \
    } \
    inline LVObjectPtr& operator=(_class *x) \
    {  \
        LVObjectType tOldType; \
        LVObjectValue unOldVal; \
        tOldType=_type; \
        unOldVal=_unVal; \
        _type = type; \
        REFOBJECT_INIT() \
        _unVal.sym = x; \
        _unVal.pRefCounted->_uiRef++; \
        __Release(tOldType,unOldVal); \
        return *this; \
    }

#define _SCALAR_TYPE_DECL(type,_class,sym) \
    LVObjectPtr(_class x) \
    { \
        OBJECT_RAWINIT() \
        _type=type; \
        _unVal.sym = x; \
    } \
    inline LVObjectPtr& operator=(_class x) \
    {  \
        __Release(_type,_unVal); \
        _type = type; \
        OBJECT_RAWINIT() \
        _unVal.sym = x; \
        return *this; \
    }

struct LVObjectPtr : public LVObject {
	LVObjectPtr() {
		OBJECT_RAWINIT()
		_type = OT_NULL;
		_unVal.pUserPointer = NULL;
	}

	LVObjectPtr(const LVObjectPtr& o) {
		_type = o._type;
		_unVal = o._unVal;
		__AddRef(_type, _unVal);
	}

	LVObjectPtr(const LVObject& o) {
		_type = o._type;
		_unVal = o._unVal;
		__AddRef(_type, _unVal);
	}

	_REF_TYPE_DECL(OT_TABLE, LVTable, pTable)
	_REF_TYPE_DECL(OT_CLASS, LVClass, pClass)
	_REF_TYPE_DECL(OT_INSTANCE, LVInstance, pInstance)
	_REF_TYPE_DECL(OT_ARRAY, LVArray, pArray)
	_REF_TYPE_DECL(OT_CLOSURE, LVClosure, pClosure)
	_REF_TYPE_DECL(OT_NATIVECLOSURE, LVNativeClosure, pNativeClosure)
	_REF_TYPE_DECL(OT_OUTER, LVOuter, pOuter)
	_REF_TYPE_DECL(OT_GENERATOR, LVGenerator, pGenerator)
	_REF_TYPE_DECL(OT_STRING, LVString, pString)
	_REF_TYPE_DECL(OT_USERDATA, LVUserData, pUserData)
	_REF_TYPE_DECL(OT_WEAKREF, LVWeakRef, pWeakRef)
	_REF_TYPE_DECL(OT_THREAD, LVVM, pThread)
	_REF_TYPE_DECL(OT_FUNCPROTO, FunctionPrototype, pFunctionProto)

	_SCALAR_TYPE_DECL(OT_INTEGER, LVInteger, nInteger)
	_SCALAR_TYPE_DECL(OT_FLOAT, LVFloat, fFloat)
	_SCALAR_TYPE_DECL(OT_USERPOINTER, LVUserPointer, pUserPointer)

	LVObjectPtr(bool bBool) {
		OBJECT_RAWINIT()
		_type = OT_BOOL;
		_unVal.nInteger = bBool ? 1 : 0;
	}

	inline LVObjectPtr& operator=(bool b) {
		__Release(_type, _unVal);
		OBJECT_RAWINIT()
		_type = OT_BOOL;
		_unVal.nInteger = b ? 1 : 0;
		return *this;
	}

	~LVObjectPtr() {
		__Release(_type, _unVal);
	}

	inline LVObjectPtr& operator=(const LVObjectPtr& obj) {
		LVObjectType tOldType;
		LVObjectValue unOldVal;
		tOldType = _type;
		unOldVal = _unVal;
		_unVal = obj._unVal;
		_type = obj._type;
		__AddRef(_type, _unVal);
		__Release(tOldType, unOldVal);
		return *this;
	}

	inline LVObjectPtr& operator=(const LVObject& obj) {
		LVObjectType tOldType;
		LVObjectValue unOldVal;
		tOldType = _type;
		unOldVal = _unVal;
		_unVal = obj._unVal;
		_type = obj._type;
		__AddRef(_type, _unVal);
		__Release(tOldType, unOldVal);
		return *this;
	}

	inline void Null() {
		LVObjectType tOldType = _type;
		LVObjectValue unOldVal = _unVal;
		_type = OT_NULL;
		_unVal.raw = (LVRawObjectVal)NULL;
		__Release(tOldType , unOldVal);
	}

#ifdef _DEBUG
	void dump();
#endif

  private:
	LVObjectPtr(const LVChar *) {} //safety
};

inline void _Swap(LVObject& a, LVObject& b) {
	LVObjectType tOldType = a._type;
	LVObjectValue unOldVal = a._unVal;
	a._type = b._type;
	a._unVal = b._unVal;
	b._type = tOldType;
	b._unVal = unOldVal;
}

/////////////////////////////////////////////////////////////////////////////////////
#ifndef NO_GARBAGE_COLLECTOR
#define MARK_FLAG 0x80000000
struct LVCollectable : public LVRefCounted {
	LVCollectable *_next;
	LVCollectable *_prev;
	LVSharedState *_sharedstate;
	virtual LVObjectType GetType() = 0;
	virtual void Release() = 0;
	virtual void Mark(LVCollectable **chain) = 0;
	void UnMark();
	virtual void Finalize() = 0;
	static void AddToChain(LVCollectable **chain, LVCollectable *c);
	static void RemoveFromChain(LVCollectable **chain, LVCollectable *c);
};

#define ADD_TO_CHAIN(chain,obj) AddToChain(chain,obj)
#define REMOVE_FROM_CHAIN(chain,obj) {if(!(_uiRef&MARK_FLAG))RemoveFromChain(chain,obj);}
#define CHAINABLE_OBJ LVCollectable
#define INIT_CHAIN() {_next=NULL;_prev=NULL;_sharedstate=ss;}
#else

#define ADD_TO_CHAIN(chain,obj) ((void)0)
#define REMOVE_FROM_CHAIN(chain,obj) ((void)0)
#define CHAINABLE_OBJ LVRefCounted
#define INIT_CHAIN() ((void)0)
#endif

struct LVDelegable : public CHAINABLE_OBJ {
	bool SetDelegate(LVTable *m);
	virtual bool GetMetaMethod(LVVM *v, LVMetaMethod mm, LVObjectPtr& res);
	LVTable *_delegate;
};

LVUnsignedInteger TranslateIndex(const LVObjectPtr& idx);
typedef lvvector<LVObjectPtr> LVObjectPtrVec;
typedef lvvector<LVInteger> LVIntVector;
const LVChar *GetTypeName(const LVObjectPtr& obj1);
const LVChar *IdType2Name(LVObjectType type);

#endif // _OBJECT_H_
