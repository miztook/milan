#pragma once

#include "Base.h"

// First-fit non-local allocator.
class NonLocalAllocator
{
public:
	static const uint64_t kInvalidBlock = UINT64_MAX;

	NonLocalAllocator()
	{
	}

	~NonLocalAllocator()
	{
	}

	void reset()
	{
		m_free.clear();
		m_used.clear();
	}

	void add(uint64_t _ptr, uint32_t _size)
	{
		m_free.push_back(Free(_ptr, _size));
	}

	uint64_t remove()
	{
		BX_CHECK(0 == m_used.size(), "");

		if (0 < m_free.size())
		{
			Free freeBlock = m_free.front();
			m_free.pop_front();
			return freeBlock.m_ptr;
		}

		return 0;
	}

	uint64_t alloc(uint32_t _size)
	{
		_size = bx::max(_size, 16u);

		for (FreeList::iterator it = m_free.begin(), itEnd = m_free.end(); it != itEnd; ++it)
		{
			if (it->m_size >= _size)
			{
				uint64_t ptr = it->m_ptr;

				m_used.insert(stl::make_pair(ptr, _size));

				if (it->m_size != _size)
				{
					it->m_size -= _size;
					it->m_ptr += _size;
				}
				else
				{
					m_free.erase(it);
				}

				return ptr;
			}
		}

		// there is no block large enough.
		return kInvalidBlock;
	}

	void free(uint64_t _block)
	{
		UsedList::iterator it = m_used.find(_block);
		if (it != m_used.end())
		{
			m_free.push_front(Free(it->first, it->second));
			m_used.erase(it);
		}
	}

	bool compact()
	{
		m_free.sort();

		for (FreeList::iterator it = m_free.begin(), next = it, itEnd = m_free.end(); next != itEnd;)
		{
			if ((it->m_ptr + it->m_size) == next->m_ptr)
			{
				it->m_size += next->m_size;
				next = m_free.erase(next);
			}
			else
			{
				it = next;
				++next;
			}
		}

		return 0 == m_used.size();
	}

private:
	struct Free
	{
		Free(uint64_t _ptr, uint32_t _size)
			: m_ptr(_ptr)
			, m_size(_size)
		{
		}

		bool operator<(const Free& rhs) const
		{
			return m_ptr < rhs.m_ptr;
		}

		uint64_t m_ptr;
		uint32_t m_size;
	};

	typedef stl::list<Free> FreeList;
	FreeList m_free;

	typedef stl::unordered_map<uint64_t, uint32_t> UsedList;
	UsedList m_used;
};