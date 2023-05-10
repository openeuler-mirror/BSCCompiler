/*
 * (c) Copyright 2019 by Solid Sands B.V.,
 *     Amsterdam, the Netherlands. All rights reserved.
 *     Subject to conditions in the RESTRICTIONS file.
 * (c) Copyright 2007 ACE Associated Computer Experts bv
 * (c) Copyright 2007 ACE Associated Compiler Experts bv
 * All rights reserved.  Subject to conditions in RESTRICTIONS file.
 */
long long
test_it(int x)
{
	return x < 32 ? 1ll << x : 0ll;
}

int main() {
  printf("%d\n", test_it(3));
}

