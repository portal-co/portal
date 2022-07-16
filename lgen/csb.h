#include <iostream>
#include <sstream>
#include <vector>

template <typename B1, typename B2>
class cat_streambuf : public std::streambuf {
    B1* _sb1;
    B2* _sb2;
    std::vector<char> _buf;
    bool _insb1 = true;

  public:
    cat_streambuf(B1* sb1, B2* sb2) : _sb1(sb1), _sb2(sb2), _buf(1024) {}

    int underflow() {
        if (gptr() == egptr()) {
            auto size = [this] {
                if (_insb1) {
                    if (auto size = _sb1->sgetn(_buf.data(), _buf.size()))
                        return size;
                    _insb1 = false;
                }

                return _sb2->sgetn(_buf.data(), _buf.size());
            }();

            setg(_buf.data(), _buf.data(), _buf.data() + size);
        }
        return gptr() == egptr() 
            ? std::char_traits<char>::eof()
            : std::char_traits<char>::to_int_type(*gptr());
    }
};