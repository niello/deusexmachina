#pragma once
#include <Math/Math.h>
#include <atomic>

// A lock-free work stealing queue.
// Implements https://fzn.fr/readings/ppopp13.pdf, an optimized version of Chase-Lev deque.
// See also:
// https://blog.molecular-matters.com/2015/09/25/job-system-2-0-lock-free-work-stealing-part-3-going-lock-free/
// https://www.dre.vanderbilt.edu/~schmidt/PDF/work-stealing-dequeue.pdf

namespace DEM::Jobs
{

template<typename T>
class CWorkStealingQueue final
{
private:

	// Elements need not to be atomic because they are synchronized with other (stealing) threads either by
	// read-acquire of _Bottom or read-consume of _Storage which guarantee payload to be fully observed. See:
	// https://preshing.com/20130823/the-synchronizes-with-relation/
	// https://preshing.com/20140709/the-purpose-of-memory_order_consume-in-cpp11/
	struct CStorage
	{
		size_t Capacity;
		size_t Mask;
		T*     Elements;

		CStorage(size_t Capacity_) : Capacity(Capacity_), Mask(Capacity_ - 1), Elements(new(std::nothrow) T[Capacity_]) {}

		T Get(size_t Index) const { return Elements[Index & Mask]; }
		void Set(size_t Index, T Value) const { Elements[Index & Mask] = std::move(Value); }
	};

	std::atomic<size_t>    _Bottom;  // Written only in Push() and Pop()
	std::atomic<CStorage*> _Storage; // Written only in Push()
	std::vector<CStorage*> _Garbage; // Written only in Push()

	// Skip remaining part of a cache line and one more whole cache line (due to prefetch) between _Top and other fields to prevent false sharing
	alignas(std::hardware_destructive_interference_size) char _PAD[std::hardware_destructive_interference_size];

	alignas(std::hardware_destructive_interference_size)
	std::atomic<size_t>    _Top;     // Written with sequentially consistent CAS in Steal() and rarely in Pop()

	DEM_NO_INLINE CStorage* Grow(CStorage* pStorage, size_t Top, size_t Bottom)
	{
		// Allocate a new buffer twice as big, its capacity is also a power of 2
		CStorage* pNewStorage = new(std::nothrow) CStorage(pStorage->Capacity << 1);

		// Copy elements to the same indices in the new array. Other threads still see an old array as the source for Pop and Steal.
		for (size_t i = Top; i < Bottom; ++i)
			pNewStorage->Set(i, pStorage->Get(i));

		// This prevents an allocator from reusing the same memory addresses. This guarantees that interrupted Steal() operations
		// that reference the old array will not access some new data if its location is overwritten by the new owner of this memory.
		_Garbage.push_back(pStorage);

		// Publish a new storage with store-release, propagating its contents written with relaxed pNewStorage->Set() above.
		// There is an address dependency from storage ptr to values so the reader can synchronize with consume instead of acquire.
		_Storage.store(pNewStorage, std::memory_order_release);

		return pNewStorage;
	}

public:

	CWorkStealingQueue(size_t Capacity = 256)
	{
		// NB: effective capacity must be a power of 2
		_Storage.store(new(std::nothrow) CStorage(std::max<size_t>(2, Math::NextPow2(Capacity))), std::memory_order_relaxed);
		_Top.store(0, std::memory_order_relaxed);
		_Bottom.store(0, std::memory_order_relaxed);
	}

	~CWorkStealingQueue()
	{
		for (CStorage* pGarbage : _Garbage)
			delete pGarbage;
		delete _Storage.load(std::memory_order_relaxed);
	}

