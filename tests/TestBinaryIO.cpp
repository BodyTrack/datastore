// Local
#include "DataSample.h"
#include "Tile.h"

// Module to test
#include "BinaryIO.h"

struct Foo {
  int a,b;
  Foo() {}
  Foo(int a_init, int b_init) : a(a_init), b(b_init) {}
  bool operator==(const Foo &rhs) const { return a==rhs.a && b==rhs.b; }
};

template <class T>
void test_io(T a)
{
  T b;
  std::vector<unsigned char> buf(BinaryWriter::write_length(a));
  
  BinaryWriter writer(&buf[0], &buf[buf.size()]);
  writer.write(a);
  tassert(writer.eof());

  BinaryReader reader(&buf[0], &buf[buf.size()]);
  reader.read(b);
  tassert(reader.eof());
  tassert(a == b);
}

int main(int argc, char **argv)
{
  // Scalars
  test_io(12345678);
  test_io(-12345678);
  test_io(1234567890123LL);
  test_io((double)1234.5678);
  test_io((float)1234.5678);

  // String
  test_io(std::string("abcdefghij"));

  // Struct
  test_io(Foo(123, 456));

  {
    // Ranges
    DataRanges ranges;
    ranges.times.add(333);
    ranges.times.add(444);
    ranges.double_samples.add(10);
    ranges.double_samples.add(20);
    test_io(ranges);
  }

  {
    // vector
    std::vector<double> vd;
    vd.push_back(123.456);
    vd.push_back(789.012);
    test_io(vd);
  }

  {
    // vector<DataSample<double> >
    std::vector<DataSample<double> > dsd;
    dsd.push_back(DataSample<double>(111.111, 3.1415, 1, 0));
    dsd.push_back(DataSample<double>(222.222, 6.6, 23.23, 12.21));
    test_io(dsd);
  }

  {
    // vector<DataSample<string> >
    std::vector<DataSample<std::string> > dss;
    dss.push_back(DataSample<std::string>(111.111, "3.1415", 1, 0));
    dss.push_back(DataSample<std::string>(222.222, "6.6", 23.23, 12.21));
    test_io(dss);
  }

  fprintf(stderr, "Tests succeeded\n");
  return 0;
}
