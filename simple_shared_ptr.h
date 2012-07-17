#ifndef SIMPLE_SHARED_PTR_H
#define SIMPLE_SHARED_PTR_H

template <class E>
class simple_shared_ptr {
public:
  simple_shared_ptr() : refcnt(NULL), ptr(NULL) {}
  simple_shared_ptr(E *p) : refcnt(NULL), ptr(NULL) {
    ref(p);
  }
  simple_shared_ptr(const simple_shared_ptr &rhs) : refcnt(NULL), ptr(NULL) {
    (*this) = rhs;
  }
  simple_shared_ptr<E> &operator=(const simple_shared_ptr &rhs) {
    if (refcnt != rhs.refcnt) {
      unref();
      refcnt = rhs.refcnt;
      if (refcnt) (*refcnt)++;
      ptr = rhs.ptr;
    }
    return *this;
  }
  void reset(E *p) {
    unref();
    ref(p);
  }
  E *get() {
    return ptr;
  }
  E *operator->() {
    return ptr;
  }
  E &operator*() {
    return *ptr;
  }
  ~simple_shared_ptr() {
    unref();
  }
private:
  int *refcnt;
  E *ptr;
  void ref(E *p) {
    refcnt = new int(1);
    ptr = p;
  }
  void unref() {
    if (refcnt) {
      if (--(*refcnt) == 0) {
        delete refcnt;
        delete ptr;
      }
      refcnt = NULL;
      ptr = NULL;
    }
  }
};

#endif
