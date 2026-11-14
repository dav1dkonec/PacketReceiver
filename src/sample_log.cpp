#include "sample_log.h"

#include <stdexcept>

void SampleLog::write_csv(std::ostream& out) const
{
    out << "index,seq,user_ns\n";
    for (std::size_t i = 0; i < count_; ++i) {
        out << i << ',' << samples_[i].seq << ',' << samples_[i].user_ns << '\n';
    }

    // buffered data is written here: check after it (e.g. disk full)
    out.flush();
    if (!out) {
        throw std::runtime_error("error writing CSV output");
    }
}
