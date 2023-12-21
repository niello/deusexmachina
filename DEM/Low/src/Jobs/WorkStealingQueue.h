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
	static_assert(std::atomic<T>::is_always_lock_free, "Consider using a lock-free atomic type to avoid performance penalty");

private:

	struct CStorage
	{
		size_t          Capacity;
		size_t          Mask;
		std::atomic<T>* Elements;

		CStorage(size_t Capacity_) : Capacity(Capacity_), Mask(Capacity_ - 1), Elements(new std::atomic<T>[Capacity_]) {}

		//???FIXME: why elements need to be atomic?! who might read their values when being stored? is this because of resizing?
		//when push writes to the slot where steal steals? can happen if maxsize = capacity - 1?
		T Get(size_t Index) const { return Elements[Index & Mask].load(std::memory_order_relaxed); }
		void Set(size_t Index, T Value) const { Elements[Index & Mask].store(Value, std::memory_order_relaxed); }
	};

	std::atomic<size_t>    _Bottom;  // Written only in Push() and Pop()
	std::atomic<CStorage*> _Storage; // Written only in Push()
	std::vector<CStorage*> _Garbage; // Written only in Push()

	// Skip remaining part of a ceche line and a one more whole cache line (due to prefetch) between _Top and other fields to prevent false sharing
	alignas(std::hardware_destructive_interference_size) char _PAD[std::hardware_destructive_interference_size];

	alignas(std::hardware_destructive_interference_size)
	std::atomic<size_t>    _Top;     // Written with sequentially consistent CAS in Steal() and rarely in Pop()

