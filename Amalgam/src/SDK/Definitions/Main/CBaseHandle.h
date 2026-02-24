#pragma once
#include "../Definitions.h"

class IHandleEntity;

enum INVALID_EHANDLE_tag
{
	INVALID_EHANDLE
};

class CBaseHandle
{
public:
	CBaseHandle();
	CBaseHandle(INVALID_EHANDLE_tag);
	CBaseHandle(const CBaseHandle& other);
	explicit CBaseHandle(IHandleEntity* pHandleObj);
	CBaseHandle(int iEntry, int iSerialNumber);
	static CBaseHandle UnsafeFromIndex(int index);
	void Init(int iEntry, int iSerialNumber);
	void Term();
	bool IsValid() const;
	int GetEntryIndex() const;
	int GetSerialNumber() const;
	int ToInt() const;
	bool operator !=(const CBaseHandle& other) const;
	bool operator ==(const CBaseHandle& other) const;
	bool operator ==(const IHandleEntity* pEnt) const;
	bool operator !=(const IHandleEntity* pEnt) const;
	bool operator <(const CBaseHandle& other) const;
	bool operator <(const IHandleEntity* pEnt) const;
	const CBaseHandle& operator=(const IHandleEntity* pEntity);
	const CBaseHandle& Set(const IHandleEntity* pEntity);
	IHandleEntity* Get() const;
protected:
	uint32 m_Index;
};

#include "ihandleentity.h"

inline CBaseHandle::CBaseHandle()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CBaseHandle::CBaseHandle(INVALID_EHANDLE_tag)
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CBaseHandle::CBaseHandle(const CBaseHandle& other)
{
	m_Index = other.m_Index;
}

inline CBaseHandle::CBaseHandle(IHandleEntity* pEntity)
{
	Set(pEntity);
}

inline CBaseHandle::CBaseHandle(int iEntry, int iSerialNumber)
{
	Init(iEntry, iSerialNumber);
}

inline CBaseHandle CBaseHandle::UnsafeFromIndex(int index)
{
	CBaseHandle ret;
	ret.m_Index = index;
	return ret;
}

inline void CBaseHandle::Init(int iEntry, int iSerialNumber)
{
	m_Index = iEntry | (iSerialNumber << NUM_SERIAL_NUM_SHIFT_BITS);
}

inline void CBaseHandle::Term()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline bool CBaseHandle::IsValid() const
{
	return m_Index != INVALID_EHANDLE_INDEX;
}

inline int CBaseHandle::GetEntryIndex() const
{
	if (!IsValid())
		return NUM_ENT_ENTRIES - 1;
	return m_Index & ENT_ENTRY_MASK;
}

inline int CBaseHandle::GetSerialNumber() const
{
	return m_Index >> NUM_SERIAL_NUM_SHIFT_BITS;
}

inline int CBaseHandle::ToInt() const
{
	return (int)m_Index;
}

inline bool CBaseHandle::operator !=(const CBaseHandle& other) const
{
	return m_Index != other.m_Index;
}

inline bool CBaseHandle::operator ==(const CBaseHandle& other) const
{
	return m_Index == other.m_Index;
}

inline bool CBaseHandle::operator ==(const IHandleEntity* pEnt) const
{
	return Get() == pEnt;
}

inline bool CBaseHandle::operator !=(const IHandleEntity* pEnt) const
{
	return Get() != pEnt;
}

inline bool CBaseHandle::operator <(const CBaseHandle& other) const
{
	return m_Index < other.m_Index;
}

inline bool CBaseHandle::operator <(const IHandleEntity* pEntity) const
{
	uint32 otherIndex = (pEntity) ? pEntity->GetRefEHandle().m_Index : INVALID_EHANDLE_INDEX;
	return m_Index < otherIndex;
}

inline const CBaseHandle& CBaseHandle::operator=(const IHandleEntity* pEntity)
{
	return Set(pEntity);
}

inline const CBaseHandle& CBaseHandle::Set(const IHandleEntity* pEntity)
{
	if (pEntity)
	{
		*this = pEntity->GetRefEHandle();
	}
	else
	{
		m_Index = INVALID_EHANDLE_INDEX;
	}

	return *this;
}

template <class T>
class CHandle : public CBaseHandle
{
public:
	CHandle();
	CHandle(int iEntry, int iSerialNumber);
	CHandle(T* pVal);
	CHandle(INVALID_EHANDLE_tag);

	static CHandle<T> UnsafeFromBaseHandle(const CBaseHandle& handle);
	static CHandle<T> UnsafeFromIndex(int index);
	bool ChangedFrom(T* ent) const;

	T* Get() const;
	void Set(const T* pVal);

	operator T* ();
	operator T* () const;

	bool               operator !() const;
	bool               operator==(T* val) const;
	bool               operator!=(T* val) const;
	CHandle& operator=(const T* val);

	T* operator->() const;
};

template <class T>
inline CHandle<T>::CHandle()
{
}

template <class T>
inline CHandle<T>::CHandle(INVALID_EHANDLE_tag) : CBaseHandle(INVALID_EHANDLE)
{
}

template <class T>
inline CHandle<T>::CHandle(int iEntry, int iSerialNumber)
{
	Init(iEntry, iSerialNumber);
}

template <class T>
inline CHandle<T>::CHandle(T* pObj) : CBaseHandle(INVALID_EHANDLE)
{
	Set(pObj);
}

template <class T>
inline CHandle<T> CHandle<T>::UnsafeFromBaseHandle(const CBaseHandle& handle)
{
	CHandle<T> ret;
	ret.m_Index = (uint32)handle.ToInt();
	return ret;
}

template <class T>
CHandle<T> CHandle<T>::UnsafeFromIndex(int index)
{
	CHandle<T> ret;
	ret.m_Index = index;
	return ret;
}

template <class T>
inline bool CHandle<T>::ChangedFrom(T* ent) const
{
	if (ent == nullptr)
		return IsValid();

	return ent != Get();
}

template <class T>
T* CHandle<T>::Get() const
{
	return reinterpret_cast<T*>(CBaseHandle::Get());
}

template <class T>
CHandle<T>::operator T* ()
{
	return Get();
}

template <class T>
CHandle<T>::operator T* () const
{
	return Get();
}

template <class T>
bool CHandle<T>::operator !() const
{
	return !Get();
}

template <class T>
bool CHandle<T>::operator==(T* val) const
{
	return Get() == val;
}

template <class T>
bool CHandle<T>::operator!=(T* val) const
{
	return Get() != val;
}

template <class T>
void CHandle<T>::Set(const T* pVal)
{
	const IHandleEntity* pValInterface = reinterpret_cast<const IHandleEntity*>(pVal);
	CBaseHandle::Set(pValInterface);
}

template <class T>
inline CHandle<T>& CHandle<T>::operator=(const T* val)
{
	Set(val);
	return *this;
}

template <class T>
T* CHandle<T>::operator ->() const
{
	return Get();
}

template<typename T>
__forceinline void EnsureValidValue(CHandle<T>& x) 
{ 
	x = INVALID_EHANDLE; 
}