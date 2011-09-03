// System
#include <stdio.h>

// Module to test
#include "TileIndex.h"

void test_index_containing(double min, double max)
{
  TileIndex ti = TileIndex::index_containing(min, max);
  tassert(ti.contains_time(min));
  tassert(ti.contains_time(max));
  tassert(!ti.left_child().contains_time(max));
  tassert(!ti.right_child().contains_time(min));
  tassert(ti.is_ancestor_of(ti.left_child()));
  tassert(ti.is_ancestor_of(ti.right_child()));
  tassert(!ti.left_child().is_ancestor_of(ti));
  tassert(!ti.right_child().is_ancestor_of(ti));
  tassert(ti.left_child().sibling() == ti.right_child());
  tassert(ti.right_child().sibling() == ti.left_child());
}

int main(int argc, char **argv)
{
  TileIndex ti(-3, 55);
  tassert(ti.level == -3);
  tassert(ti.offset == 55);
  tassert(ti == ti);
  tassert(!(ti != ti));
  tassert(!(ti < ti));

  // Test ==, !=, <
  TileIndex ti2 = ti;
  tassert(ti2.level == -3);
  tassert(ti2.offset == 55);
  tassert(ti2 == ti);
  tassert(!(ti2 != ti));
  tassert(!(ti2 < ti));

  ti2.offset = 54;
  tassert(!(ti == ti2));
  tassert(ti != ti2);
  tassert(ti2 < ti);
  tassert(!(ti < ti2));

  ti2 = ti;
  tassert(ti == ti2);
  ti2.level = -4;
  tassert(!(ti == ti2));
  tassert(ti != ti2);
  tassert(ti2 < ti);
  tassert(!(ti < ti2));

  ti2.level = -4;
  ti2.offset = 56;
  tassert(!(ti == ti2));
  tassert(ti != ti2);
  tassert(ti2 < ti);
  tassert(!(ti < ti2));

  // Test index_at_level_containing
  
  tassert(TileIndex::index_at_level_containing(0, 0.0)        == TileIndex(0, 0));
  tassert(TileIndex::index_at_level_containing(0, 0.001)      == TileIndex(0, 0));
  tassert(TileIndex::index_at_level_containing(0, 0.9999999)  == TileIndex(0, 0));
  tassert(TileIndex::index_at_level_containing(0, 1.0)        == TileIndex(0, 1));
  tassert(TileIndex::index_at_level_containing(0, -0.0000001) == TileIndex(0,-1));
  tassert(TileIndex::index_at_level_containing(0, -1)         == TileIndex(0,-1));
  tassert(TileIndex::index_at_level_containing(0, -1.0000001) == TileIndex(0,-2));
  tassert(TileIndex::index_at_level_containing(0, 10)         == TileIndex(0,10));

  tassert(TileIndex::index_at_level_containing(1, 0.0)        == TileIndex(1,0));
  tassert(TileIndex::index_at_level_containing(1, 0.001)      == TileIndex(1, 0));
  tassert(TileIndex::index_at_level_containing(1, 1.9999999)  == TileIndex(1, 0));
  tassert(TileIndex::index_at_level_containing(1, 2.0)        == TileIndex(1, 1));
  tassert(TileIndex::index_at_level_containing(1, -0.0000001) == TileIndex(1,-1));
  tassert(TileIndex::index_at_level_containing(1, -2)         == TileIndex(1,-1));
  tassert(TileIndex::index_at_level_containing(1, -2.0000001) == TileIndex(1,-2));
  tassert(TileIndex::index_at_level_containing(1, 20)         == TileIndex(1,10));

  tassert(TileIndex::index_at_level_containing(-1, 0.0)         == TileIndex(-1, 0));
  tassert(TileIndex::index_at_level_containing(-1, 0.001)       == TileIndex(-1, 0));
  tassert(TileIndex::index_at_level_containing(-1, 0.499999999) == TileIndex(-1, 0));
  tassert(TileIndex::index_at_level_containing(-1, 0.5)         == TileIndex(-1, 1));
  tassert(TileIndex::index_at_level_containing(-1, -0.0000001)  == TileIndex(-1,-1));
  tassert(TileIndex::index_at_level_containing(-1, -0.5)        == TileIndex(-1,-1));
  tassert(TileIndex::index_at_level_containing(-1, -0.50000001) == TileIndex(-1,-2));
  tassert(TileIndex::index_at_level_containing(-1, 5)           == TileIndex(-1,10));

  tassert(TileIndex::index_at_level_containing(10, 0.0)           == TileIndex(10, 0));
  tassert(TileIndex::index_at_level_containing(10, 1.0)           == TileIndex(10, 0));
  tassert(TileIndex::index_at_level_containing(10, 1023.9999999)  == TileIndex(10, 0));
  tassert(TileIndex::index_at_level_containing(10, 1024)          == TileIndex(10, 1));
  tassert(TileIndex::index_at_level_containing(10, -0.0000001)    == TileIndex(10,-1));
  tassert(TileIndex::index_at_level_containing(10, -1024)         == TileIndex(10,-1));
  tassert(TileIndex::index_at_level_containing(10, -1024.0000001) == TileIndex(10,-2));
  tassert(TileIndex::index_at_level_containing(10, 10240)         == TileIndex(10,10));

  // ~1 microsecond per tile is level -20
  tassert(TileIndex::index_at_level_containing(-20,           0.0) == TileIndex(-20,                 0LL));
  tassert(TileIndex::index_at_level_containing(-20,  2000000000.0) == TileIndex(-20,  2097152000000000LL));
  tassert(TileIndex::index_at_level_containing(-20, -2000000000.0) == TileIndex(-20, -2097152000000000LL));

  // ~1 microsecond per tile is level -20
  tassert(TileIndex::index_at_level_containing(-20,           0.0) == TileIndex(-20,                 0LL));
  tassert(TileIndex::index_at_level_containing(-20,  2000000000.0) == TileIndex(-20,  2097152000000000LL));
  tassert(TileIndex::index_at_level_containing(-20, -2000000000.0) == TileIndex(-20, -2097152000000000LL));

  // ~1 nanosecond per tile is level -30
  tassert(TileIndex::index_at_level_containing(-30,           0.0) == TileIndex(-30,                 0LL));
  tassert(TileIndex::index_at_level_containing(-30,  2000000000.0) == TileIndex(-30,  2147483648000000000LL));
  tassert(TileIndex::index_at_level_containing(-30, -2000000000.0) == TileIndex(-30, -2147483648000000000LL));

  // Test contains_time
  tassert( TileIndex(0,0).contains_time(0));
  tassert( TileIndex(0,0).contains_time(0.99999999));
  tassert(!TileIndex(0,0).contains_time(1.0));
  tassert(!TileIndex(0,0).contains_time(-0.00000001));

  tassert( TileIndex(0,1).contains_time(1.0));
  tassert( TileIndex(0,1).contains_time(1.99999999));
  tassert(!TileIndex(0,1).contains_time(2.0));
  tassert(!TileIndex(0,1).contains_time(0.99999999));

  tassert( TileIndex(0,-1).contains_time(-1.0));
  tassert( TileIndex(0,-1).contains_time(-0.0000001));
  tassert(!TileIndex(0,-1).contains_time(0.0));
  tassert(!TileIndex(0,-1).contains_time(-1.0000001));
  
  // Test parents and children

  //tassert(TileIndex(0,-3).parent() == TileIndex(1, -2));
  //tassert(TileIndex(0,-2).parent() == TileIndex(1, -1));
  tassert(TileIndex(0,-1).parent() == TileIndex(1, -1));
  tassert(TileIndex(0,0).parent()  == TileIndex(1,  0));
  tassert(TileIndex(0,1).parent()  == TileIndex(1,  0));
  tassert(TileIndex(0,2).parent()  == TileIndex(1,  1));

  tassert(TileIndex(-1,0).parent() == TileIndex(0, 0));
  tassert(TileIndex(0,10).parent() == TileIndex(1, 5));
  tassert(TileIndex(0,11).parent() == TileIndex(1, 5));
  
  tassert(TileIndex(0,0).left_child() == TileIndex(-1,0));
  tassert(TileIndex(0,0).right_child() == TileIndex(-1,1));

  tassert(TileIndex(0,-1).left_child() == TileIndex(-1,-2));
  tassert(TileIndex(0,-1).right_child() == TileIndex(-1,-1));
  
  tassert(TileIndex(-30,  2147483648000000000LL).parent() == TileIndex(-29, 1073741824000000000LL));
  tassert(TileIndex(-30,  2147483648000000000LL).left_child() == TileIndex(-31, 4294967296000000000LL));
  tassert(TileIndex(-30,  2147483648000000000LL).right_child() == TileIndex(-31, 4294967296000000001LL));

  // Test is_ancestor_of
  tassert(!TileIndex(0,0).is_ancestor_of(TileIndex(-1, -2)));
  tassert(!TileIndex(0,0).is_ancestor_of(TileIndex(-1, -1)));
  tassert( TileIndex(0,0).is_ancestor_of(TileIndex(-1,  0)));
  tassert( TileIndex(0,0).is_ancestor_of(TileIndex(-1,  1)));
  tassert(!TileIndex(0,0).is_ancestor_of(TileIndex(-1,  2)));
  tassert(!TileIndex(0,0).is_ancestor_of(TileIndex(-1,  3)));

  tassert(!TileIndex(0,-1).is_ancestor_of(TileIndex(-1, -4)));
  tassert(!TileIndex(0,-1).is_ancestor_of(TileIndex(-1, -3)));
  tassert( TileIndex(0,-1).is_ancestor_of(TileIndex(-1, -2)));
  tassert( TileIndex(0,-1).is_ancestor_of(TileIndex(-1, -1)));
  tassert(!TileIndex(0,-1).is_ancestor_of(TileIndex(-1,  0)));
  tassert(!TileIndex(0,-1).is_ancestor_of(TileIndex(-1,  1)));

  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex(11,  50)));
  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex(10, 100)));

  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex( 9, 198)));
  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex( 9, 199)));
  tassert( TileIndex(10,100).is_ancestor_of(TileIndex( 9, 200)));
  tassert( TileIndex(10,100).is_ancestor_of(TileIndex( 9, 201)));
  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex( 9, 202)));
  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex( 9, 203)));
  
  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex( 8, 398)));
  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex( 8, 399)));
  tassert( TileIndex(10,100).is_ancestor_of(TileIndex( 8, 400)));
  tassert( TileIndex(10,100).is_ancestor_of(TileIndex( 8, 401)));
  tassert( TileIndex(10,100).is_ancestor_of(TileIndex( 8, 402)));
  tassert( TileIndex(10,100).is_ancestor_of(TileIndex( 8, 403)));
  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex( 8, 404)));
  tassert(!TileIndex(10,100).is_ancestor_of(TileIndex( 8, 405)));

  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex(11,  -50)));
  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex(10, -100)));

  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex( 9, -197)));
  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex( 9, -198)));
  tassert( TileIndex(10,-100).is_ancestor_of(TileIndex( 9, -199)));
  tassert( TileIndex(10,-100).is_ancestor_of(TileIndex( 9, -200)));
  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex( 9, -201)));
  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex( 9, -202)));
  
  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex( 8, -395)));
  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex( 8, -396)));
  tassert( TileIndex(10,-100).is_ancestor_of(TileIndex( 8, -397)));
  tassert( TileIndex(10,-100).is_ancestor_of(TileIndex( 8, -398)));
  tassert( TileIndex(10,-100).is_ancestor_of(TileIndex( 8, -399)));
  tassert( TileIndex(10,-100).is_ancestor_of(TileIndex( 8, -400)));
  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex( 8, -401)));
  tassert(!TileIndex(10,-100).is_ancestor_of(TileIndex( 8, -402)));
  
  // Test index_containing
  test_index_containing(0.0,1.0);
  test_index_containing(-1.0,-0.000001);

  test_index_containing(1309792268, 1309792268+1e-6);
  test_index_containing(1309792268, 1309792268+1e-3);
  test_index_containing(1309792268, 1309792268+1e0);
  test_index_containing(1309792268, 1309792268+1e+3);
  test_index_containing(1309792268, 1309792268+1e+6);
  test_index_containing(1309792268, 1309792268+1e+9);

  test_index_containing(-1309792268, -1309792268+1e-6);
  test_index_containing(-1309792268, -1309792268+1e-3);
  test_index_containing(-1309792268, -1309792268+1e0);
  test_index_containing(-1309792268, -1309792268+1e+3);
  test_index_containing(-1309792268, -1309792268+1e+6);
  test_index_containing(-1309792268, -1309792268+1e+9);

  // Done
  fprintf(stderr, "Tests succeeded\n");
  return 0;
}
