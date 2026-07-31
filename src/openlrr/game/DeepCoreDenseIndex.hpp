// DeepCoreDenseIndex.hpp : O(alive) iteration over a ListSet-shaped container.
//
// THE PROBLEM
// -----------
// A ListSet stores items in a set of geometrically-sized lists: list i holds 2^i items, so a
// container with listCount lists has capacity 2^listCount - 1. Liveness is a property of the
// item itself -- a free item's `nextFree` points at itself -- so enumeration is a full walk of
// every allocated slot, testing each one.
//
// Two things make that expensive in this game rather than merely theoretical:
//
//   1. Capacity is MONOTONIC for the life of a level. Lists are pushed as needed and never
//      popped, so a map that momentarily peaked at 200 objects keeps paying 255 slots of walk
//      cost for the rest of the mission, however few objects survive.
//   2. LegoObject is 1036 bytes. The walk touches one pointer per item at a 1036-byte stride,
//      so it misses cache on essentially every slot, and the frame performs eight-plus of these
//      walks (see docs/PERFORMANCE.md).
//
// So the per-frame cost tracks the high-water mark of object count, not the current one. That
// is the property this class removes.
//
// THE DESIGN, AND WHY NOT A HASH MAP
// ----------------------------------
// A packed vector of live pointers gives O(alive) iteration, but removal then needs to find an
// item's position. The obvious answer is an unordered_map<TItem*, size_t>, which hashes on every
// add and remove and allocates nodes.
//
// It is not needed. A ListSet already defines a stable dense integer for every slot --
// ListSet::IndexOfInListSet(listIndex, itemIndex) -- in [0, capacity). So:
//
//   m_live   packed array of live pointers, iterated directly
//   m_where  slot id -> position in m_live, or -1 when not live
//
// Both are sized by capacity, allocated once, and reused. Add appends and records; Remove swaps
// the last element into the hole and fixes one entry. No hashing, no per-operation allocation,
// and iteration is a contiguous pointer array.
//
// CORRECTNESS DOES NOT DEPEND ON AN AUDIT
// ---------------------------------------
// The index is only valid if every creation and destruction passes through code that updates it.
// `GameState.cpp:1792-1795` records real doubt about exactly that, and an index that silently
// diverges would skip live objects -- far worse than the slowness it fixes.
//
// So this class is DIRTY BY DEFAULT and rebuildable. Any path that cannot be proven to maintain
// it calls MarkDirty(); the next iteration performs one full O(capacity) walk to resync and then
// resumes O(alive). Worst case is therefore exactly today's cost, and correctness never rests on
// an audit being complete -- only performance does. An audit can then retire dirty flags one at
// a time, each retirement a measurable improvement rather than a correctness gamble.
//
// Header-only and stdlib-only on purpose: tools/harness/ instantiates it over a synthetic
// 1036-byte item to test and benchmark it without the game.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>


namespace DeepCore
{; // !<---

namespace Detail
{; // !<---

/// Packed live-pointer index over a slot-addressed container.
///
/// TItem is only ever held as a pointer, so this compiles against an incomplete type and can be
/// instantiated over the real exe-overlaid item structs without including them.
template <typename TItem>
class DenseLiveIndex
{
public:
	static const std::size_t npos = (std::size_t)-1;

	/// Size the index for a container capacity. Safe to call repeatedly; growing preserves
	/// nothing and marks dirty, because slot ids are only meaningful for a fixed capacity.
	void Reserve(std::size_t capacity)
	{
		if (capacity == m_capacity) return;
		m_capacity = capacity;
		m_where.assign(capacity, -1);
		m_live.clear();
		m_live.reserve(capacity);
		m_dirty = true;
	}

	std::size_t Capacity() const { return m_capacity; }
	std::size_t Count() const    { return m_live.size(); }
	bool Dirty() const           { return m_dirty; }

	/// Force a resync on the next EnsureFresh. Cheap, and the safe thing to do whenever
	/// anything might have mutated the container behind our back.
	void MarkDirty() { m_dirty = true; }

	/// The packed live set. Only meaningful after EnsureFresh.
	const std::vector<TItem*>& Live() const { return m_live; }

	/// Record a newly live item at a known slot.
	/// Idempotent: adding an item already present is a no-op rather than a duplicate, because a
	/// duplicate would cause double-processing in every consumer.
	void OnAdd(std::size_t slot, TItem* item)
	{
		if (slot >= m_capacity || item == nullptr) { m_dirty = true; return; }
		if (m_where[slot] >= 0) return;
		m_where[slot] = (std::int32_t)m_live.size();
		m_live.push_back(item);
		m_slotOf.push_back(slot);   // kept in step with m_live; OnRemove needs it to fix the swap
	}

	/// Record an item becoming free. Swap-with-last, so removal is O(1) and iteration order is
	/// unspecified -- which is fine, because a ListSet walk's order is an artefact of the
	/// storage layout and no caller may rely on it.
	void OnRemove(std::size_t slot)
	{
		if (slot >= m_capacity) { m_dirty = true; return; }
		const std::int32_t pos = m_where[slot];
		if (pos < 0) return;

		const std::size_t last = m_live.size() - 1;
		const std::size_t at = (std::size_t)pos;

		if (at != last) {
			m_live[at] = m_live[last];
			// Fix the moved item's back-reference. Linear only over the small set of slots
			// that could hold it would be wrong; instead the caller-supplied slot of the moved
			// item is recovered from m_slotOf, kept in step with m_live.
			m_where[m_slotOf[last]] = (std::int32_t)at;
			m_slotOf[at] = m_slotOf[last];
		}
		m_live.pop_back();
		m_slotOf.pop_back();
		m_where[slot] = -1;
	}

	/// Begin a rebuild: clears the index and prepares to receive the full live set.
	void RebuildBegin()
	{
		m_live.clear();
		m_slotOf.clear();
		m_where.assign(m_capacity, -1);
	}

	/// Feed one live item during a rebuild.
	void RebuildAdd(std::size_t slot, TItem* item)
	{
		if (slot >= m_capacity || item == nullptr) return;
		if (m_where[slot] >= 0) return;
		m_where[slot] = (std::int32_t)m_live.size();
		m_live.push_back(item);
		m_slotOf.push_back(slot);
	}

	void RebuildEnd() { m_dirty = false; }

	/// True if the index currently believes this slot is live.
	bool IsTracked(std::size_t slot) const
	{
		return slot < m_capacity && m_where[slot] >= 0;
	}

	void Clear()
	{
		m_live.clear();
		m_slotOf.clear();
		m_where.assign(m_capacity, -1);
		m_dirty = true;
	}

private:
	std::size_t              m_capacity = 0;
	std::vector<TItem*>      m_live;     ///< packed live pointers
	std::vector<std::size_t> m_slotOf;   ///< m_live[i] came from this slot
	std::vector<std::int32_t> m_where;   ///< slot -> index into m_live, or -1
	bool                     m_dirty = true;
};

} // namespace Detail

} // namespace DeepCore
