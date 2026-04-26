#pragma once

#include "Array.hpp"
#include "Base.hpp"

template<typename T, typename Compare>
usize Partition(T* sort, usize lowIndex, usize highIndex, const Compare& compare)
{
	const usize middleIndex = lowIndex + (highIndex - lowIndex) / 2;
	if (compare(sort[middleIndex], sort[lowIndex]))
	{
		Swap(sort[lowIndex], sort[middleIndex]);
	}
	if (compare(sort[highIndex], sort[lowIndex]))
	{
		Swap(sort[lowIndex], sort[highIndex]);
	}
	if (compare(sort[highIndex], sort[middleIndex]))
	{
		Swap(sort[middleIndex], sort[highIndex]);
	}
	Swap(sort[middleIndex], sort[highIndex]);

	usize pivotIndex = lowIndex;
	for (usize scanIndex = lowIndex; scanIndex < highIndex; ++scanIndex)
	{
		if (compare(sort[scanIndex], sort[highIndex]))
		{
			Swap(sort[pivotIndex], sort[scanIndex]);
			++pivotIndex;
		}
	}
	Swap(sort[pivotIndex], sort[highIndex]);

	return pivotIndex;
}

template<typename T, typename Compare>
void QuickSort(T* sort, usize lowIndex, usize highIndex, const Compare& compare)
{
	if (highIndex <= lowIndex)
	{
		return;
	}

	const usize pivotIndex = Partition(sort, lowIndex, highIndex, compare);
	if (pivotIndex > lowIndex)
	{
		QuickSort(sort, lowIndex, pivotIndex - 1, compare);
	}
	if (pivotIndex < highIndex)
	{
		QuickSort(sort, pivotIndex + 1, highIndex, compare);
	}
}

template<typename T, typename Compare>
void Sort(T* sort, usize sortCount, const Compare& compare)
{
	if (sortCount != 0)
	{
		CHECK(sort);
	}

	if (sortCount <= 1)
	{
		return;
	}

	QuickSort(sort, 0, sortCount - 1, compare);
}

template<typename T>
void Sort(T* sort, usize sortCount)
{
	Sort(sort, sortCount, [](const T& a, const T& b) { return a < b; });
}

template<typename T, typename Compare>
void Sort(Array<T>* sort, const Compare& compare)
{
	CHECK(sort);
	Sort(sort->GetData(), sort->GetCount(), compare);
}

template<typename T>
void Sort(Array<T>* sort)
{
	CHECK(sort);
	Sort(sort->GetData(), sort->GetCount(), [](const T& a, const T& b) { return a < b; });
}

template<typename T, typename Compare>
void SortStable(T* sort, usize sortCount, Allocator* allocator, const Compare& compare)
{
	CHECK(allocator);
	if (sortCount != 0)
	{
		CHECK(sort);
	}

	if (sortCount <= 1)
	{
		return;
	}

	Array<usize> order(allocator);
	order.AddUninitialized(sortCount);
	for (usize orderIndex = 0; orderIndex < sortCount; ++orderIndex)
	{
		order[orderIndex] = orderIndex;
	}

	QuickSort(order.GetData(), 0, sortCount - 1, [sort, compare](usize a, usize b)
	{
		return compare(sort[a], sort[b]) || (!compare(sort[b], sort[a]) && a < b);
	});

	Array<T> sorted(sortCount, allocator);
	for (usize orderIndex = 0; orderIndex < sortCount; ++orderIndex)
	{
		sorted.Emplace(Move(sort[order[orderIndex]]));
	}

	for (usize sortIndex = 0; sortIndex < sortCount; ++sortIndex)
	{
		sort[sortIndex] = Move(sorted[sortIndex]);
	}
}

template<typename T>
void SortStable(T* sort, usize sortCount)
{
	SortStable(sort, sortCount, &GlobalAllocator::Get(), [](const T& a, const T& b) { return a < b; });
}

template<typename T>
void SortStable(T* sort, usize sortCount, Allocator* allocator)
{
	SortStable(sort, sortCount, allocator, [](const T& a, const T& b) { return a < b; });
}

template<typename T>
void SortStable(Array<T>* sort)
{
	SortStable(sort->GetData(), sort->GetCount());
}

template<typename T>
void SortStable(Array<T>* sort, Allocator* allocator)
{
	SortStable(sort->GetData(), sort->GetCount(), allocator);
}

template<typename T, typename Compare>
void SortStable(Array<T>* sort, Allocator* allocator, const Compare& compare)
{
	SortStable(sort->GetData(), sort->GetCount(), allocator, compare);
}
