/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMING_BLOCKING_COLLECTION_H
#define HLS_STREAMING_BLOCKING_COLLECTION_H

#include "debug_message.h"

#include <cstddef>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <pplx/pplxtasks.h>

namespace hls {
	namespace streamer {

		template<class T>
		class blocking_collection
		{
			std::queue<T> queue_;			 // data storage container
			std::size_t max_size_;			 // max queue size
			std::atomic<std::size_t> count_; // current queue size
			std::atomic<bool> is_completed_; // no more item will be added to the queue

			// synchronization
			mutable std::mutex mutex_;		// mutable for using in const functions
			std::condition_variable full_;  // to wait free place to add item to queue
			std::condition_variable empty_; // to wait available item in queue

		public:
			blocking_collection(std::size_t size)
				: max_size_(size)
				, count_(0)
				, is_completed_(false)
			{}
			~blocking_collection()
			{
				DEBUG_MESSAGE("blocking_collection::~blocking_collection(): deleted: " << queue_.size());
			}

			// try add item to queue. it will wait if queue is full
			void add(T item, pplx::cancellation_token ct)
			{
				if (ct.is_canceled()) {
					DEBUG_MESSAGE("blocking_collection::add(): canceled");
					pplx::cancel_current_task();
				}
				if (is_completed())	{
					DEBUG_MESSAGE("blocking_collection::add(): invalid operation");
					throw pplx::invalid_operation();
				}

				while (count_ == max_size_)
				{
					if (ct.is_canceled()) {
						DEBUG_MESSAGE("blocking_collection::add(): canceled");
						pplx::cancel_current_task();
					}
					// wait for free space
					std::unique_lock<std::mutex> lock(mutex_);
					full_.wait_for(lock, std::chrono::milliseconds(250));
				}
				queue_.push(std::move(item));
				//DEBUG_MESSAGE("item - added");

				++count_;
				if (count_ == 1) {
					empty_.notify_one();
				}
			}

			// try take item from queue. it will wait if the queue is empty
			T take(pplx::cancellation_token ct)
			{
				if (ct.is_canceled()) {
					DEBUG_MESSAGE("blocking_collection::take(): canceled");
					pplx::cancel_current_task();
				}
				if (is_completed() && count_ == 0 && queue_.size() == 0) {
					DEBUG_MESSAGE("blocking_collection::take(): invalid operation");
					throw pplx::invalid_operation();
				}

				while (count_ == 0)
				{
					if (ct.is_canceled()) {
						DEBUG_MESSAGE("blocking_collection::take(): canceled");
						pplx::cancel_current_task();
					}
					// wait while queue is empty
					std::unique_lock<std::mutex> lock(mutex_);
					empty_.wait_for(lock, std::chrono::milliseconds(250));
				}
				T item = std::move(queue_.front());
				queue_.pop();
				//DEBUG_MESSAGE("item - taken");

				--count_;
				if (count_ == max_size_ - 1) {
					full_.notify_one();
				}
				return item;
			}

			// no more item will be added
			// add() and take() will throw invalid_operation exception
			void complete_adding()
			{
				std::lock_guard<std::mutex> lock(mutex_);
				is_completed_ = true;
			}

			// true if adding is completed
			bool is_completed() const
			{
				std::lock_guard<std::mutex> lock(mutex_);
				return is_completed_;
			}

			// number of items in the queue
			std::size_t count() const
			{
				std::lock_guard<std::mutex> lock(mutex_);
				return count_;
			}

			// storage queue size()
			std::size_t size() const
			{
				std::lock_guard<std::mutex> lock(mutex_);
				return queue_.size();
			}
		};
	}
}

#endif