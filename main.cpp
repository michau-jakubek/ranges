#include <iostream>
#include <numeric>
#include "view.hpp"

using namespace views;

struct point
{
	int x,y,z;
	point() : x(640), y(480), z(32) {}
	point(int x_, int y_) : x(x_), y(y_), z(32) {}
	point(int x_, int y_, int z_) : x(x_), y(y_) , z(z_) {}
	friend std::ostream& operator<<(std::ostream& str, const point& p) {
		return str << "{ " << p.x << ", " << p.y << ", " << p.z << " }";
	}
};

void test_where_and_select()
{
	std::vector<int> source(20);
	std::iota(source.begin(), source.end(), 1);

	auto even = [](auto&& a){ return 0 == (a % 2); };
	auto divisibleBy3 = [](auto& b){ return 0 == (b % 3); };
	auto pwr2 = [](const int& x) { return x*x; };

	auto v = source | where(even) | where(divisibleBy3)
		| selecting(selector<point>(), splaceholder(), splaceholder(pwr2), 256);

	std::copy(v.begin(), v.end(), std::ostream_iterator<point>(std::cout, "\n"));
}

int main()
{
	test_where_and_select();
	return 0;
}

