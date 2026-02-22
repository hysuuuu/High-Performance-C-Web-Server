#pragma once

#include <deque>
#include <memory>
#include <unordered_set>

class Connection;

struct Entry {
    explicit Entry(const std::weak_ptr<Connection>& conn);
    ~Entry();

    std::weak_ptr<Connection> conn_;
};

using EntryPtr = std::shared_ptr<Entry>;
using WeakEntryPtr = std::weak_ptr<Entry>;
using Bucket = std::unordered_set<EntryPtr>;
using TimingWheel = std::deque<Bucket>;
