// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <algorithm>
#include <cstdint>
#include <string>
#include <iostream>

#include "leveldb/comparator.h"
#include "leveldb/slice.h"
#include "util/logging.h"
#include "util/no_destructor.h"

namespace leveldb {

    Comparator::~Comparator() {}

    namespace {

        class MyComparatorImpl : public Comparator {
        public:
            MyComparatorImpl() {}

            virtual const char* Name() const { return "test.NumberComparator"; }
            virtual int Compare(const Slice& a, const Slice& b) const {
                return ToNumber(a) - ToNumber(b);
            }
            virtual void FindShortestSeparator(std::string* s, const Slice& l) const {
                ToNumber(*s);     // Check format
                ToNumber(l);      // Check format
            }
            virtual void FindShortSuccessor(std::string* key) const {
                ToNumber(*key);   // Check format
            }
        private:
            static int ToNumber(const Slice& x) {
                // Check that there are no extra characters.

                return std::stoi(x.ToString());
            }
        };

        class BytewiseComparatorImpl : public Comparator {
        public:
            BytewiseComparatorImpl() {}

            virtual const char *Name() const {
                return "leveldb.BytewiseComparator";
            }

            virtual int Compare(const Slice &a, const Slice &b) const {
                return a.compare(b);
            }

            // 什么作用呢？
            // 举个例子：
            // 1. 假设传进来两个 key: "the who" 和 "the whose"
            //    那么该函数会将 start 的值改为 “the who"
            // 2. 假设传进来两个 key: "the quick brown fox" 和 "the who"
            //    那么该函数会将 start 的值改为 “the r"
            // 3. 假设传进来两个 key: "the quick brown fox" 和 "the reason"
            //    那么该函数会将 start 的值改为 “the q"

            // 这么做的目的是什么呢？ 让 start 字符串长度尽可能的短，且不能小于原始的 start 值
            virtual void FindShortestSeparator(
                    std::string *start,
                    const Slice &limit) const {
                // Find length of common prefix
                size_t min_length = std::min(start->size(), limit.size());
                size_t diff_index = 0;
                while ((diff_index < min_length) &&
                       ((*start)[diff_index] == limit[diff_index])) {
                    diff_index++;
                }

                if (diff_index >= min_length) {
                    // Do not shorten if one string is a prefix of the other
                } else {
                    uint8_t diff_byte = static_cast<uint8_t>((*start)[diff_index]);
                    if (diff_byte < static_cast<uint8_t>(0xff) &&
                        diff_byte + 1 < static_cast<uint8_t>(limit[diff_index])) {
                        (*start)[diff_index]++;
                        start->resize(diff_index + 1);
                        assert(Compare(*start, limit) < 0);
                    }
                }
            }

            virtual void FindShortSuccessor(std::string *key) const {
                // Find first character that can be incremented
                size_t n = key->size();
                for (size_t i = 0; i < n; i++) {
                    const uint8_t byte = (*key)[i];
                    if (byte != static_cast<uint8_t>(0xff)) {
                        (*key)[i] = byte + 1;
                        key->resize(i + 1);
                        return;
                    }
                }
                // *key is a run of 0xffs.  Leave it alone.
            }
        };
    }  // namespace

    const Comparator *MyComparator() {
        static NoDestructor<MyComparatorImpl> singleton;
        return singleton.get();
    }

    const Comparator *BytewiseComparator() {
        static NoDestructor<BytewiseComparatorImpl> singleton;
        return singleton.get();
    }

}  // namespace leveldb
