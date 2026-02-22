#include "TimingWheel.h"

#include "Connection.h"

Entry::Entry(const std::weak_ptr<Connection>& conn) : conn_(conn) {}

Entry::~Entry() {
    auto conn = conn_.lock();
    if (conn) {
        conn->disconnect();
    }
}