public:

	CWorkStealingQueue(size_t Capacity = 256)
	{
		// NB: effective capacity must be a power of 2
		_Storage.store(new CStorage(std::max<size_t>(2, Math::NextPow2(Capacity))), std::memory_order_relaxed);
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
		//if (NewElement == {}) return;

		const size_t Bottom = _Bottom.load(std::memory_order_relaxed);
		const size_t Top = _Top.load(std::memory_order_acquire);
		CStorage* pStorage = _Storage.load(std::memory_order_relaxed);

		// Resize a queue if it is full.
		// The last cell in the array remains unused for possible extension according to Chase & Lev paper p.4.1, when a full array indicates an empty array.
		if (Bottom >= Top + pStorage->Capacity - 1)
		{
			// TODO: if (Bottom == std::numeric_limits<size_t>().max()) - how to reset?! Start returning false, so that calling code can wait all jobs and recreate queue then?

			// Allocate a new buffer twice as big, its capacity is also a power of 2
			CStorage* pNewStorage = new CStorage(pStorage->Capacity << 1);

			// Copy elements to the same indices in the new array. Other threads still see an old array as the source for Pop and Steal.
			for (size_t i = Top; i < Bottom; ++i)
				pNewStorage->Set(i, pStorage->Get(i));

			// This prevents an allocator from reusing the same memory addresses. This guarantees that interrupted Steal() operations
			// that reference the old array will not access some new data if its location is overwritten by the new owner of this memory.
			_Garbage.push_back(pStorage);

			// Use a new array instead of an old one. Until Steal() in other threads see this store, they will access a previous array which remains valid.
			// Old array doesn't contain our new record, but Bottom is stored with "release" and reading it will imply reading the new array address.
			pStorage = pNewStorage;
			_Storage.store(pNewStorage, std::memory_order_relaxed); //???FIXME: relaxed or release?
		}

		pStorage->Set(Bottom, NewElement);

		// Publish a new element with store-release, so that stealing threads can see it
		_Bottom.store(Bottom + 1, std::memory_order_release);
	}

	// LIFO. Called only from an owner thread.
	T Pop()
	{
		T Value{};

		const size_t Bottom = _Bottom.load(std::memory_order_relaxed);

		// Top may be out of date in our thread at the moment, but then it is less than actual, since Top
		// only grows. So actual Top gives us a useful early exit and inactual Top does no harm.
		if (_Top.load(std::memory_order_relaxed) < Bottom)
		{
			CStorage* pStorage = _Storage.load(std::memory_order_relaxed);

			const size_t NewBottom = Bottom - 1;

			// Notify concurrent stealing threads that we want to take the bottommost element
#if DEM_CPU_ARCH_X86_COMPATIBLE
			// 'xchg' instruction on x86 acts like a memory barrier and is a bit cheaper than 'mfence'
			//!!!TODO: PROFILE! Other branch generates lock inc, maube it is not slower or even faster! Check in real multithreaded app, optimizations might cut something!
			_Bottom.exchange(NewBottom, std::memory_order_seq_cst);
#else
			_Bottom.store(NewBottom, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_seq_cst);
#endif

			// There is a situation in which Pop() could break everything by returning the same element as Steal().
			// For this to happen, Steal() and Pop() must fight for the last element in a queue, but Pop() must
			// think that it is not the last element. Then Pop() will not execute CAS on Top and race will occur.
			// Since Top monotonically increases, this is possible only if Pop() holds a stale value of Top,
			// and Steal() didn't see _Bottom decrement above. To win CAS, our Steal() must see the latest Top.
			// Let: P - pop, S - steal, R - read, W - write, b - bottom, t - top. Then for Pop() to see a stale
			// Top, it must read it before the last write from Steal() became observable in Pop()'s thread:
			//   READ_t = PRt (stale) -> <t incremented by other S> -> SRt (latest)
			// In this case the only way for error to happen is when SRb didn't see PWb-1 (a store above).
			// a) If PRt -> ... -> PWb-1, nothing prevents us from executing a successful Steal() inside "..."
			//    and then continue with inactual Top. To prevent this, we call PWb-1 before PRt and forbid
			//    reordering. Since this is StoreLoad pair, we have to make a memory fence, because even on
			//    strong x86 StoreLoad reordering is allowed at runtime. I.e. even if PWb-1 is executed before PRt,
			//    Steal() has a chance not to observe it in time.
			// b) Given PWb-1 -> PRt (by "a") and PRt -> SRt (by definition), SRb -> PWb-1 is still possible, but
			//    only if SRb -> SRt. Steal() reads Bottom, then Pop() writes Bottom-1, then it reads stale Top,
			//    then Steal() regains control and reads actual Top. To prevent this, we can simply ensure that
			//    SRt -> SRb. In this case, since we read actual Top, Pop() has already propagated actual Bottom.
			size_t Top = _Top.load(std::memory_order_relaxed);

			if (Top > NewBottom)
			{
				// The queue is empty, undo Bottom decrement, resetting the queue to the canonical empty state (Bottom == Top)
				_Bottom.store(Bottom, std::memory_order_relaxed);
			}
			else
			{
				//???FIXME: why reading now? Push() can't take this slot because it is in the same thread with us!
				Value = pStorage->Get(NewBottom);

				if (Top == NewBottom)
				{
					// The queue contains the last element, must compete for it with Steal(). CAS failure means that we
					// have lost the race, so our Value was stolen by another consumer and we must not return it.
					// NB: for the last element Pop() increments Top instead of decrementing Bottom to participate in CAS.
					if (!_Top.compare_exchange_strong(Top, Top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
						Value = {};

					// Whether we won or lost the race, Top is now incremented, so Bottom decrement must be undone.
					// Stealing threads will eventually observe this, but we don't need to hurry.
					_Bottom.store(Bottom, std::memory_order_relaxed);
				}
			}
		}

		return Value;
	}

	// FIFO. Can be called from any thread.
	T Steal()
	{
// The reason is that it must be guaranteed that the load operation in steal sees the updated value written in pop.
// Otherwise you could have a race condition where the same item could be returned twice(once from pop and once from steal).
//-
// A very crucial thing to note here is that top is always read before bottom, ensuring that the values represent a consistent
// view of the memory. Still, a subtle race may occur if the deque is emptied by a concurrent Pop() after bottom is read and before
// the CAS is executed. We need to ensure that no concurrent Pop() and Steal() operations both return the last job remaining in the deque,
// which is achieved by also trying to modify top in the implementation of Pop() using a CAS operation.
		const auto Top = _Top.load(std::memory_order_acquire);

		/*
		// Possibly inactual Bottom will lead to a false negative (empty queue, nothing to steal), but it saves
		// us from hitting unnecessary std::memory_order_seq_cst fence at each Steal() on empty queues
		const auto Bottom = _Bottom.load(std::memory_order_acquire);
		if (Top >= Bottom) return {};
		*/

//???WHY IN MOLECULAR IS THIS???
//ensure that top is always read before bottom. loads will not be reordered with other loads on x86, so a compiler barrier is enough.
//On x86, an mfence instruction is added between the two reads in steal (c) ppopp13 - but a compiler barrier would be enough.
		std::atomic_thread_fence(std::memory_order_seq_cst);
		const auto Bottom = _Bottom.load(std::memory_order_acquire);
// Bottom becomes seq-cst load-acquire with store in pop in total order and syncs with store-release in push

		// The queue is empty, nothing to steal
		if (Top >= Bottom) return {};

		auto Elements = _Elements.load(std::memory_order_consume);

		//???!!!capacity isn't protected now!? prevoiusly it was stored inside an array and was obtained with pointer!
		//!!!FIXME: now Elements may not match _Capacity! And even if no problem here because of the same thread, Steal() may suffer!
		//???allocate new array as a single buffer? store both capacity & fixed number of elements there? how about alignment? is worth? rare operation!
		T Value = Elements[Top & (_Capacity - 1)].load(std::memory_order_relaxed);

//Since the algorithm uses a cyclic array, it is important to read the element from the array before we do
//the CAS, because after the CAS completes, this location may be refilled with a new value by a concurrent pushBottom operation.
//-
//Note that it is important to read the job before carrying out the CAS, because the location in the array could be overwritten
//by concurrent Push() operations happening after the CAS has completed.
//-
//Additionally, we would need another barrier that guarantees that the read from the array is carried out before the CAS.
//In this case however, the interlocked function acts as a compiler barrier implicitly.
		if (!_Top.compare_exchange_strong(Top, Top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
			Value = {};

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