	// Called only from an owner thread
	void Push(T NewElement)
	{
		const size_t Bottom = _Bottom.load(std::memory_order_relaxed);
		const size_t Top = _Top.load(std::memory_order_acquire);
		CStorage* pStorage = _Storage.load(std::memory_order_relaxed);

		// Resize a queue if it is full.
		// The last cell in the array remains unused for possible extension according to Chase & Lev paper p.4.1, when a full array indicates an empty array.
		if (Bottom >= Top + pStorage->Capacity - 1) // TODO: C++20 [[unlikely]]
		{
			// TODO: if (Bottom == std::numeric_limits<size_t>().max()) - how to reset?! Start returning false, so that calling code can wait all jobs and recreate queue then?

			// Use a new array instead of an old one
			pStorage = Grow(pStorage, Top, Bottom);
		}

		pStorage->Set(Bottom, NewElement);

		// Publish a new element with store-release, so that stealing threads can see it. If a resize has happened,
		// a reader of Bottom will also see a new value of Storage. Until Steal() threads see this change they will
		// be accessing an old storage. Bottom values will be also old, and values in the old array are preserved at
		// correct indices and are never destroyed, so there won't be any incorrect access.
		_Bottom.store(Bottom + 1, std::memory_order_release);
	}

	// LIFO. Called only from an owner thread.
	T Pop()
	{
		T Value{};

		const size_t Bottom = _Bottom.load(std::memory_order_relaxed);

		// Top may be out of date in our thread at the moment, but then it is less than an actual Top, since it
		// only grows. Actual Top here gives us a useful early exit and inactual Top does no harm.
		if (_Top.load(std::memory_order_relaxed) < Bottom)
		{
			// Notify concurrent stealing threads that we want to take the bottommost element.
			// ---
			// There is a situation in which Pop() could break everything by returning the same element as Steal().
			// For this to happen, Steal() and Pop() must fight for the last element in a queue, but Pop() must
			// think that it is not the last element. Then Pop() will not execute CAS on Top and race will occur.
			// Since Top monotonically increases, this is possible only if Pop() holds a stale value of Top. To be
			// successful, Steal() must see the latest Top to win CAS and don't yet see --Bottom to proceed to it.
			// Let: P - pop, S - steal, R - read, W - write, b - bottom, t - top. Then for Pop() to see a stale
			// Top, it must read it before the last write from Steal() became observable in Pop()'s thread:
			//   PRt (stale) -> <++Top by sequentially consistent CAS in other S> -> SRt (latest)
			// In this case the only way for error to happen is when SRb didn't see PWb-1.
			// a) If PRt -> PWb-1, we can execute a successful Steal() in between and then continue with inactual
			//    Top. To prevent this, we call PWb-1 before PRt and forbid reordering. Since this is StoreLoad
			//    pair, we have to make a memory fence, because even on strong x86 StoreLoad reordering is allowed
			//    at runtime. I.e. even if PWb-1 is executed before PRt, Steal() might not observe it in time.
			// b) Given PWb-1 -> PRt (by "a") and PRt -> SRt (by definition), SRb -> PWb-1 is still possible, but
			//    only if SRb -> SRt. Steal() reads Bottom, then Pop() writes Bottom-1, then it reads stale Top,
			//    then Steal() regains control and reads actual Top. To prevent this, we can simply ensure that
			//    SRt -> SRb. Then, since Steal() reads actual Top, PWb-1 has also been already propagated to it.
			// So, either this barrier in Pop() is ordered before barrier in Steal() and it sees actual Bottom
			// or barrier in Steal() is ordered before this barrier in Pop() and Top that is read in Steal() will
			// be propagated here. In any case there won't be a situation when Steal() sees a stale Bottom and
			// Pop() sees a stale Top() at the same time, and this is enough to prevent the undesired behaviour.
			const size_t NewBottom = Bottom - 1;
#if DEM_CPU_ARCH_X86_COMPATIBLE
			// RMW here generates x86 'xchg' instruction that acts like a memory barrier and is a bit cheaper than a
			// separate fence, being it 'mfence' or 'mov' + 'lock inc' on an internal guard variable (MSVC does that).
			// See https://stackoverflow.com/questions/60332591/why-is-lock-a-full-barrier-on-x86.
			_Bottom.exchange(NewBottom, std::memory_order_seq_cst);
#else
			_Bottom.store(NewBottom, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_seq_cst);
#endif

			// The value read here is not older than one read in the latest Steal() which triggered a memory barrier
			// before the barrier right above. See the comment to _Top.load(...) in Steal(). On x86 there is no barrier
			// as the value read in Steal() is guaranteed to be propagated to all threads and therefore be seen here.
			size_t Top = _Top.load(std::memory_order_relaxed);

			if (Top < NewBottom)
			{
				// More than one element left. Simply read a value, there is no concurrency for it.
				Value = _Storage.load(std::memory_order_relaxed)->Get(NewBottom);
			}
			else
			{
				if (Top == NewBottom)
				{
					// The queue contains the last element, must compete for it with Steal() instances who managed to read
					// Top and Bottom before we reserved an element by decrementing Bottom. This requires us to
					// participate in CAS on Top. This is possible because when there is only one element in a queue
					// we can either decrement bottom or increment top to claim it.
					if (_Top.compare_exchange_strong(Top, Top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
						Value = _Storage.load(std::memory_order_relaxed)->Get(NewBottom);
				}

				// Whether the queue is empty or we won or lost the race for the last element, a Bottom decrement must be undone.
				// In the first case we reserved inexistent element. In the second case Top was modified instead of Bottom by CAS.
				// Stealing threads will eventually observe this store, but we don't need to hurry.
				_Bottom.store(Bottom, std::memory_order_relaxed);
			}
		}

		return Value;
	}

	// FIFO. Can be called from any thread.
	T Steal()
	{
		// Read Top synchronized with the global-order-previous write by CAS, making a total order:
		//  (++Top in latest CAS) -> (this load-acquire) -> (fence right below)
		// As acquire, this won't be reordered with operations below. On weak architectures this may not
		// synchronize with Bottom changes from Pop() because its thread didn't perform CAS. Anyway this
		// operation prevents a compiler from reordering instructions, which is important for x86 where it
		// is enough here for a proper sync.
		auto Top = _Top.load(std::memory_order_acquire);

		// Value of Bottom must not be older than a Top value we've just read. See the long comment in Pop(), near writing
		// NewBottom into _Bottom, case "b". This is LoadLoad pair, so on weak memory architectures a runtime reordering
		// may occur, and we need a memory barrier to see an actual value of Bottom. In x86 strong memory model LoadLoad
		// can't be reordered at runtime, so a compiler barrier is enough. We will either read an actual Bottom if Pop()
		// already stored it, or the Pop() will read an actual Top() because we just have read it and x86 memory model
		// guarantees that its value is now visible to all threads.
#if !DEM_CPU_ARCH_X86_COMPATIBLE
		// Possibly inactual Bottom will lead to a false negative (seeing the queue empty when it isn't), but it
		// saves us from hitting unnecessary std::memory_order_seq_cst fence at each Steal() on empty queues
		if (Top >= _Bottom.load(std::memory_order_relaxed)) return {};

		std::atomic_thread_fence(std::memory_order_seq_cst);
#endif

		// This read is either synchronized with:
		// - some of previous writes in Push(), propagating actual _Storage and added element here
		// - latest Pop() which reserved an element and triggered a memory barrier (simply latest Pop() on x86)
		// So we either will read an actual element or won't compete for the last element which is reserved.
		const auto Bottom = _Bottom.load(std::memory_order_acquire);

		T Value{};

		// Check that the queue is not empty
		if (Top < Bottom)
		{
			// Load an address of the storage and exploit address dependency to consume element values written during resize
			CStorage* pStorage = _Storage.load(std::memory_order_consume);

			// Read a value before CAS. In the case of successful CAS this slot will become free and can be refilled
			// with Push() because the buffer is curcular and when it is full the next Bottom will map to this Top.
			// CAS itself serves as a barrier so this read will not be reordered past it. Note that reserving the last
			// element is not enough because any number of Steal() instances can be preempted right after CAS and
			// any number of Push() instances can follow, overwriting slots to which pending steals are targeting.
			Value = pStorage->Get(Top);

			// Compete with other Steal() threads. In case of the last element there is also possible to compete with Pop()
			// if we reat Top & Bottom first and then Pop() tries to claim an element with CAS.
			if (!_Top.compare_exchange_strong(Top, Top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
				Value = {};
		}

		return Value;
	}

	bool empty() const { return _Bottom.load(std::memory_order_relaxed) <= _Top.load(std::memory_order_relaxed); }

	size_t size() const
	{
		const size_t Top = _Top.load(std::memory_order_relaxed);
		const size_t Bottom = _Bottom.load(std::memory_order_relaxed);
		return (Bottom > Top) ? (Bottom - Top) : 0;
	}
};

}
