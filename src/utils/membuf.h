#ifndef SERVER_UTILS_MEMBUF_H
#define SERVER_UTILS_MEMBUF_H

#include <cassert>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <type_traits>

namespace Server {
namespace Utils {

class membuf : public std::basic_streambuf<char> {
public:
    membuf(std::initializer_list<uint8_t> c) : _data(c.begin(), c.end()) {
        setg((char *)&_data.front(), (char *)&_data.front(), (char *)&_data.back() + 1);
    }
    membuf(const char *p, size_t l) { setg((char *)p, (char *)p, (char *)p + l); }

    inline std::streamsize data_size() const { return _data.size(); }

    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        assert(which == std::ios_base::in);

        char *npos = nullptr;
        char *_beg = eback();
        char *_cur = gptr();
        char *_end = egptr();
        switch (way) {
        case std::ios_base::beg:
            npos = _beg + off;
            break;
        case std::ios_base::cur:
            npos = _cur + off;
            break;
        case std::ios_base::end:
            npos = _end + off;
            break;
        default:
            throw std::runtime_error("Unknown direction");
        }

        if (_beg > npos || npos > _end) {
            return std::streampos(-1);
        }

        setg(_beg, npos, _end);
        return std::streampos(npos - _beg);
    }

private:
    std::vector<uint8_t> _data;
};

} // namespace Utils
} // namespace Server

#endif // SERVER_UTILS_MEMBUF_H
