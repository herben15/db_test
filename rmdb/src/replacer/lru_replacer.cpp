/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "lru_replacer.h"


/*
此模块作用是实现LRU策略替代页，因为需要对公共资源LRUlist和LRUhash写入，因此需要加锁。victim用来删除页并返回页id，注意LRUlist为空
时的操作。这两个数据结构只是为了替换页所存在的，并不影响页的使用。因此在pin中锁定页不让其被替换，就只要在这两链表上进行删除即可。
同理unpin的作用是让某一页可以被替换，其实就是将此页加到两链表中，但要注意在LRUlist中不能存在相同的页。
*/

LRUReplacer::LRUReplacer(size_t num_pages) { max_size_ = num_pages; }

LRUReplacer::~LRUReplacer() = default;  

/**
 * @description: 使用LRU策略删除一个victim frame，并返回该frame的id
 * @param {frame_id_t*} frame_id 被移除的frame的id，如果没有frame被移除返回nullptr
 * @return {bool} 如果成功淘汰了一个页面则返回true，否则返回false
 */
bool LRUReplacer::victim(frame_id_t* frame_id) {
    // C++17 std::scoped_lock
    // 它能够避免死锁发生，其构造函数能够自动进行上锁操作，析构函数会对互斥量进行解锁操作，保证线程安全。
    std::lock_guard<std::mutex> guard{latch_};  //  如果编译报错可以替换成其他lock

    // Todo:
    //  利用lru_replacer中的LRUlist_,LRUHash_实现LRU策略
    //  选择合适的frame指定为淘汰页面,赋值给*frame_id
	if(LRUlist_.empty()){
		frame_id = nullptr;
		return false;
	}
	*frame_id = LRUlist_.back();
	if(!LRUhash_.erase(*frame_id))
		return false;
	LRUlist_.pop_back();
    return true;
}

/**
 * @description: 固定指定的frame，即该页面无法被淘汰
 * @param {frame_id_t} 需要固定的frame的id
 */
void LRUReplacer::pin(frame_id_t frame_id) {
    std::lock_guard<std::mutex> guard{latch_};
    // Todo:
    // 固定指定id的frame
    // 在数据结构中移除该frame
	auto it = LRUhash_.find(frame_id);
	if(it!=LRUhash_.end()){
		LRUhash_.erase(it->first);
		LRUlist_.erase(it->second);
	}
}

/**
 * @description: 取消固定一个frame，代表该页面可以被淘汰
 * @param {frame_id_t} frame_id 取消固定的frame的id
 */
void LRUReplacer::unpin(frame_id_t frame_id) {
    // Todo:
    //  支持并发锁
    //  选择一个frame取消固定
	std::lock_guard<std::mutex> guard{latch_};
	if(LRUlist_.size()>=max_size_)
		return;
	for(auto &it:LRUlist_){
		if(it==frame_id)
			return;
	}
	LRUlist_.push_front(frame_id);
	LRUhash_[frame_id]=LRUlist_.begin();
}

/**
 * @description: 获取当前replacer中可以被淘汰的页面数量
 */
size_t LRUReplacer::Size() { return LRUlist_.size(); }
