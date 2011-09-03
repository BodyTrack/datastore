// Local
#include "utils.h"

// File to test
#include "Range.h"

int main(int argc, char **argv)
{
  Range all = Range::all();
  Range none = Range();
  tassert(none == Range::none());
  tassert(!(all == none));

  tassert(all.min == -std::numeric_limits<double>::max());
  tassert(all.max == std::numeric_limits<double>::max());
  
  tassert(all.includes(-std::numeric_limits<double>::max()));
  tassert(all.includes(-1));
  tassert(all.includes(0));
  tassert(all.includes(1));
  tassert(all.includes(std::numeric_limits<double>::max()));
  
  tassert(none.min == std::numeric_limits<double>::max());
  tassert(none.max == -std::numeric_limits<double>::max());

  tassert(!none.includes(-std::numeric_limits<double>::max()));
  tassert(!none.includes(-1));
  tassert(!none.includes(0));
  tassert(!none.includes(1));
  tassert(!none.includes(std::numeric_limits<double>::max()));

  Range a = all;
  a.add(-100);
  tassert(a == Range::all());
  
  Range b = none;
  b.add(-100);
  tassert(!(b == none));

  tassert(b.min == -100);
  tassert(b.max == -100);
  tassert(!b.includes(-std::numeric_limits<double>::max()));
  tassert(!b.includes(-101));
  tassert( b.includes(-100));
  tassert(!b.includes(-99));
  tassert(!b.includes(-1));
  tassert(!b.includes(0));
  tassert(!b.includes(1));
  tassert(!b.includes(std::numeric_limits<double>::max()));
  
  Range c = b;
  c.add(1000);
  tassert(!(b == c));
  
  tassert(c.min == -100);
  tassert(c.max == 1000);
  tassert(!c.includes(-std::numeric_limits<double>::max()));
  tassert(!c.includes(-101));
  tassert( c.includes(-100));
  tassert( c.includes(-99));
  tassert( c.includes(-1));
  tassert( c.includes(0));
  tassert( c.includes(1));
  tassert( c.includes(999));
  tassert( c.includes(1000));
  tassert(!c.includes(1001));
  tassert(!c.includes(std::numeric_limits<double>::max()));
  
  c.add(none);
  tassert(c == Range(-100, 1000));
  
  c.add(all);
  tassert(!(c == Range(-100, 1000)));
  tassert(c == Range::all());

  tassert(!Range(-100,100).empty());
  tassert(Range(100,-100).empty());

  tassert(!Range(100,200).intersects(Range(0,99.999)));
  tassert(Range(100,200).intersects(Range(0,100)));
  tassert(Range(100,200).intersects(Range(50,150)));
  tassert(Range(100,200).intersects(Range(100,200)));
  tassert(Range(100,200).intersects(Range(150,250)));
  tassert(Range(100,200).intersects(Range(200,300)));
  tassert(!Range(100,200).intersects(Range(200.01,300.01)));

  tassert(Range(100,200).intersects(Range(150, 150)));
  tassert(Range(100,200).intersects(Range(150, 151)));
  tassert(!Range(100,200).intersects(Range(151, 150)));

  tassert(!Range::none().intersects(Range::all()));
  tassert(!Range::all().intersects(Range::none()));

  return 0;
}
