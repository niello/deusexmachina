#pragma once
#include <Math/Math.h>
#include <atomic>

// A lock-free work stealing queue.
// Implements https://fzn.fr/readings/ppopp13.pdf, and optimized version of Chase-Lev deque.

namespace DEM::Jobs
{

template<typename T>
class CWorkStealingQueue final
{
	static_assert(std::atomic<T>::is_always_lock_free, "Consider using a lock-free atomic type to avoid performance penalty");

private:

	//???TODO: force top & bottom different cache lines to avoid false sharing?! 2 lines to combat prefetching!
	//???can also exploit true sharing? to load top or bottom with other useful fields, but avoiding false sharing on write.
	std::atomic<size_t>          _Top;           // Written only in Steal()
	std::atomic<size_t>          _Bottom;        // Written only in Push() and Pop()
	std::atomic<std::atomic<T>*> _Elements;
	size_t                       _Capacity = 0;
	std::vector<std::atomic<T>*> _Garbage;       // We should prevent reuse of memory that was already used for the queue, so we store it here

public:

	CWorkStealingQueue(size_t Capacity = 256)
	{
		// NB: effective capacity must be a power of 2
		_Capacity = std::max(2, Math::NextPow2(Capacity));
		_Elements.store(new std::atomic<T>[_Capacity], std::memory_order_relaxed);
		_Top.store(0, std::memory_order_relaxed);
		_Bottom.store(0, std::memory_order_relaxed);
	}

	// Called only from an owner thread
	void Push(T NewElement)
	{
		//if (NewElement == {}) return;

		const size_t Bottom = _Bottom.load(std::memory_order_relaxed);
		const size_t Top = _Top.load(std::memory_order_acquire);
		auto Elements = _Elements.load(std::memory_order_relaxed);

		const auto Mask = _Capacity - 1;

		// Resize a queue if it is full.
		// The last cell in the array remains unused for possible extension according to Chase & Lev paper p.4.1, when a full array indicates an empty array.
		if (Bottom >= Top + _Capacity - 1)
		{
			// TODO: if (Bottom == std::numeric_limits<size_t>().max()) - how to reset?!

			// Allocate a new buffer twice as big, its capacity is also a power of 2
			const auto NewCapacity = _Capacity << 1;
			const auto NewMask = NewCapacity - 1;
			auto NewElements = new std::atomic<T>[NewCapacity];

			//???FIXME: why elements need to be atomic?! who might read their values when being stored? is this because of resizing?
			// Copy elements to the same indices in the new array. Other threads still see an old array as the source for Pop and Steal.
			for (size_t i = Top; i < Bottom; ++i)
				NewElements[i & NewMask].store(Elements[i & Mask].load(std::memory_order_relaxed), std::memory_order_relaxed);

			// This prevents an allocator from reusing the same memory addresses and therefore guarantees that ...
			//!!!TODO: see p.4.3 Lemma 5, understand better!
			_Garbage.push_back(Elements);

			// Use a new array instead of an old one. Until other threads see this store, they will access a previous array which remains valid.
			// Old array doesn't contain our new record, but Bottom is stored with "release" and reading it will imply reading the new array address.
			Elements = NewElements;
			_Elements.store(NewElements, std::memory_order_relaxed);
		}

		//???FIXME: why elements need to be atomic?! who might read their values when being stored? is this because of resizing?
		Elements[Bottom & Mask].store(NewElement, std::memory_order_relaxed);

		// Publish a new element with store-release, so that Steal() can see it
		std::atomic_thread_fence(std::memory_order_release);  // Like in the original paper
		_Bottom.store(Bottom + 1, std::memory_order_relaxed); // Like in the original paper
		//_Bottom.store(Bottom + 1, std::memory_order_release);   // Like in https://github.com/apache/brpc/blob/master/src/bthread/work_stealing_queue.h#L79
	}

	// LIFO. Called only from an owner thread.
	T Pop()
	{
		const size_t Bottom = _Bottom.load(std::memory_order_relaxed);
		size_t Top = _Top.load(std::memory_order_relaxed);

		// Top may be out of date here but then it will be reloaded with sequentially consistent load-acquire below.
		// Only an actual Top value may trigger this early exit condition because Top never decrements.
		if (Top >= Bottom) return {};

		auto Elements = _Elements.load(std::memory_order_relaxed);

		// Notify concurrent steals that we want to take the bottommost element
		const size_t NewBottom = Bottom - 1;
		_Bottom.store(NewBottom, std::memory_order_relaxed);

//adding just a compiler barrier between the store to bottom = b and the read from long t = top is not enough,
//because the memory model explicitly allows that “Loads may be reordered with older stores to different locations”
//-
// Can be also this, but portability is not guaranteed:
//long b = m_bottom - 1;
//_InterlockedExchange(&m_bottom, b); // _Bottom.exchange?
//long t = m_top;
//-
// We have to use seq-cst order for operations on _bottom as well as _top to ensure
// that when two threads compete for the last item either one sees the updated _bottom
// (pop wins), or one sees the updated _top (steal wins).
//-
//it must be that two concurrent steal and
//take do not read “old” values of both bottom and top, where “old”
//could be defined as “older than the value known to the other party
//in coherence order”
//The presence of the two cumulative barriers in
//	steal and take on ARMv7 guarantee such a condition :
// if the take barrier is ordered before the barrier in steal, then the
//	program - order - previous write to bottom will be propagated to
//	the instance of steal;
// conversely, if the steal barrier is ordered before the barrier in
//	take, then value read by the program - order - previous read from
//	top will be propagated to the instance of take.
		std::atomic_thread_fence(std::memory_order_seq_cst);

//In contrast to the implementation of Steal(), this time around we need to ensure that we first decrement bottom before
//attempting to read top.Otherwise, concurrent Steal() operations could remove several jobs from the deque without Pop() noticing.
		Top = _Top.load(std::memory_order_relaxed); //???!!!this becomes seq-cst load-acquire and syncs with seq-cst CAS in a total order?

		T Value{};
		if (Top > NewBottom)
		{
			// The queue is empty, reset Bottom to the canonical empty state (Bottom == Top)
			_Bottom.store(Bottom, std::memory_order_relaxed);
		}
		else
		{
			//???!!!capacity isn't protected now!? prevoiusly it was stored inside an array and was obtained with pointer!
			//!!!FIXME: now Elements may not match _Capacity! And even if no problem here because of the same thread, Steal() may suffer!
			T Value = Elements[NewBottom & (_Capacity - 1)].load(std::memory_order_relaxed);

			if (Top == NewBottom)
			{
				// The queue contains the last element, must compete for it with Steal().
				// CAS failure means that we have lost the race and Value was stolen by another consumer.
				if (!_Top.compare_exchange_strong(Top, Top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
					Value = {};

				// Whether we won or lost the race, Top is now incremented, so Bottom decrement must be undone
				_Bottom.store(Bottom, std::memory_order_relaxed);
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
