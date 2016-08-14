#ifndef _LVSTRING_H_
#define _LVSTRING_H_

inline LVHash _hashstr (const LVChar *s, size_t l) {
	LVHash h = (LVHash)l;  /* seed */
	size_t step = (l >> 5) | 1; /* if string is too long, don't hash all its chars */
	for (; l >= step; l -= step)
		h = h ^ ((h << 5) + (h >> 2) + (unsigned short) * (s++));
	return h;
}

struct LVString : public LVRefCounted {
	LVString() {}
	~LVString() {}

  public:
	static LVString *Create(LVSharedState *ss, const LVChar *, LVInteger len = -1);
	LVInteger Next(const LVObjectPtr& refpos, LVObjectPtr& outkey, LVObjectPtr& outval);
	void Release();
	LVSharedState *_sharedstate;
	LVString *_next; //chain for the string table
	LVInteger _len;
	LVHash _hash;
	LVChar _val[1];
};

#endif // _LVSTRING_H_
