#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

namespace lotus {

class noncopyable {
//inherited and used by the sub-class only
protected:
	noncopyable() = default; //default constructor
	~noncopyable() = default;

private:
	noncopyable(const noncopyable&) = delete; //non-copyable
	noncopyable& operator=(const noncopyable&) = delete; //preventing assignment
	//noncopyable(noncopyable &&) = delete;
	//noncopyable& operator=(noncopyable&&) = delete;

};

}

#endif
